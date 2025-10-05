#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <sstream>
#include <thread>

// FIX: Добавляем недостающие определения GUIDs для MinGW
#include <guiddef.h>
DEFINE_GUID(FOLDERID_Desktop, 0xB4BFCC3A, 0xDB2C, 0x424C, 0xB0, 0x29, 0x7F, 0xE9, 0x9A, 0x87, 0xC6, 0x41);
DEFINE_GUID(CLSID_FileOpenDialog, 0xDC1C5A9C, 0xE88A, 0x4dde, 0xA5, 0xA1, 0x60, 0xF8, 0x2A, 0x20, 0xAE, 0xF7);
DEFINE_GUID(IID_IFileOpenDialog, 0xd57c7288, 0xd4AD, 0x4768, 0xBE, 0x02, 0x9D, 0x96, 0x95, 0x32, 0xD9, 0x60);
// ---

#pragma comment(lib, "comctl32.lib")

// --- Глобальные переменные и константы ---
#define IDC_URL_EDIT 101
#define IDC_DOWNLOAD_BUTTON 102
#define IDC_BROWSE_BUTTON 103
#define IDC_PATH_EDIT 104
#define IDC_PROGRESS_BAR 105
#define IDC_LOG_TOGGLE_BUTTON 106
#define IDC_LOG_EDIT 107
#define IDC_TAB_CONTROL 108

// IDs для элементов на вкладках
#define IDC_PROXY_EDIT 201
#define IDC_BATCH_FILE_EDIT 202
#define IDC_BATCH_FILE_BUTTON 203
#define IDC_OUTPUT_TEMPLATE_EDIT 204
#define IDC_CONVERT_SUBS_COMBO 205
#define IDC_CONVERT_THUMB_COMBO 206
#define IDC_MERGE_FORMAT_COMBO 207
#define IDC_EMBED_SUBS_CHECK 208
#define IDC_EMBED_THUMB_CHECK 209
#define IDC_EMBED_META_CHECK 210
#define IDC_SPLIT_CHAPTERS_CHECK 211
#define IDC_COOKIES_BROWSER_COMBO 212

#define WM_UPDATE_PROGRESS (WM_APP + 1)
#define WM_APPEND_LOG (WM_APP + 2)
#define WM_DOWNLOAD_COMPLETE (WM_APP + 3)

HINSTANCE hInst;
HWND hMainWindow;
HWND hUrlEdit, hPathEdit, hDownloadButton, hBrowseButton, hProgressBar, hLogToggleButton, hLogEdit, hTabControl;
HWND hTabGeneral, hTabFormats, hTabMetadata, hTabAdvanced; // Окна-контейнеры для вкладок
std::vector<HWND> tabGeneralControls, tabFormatsControls, tabMetadataControls, tabAdvancedControls;

bool isLogVisible = false;
bool isDownloading = false;
const int LOG_HEIGHT = 150;
const int PADDING = 10;
const int CONTROL_HEIGHT = 25;

// --- Прототипы функций ---
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateControls(HWND);
void ResizeControls(HWND);
void OnDownloadClick();
void OnBrowseClick();
void OnToggleLogClick();
void SaveSettings();
void LoadSettings();
std::wstring GetIniPath();
void CreateTabsAndControls(HWND);
void ShowTabControls(int tabIndex);

// --- Поток для выполнения yt-dlp ---
struct DownloadParams {
    std::wstring command;
};

