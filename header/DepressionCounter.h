#ifndef __DepressionCounter_h__
#define __DepressionCounter_h__

#include <memory>
#include <iostream>
#include <fstream>
#include <map>
#include "Counter.h"

class DepressionParams;

class DepressionCounter : public Counter<DepressionParams>
{
public:
	typedef std::map<std::string, std::map<std::string, double>> HashMap;

	DepressionCounter();
	DepressionCounter(std::shared_ptr<DepressionParams>);

	virtual ~DepressionCounter();

	void initialize();
	void computeOutcomes(std::string);

	void output(std::string);
	void clear();
private:
	void computePrevalence(std::string);
	void outputPrevalence(std::string);

	std::shared_ptr<DepressionParams> param;
	HashMap m_depressionPrevalence, m_popBySexAgeDepression;
};
#endif