#ifndef __DepressionAgent_h__
#define __DepressionAgent_h__

#include <memory>
#include <string>
#include <vector>
#include "Agent.h"

class DepressionParams;
class Random;
class DepressionCounter;

template<class GenericParams>
class PersonPums;

namespace Depression
{
	const double first_q = 0.25;
	const double second_q = 0.5;
	const double third_q = 0.75;
	const double fourth_q = 1.0;
}

class DepressionAgent : public Agent<DepressionParams>
{
public:
	typedef std::tuple<double, double, double, double>Tuple;
	typedef std::pair<double, int> PairDblInt;
	typedef std::vector<PairDblInt> VecPairDblInt;
	
	DepressionAgent();
	DepressionAgent(std::shared_ptr<DepressionParams>, const PersonPums<DepressionParams> *, Random *, DepressionCounter *, int, int);
	virtual ~DepressionAgent();

	void update();

	void setAgeCat();
	void setIncomeToPovertyRatio(double, int, int, int);
	void setDepressionType(short int);
	void setDepressionSymptoms();

	std::string getAgentID() const;
	short int getAgeCat() const;
	short int getDepressionType() const;

	std::string getAgentType1() const;
	std::string getAgentType2() const;
	std::string getAgentType3() const;
	std::string getAgentType4() const;
	std::string getAgentType5() const;

	DepressionCounter *getCounter();

private:
	void reduceWageGap();

	Random *random;
	DepressionCounter *counter;
	std::shared_ptr<DepressionParams> parameters;

	std::string agentIdx;
	short int ageCat;
	double ipRatio;
	short int ipRatioTag;
	short int depressionType;
	double symptoms;

};

#endif