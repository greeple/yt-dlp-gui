#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <memory>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

// Controls IDs
#define ID_URL_EDIT 1001
#define ID_DOWNLOAD_BTN 1002
#define ID_BROWSE_BTN 1003
#define ID_OUTPUT_PATH 1004
#define ID_PROGRESS_BAR 1005
#define ID_STATUS_TEXT 1006
#define ID_CONSOLE_OUTPUT 1007
#define ID_TOGGLE_CONSOLE 1008
#define ID_FORMAT_COMBO 1009
#define ID_QUALITY_COMBO 1010
#define ID_TAB_CONTROL 1011

// Advanced options IDs
#define ID_IGNORE_CONFIG 2001
#define ID_CONFIG_LOCATION 2002
#define ID_LIVE_FROM_START 2003
#define ID_PROXY_URL 2004
#define ID_DOWNLOADER_COMBO 2005
#define ID_DOWNLOADER_ARGS 2006
#define ID_BATCH_FILE 2007
#define ID_BATCH_BROWSE 2008
#define ID_OUTPUT_TEMPLATE 2009
#define ID_COOKIES_FILE 2010
#define ID_COOKIES_BROWSE 2011
#define ID_COOKIES_BROWSER 2012
#define ID_CACHE_DIR 2013
#define ID_NO_CACHE 2014
#define ID_RM_CACHE 2015
#define ID_SKIP_DOWNLOAD 2016
#define ID_JSON_OUTPUT 2017
#define ID_LIST_FORMATS 2018
#define ID_MERGE_FORMAT 2019
#define ID_CHECK_FORMATS 2020
#define ID_EMBED_SUBS 2021
#define ID_EMBED_THUMBNAIL 2022
#define ID_EMBED_METADATA 2023
#define ID_EMBED_CHAPTERS 2024
#define ID_CONVERT_SUBS 2025
#define ID_CONVERT_THUMBS 2026
#define ID_SPLIT_CHAPTERS 2027
#define ID_REMOVE_CHAPTERS 2028
#define ID_FORCE_KEYFRAMES 2029

// Global variables
HWND hMainWindow;
HWND hUrlEdit;
HWND hDownloadBtn;
HWND hOutputPath;
HWND hProgressBar;
HWND hStatusText;
HWND hConsoleOutput;
HWND hToggleConsole;
HWND hFormatCombo;
HWND hQualityCombo;
HWND hTabControl;
HWND hBasicTab;
HWND hAdvancedTab;
HWND hPostProcessTab;

std::atomic<bool> isDownloading(false);
std::atomic<bool> showConsole(false);
std::wstring settingsFile = L"ytdlp_gui_settings.json";

// Tab pages
std::vector<HWND> tabPages;

struct Settings {
    std::wstring outputPath;
    std::wstring format;
    std::wstring quality;
    bool ignoreConfig = false;
    std::wstring configLocation;
    bool liveFromStart = false;
    std::wstring proxy;
    std::wstring downloader;
    std::wstring downloaderArgs;
    std::wstring batchFile;
    std::wstring outputTemplate;
    std::wstring cookiesFile;
    std::wstring cookiesBrowser;
    std::wstring cacheDir;
    bool noCache = false;
    bool rmCache = false;
    bool skipDownload = false;
    bool jsonOutput = false;
    bool listFormats = false;
    std::wstring mergeFormat;
    bool checkFormats = false;
    bool embedSubs = false;
    bool embedThumbnail = false;
    bool embedMetadata = false;
    bool embedChapters = false;
    std::wstring convertSubs;
    std::wstring convertThumbs;
    bool splitChapters = false;
    std::wstring removeChapters;
    bool forceKeyframes = false;
};

