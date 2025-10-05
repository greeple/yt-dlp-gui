// yt-dlp GUI (Win32, Unicode)
// Прогресс-бар, вкладки, лог (спойлер), сохранение INI, опции yt-dlp
// Сборка: CMake + MSVC. Требует comctl32 v6 (манифест включён).

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <atomic>
#include <memory>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Ole32.lib")

// -------------------- IDs --------------------
enum {
    ID_EDIT_URL = 1001,
    ID_BTN_DOWNLOAD,
    ID_BTN_LIST_FORMATS,
    ID_BTN_JSON,
    ID_EDIT_SAVEFOLDER,
    ID_BTN_BROWSE_SAVEFOLDER,
    ID_PROGRESS,
    ID_LBL_PROGRESS,
    ID_BTN_TOGGLE_LOG,
    ID_EDIT_LOG,
    ID_TABS,

    // Tab: Основное
    ID_CHK_IGNORE_CONFIG = 2001,
    ID_EDIT_CONFIG_LOCATIONS,
    ID_CHK_LIVE_FROM_START,
    ID_EDIT_OUTPUT_TEMPLATE,
    ID_CHK_SKIP_DOWNLOAD,
    ID_CHK_CHECK_FORMATS,
    ID_COMBO_MERGE_FMT,

    // Tab: Сеть/Загрузчик
    ID_EDIT_PROXY,
    ID_COMBO_DOWNLOADER,
    ID_EDIT_DOWNLOADER_PROTOCOLS,
    ID_BTN_ADD_DL_MAP,
    ID_LIST_DL_MAP,
    ID_BTN_REMOVE_DL_MAP,
    ID_EDIT_DOWNLOADER_ARGS,

    // Tab: Файлы/Куки/Кэш
    ID_EDIT_BATCH_FILE,
    ID_BTN_BROWSE_BATCH,
    ID_EDIT_COOKIES_FILE,
    ID_BTN_BROWSE_COOKIES,
    ID_EDIT_COOKIES_FROM_BROWSER,
    ID_EDIT_CACHE_DIR,
    ID_BTN_BROWSE_CACHE,
    ID_CHK_NO_CACHE_DIR,
    ID_CHK_RM_CACHE_DIR,

    // Tab: Эмбед/Субтитры
    ID_CHK_EMBED_SUBS,
    ID_CHK_EMBED_THUMB,
    ID_CHK_EMBED_META,
    ID_CHK_EMBED_CHAPS,
    ID_COMBO_CONVERT_SUBS,
    ID_COMBO_CONVERT_THUMBS,

    // Tab: Главы/Монтаж
    ID_CHK_SPLIT_CHAPTERS,
    ID_EDIT_REMOVE_CHAPTERS,
    ID_CHK_FORCE_KF_CUTS,

    // Tab: Дополнительно
    ID_COMBO_LOG_ENCODING,
    ID_EDIT_CONFIG_YTDLP_PATH
};

// Custom messages
#define WM_APP_LOG_APPEND (WM_APP+1)
#define WM_APP_PROGRESS   (WM_APP+2)
#define WM_APP_DONE       (WM_APP+3)
#define WM_APP_STATUS     (WM_APP+4)

// -------------------- Globals --------------------
HINSTANCE g_hInst = nullptr;
HWND g_hWnd = nullptr;
HWND hEditUrl, hBtnDownload, hBtnListFormats, hBtnJson;
HWND hEditSaveFolder, hBtnBrowseSaveFolder;
HWND hProgress, hLblProgress;
HWND hBtnToggleLog, hEditLog;
HWND hTabs;

HWND hPageMain = nullptr, hPageNet = nullptr, hPageFiles = nullptr, hPageEmbed = nullptr, hPageChapters = nullptr, hPageAdv = nullptr;

std::atomic<bool> g_LogVisible{true};
std::atomic<bool> g_Running{false};
std::atomic<bool> g_JsonMode{false};
std::atomic<bool> g_ListFormatsMode{false};
std::vector<BYTE> g_LastRawStdout; // для JSON сохранения
std::mutex g_LogMutex;

// INI
std::wstring g_IniPath;

// -------------------- Helpers --------------------
std::wstring GetExeDir() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    PathRemoveFileSpecW(buf);
    return buf;
}
std::wstring GetDesktopPath() {
    PWSTR p = nullptr;
    std::wstring res;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &p))) {
        res = p;
        CoTaskMemFree(p);
    } else {
        res = GetExeDir();
    }
    return res;
}
void IniWrite(const wchar_t* key, const std::wstring& val) {
    WritePrivateProfileStringW(L"settings", key, val.c_str(), g_IniPath.c_str());
}
std::wstring IniRead(const wchar_t* key, const wchar_t* def = L"") {
    wchar_t buf[4096];
    GetPrivateProfileStringW(L"settings", key, def, buf, 4096, g_IniPath.c_str());
    return buf;
}
void SetText(HWND h, const std::wstring& s) { SetWindowTextW(h, s.c_str()); }
std::wstring GetText(HWND h) {
    int len = GetWindowTextLengthW(h);
    std::wstring s(len, L'\0');
    GetWindowTextW(h, s.data(), len+1);
    return s;
}
void AppendLog(const std::wstring& s) {
    if (!hEditLog) return;
    int len = GetWindowTextLengthW(hEditLog);
    SendMessageW(hEditLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hEditLog, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
}
void SetProgressText(double percent) {
    wchar_t buf[64];
    if (percent >= 0.0) {
        swprintf(buf, 64, L"%.1f%%", percent);
    } else {
        wcscpy_s(buf, L"...");
    }
    SetText(hLblProgress, buf);
}
void EnableMarquee(bool on) {
    SendMessageW(hProgress, PBM_SETMARQUEE, on, 30);
}

// Quote argument for command line
std::wstring QuoteArg(const std::wstring& a) {
    if (a.empty()) return L"\"\"";
    if (a.find_first_of(L" \t\"") == std::wstring::npos) return a;
    std::wstring r = L"\"";
    for (auto ch : a) {
        if (ch == L'"') r += L"\\\"";
        else r += ch;
    }
    r += L"\"";
    return r;
}

bool IsValidUTF8(const std::vector<BYTE>& data) {
    const unsigned char* s = data.data();
    size_t i = 0, n = data.size();
    while (i < n) {
        if (s[i] < 0x80) { i++; continue; }
        if ((s[i] & 0xE0) == 0xC0) {
            if (i+1 >= n || (s[i+1] & 0xC0) != 0x80) return false;
            i += 2; continue;
        } else if ((s[i] & 0xF0) == 0xE0) {
            if (i+2 >= n || (s[i+1] & 0xC0) != 0x80 || (s[i+2] & 0xC0) != 0x80) return false;
            i += 3; continue;
        } else if ((s[i] & 0xF8) == 0xF0) {
            if (i+3 >= n || (s[i+1] & 0xC0) != 0x80 || (s[i+2] & 0xC0) != 0x80 || (s[i+3] & 0xC0) != 0x80) return false;
            i += 4; continue;
        }
        return false;
    }
    return true;
}

std::wstring DecodeBytes(const std::vector<BYTE>& buf, int forcedCp /*0=auto, 65001, 1251, 866*/) {
    if (buf.empty()) return L"";
    if (forcedCp == 65001 || forcedCp == 1251 || forcedCp == 866) {
        int len = MultiByteToWideChar(forcedCp, 0, (LPCSTR)buf.data(), (int)buf.size(), NULL, 0);
        std::wstring w(len, L'\0');
        MultiByteToWideChar(forcedCp, 0, (LPCSTR)buf.data(), (int)buf.size(), w.data(), len);
        return w;
    } else {
        if (IsValidUTF8(buf)) {
            int len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf.data(), (int)buf.size(), NULL, 0);
            std::wstring w(len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf.data(), (int)buf.size(), w.data(), len);
            return w;
        }
        for (int cp : {1251, 866}) {
            int len = MultiByteToWideChar(cp, 0, (LPCSTR)buf.data(), (int)buf.size(), NULL, 0);
            if (len > 0) {
                std::wstring w(len, L'\0');
                MultiByteToWideChar(cp, 0, (LPCSTR)buf.data(), (int)buf.size(), w.data(), len);
                return w;
            }
        }
        int len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)buf.data(), (int)buf.size(), NULL, 0);
        std::wstring w(len, L'\0');
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)buf.data(), (int)buf.size(), w.data(), len);
        return w;
    }
}

