#include <iostream>
#include <Windows.h>
#include <vector>
#include "prehrajto-api.h"

using namespace std;

HANDLE col;

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

inline bool fileExists(string path){
    ifstream stream(path.c_str());
    const bool good = stream.good();
    stream.close();
    return good;
}

int main() {
    string link;
    cout << R"(  ____           _                _ _              _____                ____            _       _     ____    _   ____  
 |  _ \ _ __ ___| |__  _ __ __ _ (_) |_ ___       |  ___| __ ___  ___  / ___|  ___ _ __(_)_ __ | |_  |___ \  / | |___ \ 
 | |_) | '__/ _ \ '_ \| '__/ _` || | __/ _ \ _____| |_ | '__/ _ \/ _ \ \___ \ / __| '__| | '_ \| __|   __) | | |   __) |
 |  __/| | |  __/ | | | | | (_| || | || (_) |_____|  _|| | |  __/  __/  ___) | (__| |  | | |_) | |_   / __/ _| |_ / __/ 
 |_|   |_|  \___|_| |_|_|  \__,_|/ |\__\___/      |_|  |_|  \___|\___| |____/ \___|_|  |_| .__/ \__| |_____(_)_(_)_____|
                               |__/                                                      |_|                            )" << endl;
    cout <<   " ver. 2.1.2                                                                                     developed by Null-X" << endl;
    cout << endl;

    col = GetStdHandle(STD_OUTPUT_HANDLE);

    stdWrite(2, "Please enter video link: ", false);
    cin >> link;

    if (link == "") {
        stdWrite(1, "Link cannot be empty", true);
        system("pause");
        exit(0x00);
    }
    
    PrehrajtoFile p_file(link);
    if (p_file.errorOccured()) {
        stdWrite(1, p_file.getLastError_str(), true);
        system("pause");
        exit(0x00);
    }

    vector<string> quality = p_file.getQualities_str();
    int linknum = p_file.getQualities_int();

    for (int i = 0; i < linknum; i++) {
        stdWrite(0, to_string(i) + ": " + quality[i], true);
    }
    stdWrite(2, "Please select quality by selecting number: ", false);

    int selectedQuality;
    cin >> selectedQuality;
    p_file.selectQuality(selectedQuality);
    if (p_file.errorOccured()) {
        stdWrite(1, p_file.getLastError_str(), true);
        system("pause");
        exit(0x00);
    }

    stdWrite(2, "1 - Play; 2 - Download: ", false);
    int mode;
    cin >> mode;

    if (mode == 2) {
        p_file.download(p_file.getFileName());
        if (p_file.errorOccured()) {
            stdWrite(1, p_file.getLastError_str(), true);
            system("pause");
            exit(0x00);
        }
        stdWrite(3, "Done. Thank you for using :)", true);
        system("pause");
    }
    else {
        p_file.play();
        if (p_file.errorOccured()) {
            stdWrite(1, p_file.getLastError_str(), true);
            system("pause");
            exit(0x00);
        }
        stdWrite(3, "Done. Thank you for using :)", true);
        system("pause");
    }
}
