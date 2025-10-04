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
bool isDownloading = false;

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

std::string WideToAnsi(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string result(size, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &result[0], size, NULL, NULL);
    return result;
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
    
    // Создаем изменяемую копию команды
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
    
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
        
        // Конвертируем в UTF-8 для отображения
        std::wstring status = Utf8ToWide(output.substr(output.length() > 100 ? output.length() - 100 : 0));
        SendMessageW(hStatusText, WM_SETTEXT, 0, (LPARAM)status.c_str());
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);
    
    return Utf8ToWide(output);
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
    
    int qualityIdx = SendMessage(hQualityCombo, CB_GETCURSEL, 0, 0);
    int formatIdx = SendMessage(hFormatCombo, CB_GETCURSEL, 0, 0);
    
    std::wstring command = L"yt-dlp ";
    
    // Формат
    if (formatIdx == 1) { // Только аудио
        command += L"-x --audio-format mp3 ";
    } else if (formatIdx == 2) { // Только видео
        command += L"-f \"bv*\" ";
    }
    
    // Качество
    switch (qualityIdx) {
        case 0: command += L"-f \"bv*[height<=2160]+ba/b[height<=2160]\" "; break; // 4K
        case 1: command += L"-f \"bv*[height<=1440]+ba/b[height<=1440]\" "; break; // 1440p
        case 2: command += L"-f \"bv*[height<=1080]+ba/b[height<=1080]\" "; break; // 1080p
        case 3: command += L"-f \"bv*[height<=720]+ba/b[height<=720]\" "; break;  // 720p
        case 4: command += L"-f \"bv*[height<=480]+ba/b[height<=480]\" "; break;  // 480p
        case 5: break; // Лучшее качество
    }
    
    // Путь сохранения
    if (wcslen(outputPath) > 0) {
        command += L"-o \"" + std::wstring(outputPath) + L"/%(title)s.%(ext)s\" ";
    }
    
    command += L"--newline --progress ";
    command += L"\"" + std::wstring(url) + L"\"";
    
    SendMessage(hProgressBar, PBM_SETMARQUEE, TRUE, 0);
    SendMessageW(hStatusText, WM_SETTEXT, 0, (LPARAM)L"Загрузка...");
    
    std::wstring result = ExecuteCommand(command);
    
    SendMessage(hProgressBar, PBM_SETMARQUEE, FALSE, 0);
    
    if (result.find(L"ERROR") != std::wstring::npos || result.find(L"Error") != std::wstring::npos) {
        MessageBoxW(hMainWindow, L"Ошибка загрузки! Проверьте вывод программы.", L"Ошибка", MB_ICONERROR);
        SendMessageW(hStatusText, WM_SETTEXT, 0, (LPARAM)L"Ошибка!");
    } else {
        MessageBoxW(hMainWindow, L"Загрузка завершена!", L"Успех", MB_ICONINFORMATION);
        SendMessageW(hStatusText, WM_SETTEXT, 0, (LPARAM)L"Готово!");
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
            // URL
            CreateWindowW(L"STATIC", L"URL видео:", WS_VISIBLE | WS_CHILD,
                         10, 10, 100, 20, hwnd, NULL, NULL, NULL);
            hUrlEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                    10, 35, 560, 25, hwnd, NULL, NULL, NULL);
            
            // Путь сохранения
            CreateWindowW(L"STATIC", L"Папка сохранения:", WS_VISIBLE | WS_CHILD,
                         10, 70, 150, 20, hwnd, NULL, NULL, NULL);
            hOutputEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                       10, 95, 470, 25, hwnd, NULL, NULL, NULL);
            hBrowseBtn = CreateWindowW(L"BUTTON", L"Обзор...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                       490, 95, 80, 25, hwnd, (HMENU)IDC_BROWSE, NULL, NULL);
            
            // Качество
            CreateWindowW(L"STATIC", L"Качество:", WS_VISIBLE | WS_CHILD,
                         10, 130, 100, 20, hwnd, NULL, NULL, NULL);
            hQualityCombo = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                         10, 155, 200, 200, hwnd, NULL, NULL, NULL);
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"4K (2160p)");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1440p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1080p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"720p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"480p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"Лучшее доступное");
            SendMessage(hQualityCombo, CB_SETCURSEL, 2, 0); // 1080p по умолчанию
            
            // Формат
            CreateWindowW(L"STATIC", L"Формат:", WS_VISIBLE | WS_CHILD,
                         230, 130, 100, 20, hwnd, NULL, NULL, NULL);
            hFormatCombo = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                        230, 155, 200, 200, hwnd, NULL, NULL, NULL);
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Видео + Аудио");
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Только аудио (MP3)");
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Только видео");
            SendMessage(hFormatCombo, CB_SETCURSEL, 0, 0);
            
            // Кнопка загрузки
            hDownloadBtn = CreateWindowW(L"BUTTON", L"Скачать", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                        10, 195, 560, 35, hwnd, (HMENU)IDC_DOWNLOAD, NULL, NULL);
            
            // Прогресс бар
            hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                                          WS_VISIBLE | WS_CHILD | PBS_MARQUEE,
                                          10, 245, 560, 25, hwnd, NULL, NULL, NULL);
            
            // Статус
            hStatusText = CreateWindowW(L"STATIC", L"Готов к загрузке", WS_VISIBLE | WS_CHILD | SS_CENTER,
                                       10, 280, 560, 20, hwnd, NULL, NULL, NULL);
            
            // Установка шрифта
            HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            EnumChildWindows(hwnd, [](HWND hwndChild, LPARAM lParam) -> BOOL {
                SendMessage(hwndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
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
            break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Устанавливаем UTF-8 кодовую страницу
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // Инициализация Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);
    
    CoInitialize(NULL);
    
    // Регистрация класса окна
    const wchar_t CLASS_NAME[] = L"YtDlpGuiClass";
    
    WNDCLASSW wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassW(&wc);
    
    // Создание окна
    hMainWindow = CreateWindowExW(
        0,
        CLASS_NAME,
        L"yt-dlp GUI",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 360,
        NULL, NULL, hInstance, NULL
    );
    
    if (hMainWindow == NULL) {
        return 0;
    }
    
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
    
    // Цикл сообщений
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    CoUninitialize();
    return 0;
}
