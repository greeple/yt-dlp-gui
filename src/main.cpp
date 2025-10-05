// main.cpp
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <process.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

// ======================
// Глобальные переменные
// ======================

HWND g_hMainWnd = nullptr;
HWND g_hTab = nullptr;
HWND g_hProgress = nullptr;
HWND g_hLogEdit = nullptr;
HWND g_hShowLogBtn = nullptr;
bool g_logVisible = false;

// Элементы вкладок
HWND g_hUrlEdit = nullptr;
HWND g_hSavePathEdit = nullptr;
HWND g_hBrowseBtn = nullptr;
HWND g_hDownloadBtn = nullptr;

HWND g_hOutputTemplateEdit = nullptr;
HWND g_hMergeFormatCombo = nullptr;
HWND g_hListFormatsCheck = nullptr;
HWND g_hSkipDownloadCheck = nullptr;
HWND g_hWriteJsonCheck = nullptr;

HWND g_hEmbedSubsCheck = nullptr;
HWND g_hEmbedThumbCheck = nullptr;
HWND g_hEmbedMetaCheck = nullptr;
HWND g_hEmbedChaptersCheck = nullptr;
HWND g_hConvertSubsCombo = nullptr;
HWND g_hConvertThumbsCombo = nullptr;

HWND g_hProxyEdit = nullptr;
HWND g_hDownloaderCombo = nullptr;
HWND g_hDownloaderArgsEdit = nullptr;
HWND g_hCookiesBrowserCombo = nullptr;

HWND g_hIgnoreConfigCheck = nullptr;
HWND g_hConfigLocationsEdit = nullptr;
HWND g_hLiveFromStartCheck = nullptr;
HWND g_hBatchFileEdit = nullptr;
HWND g_hBatchBrowseBtn = nullptr;
HWND g_hNoCacheDirCheck = nullptr;
HWND g_hCacheDirEdit = nullptr;

struct Settings {
    std::wstring savePath;
    std::wstring url;
    bool ignoreConfig = false;
    std::wstring configLocations;
    bool liveFromStart = false;
    std::wstring proxy;
    std::wstring downloader = L"native";
    std::wstring downloaderArgs;
    std::wstring batchFile;
    std::wstring outputTemplate = L"%(title)s.%(ext)s";
    std::wstring mergeFormat = L"mp4";
    bool listFormats = false;
    bool skipDownload = false;
    bool writeJson = false;
    bool embedSubs = false;
    bool embedThumbnail = false;
    bool embedMetadata = false;
    bool embedChapters = false;
    std::wstring convertSubs = L"none";
    std::wstring convertThumbs = L"none";
    std::wstring cookiesFromBrowser;
    bool noCacheDir = false;
    std::wstring cacheDir;
} g_settings;

// ======================
// Вспомогательные функции
// ======================

std::wstring GetExePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring exePath(path);
    size_t pos = exePath.find_last_of(L"\\/");
    return exePath.substr(0, pos + 1);
}

std::wstring GetDesktopPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOP, nullptr, 0, path))) {
        return std::wstring(path);
    }
    return L"C:\\Users\\Public\\Desktop";
}

