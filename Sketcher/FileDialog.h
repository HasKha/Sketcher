#pragma once

#include <vector>
#include <iostream>

std::string file_dialog(const std::vector<std::pair<std::string, 
	std::string>> &filetypes, bool save);

void StringSplit(const std::string &s, std::vector<std::string> &strs, char ch);