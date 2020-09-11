#include "HouseholdPums.h"
#include "ACS.h"
#include "ViolenceParams.h"
#include "CardioParams.h"
#include "DepressionParams.h"
#include "PersonPums.h"

template class HouseholdPums<ViolenceParams>;
template class HouseholdPums<CardioParams>;
template class HouseholdPums<DepressionParams>;

template<class GenericParams>
HouseholdPums<GenericParams>::HouseholdPums(std::shared_ptr<GenericParams> param) :
	parameters(param)
{
}

template<class GenericParams>
HouseholdPums<GenericParams>::~HouseholdPums()
{
	
}

template<class GenericParams>
void HouseholdPums<GenericParams>::setPUMA(std::string hh_puma)
{
	if(!parameters->isStateLevel())
		this->puma = to_number<int>(hh_puma);
	else
		this->puma = 100;
}

template<class GenericParams>
void HouseholdPums<GenericParams>::setHouseholds(std::string hh_idx, std::string hh_type, std::string hh_size, std::string hh_income, std::string num_child, std::string adj_inc)
{
	this->hhIdx = to_number<double>(hh_idx);

	setHouseholdSize(to_number<short int>(hh_size));
	setHouseholdType(to_number<short int>(hh_type));
	setHouseholdIncome(to_number<int>(hh_income), adj_inc);
	setNumChildren(to_number<short int>(num_child));
}

template<class GenericParams>
void HouseholdPums<GenericParams>::setHouseholdType(short int type)
{
	std::map<std::string, int> m_householdType, tempHHType;
	m_householdType = (parameters->getACSCodeBook().find(ACS::PumsVar::HHT)->second);
	
	if(type == m_householdType.at("Male householder-living alone-nonfamily") || type == m_householdType.at("Female householder-living alone-nonfamily")){
		this->hhType = ACS::HHType::NonFamily;
	}
	else if(type == m_householdType.at("Male householder-not living alone-nonfamily") || type == m_householdType.at("Female householder-not living alone-nonfamily")){
		this->hhType = ACS::HHType::NonFamily;
	}
	else{
		this->hhType = type;
	}
}

template<class GenericParams>
void HouseholdPums<GenericParams>::setHouseholdSize(short int size)
{
	this->totalPersons = size;
	if(size >= ACS::HHSize::HHsize7){
		this->hhSize = ACS::HHSize::HHsize7;
	}
	else{
		this->hhSize = size;
	}
}

template<class GenericParams>
void HouseholdPums<GenericParams>::setHouseholdIncome(int income, std::string adj_inc)
{
	std::map<std::string, int> m_householdIncome(parameters->getACSCodeBook().find(ACS::PumsVar::HINCP)->second);
	double d_adj_inc = to_number<double>(adj_inc)/pow(10, 6);

	if(income < 0)
		this->hhIncome = 0;
	else
		this->hhIncome = income;

	this->hhIncome *= d_adj_inc;	
	for(auto incCat : ACS::HHIncome::_values())
	{
		if(income < 0){
			this->hhIncomeCat = -1;
			break;
		}
		std::string inc_str = incCat._to_string();
		if(income < m_householdIncome.at(inc_str)){
			this->hhIncomeCat = incCat;
			break;
		}
	}

}

template <class GenericParams>
void HouseholdPums<GenericParams>::setNumChildren(short int numChild)
{
	this->numChildren = numChild;
}

//template<class GenericParams>
//void HouseholdPums<GenericParams>::addPersons(typename PersonPums<GenericParams> person)
//{
//	hhPersons.push_back(person);
//}


template<class GenericParams>
void HouseholdPums<GenericParams>::addPersons(PersonPums<GenericParams> person)
{
	hhPersons.push_back(person);
}

template<class GenericParams>
int HouseholdPums<GenericParams>::getPUMA() const
{
	return puma;
}

template<class GenericParams>
double HouseholdPums<GenericParams>::getHouseholdIndex() const
{
	return hhIdx;
}

template<class GenericParams>
short int HouseholdPums<GenericParams>::getHouseholdType() const
{
	return hhType;
}

template<class GenericParams>
short int HouseholdPums<GenericParams>::getHouseholdSize() const
{
	return hhSize;
}

template<class GenericParams>
short int HouseholdPums<GenericParams>::getTotalPersons() const
{
	return totalPersons;
}

template<class GenericParams>
short int HouseholdPums<GenericParams>::getNumChildren() const
{
	return numChildren;
}

template<class GenericParams>
double HouseholdPums<GenericParams>::getHouseholdIncome() const
{
	return hhIncome;
}

template<class GenericParams>
short int HouseholdPums<GenericParams>::getHouseholdIncCat() const
{
	return hhIncomeCat;
}

template<class GenericParams>
short int HouseholdPums<GenericParams>::getHHTypeBySize() const
{
	if(hhType > 0)
		return ((hhType-1)*ACS::HHSize::_size()+hhSize);
	else
		return -1;
}

template<class GenericParams>
std::vector<PersonPums<GenericParams>> HouseholdPums<GenericParams>::getPersons() const
{
	return hhPersons;
}

template<class GenericParams>
void HouseholdPums<GenericParams>::clearPersonList()
{
	hhPersons.clear();
	hhPersons.shrink_to_fit();
}


template<class GenericParams>
template<class T>
T HouseholdPums<GenericParams>::to_number(const std::string &data)
{
	if(!is_number(data) || data.empty())
		return -1;

	std::istringstream ss(data);
	T num;
	ss >> num;
	return num;
}

template<class GenericParams>
bool HouseholdPums<GenericParams>::is_number(const std::string data)
{
	char *end = 0;
	double val = std::strtod(data.c_str(), &end);
	bool flag = (end != data.c_str() && val != HUGE_VAL);
	return flag;
}