Settings appSettings;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hwnd);
void CreateBasicTab(HWND parent);
void CreateAdvancedTab(HWND parent);
void CreatePostProcessTab(HWND parent);
void OnResize(HWND hwnd);
void ToggleConsoleOutput();
void StartDownload();
void ExecuteYtDlp(const std::wstring& url);
std::wstring GetDesktopPath();
void BrowseFolder();
void LoadSettings();
void SaveSettings();
std::wstring ReadFromPipe(HANDLE hPipe);
std::wstring UTF8ToWide(const std::string& utf8);
std::string WideToUTF8(const std::wstring& wide);
void UpdateProgress(const std::wstring& line);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // Register window class
    const wchar_t CLASS_NAME[] = L"YTDLPGuiWindow";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClass(&wc);

    // Create window
    hMainWindow = CreateWindowEx(
        0,
        CLASS_NAME,
        L"YouTube Downloader (yt-dlp GUI)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hMainWindow == nullptr) {
        return 0;
    }

    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        CreateControls(hwnd);
        LoadSettings();
        return 0;

    case WM_SIZE:
        OnResize(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_DOWNLOAD_BTN:
            if (!isDownloading) {
                StartDownload();
            }
            break;
        case ID_BROWSE_BTN:
            BrowseFolder();
            break;
        case ID_TOGGLE_CONSOLE:
            ToggleConsoleOutput();
            break;
        case ID_BATCH_BROWSE:
            {
                OPENFILENAME ofn = {};
                wchar_t fileName[MAX_PATH] = L"";
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST;

                if (GetOpenFileName(&ofn)) {
                    HWND hBatchFile = GetDlgItem(hAdvancedTab, ID_BATCH_FILE);
                    SetWindowText(hBatchFile, fileName);
                }
            }
            break;
        case ID_COOKIES_BROWSE:
            {
                OPENFILENAME ofn = {};
                wchar_t fileName[MAX_PATH] = L"";
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST;

                if (GetOpenFileName(&ofn)) {
                    HWND hCookiesFile = GetDlgItem(hAdvancedTab, ID_COOKIES_FILE);
                    SetWindowText(hCookiesFile, fileName);
                }
            }
            break;
        }
        return 0;

    case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = (LPNMHDR)lParam;
            if (lpnmhdr->idFrom == ID_TAB_CONTROL && lpnmhdr->code == TCN_SELCHANGE) {
                int iPage = TabCtrl_GetCurSel(hTabControl);
                for (size_t i = 0; i < tabPages.size(); i++) {
                    ShowWindow(tabPages[i], (i == iPage) ? SW_SHOW : SW_HIDE);
                }
            }
        }
        return 0;

    case WM_DESTROY:
        SaveSettings();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CreateControls(HWND hwnd) {
    // Create tab control
    hTabControl = CreateWindow(WC_TABCONTROL, L"", 
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        10, 10, 870, 680,
        hwnd, (HMENU)ID_TAB_CONTROL, GetModuleHandle(nullptr), nullptr);

    TCITEM tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = (LPWSTR)L"Basic";
    TabCtrl_InsertItem(hTabControl, 0, &tie);
    tie.pszText = (LPWSTR)L"Advanced";
    TabCtrl_InsertItem(hTabControl, 1, &tie);
    tie.pszText = (LPWSTR)L"Post-Processing";
    TabCtrl_InsertItem(hTabControl, 2, &tie);

    // Create tab pages
    RECT rcTab;
    GetClientRect(hTabControl, &rcTab);
    TabCtrl_AdjustRect(hTabControl, FALSE, &rcTab);

    hBasicTab = CreateWindow(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE,
        rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
        hTabControl, nullptr, GetModuleHandle(nullptr), nullptr);
    tabPages.push_back(hBasicTab);

    hAdvancedTab = CreateWindow(L"STATIC", L"",
        WS_CHILD,
        rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
        hTabControl, nullptr, GetModuleHandle(nullptr), nullptr);
    tabPages.push_back(hAdvancedTab);

    hPostProcessTab = CreateWindow(L"STATIC", L"",
        WS_CHILD,
        rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
        hTabControl, nullptr, GetModuleHandle(nullptr), nullptr);
    tabPages.push_back(hPostProcessTab);

    CreateBasicTab(hBasicTab);
    CreateAdvancedTab(hAdvancedTab);
    CreatePostProcessTab(hPostProcessTab);
}

