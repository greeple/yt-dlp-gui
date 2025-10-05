#include "MainForm.h"

MainForm::MainForm(void)
{
    settings = gcnew Settings();
    isDownloading = false;
    InitializeComponent();
    LoadSettings();
}

MainForm::~MainForm()
{
    if (components)
        delete components;
}

void MainForm::InitializeComponent(void)
{
    this->Text = L"yt-dlp GUI";
    this->Size = System::Drawing::Size(800, 600);
    this->StartPosition = FormStartPosition::CenterScreen;
    this->MinimumSize = System::Drawing::Size(600, 400);
    
    // Основной TabControl
    tabControl = gcnew TabControl();
    tabControl->Dock = DockStyle::Fill;
    tabControl->TabPages->Add(L"Основное");
    tabControl->TabPages->Add(L"Форматы");
    tabControl->TabPages->Add(L"Субтитры");
    tabControl->TabPages->Add(L"Дополнительно");
    
    // === Вкладка "Основное" ===
    TabPage^ mainTab = tabControl->TabPages[0];
    
    // URL ввода
    Label^ urlLabel = gcnew Label();
    urlLabel->Text = L"URL видео:";
    urlLabel->Location = Point(10, 10);
    urlLabel->Size = Size(100, 20);
    mainTab->Controls->Add(urlLabel);
    
    urlTextBox = gcnew TextBox();
    urlTextBox->Location = Point(120, 10);
    urlTextBox->Size = Size(400, 20);
    urlTextBox->Anchor = AnchorStyles::Top | AnchorStyles::Left | AnchorStyles::Right;
    mainTab->Controls->Add(urlTextBox);
    
    // Папка сохранения
    Label^ outputDirLabel = gcnew Label();
    outputDirLabel->Text = L"Папка сохранения:";
    outputDirLabel->Location = Point(10, 40);
    outputDirLabel->Size = Size(100, 20);
    mainTab->Controls->Add(outputDirLabel);
    
    outputDirTextBox = gcnew TextBox();
    outputDirTextBox->Location = Point(120, 40);
    outputDirTextBox->Size = Size(350, 20);
    outputDirTextBox->Anchor = AnchorStyles::Top | AnchorStyles::Left | AnchorStyles::Right;
    mainTab->Controls->Add(outputDirTextBox);
    
    Button^ browseOutputDirButton = gcnew Button();
    browseOutputDirButton->Text = L"...";
    browseOutputDirButton->Location = Point(480, 40);
    browseOutputDirButton->Size = Size(40, 20);
    browseOutputDirButton->Anchor = AnchorStyles::Top | AnchorStyles::Right;
    browseOutputDirButton->Click += gcnew EventHandler(this, &MainForm::browseOutputDirButton_Click);
    mainTab->Controls->Add(browseOutputDirButton);
    
    // Шаблон имени файла
    Label^ outputTemplateLabel = gcnew Label();
    outputTemplateLabel->Text = L"Шаблон имени:";
    outputTemplateLabel->Location = Point(10, 70);
    outputTemplateLabel->Size = Size(100, 20);
    mainTab->Controls->Add(outputTemplateLabel);
    
    outputTemplateTextBox = gcnew TextBox();
    outputTemplateTextBox->Location = Point(120, 70);
    outputTemplateTextBox->Size = Size(400, 20);
    outputTemplateTextBox->Anchor = AnchorStyles::Top | AnchorStyles::Left | AnchorStyles::Right;
    outputTemplateTextBox->Text = L"%(title)s.%(ext)s";
    mainTab->Controls->Add(outputTemplateTextBox);
    
    // Пакетный файл
    Label^ batchFileLabel = gcnew Label();
    batchFileLabel->Text = L"Пакетный файл:";
    batchFileLabel->Location = Point(10, 100);
    batchFileLabel->Size = Size(100, 20);
    mainTab->Controls->Add(batchFileLabel);
    
    batchFileTextBox = gcnew TextBox();
    batchFileTextBox->Location = Point(120, 100);
    batchFileTextBox->Size = Size(350, 20);
    mainTab->Controls->Add(batchFileTextBox);
    
    Button^ browseBatchFileButton = gcnew Button();
    browseBatchFileButton->Text = L"...";
    browseBatchFileButton->Location = Point(480, 100);
    browseBatchFileButton->Size = Size(40, 20);
    browseBatchFileButton->Click += gcnew EventHandler(this, &MainForm::browseBatchFileButton_Click);
    mainTab->Controls->Add(browseBatchFileButton);
    
    // === Вкладка "Форматы" ===
    TabPage^ formatsTab = tabControl->TabPages[1];
    
    // Формат слияния
    Label^ mergeFormatLabel = gcnew Label();
    mergeFormatLabel->Text = L"Формат слияния:";
    mergeFormatLabel->Location = Point(10, 10);
    mergeFormatLabel->Size = Size(100, 20);
    formatsTab->Controls->Add(mergeFormatLabel);
    
    mergeFormatComboBox = gcnew ComboBox();
    mergeFormatComboBox->Location = Point(120, 10);
    mergeFormatComboBox->Size = Size(200, 20);
    mergeFormatComboBox->Items->AddRange(gcnew array<String^> { L"", L"mp4", L"mkv", L"avi", L"flv", L"mov", L"webm" });
    mergeFormatComboBox->DropDownStyle = ComboBoxStyle::DropDownList;
    formatsTab->Controls->Add(mergeFormatComboBox);
    
    // Разделение по главам
    splitChaptersCheckBox = gcnew CheckBox();
    splitChaptersCheckBox->Text = L"Разделить по главам";
    splitChaptersCheckBox->Location = Point(10, 40);
    splitChaptersCheckBox->Size = Size(150, 20);
    formatsTab->Controls->Add(splitChaptersCheckBox);
    
    // Удаление глав
    Label^ removeChaptersLabel = gcnew Label();
    removeChaptersLabel->Text = L"Удалить главы (regex):";
    removeChaptersLabel->Location = Point(10, 70);
    removeChaptersLabel->Size = Size(120, 20);
    formatsTab->Controls->Add(removeChaptersLabel);
    
    removeChaptersTextBox = gcnew TextBox();
    removeChaptersTextBox->Location = Point(140, 70);
    removeChaptersTextBox->Size = Size(300, 20);
    formatsTab->Controls->Add(removeChaptersTextBox);
    
    // Проверка форматов
    checkFormatsCheckBox = gcnew CheckBox();
    checkFormatsCheckBox->Text = L"Проверить форматы";
    checkFormatsCheckBox->Location = Point(10, 100);
    checkFormatsCheckBox->Size = Size(150, 20);
    formatsTab->Controls->Add(checkFormatsCheckBox);
    
    // Ключевые кадры
    forceKeyframesCheckBox = gcnew CheckBox();
    forceKeyframesCheckBox->Text = L"Принудительные ключевые кадры";
    forceKeyframesCheckBox->Location = Point(10, 130);
    forceKeyframesCheckBox->Size = Size(200, 20);
    formatsTab->Controls->Add(forceKeyframesCheckBox);
    
    // === Вкладка "Субтитры" ===
    TabPage^ subsTab = tabControl->TabPages[2];
    
    // Встроить субтитры
    embedSubsCheckBox = gcnew CheckBox();
    embedSubsCheckBox->Text = L"Встроить субтитры";
    embedSubsCheckBox->Location = Point(10, 10);
    embedSubsCheckBox->Size = Size(150, 20);
    subsTab->Controls->Add(embedSubsCheckBox);
    
    // Конвертировать субтитры
    Label^ convertSubsLabel = gcnew Label();
    convertSubsLabel->Text = L"Конвертировать субтитры:";
    convertSubsLabel->Location = Point(10, 40);
    convertSubsLabel->Size = Size(140, 20);
    subsTab->Controls->Add(convertSubsLabel);
    
    convertSubsComboBox = gcnew ComboBox();
    convertSubsComboBox->Location = Point(160, 40);
    convertSubsComboBox->Size = Size(100, 20);
    convertSubsComboBox->Items->AddRange(gcnew array<String^> { L"", L"ass", L"lrc", L"srt", L"vtt" });
    convertSubsComboBox->DropDownStyle = ComboBoxStyle::DropDownList;
    subsTab->Controls->Add(convertSubsComboBox);
    
    // Встроить обложку
    embedThumbnailCheckBox = gcnew CheckBox();
    embedThumbnailCheckBox->Text = L"Встроить обложку";
    embedThumbnailCheckBox->Location = Point(10, 70);
    embedThumbnailCheckBox->Size = Size(150, 20);
    subsTab->Controls->Add(embedThumbnailCheckBox);
    
    // Конвертировать обложки
    Label^ convertThumbnailsLabel = gcnew Label();
    convertThumbnailsLabel->Text = L"Конвертировать обложки:";
    convertThumbnailsLabel->Location = Point(10, 100);
    convertThumbnailsLabel->Size = Size(140, 20);
    subsTab->Controls->Add(convertThumbnailsLabel);
    
    convertThumbnailsComboBox = gcnew ComboBox();
    convertThumbnailsComboBox->Location = Point(160, 100);
    convertThumbnailsComboBox->Size = Size(100, 20);
    convertThumbnailsComboBox->Items->AddRange(gcnew array<String^> { L"", L"jpg", L"png", L"webp" });
    convertThumbnailsComboBox->DropDownStyle = ComboBoxStyle::DropDownList;
    subsTab->Controls->Add(convertThumbnailsComboBox);
    
    // Встроить метаданные
    embedMetadataCheckBox = gcnew CheckBox();
    embedMetadataCheckBox->Text = L"Встроить метаданные";
    embedMetadataCheckBox->Location = Point(10, 130);
    embedMetadataCheckBox->Size = Size(150, 20);
    subsTab->Controls->Add(embedMetadataCheckBox);
    
    // Встроить главы
    embedChaptersCheckBox = gcnew CheckBox();
    embedChaptersCheckBox->Text = L"Встроить главы";
    embedChaptersCheckBox->Location = Point(10, 160);
    embedChaptersCheckBox->Size = Size(150, 20);
    subsTab->Controls->Add(embedChaptersCheckBox);
    
    // === Вкладка "Дополнительно" ===
    TabPage^ advancedTab = tabControl->TabPages[3];
    
    // Игнорировать конфиг
    ignoreConfigCheckBox = gcnew CheckBox();
    ignoreConfigCheckBox->Text = L"Игнорировать конфиг";
    ignoreConfigCheckBox->Location = Point(10, 10);
    ignoreConfigCheckBox->Size = Size(150, 20);
    advancedTab->Controls->Add(ignoreConfigCheckBox);
    
    // Расположение конфига
    Label^ configLocationLabel = gcnew Label();
    configLocationLabel->Text = L"Расположение конфига:";
    configLocationLabel->Location = Point(10, 40);
    configLocationLabel->Size = Size(120, 20);
    advancedTab->Controls->Add(configLocationLabel);
    
    configLocationTextBox = gcnew TextBox();
    configLocationTextBox->Location = Point(140, 40);
    configLocationTextBox->Size = Size(300, 20);
    advancedTab->Controls->Add(configLocationTextBox);
    
    Button^ browseConfigLocationButton = gcnew Button();
    browseConfigLocationButton->Text = L"...";
    browseConfigLocationButton->Location = Point(450, 40);
    browseConfigLocationButton->Size = Size(40, 20);
    browseConfigLocationButton->Click += gcnew EventHandler(this, &MainForm::browseConfigLocationButton_Click);
    advancedTab->Controls->Add(browseConfigLocationButton);
    
    // Прямой эфир с начала
    liveFromStartCheckBox = gcnew CheckBox();
    liveFromStartCheckBox->Text = L"Прямой эфир с начала";
    liveFromStartCheckBox->Location = Point(10, 70);
    liveFromStartCheckBox->Size = Size(150, 20);
    advancedTab->Controls->Add(liveFromStartCheckBox);
    
    // Прокси
    Label^ proxyLabel = gcnew Label();
    proxyLabel->Text = L"Прокси:";
    proxyLabel->Location = Point(10, 100);
    proxyLabel->Size = Size(100, 20);
    advancedTab->Controls->Add(proxyLabel);
    
    proxyTextBox = gcnew TextBox();
    proxyTextBox->Location = Point(120, 100);
    proxyTextBox->Size = Size(300, 20);
    advancedTab->Controls->Add(proxyTextBox);
    
    // Загрузчик
    Label^ downloaderLabel = gcnew Label();
    downloaderLabel->Text = L"Загрузчик:";
    downloaderLabel->Location = Point(10, 130);
    downloaderLabel->Size = Size(100, 20);
    advancedTab->Controls->Add(downloaderLabel);
    
    downloaderComboBox = gcnew ComboBox();
    downloaderComboBox->Location = Point(120, 130);
    downloaderComboBox->Size = Size(200, 20);
    downloaderComboBox->Items->AddRange(gcnew array<String^> {
        L"", L"native", L"aria2c", L"axel", L"curl", L"ffmpeg", L"httpie", L"wget"
    });
    advancedTab->Controls->Add(downloaderComboBox);
    
    // Аргументы загрузчика
    Label^ downloaderArgsLabel = gcnew Label();
    downloaderArgsLabel->Text = L"Аргументы загрузчика:";
    downloaderArgsLabel->Location = Point(10, 160);
    downloaderArgsLabel->Size = Size(120, 20);
    advancedTab->Controls->Add(downloaderArgsLabel);
    
    downloaderArgsTextBox = gcnew TextBox();
    downloaderArgsTextBox->Location = Point(140, 160);
    downloaderArgsTextBox->Size = Size(300, 20);
    advancedTab->Controls->Add(downloaderArgsTextBox);
    
    // Куки
    Label^ cookiesLabel = gcnew Label();
    cookiesLabel->Text = L"Файл куки:";
    cookiesLabel->Location = Point(10, 190);
    cookiesLabel->Size = Size(100, 20);
    advancedTab->Controls->Add(cookiesLabel);
    
    cookiesTextBox = gcnew TextBox();
    cookiesTextBox->Location = Point(120, 190);
    cookiesTextBox->Size = Size(300, 20);
    advancedTab->Controls->Add(cookiesTextBox);
    
    Button^ browseCookiesButton = gcnew Button();
    browseCookiesButton->Text = L"...";
    browseCookiesButton->Location = Point(430, 190);
    browseCookiesButton->Size = Size(40, 20);
    browseCookiesButton->Click += gcnew EventHandler(this, &MainForm::browseCookiesButton_Click);
    advancedTab->Controls->Add(browseCookiesButton);
    
    // Браузер для куки
    Label^ cookiesBrowserLabel = gcnew Label();
    cookiesBrowserLabel->Text = L"Браузер для куки:";
    cookiesBrowserLabel->Location = Point(10, 220);
    cookiesBrowserLabel->Size = Size(120, 20);
    advancedTab->Controls->Add(cookiesBrowserLabel);
    
    cookiesBrowserComboBox = gcnew ComboBox();
    cookiesBrowserComboBox->Location = Point(140, 220);
    cookiesBrowserComboBox->Size = Size(200, 20);
    cookiesBrowserComboBox->Items->AddRange(gcnew array<String^> {
        L"", L"brave", L"chrome", L"chromium", L"edge", L"firefox", L"opera", L"safari", L"vivaldi", L"whale"
    });
    advancedTab->Controls->Add(cookiesBrowserComboBox);
    
    // Кэш директория
    Label^ cacheDirLabel = gcnew Label();
    cacheDirLabel->Text = L"Директория кэша:";
    cacheDirLabel->Location = Point(10, 250);
    cacheDirLabel->Size = Size(120, 20);
    advancedTab->Controls->Add(cacheDirLabel);
    
    cacheDirTextBox = gcnew TextBox();
    cacheDirTextBox->Location = Point(140, 250);
    cacheDirTextBox->Size = Size(300, 20);
    advancedTab->Controls->Add(cacheDirTextBox);
    
    Button^ browseCacheDirButton = gcnew Button();
    browseCacheDirButton->Text = L"...";
    browseCacheDirButton->Location = Point(450, 250);
    browseCacheDirButton->Size = Size(40, 20);
    browseCacheDirButton->Click += gcnew EventHandler(this, &MainForm::browseCacheDirButton_Click);
    advancedTab->Controls->Add(browseCacheDirButton);
    
    // Без кэша
    noCacheDirCheckBox = gcnew CheckBox();
    noCacheDirCheckBox->Text = L"Без кэша";
    noCacheDirCheckBox->Location = Point(10, 280);
    noCacheDirCheckBox->Size = Size(100, 20);
    advancedTab->Controls->Add(noCacheDirCheckBox);
    
    // Очистить кэш
    clearCacheButton = gcnew Button();
    clearCacheButton->Text = L"Очистить кэш";
    clearCacheButton->Location = Point(120, 280);
    clearCacheButton->Size = Size(100, 20);
    clearCacheButton->Click += gcnew EventHandler(this, &MainForm::clearCacheButton_Click);
    advancedTab->Controls->Add(clearCacheButton);
    
    // Пропустить загрузку
    skipDownloadCheckBox = gcnew CheckBox();
    skipDownloadCheckBox->Text = L"Пропустить загрузку";
    skipDownloadCheckBox->Location = Point(10, 310);
    skipDownloadCheckBox->Size = Size(150, 20);
    advancedTab->Controls->Add(skipDownloadCheckBox);
    
    // Список форматов
    listFormatsCheckBox = gcnew CheckBox();
    listFormatsCheckBox->Text = L"Список форматов";
    listFormatsCheckBox->Location = Point(10, 340);
    listFormatsCheckBox->Size = Size(150, 20);
    advancedTab->Controls->Add(listFormatsCheckBox);
    
    // Основные элементы управления
    downloadButton = gcnew Button();
    downloadButton->Text = L"Скачать";
    downloadButton->Location = Point(10, 500);
    downloadButton->Size = Size(100, 30);
    downloadButton->Anchor = AnchorStyles::Bottom | AnchorStyles::Left;
    downloadButton->Click += gcnew EventHandler(this, &MainForm::downloadButton_Click);
    this->Controls->Add(downloadButton);
    
    progressBar = gcnew ProgressBar();
    progressBar->Location = Point(120, 500);
    progressBar->Size = Size(400, 30);
    progressBar->Anchor = AnchorStyles::Bottom | AnchorStyles::Left | AnchorStyles::Right;
    progressBar->Visible = false;
    this->Controls->Add(progressBar);
    
    toggleOutputButton = gcnew Button();
    toggleOutputButton->Text = L"Показать вывод";
    toggleOutputButton->Location = Point(530, 500);
    toggleOutputButton->Size = Size(100, 30);
    toggleOutputButton->Anchor = AnchorStyles::Bottom | AnchorStyles::Right;
    toggleOutputButton->Click += gcnew EventHandler(this, &MainForm::toggleOutputButton_Click);
    this->Controls->Add(toggleOutputButton);
    
    outputPanel = gcnew Panel();
    outputPanel->Location = Point(10, 540);
    outputPanel->Size = Size(760, 200);
    outputPanel->Anchor = AnchorStyles::Bottom | AnchorStyles::Left | AnchorStyles::Right;
    outputPanel->Visible = false;
    outputPanel->BorderStyle = BorderStyle::FixedSingle;
    this->Controls->Add(outputPanel);
    
    outputTextBox = gcnew TextBox();
    outputTextBox->Multiline = true;
    outputTextBox->ScrollBars = ScrollBars::Both;
    outputTextBox->Dock = DockStyle::Fill;
    outputTextBox->ReadOnly = true;
    outputTextBox->Font = gcnew Drawing::Font(L"Consolas", 9);
    outputPanel->Controls->Add(outputTextBox);
    
    this->Controls->Add(tabControl);
}

