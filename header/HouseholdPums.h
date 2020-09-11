#ifndef __HouseholdPums_h__
#define __HouseholdPums_h__

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <map>
#include <cmath>

//class Parameters;

template<class GenericParams>
class PersonPums;

template<class GenericParams>
class HouseholdPums
{
public:
	
	HouseholdPums(std::shared_ptr<GenericParams>);

	virtual ~HouseholdPums();

	void setPUMA(std::string);
	void setHouseholds(std::string, std::string, std::string, std::string, std::string, std::string);
	void addPersons(PersonPums<GenericParams>);

	int getPUMA() const;
	double getHouseholdIndex() const;
	short int getHouseholdType() const;
	short int getHouseholdSize() const;
	short int getTotalPersons() const;
	short int getNumChildren() const;
	double getHouseholdIncome() const;
	short int getHouseholdIncCat() const;
	short int getHHTypeBySize() const;
	std::vector<PersonPums<GenericParams>> getPersons() const;

	void clearPersonList();
	
private:

	void setHouseholdType(short int);
	void setHouseholdSize(short int);
	void setHouseholdIncome(int, std::string);
	void setNumChildren(short int);

	template<class T>
	T to_number(const std::string &);
	bool is_number(const std::string);

	std::shared_ptr<GenericParams> parameters;
	
	int puma;
	double hhIdx;
	short int hhSize, hhType, hhIncomeCat;
	double hhIncome;
	short int numChildren;
	short int totalPersons;

	std::vector<PersonPums<GenericParams>> hhPersons;
};

#endif __HouseholdPums_h__