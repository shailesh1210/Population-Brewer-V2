#ifndef __Parameters_h__
#define __Parameters_h__

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <numeric>
#include <tuple>
//#include <unordered_map>
#include <boost/tokenizer.hpp>
#include "ACS.h"
#include "csv.h"

#define EQUITY_EFFICIENCY 1
#define MASS_VIOLENCE 2
#define POP_MENTAL_HEALTH 3
#define NUM_RISK_STRATA 32


class Parameters
{
public:
	typedef std::vector<std::string> Columns;
	typedef std::vector<Columns> Rows;
	typedef boost::tokenizer<boost::escaped_list_separator<char>> Tokenizer;
	typedef std::multimap<std::string, Columns> MultiMapCSV;
	typedef std::pair<double, double> PairDD;
	typedef std::tuple<double, double, double, double>Tuple;
	
	typedef std::map<std::string, int> MapInt;
	typedef std::map<std::string, double>MapDbl;
	typedef std::multimap<int, MapInt> MultiMapCB;
	typedef std::map<std::string, Tuple> PairTuple;

	typedef std::vector<std::string> Pool;
	typedef std::map<std::string, Pool> PoolMap;

	Parameters();
	Parameters(const char*, const char*, const int, const int);

	virtual ~Parameters();

	std::string getInputDir() const;
	std::string getOutputDir() const;
	
	const char* getMSAListFile();
	const char* getUSStateListFile();
	const char* getCountiesListFile();
	const char* getPUMAListFile();
	const char* getHouseholdPumsFile(std::string);
	const char* getPersonPumsFile(std::string);

	const char* getRaceMarginalFile();
	const char* getEducationMarginalFile();

	const char* getHHTypeMarginalFile();
	const char* getHHSizeMarginalFile();
	const char* getHHIncomeMarginalFile();
	const char* getGQMarginalFile();

	double getAlpha() const;
	double getMinSampleSize() const;
	int getMaxDraws() const;
	short int getSimType() const;
	short int getGeoType() const;
	bool isStateLevel() const;
	bool writeToFile() const;

	const Pool *getHouseholdPool() const;
	const Pool *getPersonPool() const;
	const Pool *getNhanesPool() const;
	const Pool *getNhanesPoolMap(std::string);

	MultiMapCB getACSCodeBook() const;
	
	std::multimap<int, int> getVariableMap(int) const;
	MapInt getOriginMapping() const;
	
protected:

	void readACSCodeBookFile();
	void readAgeGenderMappingFile();
	void readHHIncomeMappingFile();
	void readOriginListFile();

	void createHouseholdPool();
	void createPersonPool();
	void createNhanesPool();

	std::string getNHANESpersonType(const char*, const char*, const char*, const char*);
	std::string getNHANESpersonType(const char*, const char*, const char*);
	std::string getNHANESpersonType(int, int, int, int);

	bool isEmpty(const char *[], int);

	MapInt createCodeBookMap(ACS::PumsVar);
	const char* getFilePath(const char *);

	Rows readCSVFile(const char*);

	const char *inputDir;
	const char *outputDir;

	MultiMapCSV m_codeBook;
	MultiMapCB m_acsCodes;

	std::multimap<int, int> m_eduAgeGender;
	std::multimap<int, int> m_hhIncome;
	MapInt m_originByRace;

	double alpha, minSampleSize;
	int max_draws;
	short int simType;
	short int geoLevel;
	bool output;

	Pool hhPool, personPool, nhanesPool;
	PoolMap m_nhanesPool;

};
#endif __Parameters_h__