// Функция для конвертации вывода консоли (OEM) в UTF-16
std::wstring OemToWide(const std::string& str) {
    int size_needed = MultiByteToWideChar(GetConsoleOutputCP(), 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(GetConsoleOutputCP(), 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

void DownloadThread(DownloadParams params) {
    isDownloading = true;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi;
    if (!CreateProcessW(NULL, &params.command[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        PostMessage(hMainWindow, WM_APPEND_LOG, 0, (LPARAM)new std::wstring(L"Error: Failed to start yt-dlp.exe. Make sure it's in the same folder.\r\n"));
        PostMessage(hMainWindow, WM_DOWNLOAD_COMPLETE, 0, 0);
        CloseHandle(hWrite);
        CloseHandle(hRead);
        isDownloading = false;
        return;
    }

    CloseHandle(hWrite);

    char buffer[4096];
    DWORD bytesRead;
    std::string output;
    
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead != 0) {
        buffer[bytesRead] = '\0';
        output.append(buffer);

        // Поиск прогресса
        std::string s_buffer(buffer);
        size_t pos = s_buffer.rfind("[download]");
        if (pos != std::string::npos) {
            size_t percent_pos = s_buffer.find('%', pos);
            if (percent_pos != std::string::npos) {
                size_t start = s_buffer.rfind(' ', percent_pos) + 1;
                if (start != std::string::npos) {
                    std::string percent_str = s_buffer.substr(start, percent_pos - start);
                    try {
                        float percent = std::stof(percent_str);
                        PostMessage(hMainWindow, WM_UPDATE_PROGRESS, (WPARAM)percent, 0);
                    } catch (...) {}
                }
            }
        }
        
        // Отправляем чанками в главный поток, чтобы не перегружать сообщения
        size_t last_newline = output.rfind('\n');
        if (last_newline != std::string::npos) {
            std::string to_send = output.substr(0, last_newline + 1);
            output = output.substr(last_newline + 1);
            std::wstring* wstr = new std::wstring(OemToWide(to_send));
            PostMessage(hMainWindow, WM_APPEND_LOG, 0, (LPARAM)wstr);
        }
    }
    // Отправить остатки
    if (!output.empty()) {
        std::wstring* wstr = new std::wstring(OemToWide(output));
        PostMessage(hMainWindow, WM_APPEND_LOG, 0, (LPARAM)wstr);
    }


    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    PostMessage(hMainWindow, WM_DOWNLOAD_COMPLETE, 0, 0);
    isDownloading = false;
}


// --- WinMain ---
// FIX: Добавляем extern "C" для правильной линковки с MinGW
extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    hInst = hInstance;
    const wchar_t CLASS_NAME[] = L"YtDlpGuiClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    hMainWindow = CreateWindowExW(0, CLASS_NAME, L"yt-dlp GUI",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if (hMainWindow == NULL) {
        return 0;
    }

    ShowWindow(hMainWindow, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// --- WndProc ---
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            InitCommonControls(); // Инициализация общих элементов управления
            CreateControls(hWnd);
            LoadSettings();
            ResizeControls(hWnd);
            break;

        case WM_SIZE:
            ResizeControls(hWnd);
            break;

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case IDC_DOWNLOAD_BUTTON:
                    OnDownloadClick();
                    break;
                case IDC_BROWSE_BUTTON:
                    OnBrowseClick();
                    break;
                case IDC_LOG_TOGGLE_BUTTON:
                    OnToggleLogClick();
                    break;
                case IDC_BATCH_FILE_BUTTON: {
                    wchar_t filename[MAX_PATH] = { 0 };
                    OPENFILENAMEW ofn = { 0 };
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFile = filename;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileNameW(&ofn)) {
                        SetWindowTextW(GetDlgItem(hTabGeneral, IDC_BATCH_FILE_EDIT), filename);
                    }
                } break;
            }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            if (lpnm->idFrom == IDC_TAB_CONTROL && lpnm->code == TCN_SELCHANGE) {
                int iPage = TabCtrl_GetCurSel(hTabControl);
                ShowTabControls(iPage);
            }
            break;
        }

        case WM_UPDATE_PROGRESS:
            SendMessage(hProgressBar, PBM_SETPOS, wParam, 0);
            break;
            
        case WM_APPEND_LOG: {
            std::wstring* logText = (std::wstring*)lParam;
            int len = GetWindowTextLength(hLogEdit);
            SendMessage(hLogEdit, EM_SETSEL, len, len);
            SendMessage(hLogEdit, EM_REPLACESEL, 0, (LPARAM)logText->c_str());
            delete logText;
            break;
        }

        case WM_DOWNLOAD_COMPLETE:
            SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
            EnableWindow(hDownloadButton, TRUE);
            SetWindowText(hDownloadButton, L"Download");
            {
                std::wstring* msg = new std::wstring(L"\r\n--- DOWNLOAD FINISHED ---\r\n");
                PostMessage(hWnd, WM_APPEND_LOG, 0, (LPARAM)msg);
            }
            break;

        case WM_CLOSE:
            SaveSettings();
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// --- Реализация функций ---

void CreateTabsAndControls(HWND hWnd) {
    // Создание Tab Control
    hTabControl = CreateWindowEx(0, WC_TABCONTROL, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0, 0, 1, 1, hWnd, (HMENU)IDC_TAB_CONTROL, hInst, NULL);

    TCITEMW tie;
    tie.mask = TCIF_TEXT;
    
    const wchar_t* tabNames[] = { L"General", L"Formats", L"Metadata", L"Advanced" };
    for (int i = 0; i < 4; ++i) {
        tie.pszText = (LPWSTR)tabNames[i];
        TabCtrl_InsertItem(hTabControl, i, &tie);
    }

    // Создание контейнеров для вкладок
    hTabGeneral = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_BORDER | WS_GROUP, 0, 0, 1, 1, hWnd, NULL, hInst, NULL);
    hTabFormats = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_BORDER | WS_GROUP, 0, 0, 1, 1, hWnd, NULL, hInst, NULL);
    hTabMetadata = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_BORDER | WS_GROUP, 0, 0, 1, 1, hWnd, NULL, hInst, NULL);
    hTabAdvanced = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_BORDER | WS_GROUP, 0, 0, 1, 1, hWnd, NULL, hInst, NULL);
    
    // --- Вкладка 1: General ---
    tabGeneralControls.push_back(CreateWindowEx(0, L"STATIC", L"Output Template:", WS_CHILD | WS_VISIBLE, PADDING, PADDING, 120, CONTROL_HEIGHT, hTabGeneral, NULL, hInst, NULL));
    tabGeneralControls.push_back(CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, PADDING, 400, CONTROL_HEIGHT, hTabGeneral, (HMENU)IDC_OUTPUT_TEMPLATE_EDIT, hInst, NULL));
    tabGeneralControls.push_back(CreateWindowEx(0, L"STATIC", L"Batch File:", WS_CHILD | WS_VISIBLE, PADDING, PADDING * 2 + CONTROL_HEIGHT, 120, CONTROL_HEIGHT, hTabGeneral, NULL, hInst, NULL));
    tabGeneralControls.push_back(CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, PADDING * 2 + CONTROL_HEIGHT, 310, CONTROL_HEIGHT, hTabGeneral, (HMENU)IDC_BATCH_FILE_EDIT, hInst, NULL));
    tabGeneralControls.push_back(CreateWindowEx(0, L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 450, PADDING * 2 + CONTROL_HEIGHT, 80, CONTROL_HEIGHT, hTabGeneral, (HMENU)IDC_BATCH_FILE_BUTTON, hInst, NULL));
    tabGeneralControls.push_back(CreateWindowEx(0, L"STATIC", L"Proxy URL:", WS_CHILD | WS_VISIBLE, PADDING, PADDING * 3 + CONTROL_HEIGHT * 2, 120, CONTROL_HEIGHT, hTabGeneral, NULL, hInst, NULL));
    tabGeneralControls.push_back(CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, PADDING * 3 + CONTROL_HEIGHT * 2, 400, CONTROL_HEIGHT, hTabGeneral, (HMENU)IDC_PROXY_EDIT, hInst, NULL));
    
    // --- Вкладка 2: Formats ---
    tabFormatsControls.push_back(CreateWindowEx(0, L"STATIC", L"Convert Subs:", WS_CHILD | WS_VISIBLE, PADDING, PADDING, 120, CONTROL_HEIGHT, hTabFormats, NULL, hInst, NULL));
    HWND hConvertSubs = CreateWindowEx(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 130, PADDING, 150, 150, hTabFormats, (HMENU)IDC_CONVERT_SUBS_COMBO, hInst, NULL);
    const wchar_t* subs[] = { L"none", L"srt", L"ass", L"vtt", L"lrc" };
    for (const auto& s : subs) SendMessageW(hConvertSubs, CB_ADDSTRING, 0, (LPARAM)s);
    SendMessage(hConvertSubs, CB_SETCURSEL, 0, 0);
    tabFormatsControls.push_back(hConvertSubs);

    tabFormatsControls.push_back(CreateWindowEx(0, L"STATIC", L"Convert Thumb:", WS_CHILD | WS_VISIBLE, PADDING, PADDING * 2 + CONTROL_HEIGHT, 120, CONTROL_HEIGHT, hTabFormats, NULL, hInst, NULL));
    HWND hConvertThumb = CreateWindowEx(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 130, PADDING*2 + CONTROL_HEIGHT, 150, 150, hTabFormats, (HMENU)IDC_CONVERT_THUMB_COMBO, hInst, NULL);
    const wchar_t* thumbs[] = { L"none", L"jpg", L"png", L"webp" };
    for (const auto& t : thumbs) SendMessageW(hConvertThumb, CB_ADDSTRING, 0, (LPARAM)t);
    SendMessage(hConvertThumb, CB_SETCURSEL, 0, 0);
    tabFormatsControls.push_back(hConvertThumb);
    
    tabFormatsControls.push_back(CreateWindowEx(0, L"STATIC", L"Merge Format:", WS_CHILD | WS_VISIBLE, PADDING, PADDING * 3 + CONTROL_HEIGHT * 2, 120, CONTROL_HEIGHT, hTabFormats, NULL, hInst, NULL));
    HWND hMergeFormat = CreateWindowEx(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 130, PADDING * 3 + CONTROL_HEIGHT * 2, 150, 150, hTabFormats, (HMENU)IDC_MERGE_FORMAT_COMBO, hInst, NULL);
    const wchar_t* merges[] = { L"default", L"mkv", L"mp4", L"webm", L"mov", L"avi", L"flv" };
    for (const auto& m : merges) SendMessageW(hMergeFormat, CB_ADDSTRING, 0, (LPARAM)m);
    SendMessage(hMergeFormat, CB_SETCURSEL, 0, 0);
    tabFormatsControls.push_back(hMergeFormat);

    // --- Вкладка 3: Metadata ---
    tabMetadataControls.push_back(CreateWindowEx(0, L"BUTTON", L"Embed Subtitles", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, PADDING, PADDING, 150, CONTROL_HEIGHT, hTabMetadata, (HMENU)IDC_EMBED_SUBS_CHECK, hInst, NULL));
    tabMetadataControls.push_back(CreateWindowEx(0, L"BUTTON", L"Embed Thumbnail", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, PADDING, PADDING * 2 + CONTROL_HEIGHT, 150, CONTROL_HEIGHT, hTabMetadata, (HMENU)IDC_EMBED_THUMB_CHECK, hInst, NULL));
    tabMetadataControls.push_back(CreateWindowEx(0, L"BUTTON", L"Embed Metadata", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, PADDING, PADDING * 3 + CONTROL_HEIGHT * 2, 150, CONTROL_HEIGHT, hTabMetadata, (HMENU)IDC_EMBED_META_CHECK, hInst, NULL));
    tabMetadataControls.push_back(CreateWindowEx(0, L"BUTTON", L"Split Chapters", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, PADDING, PADDING * 4 + CONTROL_HEIGHT * 3, 150, CONTROL_HEIGHT, hTabMetadata, (HMENU)IDC_SPLIT_CHAPTERS_CHECK, hInst, NULL));

    // --- Вкладка 4: Advanced ---
    tabAdvancedControls.push_back(CreateWindowEx(0, L"STATIC", L"Cookies from:", WS_CHILD | WS_VISIBLE, PADDING, PADDING, 120, CONTROL_HEIGHT, hTabAdvanced, NULL, hInst, NULL));
    HWND hCookies = CreateWindowEx(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 130, PADDING, 150, 150, hTabAdvanced, (HMENU)IDC_COOKIES_BROWSER_COMBO, hInst, NULL);
    const wchar_t* browsers[] = { L"none", L"brave", L"chrome", L"chromium", L"edge", L"firefox", L"opera", L"safari", L"vivaldi" };
    for (const auto& b : browsers) SendMessageW(hCookies, CB_ADDSTRING, 0, (LPARAM)b);
    SendMessage(hCookies, CB_SETCURSEL, 0, 0);
    tabAdvancedControls.push_back(hCookies);
    
    ShowTabControls(0);
}

