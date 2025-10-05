#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Глобальные переменные
HINSTANCE g_hInst;
HWND g_hMainWnd;
HWND g_hTabControl;
HWND g_hProgressBar;
HWND g_hOutputEdit;
HWND g_hToggleOutputBtn;
HWND g_hDownloadBtn;
HANDLE g_hDownloadThread = NULL;
bool g_outputVisible = false;
int g_originalHeight = 600;

// Структура для хранения настроек
struct Settings {
    std::wstring savePath;
    std::wstring format;
    std::wstring quality;
    std::wstring proxy;
    std::wstring downloader;
    std::wstring downloaderArgs;
    std::wstring cookiesBrowser;
    std::wstring cookiesFile;
    std::wstring mergeFormat;
    std::wstring convertSubs;
    std::wstring convertThumbnails;
    bool ignoreConfig = false;
    bool liveFromStart = false;
    bool embedSubs = false;
    bool embedThumbnail = false;
    bool embedMetadata = false;
    bool embedChapters = false;
    bool splitChapters = false;
    bool skipDownload = false;
    bool checkFormats = false;
    bool saveJson = false;
    bool listFormats = false;
} g_settings;

// Контролы для вкладок
struct TabControls {
    // Основные
    HWND hUrlEdit;
    HWND hSavePathEdit;
    HWND hBrowseBtn;
    HWND hFormatCombo;
    HWND hQualityCombo;
    
    // Сеть
    HWND hProxyEdit;
    HWND hCookiesBrowserCombo;
    HWND hCookiesFileEdit;
    HWND hCookiesFileBrowseBtn;
    
    // Загрузчик
    HWND hDownloaderCombo;
    HWND hDownloaderArgsEdit;
    
    // Обработка
    HWND hMergeFormatCombo;
    HWND hConvertSubsCombo;
    HWND hConvertThumbnailsCombo;
    HWND hEmbedSubsCheck;
    HWND hEmbedThumbnailCheck;
    HWND hEmbedMetadataCheck;
    HWND hEmbedChaptersCheck;
    HWND hSplitChaptersCheck;
    
    // Дополнительно
    HWND hIgnoreConfigCheck;
    HWND hLiveFromStartCheck;
    HWND hCheckFormatsCheck;
    HWND hSkipDownloadCheck;
    HWND hSaveJsonCheck;
    HWND hListFormatsCheck;
    HWND hBatchFileEdit;
    HWND hBatchFileBrowseBtn;
    HWND hOutputTemplateEdit;
    HWND hRemoveChaptersEdit;
    
    // Панели вкладок
    HWND hTabBasic;
    HWND hTabNetwork;
    HWND hTabDownloader;
    HWND hTabProcessing;
    HWND hTabAdvanced;
} g_controls;

// Утилиты для работы со строками
std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result.c_str();
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, NULL, NULL);
    return result.c_str();
}

std::wstring GetDesktopPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
        return path;
    }
    return L"";
}

std::wstring GetAppDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring::size_type pos = std::wstring(path).find_last_of(L"\\/");
    return std::wstring(path).substr(0, pos);
}

// Сохранение и загрузка настроек
void SaveSettings() {
    std::wstring iniPath = GetAppDirectory() + L"\\settings.ini";
    
    WritePrivateProfileStringW(L"Paths", L"SavePath", g_settings.savePath.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Download", L"Format", g_settings.format.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Download", L"Quality", g_settings.quality.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Network", L"Proxy", g_settings.proxy.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Network", L"CookiesBrowser", g_settings.cookiesBrowser.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Network", L"CookiesFile", g_settings.cookiesFile.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Downloader", L"Name", g_settings.downloader.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Downloader", L"Args", g_settings.downloaderArgs.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Processing", L"MergeFormat", g_settings.mergeFormat.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Processing", L"ConvertSubs", g_settings.convertSubs.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Processing", L"ConvertThumbnails", g_settings.convertThumbnails.c_str(), iniPath.c_str());
    
    WritePrivateProfileStringW(L"Options", L"IgnoreConfig", g_settings.ignoreConfig ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"LiveFromStart", g_settings.liveFromStart ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"EmbedSubs", g_settings.embedSubs ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"EmbedThumbnail", g_settings.embedThumbnail ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"EmbedMetadata", g_settings.embedMetadata ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"EmbedChapters", g_settings.embedChapters ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"SplitChapters", g_settings.splitChapters ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"CheckFormats", g_settings.checkFormats ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"SkipDownload", g_settings.skipDownload ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"SaveJson", g_settings.saveJson ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"Options", L"ListFormats", g_settings.listFormats ? L"1" : L"0", iniPath.c_str());
}

