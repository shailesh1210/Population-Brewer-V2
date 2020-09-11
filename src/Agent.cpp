#include "Agent.h"
#include "PersonPums.h"
#include "ViolenceParams.h"
#include "CardioParams.h"
#include "DepressionParams.h"

template class Agent<ViolenceParams>;
template class Agent<CardioParams>;
template class Agent<DepressionParams>;

template<class GenericParams>
Agent<GenericParams>::Agent()
{
}

template<class GenericParams>
Agent<GenericParams>::Agent(const PersonPums<GenericParams> *person)
{
	this->householdID = person->getPUMSID();
	this->puma = person->getPumaCode();

	this->age = person->getAge();
	this->sex = person->getSex();
	this->origin = person->getOrigin();

	this->education = person->getEducation();
	this->income = person->getIncome();
}

template<class GenericParams>
Agent<GenericParams>::~Agent()
{

}

template<class GenericParams>
void Agent<GenericParams>::setHouseholdID(double h_idx)
{
	this->householdID = h_idx;
}

template<class GenericParams>
void Agent<GenericParams>::setPUMA(int puma_code)
{
	this->puma = puma_code;
}

template<class GenericParams>
void Agent<GenericParams>::setAge(short int p_age)
{
	this->age = p_age;
}

template<class GenericParams>
void Agent<GenericParams>::setSex(short int p_sex)
{
	this->sex = p_sex;
}

template<class GenericParams>
void Agent<GenericParams>::setOrigin(short int p_org)
{
	this->origin = p_org;
}

template<class GenericParams>
void Agent<GenericParams>::setEducation(short int p_edu)
{
	this->education = p_edu;
}

template<class GenericParams>
void Agent<GenericParams>::setIncome(double p_income)
{
	this->income = p_income;
}

template<class GenericParams>
double Agent<GenericParams>::getHouseholdID() const
{
	return householdID;
}

template<class GenericParams>
int Agent<GenericParams>::getPUMA() const
{
	return puma;
}

template<class GenericParams>
short int Agent<GenericParams>::getAge() const
{
	return age;
}

template<class GenericParams>
short int Agent<GenericParams>::getSex() const
{
	return sex;
}

template<class GenericParams>
short int Agent<GenericParams>::getOrigin() const
{
	return origin;
}

template<class GenericParams>
short int Agent<GenericParams>::getEducation() const
{
	return education;
}

template<class GenericParams>
double Agent<GenericParams>::getIncome() const
{
	return income;
}