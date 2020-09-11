#ifndef __DepressionModel_h__
#define __DepressionModel_h__

#include <vector>
#include <map>
#include <memory>
#include "PopBrewer.h"
#include "DepressionAgent.h"
#include <boost/math/distributions/chi_squared.hpp>
#include <boost/range/algorithm.hpp>
//#include "DepressionHousehold.h"

class DepressionParams;
class DepressionCounter;
class DepressionHousehold;
class Random;

template<class GenericParams>
class PersonPums;

template<class GenericParams>
class HouseholdPums;

class DepressionModel : public PopBrewer<DepressionParams>
{
public:
	typedef std::map<std::string, std::vector<DepressionAgent *>> AgentMap;
	typedef std::pair<double, int> PairDblInt;
	typedef std::vector<PairDblInt> VecPairDblInt;
	typedef std::map<std::string, VecPairDblInt> MapVecPair;

	DepressionModel();
	DepressionModel(const char*, const char*, int, int);

	virtual ~DepressionModel();

	void start(int);

	void addHousehold(const HouseholdPums<DepressionParams> *, int);
	void addAgent(const PersonPums<DepressionParams> *);

	DepressionCounter *getCounter() const;
	int getPopulation(std::string);

	void setSize(int);
	
	void resize();
	void clearList();
	

private:
	void createPopulation(Area<DepressionParams> *);
	void computeWageGap();
	void setDepressionType(AgentMap *, std::string);
	void execute(std::string);

	bool checkFit(const VecPairDblInt*, std::string[], std::string, int, int);
	bool chiSquareTest(const std::vector<std::pair<double, double>>, int, int);

	void resetPersonCount(int);
	void resetPersonCount(std::string [], std::string);

	std::string prefixByDepression();
	std::string prefixByIPratio();
	std::string prefixBySexAge();


	DepressionCounter *count;
	Random *random;

	AgentMap m_agents;
	//std::vector<DepressionHousehold *> households;

};
#endif