void SaveSettings() {
    std::wstring ini = GetExePath() + L"settings.ini";
    WritePrivateProfileStringW(L"General", L"SavePath", g_settings.savePath.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"General", L"IgnoreConfig", std::to_wstring(g_settings.ignoreConfig).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"General", L"ConfigLocations", g_settings.configLocations.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"General", L"LiveFromStart", std::to_wstring(g_settings.liveFromStart).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"General", L"Proxy", g_settings.proxy.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"General", L"Downloader", g_settings.downloader.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"General", L"DownloaderArgs", g_settings.downloaderArgs.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"General", L"BatchFile", g_settings.batchFile.c_str(), ini.c_str());

    WritePrivateProfileStringW(L"Output", L"OutputTemplate", g_settings.outputTemplate.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Output", L"MergeFormat", g_settings.mergeFormat.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Output", L"ListFormats", std::to_wstring(g_settings.listFormats).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Output", L"SkipDownload", std::to_wstring(g_settings.skipDownload).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Output", L"WriteJson", std::to_wstring(g_settings.writeJson).c_str(), ini.c_str());

    WritePrivateProfileStringW(L"Subs", L"EmbedSubs", std::to_wstring(g_settings.embedSubs).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Subs", L"EmbedThumbnail", std::to_wstring(g_settings.embedThumbnail).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Subs", L"EmbedMetadata", std::to_wstring(g_settings.embedMetadata).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Subs", L"EmbedChapters", std::to_wstring(g_settings.embedChapters).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Subs", L"ConvertSubs", g_settings.convertSubs.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Subs", L"ConvertThumbs", g_settings.convertThumbs.c_str(), ini.c_str());

    WritePrivateProfileStringW(L"Advanced", L"CookiesFromBrowser", g_settings.cookiesFromBrowser.c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Advanced", L"NoCacheDir", std::to_wstring(g_settings.noCacheDir).c_str(), ini.c_str());
    WritePrivateProfileStringW(L"Advanced", L"CacheDir", g_settings.cacheDir.c_str(), ini.c_str());
}

void LoadSettings() {
    std::wstring ini = GetExePath() + L"settings.ini";
    wchar_t buf[1024] = {0};

    GetPrivateProfileStringW(L"General", L"SavePath", GetDesktopPath().c_str(), buf, 1024, ini.c_str());
    g_settings.savePath = buf;

    g_settings.ignoreConfig = GetPrivateProfileIntW(L"General", L"IgnoreConfig", FALSE, ini.c_str()) != 0;
    GetPrivateProfileStringW(L"General", L"ConfigLocations", L"", buf, 1024, ini.c_str());
    g_settings.configLocations = buf;
    g_settings.liveFromStart = GetPrivateProfileIntW(L"General", L"LiveFromStart", FALSE, ini.c_str()) != 0;
    GetPrivateProfileStringW(L"General", L"Proxy", L"", buf, 1024, ini.c_str());
    g_settings.proxy = buf;
    GetPrivateProfileStringW(L"General", L"Downloader", L"native", buf, 1024, ini.c_str());
    g_settings.downloader = buf;
    GetPrivateProfileStringW(L"General", L"DownloaderArgs", L"", buf, 1024, ini.c_str());
    g_settings.downloaderArgs = buf;
    GetPrivateProfileStringW(L"General", L"BatchFile", L"", buf, 1024, ini.c_str());
    g_settings.batchFile = buf;

    GetPrivateProfileStringW(L"Output", L"OutputTemplate", L"%(title)s.%(ext)s", buf, 1024, ini.c_str());
    g_settings.outputTemplate = buf;
    GetPrivateProfileStringW(L"Output", L"MergeFormat", L"mp4", buf, 1024, ini.c_str());
    g_settings.mergeFormat = buf;
    g_settings.listFormats = GetPrivateProfileIntW(L"Output", L"ListFormats", FALSE, ini.c_str()) != 0;
    g_settings.skipDownload = GetPrivateProfileIntW(L"Output", L"SkipDownload", FALSE, ini.c_str()) != 0;
    g_settings.writeJson = GetPrivateProfileIntW(L"Output", L"WriteJson", FALSE, ini.c_str()) != 0;

    g_settings.embedSubs = GetPrivateProfileIntW(L"Subs", L"EmbedSubs", FALSE, ini.c_str()) != 0;
    g_settings.embedThumbnail = GetPrivateProfileIntW(L"Subs", L"EmbedThumbnail", FALSE, ini.c_str()) != 0;
    g_settings.embedMetadata = GetPrivateProfileIntW(L"Subs", L"EmbedMetadata", FALSE, ini.c_str()) != 0;
    g_settings.embedChapters = GetPrivateProfileIntW(L"Subs", L"EmbedChapters", FALSE, ini.c_str()) != 0;
    GetPrivateProfileStringW(L"Subs", L"ConvertSubs", L"none", buf, 1024, ini.c_str());
    g_settings.convertSubs = buf;
    GetPrivateProfileStringW(L"Subs", L"ConvertThumbs", L"none", buf, 1024, ini.c_str());
    g_settings.convertThumbs = buf;

    GetPrivateProfileStringW(L"Advanced", L"CookiesFromBrowser", L"", buf, 1024, ini.c_str());
    g_settings.cookiesFromBrowser = buf;
    g_settings.noCacheDir = GetPrivateProfileIntW(L"Advanced", L"NoCacheDir", FALSE, ini.c_str()) != 0;
    GetPrivateProfileStringW(L"Advanced", L"CacheDir", L"", buf, 1024, ini.c_str());
    g_settings.cacheDir = buf;
}

std::wstring AnsiToUtf16(const std::string& ansi) {
    if (ansi.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, ansi.data(), (int)ansi.size(), nullptr, 0);
    if (size <= 0) return L"";
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, ansi.data(), (int)ansi.size(), &wstr[0], size);
    return wstr;
}

void AppendLog(const std::wstring& text) {
    if (!g_hLogEdit) return;
    SendMessageW(g_hLogEdit, EM_SETSEL, -1, -1);
    SendMessageW(g_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(g_hLogEdit, EM_SCROLLCARET, 0, 0);
}

std::wstring BuildYtDlpCommand() {
    std::wstring cmd = L"yt-dlp.exe";

    if (g_settings.ignoreConfig) cmd += L" --ignore-config";
    if (!g_settings.configLocations.empty()) {
        cmd += L" --config-locations \"";
        cmd += g_settings.configLocations;
        cmd += L"\"";
    }
    if (g_settings.liveFromStart) cmd += L" --live-from-start";
    if (!g_settings.proxy.empty()) {
        cmd += L" --proxy \"";
        cmd += g_settings.proxy;
        cmd += L"\"";
    }
    if (!g_settings.downloader.empty()) {
        cmd += L" --downloader \"";
        cmd += g_settings.downloader;
        cmd += L"\"";
    }
    if (!g_settings.downloaderArgs.empty()) {
        cmd += L" --downloader-args \"";
        cmd += g_settings.downloaderArgs;
        cmd += L"\"";
    }
    if (!g_settings.batchFile.empty()) {
        cmd += L" -a \"";
        cmd += g_settings.batchFile;
        cmd += L"\"";
    }
    if (!g_settings.outputTemplate.empty()) {
        cmd += L" -o \"";
        cmd += g_settings.outputTemplate;
        cmd += L"\"";
    }
    if (!g_settings.mergeFormat.empty() && g_settings.mergeFormat != L"none") {
        cmd += L" --merge-output-format ";
        cmd += g_settings.mergeFormat;
    }
    if (g_settings.listFormats) cmd += L" -F";
    if (g_settings.skipDownload) cmd += L" --skip-download";
    if (g_settings.writeJson) cmd += L" -J";
    if (g_settings.embedSubs) cmd += L" --embed-subs";
    if (g_settings.embedThumbnail) cmd += L" --embed-thumbnail";
    if (g_settings.embedMetadata) cmd += L" --embed-metadata";
    if (g_settings.embedChapters) cmd += L" --embed-chapters";
    if (!g_settings.convertSubs.empty() && g_settings.convertSubs != L"none") {
        cmd += L" --convert-subs ";
        cmd += g_settings.convertSubs;
    }
    if (!g_settings.convertThumbs.empty() && g_settings.convertThumbs != L"none") {
        cmd += L" --convert-thumbnails ";
        cmd += g_settings.convertThumbs;
    }
    if (!g_settings.cookiesFromBrowser.empty()) {
        cmd += L" --cookies-from-browser ";
        cmd += g_settings.cookiesFromBrowser;
    }
    if (g_settings.noCacheDir) {
        cmd += L" --no-cache-dir";
    } else if (!g_settings.cacheDir.empty()) {
        cmd += L" --cache-dir \"";
        cmd += g_settings.cacheDir;
        cmd += L"\"";
    }

    cmd += L" --paths \"";
    cmd += g_settings.savePath;
    cmd += L"\"";

    if (g_settings.batchFile.empty()) {
        wchar_t urlBuf[2048] = {0};
        GetWindowTextW(g_hUrlEdit, urlBuf, 2048);
        if (wcslen(urlBuf) > 0) {
            cmd += L" \"";
            cmd += urlBuf;
            cmd += L"\"";
        }
    }

    return cmd;
}

DWORD WINAPI RunYtDlpThread(LPVOID) {
    std::wstring cmd = BuildYtDlpCommand();
    AppendLog(L"[CMD] " + cmd + L"\r\n");

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        PostMessageW(g_hMainWnd, WM_USER + 2, 0, 0);
        return 1;
    }

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi = {0};
    wchar_t* cmdMutable = &cmd[0];
    BOOL created = CreateProcessW(nullptr, cmdMutable, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);

    if (created) {
        char buffer[2048];
        DWORD bytesRead;
        while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string line(buffer);
            if (!line.empty() && line.back() == '\n') line.pop_back();
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) {
                std::wstring wline = AnsiToUtf16(line) + L"\r\n";
                PostMessageW(g_hMainWnd, WM_USER + 1, 0, (LPARAM)new std::wstring(wline));
            }
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        std::wstring err = L"Не удалось запустить yt-dlp.exe. Убедитесь, что он находится в той же папке.\r\n";
        PostMessageW(g_hMainWnd, WM_USER + 1, 0, (LPARAM)new std::wstring(err));
    }

    CloseHandle(hRead);
    PostMessageW(g_hMainWnd, WM_USER + 2, 0, 0);
    return 0;
}