void MainForm::LoadSettings()
{
    outputDirTextBox->Text = settings->OutputDirectory;
    outputTemplateTextBox->Text = settings->OutputTemplate;
    ignoreConfigCheckBox->Checked = settings->IgnoreConfig;
    configLocationTextBox->Text = settings->ConfigLocation;
    liveFromStartCheckBox->Checked = settings->LiveFromStart;
    proxyTextBox->Text = settings->Proxy;
    downloaderComboBox->Text = settings->Downloader;
    downloaderArgsTextBox->Text = settings->DownloaderArgs;
    batchFileTextBox->Text = settings->BatchFile;
    cookiesTextBox->Text = settings->Cookies;
    cookiesBrowserComboBox->Text = settings->CookiesFromBrowser;
    cacheDirTextBox->Text = settings->CacheDir;
    noCacheDirCheckBox->Checked = settings->NoCacheDir;
    skipDownloadCheckBox->Checked = settings->SkipDownload;
    listFormatsCheckBox->Checked = settings->ListFormats;
    mergeFormatComboBox->Text = settings->MergeOutputFormat;
    checkFormatsCheckBox->Checked = settings->CheckFormats;
    embedSubsCheckBox->Checked = settings->EmbedSubs;
    embedThumbnailCheckBox->Checked = settings->EmbedThumbnail;
    embedMetadataCheckBox->Checked = settings->EmbedMetadata;
    embedChaptersCheckBox->Checked = settings->EmbedChapters;
    convertSubsComboBox->Text = settings->ConvertSubs;
    convertThumbnailsComboBox->Text = settings->ConvertThumbnails;
    splitChaptersCheckBox->Checked = settings->SplitChapters;
    removeChaptersTextBox->Text = settings->RemoveChapters;
    forceKeyframesCheckBox->Checked = settings->ForceKeyframesAtCuts;
}