void LoadSettings() {
    std::wstring iniPath = GetAppDirectory() + L"\\settings.ini";
    wchar_t buffer[1024];
    
    GetPrivateProfileStringW(L"Paths", L"SavePath", GetDesktopPath().c_str(), buffer, 1024, iniPath.c_str());
    g_settings.savePath = buffer;
    
    GetPrivateProfileStringW(L"Download", L"Format", L"bestvideo+bestaudio/best", buffer, 1024, iniPath.c_str());
    g_settings.format = buffer;
    
    GetPrivateProfileStringW(L"Download", L"Quality", L"best", buffer, 1024, iniPath.c_str());
    g_settings.quality = buffer;
    
    GetPrivateProfileStringW(L"Network", L"Proxy", L"", buffer, 1024, iniPath.c_str());
    g_settings.proxy = buffer;
    
    GetPrivateProfileStringW(L"Network", L"CookiesBrowser", L"", buffer, 1024, iniPath.c_str());
    g_settings.cookiesBrowser = buffer;
    
    GetPrivateProfileStringW(L"Network", L"CookiesFile", L"", buffer, 1024, iniPath.c_str());
    g_settings.cookiesFile = buffer;
    
    GetPrivateProfileStringW(L"Downloader", L"Name", L"", buffer, 1024, iniPath.c_str());
    g_settings.downloader = buffer;
    
    GetPrivateProfileStringW(L"Downloader", L"Args", L"", buffer, 1024, iniPath.c_str());
    g_settings.downloaderArgs = buffer;
    
    GetPrivateProfileStringW(L"Processing", L"MergeFormat", L"mp4", buffer, 1024, iniPath.c_str());
    g_settings.mergeFormat = buffer;
    
    GetPrivateProfileStringW(L"Processing", L"ConvertSubs", L"none", buffer, 1024, iniPath.c_str());
    g_settings.convertSubs = buffer;
    
    GetPrivateProfileStringW(L"Processing", L"ConvertThumbnails", L"none", buffer, 1024, iniPath.c_str());
    g_settings.convertThumbnails = buffer;
    
    g_settings.ignoreConfig = GetPrivateProfileIntW(L"Options", L"IgnoreConfig", 0, iniPath.c_str()) != 0;
    g_settings.liveFromStart = GetPrivateProfileIntW(L"Options", L"LiveFromStart", 0, iniPath.c_str()) != 0;
    g_settings.embedSubs = GetPrivateProfileIntW(L"Options", L"EmbedSubs", 0, iniPath.c_str()) != 0;
    g_settings.embedThumbnail = GetPrivateProfileIntW(L"Options", L"EmbedThumbnail", 0, iniPath.c_str()) != 0;
    g_settings.embedMetadata = GetPrivateProfileIntW(L"Options", L"EmbedMetadata", 0, iniPath.c_str()) != 0;
    g_settings.embedChapters = GetPrivateProfileIntW(L"Options", L"EmbedChapters", 0, iniPath.c_str()) != 0;
    g_settings.splitChapters = GetPrivateProfileIntW(L"Options", L"SplitChapters", 0, iniPath.c_str()) != 0;
    g_settings.checkFormats = GetPrivateProfileIntW(L"Options", L"CheckFormats", 0, iniPath.c_str()) != 0;
    g_settings.skipDownload = GetPrivateProfileIntW(L"Options", L"SkipDownload", 0, iniPath.c_str()) != 0;
    g_settings.saveJson = GetPrivateProfileIntW(L"Options", L"SaveJson", 0, iniPath.c_str()) != 0;
    g_settings.listFormats = GetPrivateProfileIntW(L"Options", L"ListFormats", 0, iniPath.c_str()) != 0;
}

