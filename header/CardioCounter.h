#ifndef __CardioCounter_h__
#define __CardioCounter_h__

#include <memory>
#include <map>
#include <string>
#include "Counter.h"

class CardioAgent;
class CardioParams;

class CardioCounter : public Counter<CardioParams>
{
public:
	typedef std::pair<double, double> PairDouble;
	typedef std::pair<int, int> PairInts;
	typedef std::map<std::string, double> MapStringDbl;
	typedef std::map<std::string, std::vector<double>> CHDRiskScores, IncomeByRace;
	typedef std::map<std::string, std::map<std::string, CHDRiskScores>> CHDRisksRaceGender;
	typedef std::map<std::string, std::map<int, double>> RiskFacMap;
	typedef std::map<std::string, RiskFacMap> MeanRiskFactors, RiskFactorDiff;
	typedef std::map<std::string, std::map<std::string, RiskFacMap>> TotalRiskFactors, StdDevRiskFac;
	typedef std::map<std::string, std::map<std::string, MapStringDbl>> TotalCHDScores, YearsLifeLost;
	typedef std::map<std::string, MapStringDbl> PopulationTotals;

	typedef std::map<std::string, std::map<std::string, std::map<PairInts, double>>> SmokingChangeProportion;
	typedef std::map<std::string, MapStringDbl> SmokerCount;

	typedef std::vector<double> StatinsUseByYear;

	CardioCounter();
	CardioCounter(std::shared_ptr<CardioParams>);

	virtual ~CardioCounter();

	void initialize();

	void clearRiskFactor();
	void clearCHDRisks(std::string);
	void clearCHDdeaths();

	void output(std::string);

	//void resetPersonCount(std::string);

	//CVD model
	void addRiskFactorCount(std::string);
	void addRiskFactorDifference(std::string, std::string, int, double);
	void addTenYearTotalCHDRisk(std::string, std::string, std::string, double);
	void addTenYearFatalCHDRisk(std::string, std::string, std::string, double);
	void addIncome(std::string, double);
	void addRiskFactors(CardioAgent *, std::string, std::string);
	void addStatinsUsage(CardioAgent *, std::string, std::string, int, int, int, bool);
	void addChdDeaths(std::string, std::string, std::string);
	void addYearsLifeLost(std::string, std::string, std::string, double);

	void addSmokingChange(std::string, std::string, PairInts);
	void addSmokers(std::string, std::string);

	void computeSmokeChange();

	void computeMeanRisk(std::string);
	void sumOutcomes(std::string);

	double getMeanRiskFactor(std::string, std::string, int);
	double getChdDeaths(std::string, int);

	//void outputRiskFactorPercent();

private:
	void initRiskFacCounter();

	void accumulateRiskFacs(CardioAgent *, std::string, std::string);
	RiskFacMap computeMean();
	void computeMean(RiskFacMap &, std::string);

	void sumMeanRiskFactors(std::string);
	void sumStdDevRiskFactors(std::string);
	void sumTenYearTotalCHDRisk(std::string);
	void sumTenYearFatalCHDRisk(std::string);
	void sumTenYearCHDRisk(std::map<std::string, CHDRiskScores> &, TotalCHDScores &, std::string, std::string);

	//CVD model outputs

	void outputMeanRiskFactor(std::string);
	void outputTenYearTotalCHDRisk(std::string);
	void outputTenYearFatalCHDRisk(std::string);
	void outputTenYearCHDRisk(TotalCHDScores &, std::string, std::string);
	void outputStatinsUsage(std::string);
	void outputCostEffectiveness(std::string);

	void outputPercentSmokeChange(std::string);

	//Cost Effectiveness methods
	PairDouble computeTotalCost(std::string, std::string, StatinsUseByYear &);
	double computeDALYs(std::string, std::string);

	PairDouble computeTotalCostStatins(std::string, std::string, StatinsUseByYear &);

	std::string getYear(std::string, std::string);

	void clearRiskFactorCounter();
	void clearCHDRiskCounter(std::string);

	std::shared_ptr<CardioParams> param;

	//Counters for EET model
	IncomeByRace m_incomeByRace;
	RiskFacMap m_sumRiskFac; //, m_meanRiskFac;
	MeanRiskFactors m_meanRiskFac;
	RiskFactorDiff m_sumDiffRiskFac;
	TypeMap m_riskFacCount;
	CHDRisksRaceGender m_totalChdRiskScore, m_fatalChdRiskScore;

	//Mean Risk factors and 10-year CHD risk totals
	TotalRiskFactors m_totalMeanRiskFactors;
	StdDevRiskFac m_stdDevRiskFac;
	TotalCHDScores m_totalFraminghamRisk, m_totalSCORERisk;
	PopulationTotals m_popTotalsByRaceGender, m_popTotalsRaceGenderEdu;
	
	//Fatal CHD deaths and YLLs
	PopulationTotals m_totChdDeathsPerYear, m_totChdDeathsByRaceGender; 
	YearsLifeLost m_totYLL;

	//Statins usage and eligiblity
	std::map<std::string, PopulationTotals> m_statinUsage, m_statinEligibles;
	//PopulationTotals m_statinEligibles;

	//Counter for smoking status change from Smoker to former smoker, Smoker to non smoker
	SmokingChangeProportion proportionSmokeChange, avgPropSmokeChange;
	SmokerCount countSmokers;

};

#endif