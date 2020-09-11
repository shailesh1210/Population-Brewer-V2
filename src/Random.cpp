#include "Random.h"

Random::Random() : rng((uint32_t)time(NULL))
{
}

Random::~Random()
{
}

double Random::uniform_real_dist()
{
	boost::random::uniform_real_distribution<>dist(0, 1);
	boost::random::variate_generator<boost::mt19937&, boost::random::uniform_real_distribution<>>generator(rng, dist);

	return generator();
}

double Random::uniform_real_dist(double min, double max)
{
	boost::random::uniform_real_distribution<>dist(min, max);
	boost::random::variate_generator<boost::mt19937&, boost::random::uniform_real_distribution<>>generator(rng, dist);

	return generator();
}

int Random::random_int(int min, int max)
{
	boost::random::uniform_int_distribution<>dist(min, max);
	boost::random::variate_generator<boost::mt19937&, boost::random::uniform_int_distribution<>>generator(rng, dist);

	return generator();
}


double Random::normal_dist(double mean, double std_err)
{
	boost::random::normal_distribution<>dist(mean, std_err);
	boost::random::variate_generator<boost::mt19937&, boost::random::normal_distribution<>>generator(rng, dist);

	return generator();
}

int Random::poisson_dist(double mean)
{
	boost::random::poisson_distribution<int>dist(mean);
	boost::random::variate_generator<boost::mt19937&, boost::random::poisson_distribution<>>generator(rng, dist);

	return generator();
}