void CreateBasicTab(HWND parent) {
    // URL input
    CreateWindow(L"STATIC", L"Video URL:",
        WS_CHILD | WS_VISIBLE,
        10, 10, 100, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    hUrlEdit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        10, 35, 840, 25,
        parent, (HMENU)ID_URL_EDIT, GetModuleHandle(nullptr), nullptr);

    // Output path
    CreateWindow(L"STATIC", L"Output Path:",
        WS_CHILD | WS_VISIBLE,
        10, 70, 100, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    hOutputPath = CreateWindow(L"EDIT", GetDesktopPath().c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        10, 95, 750, 25,
        parent, (HMENU)ID_OUTPUT_PATH, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        770, 94, 80, 27,
        parent, (HMENU)ID_BROWSE_BTN, GetModuleHandle(nullptr), nullptr);

    // Format selection
    CreateWindow(L"STATIC", L"Format:",
        WS_CHILD | WS_VISIBLE,
        10, 130, 60, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    hFormatCombo = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        80, 130, 150, 200,
        parent, (HMENU)ID_FORMAT_COMBO, GetModuleHandle(nullptr), nullptr);

    SendMessage(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"Best");
    SendMessage(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mp4");
    SendMessage(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mkv");
    SendMessage(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"webm");
    SendMessage(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mp3");
    SendMessage(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"m4a");
    SendMessage(hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"wav");
    SendMessage(hFormatCombo, CB_SETCURSEL, 0, 0);

    // Quality selection
    CreateWindow(L"STATIC", L"Quality:",
        WS_CHILD | WS_VISIBLE,
        250, 130, 60, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    hQualityCombo = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        320, 130, 150, 200,
        parent, (HMENU)ID_QUALITY_COMBO, GetModuleHandle(nullptr), nullptr);

    SendMessage(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"Best");
    SendMessage(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"2160p (4K)");
    SendMessage(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1440p");
    SendMessage(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1080p");
    SendMessage(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"720p");
    SendMessage(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"480p");
    SendMessage(hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"360p");
    SendMessage(hQualityCombo, CB_SETCURSEL, 0, 0);

    // Download button
    hDownloadBtn = CreateWindow(L"BUTTON", L"Download",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 170, 150, 35,
        parent, (HMENU)ID_DOWNLOAD_BTN, GetModuleHandle(nullptr), nullptr);

    // Progress bar
    hProgressBar = CreateWindow(PROGRESS_CLASS, L"",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        10, 220, 840, 25,
        parent, (HMENU)ID_PROGRESS_BAR, GetModuleHandle(nullptr), nullptr);

    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    // Status text
    hStatusText = CreateWindow(L"STATIC", L"Ready",
        WS_CHILD | WS_VISIBLE,
        10, 255, 840, 20,
        parent, (HMENU)ID_STATUS_TEXT, GetModuleHandle(nullptr), nullptr);

    // Toggle console button
    hToggleConsole = CreateWindow(L"BUTTON", L"Show Console Output ▼",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 285, 200, 30,
        parent, (HMENU)ID_TOGGLE_CONSOLE, GetModuleHandle(nullptr), nullptr);

    // Console output
    hConsoleOutput = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
        10, 325, 840, 250,
        parent, (HMENU)ID_CONSOLE_OUTPUT, GetModuleHandle(nullptr), nullptr);
}

void CreateAdvancedTab(HWND parent) {
    int yPos = 10;
    int yStep = 35;

    // Ignore config
    CreateWindow(L"BUTTON", L"Ignore Config",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 200, 25,
        parent, (HMENU)ID_IGNORE_CONFIG, GetModuleHandle(nullptr), nullptr);

    // Config location
    yPos += yStep;
    CreateWindow(L"STATIC", L"Config Location:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        140, yPos, 400, 25,
        parent, (HMENU)ID_CONFIG_LOCATION, GetModuleHandle(nullptr), nullptr);

    // Live from start
    yPos += yStep;
    CreateWindow(L"BUTTON", L"Live From Start",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 200, 25,
        parent, (HMENU)ID_LIVE_FROM_START, GetModuleHandle(nullptr), nullptr);

    // Proxy
    yPos += yStep;
    CreateWindow(L"STATIC", L"Proxy URL:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        140, yPos, 400, 25,
        parent, (HMENU)ID_PROXY_URL, GetModuleHandle(nullptr), nullptr);

    // Downloader
    yPos += yStep;
    CreateWindow(L"STATIC", L"Downloader:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    HWND hDownloaderCombo = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        140, yPos, 200, 200,
        parent, (HMENU)ID_DOWNLOADER_COMBO, GetModuleHandle(nullptr), nullptr);

    SendMessage(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"native");
    SendMessage(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"aria2c");
    SendMessage(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"axel");
    SendMessage(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"curl");
    SendMessage(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"ffmpeg");
    SendMessage(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"httpie");
    SendMessage(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"wget");
    SendMessage(hDownloaderCombo, CB_SETCURSEL, 0, 0);

    // Downloader args
    yPos += yStep;
    CreateWindow(L"STATIC", L"Downloader Args:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        140, yPos, 400, 25,
        parent, (HMENU)ID_DOWNLOADER_ARGS, GetModuleHandle(nullptr), nullptr);

    // Batch file
    yPos += yStep;
    CreateWindow(L"STATIC", L"Batch File:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        140, yPos, 320, 25,
        parent, (HMENU)ID_BATCH_FILE, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        470, yPos - 1, 70, 27,
        parent, (HMENU)ID_BATCH_BROWSE, GetModuleHandle(nullptr), nullptr);

    // Output template
    yPos += yStep;
    CreateWindow(L"STATIC", L"Output Template:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"EDIT", L"%(title)s.%(ext)s",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        140, yPos, 400, 25,
        parent, (HMENU)ID_OUTPUT_TEMPLATE, GetModuleHandle(nullptr), nullptr);

    // Cookies file
    yPos += yStep;
    CreateWindow(L"STATIC", L"Cookies File:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        140, yPos, 320, 25,
        parent, (HMENU)ID_COOKIES_FILE, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        470, yPos - 1, 70, 27,
        parent, (HMENU)ID_COOKIES_BROWSE, GetModuleHandle(nullptr), nullptr);

    // Cookies from browser
    yPos += yStep;
    CreateWindow(L"STATIC", L"Cookies Browser:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    HWND hCookiesBrowser = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        140, yPos, 200, 200,
        parent, (HMENU)ID_COOKIES_BROWSER, GetModuleHandle(nullptr), nullptr);

    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"none");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"brave");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"chrome");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"chromium");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"edge");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"firefox");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"opera");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"safari");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"vivaldi");
    SendMessage(hCookiesBrowser, CB_ADDSTRING, 0, (LPARAM)L"whale");
    SendMessage(hCookiesBrowser, CB_SETCURSEL, 0, 0);

    // Cache options
    yPos += yStep;
    CreateWindow(L"BUTTON", L"No Cache",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 100, 25,
        parent, (HMENU)ID_NO_CACHE, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"BUTTON", L"Remove Cache",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        120, yPos, 120, 25,
        parent, (HMENU)ID_RM_CACHE, GetModuleHandle(nullptr), nullptr);

    // Skip download
    yPos += yStep;
    CreateWindow(L"BUTTON", L"Skip Download",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 150, 25,
        parent, (HMENU)ID_SKIP_DOWNLOAD, GetModuleHandle(nullptr), nullptr);

    // JSON output
    CreateWindow(L"BUTTON", L"JSON Output",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        170, yPos, 120, 25,
        parent, (HMENU)ID_JSON_OUTPUT, GetModuleHandle(nullptr), nullptr);

    // List formats
    CreateWindow(L"BUTTON", L"List Formats",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        300, yPos, 120, 25,
        parent, (HMENU)ID_LIST_FORMATS, GetModuleHandle(nullptr), nullptr);
}

