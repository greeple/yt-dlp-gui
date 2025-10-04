#ifndef SETTINGS_H
#define SETTINGS_H

#include <windows.h>
#include <string>
#include <map>
#include <vector>

class Settings {
public:
    std::wstring outputPath;
    int quality;
    int format;
    
    // Дополнительные опции
    bool ignoreConfig;
    std::wstring configLocations;
    bool liveFromStart;
    std::wstring waitForVideo;
    std::wstring proxyUrl;
    std::wstring downloadSections;
    std::wstring downloader;
    std::wstring downloaderArgs;
    std::wstring batchFile;
    std::wstring outputTemplate;
    std::wstring cookiesFile;
    std::wstring cookiesFromBrowser;
    std::wstring cacheDir;
    bool noCacheDir;
    bool skipDownload;
    bool saveJson;
    bool listFormats;
    std::wstring mergeOutputFormat;
    bool checkFormats;
    bool embedSubs;
    bool embedThumbnail;
    bool embedMetadata;
    bool embedChapters;
    std::wstring convertSubs;
    std::wstring convertThumbnails;
    bool splitChapters;
    std::wstring removeChapters;
    bool forceKeyframesAtCuts;
    std::wstring formatSort;
    std::wstring extractorArgs;
    std::wstring addHeaders;
    
    Settings();
    void Load(const std::wstring& profile = L"");
    void Save(const std::wstring& profile = L"");
    std::vector<std::wstring> GetProfiles();
    void DeleteProfile(const std::wstring& profile);
    
private:
    std::wstring GetIniPath();
    std::wstring GetPresetsPath();
};

#endif