void MainForm::SaveSettings()
{
    settings->OutputDirectory = outputDirTextBox->Text;
    settings->OutputTemplate = outputTemplateTextBox->Text;
    settings->IgnoreConfig = ignoreConfigCheckBox->Checked;
    settings->ConfigLocation = configLocationTextBox->Text;
    settings->LiveFromStart = liveFromStartCheckBox->Checked;
    settings->Proxy = proxyTextBox->Text;
    settings->Downloader = downloaderComboBox->Text;
    settings->DownloaderArgs = downloaderArgsTextBox->Text;
    settings->BatchFile = batchFileTextBox->Text;
    settings->Cookies = cookiesTextBox->Text;
    settings->CookiesFromBrowser = cookiesBrowserComboBox->Text;
    settings->CacheDir = cacheDirTextBox->Text;
    settings->NoCacheDir = noCacheDirCheckBox->Checked;
    settings->SkipDownload = skipDownloadCheckBox->Checked;
    settings->ListFormats = listFormatsCheckBox->Checked;
    settings->MergeOutputFormat = mergeFormatComboBox->Text;
    settings->CheckFormats = checkFormatsCheckBox->Checked;
    settings->EmbedSubs = embedSubsCheckBox->Checked;
    settings->EmbedThumbnail = embedThumbnailCheckBox->Checked;
    settings->EmbedMetadata = embedMetadataCheckBox->Checked;
    settings->EmbedChapters = embedChaptersCheckBox->Checked;
    settings->ConvertSubs = convertSubsComboBox->Text;
    settings->ConvertThumbnails = convertThumbnailsComboBox->Text;
    settings->SplitChapters = splitChaptersCheckBox->Checked;
    settings->RemoveChapters = removeChaptersTextBox->Text;
    settings->ForceKeyframesAtCuts = forceKeyframesCheckBox->Checked;
    
    settings->Save();
}

