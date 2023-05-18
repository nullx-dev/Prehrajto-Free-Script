#include <iostream>
#include <sstream>
#include <fstream>
#include <Windows.h>
#include <vector>
#include <direct.h>
#include <curl/curl.h>

using namespace std;

HANDLE col;

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
    stringstream ss(response);
    string line;
    bool got = false;
    bool end = false;
    string links;
    while (getline(ss, line)) {
        if (line.find("var tracks = [") != string::npos) {
            end = true;
        }
        if (got && !end) {
            links = links + line;
        }
        if (line.find("var sources") != string::npos) {
            got = true;
        }
    }

    char* pos = nullptr;

    char* ptr = strtok_s((char*)links.c_str(), "}", &pos);
    int linknum = 0;
    vector<string> out;

    while (ptr != NULL) {
        linknum++;
        out.push_back(ptr);
        ptr = strtok_s(NULL, "}", &pos);
    }
    linknum--;
    out.pop_back();
    *lnknum = linknum;

    for (int i = 0; i < linknum; i++) {
        size_t start = out[i].find("https://");
        size_t end = out[i].find_last_of('\"');
        outlink->push_back(out[i].substr(start, end - start));

        start = out[i].find("'");
        end = out[i].find_last_of("'");
        quality->push_back(out[i].substr(start + 1, end - start - 1));
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
    cout <<   " ver. 2.0.0 beta                                                                                developed by Null-X" << endl;
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
        string filename;
        stdWrite(2, "Please enter file name (without suffix): ", false);
        cin >> filename;
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