#pragma once
#include <string>
#include <vector>
#include <msclr/marshal_cppstd.h>
#include "Settings.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::IO;
using namespace System::Diagnostics;
using namespace System::Threading;

public ref class MainForm : public Form
{
private:
    Settings^ settings;
    Process^ ytDlpProcess;
    Thread^ outputThread;
    bool isDownloading;
    
    // Основные элементы управления
    TabControl^ tabControl;
    TextBox^ urlTextBox;
    Button^ downloadButton;
    ProgressBar^ progressBar;
    Button^ toggleOutputButton;
    TextBox^ outputTextBox;
    Panel^ outputPanel;
    
    // Элементы для настроек
    TextBox^ outputDirTextBox;
    CheckBox^ ignoreConfigCheckBox;
    TextBox^ configLocationTextBox;
    CheckBox^ liveFromStartCheckBox;
    TextBox^ proxyTextBox;
    ComboBox^ downloaderComboBox;
    TextBox^ downloaderArgsTextBox;
    TextBox^ batchFileTextBox;
    TextBox^ outputTemplateTextBox;
    TextBox^ cookiesTextBox;
    ComboBox^ cookiesBrowserComboBox;
    TextBox^ cacheDirTextBox;
    CheckBox^ noCacheDirCheckBox;
    Button^ clearCacheButton;
    CheckBox^ skipDownloadCheckBox;
    CheckBox^ listFormatsCheckBox;
    ComboBox^ mergeFormatComboBox;
    CheckBox^ checkFormatsCheckBox;
    CheckBox^ embedSubsCheckBox;
    CheckBox^ embedThumbnailCheckBox;
    CheckBox^ embedMetadataCheckBox;
    CheckBox^ embedChaptersCheckBox;
    ComboBox^ convertSubsComboBox;
    ComboBox^ convertThumbnailsComboBox;
    CheckBox^ splitChaptersCheckBox;
    TextBox^ removeChaptersTextBox;
    CheckBox^ forceKeyframesCheckBox;

public:
    MainForm(void);

protected:
    ~MainForm();

private:
    void InitializeComponent(void);
    void LoadSettings();
    void SaveSettings();
    void downloadButton_Click(Object^ sender, EventArgs^ e);
    void browseOutputDirButton_Click(Object^ sender, EventArgs^ e);
    void browseBatchFileButton_Click(Object^ sender, EventArgs^ e);
    void browseConfigLocationButton_Click(Object^ sender, EventArgs^ e);
    void browseCookiesButton_Click(Object^ sender, EventArgs^ e);
    void browseCacheDirButton_Click(Object^ sender, EventArgs^ e);
    void toggleOutputButton_Click(Object^ sender, EventArgs^ e);
    void clearCacheButton_Click(Object^ sender, EventArgs^ e);
    void ReadOutput();
    void UpdateProgress(String^ line);
    void DownloadCompleted();
    String^ BuildArguments();
    void UpdateOutputEncoding(ProcessStartInfo^ startInfo);
};
