#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <mutex>
#include <atomic>

using namespace std;
using namespace chrono;

const size_t FILE_SIZE = 100 * 1024 * 1024; // 100 MB
const size_t BUFFER_SIZE = 64 * 1024; // 64 KB
const size_t ASYNC_OPERATIONS = 4; // Количество параллельных асинхронных операций
const int NUM_THREADS = 4; // Количество потоков

struct BenchmarkResult {
    string method_name;
    double write_time_ms;
    double read_time_ms;
    bool success;
    string error_message;
};

// Вспомогательные функции
string FormatTime(double ms) {
    char buffer[64];
    sprintf(buffer, "%.2f ms (%.2f MB/s)", ms, (FILE_SIZE / (1024.0 * 1024.0)) / (ms / 1000.0));
    return string(buffer);
}

void PrintResult(const BenchmarkResult &result) {
    cout << "\n=== " << result.method_name << " ===" << endl;
    if (result.success) {
        cout << "  Write: " << FormatTime(result.write_time_ms) << endl;
        cout << "  Read:  " << FormatTime(result.read_time_ms) << endl;
    } else {
        cout << "  ERROR: " << result.error_message << endl;
    }
}

// 1. Блокирующий (синхронный) ввод-вывод
BenchmarkResult TestBlockingIO() {
    BenchmarkResult result;
    result.method_name = "1. Blocking I/O (Synchronous)";
    result.success = true;

    const char *filename = "test_blocking.dat";
    vector<BYTE> buffer(BUFFER_SIZE);

    // Заполняем буфер тестовыми данными
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = static_cast<BYTE>(i % 256);
    }

    // ЗАПИСЬ
    auto start = high_resolution_clock::now();

    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot create file for writing";
        return result;
    }

    size_t totalWritten = 0;
    while (totalWritten < FILE_SIZE) {
        DWORD bytesWritten;
        size_t toWrite = min(BUFFER_SIZE, FILE_SIZE - totalWritten);

        if (!WriteFile(hFile, buffer.data(), static_cast<DWORD>(toWrite), &bytesWritten, NULL)) {
            result.success = false;
            result.error_message = "Write error";
            CloseHandle(hFile);
            return result;
        }

        totalWritten += bytesWritten;
    }

    CloseHandle(hFile);
    auto end = high_resolution_clock::now();
    result.write_time_ms = duration<double, milli>(end - start).count();

    // ЧТЕНИЕ
    start = high_resolution_clock::now();

    hFile = CreateFileA(
        filename,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot open file for reading";
        return result;
    }

    size_t totalRead = 0;
    while (totalRead < FILE_SIZE) {
        DWORD bytesRead;

        if (!ReadFile(hFile, buffer.data(), BUFFER_SIZE, &bytesRead, NULL)) {
            result.success = false;
            result.error_message = "Read error";
            CloseHandle(hFile);
            return result;
        }

        if (bytesRead == 0) break;
        totalRead += bytesRead;
    }

    CloseHandle(hFile);
    end = high_resolution_clock::now();
    result.read_time_ms = duration<double, milli>(end - start).count();

    DeleteFileA(filename);
    return result;
}

