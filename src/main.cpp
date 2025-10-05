#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <codecvt>
#include <locale>
#include <regex>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shlwapi.lib")

// Global variables
HINSTANCE hInst;
HWND hMainWnd;
HWND hUrlEdit, hOutputEdit, hProgress, hOutputList;
HWND hTabCtrl, hSettingsTab, hDownloadTab;
HWND hIgnoreConfig, hLiveFromStart, hNoCache, hSkipDownload, hEmbedSubs, hEmbedThumbnail, hEmbedMetadata, hEmbedChapters, hCheckFormats;
HWND hProxyEdit, hBatchFileEdit, hCookiesFileEdit, hCacheDirEdit, hOutputTemplateEdit, hMergeFormatCombo;
HWND hConvertSubsCombo, hConvertThumbnailsCombo, hDownloaderCombo, hDownloaderArgsEdit;
HWND hCookiesFromBrowserCombo, hConfigLocationsEdit, hDownloadSectionsEdit;
HWND hOutputBtn, hBatchFileBtn, hCookiesFileBtn, hCacheDirBtn;
HWND hOutputPanel, hOutputToggleBtn;
HWND hFormatListBtn, hJsonBtn, hSplitChapters, hForceKeyframes;
HWND hRemoveChaptersEdit;

std::wstring settingsPath;
std::wstring currentOutputDir = L"";
std::wstring currentOutputText = L"";
std::wstring lastDownloadUrl = L"";
std::vector<std::wstring> recentUrls;
bool outputPanelVisible = false;

// Settings structure
struct Settings {
    std::wstring outputDir = L"";
    std::vector<std::wstring> recentUrls;
    std::wstring proxy = L"";
    std::wstring batchFile = L"";
    std::wstring cookiesFile = L"";
    std::wstring cacheDir = L"";
    std::wstring outputTemplate = L"";
    std::wstring mergeFormat = L"mp4";
    std::wstring convertSubs = L"none";
    std::wstring convertThumbnails = L"none";
    std::wstring downloader = L"native";
    std::wstring downloaderArgs = L"";
    std::wstring cookiesFromBrowser = L"";
    std::wstring configLocations = L"";
    std::wstring downloadSections = L"";
    
    bool ignoreConfig = false;
    bool liveFromStart = false;
    bool noCache = false;
    bool skipDownload = false;
    bool embedSubs = false;
    bool embedThumbnail = false;
    bool embedMetadata = false;
    bool embedChapters = false;
    bool checkFormats = false;
    bool splitChapters = false;
    bool forceKeyframes = false;
} settings;