void ShowTabControls(int tabIndex) {
    ShowWindow(hTabGeneral, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(hTabFormats, tabIndex == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(hTabMetadata, tabIndex == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(hTabAdvanced, tabIndex == 3 ? SW_SHOW : SW_HIDE);
}


void CreateControls(HWND hWnd) {
    // URL
    CreateWindowEx(0, L"STATIC", L"Video URL:", WS_CHILD | WS_VISIBLE, PADDING, PADDING, 100, CONTROL_HEIGHT, hWnd, NULL, hInst, NULL);
    hUrlEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 110, PADDING, 400, CONTROL_HEIGHT, hWnd, (HMENU)IDC_URL_EDIT, hInst, NULL);

    // Save Path
    CreateWindowEx(0, L"STATIC", L"Save to:", WS_CHILD | WS_VISIBLE, PADDING, PADDING * 2 + CONTROL_HEIGHT, 100, CONTROL_HEIGHT, hWnd, NULL, hInst, NULL);
    hPathEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 110, PADDING * 2 + CONTROL_HEIGHT, 400, CONTROL_HEIGHT, hWnd, (HMENU)IDC_PATH_EDIT, hInst, NULL);
    hBrowseButton = CreateWindowEx(0, L"BUTTON", L"...", WS_CHILD | WS_VISIBLE, 520, PADDING * 2 + CONTROL_HEIGHT, 40, CONTROL_HEIGHT, hWnd, (HMENU)IDC_BROWSE_BUTTON, hInst, NULL);
    
    // Download Button
    hDownloadButton = CreateWindowEx(0, L"BUTTON", L"Download", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, PADDING, PADDING * 3 + CONTROL_HEIGHT * 2, 120, 30, hWnd, (HMENU)IDC_DOWNLOAD_BUTTON, hInst, NULL);

    // Progress Bar
    hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 140, PADDING * 3 + CONTROL_HEIGHT * 2, 300, 30, hWnd, (HMENU)IDC_PROGRESS_BAR, hInst, NULL);
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    // Tabs
    CreateTabsAndControls(hWnd);

    // Log Controls
    hLogToggleButton = CreateWindowEx(0, L"BUTTON", L"Show Log", WS_CHILD | WS_VISIBLE, 0, 0, 100, CONTROL_HEIGHT, hWnd, (HMENU)IDC_LOG_TOGGLE_BUTTON, hInst, NULL);
    hLogEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 0, 0, 1, 1, hWnd, (HMENU)IDC_LOG_EDIT, hInst, NULL);
}


