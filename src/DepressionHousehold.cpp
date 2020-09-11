#include "HouseholdPums.h"
#include "DepressionHousehold.h"
#include "ACS.h"
#include "DepressionCounter.h"

DepressionHousehold::DepressionHousehold()
{
}

DepressionHousehold::DepressionHousehold(const HouseholdPums<DepressionParams> *hh)
{
	this->hhType = hh->getHouseholdType();
	this->hhSize = hh->getHouseholdSize();
	this->hhIncome = hh->getHouseholdIncome();

	this->numChildren = hh->getNumChildren();
	this->totalPersons = hh->getTotalPersons();

	members.reserve(totalPersons);
}

DepressionHousehold::~DepressionHousehold()
{
	for (auto agent : members)
		delete agent;
}

void DepressionHousehold::addMemebers(DepressionAgent *agent)
{
	if(agent != NULL)
	{
		DepressionAgent *member = new DepressionAgent;
		*member = *agent;

		members.push_back(member);
		if(agent->getAge() >= 18 && this->hhType == ACS::HHType::NonFamily)
		{
			agent->getCounter()->addPersonCount(agent->getAgentType4());
		}
	}
	else
	{
		std::cout << "Error: Cannot add agent to the list!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}


void DepressionHousehold::setIncomeToPovertyRatio()
{
	if(hhType == ACS::HHType::NonFamily)
	{
		setIncomeToPovertyNonFamily();
	}
	else
	{
		setIncomeToPovertyFamily();
	}
}

void DepressionHousehold::setIncomeToPovertyNonFamily()
{
	int ageCat = -1;
	for(auto agent : members)
	{
		ageCat = (agent->getAge() >= 65) ? ACS::AgeCat1::Over65 : ACS::AgeCat1::Under65;
		agent->setIncomeToPovertyRatio(agent->getIncome(), ACS::TotalPersons::One, ageCat, ACS::NumChild::Zero_Child);

		if(agent->getAge() >= 18)
		{
			//agent->getCounter()->addPersonCount(agent->getAgentType3());
			agent->getCounter()->addPersonCount(agent->getAgentType1());
		}
	}
}

void DepressionHousehold::setIncomeToPovertyFamily()
{
	int familySize = (this->totalPersons >= ACS::TotalPersons::Nine) ? ACS::TotalPersons::Nine : this->totalPersons;
	int relatedChildren = (this->numChildren >= ACS::NumChild::Eight_Child) ? ACS::NumChild::Eight_Child : this->numChildren;
	int ageCat = getHouseholderAgeCat();

	if(relatedChildren > familySize)
	{
		std::cout << "Error: Num Children cannot be greater than family size!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	for(auto agent : members)
	{
		agent->setIncomeToPovertyRatio(hhIncome, familySize, ageCat, relatedChildren);
	}
}

int DepressionHousehold::getHouseholderAgeCat() const
{
	int ageCat = -1;
	if(totalPersons == ACS::TotalPersons::Two)
	{
		bool isHouseholder = false;
		bool isOlder = false;

		for(auto agent1 : members)
		{
			for(auto agent2 : members)
			{
				if(agent1->getAgentID() != agent2->getAgentID())
				{
					if(agent1->getIncome() >= agent2->getIncome() && agent1->getIncome() >= 0)
					{
						isHouseholder = true;
						if(agent1->getAge() >= 65)
						{
							isOlder = true;
						}
					}
				}
			}

			if(isHouseholder)
			{
				ageCat = (isOlder) ? ACS::AgeCat1::Over65 : ACS::AgeCat1::Under65;
				break;
			}
		}
	}
	else
	{
		ageCat = ACS::AgeCat1::All;
	}

	return ageCat;
}

short int DepressionHousehold::getHouseholdSize() const
{
	return hhSize;
}

short int DepressionHousehold::getHouseholdType() const
{
	return hhType;
}

double DepressionHousehold::getHouseholdIncome() const
{
	return hhIncome;
}

short int DepressionHousehold::getNumChildren() const
{
	return numChildren;
}

short int DepressionHousehold::getTotalPersons() const
{
	return totalPersons;
}

const DepressionHousehold::FamilyMembers *DepressionHousehold::getMembers() const
{
	return &members;
}