// 2. Ввод-вывод по прерываниям
BenchmarkResult TestInterruptDrivenIO() {
    BenchmarkResult result;
    result.method_name = "2. Interrupt-Driven I/O (Event-Based)";
    result.success = true;

    const char *filename = "test_interrupt.dat";
    vector<BYTE> buffer(BUFFER_SIZE);

    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = static_cast<BYTE>(i % 256);
    }

    // Создаем событие для сигнализации
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hEvent) {
        result.success = false;
        result.error_message = "Cannot create event";
        return result;
    }

    OVERLAPPED overlapped = {0};
    overlapped.hEvent = hEvent;

    // ЗАПИСЬ
    auto start = high_resolution_clock::now();

    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot create file";
        CloseHandle(hEvent);
        return result;
    }

    size_t totalWritten = 0;
    while (totalWritten < FILE_SIZE) {
        overlapped.Offset = static_cast<DWORD>(totalWritten & 0xFFFFFFFF);
        overlapped.OffsetHigh = static_cast<DWORD>(totalWritten >> 32);

        DWORD bytesWritten;
        size_t toWrite = min(BUFFER_SIZE, FILE_SIZE - totalWritten);

        if (!WriteFile(hFile, buffer.data(), static_cast<DWORD>(toWrite), &bytesWritten, &overlapped)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                // Ждем сигнала события
                WaitForSingleObject(hEvent, INFINITE);
                GetOverlappedResult(hFile, &overlapped, &bytesWritten, FALSE);
            } else {
                result.success = false;
                result.error_message = "Write error";
                CloseHandle(hFile);
                CloseHandle(hEvent);
                return result;
            }
        }

        totalWritten += bytesWritten;
    }

    CloseHandle(hFile);
    auto end = high_resolution_clock::now();
    result.write_time_ms = duration<double, milli>(end - start).count();

    // ЧТЕНИЕ
    start = high_resolution_clock::now();

    hFile = CreateFileA(
        filename,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot open file";
        CloseHandle(hEvent);
        return result;
    }

    size_t totalRead = 0;
    while (totalRead < FILE_SIZE) {
        overlapped.Offset = static_cast<DWORD>(totalRead & 0xFFFFFFFF);
        overlapped.OffsetHigh = static_cast<DWORD>(totalRead >> 32);

        DWORD bytesRead;

        if (!ReadFile(hFile, buffer.data(), BUFFER_SIZE, &bytesRead, &overlapped)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(hEvent, INFINITE);
                GetOverlappedResult(hFile, &overlapped, &bytesRead, FALSE);
            } else {
                result.success = false;
                result.error_message = "Read error";
                CloseHandle(hFile);
                CloseHandle(hEvent);
                return result;
            }
        }

        if (bytesRead == 0) break;
        totalRead += bytesRead;
    }

    CloseHandle(hFile);
    CloseHandle(hEvent);
    end = high_resolution_clock::now();
    result.read_time_ms = duration<double, milli>(end - start).count();

    DeleteFileA(filename);
    return result;
}

// 3. Многопоточная организация
mutex mtx;
atomic<size_t> global_offset(0);

struct ThreadData {
    HANDLE hFile;
    vector<BYTE> *buffer;
    size_t thread_id;
    size_t chunk_size;
    bool success;
};

DWORD WINAPI WriteThreadProc(LPVOID param) {
    ThreadData *data = (ThreadData *) param;

    while (true) {
        size_t offset = global_offset.fetch_add(data->chunk_size);
        if (offset >= FILE_SIZE) break;

        size_t toWrite = min(data->chunk_size, FILE_SIZE - offset);

        OVERLAPPED overlapped = {0};
        overlapped.Offset = static_cast<DWORD>(offset & 0xFFFFFFFF);
        overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);

        DWORD bytesWritten;
        if (!WriteFile(data->hFile, data->buffer->data(), static_cast<DWORD>(toWrite),
                       &bytesWritten, &overlapped)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                GetOverlappedResult(data->hFile, &overlapped, &bytesWritten, TRUE);
            } else {
                data->success = false;
                return 1;
            }
        }
    }

    return 0;
}

DWORD WINAPI ReadThreadProc(LPVOID param) {
    ThreadData *data = (ThreadData *) param;

    while (true) {
        size_t offset = global_offset.fetch_add(data->chunk_size);
        if (offset >= FILE_SIZE) break;

        size_t toRead = min(data->chunk_size, FILE_SIZE - offset);

        OVERLAPPED overlapped = {0};
        overlapped.Offset = static_cast<DWORD>(offset & 0xFFFFFFFF);
        overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);

        DWORD bytesRead;
        if (!ReadFile(data->hFile, data->buffer->data(), static_cast<DWORD>(toRead),
                      &bytesRead, &overlapped)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                GetOverlappedResult(data->hFile, &overlapped, &bytesRead, TRUE);
            } else {
                data->success = false;
                return 1;
            }
        }
    }

    return 0;
}

