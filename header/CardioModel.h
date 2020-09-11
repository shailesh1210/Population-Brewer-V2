/*Class definition of CardioModel
* It inherits from PopBrewer class
*
* Always include following methods definition in model class:
* void start();
* void start(int);
* void addHousehold(const HouseholdPums<CardioParams> *, int);
* void addAgent(const PersonPums<CardioParams> *);
* void clearList();
*/

#ifndef __CardioModel_h__
#define __CardioModel_h__

#include <iostream>
#include <vector>
#include <map>
#include <tuple>

#include "CardioAgent.h"
#include "PopBrewer.h"

class CardioParams;

template<class GenericParams>
class PersonPums;

template<class GenericParams>
class HouseholdPums;

class Random;
class CardioCounter;
class ElapsedTime;

class CardioModel : public PopBrewer<CardioParams>
{
public:
	typedef std::pair<double, double> PairDD;
	typedef std::pair<int, double> PairIntDbl;
	
	typedef std::map<std::string, double> MapDbl;
	typedef std::vector<CardioAgent *> AgentList;
	typedef std::multimap<std::string, CardioAgent *> AgentPtr;

	typedef std::pair<double, EET::RiskFactors> WeightRiskPair;
	typedef std::map<int, std::vector<WeightRiskPair>> StrataRiskMap;
	typedef std::map<std::string, StrataRiskMap> AgentStrataRiskMap;

	typedef std::tuple<double, int, EET::RiskFactors> WeightStrataRiskTuple;
	typedef std::map<std::string, std::vector<WeightStrataRiskTuple>> AgentRiskTuple;
	
	CardioModel();
	CardioModel(const char *, const char *, int, int);
	virtual ~CardioModel();

	void start();
	void start(int);

	void addHousehold(const HouseholdPums<CardioParams> *, int);
	void addAgent(const PersonPums<CardioParams> *);

	CardioCounter * getCounter() const;
	
	void setSize(int);
	void clearList();

private:
	void createPopulation(Area<CardioParams> *);
	void computeEducationDifference();
	void setRiskFactors();
	void runModel();

	void educationIntervention(int, bool);
	void noIntervention(int, bool);
	void taxIntervention(int, bool);
	void statinsIntervention(int, bool);
	void taxStatinsIntervention(int, bool);

	void runSubModels(std::string, int, int, int, int, bool);

	void computeTenYearCHDRisk(std::string);
	void executeIntervention(int, int, int, int, bool);
	void processChdEvents(int, int, int, bool);

	void resetAttributes();
	void resetPersonCounter();

	void rounding(std::vector<PairDD>&, double &);
	double rounding(std::vector<WeightStrataRiskTuple>&, double &);

	std::string concatenate(int, int);
	std::string concatenate(int);
	std::string concatenate(std::string, std::string);

	int getPopulation(std::string);
	std::string getInterventionName(int, bool, std::string);
	std::string getEducationInterventionName(int);

	CardioCounter *count;
	Random *random;

	AgentList agentList;
	AgentPtr agentsPtrMap;

};
#endif