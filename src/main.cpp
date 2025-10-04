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
HWND hLogEdit, hToggleLogBtn, hTabControl, hPresetCombo;
HWND hLblProxy, hLblWait, hLblSections;
HWND hLblConvSubs, hLblConvSubsHint, hLblConvThumb, hLblConvThumbHint;
HWND hLblMerge, hLblMergeHint, hLblSort, hLblExtractor, hLblHeaders, hLblCookies;

// Вкладки
HWND hTab1, hTab2, hTab3;

// Элементы управления на вкладках
HWND hChkIgnoreConfig, hChkLiveFromStart, hChkNoCache, hChkSkipDownload;
HWND hChkSaveJson, hChkListFormats, hChkCheckFormats;
HWND hChkEmbedSubs, hChkEmbedThumbnail, hChkEmbedMetadata, hChkEmbedChapters;
HWND hChkSplitChapters, hChkForceKeyframes;

HWND hEditConfigLocations, hEditWaitForVideo, hEditProxy, hEditDownloadSections;
HWND hEditDownloader, hEditDownloaderArgs, hEditBatchFile, hEditOutputTemplate;
HWND hEditCookiesFile, hEditCookiesFromBrowser, hEditCacheDir;
HWND hEditMergeFormat, hEditConvertSubs, hEditConvertThumbnails;
HWND hEditRemoveChapters, hEditFormatSort, hEditExtractorArgs, hEditAddHeaders;

HWND hBtnBrowseBatch, hBtnBrowseCookies, hBtnBrowseCache;
HWND hBtnSavePreset, hBtnDeletePreset;

bool isDownloading = false;
bool isLogVisible = false;
int normalHeight = 640;
int expandedHeight = 890;

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
    
    SendMessage(hChkIgnoreConfig, BM_SETCHECK, settings.ignoreConfig ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkLiveFromStart, BM_SETCHECK, settings.liveFromStart ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkNoCache, BM_SETCHECK, settings.noCacheDir ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkSkipDownload, BM_SETCHECK, settings.skipDownload ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkSaveJson, BM_SETCHECK, settings.saveJson ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkListFormats, BM_SETCHECK, settings.listFormats ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkCheckFormats, BM_SETCHECK, settings.checkFormats ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkEmbedSubs, BM_SETCHECK, settings.embedSubs ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkEmbedThumbnail, BM_SETCHECK, settings.embedThumbnail ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkEmbedMetadata, BM_SETCHECK, settings.embedMetadata ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkEmbedChapters, BM_SETCHECK, settings.embedChapters ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkSplitChapters, BM_SETCHECK, settings.splitChapters ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hChkForceKeyframes, BM_SETCHECK, settings.forceKeyframesAtCuts ? BST_CHECKED : BST_UNCHECKED, 0);
    
    SetWindowTextW(hEditConfigLocations, settings.configLocations.c_str());
    SetWindowTextW(hEditWaitForVideo, settings.waitForVideo.c_str());
    SetWindowTextW(hEditProxy, settings.proxyUrl.c_str());
    SetWindowTextW(hEditDownloadSections, settings.downloadSections.c_str());
    SetWindowTextW(hEditDownloader, settings.downloader.c_str());
    SetWindowTextW(hEditDownloaderArgs, settings.downloaderArgs.c_str());
    SetWindowTextW(hEditBatchFile, settings.batchFile.c_str());
    SetWindowTextW(hEditOutputTemplate, settings.outputTemplate.c_str());
    SetWindowTextW(hEditCookiesFile, settings.cookiesFile.c_str());
    SetWindowTextW(hEditCookiesFromBrowser, settings.cookiesFromBrowser.c_str());
    SetWindowTextW(hEditCacheDir, settings.cacheDir.c_str());
    SetWindowTextW(hEditMergeFormat, settings.mergeOutputFormat.c_str());
    SetWindowTextW(hEditConvertSubs, settings.convertSubs.c_str());
    SetWindowTextW(hEditConvertThumbnails, settings.convertThumbnails.c_str());
    SetWindowTextW(hEditRemoveChapters, settings.removeChapters.c_str());
    SetWindowTextW(hEditFormatSort, settings.formatSort.c_str());
    SetWindowTextW(hEditExtractorArgs, settings.extractorArgs.c_str());
    SetWindowTextW(hEditAddHeaders, settings.addHeaders.c_str());
}

