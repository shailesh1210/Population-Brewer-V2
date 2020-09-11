#ifndef __IPU_h__
#define __IPU_h__

#include <iostream>
#include <iomanip>
#include <armadillo>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <numeric>

#include "HouseholdPums.h"
#include "PersonPums.h"

//class HouseholdPums;

//class Parameters;

using namespace arma;

template<class GenericParams>
class IPU
{
public:
	typedef std::pair<double, double> PairDD;
	//typedef std::map<std::string, std::vector<PairDD>> ProbMap;
	typedef std::map<std::string, std::map<double,std::vector<PairDD>>> ProbMap;
	typedef std::map<int, std::map<std::string, int>> IndexMap;
	typedef std::map<int, std::vector<int>> ColIndexMap;
	typedef std::map<std::string, double> CountsMap;
	
	typedef std::map<double, HouseholdPums<GenericParams>> HouseholdsMap;

	IPU(HouseholdsMap *, const std::vector<double>&, bool);
	virtual ~IPU();
	
	void start();

	bool success();
	const ProbMap *getHHProbability() const;
	double getHHCount(std::string) const;
	void clearMap();
	
private:

	void initialize();
	void solve(int, int);
	void mapIndexByType(int &);
	void mapNonZeroRowIndex(int);
	double getColWeightSum(int);
	void computeProbabilities();
	void roundWeights(std::map<std::string, double> &);
	void clear();

	std::shared_ptr<HouseholdsMap> m_households;
	sp_mat freqMatrix;
	vec cons;
	vec weights;
	double eps;
	bool printOutput;
	bool ipu_success;

	IndexMap m_idx;
	ColIndexMap m_nonZeroIdx;
	//ProbMap m_hhProbs;
	ProbMap m_hhProbs;
	CountsMap m_hhCount;
};

#endif __IPU_h__
