#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>
#include <string>

// Имя файла для сохранения состояния
const wchar_t* STATE_FILE_NAME = L"self_healing_state.dat";
// Имя класса окна
const wchar_t* CLASS_NAME = L"SelfHealingProcessClass";

int g_counter = 0;
HWND g_hwnd;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SaveState();
void LoadState();
void CreateNewInstance();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Загружаем предыдущее состояние, если оно есть
    LoadState();

    // Регистрация класса окна
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Создание окна
    g_hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Самовосстанавливающийся процесс",
        WS_OVERLAPPEDWINDOW,
        200, 200, 400, 200,
        NULL, NULL, hInstance, NULL
    );

    if (g_hwnd == NULL) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    // Устанавливаем таймер для инкремента счетчика каждую секунду
    SetTimer(g_hwnd, 1, 1000, NULL);

    // Цикл сообщений
    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Получаем PID текущего процесса
        DWORD dwProcessId = GetCurrentProcessId();

        // Отображаем счетчик и PID
        wchar_t buffer[256];
        swprintf_s(buffer, L"Счетчик: %d\r\nProcess ID: %u", g_counter, dwProcessId);

        RECT rect;
        GetClientRect(hwnd, &rect);
        DrawTextW(hdc, buffer, -1, &rect, DT_VCENTER | DT_CENTER);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        // Увеличиваем счетчик и перерисовываем окно
        g_counter++;
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_CLOSE: {
        // Сохраняем состояние
        SaveState();
        // Создаем новый экземпляр процесса
        CreateNewInstance();
        // Уничтожаем текущее окно, что приведет к завершению процесса
        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void SaveState() {
    HANDLE hFile = CreateFileW(STATE_FILE_NAME,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hFile, &g_counter, sizeof(g_counter), &bytesWritten, NULL);
        CloseHandle(hFile);
    }
}

void LoadState() {
    HANDLE hFile = CreateFileW(STATE_FILE_NAME,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesRead;
        ReadFile(hFile, &g_counter, sizeof(g_counter), &bytesRead, NULL);
        CloseHandle(hFile);
        // Удаляем файл после чтения, чтобы при следующем "чистом" запуске состояние не загружалось
        DeleteFileW(STATE_FILE_NAME);
    }
}

void CreateNewInstance() {
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(szPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) { // Используем CreateProcessW
        MessageBoxW(NULL, L"Failed to create new instance!", L"Error", MB_OK);
    }

    // Закрываем хендлы, так как они нам больше не нужны
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}