int GetLogEncodingCP() {
    int idx = (int)SendMessageW(GetDlgItem(hPageAdv, ID_COMBO_LOG_ENCODING), CB_GETCURSEL, 0, 0);
    switch (idx) {
        case 1: return 65001; // UTF-8
        case 2: return 1251;  // CP1251
        case 3: return 866;   // CP866
        default: return 0;    // Auto
    }
}

void AddComboItems(HWND h, const std::vector<std::wstring>& items, int sel = 0) {
    for (auto& s : items) SendMessageW(h, CB_ADDSTRING, 0, (LPARAM)s.c_str());
    SendMessageW(h, CB_SETCURSEL, sel, 0);
}

// -------------------- Layout --------------------
RECT g_Client{};
int g_LogHeight = 180;
int g_MinWidth = 800;
int g_MinHeight = 600;

void DoLayout() {
    RECT rc; GetClientRect(g_hWnd, &rc);
    g_Client = rc;
    int w = rc.right - rc.left;
    int y = 8, x = 8;

    // Row 1: URL + Buttons
    MoveWindow(GetDlgItem(g_hWnd, ID_EDIT_URL), x+60, y, w - 60 - 8 - 320, 24, TRUE);
    MoveWindow(GetDlgItem(g_hWnd, ID_BTN_DOWNLOAD), w - 316, y, 100, 24, TRUE);
    MoveWindow(GetDlgItem(g_hWnd, ID_BTN_LIST_FORMATS), w - 210, y, 100, 24, TRUE);
    MoveWindow(GetDlgItem(g_hWnd, ID_BTN_JSON), w - 104, y, 96, 24, TRUE);

    y += 32;
    // Row 2: Save folder + Browse
    MoveWindow(GetDlgItem(g_hWnd, ID_EDIT_SAVEFOLDER), x+120, y, w - 120 - 8 - 100, 24, TRUE);
    MoveWindow(GetDlgItem(g_hWnd, ID_BTN_BROWSE_SAVEFOLDER), w - 100, y, 92, 24, TRUE);

    y += 32;

    // Progress row
    MoveWindow(hProgress, x, y, w - 130, 20, TRUE);
    MoveWindow(hLblProgress, w - 120, y-2, 112, 24, TRUE);
    y += 28;

    // Toggle log
    MoveWindow(hBtnToggleLog, x, y, 120, 24, TRUE);
    y += 28;

    int tabsTop = y;
    int tabsHeight = rc.bottom - y - (g_LogVisible ? (g_LogHeight + 8) : 8);
    // Tabs
    MoveWindow(hTabs, x, tabsTop, w - 16, tabsHeight, TRUE);

    // Pages inside tabs
    RECT rtc; GetClientRect(hTabs, &rtc);
    TabCtrl_AdjustRect(hTabs, FALSE, &rtc);
    int pw = rtc.right - rtc.left, ph = rtc.bottom - rtc.top;
    MoveWindow(hPageMain, rtc.left, rtc.top, pw, ph, TRUE);
    MoveWindow(hPageNet, rtc.left, rtc.top, pw, ph, TRUE);
    MoveWindow(hPageFiles, rtc.left, rtc.top, pw, ph, TRUE);
    MoveWindow(hPageEmbed, rtc.left, rtc.top, pw, ph, TRUE);
    MoveWindow(hPageChapters, rtc.left, rtc.top, pw, ph, TRUE);
    MoveWindow(hPageAdv, rtc.left, rtc.top, pw, ph, TRUE);

    // Log area
    if (g_LogVisible) {
        MoveWindow(hEditLog, x, rc.bottom - g_LogHeight - 8, w - 16, g_LogHeight, TRUE);
        ShowWindow(hEditLog, SW_SHOW);
        SetWindowTextW(hBtnToggleLog, L"Скрыть лог");
    } else {
        ShowWindow(hEditLog, SW_HIDE);
        SetWindowTextW(hBtnToggleLog, L"Показать лог");
    }
}

// -------------------- UI creation --------------------
HWND CreateLabeledEdit(HWND parent, int idEdit, const wchar_t* label, int x, int y, int w, int h, int labelWidth=120) {
    CreateWindowW(L"STATIC", label, WS_CHILD|WS_VISIBLE, x, y+4, labelWidth, 20, parent, (HMENU)nullptr, g_hInst, 0);
    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, x+labelWidth+4, y, w-labelWidth-4, h, parent, (HMENU)(INT_PTR)idEdit, g_hInst, 0);
}
HWND CreateCheck(HWND parent, int id, const wchar_t* text, int x, int y) {
    return CreateWindowW(L"BUTTON", text, WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, x, y, 300, 22, parent, (HMENU)(INT_PTR)id, g_hInst, 0);
}
HWND CreateCombo(HWND parent, int id, int x, int y, int w, int h) {
    return CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, x, y, w, h, parent, (HMENU)(INT_PTR)id, g_hInst, 0);
}
HWND CreateButton(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h) {
    return CreateWindowW(L"BUTTON", text, WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, x, y, w, h, parent, (HMENU)(INT_PTR)id, g_hInst, 0);
}
HWND CreateMultilineEdit(HWND parent, int id, int x, int y, int w, int h) {
    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL|WS_VSCROLL, x, y, w, h, parent, (HMENU)(INT_PTR)id, g_hInst, 0);
}

