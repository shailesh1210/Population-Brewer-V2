#ifndef __Random_h__
#define __Random_h__

#include <ctime>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/poisson_distribution.hpp>

class Random
{
public:
	Random();
	virtual ~Random();

	double uniform_real_dist();
	double uniform_real_dist(double, double);

	int random_int(int, int);

	double normal_dist(double, double);
	int poisson_dist(double);

private:
	boost::mt19937 rng;
};
#endif