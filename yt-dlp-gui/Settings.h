#pragma once

using namespace System;
using namespace System::IO;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace System::Runtime::Serialization::Json;

public ref class Settings
{
private:
    String^ settingsPath;

public:
    Settings();

    property String^ OutputDirectory;
    property String^ OutputTemplate;
    property bool IgnoreConfig;
    property String^ ConfigLocation;
    property bool LiveFromStart;
    property String^ Proxy;
    property String^ Downloader;
    property String^ DownloaderArgs;
    property String^ BatchFile;
    property String^ Cookies;
    property String^ CookiesFromBrowser;
    property String^ CacheDir;
    property bool NoCacheDir;
    property bool SkipDownload;
    property bool ListFormats;
    property String^ MergeOutputFormat;
    property bool CheckFormats;
    property bool EmbedSubs;
    property bool EmbedThumbnail;
    property bool EmbedMetadata;
    property bool EmbedChapters;
    property String^ ConvertSubs;
    property String^ ConvertThumbnails;
    property bool SplitChapters;
    property String^ RemoveChapters;
    property bool ForceKeyframesAtCuts;

    void Load();
    void Save();
};
