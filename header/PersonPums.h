#ifndef __PersonPums_h__
#define __PersonPums_h__

#include <iostream>
#include <memory>
#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <cmath>

//class Parameters;

template<class GenericParams>
class PersonPums
{
public:
	typedef std::map<std::string, int> Map;
	typedef std::multimap<int, Map> MultiMapCB;

	typedef std::vector<std::string> Column;
	typedef std::list<Column> Row;

	PersonPums(std::shared_ptr<GenericParams>);
	virtual ~PersonPums();

	void setDemoCharacters(std::string, std::string, std::string, std::string, std::string, std::string);
	void setSocialCharacters(std::string, std::string, std::string, std::string);
	
	double getPUMSID() const;
	int getPumaCode() const;
	short int getAge() const;
	short int getAgeCat() const;
	
	short int getSex() const;
	short int getRace() const;
	short int getEthnicity() const;
	short int getOrigin() const;
	short int getEducation() const;
	short int getEduAgeCat() const;
	double getIncome() const;

private:

	void setAge(short int);
	void setAgeCat(Map &, short int, short int);
	void setSex(short int);
	void setEthnicity(short int);
	void setRace(short int);
	void setOrigin();
	void setEducation(short int);
	void setEduAgeCat();
	void setIncome(int, std::string);

	template<class T>
	T to_number(const std::string &);
	bool is_number(const std::string);
	
	std::shared_ptr<GenericParams> parameters;
	int pumaCode;
	double personID;
	short int age, ageCat, sex;
	short int race, ethnicity, originByRace; 
	short int education, eduAgeCat;
	double income;
};

#endif __PersonPums_h__