// Остальные методы реализации...
void MainForm::downloadButton_Click(Object^ sender, EventArgs^ e)
{
    if (isDownloading)
    {
        if (ytDlpProcess != nullptr && !ytDlpProcess->HasExited)
        {
            ytDlpProcess->Kill();
        }
        downloadButton->Text = L"Скачать";
        isDownloading = false;
        progressBar->Visible = false;
        return;
    }
    
    if (String::IsNullOrEmpty(urlTextBox->Text) && String::IsNullOrEmpty(batchFileTextBox->Text))
    {
        MessageBox::Show(L"Введите URL или выберите пакетный файл", L"Ошибка", 
                        MessageBoxButtons::OK, MessageBoxIcon::Error);
        return;
    }
    
    SaveSettings();
    
    downloadButton->Text = L"Остановить";
    isDownloading = true;
    progressBar->Visible = true;
    progressBar->Style = ProgressBarStyle::Marquee;
    outputTextBox->Clear();
    
    try
    {
        String^ arguments = BuildArguments();
        
        ytDlpProcess = gcnew Process();
        ytDlpProcess->StartInfo->FileName = L"yt-dlp.exe";
        ytDlpProcess->StartInfo->Arguments = arguments;
        ytDlpProcess->StartInfo->UseShellExecute = false;
        ytDlpProcess->StartInfo->RedirectStandardOutput = true;
        ytDlpProcess->StartInfo->RedirectStandardError = true;
        ytDlpProcess->StartInfo->CreateNoWindow = true;
        
        UpdateOutputEncoding(ytDlpProcess->StartInfo);
        
        ytDlpProcess->EnableRaisingEvents = true;
        ytDlpProcess->Exited += gcnew EventHandler(this, &MainForm::DownloadCompleted);
        
        ytDlpProcess->Start();
        
        outputThread = gcnew Thread(gcnew ThreadStart(this, &MainForm::ReadOutput));
        outputThread->Start();
    }
    catch (Exception^ ex)
    {
        MessageBox::Show(L"Ошибка запуска yt-dlp: " + ex->Message, L"Ошибка", 
                        MessageBoxButtons::OK, MessageBoxIcon::Error);
        DownloadCompleted();
    }
}

