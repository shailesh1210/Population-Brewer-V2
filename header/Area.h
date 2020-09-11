#ifndef __Area_h__
#define __Area_h__

#include <iostream>
#include <memory>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <boost/range/algorithm.hpp>

#include <boost/math/distributions/chi_squared.hpp>

#include "PersonPums.h"
#include "HouseholdPums.h"

class County;
//class Parameters;
class CardioCounter;
class ViolenceCounter;
class CardioModel;

template <class GenericParams>
class IPUWrapper;

template <class GenericParams>
class Area
{
public:

	typedef std::vector<std::string> Columns;
	typedef std::multimap<int, std::multimap<std::string, Columns>> ACSEstimates;
	typedef std::pair<double, double> PairDD;
	typedef std::map<std::string, std::map<double,std::vector<PairDD>>> ProbMap;
	
	typedef std::map<int,std::map<std::string, PairDD>> RiskFacMap;
	typedef std::map<std::string, PairDD> PairMap;
	typedef std::map<double, HouseholdPums<GenericParams>> PUMSHouseholdsMap;
	typedef std::multimap<int, County> CountyMap;
	typedef std::vector<double> Marginal;
	typedef std::vector<std::string> Pool;

	Area();
	Area(std::shared_ptr<GenericParams>);
	virtual ~Area();

	void setAreaIDandName(const std::string &, const std::string &);
	void setAreaAbbreviation(const std::string &);
	void setPopulation(const int &);
	void setCounties(const County &);
	void setEstimates(const Columns &, int);

	std::string getGeoID() const;
	std::string getAreaName() const;
	std::string getAreaAbbreviation() const;
	int getPopulation() const;

	std::multimap<int, County> getPumaCountyMap() const;
	int getCountyNum(std::string) const;

	template <class T>
	void createAgents(T *);
	
protected:
	
	template <class T>
	void drawHouseholds(IPUWrapper<GenericParams> *, T *);

	template<class T>
	bool checkFit(const Marginal *, const T *, int);
	
	void gofLog(double, int, int);
	
	std::shared_ptr<GenericParams> parameters;

	IPUWrapper<GenericParams> *ipuWrapper;
	
	std::string geoID;
	std::string areaName, areaAbbv;
	int population;

	CountyMap m_pumaCounty;

	ACSEstimates m_acsEstimates;
	
};

#endif __Area_h__