void CreatePostProcessTab(HWND parent) {
    int yPos = 10;
    int yStep = 35;

    // Merge format
    CreateWindow(L"STATIC", L"Merge Format:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    HWND hMergeFormat = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        140, yPos, 150, 200,
        parent, (HMENU)ID_MERGE_FORMAT, GetModuleHandle(nullptr), nullptr);

    SendMessage(hMergeFormat, CB_ADDSTRING, 0, (LPARAM)L"default");
    SendMessage(hMergeFormat, CB_ADDSTRING, 0, (LPARAM)L"mp4");
    SendMessage(hMergeFormat, CB_ADDSTRING, 0, (LPARAM)L"mkv");
    SendMessage(hMergeFormat, CB_ADDSTRING, 0, (LPARAM)L"webm");
    SendMessage(hMergeFormat, CB_ADDSTRING, 0, (LPARAM)L"avi");
    SendMessage(hMergeFormat, CB_ADDSTRING, 0, (LPARAM)L"flv");
    SendMessage(hMergeFormat, CB_ADDSTRING, 0, (LPARAM)L"mov");
    SendMessage(hMergeFormat, CB_SETCURSEL, 0, 0);

    // Check formats
    yPos += yStep;
    CreateWindow(L"BUTTON", L"Check Formats",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 150, 25,
        parent, (HMENU)ID_CHECK_FORMATS, GetModuleHandle(nullptr), nullptr);

    // Embed options
    yPos += yStep;
    CreateWindow(L"BUTTON", L"Embed Subtitles",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 150, 25,
        parent, (HMENU)ID_EMBED_SUBS, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"BUTTON", L"Embed Thumbnail",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        170, yPos, 150, 25,
        parent, (HMENU)ID_EMBED_THUMBNAIL, GetModuleHandle(nullptr), nullptr);

    yPos += yStep;
    CreateWindow(L"BUTTON", L"Embed Metadata",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 150, 25,
        parent, (HMENU)ID_EMBED_METADATA, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"BUTTON", L"Embed Chapters",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        170, yPos, 150, 25,
        parent, (HMENU)ID_EMBED_CHAPTERS, GetModuleHandle(nullptr), nullptr);

    // Convert subtitles
    yPos += yStep;
    CreateWindow(L"STATIC", L"Convert Subs:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    HWND hConvertSubs = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        140, yPos, 150, 200,
        parent, (HMENU)ID_CONVERT_SUBS, GetModuleHandle(nullptr), nullptr);

    SendMessage(hConvertSubs, CB_ADDSTRING, 0, (LPARAM)L"none");
    SendMessage(hConvertSubs, CB_ADDSTRING, 0, (LPARAM)L"ass");
    SendMessage(hConvertSubs, CB_ADDSTRING, 0, (LPARAM)L"lrc");
    SendMessage(hConvertSubs, CB_ADDSTRING, 0, (LPARAM)L"srt");
    SendMessage(hConvertSubs, CB_ADDSTRING, 0, (LPARAM)L"vtt");
    SendMessage(hConvertSubs, CB_SETCURSEL, 0, 0);

    // Convert thumbnails
    yPos += yStep;
    CreateWindow(L"STATIC", L"Convert Thumbs:",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 120, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    HWND hConvertThumbs = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        140, yPos, 150, 200,
        parent, (HMENU)ID_CONVERT_THUMBS, GetModuleHandle(nullptr), nullptr);

    SendMessage(hConvertThumbs, CB_ADDSTRING, 0, (LPARAM)L"none");
    SendMessage(hConvertThumbs, CB_ADDSTRING, 0, (LPARAM)L"jpg");
    SendMessage(hConvertThumbs, CB_ADDSTRING, 0, (LPARAM)L"png");
    SendMessage(hConvertThumbs, CB_ADDSTRING, 0, (LPARAM)L"webp");
    SendMessage(hConvertThumbs, CB_SETCURSEL, 0, 0);

    // Split chapters
    yPos += yStep;
    CreateWindow(L"BUTTON", L"Split Chapters",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 150, 25,
        parent, (HMENU)ID_SPLIT_CHAPTERS, GetModuleHandle(nullptr), nullptr);

    // Remove chapters
    yPos += yStep;
    CreateWindow(L"STATIC", L"Remove Chapters (regex):",
        WS_CHILD | WS_VISIBLE,
        10, yPos, 180, 20,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        200, yPos, 340, 25,
        parent, (HMENU)ID_REMOVE_CHAPTERS, GetModuleHandle(nullptr), nullptr);

    // Force keyframes
    yPos += yStep;
    CreateWindow(L"BUTTON", L"Force Keyframes at Cuts",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        10, yPos, 200, 25,
        parent, (HMENU)ID_FORCE_KEYFRAMES, GetModuleHandle(nullptr), nullptr);
}