String^ MainForm::BuildArguments()
{
    System::Text::StringBuilder^ args = gcnew System::Text::StringBuilder();
    
    // URL или пакетный файл
    if (!String::IsNullOrEmpty(urlTextBox->Text))
        args->Append("\"" + urlTextBox->Text + "\" ");
    else if (!String::IsNullOrEmpty(batchFileTextBox->Text))
        args->Append("--batch-file \"" + batchFileTextBox->Text + "\" ");
    
    // Основные опции
    if (!String::IsNullOrEmpty(outputDirTextBox->Text))
        args->Append("--output \"" + outputDirTextBox->Text + "\\" + outputTemplateTextBox->Text + "\" ");
    
    if (ignoreConfigCheckBox->Checked)
        args->Append("--ignore-config ");
    
    if (!String::IsNullOrEmpty(configLocationTextBox->Text))
        args->Append("--config-locations \"" + configLocationTextBox->Text + "\" ");
    
    if (liveFromStartCheckBox->Checked)
        args->Append("--live-from-start ");
    
    if (!String::IsNullOrEmpty(proxyTextBox->Text))
        args->Append("--proxy \"" + proxyTextBox->Text + "\" ");
    
    if (!String::IsNullOrEmpty(downloaderComboBox->Text))
        args->Append("--downloader \"" + downloaderComboBox->Text + "\" ");
    
    if (!String::IsNullOrEmpty(downloaderArgsTextBox->Text))
        args->Append("--downloader-args \"" + downloaderArgsTextBox->Text + "\" ");
    
    if (!String::IsNullOrEmpty(cookiesTextBox->Text))
        args->Append("--cookies \"" + cookiesTextBox->Text + "\" ");
    
    if (!String::IsNullOrEmpty(cookiesBrowserComboBox->Text))
        args->Append("--cookies-from-browser \"" + cookiesBrowserComboBox->Text + "\" ");
    
    if (!String::IsNullOrEmpty(cacheDirTextBox->Text))
        args->Append("--cache-dir \"" + cacheDirTextBox->Text + "\" ");
    
    if (noCacheDirCheckBox->Checked)
        args->Append("--no-cache-dir ");
    
    if (skipDownloadCheckBox->Checked)
        args->Append("--skip-download ");
    
    if (listFormatsCheckBox->Checked)
        args->Append("--list-formats ");
    
    if (!String::IsNullOrEmpty(mergeFormatComboBox->Text))
        args->Append("--merge-output-format \"" + mergeFormatComboBox->Text + "\" ");
    
    if (checkFormatsCheckBox->Checked)
        args->Append("--check-formats ");
    
    if (embedSubsCheckBox->Checked)
        args->Append("--embed-subs ");
    
    if (embedThumbnailCheckBox->Checked)
        args->Append("--embed-thumbnail ");
    
    if (embedMetadataCheckBox->Checked)
        args->Append("--embed-metadata ");
    
    if (embedChaptersCheckBox->Checked)
        args->Append("--embed-chapters ");
    
    if (!String::IsNullOrEmpty(convertSubsComboBox->Text))
        args->Append("--convert-subs \"" + convertSubsComboBox->Text + "\" ");
    
    if (!String::IsNullOrEmpty(convertThumbnailsComboBox->Text))
        args->Append("--convert-thumbnails \"" + convertThumbnailsComboBox->Text + "\" ");
    
    if (splitChaptersCheckBox->Checked)
        args->Append("--split-chapters ");
    
    if (!String::IsNullOrEmpty(removeChaptersTextBox->Text))
        args->Append("--remove-chapters \"" + removeChaptersTextBox->Text + "\" ");
    
    if (forceKeyframesCheckBox->Checked)
        args->Append("--force-keyframes-at-cuts ");
    
    return args->ToString();
}

