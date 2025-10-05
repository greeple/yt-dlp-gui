#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>

#include "resource.h"

// Глобальные переменные
HINSTANCE hInst;
HWND hMainWindow;
HWND hUrlEdit, hDownloadButton, hBrowseButton;
HWND hProgressBar;
HWND hOutputEdit, hToggleOutputButton;
HWND hTabControl;
HWND hStatus;
BOOL isOutputVisible = FALSE;
WCHAR configFilePath[MAX_PATH];
WCHAR downloadPath[MAX_PATH];

// --- Прототипы функций ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateMainControls(HWND);
void CreateTabs(HWND);
void OnTabChange(HWND);
void ResizeControls(HWND);
void ShowWindowContent(int);
void ExecuteYtDlp();
DWORD WINAPI YtDlpThread(LPVOID);
void ReadOutput(HANDLE, HWND);
void LoadSettings();
void SaveSettings();
std::wstring GetSettingsValue(const std::wstring&, const std::wstring&);
void SetSettingsValue(const std::wstring&, const std::wstring&, const std::wstring&);
std.wstring BuildYtDlpCommand();


// --- Точка входа ---
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    hInst = hInstance;

    // Инициализация пути к файлу настроек
    GetModuleFileNameW(NULL, configFilePath, MAX_PATH);
    *wcsrchr(configFilePath, L'\\') = L'\0';
    wcscat_s(configFilePath, MAX_PATH, L"\\settings.ini");

    LoadSettings();

    // Регистрация класса окна
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"YtDlpGuiClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // Создание главного окна
    hMainWindow = CreateWindowW(
        L"YtDlpGuiClass", L"yt-dlp GUI",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    // Цикл сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

// --- Обработчик сообщений окна ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateMainControls(hwnd);
            CreateTabs(hwnd);
            ShowWindowContent(0); // Показать первую вкладку
            break;
        case WM_SIZE:
            ResizeControls(hwnd);
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_DOWNLOAD_BUTTON) {
                ExecuteYtDlp();
            } else if (LOWORD(wParam) == IDC_TOGGLE_OUTPUT_BUTTON) {
                isOutputVisible = !isOutputVisible;
                ResizeControls(hwnd);
                SetWindowTextW(hToggleOutputButton, isOutputVisible ? L"Скрыть вывод" : L"Показать вывод");
            } else if (LOWORD(wParam) == IDC_BROWSE_BUTTON) {
                 BROWSEINFOW bi = { 0 };
                 bi.hwndOwner = hwnd;
                 bi.lpszTitle = L"Выберите папку для сохранения";
                 LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
                 if (pidl != 0) {
                     SHGetPathFromIDListW(pidl, downloadPath);
                     SetDlgItemTextW(GetDlgItem(hMainWindow, IDC_OUTPUT_PATH_EDIT), 0, downloadPath); // Предполагаем, что есть такой контрол на одной из вкладок
                     CoTaskMemFree(pidl);
                 }
            }
            break;
        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
                 OnTabChange(hwnd);
            }
            break;
        case WM_DESTROY:
            SaveSettings();
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// --- Создание основных элементов управления ---
void CreateMainControls(HWND hwnd) {
    CreateWindowW(L"STATIC", L"URL видео:", WS_VISIBLE | WS_CHILD, 10, 15, 80, 20, hwnd, NULL, hInst, NULL);
    hUrlEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 100, 10, 480, 25, hwnd, NULL, hInst, NULL);
    hDownloadButton = CreateWindowW(L"BUTTON", L"Скачать", WS_VISIBLE | WS_CHILD, 600, 10, 100, 25, hwnd, (HMENU)IDC_DOWNLOAD_BUTTON, hInst, NULL);

    hProgressBar = CreateWindowW(PROGRESS_CLASSW, NULL, WS_VISIBLE | WS_CHILD | PBS_SMOOTH, 10, 45, 690, 20, hwnd, NULL, hInst, NULL);
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    hToggleOutputButton = CreateWindowW(L"BUTTON", L"Показать вывод", WS_VISIBLE | WS_CHILD, 10, 75, 150, 25, hwnd, (HMENU)IDC_TOGGLE_OUTPUT_BUTTON, hInst, NULL);
    hOutputEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 10, 110, 760, 0, hwnd, NULL, hInst, NULL);

    hStatus = CreateStatusWindowW(WS_CHILD | WS_VISIBLE, L"Готово", hwnd, IDC_STATUS_BAR);
}


