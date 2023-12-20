#include <iostream>
#include <vector>
#include <regex>
#include "prehrajto-api.h"
#include <direct.h>
#include <fstream>
#include <curl/curl.h>

using namespace std;

volatile bool printPerc = false;

PrehrajtoFile::PrehrajtoFile(string link) : lnk(link) {
    col = GetStdHandle(STD_OUTPUT_HANDLE);
    getSourceLink();
}

//PRIV FUNCTIONS
int progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    static const int progressBarWidth = 70;

    // Calculate the percentage of completion
    int progress = (uint32_t)((dlnow / dltotal) * 100.0);
    //cout << progress << endl;

    // Calculate the number of characters to fill in the progress bar
    int pos = progressBarWidth * (dlnow / dltotal);

    // Clear the console
    std::cout << "\r";

    // Print the progress bar
    std::cout << "[";
    for (int i = 0; i < progressBarWidth; ++i) {
        if (i < pos)
            std::cout << "#";
        else if (i == pos)
            std::cout << "#";
        else
            std::cout << " ";
    }
    if (!(progress > 100)) {
        cout << "] " << progress << "%\r";
    }
    printPerc = true;
    std::cout.flush();

    return 0;
}

size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
    ofstream* file = (ofstream*)stream;
    file->write((const char*)ptr, size * nmemb);
    return size * nmemb;
}

size_t writeCallback(char* buffer, size_t size, size_t nmemb, std::string* userData) {
    userData->append(buffer, size * nmemb);
    return size * nmemb;
}

void PrehrajtoFile::setCursorState(bool state) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(col, &cursorInfo);
    if (state) {
        cursorInfo.bVisible = 1;
    }
    else {
        cursorInfo.bVisible = 0;
    }
    SetConsoleCursorInfo(col, &cursorInfo);
}

void PrehrajtoFile::getSourceLink() {
    string response;
    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        lastError = "Source Link Fetching Error: Failed to create cURL handle";
        _errorOccured = true;
        return;
    }
    curl_easy_setopt(curl, CURLOPT_URL, lnk.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        lastError = "Source Link Fetching Error: " + (string)curl_easy_strerror(res);
        _errorOccured = true;
        return;
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    regex SourcesRegex("var\\s+sources\\s*=\\s*\\[([^\\[\\]]*)\\]");
    smatch match;

    if (regex_search(response, match, SourcesRegex)) {
        string sourcesVar = match[1];
        regex linkRegex("\"(.*?)\"");
        sregex_iterator linkIter(sourcesVar.begin(), sourcesVar.end(), linkRegex);
        sregex_iterator end;

        for (; linkIter != end; ++linkIter) {
            sourceLinks.push_back((*linkIter)[1].str());
        }

        if (sourceLinks.empty()) {
            lastError = "Source Link Fetching Error: No links found";
            _errorOccured = true;
            return;
        }

        regex qualityRegex("\'(.*?)\'");
        sregex_iterator qualityIter(sourcesVar.begin(), sourcesVar.end(), qualityRegex);
        sregex_iterator qend;

        for (; qualityIter != qend; ++qualityIter) {
            qualities.push_back((*qualityIter)[1].str());
        }

        if (qualities.empty()) {
            lastError = "Source Link Fetching Error: No quality labels found";
            _errorOccured = true;
            return;
        }

        linkNum = sourceLinks.size();
    }
    else {
        lastError = "Source Link Fetching Error: Sources variable not found";
        _errorOccured = true;
        return;
    }
}

//PUB FUNCTIONS - QUALITY AND FILE NAME
vector<string> PrehrajtoFile::getQualities_str(){
    return qualities;
}

int PrehrajtoFile::getQualities_int() {
    return linkNum;
}

string PrehrajtoFile::getFileName() {
    regex fNameRegex("https:\\/\\/prehrajto\\.cz\\/(.*?)\\/");
    smatch match;
    string fName;

    if (regex_search(lnk, match, fNameRegex)) {
        fName = match[1].str();
    }
    else {
        lastError = "Filename Error: Filename not found in the link";
        return NULL;
    }
    return fName;
}

void PrehrajtoFile::selectQuality(int q) {
    if ((q > linkNum) || (q < 0)) {
        lastError = "Quality Selection Error: Number out of range";
        _errorOccured = true;
        return;
    }
    else {
        selectedQuality = q;
    }
}

//PUB FUNCTIONS - ERRORS
string PrehrajtoFile::getLastError_str() {
    string err = lastError;
    lastError = "";
    return err;
}

bool PrehrajtoFile::errorOccured() {
    bool err = _errorOccured;
    _errorOccured = false;
    return err;
}
//PUB FUNCTIONS - PLAY/DOWNLOAD

void PrehrajtoFile::download(string path) {
    CURL* curl_handle;
    ofstream outputFile(path, ios::binary);

    if ((sourceLinks.size() < selectedQuality) || (sourceLinks.size() == 0)) {
        lastError = "Download Error: No links";
        _errorOccured = true;
        return;
    }

    string link = sourceLinks[selectedQuality];

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, link.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, progressCallback);
    if (outputFile.good()) {
        setCursorState(false);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &outputFile);
        curl_easy_perform(curl_handle);
        outputFile.close();
        setCursorState(true);
    }
    else {
        lastError = "Download Error: I/O error";
        _errorOccured = true;
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        return;
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}

void PrehrajtoFile::play() {
    int vlcExists = _chdir("C:\\Program Files\\VideoLAN\\VLC");
    if (vlcExists == -1) {
        lastError = "Play Error: VLC executable not found";
        _errorOccured = true;
        return;
    }

    if ((sourceLinks.size() < selectedQuality) || (sourceLinks.size() == 0)) {
        lastError = "Play Error: No links";
        _errorOccured = true;
        return;
    }
    string cmd = "vlc \"" + sourceLinks[selectedQuality] + "\"";
    system(cmd.c_str());
}