void MainForm::UpdateOutputEncoding(ProcessStartInfo^ startInfo)
{
    // Установка кодировки для корректного отображения русского текста
    startInfo->StandardOutputEncoding = System::Text::Encoding::GetEncoding(1251);
    startInfo->StandardErrorEncoding = System::Text::Encoding::GetEncoding(1251);
}

void MainForm::ReadOutput()
{
    try
    {
        while (!ytDlpProcess->StandardOutput->EndOfStream)
        {
            String^ line = ytDlpProcess->StandardOutput->ReadLine();
            this->Invoke(gcnew Action<String^>(this, &MainForm::UpdateProgress), line);
        }
        
        String^ error = ytDlpProcess->StandardError->ReadToEnd();
        if (!String::IsNullOrEmpty(error))
        {
            this->Invoke(gcnew Action<String^>(this, &MainForm::UpdateProgress), L"ОШИБКА: " + error);
        }
    }
    catch (Exception^)
    {
        // Игнорируем ошибки при чтении вывода
    }
}

void MainForm::UpdateProgress(String^ line)
{
    outputTextBox->AppendText(line + Environment::NewLine);
    outputTextBox->ScrollToCaret();
    
    // Парсинг прогресса (упрощенная версия)
    if (line->Contains("%"))
    {
        // Можно добавить парсинг процентов для обычного прогресс-бара
    }
}

