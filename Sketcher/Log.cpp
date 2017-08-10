#include "Log.h"

bool Log::print_enabled = true;

Log* Log::instance_ = nullptr;
Log& Log::Instance() {
	if (instance_ == nullptr) instance_ = new Log();
	return *instance_;
}

void print(const char* fmt, ...) {
	if (!Log::print_enabled) return;
	va_list args;
	va_start(args, fmt);
	Log::Instance().vlog(fmt, args);
	va_end(args);
}

void Log::Clear() {
	Buf.clear(); LineOffset.clear();
}

void Log::log(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vlog(fmt, args);
	va_end(args);
}

void Log::vlog(const char* fmt, va_list args) {
	int old_size = Buf.size();
	Buf.appendv(fmt, args);
	for (int new_size = Buf.size(); old_size < new_size; ++old_size) {
		if (Buf[old_size] == '\n') {
			LineOffset.push_back(old_size);
		}
	}
	ScrollToBottom = true;
}

void Log::Draw(const char* title, bool* p_open) {
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiSetCond_FirstUseEver);
	ImGui::Begin(title, p_open);
	if (ImGui::Button("Clear")) Clear();
	ImGui::SameLine();
	bool copy = ImGui::Button("Copy");
	ImGui::Separator();
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (copy) ImGui::LogToClipboard();

	ImGui::TextUnformatted(Buf.begin());

	if (ScrollToBottom)
		ImGui::SetScrollHere(1.0f);
	ScrollToBottom = false;
	ImGui::EndChild();
	ImGui::End();
}
