#include "ThreadPool.h"
#include <thread>

NetWork::threadPool::
threadPool(int num) : _max_thread_cnt(num), _semaphore(0)
{
	for (int i = 0; i < num; ++i)
	{
		std::thread t([this](void) -> void
			{
				while (true)
				{
					// 获取到信号量，表示当前获取的线程可以执行
					_semaphore.acquire();
					if (_is_exit)
					// 未获取到，当前没有现成执行
						break;
					++_thread_alive_cnt;
					while (true)
					{
						std::unique_lock lck(_task_mx);
						if (_task_queue.size() == 0u)
							break;
						if (auto task = getTask())
						{
							lck.unlock();
							task();
						}
					}
					--_thread_alive_cnt;
				}
			});
		t.detach();
	}
}

NetWork::threadPool::
~threadPool()
{
	_is_exit = true;
	_semaphore.release(_max_thread_cnt);
}

void NetWork::threadPool::
addTask(std::function<void(void)> task)
{
	if (!task)
		return;
	// 加锁
	std::unique_lock lck(_task_mx);
	_task_queue.emplace_back(task);
	// 有多余线程可供执行
	if (_max_thread_cnt - _thread_alive_cnt > 0)
		_semaphore.release(1);
}

void NetWork::threadPool::
addTasks(std::vector<std::function<void(void)>> tasks)
{
	int num = tasks.size();
	if (num == 0u)
		return;
	std::unique_lock lck(_task_mx);
	for (int i = 0; i < num; ++i)
	{
		_task_queue.emplace_back(tasks.at(i));
		if (_max_thread_cnt - _thread_alive_cnt > 0)
			_semaphore.release(1);
	}
}

std::function<void(void)> 
NetWork::threadPool::
getTask(void)
{
	if (_task_queue.size() == 0u)
		return nullptr;
	auto task = _task_queue.front();
	_task_queue.pop_front();
	return task;
}
