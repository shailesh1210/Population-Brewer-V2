#include "DepressionAgent.h"
#include "DepressionParams.h"
#include "DepressionCounter.h"
#include "PersonPums.h"
#include "Random.h"
#include "ACS.h"

DepressionAgent::DepressionAgent()
{
}

DepressionAgent::DepressionAgent(std::shared_ptr<DepressionParams> param, const PersonPums<DepressionParams> *p, Random *rand, DepressionCounter *count, int hhCount, int countPersons)
	: parameters(param), random(rand), counter(count)
{
	this->householdID = hhCount;
	this->puma = p->getPumaCode();

	this->age = p->getAge();
	this->sex = p->getSex();
	this->origin = p->getOrigin();

	this->education = p->getEducation();
	this->income = p->getIncome();

	this->agentIdx = "Agent"+std::to_string(hhCount)+std::to_string(countPersons);

	this->ipRatio = -1;
	this->ipRatioTag = -1;

	this->depressionType = -1;
	this->symptoms = -1;

	setAgeCat();
}

DepressionAgent::~DepressionAgent()
{
}

void DepressionAgent::update()
{
	reduceWageGap();
}

void DepressionAgent::reduceWageGap()
{
	if(this->sex == ACS::Sex::Female)
	{
		const VecPairDblInt *pWageGapDist = parameters->getWageGapProbabilityDist(std::to_string(this->ageCat));
		double randP = random->uniform_real_dist();

		for(auto dist = pWageGapDist->begin(); dist != pWageGapDist->end(); ++dist)
		{
			double pWageGap = dist->first;
			if(randP < pWageGap)
			{
				this->ipRatioTag = dist->second;
				break;
			}
		}

		counter->addPersonCount(getAgentType1());
	}

}

void DepressionAgent::setAgeCat()
{
	if(age >= 18 && age < 45)
	{
		this->ageCat = Depression::AgeCat::Age_18_44;
	}
	else if(age >=45 && age < 65)
	{
		this->ageCat = Depression::AgeCat::Age_45_64;
	}
	else if(age >= 65)
	{
		this->ageCat = Depression::AgeCat::Age_65_;
	}
	else
	{
		this->ageCat = -1;
	}
}

void DepressionAgent::setIncomeToPovertyRatio(double income, int totalPersons, int ageCat, int numRelatedChildren)
{
	const VecPairDblInt *v_ipRatioTag = parameters->getIncomePovertyRatioTag();

	int povertyThres = parameters->getPovertyThreshold(totalPersons, ageCat, numRelatedChildren);
	this->ipRatio = income/povertyThres;

	int max_ip_ratio_thres = v_ipRatioTag->back().first;

	if(this->ipRatio >= max_ip_ratio_thres)
	{
		this->ipRatioTag = ACS::IncomeToPovertyRatio::IP_500_;
	}
	else
	{
		for(auto pair = v_ipRatioTag->begin(); pair != v_ipRatioTag->end(); ++pair)
		{
			double ip_ratio_thres = pair->first;
			int ip_ratio_tag = pair->second;

			if(this->ipRatio < ip_ratio_thres)
			{
				this->ipRatioTag = ip_ratio_tag;
				break;
			}
		}
	}
}

void DepressionAgent::setDepressionType(short int depression_type)
{
	this->depressionType = depression_type;
	setDepressionSymptoms();
}

void DepressionAgent::setDepressionSymptoms()
{
	counter->addPersonCount(getAgentType2());
	//counter->addPersonCount(getAgentType5());
	
	const Tuple *depression_symptoms = parameters->getDepressionSymptoms(getAgentType2());
	double randP = random->uniform_real_dist();

	if(randP < Depression::first_q)
	{
		this->symptoms = std::get<0>(*depression_symptoms);
		counter->addPersonCount(getAgentType2(), "1Q");
	}
	else if(randP < Depression::second_q)
	{
		this->symptoms = std::get<1>(*depression_symptoms);
		counter->addPersonCount(getAgentType2(), "2Q");
	}
	else if(randP < Depression::third_q)
	{
		this->symptoms = std::get<2>(*depression_symptoms);
		counter->addPersonCount(getAgentType2(), "3Q");
	}
	else if(randP < Depression::fourth_q)
	{
		this->symptoms = std::get<3>(*depression_symptoms);
		counter->addPersonCount(getAgentType2(), "4Q");
	}

}

std::string DepressionAgent::getAgentID() const
{
	return agentIdx;
}

short int DepressionAgent::getDepressionType() const
{
	return depressionType;
}

/**
*	@brief returns agentType by sex, age cat and income to poverty ratio category
*	@param none
*	@return string
*/
std::string DepressionAgent::getAgentType1() const
{
	return "IP_Ratio" + std::to_string(sex) + std::to_string(ageCat) + std::to_string(ipRatioTag);
}

/**
*	@brief returns agentType by depression type, sex, age cat and income to poverty ratio category
*	@param none
*	@return string
*/
std::string DepressionAgent::getAgentType2() const
{
	return "Type" + std::to_string(depressionType) + getAgentType1();
}

/**
*	@brief returns agentType by sex and income to poverty ratio cat
*	@param none
*	@return string
*/
std::string DepressionAgent::getAgentType3() const
{
	return "IP" + std::to_string(sex) + std::to_string(ipRatioTag);
}

/**
*	@brief returns agentType by sex and age cat
*	@param none
*	@return string
*/
std::string DepressionAgent::getAgentType4() const
{
	return "Sex_Age_Cat" + std::to_string(sex) + std::to_string(ageCat);
}

/**
*	@brief returns agentType by depression type, sex and age cat
*	@param none
*	@return string
*/
std::string DepressionAgent::getAgentType5() const
{
	return "Depression_Type_" + getAgentType4() + std::to_string(depressionType);
}

DepressionCounter *DepressionAgent::getCounter()
{
	return counter;
}