// Convert UTF-8 string to wide string
std::wstring utf8_to_wstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Convert wide string to UTF-8
std::string wstring_to_utf8(const std::wstring& str) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Execute yt-dlp command
void executeYtdlp(const std::wstring& cmd) {
    // Clear output panel
    SendMessage(hOutputList, LB_RESETCONTENT, 0, 0);
    
    // Create temporary batch file to execute yt-dlp
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring batchFile = std::wstring(tempPath) + L"yt_dlp_temp.bat";
    
    std::wofstream batch(batchFile);
    batch << L"@echo off\n";
    batch << L"chcp 65001 >nul\n"; // Set UTF-8 code page
    batch << cmd << L"\n";
    batch.close();
    
    // Create process
    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    if (CreateProcessW(NULL, const_cast<LPWSTR>(batchFile.c_str()), NULL, NULL, FALSE, 
                       CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        // Read output from a temporary file (yt-dlp output redirected)
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    
    // Delete temporary batch file
    DeleteFileW(batchFile.c_str());
}

// Save settings to file
void saveSettings() {
    std::wofstream file(settingsPath);
    if (!file.is_open()) return;
    
    file << L"outputDir=" << settings.outputDir << L"\n";
    file << L"recentUrls=" << recentUrls.size() << L"\n";
    for (const auto& url : recentUrls) {
        file << url << L"\n";
    }
    file << L"proxy=" << settings.proxy << L"\n";
    file << L"batchFile=" << settings.batchFile << L"\n";
    file << L"cookiesFile=" << settings.cookiesFile << L"\n";
    file << L"cacheDir=" << settings.cacheDir << L"\n";
    file << L"outputTemplate=" << settings.outputTemplate << L"\n";
    file << L"mergeFormat=" << settings.mergeFormat << L"\n";
    file << L"convertSubs=" << settings.convertSubs << L"\n";
    file << L"convertThumbnails=" << settings.convertThumbnails << L"\n";
    file << L"downloader=" << settings.downloader << L"\n";
    file << L"downloaderArgs=" << settings.downloaderArgs << L"\n";
    file << L"cookiesFromBrowser=" << settings.cookiesFromBrowser << L"\n";
    file << L"configLocations=" << settings.configLocations << L"\n";
    file << L"downloadSections=" << settings.downloadSections << L"\n";
    
    file << L"ignoreConfig=" << (settings.ignoreConfig ? L"1" : L"0") << L"\n";
    file << L"liveFromStart=" << (settings.liveFromStart ? L"1" : L"0") << L"\n";
    file << L"noCache=" << (settings.noCache ? L"1" : L"0") << L"\n";
    file << L"skipDownload=" << (settings.skipDownload ? L"1" : L"0") << L"\n";
    file << L"embedSubs=" << (settings.embedSubs ? L"1" : L"0") << L"\n";
    file << L"embedThumbnail=" << (settings.embedThumbnail ? L"1" : L"0") << L"\n";
    file << L"embedMetadata=" << (settings.embedMetadata ? L"1" : L"0") << L"\n";
    file << L"embedChapters=" << (settings.embedChapters ? L"1" : L"0") << L"\n";
    file << L"checkFormats=" << (settings.checkFormats ? L"1" : L"0") << L"\n";
    file << L"splitChapters=" << (settings.splitChapters ? L"1" : L"0") << L"\n";
    file << L"forceKeyframes=" << (settings.forceKeyframes ? L"1" : L"0") << L"\n";
    
    file.close();
}

// Load settings from file
void loadSettings() {
    std::wifstream file(settingsPath);
    if (!file.is_open()) return;
    
    std::wstring line;
    std::getline(file, line);
    if (line.substr(0, 10) == L"outputDir=") {
        settings.outputDir = line.substr(10);
    }
    
    std::getline(file, line);
    if (line.substr(0, 12) == L"recentUrls=") {
        int count = std::stoi(line.substr(12));
        for (int i = 0; i < count; i++) {
            std::getline(file, line);
            recentUrls.push_back(line);
        }
    }
    
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == L"proxy=") {
            settings.proxy = line.substr(6);
        } else if (line.substr(0, 10) == L"batchFile=") {
            settings.batchFile = line.substr(10);
        } else if (line.substr(0, 12) == L"cookiesFile=") {
            settings.cookiesFile = line.substr(12);
        } else if (line.substr(0, 9) == L"cacheDir=") {
            settings.cacheDir = line.substr(9);
        } else if (line.substr(0, 15) == L"outputTemplate=") {
            settings.outputTemplate = line.substr(15);
        } else if (line.substr(0, 12) == L"mergeFormat=") {
            settings.mergeFormat = line.substr(12);
        } else if (line.substr(0, 12) == L"convertSubs=") {
            settings.convertSubs = line.substr(12);
        } else if (line.substr(0, 17) == L"convertThumbnails=") {
            settings.convertThumbnails = line.substr(17);
        } else if (line.substr(0, 11) == L"downloader=") {
            settings.downloader = line.substr(11);
        } else if (line.substr(0, 14) == L"downloaderArgs=") {
            settings.downloaderArgs = line.substr(14);
        } else if (line.substr(0, 19) == L"cookiesFromBrowser=") {
            settings.cookiesFromBrowser = line.substr(19);
        } else if (line.substr(0, 16) == L"configLocations=") {
            settings.configLocations = line.substr(16);
        } else if (line.substr(0, 17) == L"downloadSections=") {
            settings.downloadSections = line.substr(17);
        } else if (line.substr(0, 13) == L"ignoreConfig=") {
            settings.ignoreConfig = (line.substr(13) == L"1");
        } else if (line.substr(0, 14) == L"liveFromStart=") {
            settings.liveFromStart = (line.substr(14) == L"1");
        } else if (line.substr(0, 9) == L"noCache=") {
            settings.noCache = (line.substr(8) == L"1");
        } else if (line.substr(0, 13) == L"skipDownload=") {
            settings.skipDownload = (line.substr(13) == L"1");
        } else if (line.substr(0, 10) == L"embedSubs=") {
            settings.embedSubs = (line.substr(10) == L"1");
        } else if (line.substr(0, 15) == L"embedThumbnail=") {
            settings.embedThumbnail = (line.substr(15) == L"1");
        } else if (line.substr(0, 13) == L"embedMetadata=") {
            settings.embedMetadata = (line.substr(13) == L"1");
        } else if (line.substr(0, 14) == L"embedChapters=") {
            settings.embedChapters = (line.substr(14) == L"1");
        } else if (line.substr(0, 12) == L"checkFormats=") {
            settings.checkFormats = (line.substr(12) == L"1");
        } else if (line.substr(0, 15) == L"splitChapters=") {
            settings.splitChapters = (line.substr(15) == L"1");
        } else if (line.substr(0, 15) == L"forceKeyframes=") {
            settings.forceKeyframes = (line.substr(15) == L"1");
        }
    }
    
    file.close();
}