void UpdateSettingsFromControls() {
    wchar_t buffer[1024];
    
    GetWindowTextW(g_controls.hSavePathEdit, buffer, 1024);
    g_settings.savePath = buffer;
    
    int idx = SendMessageW(g_controls.hFormatCombo, CB_GETCURSEL, 0, 0);
    if (idx != CB_ERR) {
        SendMessageW(g_controls.hFormatCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
        g_settings.format = buffer;
    }
    
    idx = SendMessageW(g_controls.hQualityCombo, CB_GETCURSEL, 0, 0);
    if (idx != CB_ERR) {
        SendMessageW(g_controls.hQualityCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
        g_settings.quality = buffer;
    }
    
    GetWindowTextW(g_controls.hProxyEdit, buffer, 1024);
    g_settings.proxy = buffer;
    
    idx = SendMessageW(g_controls.hCookiesBrowserCombo, CB_GETCURSEL, 0, 0);
    if (idx != CB_ERR && idx != 0) {
        SendMessageW(g_controls.hCookiesBrowserCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
        g_settings.cookiesBrowser = buffer;
    } else {
        g_settings.cookiesBrowser = L"";
    }
    
    GetWindowTextW(g_controls.hCookiesFileEdit, buffer, 1024);
    g_settings.cookiesFile = buffer;
    
    idx = SendMessageW(g_controls.hDownloaderCombo, CB_GETCURSEL, 0, 0);
    if (idx != CB_ERR && idx != 0) {
        SendMessageW(g_controls.hDownloaderCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
        g_settings.downloader = buffer;
    } else {
        g_settings.downloader = L"";
    }
    
    GetWindowTextW(g_controls.hDownloaderArgsEdit, buffer, 1024);
    g_settings.downloaderArgs = buffer;
    
    idx = SendMessageW(g_controls.hMergeFormatCombo, CB_GETCURSEL, 0, 0);
    if (idx != CB_ERR) {
        SendMessageW(g_controls.hMergeFormatCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
        g_settings.mergeFormat = buffer;
    }
    
    idx = SendMessageW(g_controls.hConvertSubsCombo, CB_GETCURSEL, 0, 0);
    if (idx != CB_ERR) {
        SendMessageW(g_controls.hConvertSubsCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
        g_settings.convertSubs = buffer;
    }
    
    idx = SendMessageW(g_controls.hConvertThumbnailsCombo, CB_GETCURSEL, 0, 0);
    if (idx != CB_ERR) {
        SendMessageW(g_controls.hConvertThumbnailsCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
        g_settings.convertThumbnails = buffer;
    }
    
    g_settings.embedSubs = SendMessageW(g_controls.hEmbedSubsCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.embedThumbnail = SendMessageW(g_controls.hEmbedThumbnailCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.embedMetadata = SendMessageW(g_controls.hEmbedMetadataCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.embedChapters = SendMessageW(g_controls.hEmbedChaptersCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.splitChapters = SendMessageW(g_controls.hSplitChaptersCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.ignoreConfig = SendMessageW(g_controls.hIgnoreConfigCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.liveFromStart = SendMessageW(g_controls.hLiveFromStartCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.checkFormats = SendMessageW(g_controls.hCheckFormatsCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.skipDownload = SendMessageW(g_controls.hSkipDownloadCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.saveJson = SendMessageW(g_controls.hSaveJsonCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_settings.listFormats = SendMessageW(g_controls.hListFormatsCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void UpdateControlsFromSettings() {
    SetWindowTextW(g_controls.hSavePathEdit, g_settings.savePath.c_str());
    
    SendMessageW(g_controls.hFormatCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.format.c_str());
    SendMessageW(g_controls.hQualityCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.quality.c_str());
    
    SetWindowTextW(g_controls.hProxyEdit, g_settings.proxy.c_str());
    
    if (!g_settings.cookiesBrowser.empty()) {
        SendMessageW(g_controls.hCookiesBrowserCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.cookiesBrowser.c_str());
    }
    
    SetWindowTextW(g_controls.hCookiesFileEdit, g_settings.cookiesFile.c_str());
    
    if (!g_settings.downloader.empty()) {
        SendMessageW(g_controls.hDownloaderCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.downloader.c_str());
    }
    
    SetWindowTextW(g_controls.hDownloaderArgsEdit, g_settings.downloaderArgs.c_str());
    
    SendMessageW(g_controls.hMergeFormatCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.mergeFormat.c_str());
    SendMessageW(g_controls.hConvertSubsCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.convertSubs.c_str());
    SendMessageW(g_controls.hConvertThumbnailsCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.convertThumbnails.c_str());
    
    SendMessageW(g_controls.hEmbedSubsCheck, BM_SETCHECK, g_settings.embedSubs ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hEmbedThumbnailCheck, BM_SETCHECK, g_settings.embedThumbnail ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hEmbedMetadataCheck, BM_SETCHECK, g_settings.embedMetadata ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hEmbedChaptersCheck, BM_SETCHECK, g_settings.embedChapters ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hSplitChaptersCheck, BM_SETCHECK, g_settings.splitChapters ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hIgnoreConfigCheck, BM_SETCHECK, g_settings.ignoreConfig ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hLiveFromStartCheck, BM_SETCHECK, g_settings.liveFromStart ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hCheckFormatsCheck, BM_SETCHECK, g_settings.checkFormats ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hSkipDownloadCheck, BM_SETCHECK, g_settings.skipDownload ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hSaveJsonCheck, BM_SETCHECK, g_settings.saveJson ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.hListFormatsCheck, BM_SETCHECK, g_settings.listFormats ? BST_CHECKED : BST_UNCHECKED, 0);
}

// Создание вкладок
HWND CreateTabPanel(HWND hParent, int id) {
    return CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE, 
                          0, 0, 0, 0, hParent, (HMENU)(INT_PTR)id, g_hInst, NULL);
}

void CreateBasicTab(HWND hTab) {
    int y = 15;
    
    CreateWindowExW(0, L"Static", L"URL видео:", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hUrlEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                         120, y, 550, 24, hTab, (HMENU)IDC_URL_EDIT, g_hInst, NULL);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Путь сохранения:", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hSavePathEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                              120, y, 450, 24, hTab, (HMENU)IDC_SAVEPATH_EDIT, g_hInst, NULL);
    g_controls.hBrowseBtn = CreateWindowExW(0, L"Button", L"Обзор...",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                           580, y, 90, 24, hTab, (HMENU)IDC_BROWSE_BTN, g_hInst, NULL);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Формат:", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hFormatCombo = CreateWindowExW(0, L"ComboBox", L"",
                                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                             120, y, 250, 200, hTab, (HMENU)IDC_FORMAT_COMBO, g_hInst, NULL);
    SendMessageW(g_controls.hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"bestvideo+bestaudio/best");
    SendMessageW(g_controls.hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"best");
    SendMessageW(g_controls.hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"bestvideo+bestaudio");
    SendMessageW(g_controls.hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"bestvideo");
    SendMessageW(g_controls.hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"bestaudio");
    SendMessageW(g_controls.hFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"worst");
    SendMessageW(g_controls.hFormatCombo, CB_SETCURSEL, 0, 0);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Качество:", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hQualityCombo = CreateWindowExW(0, L"ComboBox", L"",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                              120, y, 250, 200, hTab, (HMENU)IDC_QUALITY_COMBO, g_hInst, NULL);
    SendMessageW(g_controls.hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"best");
    SendMessageW(g_controls.hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"2160p");
    SendMessageW(g_controls.hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1440p");
    SendMessageW(g_controls.hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"1080p");
    SendMessageW(g_controls.hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"720p");
    SendMessageW(g_controls.hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"480p");
    SendMessageW(g_controls.hQualityCombo, CB_ADDSTRING, 0, (LPARAM)L"360p");
    SendMessageW(g_controls.hQualityCombo, CB_SETCURSEL, 0, 0);
}

void CreateNetworkTab(HWND hTab) {
    int y = 15;
    
    CreateWindowExW(0, L"Static", L"Прокси URL:", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hProxyEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                           120, y, 550, 24, hTab, (HMENU)IDC_PROXY_EDIT, g_hInst, NULL);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Cookies (браузер):", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hCookiesBrowserCombo = CreateWindowExW(0, L"ComboBox", L"",
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                                     120, y, 250, 200, hTab, (HMENU)IDC_COOKIES_BROWSER_COMBO, g_hInst, NULL);
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"Не использовать");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"chrome");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"firefox");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"edge");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"opera");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"brave");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"chromium");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"safari");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"vivaldi");
    SendMessageW(g_controls.hCookiesBrowserCombo, CB_SETCURSEL, 0, 0);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Cookies (файл):", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hCookiesFileEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                 120, y, 450, 24, hTab, (HMENU)IDC_COOKIES_FILE_EDIT, g_hInst, NULL);
    g_controls.hCookiesFileBrowseBtn = CreateWindowExW(0, L"Button", L"Обзор...",
                                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                                      580, y, 90, 24, hTab, (HMENU)IDC_COOKIES_FILE_BROWSE_BTN, g_hInst, NULL);
}

void CreateDownloaderTab(HWND hTab) {
    int y = 15;
    
    CreateWindowExW(0, L"Static", L"Загрузчик:", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hDownloaderCombo = CreateWindowExW(0, L"ComboBox", L"",
                                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                                 120, y, 250, 200, hTab, (HMENU)IDC_DOWNLOADER_COMBO, g_hInst, NULL);
    SendMessageW(g_controls.hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"По умолчанию");
    SendMessageW(g_controls.hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"native");
    SendMessageW(g_controls.hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"aria2c");
    SendMessageW(g_controls.hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"axel");
    SendMessageW(g_controls.hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"curl");
    SendMessageW(g_controls.hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"ffmpeg");
    SendMessageW(g_controls.hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"httpie");
    SendMessageW(g_controls.hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"wget");
    SendMessageW(g_controls.hDownloaderCombo, CB_SETCURSEL, 0, 0);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Аргументы загрузчика:", WS_CHILD | WS_VISIBLE,
                   10, y, 100, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hDownloaderArgsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                    120, y, 550, 24, hTab, (HMENU)IDC_DOWNLOADER_ARGS_EDIT, g_hInst, NULL);
}

void CreateProcessingTab(HWND hTab) {
    int y = 15;
    
    CreateWindowExW(0, L"Static", L"Формат объединения:", WS_CHILD | WS_VISIBLE,
                   10, y, 120, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hMergeFormatCombo = CreateWindowExW(0, L"ComboBox", L"",
                                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                                  140, y, 150, 200, hTab, (HMENU)IDC_MERGE_FORMAT_COMBO, g_hInst, NULL);
    SendMessageW(g_controls.hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mp4");
    SendMessageW(g_controls.hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mkv");
    SendMessageW(g_controls.hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"webm");
    SendMessageW(g_controls.hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"avi");
    SendMessageW(g_controls.hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"flv");
    SendMessageW(g_controls.hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mov");
    SendMessageW(g_controls.hMergeFormatCombo, CB_SETCURSEL, 0, 0);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Конвертация субтитров:", WS_CHILD | WS_VISIBLE,
                   10, y, 120, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hConvertSubsCombo = CreateWindowExW(0, L"ComboBox", L"",
                                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                                  140, y, 150, 200, hTab, (HMENU)IDC_CONVERT_SUBS_COMBO, g_hInst, NULL);
    SendMessageW(g_controls.hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"none");
    SendMessageW(g_controls.hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"srt");
    SendMessageW(g_controls.hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"ass");
    SendMessageW(g_controls.hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"vtt");
    SendMessageW(g_controls.hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"lrc");
    SendMessageW(g_controls.hConvertSubsCombo, CB_SETCURSEL, 0, 0);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Конвертация миниатюр:", WS_CHILD | WS_VISIBLE,
                   10, y, 120, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hConvertThumbnailsCombo = CreateWindowExW(0, L"ComboBox", L"",
                                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                                        140, y, 150, 200, hTab, (HMENU)IDC_CONVERT_THUMBNAILS_COMBO, g_hInst, NULL);
    SendMessageW(g_controls.hConvertThumbnailsCombo, CB_ADDSTRING, 0, (LPARAM)L"none");
    SendMessageW(g_controls.hConvertThumbnailsCombo, CB_ADDSTRING, 0, (LPARAM)L"jpg");
    SendMessageW(g_controls.hConvertThumbnailsCombo, CB_ADDSTRING, 0, (LPARAM)L"png");
    SendMessageW(g_controls.hConvertThumbnailsCombo, CB_ADDSTRING, 0, (LPARAM)L"webp");
    SendMessageW(g_controls.hConvertThumbnailsCombo, CB_SETCURSEL, 0, 0);
    y += 35;
    
    g_controls.hEmbedSubsCheck = CreateWindowExW(0, L"Button", L"Встраивать субтитры",
                                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                10, y, 200, 20, hTab, (HMENU)IDC_EMBED_SUBS_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hEmbedThumbnailCheck = CreateWindowExW(0, L"Button", L"Встраивать миниатюру",
                                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                      10, y, 200, 20, hTab, (HMENU)IDC_EMBED_THUMBNAIL_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hEmbedMetadataCheck = CreateWindowExW(0, L"Button", L"Встраивать метаданные",
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                     10, y, 200, 20, hTab, (HMENU)IDC_EMBED_METADATA_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hEmbedChaptersCheck = CreateWindowExW(0, L"Button", L"Встраивать главы",
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                     10, y, 200, 20, hTab, (HMENU)IDC_EMBED_CHAPTERS_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hSplitChaptersCheck = CreateWindowExW(0, L"Button", L"Разделить по главам",
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                     10, y, 200, 20, hTab, (HMENU)IDC_SPLIT_CHAPTERS_CHECK, g_hInst, NULL);
}

void CreateAdvancedTab(HWND hTab) {
    int y = 15;
    
    g_controls.hIgnoreConfigCheck = CreateWindowExW(0, L"Button", L"Игнорировать конфигурационные файлы (--ignore-config)",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                    10, y, 400, 20, hTab, (HMENU)IDC_IGNORE_CONFIG_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hLiveFromStartCheck = CreateWindowExW(0, L"Button", L"Скачивать трансляцию с начала (--live-from-start)",
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                     10, y, 400, 20, hTab, (HMENU)IDC_LIVE_FROM_START_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hCheckFormatsCheck = CreateWindowExW(0, L"Button", L"Проверять форматы (--check-formats)",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                    10, y, 300, 20, hTab, (HMENU)IDC_CHECK_FORMATS_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hSkipDownloadCheck = CreateWindowExW(0, L"Button", L"Пропустить загрузку (--skip-download)",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                    10, y, 300, 20, hTab, (HMENU)IDC_SKIP_DOWNLOAD_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hSaveJsonCheck = CreateWindowExW(0, L"Button", L"Сохранить JSON информацию (-J)",
                                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                               10, y, 300, 20, hTab, (HMENU)IDC_SAVE_JSON_CHECK, g_hInst, NULL);
    y += 25;
    
    g_controls.hListFormatsCheck = CreateWindowExW(0, L"Button", L"Показать доступные форматы (-F)",
                                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                  10, y, 300, 20, hTab, (HMENU)IDC_LIST_FORMATS_CHECK, g_hInst, NULL);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Batch файл (список URL):", WS_CHILD | WS_VISIBLE,
                   10, y, 120, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hBatchFileEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                               140, y, 430, 24, hTab, (HMENU)IDC_BATCH_FILE_EDIT, g_hInst, NULL);
    g_controls.hBatchFileBrowseBtn = CreateWindowExW(0, L"Button", L"Обзор...",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                                    580, y, 90, 24, hTab, (HMENU)IDC_BATCH_FILE_BROWSE_BTN, g_hInst, NULL);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Шаблон имени файла (-o):", WS_CHILD | WS_VISIBLE,
                   10, y, 120, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hOutputTemplateEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                    140, y, 530, 24, hTab, (HMENU)IDC_OUTPUT_TEMPLATE_EDIT, g_hInst, NULL);
    y += 35;
    
    CreateWindowExW(0, L"Static", L"Удалить главы (regex):", WS_CHILD | WS_VISIBLE,
                   10, y, 120, 20, hTab, NULL, g_hInst, NULL);
    g_controls.hRemoveChaptersEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                    140, y, 530, 24, hTab, (HMENU)IDC_REMOVE_CHAPTERS_EDIT, g_hInst, NULL);
}

void ShowTab(int index) {
    ShowWindow(g_controls.hTabBasic, index == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_controls.hTabNetwork, index == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_controls.hTabDownloader, index == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_controls.hTabProcessing, index == 3 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_controls.hTabAdvanced, index == 4 ? SW_SHOW : SW_HIDE);
}

void ResizeControls(int width, int height) {
    int tabHeight = height - 150;
    if (g_outputVisible) {
        tabHeight = height - 350;
    }
    
    // Tab control
    MoveWindow(g_hTabControl, 10, 10, width - 20, tabHeight, TRUE);
    
    // Tab panels
    RECT rc;
    GetClientRect(g_hTabControl, &rc);
    TabCtrl_AdjustRect(g_hTabControl, FALSE, &rc);
    
    MoveWindow(g_controls.hTabBasic, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    MoveWindow(g_controls.hTabNetwork, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    MoveWindow(g_controls.hTabDownloader, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    MoveWindow(g_controls.hTabProcessing, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    MoveWindow(g_controls.hTabAdvanced, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    
    // Resize controls in Basic tab
    if (g_controls.hUrlEdit) {
        MoveWindow(g_controls.hUrlEdit, 120, 15, rc.right - rc.left - 130, 24, TRUE);
    }
    if (g_controls.hSavePathEdit) {
        MoveWindow(g_controls.hSavePathEdit, 120, 50, rc.right - rc.left - 220, 24, TRUE);
        MoveWindow(g_controls.hBrowseBtn, rc.right - rc.left - 90, 50, 80, 24, TRUE);
    }
    
    int bottomY = 10 + tabHeight;
    
    // Progress bar
    MoveWindow(g_hProgressBar, 10, bottomY, width - 20, 25, TRUE);
    bottomY += 30;
    
    // Toggle output button
    MoveWindow(g_hToggleOutputBtn, 10, bottomY, 150, 25, TRUE);
    bottomY += 30;
    
    // Output edit (if visible)
    if (g_outputVisible) {
        MoveWindow(g_hOutputEdit, 10, bottomY, width - 20, 150, TRUE);
        bottomY += 155;
    }
    
    // Download button
    MoveWindow(g_hDownloadBtn, width - 120, bottomY, 110, 30, TRUE);
}

void ToggleOutput() {
    g_outputVisible = !g_outputVisible;
    
    RECT rc;
    GetClientRect(g_hMainWnd, &rc);
    
    if (g_outputVisible) {
        SetWindowTextW(g_hToggleOutputBtn, L"Скрыть вывод ▲");
        ShowWindow(g_hOutputEdit, SW_SHOW);
    } else {
        SetWindowTextW(g_hToggleOutputBtn, L"Показать вывод ▼");
        ShowWindow(g_hOutputEdit, SW_HIDE);
    }
    
    ResizeControls(rc.right, rc.bottom);
}

void AppendOutput(const std::wstring& text) {
    int len = GetWindowTextLengthW(g_hOutputEdit);
    SendMessageW(g_hOutputEdit, EM_SETSEL, len, len);
    SendMessageW(g_hOutputEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
}

std::wstring BuildYtdlpCommand(const std::wstring& url) {
    std::wstring cmd = L"yt-dlp.exe";
    
    if (g_settings.ignoreConfig) {
        cmd += L" --ignore-config";
    }
    
    if (g_settings.liveFromStart) {
        cmd += L" --live-from-start";
    }
    
    if (!g_settings.proxy.empty()) {
        cmd += L" --proxy \"" + g_settings.proxy + L"\"";
    }
    
    if (!g_settings.cookiesBrowser.empty()) {
        cmd += L" --cookies-from-browser " + g_settings.cookiesBrowser;
    }
    
    if (!g_settings.cookiesFile.empty()) {
        cmd += L" --cookies \"" + g_settings.cookiesFile + L"\"";
    }
    
    if (!g_settings.downloader.empty()) {
        cmd += L" --downloader " + g_settings.downloader;
    }
    
    if (!g_settings.downloaderArgs.empty()) {
        cmd += L" --downloader-args \"" + g_settings.downloaderArgs + L"\"";
    }
    
    wchar_t batchFile[1024];
    GetWindowTextW(g_controls.hBatchFileEdit, batchFile, 1024);
    if (wcslen(batchFile) > 0) {
        cmd += L" --batch-file \"" + std::wstring(batchFile) + L"\"";
    }
    
    wchar_t outputTemplate[1024];
    GetWindowTextW(g_controls.hOutputTemplateEdit, outputTemplate, 1024);
    if (wcslen(outputTemplate) > 0) {
        cmd += L" -o \"" + std::wstring(outputTemplate) + L"\"";
    } else {
        cmd += L" -o \"%(title)s.%(ext)s\"";
    }
    
    if (!g_settings.format.empty() && g_settings.format != L"best") {
        cmd += L" -f \"" + g_settings.format + L"\"";
    }
    
    if (g_settings.listFormats) {
        cmd += L" -F";
    }
    
    if (!g_settings.mergeFormat.empty()) {
        cmd += L" --merge-output-format " + g_settings.mergeFormat;
    }
    
    if (g_settings.checkFormats) {
        cmd += L" --check-formats";
    }
    
    if (g_settings.embedSubs) {
        cmd += L" --embed-subs";
    }
    
    if (g_settings.embedThumbnail) {
        cmd += L" --embed-thumbnail";
    }
    
    if (g_settings.embedMetadata) {
        cmd += L" --embed-metadata";
    }
    
    if (g_settings.embedChapters) {
        cmd += L" --embed-chapters";
    }
    
    if (g_settings.convertSubs != L"none") {
        cmd += L" --convert-subs " + g_settings.convertSubs;
    }
    
    if (g_settings.convertThumbnails != L"none") {
        cmd += L" --convert-thumbnails " + g_settings.convertThumbnails;
    }
    
    if (g_settings.splitChapters) {
        cmd += L" --split-chapters";
    }
    
    wchar_t removeChapters[1024];
    GetWindowTextW(g_controls.hRemoveChaptersEdit, removeChapters, 1024);
    if (wcslen(removeChapters) > 0) {
        cmd += L" --remove-chapters \"" + std::wstring(removeChapters) + L"\"";
    }
    
    if (g_settings.skipDownload) {
        cmd += L" --skip-download";
    }
    
    if (g_settings.saveJson) {
        cmd += L" -J";
    }
    
    cmd += L" --newline --encoding utf-8";
    cmd += L" -P \"" + g_settings.savePath + L"\"";
    cmd += L" \"" + url + L"\"";
    
    return cmd;
}

DWORD WINAPI DownloadThread(LPVOID lpParam) {
    std::wstring url = *(std::wstring*)lpParam;
    delete (std::wstring*)lpParam;
    
    SendMessageW(g_hProgressBar, PBM_SETMARQUEE, TRUE, 0);
    SetWindowTextW(g_hDownloadBtn, L"Остановить");
    AppendOutput(L"Начало загрузки...\r\n");
    
    std::wstring cmd = BuildYtdlpCommand(url);
    AppendOutput(L"Команда: " + cmd + L"\r\n\r\n");
    
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    
    HANDLE hStdOutRead, hStdOutWrite;
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        AppendOutput(L"Ошибка создания pipe\r\n");
        SendMessageW(g_hProgressBar, PBM_SETMARQUEE, FALSE, 0);
        SetWindowTextW(g_hDownloadBtn, L"Скачать");
        return 1;
    }
    
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
    
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;
    
    PROCESS_INFORMATION pi;
    
    wchar_t* cmdLine = _wcsdup(cmd.c_str());
    BOOL result = CreateProcessW(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, 
                                g_settings.savePath.c_str(), &si, &pi);
    free(cmdLine);
    
    CloseHandle(hStdOutWrite);
    
    if (!result) {
        AppendOutput(L"Ошибка запуска yt-dlp. Убедитесь, что yt-dlp.exe находится в PATH или в папке с программой.\r\n");
        CloseHandle(hStdOutRead);
        SendMessageW(g_hProgressBar, PBM_SETMARQUEE, FALSE, 0);
        SetWindowTextW(g_hDownloadBtn, L"Скачать");
        return 1;
    }
    
    // Чтение вывода
    char buffer[4096];
    DWORD bytesRead;
    std::string outputBuffer;
    
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        outputBuffer += buffer;
        
        // Обработка построчно
        size_t pos;
        while ((pos = outputBuffer.find('\n')) != std::string::npos) {
            std::string line = outputBuffer.substr(0, pos + 1);
            outputBuffer.erase(0, pos + 1);
            
            // Конвертация из UTF-8 в Wide
            std::wstring wline = Utf8ToWide(line);
            AppendOutput(wline);
            
            // Парсинг прогресса
            if (line.find("[download]") != std::string::npos) {
                size_t percentPos = line.find('%');
                if (percentPos != std::string::npos) {
                    // Ищем число перед %
                    size_t start = percentPos;
                    while (start > 0 && (isdigit(line[start - 1]) || line[start - 1] == '.')) {
                        start--;
                    }
                    if (start < percentPos) {
                        std::string percentStr = line.substr(start, percentPos - start);
                        float percent = (float)atof(percentStr.c_str());
                        SendMessageW(g_hProgressBar, PBM_SETMARQUEE, FALSE, 0);
                        SendMessageW(g_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                        SendMessageW(g_hProgressBar, PBM_SETPOS, (int)percent, 0);
                    }
                }
            }
        }
    }
    
    if (!outputBuffer.empty()) {
        AppendOutput(Utf8ToWide(outputBuffer));
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdOutRead);
    
    if (exitCode == 0) {
        AppendOutput(L"\r\nЗагрузка завершена успешно!\r\n");
        SendMessageW(g_hProgressBar, PBM_SETPOS, 100, 0);
    } else {
        AppendOutput(L"\r\nОшибка при загрузке!\r\n");
        SendMessageW(g_hProgressBar, PBM_SETPOS, 0, 0);
    }
    
    SetWindowTextW(g_hDownloadBtn, L"Скачать");
    g_hDownloadThread = NULL;
    
    return exitCode;
}

void StartDownload() {
    if (g_hDownloadThread != NULL) {
        // TODO: Implement process termination
        return;
    }
    
    wchar_t url[2048];
    GetWindowTextW(g_controls.hUrlEdit, url, 2048);
    
    if (wcslen(url) == 0) {
        MessageBoxW(g_hMainWnd, L"Введите URL видео!", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    
    UpdateSettingsFromControls();
    SaveSettings();
    
    SetWindowTextW(g_hOutputEdit, L"");
    SendMessageW(g_hProgressBar, PBM_SETPOS, 0, 0);
    
    std::wstring* urlCopy = new std::wstring(url);
    g_hDownloadThread = CreateThread(NULL, 0, DownloadThread, urlCopy, 0, NULL);
}

void BrowseFolder(HWND hEdit) {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = g_hMainWnd;
    bi.lpszTitle = L"Выберите папку для сохранения";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            SetWindowTextW(hEdit, path);
        }
        CoTaskMemFree(pidl);
    }
}

void BrowseFile(HWND hEdit, const wchar_t* filter, const wchar_t* title) {
    OPENFILENAMEW ofn = { 0 };
    wchar_t fileName[MAX_PATH] = L"";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hMainWnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    
    if (GetOpenFileNameW(&ofn)) {
        SetWindowTextW(hEdit, fileName);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Создание TabControl
        g_hTabControl = CreateWindowExW(0, WC_TABCONTROLW, L"",
                                       WS_CHILD | WS_VISIBLE | WS_CLIPTABSTOP,
                                       10, 10, 780, 450, hWnd, (HMENU)IDC_TAB_CONTROL, g_hInst, NULL);
        
        TCITEMW tie;
        tie.mask = TCIF_TEXT;
        
        tie.pszText = (LPWSTR)L"Основное";
        TabCtrl_InsertItem(g_hTabControl, 0, &tie);
        
        tie.pszText = (LPWSTR)L"Сеть";
        TabCtrl_InsertItem(g_hTabControl, 1, &tie);
        
        tie.pszText = (LPWSTR)L"Загрузчик";
        TabCtrl_InsertItem(g_hTabControl, 2, &tie);
        
        tie.pszText = (LPWSTR)L"Обработка";
        TabCtrl_InsertItem(g_hTabControl, 3, &tie);
        
        tie.pszText = (LPWSTR)L"Дополнительно";
        TabCtrl_InsertItem(g_hTabControl, 4, &tie);
        
        // Создание панелей вкладок
        g_controls.hTabBasic = CreateTabPanel(g_hTabControl, IDC_TAB_BASIC);
        g_controls.hTabNetwork = CreateTabPanel(g_hTabControl, IDC_TAB_NETWORK);
        g_controls.hTabDownloader = CreateTabPanel(g_hTabControl, IDC_TAB_DOWNLOADER);
        g_controls.hTabProcessing = CreateTabPanel(g_hTabControl, IDC_TAB_PROCESSING);
        g_controls.hTabAdvanced = CreateTabPanel(g_hTabControl, IDC_TAB_ADVANCED);
        
        CreateBasicTab(g_controls.hTabBasic);
        CreateNetworkTab(g_controls.hTabNetwork);
        CreateDownloaderTab(g_controls.hTabDownloader);
        CreateProcessingTab(g_controls.hTabProcessing);
        CreateAdvancedTab(g_controls.hTabAdvanced);
        
        ShowTab(0);
        
        // Progress bar
        g_hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                                        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                        10, 470, 780, 25, hWnd, (HMENU)IDC_PROGRESS_BAR, g_hInst, NULL);
        SendMessageW(g_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(g_hProgressBar, PBM_SETSTEP, 1, 0);
        
        // Toggle output button
        g_hToggleOutputBtn = CreateWindowExW(0, L"Button", L"Показать вывод ▼",
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                            10, 500, 150, 25, hWnd, (HMENU)IDC_TOGGLE_OUTPUT_BTN, g_hInst, NULL);
        
        // Output edit
        g_hOutputEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", L"",
                                       WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                       10, 530, 780, 150, hWnd, (HMENU)IDC_OUTPUT_EDIT, g_hInst, NULL);
        
        // Download button
        g_hDownloadBtn = CreateWindowExW(0, L"Button", L"Скачать",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                        680, 500, 110, 30, hWnd, (HMENU)IDC_DOWNLOAD_BTN, g_hInst, NULL);
        
        LoadSettings();
        UpdateControlsFromSettings();
        
        RECT rc;
        GetClientRect(hWnd, &rc);
        ResizeControls(rc.right, rc.bottom);
        break;
    }
    
    case WM_NOTIFY: {
        NMHDR* pNmhdr = (NMHDR*)lParam;
        if (pNmhdr->idFrom == IDC_TAB_CONTROL && pNmhdr->code == TCN_SELCHANGE) {
            int index = TabCtrl_GetCurSel(g_hTabControl);
            ShowTab(index);
        }
        break;
    }
    
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_BROWSE_BTN:
            BrowseFolder(g_controls.hSavePathEdit);
            break;
        case IDC_COOKIES_FILE_BROWSE_BTN:
            BrowseFile(g_controls.hCookiesFileEdit, L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0", L"Выберите файл cookies");
            break;
        case IDC_BATCH_FILE_BROWSE_BTN:
            BrowseFile(g_controls.hBatchFileEdit, L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0", L"Выберите batch файл");
            break;
        case IDC_TOGGLE_OUTPUT_BTN:
            ToggleOutput();
            break;
        case IDC_DOWNLOAD_BTN:
            StartDownload();
            break;
        }
        break;
    }
    
    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        ResizeControls(width, height);
        break;
    }
    
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 700;
        mmi->ptMinTrackSize.y = 550;
        break;
    }
    
    case WM_DESTROY:
        UpdateSettingsFromControls();
        SaveSettings();
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;
    
    // Инициализация Common Controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icc);
    
    // Регистрация класса окна
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"YtDlpGUI";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wcex)) {
        MessageBoxW(NULL, L"Ошибка регистрации класса окна!", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Создание главного окна
    g_hMainWnd = CreateWindowExW(0, L"YtDlpGUI", L"yt-dlp GUI",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, 0, 820, 600,
                                NULL, NULL, hInstance, NULL);
    
    if (!g_hMainWnd) {
        MessageBoxW(NULL, L"Ошибка создания окна!", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);
    
    // Главный цикл сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hMainWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return (int)msg.wParam;
}
