#include "DepressionCounter.h"
#include "DepressionParams.h"

DepressionCounter::DepressionCounter()
{
}

DepressionCounter::DepressionCounter(std::shared_ptr<DepressionParams> p) : param(p)
{
}

DepressionCounter::~DepressionCounter()
{
}

void DepressionCounter::initialize()
{
	initHouseholdCounter(param);
	initPersonCounter(param);
}

void DepressionCounter::computeOutcomes(std::string timeFrame)
{
	computePrevalence(timeFrame);
}


void DepressionCounter::output(std::string geoID)
{
	outputPrevalence(geoID);
}

void DepressionCounter::computePrevalence(std::string timeFrame)
{
	std::string prefix_type = "Type";
	std::string prefix_ip_ratio = "IP_Ratio";
	std::string prefix_sex_age_cat = "Sex_Age_Cat";

	std::cout << timeFrame <<  std::endl;
	for(auto sex : ACS::Sex::_values())
	{
		for(auto ageCat : Depression::AgeCat::_values())
		{
			//DepressionAgent - agentType4() - "Sex_Age_Cat" + sex + ageCat
			std::string agent_type1 = std::to_string(sex) + std::to_string(ageCat);
			int countSexAge = getPersonCount(prefix_sex_age_cat + agent_type1);

			int pop = 0;
			for(auto type : Depression::DepressionType::_values())
			{
				std::string agent_type2 = agent_type1 + std::to_string(type);
				int countDepression = 0;
				for(auto ipRatio : ACS::IncomeToPovertyRatio::_values())
				{
					//DepressionAgent - agentType2() - "Type" + Depression Type + "IP_Ratio" + Sex + AgeCat + IpRatio
					std::string agent_type3 = std::to_string(type) + prefix_ip_ratio + agent_type1 + std::to_string(ipRatio);  
					countDepression += getPersonCount(prefix_type + agent_type3);
				}
				
				pop += countDepression;
				if(countSexAge > 0)
				{
					m_depressionPrevalence[timeFrame][agent_type2] += (double)countDepression/countSexAge;
					m_popBySexAgeDepression[timeFrame][agent_type2] += countDepression;
				}
				else
				{
					m_depressionPrevalence[timeFrame][agent_type2] = 0;
					m_popBySexAgeDepression[timeFrame][agent_type2] = 0;
				}

			}

			if(pop != countSexAge)
			{
				std::cout << "Error: Population for agent type " << agent_type1 << " doesn't match!" << std::endl;
				exit(EXIT_SUCCESS);
			}
		}
	}
}

void DepressionCounter::outputPrevalence(std::string geoID)
{
	std::string filename = "Depression/prevalence_" + geoID + ".csv";
	std::ofstream file(param->getOutputDir() + filename, std::ios::app);

	if(!file.is_open())
		exit(EXIT_SUCCESS);

	file << "State, Sex, Age_Cat, Depression_Type, Preval_Before, Preval_After, Pop_Before, Pop_After" << std::endl;
	for(auto sex : ACS::Sex::_values())
	{
		for(auto ageCat : Depression::AgeCat::_values())
		{
			for(auto type : Depression::DepressionType::_values())
			{
				std::string agent_type = std::to_string(sex) + std::to_string(ageCat) + std::to_string(type);
				
				file << geoID << ", " << sex._to_string() << ", " << ageCat._to_string() << ", " << type._to_string() << ", " 
					<< m_depressionPrevalence["Before Intervention"][agent_type] << ", " 
					<< m_depressionPrevalence["After Intervention"][agent_type] << ", "
					<< m_popBySexAgeDepression["Before Intervention"][agent_type] << ", "
					<< m_popBySexAgeDepression["After Intervention"][agent_type] << std::endl;
			}
		}
	}
}

void DepressionCounter::clear()
{
}