#ifndef __Counter_h__
#define __Counter_h__

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <fstream>
#include <cmath>

#define WEEKS_IN_YEAR 52
#define DAYS_IN_WEEK 7

template<class GenericParams>
class Counter
{
public:
	typedef std::map<std::string, std::vector<bool>> TypeMap;
	typedef std::vector<std::string> Pool;

	Counter();
	virtual ~Counter();

	int getPersonCount(std::string) const;
	int getHouseholdCount(std::string) const;

	void initialize();

	void addHouseholdCount(std::string);
	void addPersonCount(const char *);
	void addPersonCount(std::string);
	void addPersonCount(int, int);
	void addPersonCount(std::string, std::string);

	void resetPersonCount(std::string);

protected:
	void initHouseholdCounter(std::shared_ptr<GenericParams>);
	void initPersonCounter(std::shared_ptr<GenericParams>);
	
	void clearMap(TypeMap &);

	void outputHouseholdCounts(std::shared_ptr<GenericParams>, std::string);
	void outputPersonCounts(std::shared_ptr<GenericParams>, std::string);

	TypeMap m_personCount, m_householdCount;

};
#endif __Counter_h__