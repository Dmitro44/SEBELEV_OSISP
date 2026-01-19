#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <string>
#include <sstream>

struct DataBlock {
    int id;
    int data[10];
    DWORD timestamp;
    char status[32];
};

// ПРОЦЕСС 1: ГЕНЕРАТОР ДАННЫХ
void GeneratorProcess() {
    std::cout << "[ГЕНЕРАТОР] Запуск процесса генерации данных\n";

    HANDLE hPipe = CreateNamedPipeA(
        "\\\\.\\pipe\\PipelineStage1",
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        sizeof(DataBlock) * 10,
        sizeof(DataBlock) * 10,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "[ГЕНЕРАТОР] Ошибка создания канала: " << GetLastError() << "\n";
        return;
    }

    std::cout << "[ГЕНЕРАТОР] Ожидание подключения обработчика...\n";

    if (!ConnectNamedPipe(hPipe, NULL)) {
        std::cerr << "[ГЕНЕРАТОР] Ошибка подключения: " << GetLastError() << "\n";
        CloseHandle(hPipe);
        return;
    }

    std::cout << "[ГЕНЕРАТОР] Обработчик подключен. Начало генерации данных.\n\n";

    int totalBlocks = 5;
    for (int blockId = 1; blockId <= totalBlocks; blockId++) {
        DataBlock block;
        block.id = blockId;
        block.timestamp = GetTickCount();
        strcpy_s(block.status, "generated");

        std::cout << "[ГЕНЕРАТОР] Блок #" << blockId << " | Данные: [";
        for (int i = 0; i < 10; i++) {
            block.data[i] = rand() % 100;
            std::cout << block.data[i];
            if (i < 9) std::cout << ", ";
        }
        std::cout << "]\n";

        DWORD written;
        if (!WriteFile(hPipe, &block, sizeof(DataBlock), &written, NULL)) {
            std::cerr << "[ГЕНЕРАТОР] Ошибка записи в канал\n";
            break;
        }

        Sleep(1000);
    }

    std::cout << "\n[ГЕНЕРАТОР] Генерация завершена. Закрытие канала.\n";
    CloseHandle(hPipe);
}

// ПРОЦЕСС 2: СОРТИРОВКА
void ProcessorProcess() {
    std::cout << "[ОБРАБОТЧИК] Запуск процесса обработки данных\n";

    Sleep(500);

    HANDLE hPipeIn = CreateFileA(
        "\\\\.\\pipe\\PipelineStage1",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipeIn == INVALID_HANDLE_VALUE) {
        std::cerr << "[ОБРАБОТЧИК] Ошибка подключения к входному каналу: " << GetLastError() << "\n";
        return;
    }

    std::cout << "[ОБРАБОТЧИК] Подключено к генератору\n";

    HANDLE hPipeOut = CreateNamedPipeA(
        "\\\\.\\pipe\\PipelineStage2",
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        sizeof(DataBlock) * 10,
        sizeof(DataBlock) * 10,
        0,
        NULL
    );

    if (hPipeOut == INVALID_HANDLE_VALUE) {
        std::cerr << "[ОБРАБОТЧИК] Ошибка создания выходного канала: " << GetLastError() << "\n";
        CloseHandle(hPipeIn);
        return;
    }

    std::cout << "[ОБРАБОТЧИК] Ожидание подключения визуализатора...\n";

    if (!ConnectNamedPipe(hPipeOut, NULL)) {
        std::cerr << "[ОБРАБОТЧИК] Ошибка подключения визуализатора: " << GetLastError() << "\n";
        CloseHandle(hPipeIn);
        CloseHandle(hPipeOut);
        return;
    }

    std::cout << "[ОБРАБОТЧИК] Визуализатор подключен. Начало обработки.\n\n";

    while (true) {
        DataBlock block;
        DWORD read;

        if (!ReadFile(hPipeIn, &block, sizeof(DataBlock), &read, NULL) || read == 0) {
            break;
        }

        std::cout << "[ОБРАБОТЧИК] Получен блок #" << block.id << "\n";
        std::cout << "              До сортировки: [";
        for (int i = 0; i < 10; i++) {
            std::cout << block.data[i];
            if (i < 9) std::cout << ", ";
        }
        std::cout << "]\n";

        std::sort(block.data, block.data + 10);
        strcpy_s(block.status, "sorted");

        std::cout << "              После сортировки: [";
        for (int i = 0; i < 10; i++) {
            std::cout << block.data[i];
            if (i < 9) std::cout << ", ";
        }
        std::cout << "]\n\n";

        DWORD written;
        if (!WriteFile(hPipeOut, &block, sizeof(DataBlock), &written, NULL)) {
            std::cerr << "[ОБРАБОТЧИК] Ошибка записи в выходной канал\n";
            break;
        }

        Sleep(500);
    }

    std::cout << "[ОБРАБОТЧИК] Обработка завершена. Закрытие каналов.\n";
    CloseHandle(hPipeIn);
    CloseHandle(hPipeOut);
}

