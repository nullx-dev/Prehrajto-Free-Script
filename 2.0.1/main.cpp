#include <iostream>
#include <fstream>
#include <Windows.h>
#include <vector>
#include <direct.h>
#include <curl/curl.h>
#include <regex>

using namespace std;

HANDLE col;

size_t filenameCallback(void* contents, size_t size, size_t nmemb, std::wstring* response) {
    size_t totalSize = size * nmemb;
    size_t wstrSize = totalSize / sizeof(wchar_t);
    const wchar_t* data = static_cast<const wchar_t*>(contents);
    response->append(data, wstrSize);
    return totalSize;
}

inline bool fileExists(string path){
    ifstream stream(path.c_str());
    const bool good = stream.good();
    stream.close();
    return good;
}

inline void setCursorState(bool state) {
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

static size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
    return written;
}

void stdWrite(int mode, string txt, bool el) {
    switch (mode) { //0 = INFO; 1 = ERR; 2 = QUESTION; 3 = SUCESS
    case 0:
        SetConsoleTextAttribute(col, 7);
        cout << "[*] ";
        break;

    case 1:
        SetConsoleTextAttribute(col, 4);
        cout << "[-] ";
        break;

    case 2:
        SetConsoleTextAttribute(col, 1);
        cout << "[?] ";
        break;

    case 3:
        SetConsoleTextAttribute(col, 2);
        cout << "[+] ";
        break;
    }
    SetConsoleTextAttribute(col, 7);

    if (el) {
        cout << txt << endl;
    }
    else {
        cout << txt;
    }
}

size_t writeCallback(char* buffer, size_t size, size_t nmemb, std::string* userData) {
    userData->append(buffer, size * nmemb);
    return size * nmemb;
}

int progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    static const int progressBarWidth = 70;

    // Calculate the percentage of completion
    int progress = static_cast<int>((dlnow / dltotal) * 100.0);
    //cout << progress << endl;

    // Calculate the number of characters to fill in the progress bar
    int pos = progressBarWidth * (dlnow / dltotal);

    // Clear the console
    std::cout << "\r";

    // Print the progress bar
    std::cout << "[";
    for (int i = 0; i < progressBarWidth; ++i) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    if (progress > 0) {
        cout << "] " << progress << "%\r";
    }
    std::cout.flush();

    return 0;
}

string getFileName(string link) {
    regex fNameRegex("https:\\/\\/prehrajto\\.cz\\/(.*?)\\/");
    smatch match;
    string fName;

    if (regex_search(link, match, fNameRegex)) {
        fName = match[1].str();
    }
    else {
        stdWrite(1, "No filename found. Ensure you have entered correct site", true);
        system("pause");
        exit(0x00);
    }
    return fName;
}

void getSourceLink(string lnk, vector<string>* outlink, vector<string>* quality, int* lnknum) {
    string response;
    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        stdWrite(1, "Failed to create cURL handle", true);
        system("pause");
        exit(0x00);
    }
    curl_easy_setopt(curl, CURLOPT_URL, lnk.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK){
        stdWrite(1, "Failed to fetch website: " + (string)curl_easy_strerror(res), true);
        system("pause");
        exit(0x00);
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
            outlink->push_back((*linkIter)[1].str());
        }
        
        if (outlink->empty()) {
            stdWrite(1, "No links found. Ensure you have entered correct site", true);
            system("pause");
            exit(0x00);
        }
        
        regex qualityRegex("\'(.*?)\'");
        sregex_iterator qualityIter(sourcesVar.begin(), sourcesVar.end(), qualityRegex);
        sregex_iterator qend;

        for (; qualityIter != qend; ++qualityIter) {
            quality->push_back((*qualityIter)[1].str());
        }

        if (quality->empty()) {
            stdWrite(1, "No quality labels found. Ensure you have entered correct site", true);
            system("pause");
            exit(0x00);
        }

        *lnknum = outlink->size();
    }
    else {
        stdWrite(1, "Sources variable not found. Ensure you have entered correct site", true);
        system("pause");
        exit(0x00);
    }
}

void download(string link, string path) {
    CURL* curl_handle;
    static const char* pagefilename = path.c_str();
    FILE* pagefile;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, link.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, progressCallback);

    fopen_s(&pagefile, pagefilename, "wb");
    if (pagefile) {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
        curl_easy_perform(curl_handle);
        fclose(pagefile);
    }
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}

int main() {
    string link;
    int mode;
    int selectedQuality;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    col = GetStdHandle(STD_OUTPUT_HANDLE);
    cout << R"(  ____           _                _ _              _____                ____            _       _     ____    ___  )" << endl;
    cout << R"( |  _ \ _ __ ___| |__  _ __ __ _ (_) |_ ___       |  ___| __ ___  ___  / ___|  ___ _ __(_)_ __ | |_  |___ \  / _ \ )" << endl;
    cout << R"( | |_) | '__/ _ \ '_ \| '__/ _` || | __/ _ \ _____| |_ | '__/ _ \/ _ \ \___ \ / __| '__| | '_ \| __|   __) || | | |)" << endl;
    cout << R"( |  __/| | |  __/ | | | | | (_| || | || (_) |_____|  _|| | |  __/  __/  ___) | (__| |  | | |_) | |_   / __/ | |_| |)" << endl;
    cout << R"( |_|   |_|  \___|_| |_|_|  \__,_|/ |\__\___/      |_|  |_|  \___|\___| |____/ \___|_|  |_| .__/ \__| |_____(_)___/ )" << endl;
    cout << R"(                               |__/                                                      |_|                       )" << endl;
    cout <<   " ver. 2.0.1                                                                                     developed by Null-X" << endl;
    cout << endl;
    stdWrite(2, "Please enter video link: ", false);
    cin >> link;

    if (link == "") {
        stdWrite(1, "Link cannot be empty", true);
        system("pause");
        exit(0x00);
    }
    int linknum;
    vector <string> quality;
    vector <string> links;
    getSourceLink(link, &links, &quality, &linknum);

    for (int i = 0; i < linknum; i++) {
        stdWrite(0, to_string(i) + ": " + quality[i], true);
    }
    stdWrite(2, "Please select quality by selecting number: ", false);
    cin >> selectedQuality;

    if (selectedQuality > linknum) {
        stdWrite(1, "Quality number is not valid, selecting " + to_string(--linknum), true);
        selectedQuality = linknum;
    }

    stdWrite(2, "1 - Play; 2 - Download: ", false);
    cin >> mode;

    if (mode == 2) {
        string filename = getFileName(link);
        setCursorState(false);
        download(links[selectedQuality], filename + ".mp4");
        cout << endl;
        setCursorState(true);
        stdWrite(3, "Done. Thank you for using :)", true);
        system("pause");
    }
    else {
        int vlcExists = _chdir("C:\\Program Files\\VideoLAN\\VLC");
        if (vlcExists == -1) {
            stdWrite(1, "VLC executable not found", true);
            system("pause");
            exit(0x00);
        }
        string cmd = "vlc \"" + links[selectedQuality] + "\"";
        system(cmd.c_str());
        stdWrite(3, "Thank you for using :)", true);
        system("pause");
    }

}
