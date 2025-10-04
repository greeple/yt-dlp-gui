#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <thread>
#include "resource.h"
#include "settings.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Глобальные переменные
HWND hMainWindow;
HWND hUrlEdit, hOutputEdit, hQualityCombo, hFormatCombo;
HWND hDownloadBtn, hBrowseBtn, hProgressBar, hStatusText;
HWND hLogEdit, hToggleLogBtn, hPresetCombo;
HWND hBtnSavePreset, hBtnDeletePreset;

bool isDownloading = false;
bool isLogVisible = false;
int normalHeight = 420;
int expandedHeight = 670;

Settings settings;
std::wstring currentPreset;

// Конвертер UTF-8 <-> Wide String
std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size <= 0) return std::wstring();
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result;
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (size <= 0) return std::string();
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

void ClearLog() {
    SetWindowTextW(hLogEdit, L"");
}

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

// Загрузка настроек в UI
void LoadSettingsToUI() {
    SetWindowTextW(hOutputEdit, settings.outputPath.c_str());
    SendMessage(hQualityCombo, CB_SETCURSEL, settings.quality, 0);
    SendMessage(hFormatCombo, CB_SETCURSEL, settings.format, 0);
}

// Сохранение настроек из UI
void SaveSettingsFromUI() {
    wchar_t buffer[1024];
    
    GetWindowTextW(hOutputEdit, buffer, 1024);
    settings.outputPath = buffer;
    
    settings.quality = SendMessage(hQualityCombo, CB_GETCURSEL, 0, 0);
    settings.format = SendMessage(hFormatCombo, CB_GETCURSEL, 0, 0);
}

// Загрузка пресетов в комбобокс
void LoadPresetsToCombo() {
    SendMessage(hPresetCombo, CB_RESETCONTENT, 0, 0);
    SendMessageW(hPresetCombo, CB_ADDSTRING, 0, (LPARAM)L"[Без пресета - использовать настройки]");
    
    auto profiles = settings.GetProfiles();
    for (const auto& profile : profiles) {
        SendMessageW(hPresetCombo, CB_ADDSTRING, 0, (LPARAM)profile.c_str());
    }
    
    SendMessage(hPresetCombo, CB_SETCURSEL, 0, 0);
}

// Построение команды yt-dlp
std::wstring BuildCommand(const std::wstring& url) {
    std::wstring command = L"yt-dlp ";
    
    // Если выбран пресет, загружаем его настройки
    int presetIdx = SendMessage(hPresetCombo, CB_GETCURSEL, 0, 0);
    if (presetIdx > 0) {
        wchar_t presetName[256];
        SendMessageW(hPresetCombo, CB_GETLBTEXT, presetIdx, (LPARAM)presetName);
        
        Settings presetSettings;
        presetSettings.Load(presetName);
        settings = presetSettings;
    } else {
        SaveSettingsFromUI();
    }
    
    // Формат
    if (settings.format == 1) {
        command += L"-x --audio-format mp3 ";
    } else if (settings.format == 2) {
        command += L"-f \"bv*\" ";
    }
    
    // Качество
    switch (settings.quality) {
        case 0: command += L"-f \"bv*[height<=2160]+ba/b[height<=2160]\" "; break;
        case 1: command += L"-f \"bv*[height<=1440]+ba/b[height<=1440]\" "; break;
        case 2: command += L"-f \"bv*[height<=1080]+ba/b[height<=1080]\" "; break;
        case 3: command += L"-f \"bv*[height<=720]+ba/b[height<=720]\" "; break;
        case 4: command += L"-f \"bv*[height<=480]+ba/b[height<=480]\" "; break;
    }
    
    // Вывод
    if (!settings.outputPath.empty()) {
        command += L"-o \"" + settings.outputPath + L"/%(title)s.%(ext)s\" ";
    }
    
    // Прогресс
    command += L"--newline --progress --encoding UTF-8 ";
    
    // URL
    command += L"\"" + url + L"\"";
    
    return command;
}