void ResizeControls(HWND hWnd) {
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    int top_offset = PADDING * 3 + CONTROL_HEIGHT * 2 + 30 + PADDING; // Height of top controls + padding

    // URL and Path
    MoveWindow(hUrlEdit, 110, PADDING, width - 120, CONTROL_HEIGHT, TRUE);
    MoveWindow(hPathEdit, 110, PADDING * 2 + CONTROL_HEIGHT, width - 120 - 50, CONTROL_HEIGHT, TRUE);
    MoveWindow(hBrowseButton, width - 50 - PADDING, PADDING * 2 + CONTROL_HEIGHT, 40, CONTROL_HEIGHT, TRUE);
    
    // Download and Progress
    MoveWindow(hDownloadButton, PADDING, PADDING * 3 + CONTROL_HEIGHT * 2, 120, 30, TRUE);
    MoveWindow(hProgressBar, 140, PADDING * 3 + CONTROL_HEIGHT * 2, width - 150 - PADDING, 30, TRUE);
    
    // Log Button
    int logButtonY = isLogVisible ? height - LOG_HEIGHT - CONTROL_HEIGHT - PADDING : height - CONTROL_HEIGHT - PADDING;
    MoveWindow(hLogToggleButton, PADDING, logButtonY, 100, CONTROL_HEIGHT, TRUE);

    // Log Edit
    if (isLogVisible) {
        ShowWindow(hLogEdit, SW_SHOW);
        MoveWindow(hLogEdit, PADDING, height - LOG_HEIGHT, width - PADDING * 2, LOG_HEIGHT - PADDING, TRUE);
    } else {
        ShowWindow(hLogEdit, SW_HIDE);
    }
    
    // Tab Control
    int tabHeight = logButtonY - top_offset - PADDING;
    MoveWindow(hTabControl, PADDING, top_offset, width - PADDING * 2, tabHeight, TRUE);
    
    RECT tabRect;
    GetClientRect(hTabControl, &tabRect);
    TabCtrl_AdjustRect(hTabControl, FALSE, &tabRect);
    
    MoveWindow(hTabGeneral, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, TRUE);
    MoveWindow(hTabFormats, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, TRUE);
    MoveWindow(hTabMetadata, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, TRUE);
    MoveWindow(hTabAdvanced, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, TRUE);
}