// --- Создание вкладок ---
void CreateTabs(HWND hwnd) {
    hTabControl = CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE, 10, 110, 760, 420, hwnd, (HMENU)IDC_TAB_CONTROL, hInst, NULL);

    TCITEMW tie;
    tie.mask = TCIF_TEXT;
    
    wchar_t* titles[] = {L"Основные", L"Сеть", L"Вывод", L"Форматы", L"Субтитры", L"Метаданные"};
    for(int i = 0; i < 6; i++) {
        tie.pszText = titles[i];
        TabCtrl_InsertItem(hTabControl, i, &tie);
    }
    
    // Здесь мы должны создать ВСЕ элементы для ВСЕХ вкладок, а потом просто показывать/скрывать их
    // Пример для первой вкладки:
    CreateWindowW(L"STATIC", L"Путь сохранения:", WS_CHILD, 20, 150, 120, 20, hwnd, (HMENU)IDC_STATIC_PATH, hInst, NULL);
    CreateWindowW(L"EDIT", downloadPath, WS_CHILD | WS_BORDER, 150, 145, 400, 25, hwnd, (HMENU)IDC_OUTPUT_PATH_EDIT, hInst, NULL);
    hBrowseButton = CreateWindowW(L"BUTTON", L"Обзор...", WS_CHILD, 560, 145, 100, 25, hwnd, (HMENU)IDC_BROWSE_BUTTON, hInst, NULL);
    
    // Другие элементы для других вкладок создаются здесь же...
}

// --- Обработка смены вкладок ---
void OnTabChange(HWND hwnd) {
    int iSel = TabCtrl_GetCurSel(hTabControl);
    ShowWindowContent(iSel);
}

// --- Показать/скрыть контент вкладок ---
void ShowWindowContent(int tabIndex) {
    // Сначала скроем все
    ShowWindow(GetDlgItem(hwnd, IDC_STATIC_PATH), SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_OUTPUT_PATH_EDIT), SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_BROWSE_BUTTON), SW_HIDE);
    // ... и так для всех элементов всех вкладок ...

    // Теперь покажем нужные
    switch(tabIndex) {
        case 0: // Основные
            ShowWindow(GetDlgItem(hwnd, IDC_STATIC_PATH), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_OUTPUT_PATH_EDIT), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_BROWSE_BUTTON), SW_SHOW);
            break;
        case 1: // Сеть
            // Показать элементы для вкладки "Сеть"
            break;
        // ... и так далее для остальных вкладок
    }
}


// --- Адаптивный размер элементов ---
void ResizeControls(HWND hwnd) {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int width = rcClient.right;
    int height = rcClient.bottom;

    MoveWindow(hUrlEdit, 100, 10, width - 220, 25, TRUE);
    MoveWindow(hDownloadButton, width - 110, 10, 100, 25, TRUE);
    MoveWindow(hProgressBar, 10, 45, width - 20, 20, TRUE);
    MoveWindow(hToggleOutputButton, 10, 75, 150, 25, TRUE);

    int outputHeight = isOutputVisible ? 150 : 0;
    int tabTop = 80;
    int tabHeight = height - tabTop - (isOutputVisible ? outputHeight + 10 : 0) - 30; // 30 для статус-бара

    if (isOutputVisible) {
        MoveWindow(hTabControl, 10, tabTop, width - 20, tabHeight, TRUE);
        MoveWindow(hOutputEdit, 10, tabTop + tabHeight + 5, width - 20, outputHeight, TRUE);
    } else {
        MoveWindow(hTabControl, 10, tabTop, width - 20, height - tabTop - 30, TRUE);
    }
    ShowWindow(hOutputEdit, isOutputVisible ? SW_SHOW : SW_HIDE);
    
    // Адаптация элементов внутри вкладок
    RECT rcTab;
    GetWindowRect(hTabControl, &rcTab);
    MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rcTab, 2);
    
    MoveWindow(GetDlgItem(hwnd, IDC_OUTPUT_PATH_EDIT), rcTab.left + 140, rcTab.top + 35, rcTab.right - rcTab.left - 260, 25, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_BROWSE_BUTTON), rcTab.right - 120, rcTab.top + 35, 100, 25, TRUE);


    SendMessage(hStatus, WM_SIZE, 0, 0);
}