void CreateTopControls(HWND hWnd) {
    // URL
    hEditUrl = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, 8+60, 8, 600, 24, hWnd, (HMENU)(INT_PTR)ID_EDIT_URL, g_hInst, 0);
    CreateWindowW(L"STATIC", L"URL:", WS_CHILD|WS_VISIBLE, 8, 12, 50, 20, hWnd, (HMENU)nullptr, g_hInst, 0);

    hBtnDownload = CreateButton(hWnd, ID_BTN_DOWNLOAD, L"Скачать", 0,0,0,0);
    hBtnListFormats = CreateButton(hWnd, ID_BTN_LIST_FORMATS, L"Форматы (-F)", 0,0,0,0);
    hBtnJson = CreateButton(hWnd, ID_BTN_JSON, L"JSON (-J)", 0,0,0,0);

    // Save folder
    hEditSaveFolder = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, 8+120, 8+32, 600, 24, hWnd, (HMENU)(INT_PTR)ID_EDIT_SAVEFOLDER, g_hInst, 0);
    CreateWindowW(L"STATIC", L"Папка:", WS_CHILD|WS_VISIBLE, 8, 8+32+4, 110, 20, hWnd, (HMENU)nullptr, g_hInst, 0);
    hBtnBrowseSaveFolder = CreateButton(hWnd, ID_BTN_BROWSE_SAVEFOLDER, L"Обзор...", 0,0,0,0);

    // Progress
    hProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL, WS_CHILD|WS_VISIBLE, 8, 8+64, 400, 20, hWnd, (HMENU)(INT_PTR)ID_PROGRESS, g_hInst, 0);
    SendMessageW(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    hLblProgress = CreateWindowW(L"STATIC", L"0%", WS_CHILD|WS_VISIBLE|SS_RIGHT, 0,0,60,20, hWnd, (HMENU)(INT_PTR)ID_LBL_PROGRESS, g_hInst, 0);

    // Toggle log
    hBtnToggleLog = CreateButton(hWnd, ID_BTN_TOGGLE_LOG, L"Скрыть лог", 8, 8+64+28, 120, 24);

    // Tabs
    hTabs = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
                            8, 8+64+28+28, 600, 300, hWnd, (HMENU)(INT_PTR)ID_TABS, g_hInst, 0);
    TCITEMW ti = {0}; ti.mask = TCIF_TEXT;
    ti.pszText = (LPWSTR)L"Основное"; TabCtrl_InsertItem(hTabs, 0, &ti);
    ti.pszText = (LPWSTR)L"Сеть/Загрузчик"; TabCtrl_InsertItem(hTabs, 1, &ti);
    ti.pszText = (LPWSTR)L"Файлы/Куки/Кэш"; TabCtrl_InsertItem(hTabs, 2, &ti);
    ti.pszText = (LPWSTR)L"Эмбед/Субтитры"; TabCtrl_InsertItem(hTabs, 3, &ti);
    ti.pszText = (LPWSTR)L"Главы/Монтаж"; TabCtrl_InsertItem(hTabs, 4, &ti);
    ti.pszText = (LPWSTR)L"Дополнительно"; TabCtrl_InsertItem(hTabs, 5, &ti);

    // Pages
    hPageMain = CreateWindowExW(0, L"STATIC", L"", WS_CHILD|WS_VISIBLE, 0,0,0,0, hTabs, (HMENU)nullptr, g_hInst, 0);
    hPageNet = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0,0,0,0, hTabs, (HMENU)nullptr, g_hInst, 0);
    hPageFiles = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0,0,0,0, hTabs, (HMENU)nullptr, g_hInst, 0);
    hPageEmbed = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0,0,0,0, hTabs, (HMENU)nullptr, g_hInst, 0);
    hPageChapters = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0,0,0,0, hTabs, (HMENU)nullptr, g_hInst, 0);
    hPageAdv = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0,0,0,0, hTabs, (HMENU)nullptr, g_hInst, 0);

    // Page: Основное
    {
        CreateLabeledEdit(hPageMain, ID_EDIT_OUTPUT_TEMPLATE, L"Шаблон (-o):", 8, 8, 600, 24);
        HWND chkIgnore = CreateCheck(hPageMain, ID_CHK_IGNORE_CONFIG, L"--ignore-config", 8, 44);
        HWND chkLive = CreateCheck(hPageMain, ID_CHK_LIVE_FROM_START, L"--live-from-start", 180, 44);
        HWND chkSkip = CreateCheck(hPageMain, ID_CHK_SKIP_DOWNLOAD, L"--skip-download", 360, 44);
        HWND chkCF = CreateCheck(hPageMain, ID_CHK_CHECK_FORMATS, L"--check-formats", 8, 70);

        CreateWindowW(L"STATIC", L"--merge-output-format:", WS_CHILD|WS_VISIBLE, 8, 98, 160, 22, hPageMain, (HMENU)nullptr, g_hInst, 0);
        HWND comboMerge = CreateCombo(hPageMain, ID_COMBO_MERGE_FMT, 170, 96, 180, 200);
        AddComboItems(comboMerge, {L"(не задано)", L"avi", L"flv", L"mkv", L"mov", L"mp4", L"webm"}, 0);

        // defaults
        SendMessageW(chkIgnore, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(chkLive, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(chkSkip, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(chkCF, BM_SETCHECK, BST_UNCHECKED, 0);
        SetText(GetDlgItem(hPageMain, ID_EDIT_OUTPUT_TEMPLATE), L"%(title)s.%(ext)s");

        // --config-locations (по смыслу тоже тут)
        CreateLabeledEdit(hPageMain, ID_EDIT_CONFIG_LOCATIONS, L"--config-locations:", 8, 130, 600, 24);
    }

    // Page: Сеть/Загрузчик
    {
        CreateLabeledEdit(hPageNet, ID_EDIT_PROXY, L"--proxy URL:", 8, 8, 600, 24);

        CreateWindowW(L"STATIC", L"--downloader:", WS_CHILD|WS_VISIBLE, 8, 40, 120, 22, hPageNet, (HMENU)nullptr, g_hInst, 0);
        HWND comboDl = CreateCombo(hPageNet, ID_COMBO_DOWNLOADER, 130, 38, 180, 200);
        AddComboItems(comboDl, {L"(native)", L"aria2c", L"axel", L"curl", L"ffmpeg", L"httpie", L"wget"}, 0);

        CreateLabeledEdit(hPageNet, ID_EDIT_DOWNLOADER_PROTOCOLS, L"Протоколы (опц.):", 8, 68, 600, 24);
        CreateWindowW(L"STATIC", L"Сопоставления:", WS_CHILD|WS_VISIBLE, 8, 100, 120, 22, hPageNet, (HMENU)nullptr, g_hInst, 0);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL, 8, 120, 400, 100, hPageNet, (HMENU)(INT_PTR)ID_LIST_DL_MAP, g_hInst, 0);
        CreateButton(hPageNet, ID_BTN_ADD_DL_MAP, L"Добавить", 420, 120, 100, 24);
        CreateButton(hPageNet, ID_BTN_REMOVE_DL_MAP, L"Удалить", 420, 150, 100, 24);

        CreateLabeledEdit(hPageNet, ID_EDIT_DOWNLOADER_ARGS, L"--downloader-args:", 8, 230, 600, 24);
    }

    // Page: Файлы/Куки/Кэш
    {
        CreateLabeledEdit(hPageFiles, ID_EDIT_BATCH_FILE, L"-a, --batch-file:", 8, 8, 520, 24);
        CreateButton(hPageFiles, ID_BTN_BROWSE_BATCH, L"Обзор...", 534, 8, 80, 24);

        CreateLabeledEdit(hPageFiles, ID_EDIT_COOKIES_FILE, L"--cookies FILE:", 8, 40, 520, 24);
        CreateButton(hPageFiles, ID_BTN_BROWSE_COOKIES, L"Обзор...", 534, 40, 80, 24);

        CreateLabeledEdit(hPageFiles, ID_EDIT_COOKIES_FROM_BROWSER, L"--cookies-from-browser:", 8, 72, 600, 24);
        CreateLabeledEdit(hPageFiles, ID_EDIT_CACHE_DIR, L"--cache-dir:", 8, 104, 520, 24);
        CreateButton(hPageFiles, ID_BTN_BROWSE_CACHE, L"Обзор...", 534, 104, 80, 24);

        CreateCheck(hPageFiles, ID_CHK_NO_CACHE_DIR, L"--no-cache-dir", 8, 136);
        CreateCheck(hPageFiles, ID_CHK_RM_CACHE_DIR, L"--rm-cache-dir", 160, 136);
    }

    // Page: Эмбед/Субтитры
    {
        CreateCheck(hPageEmbed, ID_CHK_EMBED_SUBS, L"--embed-subs", 8, 8);
        CreateCheck(hPageEmbed, ID_CHK_EMBED_THUMB, L"--embed-thumbnail", 8, 34);
        CreateCheck(hPageEmbed, ID_CHK_EMBED_META, L"--embed-metadata", 8, 60);
        CreateCheck(hPageEmbed, ID_CHK_EMBED_CHAPS, L"--embed-chapters", 8, 86);

        CreateWindowW(L"STATIC", L"--convert-subs:", WS_CHILD|WS_VISIBLE, 240, 10, 120, 22, hPageEmbed, (HMENU)nullptr, g_hInst, 0);
        HWND c1 = CreateCombo(hPageEmbed, ID_COMBO_CONVERT_SUBS, 360, 8, 160, 200);
        AddComboItems(c1, {L"none", L"ass", L"lrc", L"srt", L"vtt"}, 0);

        CreateWindowW(L"STATIC", L"--convert-thumbnails:", WS_CHILD|WS_VISIBLE, 240, 42, 140, 22, hPageEmbed, (HMENU)nullptr, g_hInst, 0);
        HWND c2 = CreateCombo(hPageEmbed, ID_COMBO_CONVERT_THUMBS, 380, 40, 140, 200);
        AddComboItems(c2, {L"none", L"jpg", L"png", L"webp"}, 0);
    }

    // Page: Главы/Монтаж
    {
        CreateCheck(hPageChapters, ID_CHK_SPLIT_CHAPTERS, L"--split-chapters", 8, 8);
        CreateLabeledEdit(hPageChapters, ID_EDIT_REMOVE_CHAPTERS, L"--remove-chapters (regex):", 8, 40, 600, 24);
        CreateCheck(hPageChapters, ID_CHK_FORCE_KF_CUTS, L"--force-keyframes-at-cuts", 8, 72);
    }

    // Page: Дополнительно
    {
        CreateWindowW(L"STATIC", L"Кодировка лога:", WS_CHILD|WS_VISIBLE, 8, 8, 120, 22, hPageAdv, (HMENU)nullptr, g_hInst, 0);
        HWND c3 = CreateCombo(hPageAdv, ID_COMBO_LOG_ENCODING, 130, 6, 160, 200);
        AddComboItems(c3, {L"Auto", L"UTF-8", L"CP1251", L"CP866"}, 0);

        CreateLabeledEdit(hPageAdv, ID_EDIT_CONFIG_YTDLP_PATH, L"Путь к yt-dlp.exe (опц.):", 8, 38, 600, 24);
    }

    // Log
    hEditLog = CreateMultilineEdit(hWnd, ID_EDIT_LOG, 8, 8, 600, 180);
    SendMessageW(hEditLog, EM_SETLIMITTEXT, (WPARAM)50*1024*1024, 0); // 50MB

    // Defaults
    SetText(hEditSaveFolder, GetDesktopPath());
}

void ShowTabPage(int idx) {
    ShowWindow(hPageMain,      idx==0 ? SW_SHOW : SW_HIDE);
    ShowWindow(hPageNet,       idx==1 ? SW_SHOW : SW_HIDE);
    ShowWindow(hPageFiles,     idx==2 ? SW_SHOW : SW_HIDE);
    ShowWindow(hPageEmbed,     idx==3 ? SW_SHOW : SW_HIDE);
    ShowWindow(hPageChapters,  idx==4 ? SW_SHOW : SW_HIDE);
    ShowWindow(hPageAdv,       idx==5 ? SW_SHOW : SW_HIDE);
}

// -------------------- File dialogs --------------------
std::wstring BrowseFolder(const std::wstring& /*initial*/) {
    BROWSEINFOW bi = {0};
    bi.lpszTitle = L"Выберите папку";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    bi.hwndOwner = g_hWnd;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            CoTaskMemFree(pidl);
            return path;
        }
        CoTaskMemFree(pidl);
    }
    return L"";
}
std::wstring BrowseFileOpen(const wchar_t* filter, const wchar_t* title) {
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) return file;
    return L"";
}

