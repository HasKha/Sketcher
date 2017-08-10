#include "Worker.h"

bool Worker::Do(std::function<void()> task) {
	if (Instance().should_stop) return false;
	Instance().tasks.push(task);
	return true;
}

void Worker::Stop() {
	Instance().should_stop = true; // do not accept more jobs
	Instance().thread.join();
}

Worker::Worker() {
	should_stop = false;
	thread = std::thread([this]() {
		while (!should_stop) {
			while (!tasks.empty()) {
				tasks.front()();
				tasks.pop();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});
}
