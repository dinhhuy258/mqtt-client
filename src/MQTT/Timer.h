#ifndef _TIMER_H_
#define _TIMER_H_
#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

class Timer
{
	public:
		Timer() = default;
		~Timer() = default;
		template <class Callable, class... Arguments>
		void Wait(unsigned int ms, bool async, bool loop, Callable&& timerCallback, Arguments&&... args)
		{
			std::function<typename std::result_of<Callable(Arguments...)>::type()> task(std::bind(std::forward<Callable>(timerCallback), std::forward<Arguments>(args)...));
			if (async)
			{
				std::thread([ms, loop, task]()
				{
				loop_async:
					std::this_thread::sleep_for(std::chrono::milliseconds(ms));
					task();
					if (loop)
					{
						goto loop_async;
					}
				}).detach();
			}
			else
			{
			loop_sync:
				std::this_thread::sleep_for(std::chrono::milliseconds(ms));
				task();
				if (loop)
				{
					goto loop_sync;
				}
			}
		}
};

#endif //_TIMER_H_