void MainForm::DownloadCompleted()
{
    this->Invoke(gcnew Action(this, &MainForm::DownloadCompletedUI));
}

void MainForm::DownloadCompletedUI()
{
    downloadButton->Text = L"Скачать";
    isDownloading = false;
    progressBar->Visible = false;
    
    if (ytDlpProcess != nullptr && ytDlpProcess->HasExited && ytDlpProcess->ExitCode == 0)
    {
        MessageBox::Show(L"Загрузка завершена успешно!", L"Готово", 
                        MessageBoxButtons::OK, MessageBoxIcon::Information);
    }
}

// Методы для кнопок обзора...
void MainForm::browseOutputDirButton_Click(Object^ sender, EventArgs^ e)
{
    FolderBrowserDialog^ dialog = gcnew FolderBrowserDialog();
    dialog->SelectedPath = outputDirTextBox->Text;
    if (dialog->ShowDialog() == DialogResult::OK)
    {
        outputDirTextBox->Text = dialog->SelectedPath;
    }
}

void MainForm::browseBatchFileButton_Click(Object^ sender, EventArgs^ e)
{
    OpenFileDialog^ dialog = gcnew OpenFileDialog();
    dialog->Filter = L"Текстовые файлы (*.txt)|*.txt|Все файлы (*.*)|*.*";
    if (dialog->ShowDialog() == DialogResult::OK)
    {
        batchFileTextBox->Text = dialog->FileName;
    }
}