// -------------------- Run yt-dlp --------------------
struct RunParams {
    std::wstring cmdline;
    bool jsonMode = false;
    int logEncodingCP = 0;
};

void PostLog(const std::wstring& s) {
    auto p = new std::wstring(s);
    PostMessageW(g_hWnd, WM_APP_LOG_APPEND, 0, (LPARAM)p);
}
void PostProgress(double val) {
    int iv = (val >= 0) ? (int)(val + 0.5) : -1;
    PostMessageW(g_hWnd, WM_APP_PROGRESS, (WPARAM)iv, 0);
}

double ParseProgressPercent(const std::wstring& line) {
    int n = (int)line.size();
    for (int i = 0; i < n; ++i) {
        if (line[i] == L'%') {
            int j = i-1;
            int start = j;
            while (j>=0 && ((line[j] >= L'0' && line[j] <= L'9') || line[j]==L'.' || line[j]==L',')) { j--; }
            start = j+1;
            if (start <= i-1) {
                std::wstring num = line.substr(start, i-start);
                for (auto& ch : num) if (ch==L',') ch=L'.';
                try {
                    double v = std::stod(num);
                    if (v >= 0.0 && v <= 100.0) return v;
                } catch (...) {}
            }
        }
    }
    return -1.0;
}

DWORD WINAPI ReaderThread(LPVOID lpParam) {
    std::unique_ptr<RunParams> rp((RunParams*)lpParam); // auto free
    SECURITY_ATTRIBUTES sa{ sizeof(sa), NULL, TRUE };
    HANDLE hStdOutRead = NULL, hStdOutWrite = NULL;

    auto finish = [&]() -> DWORD {
        if (hStdOutRead) CloseHandle(hStdOutRead);
        if (hStdOutWrite) CloseHandle(hStdOutWrite);
        PostMessageW(g_hWnd, WM_APP_DONE, 0, 0);
        return 0;
    };

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        PostLog(L"[ERR] CreatePipe failed\r\n");
        return finish();
    }
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdOutput = hStdOutWrite;
    si.hStdError  = hStdOutWrite;

    // Наследуем переменные окружения: форсируем UTF-8 для Python
    SetEnvironmentVariableW(L"PYTHONIOENCODING", L"utf-8");
    SetEnvironmentVariableW(L"PYTHONLEGACYWINDOWSSTDIO", L"1");

    std::wstring ytdlpPath = GetText(GetDlgItem(hPageAdv, ID_EDIT_CONFIG_YTDLP_PATH));
    std::wstring app = ytdlpPath.empty() ? L"yt-dlp.exe" : ytdlpPath;

    std::wstring cl = app + L" " + rp->cmdline;

    PROCESS_INFORMATION pi{};
    wchar_t* cmd = _wcsdup(cl.c_str());
    if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::wstring msg = L"[ERR] Не удалось запустить yt-dlp. WinErr=" + std::to_wstring(GetLastError()) + L"\r\n";
        PostLog(msg);
        free(cmd);
        return finish();
    }
    free(cmd);
    CloseHandle(hStdOutWrite); hStdOutWrite = NULL;

    g_LastRawStdout.clear();
    const DWORD BUFSZ = 8192;
    std::vector<BYTE> buf(BUFSZ);

    while (true) {
        DWORD rd = 0;
        BOOL ok = ReadFile(hStdOutRead, buf.data(), BUFSZ, &rd, NULL);
        if (!ok || rd == 0) break;
        if (rp->jsonMode)
            g_LastRawStdout.insert(g_LastRawStdout.end(), buf.begin(), buf.begin()+rd);

        std::vector<BYTE> chunk(buf.begin(), buf.begin()+rd);
        std::wstring ws = DecodeBytes(chunk, rp->logEncodingCP);
        for (auto& ch : ws) if (ch == L'\r') ch = L'\n';
        std::wstringstream ss(ws);
        std::wstring line;
        while (std::getline(ss, line, L'\n')) {
            if (line.empty()) continue;
            PostLog(line + L"\r\n");
            double p = ParseProgressPercent(line);
            if (p >= 0.0) PostProgress(p);
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return finish();
}

std::wstring BuildCmdlineCommon(bool listFormats, bool jsonMode) {
    std::vector<std::wstring> args;
    // Базовые
    std::wstring save = GetText(hEditSaveFolder);
    if (!save.empty()) { args.push_back(L"-P"); args.push_back(QuoteArg(save)); }

    std::wstring outTpl = GetText(GetDlgItem(hPageMain, ID_EDIT_OUTPUT_TEMPLATE));
    if (!outTpl.empty()) { args.push_back(L"-o"); args.push_back(QuoteArg(outTpl)); }

    if (SendMessageW(GetDlgItem(hPageMain, ID_CHK_IGNORE_CONFIG), BM_GETCHECK, 0, 0) == BST_CHECKED) {
        args.push_back(L"--ignore-config");
    }
    std::wstring cfgLoc = GetText(GetDlgItem(hPageMain, ID_EDIT_CONFIG_LOCATIONS));
    if (!cfgLoc.empty()) { args.push_back(L"--config-locations"); args.push_back(QuoteArg(cfgLoc)); }

    if (SendMessageW(GetDlgItem(hPageMain, ID_CHK_LIVE_FROM_START), BM_GETCHECK, 0, 0) == BST_CHECKED) {
        args.push_back(L"--live-from-start");
    }

    std::wstring proxy = GetText(GetDlgItem(hPageNet, ID_EDIT_PROXY));
    if (!proxy.empty()) { args.push_back(L"--proxy"); args.push_back(QuoteArg(proxy)); }

    // Downloader + протоколы (список)
    int selDl = (int)SendMessageW(GetDlgItem(hPageNet, ID_COMBO_DOWNLOADER), CB_GETCURSEL, 0, 0);
    std::wstring dlName;
    if (selDl > 0) {
        wchar_t buf[128];
        SendMessageW(GetDlgItem(hPageNet, ID_COMBO_DOWNLOADER), CB_GETLBTEXT, selDl, (LPARAM)buf);
        dlName = buf;
        // Если нет сопоставлений, добавим дефолтный --downloader NAME
        int count = (int)SendMessageW(GetDlgItem(hPageNet, ID_LIST_DL_MAP), LB_GETCOUNT, 0, 0);
        if (count <= 0) {
            args.push_back(L"--downloader");
            args.push_back(dlName);
        }
    }
    // Сопоставления протоколов
    int mapCount = (int)SendMessageW(GetDlgItem(hPageNet, ID_LIST_DL_MAP), LB_GETCOUNT, 0, 0);
    for (int i=0; i<mapCount; ++i) {
        wchar_t line[256];
        SendMessageW(GetDlgItem(hPageNet, ID_LIST_DL_MAP), LB_GETTEXT, i, (LPARAM)line);
        args.push_back(L"--downloader");
        args.push_back(QuoteArg(line));
    }
    // downloader-args
    std::wstring dlArgs = GetText(GetDlgItem(hPageNet, ID_EDIT_DOWNLOADER_ARGS));
    if (!dlArgs.empty()) { args.push_back(L"--downloader-args"); args.push_back(QuoteArg(dlArgs)); }

    // batch-file
    std::wstring batch = GetText(GetDlgItem(hPageFiles, ID_EDIT_BATCH_FILE));
    if (!batch.empty()) { args.push_back(L"-a"); args.push_back(QuoteArg(batch)); }

    // cookies
    std::wstring cookies = GetText(GetDlgItem(hPageFiles, ID_EDIT_COOKIES_FILE));
    if (!cookies.empty()) { args.push_back(L"--cookies"); args.push_back(QuoteArg(cookies)); }
    std::wstring cfb = GetText(GetDlgItem(hPageFiles, ID_EDIT_COOKIES_FROM_BROWSER));
    if (!cfb.empty()) { args.push_back(L"--cookies-from-browser"); args.push_back(QuoteArg(cfb)); }

    // cache
    std::wstring cacheDir = GetText(GetDlgItem(hPageFiles, ID_EDIT_CACHE_DIR));
    if (!cacheDir.empty()) { args.push_back(L"--cache-dir"); args.push_back(QuoteArg(cacheDir)); }
    if (SendMessageW(GetDlgItem(hPageFiles, ID_CHK_NO_CACHE_DIR), BM_GETCHECK, 0, 0) == BST_CHECKED) {
        args.push_back(L"--no-cache-dir");
    }
    if (SendMessageW(GetDlgItem(hPageFiles, ID_CHK_RM_CACHE_DIR), BM_GETCHECK, 0, 0) == BST_CHECKED) {
        args.push_back(L"--rm-cache-dir");
    }

    // Эмбед/Саб
    if (SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_SUBS), BM_GETCHECK, 0, 0) == BST_CHECKED) args.push_back(L"--embed-subs");
    if (SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_THUMB), BM_GETCHECK, 0, 0) == BST_CHECKED) args.push_back(L"--embed-thumbnail");
    if (SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_META), BM_GETCHECK, 0, 0) == BST_CHECKED) args.push_back(L"--embed-metadata");
    if (SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_CHAPS), BM_GETCHECK, 0, 0) == BST_CHECKED) args.push_back(L"--embed-chapters");

    int selCS = (int)SendMessageW(GetDlgItem(hPageEmbed, ID_COMBO_CONVERT_SUBS), CB_GETCURSEL, 0, 0);
    if (selCS >= 0) {
        wchar_t buf[64]; SendMessageW(GetDlgItem(hPageEmbed, ID_COMBO_CONVERT_SUBS), CB_GETLBTEXT, selCS, (LPARAM)buf);
        if (wcscmp(buf, L"none") != 0) {
            args.push_back(L"--convert-subs");
            args.push_back(buf);
        }
    }
    int selCT = (int)SendMessageW(GetDlgItem(hPageEmbed, ID_COMBO_CONVERT_THUMBS), CB_GETCURSEL, 0, 0);
    if (selCT >= 0) {
        wchar_t buf[64]; SendMessageW(GetDlgItem(hPageEmbed, ID_COMBO_CONVERT_THUMBS), CB_GETLBTEXT, selCT, (LPARAM)buf);
        if (wcscmp(buf, L"none") != 0) {
            args.push_back(L"--convert-thumbnails");
            args.push_back(buf);
        }
    }

    // Главы
    if (SendMessageW(GetDlgItem(hPageChapters, ID_CHK_SPLIT_CHAPTERS), BM_GETCHECK, 0, 0) == BST_CHECKED) args.push_back(L"--split-chapters");
    std::wstring rem = GetText(GetDlgItem(hPageChapters, ID_EDIT_REMOVE_CHAPTERS));
    if (!rem.empty()) { args.push_back(L"--remove-chapters"); args.push_back(QuoteArg(rem)); }
    if (SendMessageW(GetDlgItem(hPageChapters, ID_CHK_FORCE_KF_CUTS), BM_GETCHECK, 0, 0) == BST_CHECKED) args.push_back(L"--force-keyframes-at-cuts");

    // Merge output
    int selMerge = (int)SendMessageW(GetDlgItem(hPageMain, ID_COMBO_MERGE_FMT), CB_GETCURSEL, 0, 0);
    if (selMerge > 0) {
        wchar_t buf[32]; SendMessageW(GetDlgItem(hPageMain, ID_COMBO_MERGE_FMT), CB_GETLBTEXT, selMerge, (LPARAM)buf);
        args.push_back(L"--merge-output-format");
        args.push_back(buf);
    }

    if (SendMessageW(GetDlgItem(hPageMain, ID_CHK_CHECK_FORMATS), BM_GETCHECK, 0, 0) == BST_CHECKED)
        args.push_back(L"--check-formats");

    if (SendMessageW(GetDlgItem(hPageMain, ID_CHK_SKIP_DOWNLOAD), BM_GETCHECK, 0, 0) == BST_CHECKED)
        args.push_back(L"--skip-download");

    // Режимы
    if (listFormats) args.push_back(L"-F");
    if (jsonMode)    args.push_back(L"-J");

    // URL в конце (если нет batch-file)
    std::wstring url = GetText(hEditUrl);
    if (!url.empty() && batch.empty()) {
        args.push_back(QuoteArg(url));
    }

    // Соберём строку
    std::wstring cmd;
    for (size_t i=0;i<args.size();++i) {
        if (i) cmd += L" ";
        cmd += args[i];
    }
    // Без ANSI-цветов для чистого парсинга
    cmd += L" --no-color";
    return cmd;
}