// --- Функции для работы с настройками ---
void LoadSettings() {
    // Папка по умолчанию - Рабочий стол
    SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, downloadPath);
    
    std::wifstream configFile(configFilePath);
    if (configFile.is_open()) {
        std::wstring line;
        while (std::getline(configFile, line)) {
            if (line.rfind(L"DownloadPath=", 0) == 0) {
                 wcscpy_s(downloadPath, MAX_PATH, line.substr(13).c_str());
            }
            // Загрузка других настроек...
        }
        configFile.close();
    }
}

void SaveSettings() {
    std::wofstream configFile(configFilePath);
    if (configFile.is_open()) {
        configFile << L"DownloadPath=" << downloadPath << std::endl;
        // Сохранение других настроек...
        configFile.close();
    }
}

// --- Запуск yt-dlp ---
void ExecuteYtDlp() {
    EnableWindow(hDownloadButton, FALSE);
    SetWindowTextW(hStatus, L"Скачивание...");
    SetWindowTextW(hOutputEdit, L""); // Очистить предыдущий вывод
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    CreateThread(NULL, 0, YtDlpThread, NULL, 0, NULL);
}

DWORD WINAPI YtDlpThread(LPVOID lpParam) {
    wchar_t url[1024];
    GetWindowTextW(hUrlEdit, url, 1024);
    if (wcslen(url) == 0) {
        MessageBoxW(hMainWindow, L"Пожалуйста, введите URL.", L"Ошибка", MB_OK | MB_ICONERROR);
        EnableWindow(hDownloadButton, TRUE);
        return 1;
    }

    std::wstring command = BuildYtDlpCommand();

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    if (CreateProcessW(NULL, &command[0], NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hWrite); // Закрываем нашу сторону пайпа
        ReadOutput(hRead, hOutputEdit);
        CloseHandle(hRead);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        MessageBoxW(hMainWindow, (L"Не удалось запустить yt-dlp.exe. Убедитесь, что он находится в той же папке, что и программа.\nКоманда: " + command).c_str(), L"Ошибка", MB_OK | MB_ICONERROR);
    }

    SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
    SetWindowTextW(hStatus, L"Готово");
    EnableWindow(hDownloadButton, TRUE);
    return 0;
}


// --- Чтение вывода из процесса ---
void ReadOutput(HANDLE hPipe, HWND hEdit) {
    char buffer[1024];
    DWORD bytesRead;
    std::string fullOutput = "";

    while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead != 0) {
        buffer[bytesRead] = '\0';
        fullOutput += buffer;

        // Конвертация из OEM (консольной) кодировки в UTF-16
        int wideCharCount = MultiByteToWideChar(CP_OEMCP, 0, fullOutput.c_str(), -1, NULL, 0);
        std::vector<wchar_t> w_buffer(wideCharCount);
        MultiByteToWideChar(CP_OEMCP, 0, fullOutput.c_str(), -1, &w_buffer[0], wideCharCount);
        
        // Обновление текстового поля
        SetWindowTextW(hEdit, &w_buffer[0]);
        SendMessage(hEdit, EM_SCROLLCARET, 0, 0);

        // Парсинг прогресса
        std::string line = buffer;
        size_t pos = line.rfind("[download]");
        if (pos != std::string::npos) {
            size_t percent_pos = line.find('%', pos);
            if (percent_pos != std::string::npos) {
                size_t start_pos = line.rfind(' ', percent_pos) + 1;
                std::string percent_str = line.substr(start_pos, percent_pos - start_pos);
                try {
                    int progress = std::stoi(percent_str);
                    SendMessage(hProgressBar, PBM_SETPOS, progress, 0);
                } catch (...) {
                    // Ошибка парсинга
                }
            }
        }
    }
}

// --- Сборка команды для yt-dlp ---
std::wstring BuildYtDlpCommand() {
    wchar_t url[1024];
    GetWindowTextW(hUrlEdit, url, 1024);
    
    // Здесь мы должны собрать все аргументы с вкладок
    // Пример:
    // if (IsDlgButtonChecked(hMainWindow, IDC_CHECKBOX_IGNORE_CONFIG)) {
    //     command += L" --ignore-config";
    // }
    
    wchar_t pathFromEdit[MAX_PATH];
    GetDlgItemTextW(hMainWindow, IDC_OUTPUT_PATH_EDIT, pathFromEdit, MAX_PATH);

    std::wstringstream ss;
    ss << L"yt-dlp.exe --progress -o \"" << pathFromEdit << L"\\%(title)s.%(ext)s\" \"" << url << L"\"";
    
    // Добавляем другие аргументы
    // ss << L" --другой-аргумент";

    return ss.str();
}