// Выполнение команды с правильной кодировкой UTF-8
std::wstring ExecuteCommand(const std::wstring& command) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return L"ERROR: Pipe creation failed";
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
        return L"ERROR: Process creation failed";
    }
    
    CloseHandle(hWritePipe);
    
    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
        
        // Преобразуем из UTF-8 в Wide для корректного отображения
        std::wstring wBuffer = Utf8ToWide(buffer);
        AppendLog(wBuffer);
        
        // Парсинг прогресса
        std::string line = buffer;
        size_t pos = line.find("%");
        if (pos != std::string::npos && pos > 0) {
            size_t start = pos;
            while (start > 0 && (isdigit(line[start-1]) || line[start-1] == '.')) {
                start--;
            }
            if (start < pos) {
                try {
                    std::string percentStr = line.substr(start, pos - start);
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
    wchar_t url[2048];
    GetWindowTextW(hUrlEdit, url, 2048);
    
    if (wcslen(url) == 0) {
        MessageBoxW(hMainWindow, L"Введите URL!", L"Ошибка", MB_ICONERROR);
        isDownloading = false;
        EnableWindow(hDownloadBtn, TRUE);
        SetWindowTextW(hDownloadBtn, L"Скачать");
        return;
    }
    
    if (!isLogVisible) {
        ToggleLog();
    }
    
    ClearLog();
    AppendLog(L"=== Начало загрузки ===");
    AppendLog(L"URL: " + std::wstring(url));
    AppendLog(L"");
    
    std::wstring command = BuildCommand(url);
    AppendLog(L"Команда: " + command);
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
            
            // Пресеты
            HWND hGroupPreset = CreateWindowW(L"BUTTON", L"Пресеты", 
                                             WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                                             10, 5, 560, 50, hwnd, NULL, NULL, NULL);
            SendMessage(hGroupPreset, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            CreateWindowW(L"STATIC", L"Пресет:", WS_VISIBLE | WS_CHILD,
                         25, 25, 60, 20, hwnd, NULL, NULL, NULL);
            hPresetCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", 
                                          WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                                          90, 23, 280, 200, hwnd, (HMENU)IDC_PRESET_COMBO, NULL, NULL);
            
            hBtnSavePreset = CreateWindowW(L"BUTTON", L"Сохранить", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                          375, 23, 90, 24, hwnd, (HMENU)IDC_SAVE_PRESET, NULL, NULL);
            
            hBtnDeletePreset = CreateWindowW(L"BUTTON", L"Удалить", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                            470, 23, 85, 24, hwnd, (HMENU)IDC_DELETE_PRESET, NULL, NULL);
            
            // GroupBox для основных настроек
            HWND hGroupMain = CreateWindowW(L"BUTTON", L"Основные настройки", 
                                           WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                                           10, 60, 560, 160, hwnd, NULL, NULL, NULL);
            SendMessage(hGroupMain, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // URL
            CreateWindowW(L"STATIC", L"URL видео:", WS_VISIBLE | WS_CHILD,
                         25, 80, 100, 20, hwnd, NULL, NULL, NULL);
            hUrlEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                                      WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                      25, 103, 530, 24, hwnd, NULL, NULL, NULL);
            
            // Путь сохранения
            CreateWindowW(L"STATIC", L"Папка сохранения:", WS_VISIBLE | WS_CHILD,
                         25, 135, 150, 20, hwnd, NULL, NULL, NULL);
            hOutputEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY,
                                         25, 158, 445, 24, hwnd, NULL, NULL, NULL);
            hBrowseBtn = CreateWindowW(L"BUTTON", L"Обзор...", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                       478, 158, 77, 24, hwnd, (HMENU)IDC_BROWSE, NULL, NULL);
            
            // Качество и формат
            CreateWindowW(L"STATIC", L"Качество:", WS_VISIBLE | WS_CHILD,
                         25, 190, 100, 20, hwnd, NULL, NULL, NULL);
            hQualityCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", 
                                           WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                                           90, 188, 180, 200, hwnd, NULL, NULL, NULL);
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"4K (2160p)");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1440p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1080p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"720p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"480p");
            SendMessageW(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"Лучшее доступное");
            SendMessage(hQualityCombo, CB_SETCURSEL, 2, 0);
            
            CreateWindowW(L"STATIC", L"Формат:", WS_VISIBLE | WS_CHILD,
                         290, 190, 100, 20, hwnd, NULL, NULL, NULL);
            hFormatCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", 
                                          WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                                          350, 188, 205, 200, hwnd, NULL, NULL, NULL);
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Видео + Аудио");
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Только аудио (MP3)");
            SendMessageW(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Только видео");
            SendMessage(hFormatCombo, CB_SETCURSEL, 0, 0);
            
            // Кнопка загрузки
            hDownloadBtn = CreateWindowW(L"BUTTON", L"Скачать", 
                                        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                        10, 230, 560, 40, hwnd, (HMENU)IDC_DOWNLOAD, NULL, NULL);
            
            // Прогресс бар
            hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                                          WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
                                          10, 280, 560, 28, hwnd, NULL, NULL, NULL);
            SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
            
            // Статус
            hStatusText = CreateWindowW(L"STATIC", L"Готов к загрузке", 
                                       WS_VISIBLE | WS_CHILD | SS_CENTER,
                                       10, 317, 560, 22, hwnd, NULL, NULL, NULL);
            
            // Кнопка показать/скрыть лог
            hToggleLogBtn = CreateWindowW(L"BUTTON", L"▼ Показать лог", 
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                         10, 347, 560, 28, hwnd, (HMENU)IDC_TOGGLE_LOG, NULL, NULL);
            
            // Лог (скрыт по умолчанию)
            hLogEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                                      WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                                      10, 385, 560, 240, hwnd, NULL, NULL, NULL);
            
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
            
            // Загрузка настроек и пресетов
            settings.Load();
            LoadSettingsToUI();
            LoadPresetsToCombo();
            
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_DOWNLOAD && HIWORD(wParam) == BN_CLICKED) {
                if (!isDownloading) {
                    isDownloading = true;
                    EnableWindow(hDownloadBtn, FALSE);
                    SetWindowTextW(hDownloadBtn, L"Загрузка...");
                    
                    SaveSettingsFromUI();
                    settings.Save();
                    
                    std::thread(DownloadThread).detach();
                }
            }
            else if (LOWORD(wParam) == IDC_BROWSE && HIWORD(wParam) == BN_CLICKED) {
                BrowseFolder();
            }
            else if (LOWORD(wParam) == IDC_TOGGLE_LOG && HIWORD(wParam) == BN_CLICKED) {
                ToggleLog();
            }
            else if (LOWORD(wParam) == IDC_PRESET_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                int idx = SendMessage(hPresetCombo, CB_GETCURSEL, 0, 0);
                if (idx > 0) {
                    wchar_t presetName[256];
                    SendMessageW(hPresetCombo, CB_GETLBTEXT, idx, (LPARAM)presetName);
                    
                    Settings presetSettings;
                    presetSettings.Load(presetName);
                    settings = presetSettings;
                    LoadSettingsToUI();
                } else {
                    settings.Load();
                    LoadSettingsToUI();
                }
            }
            else if (LOWORD(wParam) == IDC_SAVE_PRESET && HIWORD(wParam) == BN_CLICKED) {
                wchar_t presetName[256] = L"";
                
                if (MessageBoxW(hwnd, L"Введите имя пресета в следующем окне", L"Сохранить пресет", MB_OKCANCEL) == IDOK) {
                    // Используем простой InputBox через VBScript
                    FILE* fp = _wfopen(L"input.vbs", L"w");
                    if (fp) {
                        fwprintf(fp, L"Dim result\n");
                        fwprintf(fp, L"result = InputBox(\"Введите имя пресета:\", \"Сохранить пресет\")\n");
                        fwprintf(fp, L"WScript.Echo result\n");
                        fclose(fp);
                        
                        FILE* pipe = _wpopen(L"cscript //nologo input.vbs", L"r");
                        if (pipe) {
                            if (fgetws(presetName, 256, pipe)) {
                                // Убираем перевод строки
                                presetName[wcscspn(presetName, L"\r\n")] = 0;
                                
                                if (wcslen(presetName) > 0) {
                                    SaveSettingsFromUI();
                                    settings.Save(presetName);
                                    LoadPresetsToCombo();
                                    
                                    // Выбрать сохраненный пресет
                                    int count = SendMessage(hPresetCombo, CB_GETCOUNT, 0, 0);
                                    for (int i = 1; i < count; i++) {
                                        wchar_t text[256];
                                        SendMessageW(hPresetCombo, CB_GETLBTEXT, i, (LPARAM)text);
                                        if (wcscmp(text, presetName) == 0) {
                                            SendMessage(hPresetCombo, CB_SETCURSEL, i, 0);
                                            break;
                                        }
                                    }
                                    
                                    MessageBoxW(hwnd, L"Пресет успешно сохранен!", L"Успех", MB_ICONINFORMATION);
                                }
                            }
                            _pclose(pipe);
                        }
                        DeleteFileW(L"input.vbs");
                    }
                }
            }
            else if (LOWORD(wParam) == IDC_DELETE_PRESET && HIWORD(wParam) == BN_CLICKED) {
                int idx = SendMessage(hPresetCombo, CB_GETCURSEL, 0, 0);
                if (idx > 0) {
                    wchar_t presetName[256];
                    SendMessageW(hPresetCombo, CB_GETLBTEXT, idx, (LPARAM)presetName);
                    
                    std::wstring msg = L"Удалить пресет \"" + std::wstring(presetName) + L"\"?";
                    if (MessageBoxW(hwnd, msg.c_str(), L"Подтверждение", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        settings.DeleteProfile(presetName);
                        LoadPresetsToCombo();
                        MessageBoxW(hwnd, L"Пресет удален!", L"Успех", MB_ICONINFORMATION);
                    }
                }
            }
            break;
        
        case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            SetBkColor(hdcStatic, GetSysColor(COLOR_WINDOW));
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        
        case WM_CLOSE:
        {
            SaveSettingsFromUI();
            settings.Save();
            DestroyWindow(hwnd);
            return 0;
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
    icex.dwICC = ICC_PROGRESS_CLASS | ICC_TAB_CLASSES;
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
        if (!IsDialogMessage(hMainWindow, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    CoUninitialize();
    return 0;
}
