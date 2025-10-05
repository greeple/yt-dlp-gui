#include "Settings.h"

Settings::Settings()
{
    settingsPath = Path::Combine(Path::GetDirectoryName(
        System::Reflection::Assembly::GetExecutingAssembly()->Location), 
        "yt-dlp-gui-settings.json");
    
    // Установка значений по умолчанию
    OutputDirectory = Environment::GetFolderPath(Environment::SpecialFolder::Desktop);
    OutputTemplate = "%(title)s.%(ext)s";
    
    Load();
}

void Settings::Load()
{
    if (!File::Exists(settingsPath))
        return;
        
    try
    {
        String^ json = File::ReadAllText(settingsPath, Encoding::UTF8);
        
        // Простая реализация парсинга JSON
        array<String^>^ lines = json->Split(gcnew array<wchar_t>{L'\n', L'\r'}, 
                                           StringSplitOptions::RemoveEmptyEntries);
        
        for each (String^ line in lines)
        {
            if (line->Trim()->StartsWith("\"") && line->Contains(":"))
            {
                array<String^>^ parts = line->Split(':');
                if (parts->Length >= 2)
                {
                    String^ key = parts[0]->Trim()->Trim('\"', ' ', '\t');
                    String^ value = String::Join(":", 
                        gcnew array<String^>(parts, 1, parts->Length - 1))->Trim()->Trim('\"', ' ', '\t', ',');
                    
                    if (key == "OutputDirectory") OutputDirectory = value;
                    else if (key == "OutputTemplate") OutputTemplate = value;
                    else if (key == "IgnoreConfig") IgnoreConfig = Boolean::Parse(value);
                    else if (key == "ConfigLocation") ConfigLocation = value;
                    else if (key == "LiveFromStart") LiveFromStart = Boolean::Parse(value);
                    else if (key == "Proxy") Proxy = value;
                    else if (key == "Downloader") Downloader = value;
                    else if (key == "DownloaderArgs") DownloaderArgs = value;
                    else if (key == "BatchFile") BatchFile = value;
                    else if (key == "Cookies") Cookies = value;
                    else if (key == "CookiesFromBrowser") CookiesFromBrowser = value;
                    else if (key == "CacheDir") CacheDir = value;
                    else if (key == "NoCacheDir") NoCacheDir = Boolean::Parse(value);
                    else if (key == "SkipDownload") SkipDownload = Boolean::Parse(value);
                    else if (key == "ListFormats") ListFormats = Boolean::Parse(value);
                    else if (key == "MergeOutputFormat") MergeOutputFormat = value;
                    else if (key == "CheckFormats") CheckFormats = Boolean::Parse(value);
                    else if (key == "EmbedSubs") EmbedSubs = Boolean::Parse(value);
                    else if (key == "EmbedThumbnail") EmbedThumbnail = Boolean::Parse(value);
                    else if (key == "EmbedMetadata") EmbedMetadata = Boolean::Parse(value);
                    else if (key == "EmbedChapters") EmbedChapters = Boolean::Parse(value);
                    else if (key == "ConvertSubs") ConvertSubs = value;
                    else if (key == "ConvertThumbnails") ConvertThumbnails = value;
                    else if (key == "SplitChapters") SplitChapters = Boolean::Parse(value);
                    else if (key == "RemoveChapters") RemoveChapters = value;
                    else if (key == "ForceKeyframesAtCuts") ForceKeyframesAtCuts = Boolean::Parse(value);
                }
            }
        }
    }
    catch (Exception^)
    {
        // Игнорируем ошибки загрузки
    }
}

void Settings::Save()
{
    try
    {
        System::Text::StringBuilder^ json = gcnew System::Text::StringBuilder();
        json->AppendLine("{");
        
        json->AppendLine(String::Format("  \"OutputDirectory\": \"{0}\",", OutputDirectory));
        json->AppendLine(String::Format("  \"OutputTemplate\": \"{0}\",", OutputTemplate));
        json->AppendLine(String::Format("  \"IgnoreConfig\": {0},", IgnoreConfig.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"ConfigLocation\": \"{0}\",", ConfigLocation));
        json->AppendLine(String::Format("  \"LiveFromStart\": {0},", LiveFromStart.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"Proxy\": \"{0}\",", Proxy));
        json->AppendLine(String::Format("  \"Downloader\": \"{0}\",", Downloader));
        json->AppendLine(String::Format("  \"DownloaderArgs\": \"{0}\",", DownloaderArgs));
        json->AppendLine(String::Format("  \"BatchFile\": \"{0}\",", BatchFile));
        json->AppendLine(String::Format("  \"Cookies\": \"{0}\",", Cookies));
        json->AppendLine(String::Format("  \"CookiesFromBrowser\": \"{0}\",", CookiesFromBrowser));
        json->AppendLine(String::Format("  \"CacheDir\": \"{0}\",", CacheDir));
        json->AppendLine(String::Format("  \"NoCacheDir\": {0},", NoCacheDir.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"SkipDownload\": {0},", SkipDownload.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"ListFormats\": {0},", ListFormats.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"MergeOutputFormat\": \"{0}\",", MergeOutputFormat));
        json->AppendLine(String::Format("  \"CheckFormats\": {0},", CheckFormats.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"EmbedSubs\": {0},", EmbedSubs.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"EmbedThumbnail\": {0},", EmbedThumbnail.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"EmbedMetadata\": {0},", EmbedMetadata.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"EmbedChapters\": {0},", EmbedChapters.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"ConvertSubs\": \"{0}\",", ConvertSubs));
        json->AppendLine(String::Format("  \"ConvertThumbnails\": \"{0}\",", ConvertThumbnails));
        json->AppendLine(String::Format("  \"SplitChapters\": {0},", SplitChapters.ToString()->ToLower()));
        json->AppendLine(String::Format("  \"RemoveChapters\": \"{0}\",", RemoveChapters));
        json->AppendLine(String::Format("  \"ForceKeyframesAtCuts\": {0}", ForceKeyframesAtCuts.ToString()->ToLower()));
        
        json->AppendLine("}");
        
        File::WriteAllText(settingsPath, json->ToString(), Encoding::UTF8);
    }
    catch (Exception^)
    {
        // Игнорируем ошибки сохранения
    }
}
