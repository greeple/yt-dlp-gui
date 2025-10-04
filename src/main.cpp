#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <thread>
#include <locale>
#include <codecvt>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Глобальные переменные
HWND hMainWindow;
HWND hUrlEdit, hOutputEdit, hQualityCombo, hFormatCombo;
HWND hDownloadBtn, hBrowseBtn, hProgressBar, hStatusText;
HWND hLogEdit, hToggleLogBtn;
bool isDownloading = false;
bool isLogVisible = false;
int normalHeight = 400;
int expandedHeight = 650;

// Конвертер UTF-8 <-> Wide String
std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result;
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, NULL, NULL);
    return result;
}

// Добавление текста в лог
void AppendLog(const std::wstring& text) {
    int len = GetWindowTextLengthW(hLogEdit);
    SendMessageW(hLogEdit, EM_SETSEL, len, len);
    SendMessageW(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    SendMessageW(hLogEdit, EM_SCROLLCARET, 0, 0);
}

// Очистка лога
void ClearLog() {
    SetWindowTextW(hLogEdit, L"");
}

// Переключение видимости лога
void ToggleLog() {
    isLogVisible = !isLogVisible;
    
    RECT rect;
    GetWindowRect(hMainWindow, &rect);
    
    if (isLogVisible) {
        SetWindowPos(hMainWindow, NULL, 0, 0, 
                     rect.right - rect.left, expandedHeight, 
                     SWP_NOMOVE | SWP_NOZORDER);
        ShowWindow(hLogEdit, SW_SHOW);
        SetWindowTextW(hToggleLogBtn, L"▲ Скрыть лог");
    } else {
        SetWindowPos(hMainWindow, NULL, 0, 0, 
                     rect.right - rect.left, normalHeight, 
                     SWP_NOMOVE | SWP_NOZORDER);
        ShowWindow(hLogEdit, SW_HIDE);
        SetWindowTextW(hToggleLogBtn, L"▼ Показать лог");
    }
}

// Функция для выполнения команды
std::wstring ExecuteCommand(const std::wstring& command) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return L"Ошибка создания канала";
    }
    
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;
    
    PROCESS_INFORMATION pi;
    
    std::wstring cmdCopy = command;
    
    if (!CreateProcessW(NULL, &cmdCopy[0], NULL, NULL, TRUE, 
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return L"Ошибка запуска процесса";
    }
    
    CloseHandle(hWritePipe);
    
    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    
    // Сброс прогресса
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
        
        std::wstring wBuffer = Utf8ToWide(buffer);
        AppendLog(wBuffer);
        
        // Парсинг прогресса из вывода yt-dlp
        std::string line = buffer;
        size_t pos = line.find("%");
        if (pos != std::string::npos) {
            // Ищем число перед %
            size_t start = pos;
            while (start > 0 && (isdigit(line[start-1]) || line[start-1] == '.')) {
                start--;
            }
            if (start < pos) {
                std::string percentStr = line.substr(start, pos - start);
                try {
                    float percent = std::stof(percentStr);
                    SendMessage(hProgressBar, PBM_SETPOS, (WPARAM)percent, 0);
                    
                    std::wstring status = L"Загрузка: " + std::to_wstring((int)percent) + L"%";
                    SendMessageW(hStatusText, WM_SETTEXT, 0, (LPARAM)status.c_str());
                } catch (...) {}
            }
        }
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);
    
    return exitCode == 0 ? L"SUCCESS" : L"ERROR";
}

