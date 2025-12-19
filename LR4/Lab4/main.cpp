#include <iostream>
#include <windows.h>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <random>

// Вилки как мьютексы
std::vector<HANDLE> forks;

// Статистика
std::mutex statsMutex;
std::vector<int> successfulMeals;
std::vector<int> failedMeals;
std::vector<double> thinkingTime;
std::vector<double> eatingTime;
std::vector<double> waitingTime;

std::atomic<bool> isRunning(true);

thread_local std::mt19937 rng(std::random_device{}());

int getRandomDuration(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

enum class Strategy {
    SIMPLE,
    ASYMMETRIC,
    WITH_TIMEOUT
};

Strategy currentStrategy = Strategy::SIMPLE;

void philosopher(int id, int numPhilosophers, int minThinkTime, int maxThinkTime,
                 int minEatTime, int maxEatTime, int timeout) {
    int leftFork = id;
    int rightFork = (id + 1) % numPhilosophers;

    while (isRunning) {
        // Размышление
        auto thinkStart = std::chrono::high_resolution_clock::now();
        int thinkDuration = getRandomDuration(minThinkTime, maxThinkTime);
        std::this_thread::sleep_for(std::chrono::milliseconds(thinkDuration));
        auto thinkEnd = std::chrono::high_resolution_clock::now();

        {
            std::lock_guard<std::mutex> lock(statsMutex);
            thinkingTime[id] += std::chrono::duration<double>(thinkEnd - thinkStart).count();
            std::cout << "Philosopher " << id << " is thinking..." << std::endl;
        }

        auto waitStart = std::chrono::high_resolution_clock::now();
        bool gotForks = false;

        switch (currentStrategy) {
            case Strategy::SIMPLE: {
                // Простое - берем левую, потом правую вилку
                WaitForSingleObject(forks[leftFork], INFINITE);
                // deadlock may be here
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                {
                    std::lock_guard<std::mutex> lock(statsMutex);
                    std::cout << ">>> Philosopher " << id << " took fork " << leftFork
                            << " and is WAITING for fork " << rightFork << "..." << std::endl;
                }
                WaitForSingleObject(forks[rightFork], INFINITE);
                gotForks = true;
                break;
            }

            case Strategy::ASYMMETRIC: {
                // Асимметричное - четные философы берут сначала левую,
                // нечетные - сначала правую
                if (id % 2 == 0) {
                    WaitForSingleObject(forks[leftFork], INFINITE);
                    WaitForSingleObject(forks[rightFork], INFINITE);
                } else {
                    WaitForSingleObject(forks[rightFork], INFINITE);
                    WaitForSingleObject(forks[leftFork], INFINITE);
                }
                gotForks = true;
                break;
            }

            case Strategy::WITH_TIMEOUT: {
                // с тайм-аутом
                DWORD result = WaitForSingleObject(forks[leftFork], timeout);
                if (result == WAIT_OBJECT_0) {
                    result = WaitForSingleObject(forks[rightFork], timeout);
                    if (result == WAIT_OBJECT_0) {
                        gotForks = true;
                    } else {
                        // Не удалось взять правую вилку, освобождаем левую
                        ReleaseMutex(forks[leftFork]);
                        gotForks = false;
                    }
                } else {
                    gotForks = false;
                }
                break;
            }
        }

        auto waitEnd = std::chrono::high_resolution_clock::now();

        if (gotForks) {
            {
                std::lock_guard<std::mutex> lock(statsMutex);
                waitingTime[id] += std::chrono::duration<double>(waitEnd - waitStart).count();
                std::cout << "Philosopher " << id << " is eating (forks "
                        << leftFork << " and " << rightFork << ")" << std::endl;
            }

            // Eда
            auto eatStart = std::chrono::high_resolution_clock::now();
            int eatDuration = getRandomDuration(minEatTime, maxEatTime);
            std::this_thread::sleep_for(std::chrono::milliseconds(eatDuration));
            auto eatEnd = std::chrono::high_resolution_clock::now();

            // Освобождаем вилки
            ReleaseMutex(forks[leftFork]);
            ReleaseMutex(forks[rightFork]);

            {
                std::lock_guard<std::mutex> lock(statsMutex);
                successfulMeals[id]++;
                eatingTime[id] += std::chrono::duration<double>(eatEnd - eatStart).count();
                std::cout << "Philosopher " << id << " finished eating" << std::endl;
            }
        } else {
            std::lock_guard<std::mutex> lock(statsMutex);
            failedMeals[id]++;
            waitingTime[id] += std::chrono::duration<double>(waitEnd - waitStart).count();
            std::cout << "Philosopher " << id << " failed to get forks (timeout)" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void cleanup(int numPhilosophers) {
    for (int i = 0; i < numPhilosophers; ++i) {
        CloseHandle(forks[i]);
    }
}

int main() {
    int numPhilosophers = 5;
    int minThinkTime = 100;
    int maxThinkTime = 300;
    int minEatTime = 200;
    int maxEatTime = 500;
    int timeout = 100; // мс
    int simulationTime = 10; // секунды

    std::cout << "Dining Philosophers\n";
    std::cout << "Number of philosophers: " << numPhilosophers << std::endl;
    std::cout << "Strategy: ";
    switch (currentStrategy) {
        case Strategy::SIMPLE:
            std::cout << "Simple (may deadlock)\n";
            break;
        case Strategy::ASYMMETRIC:
            std::cout << "Asymmetric\n";
            break;
        case Strategy::WITH_TIMEOUT:
            std::cout << "With timeout (" << timeout << "ms)\n";
            break;
    }
    std::cout << "Simulation time: " << simulationTime << " seconds\n\n";

    forks.resize(numPhilosophers);
    successfulMeals.resize(numPhilosophers, 0);
    failedMeals.resize(numPhilosophers, 0);
    thinkingTime.resize(numPhilosophers, 0.0);
    eatingTime.resize(numPhilosophers, 0.0);
    waitingTime.resize(numPhilosophers, 0.0);

    for (int i = 0; i < numPhilosophers; ++i) {
        forks[i] = CreateMutex(NULL, FALSE, NULL);
    }

    // Создание потоков-философов
    std::vector<std::thread> threads;
    for (int i = 0; i < numPhilosophers; ++i) {
        threads.emplace_back(philosopher, i, numPhilosophers,
                             minThinkTime, maxThinkTime,
                             minEatTime, maxEatTime, timeout);
    }

    // Симуляция
    std::this_thread::sleep_for(std::chrono::seconds(simulationTime));
    isRunning = false;

    for (auto &th: threads) {
        th.join();
    }

    // Вывод результатов
    std::cout << "\nSimulation Results\n\n";

    int totalSuccessful = 0;
    int totalFailed = 0;
    double totalThinking = 0.0;
    double totalEating = 0.0;
    double totalWaiting = 0.0;

    for (int i = 0; i < numPhilosophers; ++i) {
        std::cout << "Philosopher " << i << ":\n";
        std::cout << "  Successful meals: " << successfulMeals[i] << std::endl;
        std::cout << "  Failed attempts: " << failedMeals[i] << std::endl;
        std::cout << "  Thinking time: " << thinkingTime[i] << " sec." << std::endl;
        std::cout << "  Eating time: " << eatingTime[i] << " sec." << std::endl;
        std::cout << "  Waiting time: " << waitingTime[i] << " sec." << std::endl;

        double totalTime = thinkingTime[i] + eatingTime[i] + waitingTime[i];
        if (totalTime > 0) {
            std::cout << "  Activity ratio: " << (eatingTime[i] / totalTime * 100) << "%" << std::endl;
            std::cout << "  Blocking ratio: " << (waitingTime[i] / totalTime * 100) << "%" << std::endl;
        }
        std::cout << std::endl;

        totalSuccessful += successfulMeals[i];
        totalFailed += failedMeals[i];
        totalThinking += thinkingTime[i];
        totalEating += eatingTime[i];
        totalWaiting += waitingTime[i];
    }

    std::cout << "Statistics\n";
    std::cout << "Total successful meals: " << totalSuccessful << std::endl;
    std::cout << "Average meals per philosopher: " << (totalSuccessful / (double) numPhilosophers) << std::endl;
    std::cout << "Total thinking time: " << totalThinking << " sec." << std::endl;
    std::cout << "Total eating time: " << totalEating << " sec." << std::endl;
    std::cout << "Total waiting time: " << totalWaiting << " sec." << std::endl;

    double totalTime = totalThinking + totalEating + totalWaiting;
    if (totalTime > 0) {
        std::cout << "Overall efficiency: " << (totalEating / totalTime * 100) << "%" << std::endl;
    }

    cleanup(numPhilosophers);
    return 0;
}
