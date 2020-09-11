#include "County.h"

County::County() : cntyName(""), pumaCode(-1), popWeight(0.0)
{
}

County::~County()
{
}

std::string County::getCountyName() const
{
	return cntyName;
}

int County::getPumaCode() const
{
	return pumaCode;
}

double County::getPopulationWeight() const
{
	return popWeight;
}

int County::getPopulation() const
{
	return population;
}

void County::setCountyName(std::string county)
{
	this->cntyName = county;
}

void County::setPumaCode(int puma)
{
	this->pumaCode = puma;
}

void County::setPopulationWeight(double weight)
{
	this->popWeight = weight;
}

void County::setPopulation(int pop)
{
	this->population = pop;
}

