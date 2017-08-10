#pragma once

#include <imgui.h>

#include <string>
#include <sstream>
#include <time.h>

class Log {
public:
	static Log& Instance();

	void Clear();

	void log(const char* fmt, ...);
	void vlog(const char* fmt, va_list args);

	void Draw(const char* title, bool* p_open);

	static bool print_enabled;

private:
	static Log* instance_;

	Log() {};
	~Log() {};

	ImGuiTextBuffer Buf;
	ImVector<int> LineOffset;
	bool ScrollToBottom;
};

void print(const char* fmt, ...);
