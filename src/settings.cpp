#include "settings.h"
#include <shlobj.h>
#include <fstream>

Settings::Settings() {
    // Значения по умолчанию
    wchar_t desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        outputPath = desktopPath;
    }
    
    quality = 2; // 1080p
    format = 0;  // Видео + Аудио
    
    ignoreConfig = false;
    liveFromStart = false;
    noCacheDir = false;
    skipDownload = false;
    saveJson = false;
    listFormats = false;
    checkFormats = false;
    embedSubs = false;
    embedThumbnail = false;
    embedMetadata = false;
    embedChapters = false;
    splitChapters = false;
    forceKeyframesAtCuts = false;
}

std::wstring Settings::GetIniPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    return path.substr(0, pos + 1) + L"settings.ini";
}

std::wstring Settings::GetPresetsPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    return path.substr(0, pos + 1) + L"presets.ini";
}

void Settings::Load(const std::wstring& profile) {
    std::wstring iniPath = profile.empty() ? GetIniPath() : GetPresetsPath();
    std::wstring section = profile.empty() ? L"Settings" : profile;
    
    wchar_t buffer[1024];
    
    GetPrivateProfileStringW(section.c_str(), L"OutputPath", outputPath.c_str(), buffer, 1024, iniPath.c_str());
    outputPath = buffer;
    
    quality = GetPrivateProfileIntW(section.c_str(), L"Quality", quality, iniPath.c_str());
    format = GetPrivateProfileIntW(section.c_str(), L"Format", format, iniPath.c_str());
    
    ignoreConfig = GetPrivateProfileIntW(section.c_str(), L"IgnoreConfig", 0, iniPath.c_str()) != 0;
    liveFromStart = GetPrivateProfileIntW(section.c_str(), L"LiveFromStart", 0, iniPath.c_str()) != 0;
    noCacheDir = GetPrivateProfileIntW(section.c_str(), L"NoCacheDir", 0, iniPath.c_str()) != 0;
    skipDownload = GetPrivateProfileIntW(section.c_str(), L"SkipDownload", 0, iniPath.c_str()) != 0;
    saveJson = GetPrivateProfileIntW(section.c_str(), L"SaveJson", 0, iniPath.c_str()) != 0;
    listFormats = GetPrivateProfileIntW(section.c_str(), L"ListFormats", 0, iniPath.c_str()) != 0;
    checkFormats = GetPrivateProfileIntW(section.c_str(), L"CheckFormats", 0, iniPath.c_str()) != 0;
    embedSubs = GetPrivateProfileIntW(section.c_str(), L"EmbedSubs", 0, iniPath.c_str()) != 0;
    embedThumbnail = GetPrivateProfileIntW(section.c_str(), L"EmbedThumbnail", 0, iniPath.c_str()) != 0;
    embedMetadata = GetPrivateProfileIntW(section.c_str(), L"EmbedMetadata", 0, iniPath.c_str()) != 0;
    embedChapters = GetPrivateProfileIntW(section.c_str(), L"EmbedChapters", 0, iniPath.c_str()) != 0;
    splitChapters = GetPrivateProfileIntW(section.c_str(), L"SplitChapters", 0, iniPath.c_str()) != 0;
    forceKeyframesAtCuts = GetPrivateProfileIntW(section.c_str(), L"ForceKeyframesAtCuts", 0, iniPath.c_str()) != 0;
    
    GetPrivateProfileStringW(section.c_str(), L"ConfigLocations", L"", buffer, 1024, iniPath.c_str());
    configLocations = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"WaitForVideo", L"", buffer, 1024, iniPath.c_str());
    waitForVideo = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"ProxyUrl", L"", buffer, 1024, iniPath.c_str());
    proxyUrl = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"DownloadSections", L"", buffer, 1024, iniPath.c_str());
    downloadSections = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"Downloader", L"", buffer, 1024, iniPath.c_str());
    downloader = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"DownloaderArgs", L"", buffer, 1024, iniPath.c_str());
    downloaderArgs = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"BatchFile", L"", buffer, 1024, iniPath.c_str());
    batchFile = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"OutputTemplate", L"", buffer, 1024, iniPath.c_str());
    outputTemplate = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"CookiesFile", L"", buffer, 1024, iniPath.c_str());
    cookiesFile = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"CookiesFromBrowser", L"", buffer, 1024, iniPath.c_str());
    cookiesFromBrowser = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"CacheDir", L"", buffer, 1024, iniPath.c_str());
    cacheDir = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"MergeOutputFormat", L"", buffer, 1024, iniPath.c_str());
    mergeOutputFormat = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"ConvertSubs", L"", buffer, 1024, iniPath.c_str());
    convertSubs = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"ConvertThumbnails", L"", buffer, 1024, iniPath.c_str());
    convertThumbnails = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"RemoveChapters", L"", buffer, 1024, iniPath.c_str());
    removeChapters = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"FormatSort", L"", buffer, 1024, iniPath.c_str());
    formatSort = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"ExtractorArgs", L"", buffer, 1024, iniPath.c_str());
    extractorArgs = buffer;
    
    GetPrivateProfileStringW(section.c_str(), L"AddHeaders", L"", buffer, 1024, iniPath.c_str());
    addHeaders = buffer;
}