void StartRun(bool listFormats, bool jsonMode) {
    if (g_Running) return;

    // Очистим прогресс
    SendMessageW(hProgress, PBM_SETPOS, 0, 0);
    SetProgressText(0.0);
    EnableMarquee(false);

    if (listFormats) {
        g_ListFormatsMode = true;
        g_JsonMode = false;
    } else if (jsonMode) {
        g_ListFormatsMode = false;
        g_JsonMode = true;
    } else {
        g_ListFormatsMode = false;
        g_JsonMode = false;
    }

    std::wstring cmd = BuildCmdlineCommon(listFormats, jsonMode);
    AppendLog(L"> yt-dlp " + cmd + L"\r\n");

    RunParams* rp = new RunParams();
    rp->cmdline = cmd;
    rp->jsonMode = jsonMode;
    rp->logEncodingCP = GetLogEncodingCP();

    g_Running = true;
    DWORD tid;
    HANDLE hThr = CreateThread(NULL, 0, ReaderThread, rp, 0, &tid);
    if (hThr) CloseHandle(hThr);
}

void SaveJsonToFile() {
    std::wstring folder = GetText(hEditSaveFolder);
    if (folder.empty()) folder = GetDesktopPath();
    std::wstring path = folder + L"\\info.json";
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        AppendLog(L"[ERR] Не могу сохранить JSON в " + path + L"\r\n");
        return;
    }
    DWORD wr=0;
    if (!g_LastRawStdout.empty())
        WriteFile(h, g_LastRawStdout.data(), (DWORD)g_LastRawStdout.size(), &wr, NULL);
    CloseHandle(h);
    AppendLog(L"[OK] JSON сохранён: " + path + L"\r\n");
}