BenchmarkResult TestMultithreadedIO() {
    BenchmarkResult result;
    result.method_name = "3. Multithreaded I/O";
    result.success = true;

    const char *filename = "test_multithread.dat";
    const size_t CHUNK_SIZE = BUFFER_SIZE;

    vector<vector<BYTE> > buffers(NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++) {
        buffers[i].resize(CHUNK_SIZE);
        for (size_t j = 0; j < CHUNK_SIZE; j++) {
            buffers[i][j] = static_cast<BYTE>((i + j) % 256);
        }
    }

    // ЗАПИСЬ
    auto start = high_resolution_clock::now();

    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot create file";
        return result;
    }

    // Устанавливаем размер файла
    SetFilePointer(hFile, FILE_SIZE, NULL, FILE_BEGIN);
    SetEndOfFile(hFile);

    global_offset = 0;
    vector<HANDLE> threads;
    vector<ThreadData> threadData(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        threadData[i].hFile = hFile;
        threadData[i].buffer = &buffers[i];
        threadData[i].thread_id = i;
        threadData[i].chunk_size = CHUNK_SIZE;
        threadData[i].success = true;

        HANDLE hThread = CreateThread(NULL, 0, WriteThreadProc, &threadData[i], 0, NULL);
        threads.push_back(hThread);
    }

    WaitForMultipleObjects(NUM_THREADS, threads.data(), TRUE, INFINITE);

    for (auto h: threads) CloseHandle(h);
    threads.clear();

    for (const auto &td: threadData) {
        if (!td.success) {
            result.success = false;
            result.error_message = "Thread write error";
            CloseHandle(hFile);
            return result;
        }
    }

    CloseHandle(hFile);
    auto end = high_resolution_clock::now();
    result.write_time_ms = duration<double, milli>(end - start).count();

    // ЧТЕНИЕ
    start = high_resolution_clock::now();

    hFile = CreateFileA(
        filename,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot open file";
        return result;
    }

    global_offset = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        threadData[i].hFile = hFile;
        threadData[i].success = true;

        HANDLE hThread = CreateThread(NULL, 0, ReadThreadProc, &threadData[i], 0, NULL);
        threads.push_back(hThread);
    }

    WaitForMultipleObjects(NUM_THREADS, threads.data(), TRUE, INFINITE);

    for (auto h: threads) CloseHandle(h);

    for (const auto &td: threadData) {
        if (!td.success) {
            result.success = false;
            result.error_message = "Thread read error";
            CloseHandle(hFile);
            return result;
        }
    }

    CloseHandle(hFile);
    end = high_resolution_clock::now();
    result.read_time_ms = duration<double, milli>(end - start).count();

    DeleteFileA(filename);
    return result;
}