void Settings::Save(const std::wstring& profile) {
    std::wstring iniPath = profile.empty() ? GetIniPath() : GetPresetsPath();
    std::wstring section = profile.empty() ? L"Settings" : profile;
    
    WritePrivateProfileStringW(section.c_str(), L"OutputPath", outputPath.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"Quality", std::to_wstring(quality).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"Format", std::to_wstring(format).c_str(), iniPath.c_str());
    
    WritePrivateProfileStringW(section.c_str(), L"IgnoreConfig", ignoreConfig ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"LiveFromStart", liveFromStart ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"NoCacheDir", noCacheDir ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"SkipDownload", skipDownload ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"SaveJson", saveJson ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"ListFormats", listFormats ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"CheckFormats", checkFormats ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"EmbedSubs", embedSubs ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"EmbedThumbnail", embedThumbnail ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"EmbedMetadata", embedMetadata ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"EmbedChapters", embedChapters ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"SplitChapters", splitChapters ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"ForceKeyframesAtCuts", forceKeyframesAtCuts ? L"1" : L"0", iniPath.c_str());
    
    WritePrivateProfileStringW(section.c_str(), L"ConfigLocations", configLocations.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"WaitForVideo", waitForVideo.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"ProxyUrl", proxyUrl.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"DownloadSections", downloadSections.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"Downloader", downloader.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"DownloaderArgs", downloaderArgs.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"BatchFile", batchFile.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"OutputTemplate", outputTemplate.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"CookiesFile", cookiesFile.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"CookiesFromBrowser", cookiesFromBrowser.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"CacheDir", cacheDir.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"MergeOutputFormat", mergeOutputFormat.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"ConvertSubs", convertSubs.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"ConvertThumbnails", convertThumbnails.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"RemoveChapters", removeChapters.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"FormatSort", formatSort.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"ExtractorArgs", extractorArgs.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(section.c_str(), L"AddHeaders", addHeaders.c_str(), iniPath.c_str());
}

std::vector<std::wstring> Settings::GetProfiles() {
    std::vector<std::wstring> profiles;
    std::wstring iniPath = GetPresetsPath();
    
    wchar_t buffer[4096];
    DWORD size = GetPrivateProfileSectionNamesW(buffer, 4096, iniPath.c_str());
    
    if (size > 0) {
        wchar_t* p = buffer;
        while (*p) {
            profiles.push_back(p);
            p += wcslen(p) + 1;
        }
    }
    
    return profiles;
}

void Settings::DeleteProfile(const std::wstring& profile) {
    WritePrivateProfileStringW(profile.c_str(), NULL, NULL, GetPresetsPath().c_str());
}
