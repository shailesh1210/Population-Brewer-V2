#include "DepressionParams.h"
#include "csv.h"

DepressionParams::DepressionParams()
{
}

DepressionParams::DepressionParams(const char *inDir, const char *outDir, int simType, int geoLvl)
	:Parameters(inDir, outDir, simType, geoLvl)
{
	readPovertyThreshold();
	readDepressionPrevalence();
	readDepressionSymptoms();
}

DepressionParams::~DepressionParams()
{
	
}

void DepressionParams::readPovertyThreshold()
{
	Rows inPovertyThres = readCSVFile(getFilePath("pop_mental_health/poverty_thres.csv"));
	inPovertyThres.erase(inPovertyThres.begin());

	for(auto row = inPovertyThres.begin(); row!= inPovertyThres.end(); ++row)
	{
		int numPersons = ACS::TotalPersons::_from_string(row->at(0).c_str());
		int ageCat = ACS::AgeCat1::_from_string(row->at(1).c_str());

		int numChild = 0;
		for(auto col = row->begin()+2; col != row->end(); ++col)
		{
			m_povertyThreshhold[numPersons][ageCat][numChild] = std::stoi(*col);
			numChild++;
		}
	}

	tagIncomePovertyRatio();
}

void DepressionParams::readDepressionPrevalence()
{
	io::CSVReader<6>depression_preval(getFilePath("pop_mental_health/depression_prevalence.csv"));
	depression_preval.read_header(io::ignore_extra_column, "IP_Ratio", "Sex", "Age_Cat", "Preval_None", "Preval_Other", "Preval_Major");

	const char *inc_poverty_ratio = NULL;
	const char *sex = NULL;
	const char *age_cat = NULL;
	const char* preval_none = NULL;
	const char* preval_other = NULL;
	const char* preval_major = NULL;


	PairDblInt pair_none, pair_other, pair_major;
	while(depression_preval.read_row(inc_poverty_ratio, sex, age_cat, preval_none, preval_other, preval_major))
	{
		std::string personType = getPersonTypeByIPRatio(inc_poverty_ratio, sex, age_cat);

		pair_none = std::make_pair(std::stod(preval_none), Depression::DepressionType::None);
		pair_other = std::make_pair(std::stod(preval_none) + std::stod(preval_other), Depression::DepressionType::Other);
		pair_major = std::make_pair(std::stod(preval_none) + std::stod(preval_other) + std::stod(preval_major), Depression::DepressionType::Major);

		if(pair_major.first <= 1.0)
			pair_major.first = 1.0;

		m_depressionPrevalence[personType].push_back(pair_none);
		m_depressionPrevalence[personType].push_back(pair_other);
		m_depressionPrevalence[personType].push_back(pair_major);
	}
}

void DepressionParams::readDepressionSymptoms()
{
	io::CSVReader<8> depression_symptoms(getFilePath("pop_mental_health/depression_symptoms.csv"));
	depression_symptoms.read_header(io::ignore_extra_column, "Depression_Type", "Age_Cat", "Sex", "IP_Ratio", "FirstQ", "SecondQ", 
									"ThirdQ", "FourthQ");

	const char *depression_type = NULL;
	const char *age_cat = NULL;
	const char *sex = NULL;
	const char *ip_ratio = NULL;
	const char *first_q = NULL;
	const char *second_q = NULL;
	const char *third_q = NULL;
	const char *fourth_q = NULL;

	Tuple symptoms;
	while(depression_symptoms.read_row(depression_type, age_cat, sex, ip_ratio, first_q, second_q, third_q, fourth_q))
	{
		std::string personType = getPersonTypeByDepressionType(depression_type, ip_ratio, sex, age_cat);
		symptoms = std::make_tuple(std::stod(first_q), std::stod(second_q), std::stod(third_q),
			std::stod(fourth_q));

		m_depressionSymptoms.insert(std::make_pair(personType, symptoms));
	}
}

void DepressionParams::tagIncomePovertyRatio()
{
	double ip_ratio = 0;
	PairDblInt ipRatio_tag;
	for(auto tag : ACS::IncomeToPovertyRatio::_values())
	{
		ip_ratio += 0.5;

		ipRatio_tag = std::make_pair(ip_ratio, tag);
		v_ipRatio_ipTag.push_back(ipRatio_tag);
	}

	v_ipRatio_ipTag.pop_back();

}

void DepressionParams::setWageGapProbabilityDist(MapVecPair m_pWageGap)
{
	this->m_pWageGap.clear();
	this->m_pWageGap = m_pWageGap;
}

int DepressionParams::getPovertyThreshold(int hhSize, int ageCat, int numChild)
{
	if(m_povertyThreshhold.count(hhSize) > 0)
	{
		if(m_povertyThreshhold[hhSize].count(ageCat) > 0)
		{
			if(m_povertyThreshhold[hhSize][ageCat].count(numChild) > 0)
			{
				return m_povertyThreshhold[hhSize][ageCat][numChild];
			}
			else
			{
				std::cout <<"Error: " << numChild << " doesn't exist!" << std::endl;
				exit(EXIT_SUCCESS);
			}
		}
		else
		{
			std::cout <<"Error: " << ageCat << " doesn't exist!" << std::endl;
			exit(EXIT_SUCCESS);
		}
	}
	else
	{
		std::cout << "Error: " << hhSize << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

const DepressionParams::VecPairDblInt *DepressionParams::getIncomePovertyRatioTag() const
{
	return &v_ipRatio_ipTag;
}

const DepressionParams::VecPairDblInt *DepressionParams::getDepressionPrevalence(std::string person_type) const
{
	if(m_depressionPrevalence.count(person_type) > 0)
	{
		return &m_depressionPrevalence.at(person_type);
	}
	else
	{
		std::cout << "Error: Person type " << person_type << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

const DepressionParams::Tuple *DepressionParams::getDepressionSymptoms(std::string person_type) const
{
	if(m_depressionSymptoms.count(person_type) > 0)
		return &m_depressionSymptoms.at(person_type);
	else
	{
		std::cout << "Error: Person type " << person_type << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

const DepressionParams::VecPairDblInt *DepressionParams::getWageGapProbabilityDist(std::string person_type) const
{
	if(m_pWageGap.count(person_type) > 0)
		return &m_pWageGap.at(person_type);
	else
	{
		std::cout << "Error: Person type " << person_type << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

std::string DepressionParams::getPersonTypeByIPRatio(const char *ip_ratio, const char *sex, const char *age_cat)
{
	int inc_pov_ratio = ACS::IncomeToPovertyRatio::_from_string(ip_ratio);
	int sex_ = ACS::Sex::_from_string(sex);
	int age_cat_ = Depression::AgeCat::_from_string(age_cat);

	return "IP_Ratio" + std::to_string(sex_) + std::to_string(age_cat_) + std::to_string(inc_pov_ratio);
}

std::string DepressionParams::getPersonTypeByDepressionType(const char *type, const char *ip_ratio, const char *sex, const char* age_cat)
{
	int type_ = Depression::DepressionType::_from_string(type);
	return "Type" + std::to_string(type_) + getPersonTypeByIPRatio(ip_ratio, sex, age_cat);
}