void OnDownloadClick() {
    if (isDownloading) return;

    wchar_t url[2048]; // Увеличим буфер для URL
    GetWindowTextW(hUrlEdit, url, 2048);
    if (wcslen(url) == 0) {
        MessageBoxW(hMainWindow, L"Please enter a video URL.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    SaveSettings();
    SetWindowTextW(hLogEdit, L""); // Clear log
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

    EnableWindow(hDownloadButton, FALSE);
    SetWindowTextW(hDownloadButton, L"Downloading...");

    std::wstringstream cmd;
    cmd << L"yt-dlp.exe --progress --verbose -o \"";
    
    wchar_t path[MAX_PATH];
    GetWindowTextW(hPathEdit, path, MAX_PATH);

    wchar_t outputTemplate[256];
    GetWindowTextW(GetDlgItem(hTabGeneral, IDC_OUTPUT_TEMPLATE_EDIT), outputTemplate, 256);
    if (wcslen(outputTemplate) > 0) {
        cmd << path << L"\\" << outputTemplate << L"\"";
    } else {
        cmd << path << L"\\%(title)s [%(id)s].%(ext)s\"";
    }

    // --- Сбор опций с вкладок ---

    // General
    wchar_t buffer[MAX_PATH];
    GetWindowTextW(GetDlgItem(hTabGeneral, IDC_PROXY_EDIT), buffer, MAX_PATH);
    if (wcslen(buffer) > 0) cmd << L" --proxy \"" << buffer << L"\"";
    GetWindowTextW(GetDlgItem(hTabGeneral, IDC_BATCH_FILE_EDIT), buffer, MAX_PATH);
    if (wcslen(buffer) > 0) cmd << L" -a \"" << buffer << L"\"";
    
    // Formats
    int idx = SendMessage(GetDlgItem(hTabFormats, IDC_CONVERT_SUBS_COMBO), CB_GETCURSEL, 0, 0);
    if (idx > 0) { // 0 is "none"
        SendMessage(GetDlgItem(hTabFormats, IDC_CONVERT_SUBS_COMBO), CB_GETLBTEXT, idx, (LPARAM)buffer);
        cmd << L" --convert-subs " << buffer;
    }
    idx = SendMessage(GetDlgItem(hTabFormats, IDC_CONVERT_THUMB_COMBO), CB_GETCURSEL, 0, 0);
    if (idx > 0) { // 0 is "none"
        SendMessage(GetDlgItem(hTabFormats, IDC_CONVERT_THUMB_COMBO), CB_GETLBTEXT, idx, (LPARAM)buffer);
        cmd << L" --convert-thumbnails " << buffer;
    }
    idx = SendMessage(GetDlgItem(hTabFormats, IDC_MERGE_FORMAT_COMBO), CB_GETCURSEL, 0, 0);
    if (idx > 0) { // 0 is "default"
        SendMessage(GetDlgItem(hTabFormats, IDC_MERGE_FORMAT_COMBO), CB_GETLBTEXT, idx, (LPARAM)buffer);
        cmd << L" --merge-output-format " << buffer;
    }

    // Metadata
    if (IsDlgButtonChecked(hTabMetadata, IDC_EMBED_SUBS_CHECK)) cmd << L" --embed-subs";
    if (IsDlgButtonChecked(hTabMetadata, IDC_EMBED_THUMB_CHECK)) cmd << L" --embed-thumbnail";
    if (IsDlgButtonChecked(hTabMetadata, IDC_EMBED_META_CHECK)) cmd << L" --embed-metadata";
    if (IsDlgButtonChecked(hTabMetadata, IDC_SPLIT_CHAPTERS_CHECK)) cmd << L" --split-chapters";
    
    // Advanced
    idx = SendMessage(GetDlgItem(hTabAdvanced, IDC_COOKIES_BROWSER_COMBO), CB_GETCURSEL, 0, 0);
    if (idx > 0) { // 0 is "none"
        SendMessage(GetDlgItem(hTabAdvanced, IDC_COOKIES_BROWSER_COMBO), CB_GETLBTEXT, idx, (LPARAM)buffer);
        cmd << L" --cookies-from-browser " << buffer;
    }
    
    // URL в конце
    cmd << L" \"" << url << L"\"";

    DownloadParams params = { cmd.str() };
    std::thread(DownloadThread, params).detach();
}

void OnBrowseClick() {
    IFileOpenDialog* pfd;
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pfd))) {
            DWORD dwOptions;
            pfd->GetOptions(&dwOptions);
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
            if (SUCCEEDED(pfd->Show(NULL))) {
                IShellItem* psi;
                if (SUCCEEDED(pfd->GetResult(&psi))) {
                    PWSTR pszFilePath;
                    if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                        SetWindowTextW(hPathEdit, pszFilePath);
                        CoTaskMemFree(pszFilePath);
                    }
                    psi->Release();
                }
            }
            pfd->Release();
        }
        CoUninitialize();
    }
}

