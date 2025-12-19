#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <cstdio> // Для _stprintf_s

#pragma comment(lib, "comctl32.lib")

#define ID_LISTVIEW 1000
#define ID_BUTTON_START 1001
#define ID_BUTTON_TERMINATE 1002
#define ID_TIMER_UPDATE 1

// Структура для хранения информации о дочернем процессе
struct ChildProcessInfo {
    PROCESS_INFORMATION pi;
    std::wstring exePath;
    DWORD processId;
    HWND mainWnd;
    bool isRunning;
};

// Глобальные переменные
HINSTANCE g_hInst;
HWND g_hListView;
std::vector<ChildProcessInfo> g_childProcesses;

// Прототипы
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateUI(HWND);
void OnStartProcess(HWND);
void OnTerminateProcess(HWND);
void UpdateProcessList();
void AddProcessToList(const ChildProcessInfo&);

// Глобальная переменная для передачи данных в EnumWindowsProc
struct EnumData {
    DWORD processId;
    HWND bestHwnd;
};

// Функция обратного вызова для поиска главного окна процесса
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumData* pData = (EnumData*)lParam;
    DWORD currentProcessId;
    GetWindowThreadProcessId(hwnd, &currentProcessId);
    if (pData->processId == currentProcessId) {
        // Ищем видимое окно верхнего уровня
        if (GetParent(hwnd) == NULL && IsWindowVisible(hwnd)) {
            pData->bestHwnd = hwnd;
            return FALSE; // Окно найдено, прекращаем перебор
        }
    }
    return TRUE; // Продолжаем перебор
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInstance;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ProcessDispatcherClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        L"ProcessDispatcherClass", L"Процесс-диспетчер",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateUI(hwnd);
        SetTimer(hwnd, ID_TIMER_UPDATE, 1000, NULL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BUTTON_START:
            OnStartProcess(hwnd);
            break;
        case ID_BUTTON_TERMINATE:
            OnTerminateProcess(hwnd);
            break;
        }
        break;
    case WM_TIMER:
        if (wParam == ID_TIMER_UPDATE) {
            UpdateProcessList();
        }
        break;
    case WM_DESTROY:
        for (auto& info : g_childProcesses) {
            if (info.isRunning) {
                TerminateProcess(info.pi.hProcess, 0);
                CloseHandle(info.pi.hProcess);
                CloseHandle(info.pi.hThread);
            }
        }
        KillTimer(hwnd, ID_TIMER_UPDATE);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void CreateUI(HWND hwnd) {
    CreateWindowW(L"BUTTON", L"Запустить процесс...",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 10, 150, 30, hwnd, (HMENU)ID_BUTTON_START, g_hInst, NULL);

    CreateWindowW(L"BUTTON", L"Завершить процесс",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        170, 10, 150, 30, hwnd, (HMENU)ID_BUTTON_TERMINATE, g_hInst, NULL);

    g_hListView = CreateWindowW(WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        10, 50, 560, 280, hwnd, (HMENU)ID_LISTVIEW, g_hInst, NULL);

    ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.cx = 80;
    lvc.pszText = (LPWSTR)L"PID";
    ListView_InsertColumn(g_hListView, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.cx = 120;
    lvc.pszText = (LPWSTR)L"Статус";
    ListView_InsertColumn(g_hListView, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.cx = 350;
    lvc.pszText = (LPWSTR)L"Путь к файлу";
    ListView_InsertColumn(g_hListView, 2, &lvc);
}

void OnStartProcess(HWND hwnd) {
    WCHAR szFile[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
    ofn.lpstrFilter = L"Исполняемые файлы (*.exe)\0*.exe\0Все файлы\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) == TRUE) {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (CreateProcessW(ofn.lpstrFile, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            ChildProcessInfo info;
            info.pi = pi;
            info.exePath = ofn.lpstrFile; // Теперь это wchar_t* в std::wstring, все корректно
            info.processId = pi.dwProcessId;
            info.isRunning = true;
            info.mainWnd = NULL;
            g_childProcesses.push_back(info);
            AddProcessToList(info);
        } else {
            MessageBoxW(hwnd, L"Не удалось запустить процесс.", L"Ошибка", MB_OK);
        }
    }
}

void OnTerminateProcess(HWND hwnd) {
    int selected = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
    if (selected != -1) {
        ChildProcessInfo& info = g_childProcesses[selected];
        if (info.isRunning) {
            if (info.mainWnd == NULL) {
                EnumData data = { info.processId, NULL };
                EnumWindows(EnumWindowsProc, (LPARAM)&data);
                info.mainWnd = data.bestHwnd;
            }

            if (info.mainWnd != NULL) {
                PostMessageW(info.mainWnd, WM_CLOSE, 0, 0);
            } else {
                if (MessageBoxW(hwnd, L"Главное окно процесса не найдено. Завершить принудительно?", L"Подтверждение", MB_YESNO) == IDYES) {
                    TerminateProcess(info.pi.hProcess, 0);
                }
            }
        }
    } else {
        MessageBoxW(hwnd, L"Выберите процесс из списка.", L"Информация", MB_OK);
    }
}

void AddProcessToList(const ChildProcessInfo& info) {
    LVITEMW lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    lvi.iItem = ListView_GetItemCount(g_hListView);

    WCHAR buffer[32];
    _stprintf_s(buffer, L"%lu", info.processId);
    lvi.iSubItem = 0;
    lvi.pszText = buffer;
    ListView_InsertItem(g_hListView, &lvi);

    lvi.iSubItem = 1;
    lvi.pszText = (LPWSTR)(info.isRunning ? L"Выполняется" : L"Завершился");
    ListView_SetItemText(g_hListView, lvi.iItem, 1, lvi.pszText);

    lvi.iSubItem = 2;
    lvi.pszText = (LPWSTR)info.exePath.c_str();
    ListView_SetItemText(g_hListView, lvi.iItem, 2, lvi.pszText);
}

void UpdateProcessList() {
    for (size_t i = 0; i < g_childProcesses.size(); ++i) {
        ChildProcessInfo& info = g_childProcesses[i];
        if (info.isRunning) {
            DWORD result = WaitForSingleObject(info.pi.hProcess, 0);
            if (result == WAIT_OBJECT_0) {
                info.isRunning = false;
                CloseHandle(info.pi.hProcess);
                CloseHandle(info.pi.hThread);
                info.pi.hProcess = NULL;
                info.pi.hThread = NULL;

                LVITEMW lvi = { 0 };
                lvi.mask = LVIF_TEXT;
                lvi.iItem = (int)i;
                lvi.iSubItem = 1;
                lvi.pszText = (LPWSTR)L"Завершился";
                ListView_SetItem(g_hListView, &lvi);
            }
        }
    }
}