// Поток загрузки
void DownloadThread() {
    wchar_t url[1024], outputPath[MAX_PATH];
    GetWindowTextW(hUrlEdit, url, 1024);
    GetWindowTextW(hOutputEdit, outputPath, MAX_PATH);
    
    if (wcslen(url) == 0) {
        MessageBoxW(hMainWindow, L"Введите URL!", L"Ошибка", MB_ICONERROR);
        isDownloading = false;
        EnableWindow(hDownloadBtn, TRUE);
        SetWindowTextW(hDownloadBtn, L"Скачать");
        return;
    }
    
    // Показываем лог если скрыт
    if (!isLogVisible) {
        ToggleLog();
    }
    
    ClearLog();
    AppendLog(L"=== Начало загрузки ===");
    
    int qualityIdx = SendMessage(hQualityCombo, CB_GETCURSEL, 0, 0);
    int formatIdx = SendMessage(hFormatCombo, CB_GETCURSEL, 0, 0);
    
    std::wstring command = L"yt-dlp ";
    
    // Формат
    if (formatIdx == 1) {
        command += L"-x --audio-format mp3 ";
        AppendLog(L"Формат: Только аудио (MP3)");
    } else if (formatIdx == 2) {
        command += L"-f \"bv*\" ";
        AppendLog(L"Формат: Только видео");
    } else {
        AppendLog(L"Формат: Видео + Аудио");
    }
    
    // Качество
    const wchar_t* qualityNames[] = {
        L"4K (2160p)", L"1440p", L"1080p", L"720p", L"480p", L"Лучшее доступное"
    };
    AppendLog(std::wstring(L"Качество: ") + qualityNames[qualityIdx]);
    
    switch (qualityIdx) {
        case 0: command += L"-f \"bv*[height<=2160]+ba/b[height<=2160]\" "; break;
        case 1: command += L"-f \"bv*[height<=1440]+ba/b[height<=1440]\" "; break;
        case 2: command += L"-f \"bv*[height<=1080]+ba/b[height<=1080]\" "; break;
        case 3: command += L"-f \"bv*[height<=720]+ba/b[height<=720]\" "; break;
        case 4: command += L"-f \"bv*[height<=480]+ba/b[height<=480]\" "; break;
        case 5: break;
    }
    
    // Путь сохранения
    if (wcslen(outputPath) > 0) {
        command += L"-o \"" + std::wstring(outputPath) + L"/%(title)s.%(ext)s\" ";
        AppendLog(std::wstring(L"Путь: ") + outputPath);
    } else {
        AppendLog(L"Путь: Текущая папка");
    }
    
    command += L"--newline --progress ";
    command += L"\"" + std::wstring(url) + L"\"";
    
    AppendLog(L"");
    AppendLog(L"--- Вывод yt-dlp ---");
    
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    SendMessageW(hStatusText, WM_SETTEXT, 0, (LPARAM)L"Загрузка...");
    
    std::wstring result = ExecuteCommand(command);
    
    if (result == L"SUCCESS") {
        SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
        MessageBoxW(hMainWindow, L"Загрузка завершена успешно!", L"Успех", MB_ICONINFORMATION);
        SendMessageW(hStatusText, WM_SETTEXT, 0, (LPARAM)L"✓ Готово!");
        AppendLog(L"");
        AppendLog(L"=== Загрузка завершена успешно! ===");
    } else {
        SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
        MessageBoxW(hMainWindow, L"Произошла ошибка! Проверьте лог.", L"Ошибка", MB_ICONERROR);
        SendMessageW(hStatusText, WM_SETTEXT, 0, (LPARAM)L"✗ Ошибка!");
        AppendLog(L"");
        AppendLog(L"=== Ошибка загрузки! ===");
    }
    
    isDownloading = false;
    EnableWindow(hDownloadBtn, TRUE);
    SetWindowTextW(hDownloadBtn, L"Скачать");
}

// Обработчик выбора папки
void BrowseFolder() {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hMainWindow;
    bi.lpszTitle = L"Выберите папку для сохранения";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            SetWindowTextW(hOutputEdit, path);
        }
        CoTaskMemFree(pidl);
    }
}

