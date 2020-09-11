#ifndef __PopBrewer_h__
#define __PopBrewer_h__

#include <iostream>
#include <fstream>
#include <memory>
#include <iomanip>
#include <list>
#include <vector>
#include <map>
#include <boost/tokenizer.hpp>

template<class GenericParams>
class Area;

class County;

template <class GenericParams>
class PopBrewer
{
public:

	typedef boost::tokenizer<boost::escaped_list_separator<char>> TokenizerCSVFile;
	typedef std::vector<std::string> Columns;
	typedef std::list<Columns> Rows;
	typedef std::vector<double> Marginal;
	typedef std::vector<std::string> Pool;
	
	PopBrewer();
	PopBrewer(GenericParams *);

	virtual ~PopBrewer();

	void setParameters(const GenericParams &);
	void import();

protected:
	std::shared_ptr<GenericParams>parameters;

	void importArea(int);
	void importEstimates();

	void importMetroArea();
	void importState();

	void importRaceEstimates();
	void importEducationEstimates();

	void importHHTypeEstimates();
	void importHHSizeEstimates();
	void importHHIncomeEstimates();

	void importGQEstimates();

	void setEstimates(const Rows &, const std::map<int, int>&, const size_t, int);

	void mapMetroToCounties(const char*, std::multimap<std::string, std::string>&);
	void mapCountiesToPUMA(const char*, std::multimap<std::string, County>&);

	Rows readCSVFile(const char*);
	//int readCSVFile(const char*);

	template<class T>
	std::map<int, int> getColumnIndexMap(std::list<T>*, Columns *);
	int getColumnIndex(Columns *, std::string);
	
	std::map<std::string, Area<GenericParams>> m_geoAreas;
	
};

#endif __PopBrewer_h__