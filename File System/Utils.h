#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

class Utils {
public:

	/*
		Splits given string using given delimiter string.
		Returns vector of tokens.
	*/
	static vector<string> split(const string &str, const string &delim);

	/*
		Utility function for checking whether given opening mode is valid or not.
		Valid modes: 'r', 'w', 'a'.
	*/
	static bool isCorrect(char mode) {
		return mode == 'r' || mode == 'w' || mode == 'a';
	}

};

#endif