// Обработчик сообщений
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
        {
            HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            // GroupBox для основных настроек
            HWND hGroupMain = CreateWindowW(L"BUTTON", L"Основные настройки", 
                                           WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                                           10, 5, 560, 130, hwnd, NULL, NULL, NULL);
            SendMessage(hGroupMain, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // URL
            CreateWindowW(L"STATIC", L"URL видео:", WS_VISIBLE | WS_CHILD,
                         25, 25, 100, 20, hwnd, NULL, NULL, NULL);
            hUrlEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                                      WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                      25, 48, 530, 24, hwnd, NULL, NULL, NULL);
            
            // Путь сохранения
            CreateWindowW(L"STATIC", L"Папка сохранения:", WS_VISIBLE | WS_CHILD,
                         25, 80, 150, 20, hwnd, NULL, NULL, NULL);
            hOutputEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY,
                                         25, 103, 445, 24, hwnd, NULL, NULL, NULL);
            hBrowseBtn = CreateWindowW(L"BUTTON", L"Обзор...", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                       478, 103, 77, 24, hwnd, (HMENU)IDC_BROWSE, NULL, NULL);
            
            // GroupBox для параметров качества
            HWND hGroupQuality = CreateWindowW(L"BUTTON", L"Параметры загрузки", 
                                              WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                                              10, 143, 560, 75, hwnd, NULL, NULL, NULL);
            SendMessage(hGroupQuality, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // Качество
            CreateWindowW(L"STATIC", L"Качество:", WS_VISIBLE | WS_CHILD,
                         25, 165, 100, 20, hwnd, NULL, NULL, NULL);
            hQualityCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", 
                                           WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                                           25, 188, 250, 200, hwnd, NULL, NULL, NULL);
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"4K (2160p)");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1440p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1080p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"720p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"480p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"Лучшее доступное");
            SendMessage(hQualityCombo, CB_SETCURSEL, 2, 0);
            
            // Формат
            CreateWindowW(L"STATIC", L"Формат:", WS_VISIBLE | WS_CHILD,
                         305, 165, 100, 20, hwnd, NULL, NULL, NULL);
            hFormatCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", 
                                          WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                                          305, 188, 250, 200, hwnd, NULL, NULL, NULL);
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Видео + Аудио");
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Только аудио (MP3)");
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Только видео");
            SendMessage(hFormatCombo, CB_SETCURSEL, 0, 0);
            
            // Кнопка загрузки
            hDownloadBtn = CreateWindowW(L"BUTTON", L"Скачать", 
                                        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                        10, 228, 560, 40, hwnd, (HMENU)IDC_DOWNLOAD, NULL, NULL);
            
            // Прогресс бар
            hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                                          WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
                                          10, 278, 560, 28, hwnd, NULL, NULL, NULL);
            SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
            
            // Статус
            hStatusText = CreateWindowW(L"STATIC", L"Готов к загрузке", 
                                       WS_VISIBLE | WS_CHILD | SS_CENTER,
                                       10, 315, 560, 22, hwnd, NULL, NULL, NULL);
            
            // Кнопка показать/скрыть лог
            hToggleLogBtn = CreateWindowW(L"BUTTON", L"▼ Показать лог", 
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                         10, 345, 560, 28, hwnd, (HMENU)IDC_TOGGLE_LOG, NULL, NULL);
            
            // Лог (скрыт по умолчанию)
            hLogEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                                      WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                                      10, 383, 560, 240, hwnd, NULL, NULL, NULL);
            
            // Установка моноширинного шрифта для лога
            HFONT hLogFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
            SendMessage(hLogEdit, WM_SETFONT, (WPARAM)hLogFont, TRUE);
            
            // Установка шрифта для всех элементов кроме лога
            EnumChildWindows(hwnd, [](HWND hwndChild, LPARAM lParam) -> BOOL {
                if (hwndChild != hLogEdit) {
                    SendMessage(hwndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
                }
                return TRUE;
            }, (LPARAM)hFont);
            
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_DOWNLOAD && HIWORD(wParam) == BN_CLICKED) {
                if (!isDownloading) {
                    isDownloading = true;
                    EnableWindow(hDownloadBtn, FALSE);
                    SetWindowTextW(hDownloadBtn, L"Загрузка...");
                    std::thread(DownloadThread).detach();
                }
            }
            else if (LOWORD(wParam) == IDC_BROWSE && HIWORD(wParam) == BN_CLICKED) {
                BrowseFolder();
            }
            else if (LOWORD(wParam) == IDC_TOGGLE_LOG && HIWORD(wParam) == BN_CLICKED) {
                ToggleLog();
            }
            break;
        
        case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            SetBkColor(hdcStatic, GetSysColor(COLOR_WINDOW));
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);
    
    CoInitialize(NULL);
    
    const wchar_t CLASS_NAME[] = L"YtDlpGuiClass";
    
    WNDCLASSW wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassW(&wc);
    
    hMainWindow = CreateWindowExW(
        0,
        CLASS_NAME,
        L"yt-dlp GUI - Загрузчик видео",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, normalHeight,
        NULL, NULL, hInstance, NULL
    );
    
    if (hMainWindow == NULL) {
        return 0;
    }
    
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
    
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    CoUninitialize();
    return 0;
}
