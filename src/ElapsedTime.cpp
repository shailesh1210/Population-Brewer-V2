#include "ElapsedTime.h"



ElapsedTime::ElapsedTime()
{
	start_time = std::chrono::steady_clock::now();
	end_time = std::chrono::steady_clock::now();
}


ElapsedTime::~ElapsedTime()
{
}

void ElapsedTime::start()
{
	start_time = std::chrono::steady_clock::now();
	end_time = std::chrono::steady_clock::now();
}

void ElapsedTime::stop()
{
	end_time = std::chrono::steady_clock::now();
}

double ElapsedTime::elapsed_ms() 
{
	return ElapsedTime::elapsed_ms(start_time, end_time) ;
}


double ElapsedTime::elapsed_ms(const TimePoint &start, const TimePoint &end) {
	// If the end is less than the start return a -1 to show there's a timing problem.
	double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	if (elapsed >= 0.0)
		return elapsed;
	return -1.0 ;
}