// Сохранение настроек из UI
void SaveSettingsFromUI() {
    wchar_t buffer[1024];
    
    GetWindowTextW(hOutputEdit, buffer, 1024);
    settings.outputPath = buffer;
    
    settings.quality = SendMessage(hQualityCombo, CB_GETCURSEL, 0, 0);
    settings.format = SendMessage(hFormatCombo, CB_GETCURSEL, 0, 0);
    
    settings.ignoreConfig = SendMessage(hChkIgnoreConfig, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.liveFromStart = SendMessage(hChkLiveFromStart, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.noCacheDir = SendMessage(hChkNoCache, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.skipDownload = SendMessage(hChkSkipDownload, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.saveJson = SendMessage(hChkSaveJson, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.listFormats = SendMessage(hChkListFormats, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.checkFormats = SendMessage(hChkCheckFormats, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.embedSubs = SendMessage(hChkEmbedSubs, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.embedThumbnail = SendMessage(hChkEmbedThumbnail, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.embedMetadata = SendMessage(hChkEmbedMetadata, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.embedChapters = SendMessage(hChkEmbedChapters, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.splitChapters = SendMessage(hChkSplitChapters, BM_GETCHECK, 0, 0) == BST_CHECKED;
    settings.forceKeyframesAtCuts = SendMessage(hChkForceKeyframes, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    GetWindowTextW(hEditConfigLocations, buffer, 1024); settings.configLocations = buffer;
    GetWindowTextW(hEditWaitForVideo, buffer, 1024); settings.waitForVideo = buffer;
    GetWindowTextW(hEditProxy, buffer, 1024); settings.proxyUrl = buffer;
    GetWindowTextW(hEditDownloadSections, buffer, 1024); settings.downloadSections = buffer;
    GetWindowTextW(hEditDownloader, buffer, 1024); settings.downloader = buffer;
    GetWindowTextW(hEditDownloaderArgs, buffer, 1024); settings.downloaderArgs = buffer;
    GetWindowTextW(hEditBatchFile, buffer, 1024); settings.batchFile = buffer;
    GetWindowTextW(hEditOutputTemplate, buffer, 1024); settings.outputTemplate = buffer;
    GetWindowTextW(hEditCookiesFile, buffer, 1024); settings.cookiesFile = buffer;
    GetWindowTextW(hEditCookiesFromBrowser, buffer, 1024); settings.cookiesFromBrowser = buffer;
    GetWindowTextW(hEditCacheDir, buffer, 1024); settings.cacheDir = buffer;
    GetWindowTextW(hEditMergeFormat, buffer, 1024); settings.mergeOutputFormat = buffer;
    GetWindowTextW(hEditConvertSubs, buffer, 1024); settings.convertSubs = buffer;
    GetWindowTextW(hEditConvertThumbnails, buffer, 1024); settings.convertThumbnails = buffer;
    GetWindowTextW(hEditRemoveChapters, buffer, 1024); settings.removeChapters = buffer;
    GetWindowTextW(hEditFormatSort, buffer, 1024); settings.formatSort = buffer;
    GetWindowTextW(hEditExtractorArgs, buffer, 1024); settings.extractorArgs = buffer;
    GetWindowTextW(hEditAddHeaders, buffer, 1024); settings.addHeaders = buffer;
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
    
    // Основные опции
    if (settings.ignoreConfig) command += L"--ignore-config ";
    if (!settings.configLocations.empty()) command += L"--config-locations \"" + settings.configLocations + L"\" ";
    if (settings.liveFromStart) command += L"--live-from-start ";
    if (!settings.waitForVideo.empty()) command += L"--wait-for-video " + settings.waitForVideo + L" ";
    if (!settings.proxyUrl.empty()) command += L"--proxy \"" + settings.proxyUrl + L"\" ";
    
    // Формат (если не используется пресет или пресет не переопределяет)
    if (presetIdx == 0) {
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
    }
    
    // Опции загрузки
    if (!settings.downloadSections.empty()) command += L"--download-sections \"" + settings.downloadSections + L"\" ";
    if (!settings.downloader.empty()) command += L"--downloader " + settings.downloader + L" ";
    if (!settings.downloaderArgs.empty()) command += L"--downloader-args \"" + settings.downloaderArgs + L"\" ";
    if (!settings.batchFile.empty()) command += L"--batch-file \"" + settings.batchFile + L"\" ";
    
    // Вывод
    if (!settings.outputTemplate.empty()) {
        command += L"-o \"" + settings.outputTemplate + L"\" ";
    } else if (!settings.outputPath.empty()) {
        command += L"-o \"" + settings.outputPath + L"/%(title)s.%(ext)s\" ";
    }
    
    // Cookies и кэш
    if (!settings.cookiesFile.empty()) command += L"--cookies \"" + settings.cookiesFile + L"\" ";
    if (!settings.cookiesFromBrowser.empty()) command += L"--cookies-from-browser " + settings.cookiesFromBrowser + L" ";
    if (!settings.cacheDir.empty()) command += L"--cache-dir \"" + settings.cacheDir + L"\" ";
    if (settings.noCacheDir) command += L"--no-cache-dir ";
    
    // Режимы
    if (settings.skipDownload) command += L"--skip-download ";
    if (settings.saveJson) command += L"-J --write-info-json ";
    if (settings.listFormats) command += L"-F ";
    
    // Формат и конвертация
    if (!settings.mergeOutputFormat.empty()) command += L"--merge-output-format " + settings.mergeOutputFormat + L" ";
    if (settings.checkFormats) command += L"--check-formats ";
    
    // Встраивание
    if (settings.embedSubs) command += L"--embed-subs ";
    if (settings.embedThumbnail) command += L"--embed-thumbnail ";
    if (settings.embedMetadata) command += L"--embed-metadata ";
    if (settings.embedChapters) command += L"--embed-chapters ";
    
    // Конвертация
    if (!settings.convertSubs.empty()) command += L"--convert-subs " + settings.convertSubs + L" ";
    if (!settings.convertThumbnails.empty()) command += L"--convert-thumbnails " + settings.convertThumbnails + L" ";
    
    // Главы
    if (settings.splitChapters) command += L"--split-chapters ";
    if (!settings.removeChapters.empty()) command += L"--remove-chapters \"" + settings.removeChapters + L"\" ";
    if (settings.forceKeyframesAtCuts) command += L"--force-keyframes-at-cuts ";
    
    // Сортировка форматов
    if (!settings.formatSort.empty()) command += L"-S \"" + settings.formatSort + L"\" ";
    
    // Дополнительные аргументы
    if (!settings.extractorArgs.empty()) command += L"--extractor-args " + settings.extractorArgs + L" ";
    if (!settings.addHeaders.empty()) command += L"--add-headers \"" + settings.addHeaders + L"\" ";
    
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

// Обработчик выбора файла
void BrowseFile(HWND hEditControl, const wchar_t* filter, const wchar_t* title) {
    OPENFILENAMEW ofn = { 0 };
    wchar_t fileName[MAX_PATH] = { 0 };
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    
    if (GetOpenFileNameW(&ofn)) {
        SetWindowTextW(hEditControl, fileName);
    }
}

// Создание вкладок
void CreateTabs(HWND hwnd) {
    // Tab Control
    hTabControl = CreateWindowW(WC_TABCONTROLW, L"", 
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                10, 228, 560, 185, hwnd, (HMENU)IDC_TAB, NULL, NULL);
    
    // Важно: выделяем память для строк статически
    static wchar_t szTab1[100], szTab2[100], szTab3[100];
    wcscpy_s(szTab1, 100, L"Основные опции");
    wcscpy_s(szTab2, 100, L"Встраивание / Конвертация");
    wcscpy_s(szTab3, 100, L"Продвинутые");
    
    TCITEMW tie = {0};
    tie.mask = TCIF_TEXT;
    
    tie.pszText = szTab1;
    tie.cchTextMax = wcslen(szTab1);
    TabCtrl_InsertItem(hTabControl, 0, &tie);
    
    tie.pszText = szTab2;
    tie.cchTextMax = wcslen(szTab2);
    TabCtrl_InsertItem(hTabControl, 1, &tie);
    
    tie.pszText = szTab3;
    tie.cchTextMax = wcslen(szTab3);
    TabCtrl_InsertItem(hTabControl, 2, &tie);
}

// Создание контента вкладок
void CreateTabContent(HWND hwnd) {
    HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    
    int tabX = 25, tabY = 258;
    
    // === ВКЛАДКА 1: Основные опции ===
    hChkIgnoreConfig = CreateWindowW(L"BUTTON", L"Игнорировать конфиг (--ignore-config)", 
                                     WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY, 250, 20, hwnd, 
                                     (HMENU)IDC_CHK_IGNORE_CONFIG, NULL, NULL);
    
    hChkLiveFromStart = CreateWindowW(L"BUTTON", L"Прямой эфир с начала (--live-from-start)", 
                                      WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 25, 280, 20, hwnd, 
                                      (HMENU)IDC_CHK_LIVE_FROM_START, NULL, NULL);
    
    hChkSkipDownload = CreateWindowW(L"BUTTON", L"Пропустить загрузку (--skip-download)", 
                                     WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 50, 280, 20, hwnd, 
                                     (HMENU)IDC_CHK_SKIP_DOWNLOAD, NULL, NULL);
    
    hChkSaveJson = CreateWindowW(L"BUTTON", L"Сохранить JSON (-J)", 
                                 WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 75, 200, 20, hwnd, 
                                 (HMENU)IDC_CHK_SAVE_JSON, NULL, NULL);
    
    hChkListFormats = CreateWindowW(L"BUTTON", L"Список форматов (-F)", 
                                    WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 100, 200, 20, hwnd, 
                                    (HMENU)IDC_CHK_LIST_FORMATS, NULL, NULL);
    
    // Метки и поля справа
    hLblProxy = CreateWindowW(L"STATIC", L"Proxy URL:", WS_CHILD | SS_LEFT, tabX + 280, tabY, 100, 18, hwnd, NULL, NULL, NULL);
    hEditProxy = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                 tabX + 280, tabY + 20, 250, 22, hwnd, NULL, NULL, NULL);
    
    hLblWait = CreateWindowW(L"STATIC", L"Ожидание видео (сек):", WS_CHILD | SS_LEFT, tabX + 280, tabY + 50, 150, 18, hwnd, NULL, NULL, NULL);
    hEditWaitForVideo = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                        tabX + 280, tabY + 70, 250, 22, hwnd, NULL, NULL, NULL);
    
    hLblSections = CreateWindowW(L"STATIC", L"Секции для загрузки:", WS_CHILD | SS_LEFT, tabX + 280, tabY + 100, 150, 18, hwnd, NULL, NULL, NULL);
    hEditDownloadSections = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                            tabX + 280, tabY + 120, 250, 22, hwnd, NULL, NULL, NULL);
    
    // === ВКЛАДКА 2: Встраивание / Конвертация ===
    hChkEmbedSubs = CreateWindowW(L"BUTTON", L"Встроить субтитры (--embed-subs)", 
                                  WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY, 250, 20, hwnd, 
                                  (HMENU)IDC_CHK_EMBED_SUBS, NULL, NULL);
    
    hChkEmbedThumbnail = CreateWindowW(L"BUTTON", L"Встроить превью (--embed-thumbnail)", 
                                       WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 25, 280, 20, hwnd, 
                                       (HMENU)IDC_CHK_EMBED_THUMBNAIL, NULL, NULL);
    
    hChkEmbedMetadata = CreateWindowW(L"BUTTON", L"Встроить метаданные (--embed-metadata)", 
                                      WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 50, 280, 20, hwnd, 
                                      (HMENU)IDC_CHK_EMBED_METADATA, NULL, NULL);
    
    hChkEmbedChapters = CreateWindowW(L"BUTTON", L"Встроить главы (--embed-chapters)", 
                                      WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 75, 280, 20, hwnd, 
                                      (HMENU)IDC_CHK_EMBED_CHAPTERS, NULL, NULL);
    
    hChkSplitChapters = CreateWindowW(L"BUTTON", L"Разделить по главам (--split-chapters)", 
                                      WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 100, 280, 20, hwnd, 
                                      (HMENU)IDC_CHK_SPLIT_CHAPTERS, NULL, NULL);
    
    hChkCheckFormats = CreateWindowW(L"BUTTON", L"Проверить форматы (--check-formats)", 
                                     WS_CHILD | BS_AUTOCHECKBOX, tabX, tabY + 125, 280, 20, hwnd, 
                                     (HMENU)IDC_CHK_CHECK_FORMATS, NULL, NULL);
    
    hLblConvSubs = CreateWindowW(L"STATIC", L"Конвертировать субтитры:", WS_CHILD | SS_LEFT, tabX + 300, tabY, 150, 18, hwnd, NULL, NULL, NULL);
    hEditConvertSubs = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                       tabX + 300, tabY + 20, 220, 22, hwnd, NULL, NULL, NULL);
    hLblConvSubsHint = CreateWindowW(L"STATIC", L"(srt, ass, vtt)", WS_CHILD | SS_LEFT, tabX + 300, tabY + 44, 150, 18, hwnd, NULL, NULL, NULL);
    
    hLblConvThumb = CreateWindowW(L"STATIC", L"Конвертировать превью:", WS_CHILD | SS_LEFT, tabX + 300, tabY + 70, 150, 18, hwnd, NULL, NULL, NULL);
    hEditConvertThumbnails = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                             tabX + 300, tabY + 90, 220, 22, hwnd, NULL, NULL, NULL);
    hLblConvThumbHint = CreateWindowW(L"STATIC", L"(jpg, png, webp)", WS_CHILD | SS_LEFT, tabX + 300, tabY + 114, 150, 18, hwnd, NULL, NULL, NULL);
    
    // === ВКЛАДКА 3: Продвинутые ===
    hLblMerge = CreateWindowW(L"STATIC", L"Формат слияния:", WS_CHILD | SS_LEFT, tabX, tabY, 120, 18, hwnd, NULL, NULL, NULL);
    hEditMergeFormat = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                       tabX, tabY + 20, 250, 22, hwnd, NULL, NULL, NULL);
    hLblMergeHint = CreateWindowW(L"STATIC", L"(mp4, mkv, webm...)", WS_CHILD | SS_LEFT, tabX, tabY + 44, 150, 18, hwnd, NULL, NULL, NULL);
    
    hLblSort = CreateWindowW(L"STATIC", L"Сортировка форматов (-S):", WS_CHILD | SS_LEFT, tabX, tabY + 70, 180, 18, hwnd, NULL, NULL, NULL);
    hEditFormatSort = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                      tabX, tabY + 90, 250, 22, hwnd, NULL, NULL, NULL);
    
    hLblExtractor = CreateWindowW(L"STATIC", L"Extractor Args:", WS_CHILD | SS_LEFT, tabX + 280, tabY, 120, 18, hwnd, NULL, NULL, NULL);
    hEditExtractorArgs = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                         tabX + 280, tabY + 20, 250, 22, hwnd, NULL, NULL, NULL);
    
    hLblHeaders = CreateWindowW(L"STATIC", L"Заголовки (Headers):", WS_CHILD | SS_LEFT, tabX + 280, tabY + 50, 150, 18, hwnd, NULL, NULL, NULL);
    hEditAddHeaders = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                      tabX + 280, tabY + 70, 250, 22, hwnd, NULL, NULL, NULL);
    
    hLblCookies = CreateWindowW(L"STATIC", L"Cookies файл:", WS_CHILD | SS_LEFT, tabX, tabY + 120, 100, 18, hwnd, NULL, NULL, NULL);
    hEditCookiesFile = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY,
                                       tabX + 110, tabY + 120, 280, 22, hwnd, NULL, NULL, NULL);
    hBtnBrowseCookies = CreateWindowW(L"BUTTON", L"...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                      tabX + 395, tabY + 120, 30, 22, hwnd, (HMENU)IDC_BROWSE_COOKIES, NULL, NULL);
    
    hChkNoCache = CreateWindowW(L"BUTTON", L"Отключить кэш (--no-cache-dir)", 
                                WS_CHILD | BS_AUTOCHECKBOX, tabX + 280, tabY + 100, 250, 20, hwnd, 
                                (HMENU)IDC_CHK_NO_CACHE, NULL, NULL);
    
    // Остальные скрытые поля
    hEditConfigLocations = CreateWindowW(L"EDIT", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    hEditDownloader = CreateWindowW(L"EDIT", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    hEditDownloaderArgs = CreateWindowW(L"EDIT", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    hEditBatchFile = CreateWindowW(L"EDIT", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    hEditOutputTemplate = CreateWindowW(L"EDIT", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    hEditCookiesFromBrowser = CreateWindowW(L"EDIT", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    hEditCacheDir = CreateWindowW(L"EDIT", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    hEditRemoveChapters = CreateWindowW(L"EDIT", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    hChkForceKeyframes = CreateWindowW(L"BUTTON", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    
    // Установка шрифта
    EnumChildWindows(hwnd, [](HWND hwndChild, LPARAM lParam) -> BOOL {
        if (hwndChild != hLogEdit) {
            SendMessage(hwndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
        }
        return TRUE;
    }, (LPARAM)hFont);
}


// Переключение вкладок
void SwitchTab(int tabIndex) {
    // Скрыть все элементы всех вкладок
    ShowWindow(hChkIgnoreConfig, SW_HIDE);
    ShowWindow(hChkLiveFromStart, SW_HIDE);
    ShowWindow(hChkSkipDownload, SW_HIDE);
    ShowWindow(hChkSaveJson, SW_HIDE);
    ShowWindow(hChkListFormats, SW_HIDE);
    ShowWindow(hEditProxy, SW_HIDE);
    ShowWindow(hEditWaitForVideo, SW_HIDE);
    ShowWindow(hEditDownloadSections, SW_HIDE);
    ShowWindow(hLblProxy, SW_HIDE);
    ShowWindow(hLblWait, SW_HIDE);
    ShowWindow(hLblSections, SW_HIDE);
    
    ShowWindow(hChkEmbedSubs, SW_HIDE);
    ShowWindow(hChkEmbedThumbnail, SW_HIDE);
    ShowWindow(hChkEmbedMetadata, SW_HIDE);
    ShowWindow(hChkEmbedChapters, SW_HIDE);
    ShowWindow(hChkSplitChapters, SW_HIDE);
    ShowWindow(hChkCheckFormats, SW_HIDE);
    ShowWindow(hEditConvertSubs, SW_HIDE);
    ShowWindow(hEditConvertThumbnails, SW_HIDE);
    ShowWindow(hLblConvSubs, SW_HIDE);
    ShowWindow(hLblConvSubsHint, SW_HIDE);
    ShowWindow(hLblConvThumb, SW_HIDE);
    ShowWindow(hLblConvThumbHint, SW_HIDE);
    
    ShowWindow(hEditMergeFormat, SW_HIDE);
    ShowWindow(hEditFormatSort, SW_HIDE);
    ShowWindow(hEditExtractorArgs, SW_HIDE);
    ShowWindow(hEditAddHeaders, SW_HIDE);
    ShowWindow(hEditCookiesFile, SW_HIDE);
    ShowWindow(hBtnBrowseCookies, SW_HIDE);
    ShowWindow(hChkNoCache, SW_HIDE);
    ShowWindow(hLblMerge, SW_HIDE);
    ShowWindow(hLblMergeHint, SW_HIDE);
    ShowWindow(hLblSort, SW_HIDE);
    ShowWindow(hLblExtractor, SW_HIDE);
    ShowWindow(hLblHeaders, SW_HIDE);
    ShowWindow(hLblCookies, SW_HIDE);
    
    // Показать элементы выбранной вкладки
    switch (tabIndex) {
        case 0: // Основные
            ShowWindow(hChkIgnoreConfig, SW_SHOW);
            ShowWindow(hChkLiveFromStart, SW_SHOW);
            ShowWindow(hChkSkipDownload, SW_SHOW);
            ShowWindow(hChkSaveJson, SW_SHOW);
            ShowWindow(hChkListFormats, SW_SHOW);
            ShowWindow(hEditProxy, SW_SHOW);
            ShowWindow(hEditWaitForVideo, SW_SHOW);
            ShowWindow(hEditDownloadSections, SW_SHOW);
            ShowWindow(hLblProxy, SW_SHOW);
            ShowWindow(hLblWait, SW_SHOW);
            ShowWindow(hLblSections, SW_SHOW);
            break;
        case 1: // Встраивание
            ShowWindow(hChkEmbedSubs, SW_SHOW);
            ShowWindow(hChkEmbedThumbnail, SW_SHOW);
            ShowWindow(hChkEmbedMetadata, SW_SHOW);
            ShowWindow(hChkEmbedChapters, SW_SHOW);
            ShowWindow(hChkSplitChapters, SW_SHOW);
            ShowWindow(hChkCheckFormats, SW_SHOW);
            ShowWindow(hEditConvertSubs, SW_SHOW);
            ShowWindow(hEditConvertThumbnails, SW_SHOW);
            ShowWindow(hLblConvSubs, SW_SHOW);
            ShowWindow(hLblConvSubsHint, SW_SHOW);
            ShowWindow(hLblConvThumb, SW_SHOW);
            ShowWindow(hLblConvThumbHint, SW_SHOW);
            break;
        case 2: // Продвинутые
            ShowWindow(hEditMergeFormat, SW_SHOW);
            ShowWindow(hEditFormatSort, SW_SHOW);
            ShowWindow(hEditExtractorArgs, SW_SHOW);
            ShowWindow(hEditAddHeaders, SW_SHOW);
            ShowWindow(hEditCookiesFile, SW_SHOW);
            ShowWindow(hBtnBrowseCookies, SW_SHOW);
            ShowWindow(hChkNoCache, SW_SHOW);
            ShowWindow(hLblMerge, SW_SHOW);
            ShowWindow(hLblMergeHint, SW_SHOW);
            ShowWindow(hLblSort, SW_SHOW);
            ShowWindow(hLblExtractor, SW_SHOW);
            ShowWindow(hLblHeaders, SW_SHOW);
            ShowWindow(hLblCookies, SW_SHOW);
            break;
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
            
            // Создание вкладок
            CreateTabs(hwnd);
            CreateTabContent(hwnd);
            SwitchTab(0); // Показать первую вкладку
            
            // Кнопка загрузки
            hDownloadBtn = CreateWindowW(L"BUTTON", L"Скачать", 
                                        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                        10, 423, 560, 40, hwnd, (HMENU)IDC_DOWNLOAD, NULL, NULL);
            
            // Прогресс бар
            hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                                          WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
                                          10, 473, 560, 28, hwnd, NULL, NULL, NULL);
            SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
            
            // Статус
            hStatusText = CreateWindowW(L"STATIC", L"Готов к загрузке", 
                                       WS_VISIBLE | WS_CHILD | SS_CENTER,
                                       10, 510, 560, 22, hwnd, NULL, NULL, NULL);
            
            // Кнопка показать/скрыть лог
            hToggleLogBtn = CreateWindowW(L"BUTTON", L"▼ Показать лог", 
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                         10, 540, 560, 28, hwnd, (HMENU)IDC_TOGGLE_LOG, NULL, NULL);
            
            // Лог (скрыт по умолчанию)
            hLogEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                                      WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                                      10, 578, 560, 140, hwnd, NULL, NULL, NULL);
            
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
        
        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            if (pnmhdr->idFrom == IDC_TAB && pnmhdr->code == TCN_SELCHANGE) {
                int tabIndex = TabCtrl_GetCurSel(hTabControl);
                SwitchTab(tabIndex);
            }
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_DOWNLOAD && HIWORD(wParam) == BN_CLICKED) {
                if (!isDownloading) {
                    isDownloading = true;
                    EnableWindow(hDownloadBtn, FALSE);
                    SetWindowTextW(hDownloadBtn, L"Загрузка...");
                    
                    // Сохранение настроек перед загрузкой
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
                    currentPreset = presetName;
                    
                    Settings presetSettings;
                    presetSettings.Load(presetName);
                    settings = presetSettings;
                    LoadSettingsToUI();
                } else {
                    currentPreset.clear();
                    settings.Load();
                    LoadSettingsToUI();
                }
            }
            else if (LOWORD(wParam) == IDC_SAVE_PRESET && HIWORD(wParam) == BN_CLICKED) {
    // Создаем простой диалог для ввода имени
    HWND hDialog = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"STATIC",
        L"Сохранить пресет",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 400, 150,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (hDialog) {
        // Центрируем диалог
        RECT rcParent, rcDlg;
        GetWindowRect(hwnd, &rcParent);
        GetWindowRect(hDialog, &rcDlg);
        int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
        int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;
        SetWindowPos(hDialog, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        
        // Создаем элементы диалога
        HFONT hDlgFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        
        HWND hLabel = CreateWindowW(L"STATIC", L"Введите имя пресета:",
                                   WS_VISIBLE | WS_CHILD | SS_LEFT,
                                   20, 20, 360, 20, hDialog, NULL, NULL, NULL);
        SendMessage(hLabel, WM_SETFONT, (WPARAM)hDlgFont, TRUE);
        
        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                    WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                    20, 45, 360, 26, hDialog, (HMENU)100, NULL, NULL);
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hDlgFont, TRUE);
        SetFocus(hEdit);
        
        HWND hBtnOk = CreateWindowW(L"BUTTON", L"Сохранить",
                                   WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                   170, 85, 100, 32, hDialog, (HMENU)IDOK, NULL, NULL);
        SendMessage(hBtnOk, WM_SETFONT, (WPARAM)hDlgFont, TRUE);
        
        HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Отмена",
                                       WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                       280, 85, 100, 32, hDialog, (HMENU)IDCANCEL, NULL, NULL);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hDlgFont, TRUE);
        
        // Простой цикл сообщений
        MSG msg;
        bool dialogRunning = true;
        bool saved = false;
        wchar_t presetName[256] = L"";
        
        EnableWindow(hwnd, FALSE); // Блокируем главное окно
        
        while (dialogRunning && GetMessage(&msg, NULL, 0, 0)) {
            if (msg.hwnd == hDialog || IsChild(hDialog, msg.hwnd)) {
                if (msg.message == WM_COMMAND) {
                    if (LOWORD(msg.wParam) == IDOK) {
                        GetWindowTextW(hEdit, presetName, 256);
                        if (wcslen(presetName) > 0) {
                            saved = true;
                            dialogRunning = false;
                        } else {
                            MessageBoxW(hDialog, L"Введите имя пресета!", L"Ошибка", MB_ICONWARNING);
                        }
                    } else if (LOWORD(msg.wParam) == IDCANCEL) {
                        dialogRunning = false;
                    }
                } else if (msg.message == WM_CLOSE) {
                    dialogRunning = false;
                }
                
                if (!IsDialogMessage(hDialog, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        
        EnableWindow(hwnd, TRUE); // Разблокируем главное окно
        SetFocus(hwnd);
        DestroyWindow(hDialog);
        DeleteObject(hDlgFont);
        
        // Сохраняем пресет
        if (saved && wcslen(presetName) > 0) {
            SaveSettingsFromUI();
            settings.Save(presetName);
            LoadPresetsToCombo();
            
            // Выбираем сохраненный пресет
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
            else if (LOWORD(wParam) == IDC_BROWSE_COOKIES && HIWORD(wParam) == BN_CLICKED) {
                BrowseFile(hEditCookiesFile, L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0", L"Выберите файл cookies");
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
            // Сохранение настроек при закрытии
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
    
    // Добавлен WS_THICKFRAME и WS_MAXIMIZEBOX для изменения размера
    hMainWindow = CreateWindowExW(
        0,
        CLASS_NAME,
        L"yt-dlp GUI - Загрузчик видео",
        WS_OVERLAPPEDWINDOW, // Вместо WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX
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