void OnResize(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    if (hTabControl) {
        SetWindowPos(hTabControl, nullptr, 10, 10, rect.right - 20, rect.bottom - 20, SWP_NOZORDER);
        
        RECT rcTab;
        GetClientRect(hTabControl, &rcTab);
        TabCtrl_AdjustRect(hTabControl, FALSE, &rcTab);
        
        for (HWND hTab : tabPages) {
            SetWindowPos(hTab, nullptr, rcTab.left, rcTab.top, 
                rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);
        }
        
        // Resize controls in basic tab
        if (hUrlEdit) {
            SetWindowPos(hUrlEdit, nullptr, 0, 0, rcTab.right - rcTab.left - 20, 25, 
                SWP_NOMOVE | SWP_NOZORDER);
        }
        if (hOutputPath) {
            SetWindowPos(hOutputPath, nullptr, 0, 0, rcTab.right - rcTab.left - 120, 25, 
                SWP_NOMOVE | SWP_NOZORDER);
        }
        if (hProgressBar) {
            SetWindowPos(hProgressBar, nullptr, 0, 0, rcTab.right - rcTab.left - 20, 25, 
                SWP_NOMOVE | SWP_NOZORDER);
        }
        if (hStatusText) {
            SetWindowPos(hStatusText, nullptr, 0, 0, rcTab.right - rcTab.left - 20, 20, 
                SWP_NOMOVE | SWP_NOZORDER);
        }
        if (hConsoleOutput && IsWindowVisible(hConsoleOutput)) {
            SetWindowPos(hConsoleOutput, nullptr, 0, 0, rcTab.right - rcTab.left - 20, 250, 
                SWP_NOMOVE | SWP_NOZORDER);
        }
    }
}