void MainForm::browseConfigLocationButton_Click(Object^ sender, EventArgs^ e)
{
    OpenFileDialog^ dialog = gcnew OpenFileDialog();
    dialog->Filter = L"Конфигурационные файлы (*.conf)|*.conf|Все файлы (*.*)|*.*";
    if (dialog->ShowDialog() == DialogResult::OK)
    {
        configLocationTextBox->Text = dialog->FileName;
    }
}

void MainForm::browseCookiesButton_Click(Object^ sender, EventArgs^ e)
{
    OpenFileDialog^ dialog = gcnew OpenFileDialog();
    dialog->Filter = L"Файлы куки (*.txt)|*.txt|Все файлы (*.*)|*.*";
    if (dialog->ShowDialog() == DialogResult::OK)
    {
        cookiesTextBox->Text = dialog->FileName;
    }
}

void MainForm::browseCacheDirButton_Click(Object^ sender, EventArgs^ e)
{
    FolderBrowserDialog^ dialog = gcnew FolderBrowserDialog();
    dialog->SelectedPath = cacheDirTextBox->Text;
    if (dialog->ShowDialog() == DialogResult::OK)
    {
        cacheDirTextBox->Text = dialog->SelectedPath;
    }
}

void MainForm::toggleOutputButton_Click(Object^ sender, EventArgs^ e)
{
    outputPanel->Visible = !outputPanel->Visible;
    toggleOutputButton->Text = outputPanel->Visible ? L"Скрыть вывод" : L"Показать вывод";
    
    if (outputPanel->Visible)
    {
        this->Height += 200;
    }
    else
    {
        this->Height -= 200;
    }
}

void MainForm::clearCacheButton_Click(Object^ sender, EventArgs^ e)
{
    if (MessageBox::Show(L"Вы уверены, что хотите очистить кэш?", L"Очистка кэша", 
                        MessageBoxButtons::YesNo, MessageBoxIcon::Question) == DialogResult::Yes)
    {
        try
        {
            Process::Start("yt-dlp.exe", "--rm-cache-dir");
            MessageBox::Show(L"Кэш очищен", L"Готово", 
                           MessageBoxButtons::OK, MessageBoxIcon::Information);
        }
        catch (Exception^ ex)
        {
            MessageBox::Show(L"Ошибка очистки кэша: " + ex->Message, L"Ошибка", 
                           MessageBoxButtons::OK, MessageBoxIcon::Error);
        }
    }
}
