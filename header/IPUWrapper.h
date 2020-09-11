#ifndef __IPUWrapper_h__
#define __IPUWrapper_h__

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <memory>
//#include <unordered_map>
#include <numeric>
#include <map>
#include "PersonPums.h"
#include "HouseholdPums.h"

//class Parameters;
class County;

template<class GenericParams>
class IPU;

//class HouseholdPums;
//class PersonPums;

template<class GenericParams>
class IPUWrapper
{
public:
	typedef std::vector<std::string> Columns;
	typedef std::multimap<int, std::multimap<std::string, Columns>> ACSEstimates;
	typedef std::vector<double> Marginal;
	typedef std::map<int, Marginal> MarginalMap;
	typedef std::pair<double, double> PairDD;
	//typedef std::map<std::string, std::vector<PairDD>> ProbMap;
	typedef std::map<std::string, std::map<double,std::vector<PairDD>>> ProbMap;
	//typedef std::unordered_map<double, HouseholdPums> HouseholdsMap;
	typedef std::map<double, HouseholdPums<GenericParams>> HouseholdsMap;
	typedef std::multimap<int, County> CountyMap;
	typedef std::map<std::string, double> ConsPersonMap;

	IPUWrapper(std::shared_ptr<GenericParams>, ACSEstimates*, CountyMap*);
	virtual ~IPUWrapper();

	void startIPU(std::string, std::string, int, bool);
	void clearHHPums();

	bool successIPU();
	const ProbMap *getHouseholdProbability() const;
	const HouseholdsMap *getHouseholds() const;
	double getHouseholdCount(std::string) const;
	const Marginal *getConstraints() const;
	int getPopSize() const;
	

private:

	Columns getStateList();
	void importHouseholdPUMS(std::string);
	void importPersonPUMS(std::string);
	void computeHouseholdEst();
	void computePersonEst();
	void refineHHPumsList();
	
	Marginal getEstimatesVector(int, std::string);
	void extractRaceEstimates(Marginal &, const std::map<int, std::vector<double>> &);
	void extractHHIncEstimates(Marginal &);
	void adjustHHSizeEstimates(Marginal &, double);
	
	MarginalMap getIPFestimates(Marginal &, Marginal &, size_t, size_t, int, int, int);
	void addConstraints(const std::map<int, Marginal>&);
	ConsPersonMap getConstraintsMap();

	void createSeedMatrix(int, int, size_t, size_t, int);
	void setMarginals(Marginal &, int);
	void adjustMarginals(Marginal &, Marginal &);
	void clear();
	
	void setPopulationSize(Marginal &, int);
	bool isValidGeoID(int, std::string);
	bool isValidPUMA(int);
	double getFrequency(int, int, int, int, int);
	int getCount(int, int, int, int, int, int);
	
	std::shared_ptr<GenericParams>parameters;
	std::shared_ptr<ACSEstimates>m_acsEstimates;
	std::shared_ptr<CountyMap> m_pumaCounty;
	
	IPU<GenericParams> *ipu;

	std::string geoID, areaAbbv;
	int totalPop;

	std::multimap<std::string, bool> m_pumsHHCount;
	std::multimap<std::string, bool> m_pumsPerCount;

	HouseholdsMap m_householdPUMS;

	std::vector<double> seed;
	std::vector<Marginal> marginals;
	std::vector<int> m_size;

	Marginal ipuCons;
};

#endif 