void ToggleConsoleOutput() {
    showConsole = !showConsole;
    ShowWindow(hConsoleOutput, showConsole ? SW_SHOW : SW_HIDE);
    SetWindowText(hToggleConsole, showConsole ? L"Hide Console Output ▲" : L"Show Console Output ▼");
}

void StartDownload() {
    wchar_t url[1024];
    GetWindowText(hUrlEdit, url, 1024);
    
    if (wcslen(url) == 0) {
        MessageBox(hMainWindow, L"Please enter a URL", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    std::thread downloadThread(ExecuteYtDlp, std::wstring(url));
    downloadThread.detach();
}

std::wstring UTF8ToWide(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size == 0) return std::wstring();
    
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
    result.resize(size - 1);
    
    return result;
}

std::string WideToUTF8(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return std::string();
    
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
    result.resize(size - 1);
    
    return result;
}

void UpdateProgress(const std::wstring& line) {
    // Parse progress from yt-dlp output
    size_t pos = line.find(L"[download]");
    if (pos != std::wstring::npos) {
        pos = line.find(L"%", pos);
        if (pos != std::wstring::npos) {
            size_t start = pos;
            while (start > 0 && iswdigit(line[start - 1])) {
                start--;
            }
            
            std::wstring percentStr = line.substr(start, pos - start);
            try {
                double percent = std::stod(percentStr);
                SendMessage(hProgressBar, PBM_SETPOS, (int)percent, 0);
            } catch (...) {}
        }
    }
}

void ExecuteYtDlp(const std::wstring& url) {
    isDownloading = true;
    EnableWindow(hDownloadBtn, FALSE);
    SetWindowText(hStatusText, L"Starting download...");
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    
    // Build command
    std::wstringstream cmd;
    cmd << L"yt-dlp.exe ";
    
    // Get settings from controls
    wchar_t outputPath[MAX_PATH];
    GetWindowText(hOutputPath, outputPath, MAX_PATH);
    
    wchar_t formatText[256];
    GetWindowText(hFormatCombo, formatText, 256);
    
    wchar_t qualityText[256];
    GetWindowText(hQualityCombo, qualityText, 256);
    
    // Add basic options
    cmd << L"-P \"" << outputPath << L"\" ";
    
    if (wcscmp(formatText, L"Best") != 0) {
        if (wcscmp(formatText, L"mp3") == 0 || wcscmp(formatText, L"m4a") == 0 || wcscmp(formatText, L"wav") == 0) {
            cmd << L"-x --audio-format " << formatText << L" ";
        } else {
            cmd << L"-f \"bestvideo+bestaudio/best\" --merge-output-format " << formatText << L" ";
        }
    }
    
    if (wcscmp(qualityText, L"Best") != 0) {
        std::wstring quality = qualityText;
        size_t pos = quality.find(L"p");
        if (pos != std::wstring::npos) {
            quality = quality.substr(0, pos);
            if (quality == L"2160") quality = L"4320";
            cmd << L"-f \"bestvideo[height<=" << quality << L"]+bestaudio/best[height<=" << quality << L"]\" ";
        }
    }
    
    // Add advanced options
    if (IsDlgButtonChecked(hAdvancedTab, ID_IGNORE_CONFIG)) {
        cmd << L"--ignore-config ";
    }
    
    wchar_t buffer[1024];
    
    GetDlgItemText(hAdvancedTab, ID_CONFIG_LOCATION, buffer, 1024);
    if (wcslen(buffer) > 0) {
        cmd << L"--config-locations \"" << buffer << L"\" ";
    }
    
    if (IsDlgButtonChecked(hAdvancedTab, ID_LIVE_FROM_START)) {
        cmd << L"--live-from-start ";
    }
    
    GetDlgItemText(hAdvancedTab, ID_PROXY_URL, buffer, 1024);
    if (wcslen(buffer) > 0) {
        cmd << L"--proxy \"" << buffer << L"\" ";
    }
    
    GetDlgItemText(hAdvancedTab, ID_DOWNLOADER_COMBO, buffer, 1024);
    if (wcslen(buffer) > 0 && wcscmp(buffer, L"native") != 0) {
        cmd << L"--downloader \"" << buffer << L"\" ";
    }
    
    GetDlgItemText(hAdvancedTab, ID_BATCH_FILE, buffer, 1024);
    if (wcslen(buffer) > 0) {
        cmd << L"-a \"" << buffer << L"\" ";
    }
    
    GetDlgItemText(hAdvancedTab, ID_OUTPUT_TEMPLATE, buffer, 1024);
    if (wcslen(buffer) > 0) {
        cmd << L"-o \"" << buffer << L"\" ";
    }
    
    if (IsDlgButtonChecked(hAdvancedTab, ID_SKIP_DOWNLOAD)) {
        cmd << L"--skip-download ";
    }
    
    if (IsDlgButtonChecked(hAdvancedTab, ID_JSON_OUTPUT)) {
        cmd << L"-J ";
    }
    
    if (IsDlgButtonChecked(hAdvancedTab, ID_LIST_FORMATS)) {
        cmd << L"-F ";
    }
    
    // Add post-processing options
    if (IsDlgButtonChecked(hPostProcessTab, ID_EMBED_SUBS)) {
        cmd << L"--embed-subs ";
    }
    
    if (IsDlgButtonChecked(hPostProcessTab, ID_EMBED_THUMBNAIL)) {
        cmd << L"--embed-thumbnail ";
    }
    
    if (IsDlgButtonChecked(hPostProcessTab, ID_EMBED_METADATA)) {
        cmd << L"--embed-metadata ";
    }
    
    if (IsDlgButtonChecked(hPostProcessTab, ID_EMBED_CHAPTERS)) {
        cmd << L"--embed-chapters ";
    }
    
    if (IsDlgButtonChecked(hPostProcessTab, ID_SPLIT_CHAPTERS)) {
        cmd << L"--split-chapters ";
    }
    
    if (IsDlgButtonChecked(hPostProcessTab, ID_FORCE_KEYFRAMES)) {
        cmd << L"--force-keyframes-at-cuts ";
    }
    
    // Add URL
    cmd << L"\"" << url << L"\"";
    
    // Create pipes for output
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    
    HANDLE hStdOutRead, hStdOutWrite;
    CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
    
    // Start process
    STARTUPINFO si = {};
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hStdOutWrite;
    si.hStdOutput = hStdOutWrite;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    PROCESS_INFORMATION pi = {};
    
    std::wstring cmdLine = cmd.str();
    if (CreateProcess(nullptr, &cmdLine[0], nullptr, nullptr, TRUE, 
                     CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT, 
                     nullptr, nullptr, &si, &pi)) {
        
        CloseHandle(hStdOutWrite);
        
        // Read output
        char buffer[4096];
        DWORD bytesRead;
        std::string output;
        
        while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
            
            // Convert to wide string for display
            std::wstring wOutput = UTF8ToWide(output);
            
            // Update console output
            SetWindowText(hConsoleOutput, wOutput.c_str());
            SendMessage(hConsoleOutput, EM_SETSEL, wOutput.length(), wOutput.length());
            SendMessage(hConsoleOutput, EM_SCROLLCARET, 0, 0);
            
            // Update progress
            size_t lastNewLine = wOutput.rfind(L'\n');
            if (lastNewLine != std::wstring::npos && lastNewLine > 0) {
                size_t prevNewLine = wOutput.rfind(L'\n', lastNewLine - 1);
                std::wstring lastLine = wOutput.substr(prevNewLine + 1, lastNewLine - prevNewLine - 1);
                UpdateProgress(lastLine);
                
                // Update status
                if (lastLine.find(L"[download]") != std::wstring::npos) {
                    SetWindowText(hStatusText, L"Downloading...");
                } else if (lastLine.find(L"[ffmpeg]") != std::wstring::npos) {
                    SetWindowText(hStatusText, L"Processing...");
                }
            }
        }
        
        CloseHandle(hStdOutRead);
        
        // Wait for process to complete
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (exitCode == 0) {
            SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
            SetWindowText(hStatusText, L"Download completed!");
            MessageBox(hMainWindow, L"Download completed successfully!", L"Success", MB_OK | MB_ICONINFORMATION);
        } else {
            SetWindowText(hStatusText, L"Download failed!");
            MessageBox(hMainWindow, L"Download failed. Check console output for details.", L"Error", MB_OK | MB_ICONERROR);
        }
    } else {
        SetWindowText(hStatusText, L"Failed to start yt-dlp!");
        MessageBox(hMainWindow, L"Failed to start yt-dlp. Make sure yt-dlp.exe is in the same folder.", L"Error", MB_OK | MB_ICONERROR);
    }
    
    EnableWindow(hDownloadBtn, TRUE);
    isDownloading = false;
}

