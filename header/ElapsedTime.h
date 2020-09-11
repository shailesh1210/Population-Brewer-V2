#pragma once


#include <cmath>
#include <chrono>


typedef std::chrono::steady_clock::time_point TimePoint;


class ElapsedTime
{
public:
	ElapsedTime();
	~ElapsedTime();

	void start();
	void stop();
	// A simple function to calculate elapsed time using chrono
	double elapsed_ms();

	TimePoint start_time;
	TimePoint end_time;

	static 	double elapsed_ms(const TimePoint &start, const TimePoint &end);
};