void OnToggleLogClick() {
    isLogVisible = !isLogVisible;
    SetWindowTextW(hLogToggleButton, isLogVisible ? L"Hide Log" : L"Show Log");
    ResizeControls(hMainWindow);
}

std::wstring GetIniPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring::size_type pos = std::wstring(path).find_last_of(L"\\/");
    return std::wstring(path).substr(0, pos) + L"\\config.ini";
}

void SaveSettings() {
    std::wstring iniPath = GetIniPath();
    wchar_t buffer[MAX_PATH];

    GetWindowTextW(hPathEdit, buffer, MAX_PATH);
    WritePrivateProfileStringW(L"Settings", L"SavePath", buffer, iniPath.c_str());
    
    GetWindowTextW(GetDlgItem(hTabGeneral, IDC_PROXY_EDIT), buffer, MAX_PATH);
    WritePrivateProfileStringW(L"Settings", L"Proxy", buffer, iniPath.c_str());
}

void LoadSettings() {
    std::wstring iniPath = GetIniPath();
    wchar_t buffer[MAX_PATH];

    // Путь сохранения
    GetPrivateProfileStringW(L"Settings", L"SavePath", L"", buffer, MAX_PATH, iniPath.c_str());
    if (wcslen(buffer) > 0) {
        SetWindowTextW(hPathEdit, buffer);
    } else {
        // По умолчанию - Рабочий стол
        PWSTR pszPath;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &pszPath))) {
            SetWindowTextW(hPathEdit, pszPath);
            CoTaskMemFree(pszPath);
        }
    }
    
    // Прокси
    GetPrivateProfileStringW(L"Settings", L"Proxy", L"", buffer, MAX_PATH, iniPath.c_str());
    SetWindowTextW(GetDlgItem(hTabGeneral, IDC_PROXY_EDIT), buffer);
}