// 4. Асинхронный ввод-вывод
BenchmarkResult TestAsyncIO() {
    BenchmarkResult result;
    result.method_name = "4. Asynchronous I/O (Overlapped)";
    result.success = true;

    const char *filename = "test_async.dat";
    const size_t NUM_ASYNC = ASYNC_OPERATIONS;

    vector<vector<BYTE> > buffers(NUM_ASYNC);
    vector<OVERLAPPED> overlappeds(NUM_ASYNC);
    vector<HANDLE> events(NUM_ASYNC);

    for (size_t i = 0; i < NUM_ASYNC; i++) {
        buffers[i].resize(BUFFER_SIZE);
        for (size_t j = 0; j < BUFFER_SIZE; j++) {
            buffers[i][j] = static_cast<BYTE>((i + j) % 256);
        }

        events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        ZeroMemory(&overlappeds[i], sizeof(OVERLAPPED));
        overlappeds[i].hEvent = events[i];
    }

    // ЗАПИСЬ
    auto start = high_resolution_clock::now();

    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot create file";
        for (auto e: events) CloseHandle(e);
        return result;
    }

    size_t totalWritten = 0;
    size_t nextBuffer = 0;

    // Запускаем первую волну операций
    for (size_t i = 0; i < NUM_ASYNC && totalWritten < FILE_SIZE; i++) {
        overlappeds[i].Offset = static_cast<DWORD>(totalWritten & 0xFFFFFFFF);
        overlappeds[i].OffsetHigh = static_cast<DWORD>(totalWritten >> 32);

        DWORD bytesWritten;
        size_t toWrite = min(BUFFER_SIZE, FILE_SIZE - totalWritten);

        WriteFile(hFile, buffers[i].data(), static_cast<DWORD>(toWrite), &bytesWritten, &overlappeds[i]);
        totalWritten += toWrite;
        nextBuffer++;
    }

    // Ждем завершения операций и запускаем новые
    while (nextBuffer < NUM_ASYNC || totalWritten < FILE_SIZE) {
        DWORD waitResult = WaitForMultipleObjects(static_cast<DWORD>(min(nextBuffer, NUM_ASYNC)),
                                                  events.data(), FALSE, INFINITE);

        size_t completedIdx = waitResult - WAIT_OBJECT_0;

        DWORD bytesWritten;
        GetOverlappedResult(hFile, &overlappeds[completedIdx], &bytesWritten, FALSE);

        // Запускаем новую операцию, если есть что записывать
        if (totalWritten < FILE_SIZE) {
            overlappeds[completedIdx].Offset = static_cast<DWORD>(totalWritten & 0xFFFFFFFF);
            overlappeds[completedIdx].OffsetHigh = static_cast<DWORD>(totalWritten >> 32);

            size_t toWrite = min(BUFFER_SIZE, FILE_SIZE - totalWritten);
            WriteFile(hFile, buffers[completedIdx].data(), static_cast<DWORD>(toWrite),
                      &bytesWritten, &overlappeds[completedIdx]);
            totalWritten += toWrite;
        } else {
            nextBuffer--;
        }
    }

    CloseHandle(hFile);
    auto end = high_resolution_clock::now();
    result.write_time_ms = duration<double, milli>(end - start).count();

    // ЧТЕНИЕ
    start = high_resolution_clock::now();

    hFile = CreateFileA(
        filename,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot open file";
        for (auto e: events) CloseHandle(e);
        return result;
    }

    size_t totalRead = 0;
    nextBuffer = 0;

    for (size_t i = 0; i < NUM_ASYNC && totalRead < FILE_SIZE; i++) {
        overlappeds[i].Offset = static_cast<DWORD>(totalRead & 0xFFFFFFFF);
        overlappeds[i].OffsetHigh = static_cast<DWORD>(totalRead >> 32);

        DWORD bytesRead;
        ReadFile(hFile, buffers[i].data(), BUFFER_SIZE, &bytesRead, &overlappeds[i]);
        totalRead += min(BUFFER_SIZE, FILE_SIZE - totalRead);
        nextBuffer++;
    }

    while (nextBuffer > 0 && totalRead < FILE_SIZE) {
        DWORD waitResult = WaitForMultipleObjects(static_cast<DWORD>(min(nextBuffer, NUM_ASYNC)),
                                                  events.data(), FALSE, INFINITE);

        size_t completedIdx = waitResult - WAIT_OBJECT_0;

        DWORD bytesRead;
        GetOverlappedResult(hFile, &overlappeds[completedIdx], &bytesRead, FALSE);

        if (totalRead < FILE_SIZE) {
            overlappeds[completedIdx].Offset = static_cast<DWORD>(totalRead & 0xFFFFFFFF);
            overlappeds[completedIdx].OffsetHigh = static_cast<DWORD>(totalRead >> 32);

            ReadFile(hFile, buffers[completedIdx].data(), BUFFER_SIZE, &bytesRead,
                     &overlappeds[completedIdx]);
            totalRead += min(BUFFER_SIZE, FILE_SIZE - totalRead);
        } else {
            nextBuffer--;
        }
    }

    CloseHandle(hFile);
    end = high_resolution_clock::now();
    result.read_time_ms = duration<double, milli>(end - start).count();

    for (auto e: events) CloseHandle(e);
    DeleteFileA(filename);
    return result;
}

