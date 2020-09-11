#ifndef __CardioAgent_h__
#define __CardioAgent_h__

#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Agent.h"
#include "CardioParams.h"

//#include "Parameters.h"

template<class GenericParams>
class PersonPums;

class Random;
class CardioCounter;

const double ldl_thres = 130.0;
const double hdl_thres = 40.0;
const int statin_age_min = 40;
const int statin_age_max = 75;
const int num_years = 10;

class CardioAgent : public Agent<CardioParams>
{
public: 
	typedef std::pair<double, double> PairDD;
	typedef std::pair<int, int> PairInts;
	typedef std::map<std::string, PairDD> PairMap;
	typedef std::map<int, std::map<std::string, PairDD>> RiskMap;
	typedef std::map<int, double>MapDbl;
	typedef std::pair<double, short int> PairDblInt;

	typedef std::pair<double, EET::RiskFactors> WeightRiskPair;
	typedef std::pair<int, std::vector<WeightRiskPair>> PairIntVector;
	typedef std::vector<WeightRiskPair> VectorWeightRisks;
	typedef std::vector<PairIntVector> VectorCumulativeProbability;

	CardioAgent();
	CardioAgent(const PersonPums<CardioParams> *, std::shared_ptr<CardioParams>, CardioCounter *, Random *);

	virtual ~CardioAgent();

	void setNHANESOrigin(short int);
	void setNHANESEduCat(short int);
	void setRiskFactors(short int, EET::RiskFactors);

	void update(int, int, int, int, bool);
	void computeTenYearCHDRisk(std::string);
	void processChdEvent(int, int, int, int, bool);
	void resetAttributes();

	short int getNHANESAgeCat() const;
	short int getNHANESOrigin() const;
	short int getNHANESEduCat() const;
	short int getRiskStrata() const;
	short int getRiskStrata(bool) const;

	std::string getAgentType() const;
	std::string getAgentType1() const;
	std::string getAgentType2() const; 
	std::string getAgentType3() const;
	std::string getAgentType4() const;
	std::string getAgentType5() const;

	std::string getRiskState() const;

	double getRiskFactor(int) const;
	double getPredictedCHDProbability() const;

	double getMeanAge(std::string) const;
	double getMeanHDL(std::string) const;
	double getMeanTChols(std::string) const;
	double getMeanSysBp(std::string) const;
	double getPercentSmoking(std::string) const;
	double getPercentSmoking(bool, int) const;
	double getPercentHTN(std::string) const;

	bool isDead() const;
	bool isOnStatin() const;
	bool isStatinQualified() const;
	
private:
	void computeFraminghamCHDRisk(std::string);
	void computeScoreFatalCHDRisk(std::string);
	void computeRiskFactorDifference(std::string);

	void updateAge();
	void updateRiskFactors(int, int, int, int, bool);
	
	void resetAge();
	void resetEducation();
	void resetRiskFactors();

	void setNHANESAgeCat();

	//void setRiskState();
	void setRiskStrata(short int);
	//void setNewRiskFactors(int &, int);
	
	void smokingTaxIntervention(bool, int, int);
	void statinIntervention(int, int, int, bool, int);
	void educationIntervention();

	void updateRisksTaxIntervention(int);
	//void updateRisksStatinsIntervention(int, int, int, bool);
	void updateRisksStatinsIntervention(int, int, int, int, bool);
	void updateRisksEducationIntervention(double);

	void deathFatalChd(std::string, int);

	bool isNewRiskGreater(EET::RiskFactors);
	
	double getWeightedSumFramingham(int);
	double getWeightedSumScore();
	double getSumRisks(EET::RiskFactors);
	
	EET::Framingham getBetaFHS();
	EET::Score getBetaScore();

	double getTenYrSurvivalFramingham();
	double getTenYrTotalCHDSurvivalARIC();
	double getTenYrFatalCHDSurvivalARIC();
	double getPercentChangeSmoking(int);
	double getSmokingProbability(bool, double, int);
	double getStatinUptakeProbability();
	double getEstimatedStatinsUse(double, int );

	double getLDLReductionPercent();
	double getHDLReductionPercent();
	double getTriglycerideReductionPercent();

	double getPercentEduDifference();
	double getYLL();
	double getLifeExpectancy();

	std::shared_ptr<CardioParams>parameters;
	CardioCounter *counter;
	Random *random;

	short int initAge;
	short int nhanes_ageCat, nhanes_ageCat3;
	short int nhanes_org;
	short int nhanes_edu, init_edu;

	short int rfStrata, initRiskStrata;
	EET::RiskFactors chart, initChart;

	double pTotalCHD, pFatalCHD;
	
	bool dead;
	short int deathYear;
};

#endif __CardioAgent_h__