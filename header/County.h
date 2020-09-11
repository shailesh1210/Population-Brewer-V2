#ifndef __County_h__
#define __County_h__

#include <iostream>
#include <string>

class County
{
public:
	County();
	virtual ~County();

	std::string getCountyName() const;
	int getPumaCode() const;
	double getPopulationWeight() const;
	int getPopulation() const;

	void setCountyName(std::string);
	void setPumaCode(int);
	void setPopulationWeight(double);
	void setPopulation(int);

private:
	std::string cntyName;
	int pumaCode;
	double popWeight;
	int population;
};

#endif __County_h__