std::wstring GetDesktopPath() {
    wchar_t path[MAX_PATH];
    if (SHGetFolderPath(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, path) == S_OK) {
        return std::wstring(path);
    }
    return L"C:\\";
}

void BrowseFolder() {
    BROWSEINFO bi = {};
    bi.lpszTitle = L"Select output folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != nullptr) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            SetWindowText(hOutputPath, path);
        }
        CoTaskMemFree(pidl);
    }
}

void LoadSettings() {
    std::ifstream file(settingsFile);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (key == "outputPath" && !value.empty()) {
                SetWindowText(hOutputPath, UTF8ToWide(value).c_str());
            } else if (key == "format") {
                SendMessage(hFormatCombo, CB_SELECTSTRING, -1, (LPARAM)UTF8ToWide(value).c_str());
            } else if (key == "quality") {
                SendMessage(hQualityCombo, CB_SELECTSTRING, -1, (LPARAM)UTF8ToWide(value).c_str());
            }
        }
    }
    
    file.close();
}

void SaveSettings() {
    std::ofstream file(settingsFile);
    if (!file.is_open()) return;
    
    wchar_t buffer[MAX_PATH];
    
    GetWindowText(hOutputPath, buffer, MAX_PATH);
    file << "outputPath=" << WideToUTF8(buffer) << std::endl;
    
    GetWindowText(hFormatCombo, buffer, MAX_PATH);
    file << "format=" << WideToUTF8(buffer) << std::endl;
    
    GetWindowText(hQualityCombo, buffer, MAX_PATH);
    file << "quality=" << WideToUTF8(buffer) << std::endl;
    
    file.close();
}
