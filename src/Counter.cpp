#include "Counter.h"
#include "Parameters.h"
#include "ACS.h"
#include "CardioParams.h"
#include "ViolenceParams.h"
#include "DepressionParams.h"

template class Counter<ViolenceParams>;
template class Counter<CardioParams>;
template class Counter<DepressionParams>;

template<class GenericParams>
Counter<GenericParams>::Counter()
{
}

template<class GenericParams>
Counter<GenericParams>::~Counter()
{
}

template<class GenericParams>
int Counter<GenericParams>::getHouseholdCount(std::string hhType) const
{
	if(m_householdCount.count(hhType) > 0)
		return m_householdCount.at(hhType).size();
	else
	{
		std::cout << "Error: Household type " << hhType << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

template<class GenericParams>
int Counter<GenericParams>::getPersonCount(std::string personType) const
{
	if(m_personCount.count(personType) > 0)
		return m_personCount.at(personType).size();
	else
	{
		return 0;
		//std::cout << "Error: Person type " << personType << " doesn't exist!" << std::endl;
		//exit(EXIT_SUCCESS);
	}
}

template<class GenericParams>
void Counter<GenericParams>::initialize()
{
	//initHouseholdCounter();
	//initPersonCounter();

	//switch(parameters->getSimType())
	//{
	//case EQUITY_EFFICIENCY:
	//	//initRiskFacCounter();
	//	break;
	//case MASS_VIOLENCE:
	//	//initPtsdCounter();
	//	break;
	//default:
	//	break;
	//}
}


template<class GenericParams>
void Counter<GenericParams>::initHouseholdCounter(std::shared_ptr<GenericParams> p)
{
	clearMap(m_householdCount);

	std::vector<bool> hhCounts;
	hhCounts.reserve(0);
	
	const Pool *householdsPool = p->getHouseholdPool();

	for(size_t hh = 0; hh < householdsPool->size(); ++hh)
		m_householdCount.insert(std::make_pair(householdsPool->at(hh), hhCounts));

}

template<class GenericParams>
void Counter<GenericParams>::initPersonCounter(std::shared_ptr<GenericParams> p)
{
	clearMap(m_personCount);

	std::vector<bool> personCounts;
	personCounts.reserve(0);
	
	const Pool *indivPool = p->getPersonPool();
	for(size_t pp = 0; pp < indivPool->size(); ++pp)
		m_personCount.insert(std::make_pair(indivPool->at(pp), personCounts));
}

template<class GenericParams>
void Counter<GenericParams>::addHouseholdCount(std::string hhType)
{
	if (m_householdCount.count(hhType) > 0)
		m_householdCount[hhType].push_back(true);
}

template<class GenericParams>
void Counter<GenericParams>::addPersonCount(const char* personType)
{
	std::string s_personType(personType);
	addPersonCount(s_personType);
}

template<class GenericParams>
void Counter<GenericParams>::addPersonCount(std::string personType)
{
	if(m_personCount.count(personType) > 0)
		m_personCount[personType].push_back(true);
	else
	{
		std::vector<bool>temp;
		temp.push_back(true);

		m_personCount.insert(std::make_pair(personType, temp));
	}
}

template<class GenericParams>
void Counter<GenericParams>::addPersonCount(int origin, int sex)
{
	std::string agentType = std::to_string(origin)+std::to_string(sex);
	if(m_personCount.count(agentType) == 0)
	{
		std::vector<bool>count;
		count.push_back(true);

		m_personCount.insert(std::make_pair(agentType, count));
	}
	else
	{
		m_personCount[agentType].push_back(true);
	}
}

template<class GenericParams>
void Counter<GenericParams>::addPersonCount(std::string personType, std::string ptsdx_quartile)
{
	std::string agentType = personType + ptsdx_quartile;
	if(m_personCount.count(agentType) == 0)
	{
		std::vector<bool>count;
		count.push_back(true);

		m_personCount.insert(std::make_pair(agentType, count));
	}
	else
	{
		m_personCount[agentType].push_back(true);
	}
}

template<class GenericParams>
void Counter<GenericParams>::resetPersonCount(std::string personType)
{
	if(m_personCount.count(personType) > 0)
		m_personCount[personType].clear();
}


template<class GenericParams>
void Counter<GenericParams>::outputHouseholdCounts(std::shared_ptr<GenericParams> p, std::string geoID)
{
	std::ofstream hhFile;
	std::string fileName = "households/" + geoID + "_households.csv";
	hhFile.open(p->getOutputDir()+fileName);

	if(!hhFile.is_open()){
		std::cout << "Error: Cannot create " << fileName << "!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	hhFile << ",";
	for(auto hhSizeStr : ACS::HHSize::_values())
			hhFile << hhSizeStr._to_string() << ",";

	hhFile << std::endl;
	for(auto hhType : ACS::HHType::_values())
	{
		hhFile << hhType._to_string() << ",";
		for(auto hhSize : ACS::HHSize::_values())
		{
			int countHHSize = 0;
			for(auto hhInc : ACS::HHIncome::_values())
			{
				std::string var_type = std::to_string(hhType)+std::to_string(hhSize)+std::to_string(hhInc);
				//countHHSize += m_householdCount.count(var_type);
				if(m_householdCount.count(var_type) > 0)
					countHHSize += m_householdCount[var_type].size();
			}
			hhFile << countHHSize << ",";
		}
		hhFile << std::endl;
	}

	hhFile << std::endl;
	
	hhFile << ",";
	for(auto hhIncStr : ACS::HHIncome::_values())
		hhFile << hhIncStr._to_string() << ",";
	
	hhFile << std::endl;
	for(auto hhType : ACS::HHType::_values())
	{
		hhFile << hhType._to_string() << ",";
		for(auto hhInc : ACS::HHIncome::_values())
		{
			int countHHInc = 0;
			for(auto hhSize : ACS::HHSize::_values())
			{
				std::string var_type = std::to_string(hhType)+std::to_string(hhSize)+std::to_string(hhInc);
				//countHHInc += m_householdCount.count(var_type);
				if(m_householdCount.count(var_type) > 0)
					countHHInc += m_householdCount[var_type].size();
			}
			hhFile << countHHInc << ",";
		}
		hhFile << std::endl;
	}
	hhFile << std::endl;
}


template<class GenericParams>
void Counter<GenericParams>::outputPersonCounts(std::shared_ptr<GenericParams> p, std::string geoID)
{
	std::ofstream pFile;
	std::string fileName = "persons/" + geoID  + "_persons.csv";
	
	pFile.open(p->getOutputDir()+fileName, std::ios::app);

	if(!pFile.is_open()){
		std::cout << "Error: Cannot create " << fileName << "!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	std::string dummy = "0";

	//Person counter for age and sex
	pFile << ",";
	for(auto ageCatStr : ACS::AgeCat::_values())
		pFile << ageCatStr._to_string() << ",";
	pFile << std::endl;
	for(auto sex : ACS::Sex::_values())
	{
		pFile << sex._to_string() << ",";
		for(auto ageCat : ACS::AgeCat::_values())
		{
			int countAge = 0;
			for(auto org : ACS::Origin::_values())
			{
				std::string type = dummy+std::to_string(sex)+std::to_string(ageCat)+std::to_string(org);
				//countAge += m_personCount.count(type);
				if(m_personCount.count(type) > 0)
					countAge += m_personCount[type].size();
			}
			pFile << countAge << ",";
		}
		pFile << std::endl;
	}
	pFile << std::endl;
	
	//Person counter for origin and sex
	pFile << ",";
	for(auto orgStr : ACS::Origin::_values())
		pFile << orgStr._to_string() << ",";
	pFile << std::endl;
	for(auto sex : ACS::Sex::_values())
	{
		pFile << sex._to_string() << ",";
		for(auto org : ACS::Origin::_values())
		{
			int countOrigin = 0;
			for(auto ageCat : ACS::AgeCat::_values())
			{
				std::string type = dummy+std::to_string(sex)+std::to_string(ageCat)+std::to_string(org);
				//countOrigin += m_personCount.count(type);
				if(m_personCount.count(type) > 0)
					countOrigin += m_personCount[type].size();
			}
			pFile << countOrigin << ",";
		}
		pFile << std::endl;
	}
	pFile << std::endl;

	//Person counter for education and sex
	pFile << ",";
	for(auto eduAge : ACS::EduAgeCat::_values())
		for(auto edu : ACS::Education::_values())
			pFile << eduAge._to_string() << " " << edu._to_string() << ",";

	pFile << std::endl;
	for(auto sex : ACS::Sex::_values())
	{
		pFile << sex._to_string() << ",";
		for(auto eduAge : ACS::EduAgeCat::_values())
		{
			for(auto edu : ACS::Education::_values())
			{
				int countEdu = 0;
				for(auto org : ACS::Origin::_values())
				{
					std::string type = std::to_string(sex)+std::to_string(eduAge)+std::to_string(org)+std::to_string(edu);
					if(m_personCount.count(type) > 0)
						countEdu += m_personCount[type].size();
				}
				pFile << countEdu << ",";
			}
		}
		pFile << std::endl;
	}
	
	pFile << std::endl;
}


template<class GenericParams>
void Counter<GenericParams>::clearMap(TypeMap &type)
{
	for(auto map = type.begin(); map != type.end(); ++map)
	{
		map->second.clear();
		map->second.shrink_to_fit();
	}

	type.clear();

}
