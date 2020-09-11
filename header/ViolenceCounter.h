#ifndef __ViolenceCounter_h__
#define __ViolenceCounter_h__

#include <memory>
#include <map>
#include <string>
#include "Counter.h"
#include "ViolenceAgent.h"

class ViolenceParams;

struct Risk
{
	double val;
	double lowLim;
	double upLim;

	Risk &operator+=(const Risk &r)
	{
		this->val += r.val;
		this->lowLim += r.lowLim;
		this->upLim += r.upLim;

		return *this;
	}

	Risk &operator/=(const int num_trials)
	{
		this->val /= num_trials;
		this->lowLim /= num_trials;
		this->upLim /= num_trials;

		return *this;
	}

	friend std::ostream &operator<<(std::ostream &out, const Risk &r)
	{
		out << r.val << "," << r.lowLim << "," << r.upLim;
		return out;
	}

};

struct Outcomes
{
	double value[NUM_TREATMENT];
	Risk diff;
	Risk ratio;
};


class ViolenceCounter : public Counter<ViolenceParams>
{
public:
	typedef std::map<int, int> MapInts;
	typedef std::map<int, double> MapDbls;
	typedef std::map<std::string, std::map<std::string, double>> MapStringDbls;
	
	typedef std::map<int, Outcomes> MapOutcomes;
	typedef std::vector<std::string> Pool;
	typedef std::pair<double, double> Pair;
	typedef std::vector<ViolenceAgent*> AgentListPtr;
	typedef std::vector<double> VectorDbls;
	typedef std::map<std::string, Pair> MapPair;

	ViolenceCounter();
	ViolenceCounter(std::shared_ptr<ViolenceParams>);

	virtual ~ViolenceCounter();

	void initialize();
	void clear();
	void output(std::string, std::string);

	void addPtsdCount(int, int, int);
	void addPrevalence(int);
	void addPrevalence(MapPair *, std::string, std::string);
	void addCbtReferredNonPtsd();
	void addPtsdResolvedCount(int, int, int);
	void addCbtReach(int, int);
	void addSprReach(int, int);
	void addCbtCount(ViolenceAgent*, int, int);
	void addSprCount(ViolenceAgent*, int, int);
	void addNaturalDecayCount(int);

	void computeOutcomes(int, int);
	void computeCostEffectiveness(AgentListPtr *);

	//MV model
	
	VectorDbls getPrevalence(int, int);
	double getTotalPrevalence();
	double getCbtUptake(int);
	double getSprUptake(int);
	double getNaturalDecayUptake(int);
	double getYLD(double, int);
	double getTotalCost(int);

private:
	void initPtsdCounter();
	void initTreatmentCounter(int);

	void computePrevalence(int, int);
	void computeRecovery(int, int);
	void computeReach(int);

	void computeDALYs(AgentListPtr *);
	void computeTotalCost();
	void computeAverageCost();
	
	Pair computeError(double *, double *);
	Risk computeDiff(double *, double);
	Risk computeRatio(double *, double);

	void outputHealthOutcomes();
	void outputReach();
	void outputCostEffectiveness();
	void outputPtsdDistribution();

	void outputPrevalence();

	void clearCounter();

	std::shared_ptr<ViolenceParams> param;

	double totalPrevalence;
	//Counters for Mass Violence Model1(PTSD and PTSD resolved)
	int nonPtsdCountSC;
	MapInts m_ptsdCount[NUM_TREATMENT][NUM_PTSD], m_ptsdResolvedCount[NUM_TREATMENT][NUM_PTSD];
	MapDbls m_totPrev[NUM_TREATMENT][NUM_PTSD], m_totRecovery[NUM_TREATMENT][NUM_PTSD];
	
	MapInts m_cbtReach[NUM_TREATMENT], m_sprReach[NUM_TREATMENT];
	MapInts m_cbtCount, m_sprCount, m_ndCount; //overall count of CBT and SPR treatment
	MapInts m_totCbt[NUM_TREATMENT][NUM_CASES], m_totSpr[NUM_TREATMENT][NUM_CASES];
	
	MapOutcomes m_prevalence, m_recovery;
	MapDbls m_totDalys, m_totPtsdFreeWeeks, m_totCost, m_avgCost;
	VectorDbls totalReach[NUM_TREATMENT];

	//Counter for mass-violence model2
	std::map<std::string, MapStringDbls> m_totalPrevalence2, m_totalPtsdCases, m_totalPopulation;
};

#endif