// Update UI based on loaded settings
void updateUIWithSettings() {
    if (!settings.outputDir.empty()) {
        SetWindowTextW(hOutputEdit, settings.outputDir.c_str());
    }
    
    if (!recentUrls.empty()) {
        SetWindowTextW(hUrlEdit, recentUrls[0].c_str());
    }
    
    SetWindowTextW(hProxyEdit, settings.proxy.c_str());
    SetWindowTextW(hBatchFileEdit, settings.batchFile.c_str());
    SetWindowTextW(hCookiesFileEdit, settings.cookiesFile.c_str());
    SetWindowTextW(hCacheDirEdit, settings.cacheDir.c_str());
    SetWindowTextW(hOutputTemplateEdit, settings.outputTemplate.c_str());
    
    // Set combo box selections
    SendMessageW(hMergeFormatCombo, CB_SELECTSTRING, -1, (LPARAM)settings.mergeFormat.c_str());
    SendMessageW(hConvertSubsCombo, CB_SELECTSTRING, -1, (LPARAM)settings.convertSubs.c_str());
    SendMessageW(hConvertThumbnailsCombo, CB_SELECTSTRING, -1, (LPARAM)settings.convertThumbnails.c_str());
    SendMessageW(hDownloaderCombo, CB_SELECTSTRING, -1, (LPARAM)settings.downloader.c_str());
    SendMessageW(hCookiesFromBrowserCombo, CB_SELECTSTRING, -1, (LPARAM)settings.cookiesFromBrowser.c_str());
    
    SetWindowTextW(hDownloaderArgsEdit, settings.downloaderArgs.c_str());
    SetWindowTextW(hConfigLocationsEdit, settings.configLocations.c_str());
    SetWindowTextW(hDownloadSectionsEdit, settings.downloadSections.c_str());
    SetWindowTextW(hRemoveChaptersEdit, settings.downloadSections.c_str());
    
    // Set checkbox states
    Button_SetCheck(hIgnoreConfig, settings.ignoreConfig ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hLiveFromStart, settings.liveFromStart ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hNoCache, settings.noCache ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hSkipDownload, settings.skipDownload ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hEmbedSubs, settings.embedSubs ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hEmbedThumbnail, settings.embedThumbnail ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hEmbedMetadata, settings.embedMetadata ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hEmbedChapters, settings.embedChapters ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hCheckFormats, settings.checkFormats ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hSplitChapters, settings.splitChapters ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(hForceKeyframes, settings.forceKeyframes ? BST_CHECKED : BST_UNCHECKED);
}

// Build command line from UI options
std::wstring buildCommandLine() {
    std::wstring cmd = L"yt-dlp.exe ";
    
    // Add options based on UI state
    if (Button_GetCheck(hIgnoreConfig) == BST_CHECKED) {
        cmd += L"--ignore-config ";
    }
    
    if (!GetWindowTextLengthW(hConfigLocationsEdit) == 0) {
        std::wstring configLoc;
        int len = GetWindowTextLengthW(hConfigLocationsEdit);
        configLoc.resize(len);
        GetWindowTextW(hConfigLocationsEdit, &configLoc[0], len + 1);
        cmd += L"--config-locations \"" + configLoc + L"\" ";
    }
    
    if (Button_GetCheck(hLiveFromStart) == BST_CHECKED) {
        cmd += L"--live-from-start ";
    }
    
    if (!GetWindowTextLengthW(hProxyEdit) == 0) {
        std::wstring proxy;
        int len = GetWindowTextLengthW(hProxyEdit);
        proxy.resize(len);
        GetWindowTextW(hProxyEdit, &proxy[0], len + 1);
        cmd += L"--proxy \"" + proxy + L"\" ";
    }
    
    if (!GetWindowTextLengthW(hDownloaderCombo) == 0) {
        std::wstring downloader;
        int len = GetWindowTextLengthW(hDownloaderCombo);
        downloader.resize(len);
        GetWindowTextW(hDownloaderCombo, &downloader[0], len + 1);
        cmd += L"--downloader " + downloader + L" ";
    }
    
    if (!GetWindowTextLengthW(hDownloaderArgsEdit) == 0) {
        std::wstring args;
        int len = GetWindowTextLengthW(hDownloaderArgsEdit);
        args.resize(len);
        GetWindowTextW(hDownloaderArgsEdit, &args[0], len + 1);
        cmd += L"--downloader-args \"" + args + L"\" ";
    }
    
    if (!GetWindowTextLengthW(hBatchFileEdit) == 0) {
        std::wstring batchFile;
        int len = GetWindowTextLengthW(hBatchFileEdit);
        batchFile.resize(len);
        GetWindowTextW(hBatchFileEdit, &batchFile[0], len + 1);
        cmd += L"-a \"" + batchFile + L"\" ";
    }
    
    if (!GetWindowTextLengthW(hOutputTemplateEdit) == 0) {
        std::wstring templateStr;
        int len = GetWindowTextLengthW(hOutputTemplateEdit);
        templateStr.resize(len);
        GetWindowTextW(hOutputTemplateEdit, &templateStr[0], len + 1);
        cmd += L"-o \"" + templateStr + L"\" ";
    }
    
    if (!GetWindowTextLengthW(hCookiesFileEdit) == 0) {
        std::wstring cookiesFile;
        int len = GetWindowTextLengthW(hCookiesFileEdit);
        cookiesFile.resize(len);
        GetWindowTextW(hCookiesFileEdit, &cookiesFile[0], len + 1);
        cmd += L"--cookies \"" + cookiesFile + L"\" ";
    }
    
    if (!GetWindowTextLengthW(hCookiesFromBrowserCombo) == 0) {
        std::wstring browser;
        int len = GetWindowTextLengthW(hCookiesFromBrowserCombo);
        browser.resize(len);
        GetWindowTextW(hCookiesFromBrowserCombo, &browser[0], len + 1);
        cmd += L"--cookies-from-browser " + browser + L" ";
    }
    
    if (!GetWindowTextLengthW(hCacheDirEdit) == 0) {
        std::wstring cacheDir;
        int len = GetWindowTextLengthW(hCacheDirEdit);
        cacheDir.resize(len);
        GetWindowTextW(hCacheDirEdit, &cacheDir[0], len + 1);
        cmd += L"--cache-dir \"" + cacheDir + L"\" ";
    }
    
    if (Button_GetCheck(hNoCache) == BST_CHECKED) {
        cmd += L"--no-cache-dir ";
    }
    
    if (Button_GetCheck(hSkipDownload) == BST_CHECKED) {
        cmd += L"--skip-download ";
    }
    
    if (Button_GetCheck(hEmbedSubs) == BST_CHECKED) {
        cmd += L"--embed-subs ";
    }
    
    if (Button_GetCheck(hEmbedThumbnail) == BST_CHECKED) {
        cmd += L"--embed-thumbnail ";
    }
    
    if (Button_GetCheck(hEmbedMetadata) == BST_CHECKED) {
        cmd += L"--embed-metadata ";
    }
    
    if (Button_GetCheck(hEmbedChapters) == BST_CHECKED) {
        cmd += L"--embed-chapters ";
    }
    
    if (Button_GetCheck(hCheckFormats) == BST_CHECKED) {
        cmd += L"--check-formats ";
    }
    
    if (!GetWindowTextLengthW(hMergeFormatCombo) == 0) {
        std::wstring format;
        int len = GetWindowTextLengthW(hMergeFormatCombo);
        format.resize(len);
        GetWindowTextW(hMergeFormatCombo, &format[0], len + 1);
        cmd += L"--merge-output-format " + format + L" ";
    }
    
    if (!GetWindowTextLengthW(hConvertSubsCombo) == 0) {
        std::wstring format;
        int len = GetWindowTextLengthW(hConvertSubsCombo);
        format.resize(len);
        GetWindowTextW(hConvertSubsCombo, &format[0], len + 1);
        cmd += L"--convert-subs " + format + L" ";
    }
    
    if (!GetWindowTextLengthW(hConvertThumbnailsCombo) == 0) {
        std::wstring format;
        int len = GetWindowTextLengthW(hConvertThumbnailsCombo);
        format.resize(len);
        GetWindowTextW(hConvertThumbnailsCombo, &format[0], len + 1);
        cmd += L"--convert-thumbnails " + format + L" ";
    }
    
    if (Button_GetCheck(hSplitChapters) == BST_CHECKED) {
        cmd += L"--split-chapters ";
    }
    
    if (!GetWindowTextLengthW(hRemoveChaptersEdit) == 0) {
        std::wstring regex;
        int len = GetWindowTextLengthW(hRemoveChaptersEdit);
        regex.resize(len);
        GetWindowTextW(hRemoveChaptersEdit, &regex[0], len + 1);
        cmd += L"--remove-chapters \"" + regex + L"\" ";
    }
    
    if (Button_GetCheck(hForceKeyframes) == BST_CHECKED) {
        cmd += L"--force-keyframes-at-cuts ";
    }
    
    // Add URL
    std::wstring url;
    int len = GetWindowTextLengthW(hUrlEdit);
    url.resize(len);
    GetWindowTextW(hUrlEdit, &url[0], len + 1);
    
    cmd += L"\"" + url + L"\"";
    
    return cmd;
}

// Handle download button click
void onDownloadClick() {
    std::wstring cmd = buildCommandLine();
    executeYtdlp(cmd);
}

// Toggle output panel visibility
void toggleOutputPanel() {
    outputPanelVisible = !outputPanelVisible;
    ShowWindow(hOutputPanel, outputPanelVisible ? SW_SHOW : SW_HIDE);
    
    // Update button text
    SetWindowTextW(hOutputToggleBtn, outputPanelVisible ? L"▲ Hide Output" : L"▼ Show Output");
    
    // Trigger resize to adjust layout
    RECT rc;
    GetClientRect(hMainWnd, &rc);
    SendMessage(hMainWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
}

// Initialize common controls
void initCommonControls() {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
}

// Create main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
    {
        initCommonControls();
        
        // Create tab control
        hTabCtrl = CreateWindowW(WC_TABCONTROL, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                 0, 0, 0, 0, hWnd, NULL, hInst, NULL);
        
        TCITEMW tie;
        tie.mask = TCIF_TEXT;
        
        tie.pszText = L"Download";
        TabCtrl_InsertItemW(hTabCtrl, 0, &tie);
        
        tie.pszText = L"Settings";
        TabCtrl_InsertItemW(hTabCtrl, 1, &tie);
        
        // Create download tab
        RECT rcClient;
        GetClientRect(hTabCtrl, &rcClient);
        MapWindowPoints(hTabCtrl, hWnd, (LPPOINT)&rcClient, 2);
        
        hDownloadTab = CreateWindowW(WC_STATIC, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                     rcClient.left + 10, rcClient.top + 30, 
                                     rcClient.right - rcClient.left - 20, 
                                     rcClient.bottom - rcClient.top - 40,
                                     hTabCtrl, NULL, hInst, NULL);
        
        // Create settings tab
        hSettingsTab = CreateWindowW(WC_STATIC, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                     rcClient.left + 10, rcClient.top + 30, 
                                     rcClient.right - rcClient.left - 20, 
                                     rcClient.bottom - rcClient.top - 40,
                                     hTabCtrl, NULL, hInst, NULL);
        
        // Download tab controls
        CreateWindowW(WC_STATIC, L"URL:", WS_CHILD | WS_VISIBLE,
                      20, 20, 80, 25, hDownloadTab, NULL, hInst, NULL);
        
        hUrlEdit = CreateWindowW(WC_EDIT, L"", 
                                 WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
                                 20, 50, 500, 80, hDownloadTab, NULL, hInst, NULL);
        
        hOutputBtn = CreateWindowW(WC_BUTTON, L"Browse Output Folder", 
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                   20, 140, 150, 30, hDownloadTab, (HMENU)101, hInst, NULL);
        
        hOutputEdit = CreateWindowW(WC_EDIT, L"", 
                                    WS_CHILD | WS_VISIBLE | WS_BORDER,
                                    180, 140, 340, 30, hDownloadTab, NULL, hInst, NULL);
        
        CreateWindowW(WC_BUTTON, L"Download", 
                      WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                      20, 180, 100, 30, hDownloadTab, (HMENU)102, hInst, NULL);
        
        hFormatListBtn = CreateWindowW(WC_BUTTON, L"List Formats", 
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       130, 180, 100, 30, hDownloadTab, (HMENU)103, hInst, NULL);
        
        hJsonBtn = CreateWindowW(WC_BUTTON, L"Get JSON", 
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                 240, 180, 100, 30, hDownloadTab, (HMENU)104, hInst, NULL);
        
        hProgress = CreateWindowW(PROGRESS_CLASSW, NULL,
                                  WS_CHILD | WS_VISIBLE,
                                  20, 220, 500, 20, hDownloadTab, NULL, hInst, NULL);
        
        // Output toggle button
        hOutputToggleBtn = CreateWindowW(WC_BUTTON, L"▼ Show Output", 
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                         20, 250, 150, 30, hDownloadTab, (HMENU)105, hInst, NULL);
        
        // Output panel (initially hidden)
        hOutputPanel = CreateWindowW(WC_STATIC, L"", 
                                     WS_CHILD | WS_VISIBLE | WS_BORDER,
                                     20, 290, 500, 150, hDownloadTab, NULL, hInst, NULL);
        
        hOutputList = CreateWindowW(WC_LISTBOX, L"", 
                                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
                                    5, 5, 490, 140, hOutputPanel, NULL, hInst, NULL);
        
        ShowWindow(hOutputPanel, SW_HIDE);
        
        // Settings tab controls
        int y = 20;
        hIgnoreConfig = CreateWindowW(WC_BUTTON, L"--ignore-config", 
                                      BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--config-locations:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hConfigLocationsEdit = CreateWindowW(WC_EDIT, L"", 
                                             WS_CHILD | WS_VISIBLE | WS_BORDER,
                                             180, y, 300, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        hLiveFromStart = CreateWindowW(WC_BUTTON, L"--live-from-start", 
                                       BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--proxy:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hProxyEdit = CreateWindowW(WC_EDIT, L"", 
                                   WS_CHILD | WS_VISIBLE | WS_BORDER,
                                   180, y, 300, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--downloader:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hDownloaderCombo = CreateWindowW(WC_COMBOBOX, L"", 
                                         CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                                         180, y, 150, 200, hSettingsTab, NULL, hInst, NULL);
        
        SendMessageW(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"native");
        SendMessageW(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"aria2c");
        SendMessageW(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"axel");
        SendMessageW(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"curl");
        SendMessageW(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"ffmpeg");
        SendMessageW(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"httpie");
        SendMessageW(hDownloaderCombo, CB_ADDSTRING, 0, (LPARAM)L"wget");
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--downloader-args:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hDownloaderArgsEdit = CreateWindowW(WC_EDIT, L"", 
                                            WS_CHILD | WS_VISIBLE | WS_BORDER,
                                            180, y, 300, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"-a, --batch-file:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hBatchFileEdit = CreateWindowW(WC_EDIT, L"", 
                                       WS_CHILD | WS_VISIBLE | WS_BORDER,
                                       180, y, 250, 25, hSettingsTab, NULL, hInst, NULL);
        
        hBatchFileBtn = CreateWindowW(WC_BUTTON, L"Browse", 
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      440, y, 40, 25, hSettingsTab, (HMENU)106, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"-o, --output:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hOutputTemplateEdit = CreateWindowW(WC_EDIT, L"", 
                                            WS_CHILD | WS_VISIBLE | WS_BORDER,
                                            180, y, 300, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--cookies:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hCookiesFileEdit = CreateWindowW(WC_EDIT, L"", 
                                         WS_CHILD | WS_VISIBLE | WS_BORDER,
                                         180, y, 250, 25, hSettingsTab, NULL, hInst, NULL);
        
        hCookiesFileBtn = CreateWindowW(WC_BUTTON, L"Browse", 
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                        440, y, 40, 25, hSettingsTab, (HMENU)107, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--cookies-from-browser:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 180, 25, hSettingsTab, NULL, hInst, NULL);
        
        hCookiesFromBrowserCombo = CreateWindowW(WC_COMBOBOX, L"", 
                                                 CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                                                 180, y, 150, 200, hSettingsTab, NULL, hInst, NULL);
        
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"brave");
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"chrome");
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"chromium");
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"edge");
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"firefox");
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"opera");
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"safari");
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"vivaldi");
        SendMessageW(hCookiesFromBrowserCombo, CB_ADDSTRING, 0, (LPARAM)L"whale");
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--cache-dir:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hCacheDirEdit = CreateWindowW(WC_EDIT, L"", 
                                      WS_CHILD | WS_VISIBLE | WS_BORDER,
                                      180, y, 250, 25, hSettingsTab, NULL, hInst, NULL);
        
        hCacheDirBtn = CreateWindowW(WC_BUTTON, L"Browse", 
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                     440, y, 40, 25, hSettingsTab, (HMENU)108, hInst, NULL);
        
        y += 30;
        hNoCache = CreateWindowW(WC_BUTTON, L"--no-cache-dir", 
                                 BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                 20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        hSkipDownload = CreateWindowW(WC_BUTTON, L"--skip-download", 
                                      BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--merge-output-format:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 180, 25, hSettingsTab, NULL, hInst, NULL);
        
        hMergeFormatCombo = CreateWindowW(WC_COMBOBOX, L"", 
                                          CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                                          180, y, 100, 200, hSettingsTab, NULL, hInst, NULL);
        
        SendMessageW(hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"avi");
        SendMessageW(hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"flv");
        SendMessageW(hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mkv");
        SendMessageW(hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mov");
        SendMessageW(hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"mp4");
        SendMessageW(hMergeFormatCombo, CB_ADDSTRING, 0, (LPARAM)L"webm");
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--convert-subs:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hConvertSubsCombo = CreateWindowW(WC_COMBOBOX, L"", 
                                          CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                                          180, y, 100, 200, hSettingsTab, NULL, hInst, NULL);
        
        SendMessageW(hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"ass");
        SendMessageW(hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"lrc");
        SendMessageW(hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"srt");
        SendMessageW(hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"vtt");
        SendMessageW(hConvertSubsCombo, CB_ADDSTRING, 0, (LPARAM)L"none");
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--convert-thumbnails:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 180, 25, hSettingsTab, NULL, hInst, NULL);
        
        hConvertThumbnailsCombo = CreateWindowW(WC_COMBOBOX, L"", 
                                                CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                                                180, y, 100, 200, hSettingsTab, NULL, hInst, NULL);
        
        SendMessageW(hConvertThumbnailsCombo, CB_ADDSTRING, 0, (LPARAM)L"jpg");
        SendMessageW(hConvertThumbnailsCombo, CB_ADDSTRING, 0, (LPARAM)L"png");
        SendMessageW(hConvertThumbnailsCombo, CB_ADDSTRING, 0, (LPARAM)L"webp");
        SendMessageW(hConvertThumbnailsCombo, CB_ADDSTRING, 0, (LPARAM)L"none");
        
        y += 30;
        hEmbedSubs = CreateWindowW(WC_BUTTON, L"--embed-subs", 
                                   BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                   20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        hEmbedThumbnail = CreateWindowW(WC_BUTTON, L"--embed-thumbnail", 
                                        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                        20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        hEmbedMetadata = CreateWindowW(WC_BUTTON, L"--embed-metadata", 
                                       BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        hEmbedChapters = CreateWindowW(WC_BUTTON, L"--embed-chapters", 
                                       BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        hCheckFormats = CreateWindowW(WC_BUTTON, L"--check-formats", 
                                      BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        hSplitChapters = CreateWindowW(WC_BUTTON, L"--split-chapters", 
                                       BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       20, y, 200, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        CreateWindowW(WC_STATIC, L"--remove-chapters:", 
                      WS_CHILD | WS_VISIBLE,
                      20, y, 150, 25, hSettingsTab, NULL, hInst, NULL);
        
        hRemoveChaptersEdit = CreateWindowW(WC_EDIT, L"", 
                                            WS_CHILD | WS_VISIBLE | WS_BORDER,
                                            180, y, 300, 25, hSettingsTab, NULL, hInst, NULL);
        
        y += 30;
        hForceKeyframes = CreateWindowW(WC_BUTTON, L"--force-keyframes-at-cuts", 
                                        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                        20, y, 250, 25, hSettingsTab, NULL, hInst, NULL);
        
        // Load settings
        loadSettings();
        updateUIWithSettings();
        
        // Set default output directory to Desktop
        if (settings.outputDir.empty()) {
            PWSTR desktopPath;
            SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &desktopPath);
            settings.outputDir = desktopPath;
            CoTaskMemFree(desktopPath);
            SetWindowTextW(hOutputEdit, settings.outputDir.c_str());
        }
        
        break;
    }
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        HWND hwndCtl = (HWND)lParam;
        
        switch (id) {
        case 101: // Browse Output Folder
        {
            BROWSEINFOW bi = {0};
            bi.hwndOwner = hWnd;
            bi.lpszTitle = L"Select Output Directory";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl != 0) {
                wchar_t path[MAX_PATH];
                SHGetPathFromIDListW(pidl, path);
                SetWindowTextW(hOutputEdit, path);
                settings.outputDir = path;
                CoTaskMemFree(pidl);
            }
            break;
        }
        case 102: // Download
            onDownloadClick();
            break;
        case 103: // List Formats
        {
            std::wstring url;
            int len = GetWindowTextLengthW(hUrlEdit);
            url.resize(len);
            GetWindowTextW(hUrlEdit, &url[0], len + 1);
            
            std::wstring cmd = L"yt-dlp.exe -F \"" + url + L"\"";
            executeYtdlp(cmd);
            break;
        }
        case 104: // Get JSON
        {
            std::wstring url;
            int len = GetWindowTextLengthW(hUrlEdit);
            url.resize(len);
            GetWindowTextW(hUrlEdit, &url[0], len + 1);
            
            std::wstring cmd = L"yt-dlp.exe -J \"" + url + L"\"";
            executeYtdlp(cmd);
            break;
        }
        case 105: // Toggle Output Panel
            toggleOutputPanel();
            break;
        case 106: // Browse Batch File
        {
            OPENFILENAMEW ofn = {0};
            wchar_t fileName[MAX_PATH] = {0};
            
            ofn.lStructSize = sizeof(OPENFILENAMEW);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            
            if (GetOpenFileNameW(&ofn)) {
                SetWindowTextW(hBatchFileEdit, fileName);
                settings.batchFile = fileName;
            }
            break;
        }
        case 107: // Browse Cookies File
        {
            OPENFILENAMEW ofn = {0};
            wchar_t fileName[MAX_PATH] = {0};
            
            ofn.lStructSize = sizeof(OPENFILENAMEW);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"Cookie Files\0*.txt\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            
            if (GetOpenFileNameW(&ofn)) {
                SetWindowTextW(hCookiesFileEdit, fileName);
                settings.cookiesFile = fileName;
            }
            break;
        }
        case 108: // Browse Cache Directory
        {
            BROWSEINFOW bi = {0};
            bi.hwndOwner = hWnd;
            bi.lpszTitle = L"Select Cache Directory";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl != 0) {
                wchar_t path[MAX_PATH];
                SHGetPathFromIDListW(pidl, path);
                SetWindowTextW(hCacheDirEdit, path);
                settings.cacheDir = path;
                CoTaskMemFree(pidl);
            }
            break;
        }
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR* pnmh = (NMHDR*)lParam;
        if (pnmh->hwndFrom == hTabCtrl && pnmh->code == TCN_SELCHANGE) {
            int tabIndex = TabCtrl_GetCurSel(hTabCtrl);
            ShowWindow(hDownloadTab, tabIndex == 0 ? SW_SHOW : SW_HIDE);
            ShowWindow(hSettingsTab, tabIndex == 1 ? SW_SHOW : SW_HIDE);
        }
        break;
    }
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        
        // Resize tab control
        MoveWindow(hTabCtrl, 0, 0, width, height, TRUE);
        
        // Get tab control client area
        RECT rcTab;
        GetClientRect(hTabCtrl, &rcTab);
        MapWindowPoints(hTabCtrl, hWnd, (LPPOINT)&rcTab, 2);
        
        // Resize tabs
        MoveWindow(hDownloadTab, rcTab.left + 10, rcTab.top + 30, 
                   rcTab.right - rcTab.left - 20, rcTab.bottom - rcTab.top - 40, TRUE);
        MoveWindow(hSettingsTab, rcTab.left + 10, rcTab.top + 30, 
                   rcTab.right - rcTab.left - 20, rcTab.bottom - rcTab.top - 40, TRUE);
        
        // Resize controls in download tab
        MoveWindow(hUrlEdit, 20, 50, width - 60, 80, TRUE);
        MoveWindow(hOutputEdit, 180, 140, width - 210, 30, TRUE);
        MoveWindow(hProgress, 20, 220, width - 40, 20, TRUE);
        
        // Resize output panel if visible
        if (outputPanelVisible) {
            MoveWindow(hOutputPanel, 20, 290, width - 40, 150, TRUE);
            MoveWindow(hOutputList, 5, 5, width - 50, 140, TRUE);
        }
        
        // Resize controls in settings tab
        MoveWindow(hConfigLocationsEdit, 180, 45, width - 210, 25, TRUE);
        MoveWindow(hProxyEdit, 180, 105, width - 210, 25, TRUE);
        MoveWindow(hDownloaderArgsEdit, 180, 165, width - 210, 25, TRUE);
        MoveWindow(hBatchFileEdit, 180, 195, width - 280, 25, TRUE);
        MoveWindow(hOutputTemplateEdit, 180, 225, width - 210, 25, TRUE);
        MoveWindow(hCookiesFileEdit, 180, 255, width - 280, 25, TRUE);
        MoveWindow(hCacheDirEdit, 180, 315, width - 280, 25, TRUE);
        MoveWindow(hRemoveChaptersEdit, 180, 525, width - 210, 25, TRUE);
        
        break;
    }
    case WM_CLOSE:
        saveSettings();
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

// WinMain function
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, 
                      _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
    hInst = hInstance;
    
    // Get settings file path
    wchar_t appDataPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath);
    PathAppendW(appDataPath, L"\\yt-dlp-gui");
    CreateDirectoryW(appDataPath, NULL);
    settingsPath = std::wstring(appDataPath) + L"\\settings.ini";
    
    // Register window class
    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"YtDlpGuiClass";
    wcex.hIconSm = LoadIconW(wcex.hInstance, IDI_APPLICATION);
    
    RegisterClassExW(&wcex);
    
    // Create main window
    hMainWnd = CreateWindowW(wcex.lpszClassName, L"yt-dlp GUI", 
                             WS_OVERLAPPEDWINDOW, 
                             CW_USEDEFAULT, 0, 800, 600, 
                             NULL, NULL, hInstance, NULL);
    
    if (!hMainWnd) {
        return FALSE;
    }
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    // Message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    return (int)msg.wParam;
}
