#import "stp/stp.typ"
#show: stp.STP2024

#include "title.typ"

#outline()
#pagebreak()

= Постановка задачи

Цель данной лабораторной работы заключается в возобновлении, закреплении и развитии навыков программирования приложений Windows через изучение концепций вычислительных процессов, потоков и нитей, освоение основных этапов их жизненного цикла включая порождение, завершение, получение и изменение состояния, а также работу с программным интерфейсом API для управления процессами.

Индивидуальное задание предолагает разработку приложения, способного возобновлять свою работу после завершения сообщением #emph[WM_CLOSE]. При получении команды завершения процесс штатно завершается, но перед этим порождает свою копию из того же исполняемого файла, которая продолжает работу вместо родительского процесса. Для обеспечения непрерывной обработки данных текущее состояние приложения записывается в файл, а процесс-наследник читает данные из файла для продолжения работы.

= Описание работы программы

== Инициализация и загрузка состояния

При запуске приложения вызывается функция #emph[LoadState()], которая пытается открыть файл состояния "self_healing_state.dat" с помощью #emph[CreateFileW()] с флагом #emph[OPEN_EXISTING] @fileOperations. Если файл существует, его содержимое считывается функцией #emph[ReadFile()] в переменную #emph[g_counter], восстанавливая тем самым предыдущее состояние счетчика. После успешного чтения файл удаляется функцией #emph[DeleteFileW()], чтобы при следующем "чистом" запуске приложения состояние не загружалось автоматически. Если файл состояния отсутствует, счетчик остается равным нулю, что соответствует первому запуску приложения.

== Регистрация класса окна и создание интерфейса

Приложение регистрирует класс окна с именем "SelfHealingProcessClass" с помощью структуры #emph[WNDCLASSW] и функции #emph[RegisterClassW()], указывая в качестве оконной процедуры функцию #emph[WindowProc]. После успешной регистрации создается главное окно приложения функцией #emph[CreateWindowExW()] с заголовком "Самовосстанавливающийся процесс" и стандартными параметрами оконного интерфейса @windowProcedures. Окно отображается на экране функциями #emph[ShowWindow()] и #emph[UpdateWindow()], после чего устанавливается таймер #emph[SetTimer()] с интервалом 1000 миллисекунд для периодического обновления счетчика.

== Обработка сообщений и отображение состояния

Основной цикл сообщений #emph[GetMessageW()] обрабатывает все поступающие сообщения через функцию #emph[WindowProc()] @messages. При получении сообщения #emph[WM_PAINT] окно перерисовывается, отображая текущее значение счетчика с помощью функций #emph[BeginPaint()], #emph[DrawTextW()] и #emph[EndPaint()]. Сообщение #emph[WM_TIMER] обрабатывается увеличением значения #emph[g_counter] на единицу и вызовом #emph[InvalidateRect()] для принудительной перерисовки окна, что обеспечивает визуальное отображение работающего счетчика, увеличивающегося каждую секунду.

== Сохранение состояния приложения

При получении сообщения #emph[WM_CLOSE] приложение вызывает функцию #emph[SaveState()], которая создает файл "self_healing_state.dat" с помощью функции #emph[CreateFileW()] с флагом #emph[CREATE_ALWAYS] для перезаписи существующего файла. В этот файл записывается текущее значение счетчика #emph[g_counter] с использованием функции #emph[WriteFile()], после чего файл закрывается функцией #emph[CloseHandle()]. Такой подход обеспечивает сохранение текущего состояния приложения перед его завершением, что позволяет новому экземпляру процесса продолжить работу с того же значения счетчика.

== Создание нового экземпляра процесса

После сохранения состояния приложение вызывает функцию #emph[CreateNewInstance()], которая получает полный путь к исполняемому файлу текущего процесса с помощью #emph[GetModuleFileNameW()] и сохраняет его в буфер #emph[szPath]. Затем инициализируются структуры #emph[STARTUPINFOW] и #emph[PROCESS_INFORMATION], необходимые для создания нового процесса. Функция #emph[CreateProcessW()] запускает новый экземпляр того же приложения, передавая ему путь к исполняемому файлу, после чего закрываются дескрипторы процесса и потока с помощью #emph[CloseHandle()], поскольку родительский процесс не нуждается в контроле над дочерним процессом @processAndThreadFunc. Новый экземпляр при запуске загрузит сохраненное состояние и продолжит работу с того же значения счетчика, обеспечивая таким образом непрерывность функционирования приложения.

= Ход выполнения программы

== Пример выполнения задания

На рисунке @img3.1 отображено главное окно работающего приложения. В нем выводится текущее значение счетчика, которое инкрементируется каждую секунду, а также собственный идентификатор процесса (#emph("PID")).

#figure(
  image("img/img3.1.png", width: 85%),
  caption: [Результат работы программы]
) <img3.1>

На рисунке @img3.2 показан результат работы механизма самовосстановления после закрытия предыдущего окна. Был запущен новый экземпляр приложения с новым идентификатором процесса (#emph("PID")). При этом счетчик продолжил работу с сохраненного значения, демонстрируя успешное восстановление состояния. 

#figure(
  image("img/img3.2.png", width: 85%),
  caption: [Результат работы программы после закрытия]
) <img3.2>

#pagebreak()

#stp.heading_unnumbered[Вывод]

В ходе выполнения лабораторной работы был разработан прототип самовосстанавливающегося приложения #emph("Windows"), демонстрирующий основные концепции управления процессами и сохранения состояния приложения.

Основным достижением является успешная реализация механизма сохранения и загрузки состояния приложения. Приложение сохраняет текущее значение счетчика в файл перед завершением и восстанавливает его при запуске нового экземпляра, обеспечивая непрерывность функционирования. Кроме того, реализована функция создания нового процесса через #emph("CreateProcessW()"), которая автоматически запускает новый экземпляр приложения при получении сигнала завершения.

Пользовательский интерфейс выводит текущее значение счетчика, идентификатор текущего процесса (#emph("PID")) и идентификатор родительского процесса, позволяя наглядно видеть переход управления между процессами. Приложение корректно обрабатывает сообщения #emph("WM_PAINT"), #emph("WM_TIMER") и #emph("WM_CLOSE"), демонстрируя правильную работу основного цикла сообщений Windows.

Полученные навыки управления процессами и потоками, работа с #emph("API") функциями #emph("Windows") и понимание жизненного цикла процессов являются фундаментальными для системного программирования на платформе Windows.

#bibliography("bibliography.bib")

#stp.appendix(type:[справочное], title:[Исходный код программы], [

```
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>
#include <string>

const wchar_t* STATE_FILE_NAME = L"self_healing_state.dat";

const wchar_t* CLASS_NAME = L"SelfHealingProcessClass";

int g_counter = 0;
HWND g_hwnd;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SaveState();
void LoadState();
void CreateNewInstance();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LoadState();

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

    SetTimer(g_hwnd, 1, 1000, NULL);

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        wchar_t buffer[256];
        swprintf_s(buffer, L"Текущий счетчик: %d", g_counter);

        RECT rect;
        GetClientRect(hwnd, &rect);
        DrawTextW(hdc, buffer, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        g_counter++;
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_CLOSE: {
        SaveState();
        
        CreateNewInstance();
        
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

    if (!CreateProcessW(szPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        MessageBoxW(NULL, L"Failed to create new instance!", L"Error", MB_OK);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
```


])