// ПРОЦЕСС 3: ВИЗУАЛИЗАТОР
void VisualizerProcess() {
    std::cout << "[ВИЗУАЛИЗАТОР] Запуск процесса визуализации\n";

    Sleep(1000);

    HANDLE hPipe = CreateFileA(
        "\\\\.\\pipe\\PipelineStage2",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "[ВИЗУАЛИЗАТОР] Ошибка подключения к каналу: " << GetLastError() << "\n";
        return;
    }

    std::cout << "[ВИЗУАЛИЗАТОР] Подключено к обработчику. Ожидание данных.\n\n";

    int totalBlocks = 0;
    DWORD totalTime = 0;

    while (true) {
        DataBlock block;
        DWORD read;

        if (!ReadFile(hPipe, &block, sizeof(DataBlock), &read, NULL) || read == 0) {
            break;
        }

        totalBlocks++;
        DWORD processingTime = GetTickCount() - block.timestamp;
        totalTime += processingTime;

        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║ РЕЗУЛЬТАТ ОБРАБОТКИ БЛОКА #" << block.id << "                             ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Статус: " << block.status << "                                      ║\n";
        std::cout << "║ Отсортированные данные:                                    ║\n";
        std::cout << "║   ";

        for (int i = 0; i < 10; i++) {
            printf("%3d ", block.data[i]);
        }
        std::cout << "                          ║\n";

        std::cout << "║                                                            ║\n";
        std::cout << "║ Время обработки: " << processingTime << " мс";
        for (int i = 0; i < 34 - std::to_string(processingTime).length(); i++) std::cout << " ";
        std::cout << "║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

        Sleep(300);
    }

    std::cout << "\n[ВИЗУАЛИЗАТОР] Обработано блоков: " << totalBlocks << "\n";
    std::cout << "[ВИЗУАЛИЗАТОР] Среднее время обработки: " << (totalBlocks > 0 ? totalTime / totalBlocks : 0) <<
            " мс\n";

    CloseHandle(hPipe);
}


int main(int argc, char *argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    srand(static_cast<unsigned int>(time(NULL)));

    // Определение режима работы по аргументам командной строки
    if (argc > 1) {
        std::string mode = argv[1];

        if (mode == "generator") {
            GeneratorProcess();
        } else if (mode == "processor") {
            ProcessorProcess();
        } else if (mode == "visualizer") {
            VisualizerProcess();
        }

        system("pause");
        return 0;
    }

    std::cout << "═══════════════════════════════════════════════════════════════\n";
    std::cout << "  СИСТЕМА КОНВЕЙЕРНОЙ ОБРАБОТКИ ДАННЫХ (WinAPI)\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n\n";
    std::cout << "Архитектура конвейера:\n";
    std::cout << "  ГЕНЕРАТОР → [канал 1] → ОБРАБОТЧИК → [канал 2] → ВИЗУАЛИЗАТОР\n\n";
    std::cout << "Запуск процессов конвейера...\n\n";

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    STARTUPINFOA si1 = {sizeof(si1)}, si2 = {sizeof(si2)}, si3 = {sizeof(si3)};
    PROCESS_INFORMATION pi1, pi2, pi3;

    // Запуск процесса генератора
    std::string cmdGenerator = std::string(exePath) + " generator";
    if (!CreateProcessA(NULL, (LPSTR) cmdGenerator.c_str(), NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE, NULL, NULL, &si1, &pi1)) {
        std::cerr << "Ошибка запуска генератора: " << GetLastError() << "\n";
        return 1;
    }
    std::cout << "[КООРДИНАТОР] Генератор запущен (PID: " << pi1.dwProcessId << ")\n";

    Sleep(300);

    std::string cmdProcessor = std::string(exePath) + " processor";
    if (!CreateProcessA(NULL, (LPSTR) cmdProcessor.c_str(), NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE, NULL, NULL, &si2, &pi2)) {
        std::cerr << "Ошибка запуска обработчика: " << GetLastError() << "\n";
        return 1;
    }
    std::cout << "[КООРДИНАТОР] Обработчик запущен (PID: " << pi2.dwProcessId << ")\n";

    Sleep(300);

    std::string cmdVisualizer = std::string(exePath) + " visualizer";
    if (!CreateProcessA(NULL, (LPSTR) cmdVisualizer.c_str(), NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE, NULL, NULL, &si3, &pi3)) {
        std::cerr << "Ошибка запуска визуализатора: " << GetLastError() << "\n";
        return 1;
    }
    std::cout << "[КООРДИНАТОР] Визуализатор запущен (PID: " << pi3.dwProcessId << ")\n\n";

    std::cout << "Все процессы запущены. Ожидание завершения...\n\n";

    HANDLE processes[] = {pi1.hProcess, pi2.hProcess, pi3.hProcess};
    WaitForMultipleObjects(3, processes, TRUE, INFINITE);

    std::cout << "\n[КООРДИНАТОР] Все процессы конвейера завершены.\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";

    CloseHandle(pi1.hProcess);
    CloseHandle(pi1.hThread);
    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);
    CloseHandle(pi3.hProcess);
    CloseHandle(pi3.hThread);

    system("pause");
    return 0;
}