// -------------------- Settings --------------------
void LoadSettings() {
    SetText(hEditSaveFolder, IniRead(L"save_folder", GetDesktopPath().c_str()));
    SetText(GetDlgItem(hPageMain, ID_EDIT_OUTPUT_TEMPLATE), IniRead(L"output_tpl", L"%(title)s.%(ext)s"));

    if (IniRead(L"ignore_config", L"0") == L"1") SendMessageW(GetDlgItem(hPageMain, ID_CHK_IGNORE_CONFIG), BM_SETCHECK, BST_CHECKED, 0);
    if (IniRead(L"live_from_start", L"0") == L"1") SendMessageW(GetDlgItem(hPageMain, ID_CHK_LIVE_FROM_START), BM_SETCHECK, BST_CHECKED, 0);
    if (IniRead(L"skip_download", L"0") == L"1") SendMessageW(GetDlgItem(hPageMain, ID_CHK_SKIP_DOWNLOAD), BM_SETCHECK, BST_CHECKED, 0);
    if (IniRead(L"check_formats", L"0") == L"1") SendMessageW(GetDlgItem(hPageMain, ID_CHK_CHECK_FORMATS), BM_SETCHECK, BST_CHECKED, 0);

    std::wstring mergeFmt = IniRead(L"merge_fmt", L"");
    if (!mergeFmt.empty()) {
        HWND c = GetDlgItem(hPageMain, ID_COMBO_MERGE_FMT);
        int cnt = (int)SendMessageW(c, CB_GETCOUNT, 0, 0);
        for (int i=0;i<cnt;++i) {
            wchar_t buf[64]; SendMessageW(c, CB_GETLBTEXT, i, (LPARAM)buf);
            if (mergeFmt == buf) { SendMessageW(c, CB_SETCURSEL, i, 0); break; }
        }
    }

    if (IniRead(L"embed_subs", L"0") == L"1") SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_SUBS), BM_SETCHECK, BST_CHECKED, 0);
    if (IniRead(L"embed_thumb", L"0") == L"1") SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_THUMB), BM_SETCHECK, BST_CHECKED, 0);
    if (IniRead(L"embed_meta", L"0") == L"1") SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_META), BM_SETCHECK, BST_CHECKED, 0);
    if (IniRead(L"embed_chaps", L"0") == L"1") SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_CHAPS), BM_SETCHECK, BST_CHECKED, 0);

    auto setComboSelByText = [](HWND c, const std::wstring& text) {
        if (text.empty()) return;
        int cnt = (int)SendMessageW(c, CB_GETCOUNT, 0, 0);
        for (int i=0;i<cnt;++i) {
            wchar_t buf[64]; SendMessageW(c, CB_GETLBTEXT, i, (LPARAM)buf);
            if (text == buf) { SendMessageW(c, CB_SETCURSEL, i, 0); break; }
        }
    };
    setComboSelByText(GetDlgItem(hPageEmbed, ID_COMBO_CONVERT_SUBS), IniRead(L"convert_subs", L"none"));
    setComboSelByText(GetDlgItem(hPageEmbed, ID_COMBO_CONVERT_THUMBS), IniRead(L"convert_thumbs", L"none"));

    if (IniRead(L"split_chapters", L"0") == L"1") SendMessageW(GetDlgItem(hPageChapters, ID_CHK_SPLIT_CHAPTERS), BM_SETCHECK, BST_CHECKED, 0);
    SetText(GetDlgItem(hPageChapters, ID_EDIT_REMOVE_CHAPTERS), IniRead(L"remove_chapters", L""));
    if (IniRead(L"force_kf_cuts", L"0") == L"1") SendMessageW(GetDlgItem(hPageChapters, ID_CHK_FORCE_KF_CUTS), BM_SETCHECK, BST_CHECKED, 0);

    SetText(GetDlgItem(hPageNet, ID_EDIT_PROXY), IniRead(L"proxy", L""));
    setComboSelByText(GetDlgItem(hPageNet, ID_COMBO_DOWNLOADER), IniRead(L"downloader", L"(native)"));
    SetText(GetDlgItem(hPageNet, ID_EDIT_DOWNLOADER_PROTOCOLS), IniRead(L"dl_protocols", L""));
    SetText(GetDlgItem(hPageNet, ID_EDIT_DOWNLOADER_ARGS), IniRead(L"dl_args", L""));

    SetText(GetDlgItem(hPageFiles, ID_EDIT_BATCH_FILE), IniRead(L"batch_file", L""));
    SetText(GetDlgItem(hPageFiles, ID_EDIT_COOKIES_FILE), IniRead(L"cookies_file", L""));
    SetText(GetDlgItem(hPageFiles, ID_EDIT_COOKIES_FROM_BROWSER), IniRead(L"cookies_from_browser", L""));
    SetText(GetDlgItem(hPageFiles, ID_EDIT_CACHE_DIR), IniRead(L"cache_dir", L""));
    if (IniRead(L"no_cache_dir", L"0") == L"1") SendMessageW(GetDlgItem(hPageFiles, ID_CHK_NO_CACHE_DIR), BM_SETCHECK, BST_CHECKED, 0);
    if (IniRead(L"rm_cache_dir", L"0") == L"1") SendMessageW(GetDlgItem(hPageFiles, ID_CHK_RM_CACHE_DIR), BM_SETCHECK, BST_CHECKED, 0);

    SetText(GetDlgItem(hPageMain, ID_EDIT_CONFIG_LOCATIONS), IniRead(L"config_locations", L""));
    SetText(GetDlgItem(hPageAdv, ID_EDIT_CONFIG_YTDLP_PATH), IniRead(L"ytdlp_path", L""));
    int encIdx = _wtoi(IniRead(L"log_encoding_idx", L"0").c_str());
    SendMessageW(GetDlgItem(hPageAdv, ID_COMBO_LOG_ENCODING), CB_SETCURSEL, encIdx, 0);
}