// 5. Отображение файлов в память
BenchmarkResult TestMemoryMappedIO() {
    BenchmarkResult result;
    result.method_name = "5. Memory-Mapped I/O";
    result.success = true;

    const char *filename = "test_mmap.dat";

    // ЗАПИСЬ
    auto start = high_resolution_clock::now();

    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot create file";
        return result;
    }

    HANDLE hMapping = CreateFileMappingA(
        hFile,
        NULL,
        PAGE_READWRITE,
        static_cast<DWORD>(FILE_SIZE >> 32),
        static_cast<DWORD>(FILE_SIZE & 0xFFFFFFFF),
        NULL
    );

    if (!hMapping) {
        result.success = false;
        result.error_message = "Cannot create file mapping";
        CloseHandle(hFile);
        return result;
    }

    BYTE *pView = (BYTE *) MapViewOfFile(
        hMapping,
        FILE_MAP_WRITE,
        0,
        0,
        FILE_SIZE
    );

    if (!pView) {
        result.success = false;
        result.error_message = "Cannot map view of file";
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return result;
    }

    // Записываем данные напрямую в отображенную память
    for (size_t i = 0; i < FILE_SIZE; i++) {
        pView[i] = static_cast<BYTE>(i % 256);
    }

    // Принудительный сброс на диск
    FlushViewOfFile(pView, FILE_SIZE);

    UnmapViewOfFile(pView);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    auto end = high_resolution_clock::now();
    result.write_time_ms = duration<double, milli>(end - start).count();

    // ЧТЕНИЕ
    start = high_resolution_clock::now();

    hFile = CreateFileA(
        filename,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        result.success = false;
        result.error_message = "Cannot open file for reading";
        return result;
    }

    hMapping = CreateFileMappingA(
        hFile,
        NULL,
        PAGE_READONLY,
        static_cast<DWORD>(FILE_SIZE >> 32),
        static_cast<DWORD>(FILE_SIZE & 0xFFFFFFFF),
        NULL
    );

    if (!hMapping) {
        result.success = false;
        result.error_message = "Cannot create file mapping for reading";
        CloseHandle(hFile);
        return result;
    }

    const BYTE *pReadView = (const BYTE *) MapViewOfFile(
        hMapping,
        FILE_MAP_READ,
        0,
        0,
        FILE_SIZE
    );

    if (!pReadView) {
        result.success = false;
        result.error_message = "Cannot map view of file for reading";
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return result;
    }

    // Читаем данные из отображенной памяти
    volatile BYTE sum = 0;
    for (size_t i = 0; i < FILE_SIZE; i++) {
        sum += pReadView[i];
    }

    UnmapViewOfFile(pReadView);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    end = high_resolution_clock::now();
    result.read_time_ms = duration<double, milli>(end - start).count();

    DeleteFileA(filename);
    return result;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    cout << "\nРазмер файла: " << (FILE_SIZE / (1024 * 1024)) << " MB" << endl;
    cout << "Размер буффера: " << (BUFFER_SIZE / 1024) << " KB" << endl;
    cout << "Колличество ядер: " << NUM_THREADS << endl;
    cout << "Асинхронные операции: " << ASYNC_OPERATIONS << endl;

    vector<BenchmarkResult> results;

    results.push_back(TestBlockingIO());
    results.push_back(TestInterruptDrivenIO());
    results.push_back(TestMultithreadedIO());
    results.push_back(TestAsyncIO());
    results.push_back(TestMemoryMappedIO());

    cout << endl;
    cout << "                                 Сравнительная таблица" << endl;
    cout << endl;
    cout << left << setw(35) << "Метод"
            << right << setw(15) << "  Запись (ms)   "
            << setw(15) << "   Чтение (ms)    "
            << setw(15) << "   Суммарно (ms)  " << endl;
    cout << string(80, '-') << endl;

    for (const auto &result: results) {
        if (result.success) {
            cout << left << setw(35) << result.method_name
                    << right << setw(15) << fixed << setprecision(2) << result.write_time_ms
                    << setw(15) << result.read_time_ms
                    << setw(15) << (result.write_time_ms + result.read_time_ms) << endl;
        }
    }

    cin.get();

    return 0;
}