// ======================
// Создание вкладок
// ======================

void CreateTab0Controls(HWND parent) {
    int y = 20;
    CreateWindowW(L"STATIC", L"URL видео или плейлиста:", WS_CHILD | WS_VISIBLE, 10, y, 250, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hUrlEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                               10, y, 500, 24, parent, nullptr, nullptr, nullptr);
    y += 35;

    CreateWindowW(L"STATIC", L"Папка сохранения:", WS_CHILD | WS_VISIBLE, 10, y, 150, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hSavePathEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                    10, y, 400, 24, parent, nullptr, nullptr, nullptr);
    g_hBrowseBtn = CreateWindowW(L"BUTTON", L"Обзор...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                 420, y, 90, 24, parent, (HMENU)101, nullptr, nullptr);
    y += 35;

    g_hDownloadBtn = CreateWindowW(L"BUTTON", L"Скачать", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                   10, y, 120, 30, parent, (HMENU)200, nullptr, nullptr);
}

void CreateTab1Controls(HWND parent) {
    int y = 20;
    CreateWindowW(L"STATIC", L"Шаблон имени файла:", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hOutputTemplateEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                          10, y, 500, 24, parent, nullptr, nullptr, nullptr);
    y += 35;

    CreateWindowW(L"STATIC", L"Формат при объединении:", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hMergeFormatCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                        10, y, 150, 200, parent, nullptr, nullptr, nullptr);
    SendMessageW(g_hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mp4");
    SendMessageW(g_hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mkv");
    SendMessageW(g_hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"webm");
    SendMessageW(g_hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"avi");
    SendMessageW(g_hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mov");
    SendMessageW(g_hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"flv");
    y += 35;

    g_hListFormatsCheck = CreateWindowW(L"BUTTON", L"Показать доступные форматы (--list-formats)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                        10, y, 400, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hSkipDownloadCheck = CreateWindowW(L"BUTTON", L"Только анализировать (--skip-download)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                         10, y, 400, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hWriteJsonCheck = CreateWindowW(L"BUTTON", L"Сохранить метаданные в JSON (-J)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      10, y, 400, 20, parent, nullptr, nullptr, nullptr);
}

void CreateTab2Controls(HWND parent) {
    int y = 20;
    g_hEmbedSubsCheck = CreateWindowW(L"BUTTON", L"Встроить субтитры (--embed-subs)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      10, y, 350, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hEmbedThumbCheck = CreateWindowW(L"BUTTON", L"Встроить превью (--embed-thumbnail)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                       10, y, 350, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hEmbedMetaCheck = CreateWindowW(L"BUTTON", L"Встроить метаданные (--embed-metadata)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      10, y, 350, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hEmbedChaptersCheck = CreateWindowW(L"BUTTON", L"Встроить главы (--embed-chapters)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                          10, y, 350, 20, parent, nullptr, nullptr, nullptr);
    y += 35;

    CreateWindowW(L"STATIC", L"Конвертировать субтитры:", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hConvertSubsCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                        10, y, 120, 200, parent, nullptr, nullptr, nullptr);
    SendMessageW(g_hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"none");
    SendMessageW(g_hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"srt");
    SendMessageW(g_hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"vtt");
    SendMessageW(g_hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"ass");
    SendMessageW(g_hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"lrc");
    y += 35;

    CreateWindowW(L"STATIC", L"Конвертировать превью:", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hConvertThumbsCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                          10, y, 120, 200, parent, nullptr, nullptr, nullptr);
    SendMessageW(g_hConvertThumbsCombo, CB_ADDSTRING, 0, (LPARAM)L"none");
    SendMessageW(g_hConvertThumbsCombo, CB_ADDSTRING, 0, (LPARAM)L"jpg");
    SendMessageW(g_hConvertThumbsCombo, CB_ADDSTRING, 0, (LPARAM)L"png");
    SendMessageW(g_hConvertThumbsCombo, CB_ADDSTRING, 0, (LPARAM)L"webp");
}

void CreateTab3Controls(HWND parent) {
    int y = 20;
    CreateWindowW(L"STATIC", L"Прокси (например, socks5://127.0.0.1:9050):", WS_CHILD | WS_VISIBLE, 10, y, 350, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hProxyEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                 10, y, 400, 24, parent, nullptr, nullptr, nullptr);
    y += 35;

    CreateWindowW(L"STATIC", L"Внешний загрузчик:", WS_CHILD | WS_VISIBLE, 10, y, 150, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hDownloaderCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                       10, y, 150, 200, parent, nullptr, nullptr, nullptr);
    SendMessageW(g_hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"native");
    SendMessageW(g_hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"aria2c");
    SendMessageW(g_hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"ffmpeg");
    SendMessageW(g_hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"wget");
    SendMessageW(g_hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"curl");
    y += 35;

    CreateWindowW(L"STATIC", L"Аргументы загрузчика:", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hDownloaderArgsEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                          10, y, 400, 24, parent, nullptr, nullptr, nullptr);
    y += 35;

    CreateWindowW(L"STATIC", L"Загрузить cookies из браузера:", WS_CHILD | WS_VISIBLE, 10, y, 250, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hCookiesBrowserCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                           10, y, 150, 200, parent, nullptr, nullptr, nullptr);
    SendMessageW(g_hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"");
    SendMessageW(g_hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"chrome");
    SendMessageW(g_hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"firefox");
    SendMessageW(g_hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"brave");
    SendMessageW(g_hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"edge");
    SendMessageW(g_hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"opera");
    SendMessageW(g_hCookiesBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"vivaldi");
}

void CreateTab4Controls(HWND parent) {
    int y = 20;
    g_hIgnoreConfigCheck = CreateWindowW(L"BUTTON", L"Игнорировать конфиг (--ignore-config)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                         10, y, 350, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    CreateWindowW(L"STATIC", L"Путь к конфигу (--config-locations):", WS_CHILD | WS_VISIBLE, 10, y, 250, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hConfigLocationsEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                           10, y, 400, 24, parent, nullptr, nullptr, nullptr);
    y += 35;

    g_hLiveFromStartCheck = CreateWindowW(L"BUTTON", L"Скачать стрим с начала (--live-from-start)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                          10, y, 350, 20, parent, nullptr, nullptr, nullptr);
    y += 35;

    CreateWindowW(L"STATIC", L"Файл с URL (batch):", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hBatchFileEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                     10, y, 400, 24, parent, nullptr, nullptr, nullptr);
    g_hBatchBrowseBtn = CreateWindowW(L"BUTTON", L"Обзор...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      420, y, 90, 24, parent, (HMENU)102, nullptr, nullptr);
    y += 35;

    g_hNoCacheDirCheck = CreateWindowW(L"BUTTON", L"Отключить кэш (--no-cache-dir)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                       10, y, 250, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    CreateWindowW(L"STATIC", L"Папка кэша (--cache-dir):", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, parent, nullptr, nullptr, nullptr);
    y += 25;
    g_hCacheDirEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                    10, y, 400, 24, parent, nullptr, nullptr, nullptr);
}

// ======================
// WndProc
// ======================

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hTabPages[5] = {nullptr};

    switch (msg) {
        case WM_CREATE: {
            INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_TAB_CLASSES | ICC_PROGRESS_CLASS };
            InitCommonControlsEx(&icc);

            g_hTab = CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE,
                                   10, 10, 780, 420, hWnd, nullptr, nullptr, nullptr);

            TCITEMW tci = {0};
            tci.mask = TCIF_TEXT;
            const wchar_t* tabNames[] = {L"Основное", L"Форматы", L"Субтитры", L"Прокси/Загрузчики", L"Продвинутое"};
            for (int i = 0; i < 5; ++i) {
                tci.pszText = (LPWSTR)tabNames[i];
                TabCtrl_InsertItemW(g_hTab, i, &tci);
                hTabPages[i] = CreateWindowW(L"STATIC", L"", WS_CHILD, 0, 0, 770, 390, g_hTab, nullptr, nullptr, nullptr);
            }

            CreateTab0Controls(hTabPages[0]);
            CreateTab1Controls(hTabPages[1]);
            CreateTab2Controls(hTabPages[2]);
            CreateTab3Controls(hTabPages[3]);
            CreateTab4Controls(hTabPages[4]);

            g_hProgress = CreateWindowW(PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE,
                                        10, 440, 780, 20, hWnd, nullptr, nullptr, nullptr);
            g_hShowLogBtn = CreateWindowW(L"BUTTON", L"Показать лог", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          10, 470, 120, 25, hWnd, (HMENU)100, nullptr, nullptr);
            g_hLogEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_BORDER,
                                       10, 500, 780, 150, hWnd, nullptr, nullptr, nullptr);
            ShowWindow(g_hLogEdit, SW_HIDE);

            LoadSettings();
            SetWindowTextW(g_hSavePathEdit, g_settings.savePath.c_str());

            SetWindowTextW(g_hOutputTemplateEdit, g_settings.outputTemplate.c_str());
            SendMessageW(g_hMergeFormatCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.mergeFormat.c_str());
            CheckDlgButton(hWnd, 1000, BST_UNCHECKED); // dummy

            CheckDlgButton(g_hListFormatsCheck, g_settings.listFormats ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(g_hSkipDownloadCheck, g_settings.skipDownload ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(g_hWriteJsonCheck, g_settings.writeJson ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(g_hEmbedSubsCheck, g_settings.embedSubs ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(g_hEmbedThumbCheck, g_settings.embedThumbnail ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(g_hEmbedMetaCheck, g_settings.embedMetadata ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(g_hEmbedChaptersCheck, g_settings.embedChapters ? BST_CHECKED : BST_UNCHECKED);
            SendMessageW(g_hConvertSubsCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.convertSubs.c_str());
            SendMessageW(g_hConvertThumbsCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.convertThumbs.c_str());

            SetWindowTextW(g_hProxyEdit, g_settings.proxy.c_str());
            SendMessageW(g_hDownloaderCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.downloader.c_str());
            SetWindowTextW(g_hDownloaderArgsEdit, g_settings.downloaderArgs.c_str());
            SendMessageW(g_hCookiesBrowserCombo, CB_SELECTSTRING, -1, (LPARAM)g_settings.cookiesFromBrowser.c_str());

            CheckDlgButton(g_hIgnoreConfigCheck, g_settings.ignoreConfig ? BST_CHECKED : BST_UNCHECKED);
            SetWindowTextW(g_hConfigLocationsEdit, g_settings.configLocations.c_str());
            CheckDlgButton(g_hLiveFromStartCheck, g_settings.liveFromStart ? BST_CHECKED : BST_UNCHECKED);
            SetWindowTextW(g_hBatchFileEdit, g_settings.batchFile.c_str());
            CheckDlgButton(g_hNoCacheDirCheck, g_settings.noCacheDir ? BST_CHECKED : BST_UNCHECKED);
            SetWindowTextW(g_hCacheDirEdit, g_settings.cacheDir.c_str());

            ShowWindow(hTabPages[0], SW_SHOW);
            break;
        }
        case WM_NOTIFY: {
            if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
                int sel = TabCtrl_GetCurSel(g_hTab);
                for (int i = 0; i < 5; ++i) {
                    ShowWindow(hTabPages[i], i == sel ? SW_SHOW : SW_HIDE);
                }
            }
            break;
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            if (id == 100) {
                g_logVisible = !g_logVisible;
                ShowWindow(g_hLogEdit, g_logVisible ? SW_SHOW : SW_HIDE);
                SetWindowTextW(g_hShowLogBtn, g_logVisible ? L"Скрыть лог" : L"Показать лог");
                SendMessageW(hWnd, WM_SIZE, 0, 0);
            }
            else if (id == 101) {
                BROWSEINFOW bi = {0};
                bi.hwndOwner = hWnd;
                bi.pszDisplayName = nullptr;
                bi.lpszTitle = L"Выберите папку для сохранения";
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
                if (pidl) {
                    wchar_t path[MAX_PATH];
                    if (SHGetPathFromIDListW(pidl, path)) {
                        g_settings.savePath = path;
                        SetWindowTextW(g_hSavePathEdit, path);
                    }
                    CoTaskMemFree(pidl);
                }
            }
            else if (id == 102) {
                OPENFILENAMEW ofn = {0};
                wchar_t file[MAX_PATH] = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFile = file;
                ofn.nMaxFile = MAX_PATH;
                ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) {
                    g_settings.batchFile = file;
                    SetWindowTextW(g_hBatchFileEdit, file);
                }
            }
            else if (id == 200) {
                wchar_t buf[2048];
                GetWindowTextW(g_hSavePathEdit, buf, 2048);
                g_settings.savePath = buf;
                GetWindowTextW(g_hOutputTemplateEdit, buf, 2048);
                g_settings.outputTemplate = buf;
                g_settings.listFormats = IsDlgButtonChecked(hWnd, IDCANCEL) == BST_CHECKED; // fix below

                // FIX: используем hWnd как родительский диалог
                g_settings.listFormats = IsDlgButtonChecked(hWnd, (int)g_hListFormatsCheck) == BST_CHECKED;
                g_settings.skipDownload = IsDlgButtonChecked(hWnd, (int)g_hSkipDownloadCheck) == BST_CHECKED;
                g_settings.writeJson = IsDlgButtonChecked(hWnd, (int)g_hWriteJsonCheck) == BST_CHECKED;
                g_settings.embedSubs = IsDlgButtonChecked(hWnd, (int)g_hEmbedSubsCheck) == BST_CHECKED;
                g_settings.embedThumbnail = IsDlgButtonChecked(hWnd, (int)g_hEmbedThumbCheck) == BST_CHECKED;
                g_settings.embedMetadata = IsDlgButtonChecked(hWnd, (int)g_hEmbedMetaCheck) == BST_CHECKED;
                g_settings.embedChapters = IsDlgButtonChecked(hWnd, (int)g_hEmbedChaptersCheck) == BST_CHECKED;
                g_settings.ignoreConfig = IsDlgButtonChecked(hWnd, (int)g_hIgnoreConfigCheck) == BST_CHECKED;
                g_settings.liveFromStart = IsDlgButtonChecked(hWnd, (int)g_hLiveFromStartCheck) == BST_CHECKED;
                g_settings.noCacheDir = IsDlgButtonChecked(hWnd, (int)g_hNoCacheDirCheck) == BST_CHECKED;

                GetWindowTextW(g_hProxyEdit, buf, 2048);
                g_settings.proxy = buf;
                GetWindowTextW(g_hDownloaderArgsEdit, buf, 2048);
                g_settings.downloaderArgs = buf;
                GetWindowTextW(g_hConfigLocationsEdit, buf, 2048);
                g_settings.configLocations = buf;
                GetWindowTextW(g_hBatchFileEdit, buf, 2048);
                g_settings.batchFile = buf;
                GetWindowTextW(g_hCacheDirEdit, buf, 2048);
                g_settings.cacheDir = buf;

                LRESULT idx = SendMessageW(g_hMergeFormatCombo, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR) {
                    SendMessageW(g_hMergeFormatCombo, CB_GETLBTEXT, idx, (LPARAM)buf);
                    g_settings.mergeFormat = buf;
                }
                idx = SendMessageW(g_hConvertSubsCombo, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR) {
                    SendMessageW(g_hConvertSubsCombo, CB_GETLBTEXT, idx, (LPARAM)buf);
                    g_settings.convertSubs = buf;
                }
                idx = SendMessageW(g_hConvertThumbsCombo, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR) {
                    SendMessageW(g_hConvertThumbsCombo, CB_GETLBTEXT, idx, (LPARAM)buf);
                    g_settings.convertThumbs = buf;
                }
                idx = SendMessageW(g_hDownloaderCombo, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR) {
                    SendMessageW(g_hDownloaderCombo, CB_GETLBTEXT, idx, (LPARAM)buf);
                    g_settings.downloader = buf;
                }
                idx = SendMessageW(g_hCookiesBrowserCombo, CB_GETCURSEL, 0, 0);
                if (idx != CB_ERR) {
                    SendMessageW(g_hCookiesBrowserCombo, CB_GETLBTEXT, idx, (LPARAM)buf);
                    g_settings.cookiesFromBrowser = buf;
                }

                SaveSettings();
                AppendLog(L"\r\n=== Начало загрузки ===\r\n");
                EnableWindow(g_hDownloadBtn, FALSE);
                SendMessageW(g_hProgress, PBM_SETPOS, 0, 0);
                CreateThread(nullptr, 0, RunYtDlpThread, nullptr, 0, nullptr);
            }
            break;
        }
        case WM_USER + 1: {
            std::wstring* text = (std::wstring*)lParam;
            AppendLog(*text);
            delete text;
            break;
        }
        case WM_USER + 2: {
            EnableWindow(g_hDownloadBtn, TRUE);
            AppendLog(L"\r\n=== Завершено ===\r\n");
            break;
        }
        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            if (g_hTab) SetWindowPos(g_hTab, nullptr, 10, 10, w - 20, std::min(420, h - 200), SWP_NOZORDER);
            if (g_hProgress) SetWindowPos(g_hProgress, nullptr, 10, h - 160, w - 20, 20, SWP_NOZORDER);
            if (g_hShowLogBtn) SetWindowPos(g_hShowLogBtn, nullptr, 10, h - 130, 120, 25, SWP_NOZORDER);
            if (g_hLogEdit) SetWindowPos(g_hLogEdit, nullptr, 10, h - 100, w - 20, g_logVisible ? 100 : 0, SWP_NOZORDER);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"YtDlpGuiClass";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    g_hMainWnd = CreateWindowW(L"YtDlpGuiClass", L"yt-dlp GUI — скачать видео с YouTube и других сайтов",
                               WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
                               CW_USEDEFAULT, CW_USEDEFAULT, 800, 700, nullptr, nullptr, hInst, nullptr);
    ShowWindow(g_hMainWnd, SW_SHOW);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
