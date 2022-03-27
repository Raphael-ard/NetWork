#pragma once
#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <atomic>
#include <semaphore>
#include <list>
#include <functional>
#include <mutex>

namespace NetWork
{
	class threadPool
	{
	public:
		threadPool(int num);
		~threadPool(void);

		void addTask(std::function<void(void)> task);
		void addTasks(std::vector<std::function<void(void)>> tasks);

	private:
		std::function<void(void)>					getTask(void);

	private:
		std::atomic<bool>							_is_exit{ false };
		std::counting_semaphore<12>					_semaphore;
		std::list<std::function<void(void)>>		_task_queue;
		std::mutex									_task_mx;
		int											_max_thread_cnt;
		std::atomic<int>							_thread_alive_cnt{ 0 };
	};
}

#endif // !_THREADPOOL_H_
