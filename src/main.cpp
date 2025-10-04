#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <thread>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Глобальные переменные
HWND hMainWindow;
HWND hUrlEdit, hOutputEdit, hQualityCombo, hFormatCombo;
HWND hDownloadBtn, hBrowseBtn, hProgressBar, hStatusText;
bool isDownloading = false;

// Функция для выполнения команды и перехвата вывода
std::string ExecuteCommand(const std::string& command) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return "Error creating pipe";
    }
    
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;
    
    PROCESS_INFORMATION pi;
    
    if (!CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, TRUE, 
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return "Error starting process";
    }
    
    CloseHandle(hWritePipe);
    
    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
        
        // Обновление статуса в GUI
        SendMessageA(hStatusText, WM_SETTEXT, 0, (LPARAM)buffer);
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);
    
    return output;
}

// Поток загрузки
void DownloadThread() {
    char url[1024], outputPath[MAX_PATH];
    GetWindowTextA(hUrlEdit, url, sizeof(url));
    GetWindowTextA(hOutputEdit, outputPath, sizeof(outputPath));
    
    if (strlen(url) == 0) {
        MessageBoxA(hMainWindow, "Введите URL!", "Ошибка", MB_ICONERROR);
        isDownloading = false;
        EnableWindow(hDownloadBtn, TRUE);
        SetWindowTextA(hDownloadBtn, "Скачать");
        return;
    }
    
    int qualityIdx = SendMessage(hQualityCombo, CB_GETCURSEL, 0, 0);
    int formatIdx = SendMessage(hFormatCombo, CB_GETCURSEL, 0, 0);
    
    std::string command = "yt-dlp ";
    
    // Формат
    if (formatIdx == 1) { // Только аудио
        command += "-x --audio-format mp3 ";
    } else if (formatIdx == 2) { // Только видео
        command += "-f \"bv*\" ";
    }
    
    // Качество
    switch (qualityIdx) {
        case 0: command += "-f \"bv*[height<=2160]+ba/b[height<=2160]\" "; break; // 4K
        case 1: command += "-f \"bv*[height<=1440]+ba/b[height<=1440]\" "; break; // 1440p
        case 2: command += "-f \"bv*[height<=1080]+ba/b[height<=1080]\" "; break; // 1080p
        case 3: command += "-f \"bv*[height<=720]+ba/b[height<=720]\" "; break;  // 720p
        case 4: command += "-f \"bv*[height<=480]+ba/b[height<=480]\" "; break;  // 480p
        case 5: break; // Лучшее качество (по умолчанию)
    }
    
    // Путь сохранения
    if (strlen(outputPath) > 0) {
        command += "-o \"" + std::string(outputPath) + "/%(title)s.%(ext)s\" ";
    }
    
    command += "--newline --progress ";
    command += "\"" + std::string(url) + "\"";
    
    SendMessage(hProgressBar, PBM_SETMARQUEE, TRUE, 0);
    SendMessageA(hStatusText, WM_SETTEXT, 0, (LPARAM)"Загрузка...");
    
    std::string result = ExecuteCommand(command);
    
    SendMessage(hProgressBar, PBM_SETMARQUEE, FALSE, 0);
    
    if (result.find("ERROR") != std::string::npos || result.find("Error") != std::string::npos) {
        MessageBoxA(hMainWindow, "Ошибка загрузки! Проверьте консоль.", "Ошибка", MB_ICONERROR);
        SendMessageA(hStatusText, WM_SETTEXT, 0, (LPARAM)"Ошибка!");
    } else {
        MessageBoxA(hMainWindow, "Загрузка завершена!", "Успех", MB_ICONINFORMATION);
        SendMessageA(hStatusText, WM_SETTEXT, 0, (LPARAM)"Готово!");
    }
    
    isDownloading = false;
    EnableWindow(hDownloadBtn, TRUE);
    SetWindowTextA(hDownloadBtn, "Скачать");
}

// Обработчик выбора папки
void BrowseFolder() {
    BROWSEINFOA bi = { 0 };
    bi.hwndOwner = hMainWindow;
    bi.lpszTitle = "Выберите папку для сохранения";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != NULL) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            SetWindowTextA(hOutputEdit, path);
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
            CreateWindowA("STATIC", "URL видео:", WS_VISIBLE | WS_CHILD,
                         10, 10, 100, 20, hwnd, NULL, NULL, NULL);
            hUrlEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                    10, 35, 560, 25, hwnd, NULL, NULL, NULL);
            
            // Путь сохранения
            CreateWindowA("STATIC", "Папка сохранения:", WS_VISIBLE | WS_CHILD,
                         10, 70, 150, 20, hwnd, NULL, NULL, NULL);
            hOutputEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                       10, 95, 470, 25, hwnd, NULL, NULL, NULL);
            hBrowseBtn = CreateWindowA("BUTTON", "Обзор...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                       490, 95, 80, 25, hwnd, (HMENU)IDC_BROWSE, NULL, NULL);
            
            // Качество
            CreateWindowA("STATIC", "Качество:", WS_VISIBLE | WS_CHILD,
                         10, 130, 100, 20, hwnd, NULL, NULL, NULL);
            hQualityCombo = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                         10, 155, 200, 200, hwnd, NULL, NULL, NULL);
            SendMessageA(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)"4K (2160p)");
            SendMessageA(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)"1440p");
            SendMessageA(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)"1080p");
            SendMessageA(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)"720p");
            SendMessageA(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)"480p");
            SendMessageA(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)"Лучшее доступное");
            SendMessage(hQualityCombo, CB_SETCURSEL, 2, 0); // 1080p по умолчанию
            
            // Формат
            CreateWindowA("STATIC", "Формат:", WS_VISIBLE | WS_CHILD,
                         230, 130, 100, 20, hwnd, NULL, NULL, NULL);
            hFormatCombo = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                        230, 155, 200, 200, hwnd, NULL, NULL, NULL);
            SendMessageA(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)"Видео + Аудио");
            SendMessageA(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)"Только аудио (MP3)");
            SendMessageA(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)"Только видео");
            SendMessage(hFormatCombo, CB_SETCURSEL, 0, 0);
            
            // Кнопка загрузки
            hDownloadBtn = CreateWindowA("BUTTON", "Скачать", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                        10, 195, 560, 35, hwnd, (HMENU)IDC_DOWNLOAD, NULL, NULL);
            
            // Прогресс бар
            hProgressBar = CreateWindowExA(0, PROGRESS_CLASSA, NULL,
                                          WS_VISIBLE | WS_CHILD | PBS_MARQUEE,
                                          10, 245, 560, 25, hwnd, NULL, NULL, NULL);
            
            // Статус
            hStatusText = CreateWindowA("STATIC", "Готов к загрузке", WS_VISIBLE | WS_CHILD | SS_CENTER,
                                       10, 280, 560, 20, hwnd, NULL, NULL, NULL);
            
            // Установка шрифта
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
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
                    SetWindowTextA(hDownloadBtn, "Загрузка...");
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
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Инициализация Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);
    
    CoInitialize(NULL);
    
    // Регистрация класса окна
    const char CLASS_NAME[] = "YtDlpGuiClass";
    
    WNDCLASSA wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassA(&wc);
    
    // Создание окна
    hMainWindow = CreateWindowExA(
        0,
        CLASS_NAME,
        "yt-dlp GUI",
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
