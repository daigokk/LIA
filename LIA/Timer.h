#pragma once

#include <thread>
#include <chrono>
#include <windows.h>
#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")
class Timer
{
private:
	LARGE_INTEGER qpf;
	LARGE_INTEGER qpc_start, qpc_previous;
public:
	int numTimers = 0;
	Timer()
	{
		this->numTimers++;
		if (this->numTimers == 1)
			timeBeginPeriod(1);
		this->start();
	}

	~Timer()
	{
		this->numTimers--;
		if (this->numTimers == 0)
			timeEndPeriod(1);
	}

	static void sleepFor(const double sec)
	{	// sec秒Sleepする
		std::this_thread::sleep_for(std::chrono::microseconds((int)(sec * 1e6)));
	}

	void start()
	{	// カウンタスタート
		QueryPerformanceFrequency(&this->qpf);
		QueryPerformanceCounter(&this->qpc_start);
		qpc_previous.QuadPart = qpc_start.QuadPart;
	}

	void stop()
	{

	}

	double sleepUntil(const double theTimeSec)
	{
		LARGE_INTEGER qpc_next, qpc_now;
		qpc_next.QuadPart = (LONGLONG)(theTimeSec * this->qpf.QuadPart + this->qpc_start.QuadPart);
		while (1)
		{
			QueryPerformanceCounter(&qpc_now);
			if (qpc_next.QuadPart <= qpc_now.QuadPart)
			{
				break;
			}
			//std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
		return (double)(qpc_now.QuadPart - this->qpc_start.QuadPart)
			/ (double)this->qpf.QuadPart;
	}

	double sleepFromPreviousFor(const double sec)
	{
		LARGE_INTEGER qpc_next, qpc_now;
		qpc_next.QuadPart = (LONGLONG)(sec * this->qpf.QuadPart + this->qpc_previous.QuadPart);
		if (10e-3 < sec)
		{
			while (1)
			{
				QueryPerformanceCounter(&qpc_now);
				if (qpc_next.QuadPart <= qpc_now.QuadPart)
				{
					break;
				}
				//std::this_thread::sleep_for(std::chrono::microseconds(100));
			}
		}
		else
		{
			while (1)
			{
				QueryPerformanceCounter(&qpc_now);
				if (qpc_next.QuadPart <= qpc_now.QuadPart)
				{
					break;
				}
			}
		}
		
		this->qpc_previous.QuadPart = qpc_next.QuadPart;
		return (double)(qpc_now.QuadPart - this->qpc_start.QuadPart)
			/ (double)this->qpf.QuadPart;
	}

	double elapsedSec()
	{
		LARGE_INTEGER qpc_now;
		QueryPerformanceCounter(&qpc_now);
		return (double)(qpc_now.QuadPart - this->qpc_start.QuadPart)
			/ (double)this->qpf.QuadPart;
	}

	double elapsedFromPreviousSec()
	{
		static LARGE_INTEGER static_qpc_previous = qpc_start;
		LARGE_INTEGER qpc_now;
		QueryPerformanceCounter(&qpc_now);
		double ret = (double)(qpc_now.QuadPart - static_qpc_previous.QuadPart)
			/ (double)this->qpf.QuadPart;
		static_qpc_previous = qpc_now;
		return ret;
	}
};
