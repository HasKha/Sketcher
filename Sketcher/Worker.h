#pragma once

#include <thread>
#include <functional>
#include <queue>

class Worker {
public:
	// Enqueue the task. Returns false if the task is not accepted.
	static bool Do(std::function<void()> task);

	// Stop the worker once the current tasks are completed. Warning: blocking.
	static void Stop();

private:
	static Worker& Instance() {
		static Worker worker;
		return worker;
	}

	Worker();

	bool should_stop;
	std::thread thread;
	std::queue<std::function<void()>> tasks;
};