void SaveSettings() {
    IniWrite(L"save_folder", GetText(hEditSaveFolder));
    IniWrite(L"output_tpl", GetText(GetDlgItem(hPageMain, ID_EDIT_OUTPUT_TEMPLATE)));

    IniWrite(L"ignore_config", SendMessageW(GetDlgItem(hPageMain, ID_CHK_IGNORE_CONFIG), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");
    IniWrite(L"live_from_start", SendMessageW(GetDlgItem(hPageMain, ID_CHK_LIVE_FROM_START), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");
    IniWrite(L"skip_download", SendMessageW(GetDlgItem(hPageMain, ID_CHK_SKIP_DOWNLOAD), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");
    IniWrite(L"check_formats", SendMessageW(GetDlgItem(hPageMain, ID_CHK_CHECK_FORMATS), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");

    int selMerge = (int)SendMessageW(GetDlgItem(hPageMain, ID_COMBO_MERGE_FMT), CB_GETCURSEL, 0, 0);
    if (selMerge > 0) {
        wchar_t buf[64]; SendMessageW(GetDlgItem(hPageMain, ID_COMBO_MERGE_FMT), CB_GETLBTEXT, selMerge, (LPARAM)buf);
        IniWrite(L"merge_fmt", buf);
    } else {
        IniWrite(L"merge_fmt", L"");
    }

    IniWrite(L"embed_subs", SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_SUBS), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");
    IniWrite(L"embed_thumb", SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_THUMB), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");
    IniWrite(L"embed_meta", SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_META), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");
    IniWrite(L"embed_chaps", SendMessageW(GetDlgItem(hPageEmbed, ID_CHK_EMBED_CHAPS), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");

    auto getComboText = [](HWND c)->std::wstring{
        int sel = (int)SendMessageW(c, CB_GETCURSEL, 0, 0);
        if (sel < 0) return L"";
        wchar_t buf[64]; SendMessageW(c, CB_GETLBTEXT, sel, (LPARAM)buf);
        return buf;
    };
    IniWrite(L"convert_subs", getComboText(GetDlgItem(hPageEmbed, ID_COMBO_CONVERT_SUBS)));
    IniWrite(L"convert_thumbs", getComboText(GetDlgItem(hPageEmbed, ID_COMBO_CONVERT_THUMBS)));

    IniWrite(L"split_chapters", SendMessageW(GetDlgItem(hPageChapters, ID_CHK_SPLIT_CHAPTERS), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");
    IniWrite(L"remove_chapters", GetText(GetDlgItem(hPageChapters, ID_EDIT_REMOVE_CHAPTERS)));
    IniWrite(L"force_kf_cuts", SendMessageW(GetDlgItem(hPageChapters, ID_CHK_FORCE_KF_CUTS), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");

    IniWrite(L"proxy", GetText(GetDlgItem(hPageNet, ID_EDIT_PROXY)));
    IniWrite(L"downloader", getComboText(GetDlgItem(hPageNet, ID_COMBO_DOWNLOADER)));
    IniWrite(L"dl_protocols", GetText(GetDlgItem(hPageNet, ID_EDIT_DOWNLOADER_PROTOCOLS)));
    IniWrite(L"dl_args", GetText(GetDlgItem(hPageNet, ID_EDIT_DOWNLOADER_ARGS)));

    IniWrite(L"batch_file", GetText(GetDlgItem(hPageFiles, ID_EDIT_BATCH_FILE)));
    IniWrite(L"cookies_file", GetText(GetDlgItem(hPageFiles, ID_EDIT_COOKIES_FILE)));
    IniWrite(L"cookies_from_browser", GetText(GetDlgItem(hPageFiles, ID_EDIT_COOKIES_FROM_BROWSER)));
    IniWrite(L"cache_dir", GetText(GetDlgItem(hPageFiles, ID_EDIT_CACHE_DIR)));
    IniWrite(L"no_cache_dir", SendMessageW(GetDlgItem(hPageFiles, ID_CHK_NO_CACHE_DIR), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");
    IniWrite(L"rm_cache_dir", SendMessageW(GetDlgItem(hPageFiles, ID_CHK_RM_CACHE_DIR), BM_GETCHECK, 0, 0)==BST_CHECKED ? L"1" : L"0");

    IniWrite(L"config_locations", GetText(GetDlgItem(hPageMain, ID_EDIT_CONFIG_LOCATIONS)));
    IniWrite(L"ytdlp_path", GetText(GetDlgItem(hPageAdv, ID_EDIT_CONFIG_YTDLP_PATH)));

    int encIdx = (int)SendMessageW(GetDlgItem(hPageAdv, ID_COMBO_LOG_ENCODING), CB_GETCURSEL, 0, 0);
    IniWrite(L"log_encoding_idx", std::to_wstring(encIdx));
}

// -------------------- WndProc --------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        InitCommonControls();
        CreateTopControls(hWnd);
        g_IniPath = GetExeDir() + L"\\settings.ini";
        LoadSettings();
        DoLayout();
        ShowTabPage(0);
        return 0;
    }
    case WM_SIZE:
        DoLayout();
        return 0;
    case WM_GETMINMAXINFO: {
        LPMINMAXINFO pm = (LPMINMAXINFO)lParam;
        pm->ptMinTrackSize.x = g_MinWidth;
        pm->ptMinTrackSize.y = g_MinHeight;
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        // int code = HIWORD(wParam);
        if (id == ID_BTN_BROWSE_SAVEFOLDER) {
            std::wstring p = BrowseFolder(GetText(hEditSaveFolder));
            if (!p.empty()) SetText(hEditSaveFolder, p);
        } else if (id == ID_BTN_DOWNLOAD) {
            StartRun(false, false);
        } else if (id == ID_BTN_LIST_FORMATS) {
            StartRun(true, false);
        } else if (id == ID_BTN_JSON) {
            StartRun(false, true);
        } else if (id == ID_BTN_TOGGLE_LOG) {
            g_LogVisible = !g_LogVisible;
            DoLayout();
        } else if (id == ID_BTN_BROWSE_BATCH) {
            auto f = BrowseFileOpen(L"Text Files\0*.txt;*.lst\0All Files\0*.*\0", L"Выберите файл со списком URL");
            if (!f.empty()) SetText(GetDlgItem(hPageFiles, ID_EDIT_BATCH_FILE), f);
        } else if (id == ID_BTN_BROWSE_COOKIES) {
            auto f = BrowseFileOpen(L"All Files\0*.*\0", L"Выберите файл cookies");
            if (!f.empty()) SetText(GetDlgItem(hPageFiles, ID_EDIT_COOKIES_FILE), f);
        } else if (id == ID_BTN_BROWSE_CACHE) {
            std::wstring p = BrowseFolder(L"");
            if (!p.empty()) SetText(GetDlgItem(hPageFiles, ID_EDIT_CACHE_DIR), p);
        } else if (id == ID_BTN_ADD_DL_MAP) {
            std::wstring protos = GetText(GetDlgItem(hPageNet, ID_EDIT_DOWNLOADER_PROTOCOLS));
            int sel = (int)SendMessageW(GetDlgItem(hPageNet, ID_COMBO_DOWNLOADER), CB_GETCURSEL, 0, 0);
            if (sel <= 0) {
                MessageBoxW(hWnd, L"Выберите downloader.", L"Внимание", MB_OK|MB_ICONWARNING);
            } else if (protos.empty()) {
                MessageBoxW(hWnd, L"Укажите протоколы (например: dash,m3u8 или http,ftp).", L"Внимание", MB_OK|MB_ICONWARNING);
            } else {
                wchar_t name[64]; SendMessageW(GetDlgItem(hPageNet, ID_COMBO_DOWNLOADER), CB_GETLBTEXT, sel, (LPARAM)name);
                std::wstring line = protos + L":" + name;
                SendMessageW(GetDlgItem(hPageNet, ID_LIST_DL_MAP), LB_ADDSTRING, 0, (LPARAM)line.c_str());
            }
        } else if (id == ID_BTN_REMOVE_DL_MAP) {
            int sel = (int)SendMessageW(GetDlgItem(hPageNet, ID_LIST_DL_MAP), LB_GETCURSEL, 0, 0);
            if (sel >= 0) SendMessageW(GetDlgItem(hPageNet, ID_LIST_DL_MAP), LB_DELETESTRING, sel, 0);
        }
        return 0;
    }
    case WM_NOTIFY: {
        LPNMHDR nm = (LPNMHDR)lParam;
        if (nm->idFrom == ID_TABS && nm->code == TCN_SELCHANGE) {
            int idx = TabCtrl_GetCurSel(hTabs);
            ShowTabPage(idx);
        }
        return 0;
    }
    case WM_APP_LOG_APPEND: {
        std::wstring* p = (std::wstring*)lParam;
        AppendLog(*p);
        delete p;
        return 0;
    }
    case WM_APP_PROGRESS: {
        int iv = (int)wParam;
        if (iv >= 0) {
            EnableMarquee(false);
            SendMessageW(hProgress, PBM_SETPOS, iv, 0);
            SetProgressText((double)iv);
        } else {
            EnableMarquee(true);
            SetProgressText(-1);
        }
        return 0;
    }
    case WM_APP_DONE: {
        g_Running = false;
        EnableMarquee(false);
        if (g_JsonMode) {
            SaveJsonToFile();
            g_JsonMode = false;
        }
        return 0;
    }
    case WM_CLOSE:
        SaveSettings();
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    g_hInst = hInstance;
    // DPI aware (best effort)
    HMODULE hUser32 = GetModuleHandleW(L"user32");
    auto pSetDpi = (BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT))GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
    if (pSetDpi) pSetDpi((DPI_AWARENESS_CONTEXT)-4); // PER_MONITOR_AWARE_V2

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = L"YT_DLP_GUI";
    RegisterClassW(&wc);

    g_hWnd = CreateWindowExW(0, wc.lpszClassName, L"yt-dlp GUI", WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700, NULL, NULL, hInstance, NULL);
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
