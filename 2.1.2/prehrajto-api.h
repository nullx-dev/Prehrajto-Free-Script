#pragma once
#include <iostream>
#include <vector>
#include <Windows.h>

using namespace std;

class PrehrajtoFile {
private:
	//ERRORS
	bool _errorOccured = false;
	string lastError;
	//LINKS AND QUALITIES
	int linkNum = 0;
	int selectedQuality = 0;
	string lnk;
	vector<string> sourceLinks;
	vector<string> qualities;
	//OTHER
	HANDLE col;

	void getSourceLink();
	void setCursorState(bool state);

public:
	PrehrajtoFile(string link);
	string getLastError_str();
	bool errorOccured();

	void download(string path);
	void play();
	
	string getFileName();
	vector<string> getQualities_str();
	int getQualities_int();
	void selectQuality(int q);
};