/**
*	@file	 Parameters.cpp	
*	@author	 Shailesh Tamrakar
*	@date	 7/23/2018
*	@version 1.0
*
*	@section DESCRIPTION
*	This source file has methods/functions that - 
*	1. Return path of input files PUMS dataset, household and population-level 
*	estimates, list of Metropolitan Statistical Areas.
*	2. Read and store ACS codebook by variable type (age, education, race...)
*	3. Create maps by pairing variable indexes with its variable types.   
*	3. Create and return all the possible combinations (pool) of households 
*	by type, size and income as well as all person types by age, sex, race 
*	and education. 
*	4. Store and return parameters for testing goodness-of-fit of synthetic 
*	population. 
*/

#include "Parameters.h"
//#include "csv.h"

Parameters::Parameters() {}

Parameters::Parameters(const char *inDir, const char *outDir, const int simModel, const int geoLvl) : 
	inputDir(inDir), outputDir(outDir), alpha(0.05), minSampleSize(1000.0), max_draws(20), simType(simModel), 
	geoLevel(geoLvl), output(true)
{
	readACSCodeBookFile();
	readAgeGenderMappingFile();
	readHHIncomeMappingFile();
	readOriginListFile();

	createHouseholdPool();
	createPersonPool();
	createNhanesPool();
		
}

Parameters::~Parameters()
{
	
}

std::string Parameters::getInputDir() const
{
	return inputDir;
}

std::string Parameters::getOutputDir() const
{
	return outputDir;
}

const char* Parameters::getMSAListFile() 
{
	return getFilePath("ACS_15_METRO_LIST_fullList.csv");
}

const char* Parameters::getUSStateListFile()
{
	return getFilePath("ACS_15_US_STATES.csv");
}

const char* Parameters::getCountiesListFile() 
{
	return getFilePath("ACS_15_Metro_Counties_List.csv");
}

const char* Parameters::getPUMAListFile() 
{
	return getFilePath("ACS_15_Counties_Puma10_List.csv");
}

const char* Parameters::getHouseholdPumsFile(std::string st) 
{
	std::string hhPumsFile = "pums/households/ss15h"+st+".csv";
	return getFilePath(hhPumsFile.c_str());
}

const char* Parameters::getPersonPumsFile(std::string st) 
{
	std::string perPumsFile = "pums/persons/ss15p"+st+".csv";
	return getFilePath(perPumsFile.c_str());
}

const char* Parameters::getRaceMarginalFile() 
{
	switch(geoLevel)
	{
	case Geography::Level::MSA:
		return getFilePath("marginals/2015/MSA/ACS_15_race_by_age_sex.csv");
		break;
	case Geography::Level::STATE:
		return getFilePath("marginals/2015/State/ACS_15_race_by_age_sex.csv");
		break;
	default:
		return NULL;
		break;
	}
}

const char* Parameters::getEducationMarginalFile() 
{
	switch(geoLevel)
	{
	case Geography::Level::MSA:
		return getFilePath("marginals/2015/MSA/ACS_15_edu_by_age_sex.csv");
		break;
	case Geography::Level::STATE:
		return getFilePath("marginals/2015/State/ACS_15_edu_by_age_sex.csv");
		break;
	default:
		return NULL;
		break;
	}
}

const char* Parameters::getHHTypeMarginalFile() 
{
	switch(geoLevel)
	{
	case Geography::Level::MSA:
		return getFilePath("marginals/2015/MSA/ACS_15_household_type.csv");
		break;
	case Geography::Level::STATE:
		return getFilePath("marginals/2015/State/ACS_15_household_type.csv");
		break;
	default:
		return NULL;
		break;
	}
}

const char* Parameters::getHHSizeMarginalFile() 
{
	switch(geoLevel)
	{
	case Geography::Level::MSA:
		return getFilePath("marginals/2015/MSA/ACS_15_household_size.csv");
		break;
	case Geography::Level::STATE:
		return getFilePath("marginals/2015/State/ACS_15_household_size.csv");
		break;
	default:
		return NULL;
		break;
	}
}

const char* Parameters::getHHIncomeMarginalFile()
{
	switch(geoLevel)
	{
	case Geography::Level::MSA:
		return getFilePath("marginals/2015/MSA/ACS_15_household_income.csv");
		break;
	case Geography::Level::STATE:
		return getFilePath("marginals/2015/State/ACS_15_household_income.csv");
		break;
	default:
		return NULL;
		break;
	}
}

const char* Parameters::getGQMarginalFile() 
{
	switch(geoLevel)
	{
	case Geography::Level::MSA:
		return getFilePath("marginals/2015/MSA/ACS_15_group_quarters.csv");
		break;
	case Geography::Level::STATE:
		return getFilePath("marginals/2015/State/ACS_15_group_quarters.csv");
		break;
	default:
		return NULL;
		break;
	}
}

double Parameters::getAlpha() const
{
	return alpha;
}

double Parameters::getMinSampleSize() const
{
	return minSampleSize;
}

int Parameters::getMaxDraws() const
{
	return max_draws;
}

short int Parameters::getSimType() const
{
	return simType;
}

short int Parameters::getGeoType() const
{
	return geoLevel;
}

bool Parameters::isStateLevel() const
{
	if(geoLevel == Geography::Level::STATE)
		return true;
	else
		return false;
}

bool Parameters::writeToFile() const
{
	return output;
}

const Parameters::Pool * Parameters::getHouseholdPool() const
{
	return &hhPool;
}

const Parameters::Pool * Parameters::getPersonPool() const
{
	return &personPool;
}

const Parameters::Pool * Parameters::getNhanesPool() const
{
	return &nhanesPool;
}

const Parameters::Pool * Parameters::getNhanesPoolMap(std::string type)
{
	return &m_nhanesPool[type];
}

//std::unordered_multimap<int, int> Parameters::getVariableMap(int type) const
std::multimap<int, int> Parameters::getVariableMap(int type) const
{
	//std::unordered_multimap<int, int> temp;
	std::multimap<int, int> temp;
	switch(type)
	{
	case ACS::Estimates::estEducation:
		return m_eduAgeGender;
		break;
	case ACS::Estimates::estHHIncome:
		return m_hhIncome;
		break;
	default:
		return temp;
		break;
	}
}

//std::unordered_map<std::string, int> Parameters::getOriginMapping() const
Parameters::MapInt Parameters::getOriginMapping() const
{
	return m_originByRace;
}



Parameters::MultiMapCB Parameters::getACSCodeBook() const
{
	return m_acsCodes;
}


/**
*	@brief Reads and stores PUMS data dictionary for housing and
*	population records
*	@section DESCRIPTION
*	Iterates and stores each line of ACS Codebook file until eof is reached.
*	Create a multimap paired with PUMS variable type and a map paired with
*	ACS integer codes for attributes and its definitions.
*	@param none
*	@return void
*/
void Parameters::readACSCodeBookFile()
{
	//const char* codeBookFile = getFilePath("pums\\ACS_2010_PUMS_codebook.csv");
	const char* codeBookFile = getFilePath("pums/ACS_2015_PUMS_codebook.csv");
	std::ifstream ifs;

	ifs.open(codeBookFile, std::ios::in);

	if(!ifs.is_open()){
		std::cout << "Error: Cannot open " << codeBookFile << "!" << std::endl;
		exit(EXIT_SUCCESS);
	}
	
	Columns col;
	Rows row;
	std::string line;
	
	while(std::getline(ifs, line))
	{
		Tokenizer temp(line);
		col.assign(temp.begin(), temp.end());
		row.push_back(col);

		if(col.empty())
		{
			std::string codeACS = row.front().at(0);
			for(auto it = row.begin()+1; it != row.end(); ++it)
			{
				if(!it->empty())
					m_codeBook.insert(std::make_pair(codeACS, *it));
			}
			col.clear();
			row.clear();
		}
	}

	m_acsCodes.insert(make_pair(ACS::PumsVar::RAC1P, createCodeBookMap(ACS::PumsVar::RAC1P)));
	m_acsCodes.insert(make_pair(ACS::PumsVar::SCHL, createCodeBookMap(ACS::PumsVar::SCHL)));
	m_acsCodes.insert(make_pair(ACS::PumsVar::AGEP, createCodeBookMap(ACS::PumsVar::AGEP)));

	m_acsCodes.insert(make_pair(ACS::PumsVar::HHT, createCodeBookMap(ACS::PumsVar::HHT)));
	m_acsCodes.insert(make_pair(ACS::PumsVar::HINCP, createCodeBookMap(ACS::PumsVar::HINCP)));

}

/**
*	@brief Creates a multimap by pairing "Sex-EduAge" type ((Male, 18-24),...) with "Sex-Age"
*	variable index (POP_M_18_19,...POP_M_22_24,...). 
*	@param none
*	@return void
*/
void Parameters::readAgeGenderMappingFile() 
{
	io::CSVReader<4>age_gender_map_file(getFilePath("variables/age_gender_map.csv"));
	age_gender_map_file.read_header(io::ignore_extra_column, "Gender", "Edu_Age_Range", "Mar_Age_Range", "Race_Variable");

	const char* gender = NULL;
	const char* edu_age_range = NULL;
	const char* marital_age_range = NULL;
	const char* var_name = NULL;

	while(age_gender_map_file.read_row(gender, edu_age_range, marital_age_range, var_name))
	{
		int sex = ACS::Sex::_from_string(gender);
		int raceVarIdx = ACS::RaceMarginalVar::_from_string(var_name);

		std::string r_eduAge(edu_age_range);
		std::string r_marAge(marital_age_range);

		if(r_eduAge != "NULL")
		{
			int eduAge = ACS::EduAgeCat::_from_string(edu_age_range);
			m_eduAgeGender.insert(std::make_pair(10*sex+eduAge, raceVarIdx-1));
		}

	}
}

/**
*	@brief Creates a multimap by pairing hhIncome type (hhInc1..hhInc10) with hhIncome 
*	variable index (HH_INC1,.....HHINC16).
*	@param none
*	@return void
*/
void Parameters::readHHIncomeMappingFile()
{
	io::CSVReader<2>hhIncome_map_file(getFilePath("variables/hhIncome_map.csv"));
	hhIncome_map_file.read_header(io::ignore_extra_column, "HHIncome", "Variable");

	const char* hhIncStr = NULL;
	const char* var_name = NULL;

	while(hhIncome_map_file.read_row(hhIncStr, var_name))
	{
		int hhIncCat = ACS::HHIncome::_from_string(hhIncStr);
		int hhIncIdx = ACS::HHIncMarginalVar::_from_string(var_name);

		m_hhIncome.insert(std::make_pair(hhIncCat, hhIncIdx));
	}

}

/**
*	@brief Creates a multimap by pairing race/origin type (Hispanic, White alone..) with
*	its corresponding PUMS code(1,...,8).
*	@param none
*	@return void
*/
void Parameters::readOriginListFile()
{
	Rows originList = readCSVFile(getFilePath("variables/race_by_origin.csv"));
	originList.erase(originList.begin());

	for(auto row = originList.begin(); row != originList.end(); ++row)
		m_originByRace.insert(std::make_pair(row->front(), std::stoi(row->back())));
}


/**
*	@brief Creates pool of group-quarters and households by type, size and income.
*	@param none
*	@return void
*/
void Parameters::createHouseholdPool()
{
	std::string gqType = "-1";
	std::string gqSize = "1";
	std::string gqInc = "-1";

	hhPool.push_back(gqType+gqSize+gqInc);

	for(auto hhType : ACS::HHType::_values())
		for(auto hhSize : ACS::HHSize::_values())
			for(auto hhInc : ACS::HHIncome::_values())
				hhPool.push_back(std::to_string(hhType)+std::to_string(hhSize)+std::to_string(hhInc));

}

/**
*	@brief Creates possible pool of persons by age, sex, race and educational attaiment.
*	@param none
*	@return void
*/
void Parameters::createPersonPool()
{
	std::string dummy = "0";
	//pool of persons by sex, age and origin
	for(auto sex : ACS::Sex::_values())
		for(auto ageCat : ACS::AgeCat::_values())
			for(auto org : ACS::Origin::_values())
				personPool.push_back(dummy+std::to_string(sex)+std::to_string(ageCat)+std::to_string(org));

	//pool of person by sex, education age cat, origin and education attainment
	for(auto sex : ACS::Sex::_values())
		for(auto eduAge : ACS::EduAgeCat::_values())
			for(auto org : ACS::Origin::_values())
				for(auto edu : ACS::Education::_values())
					personPool.push_back(std::to_string(sex)+std::to_string(eduAge)+std::to_string(org)+std::to_string(edu));
}

void Parameters::createNhanesPool()
{
	for(auto org : NHANES::Org::_values())
	{
		for(auto sex : NHANES::Sex::_values())
		{
			std::string person_type1 = std::to_string(org) + std::to_string(sex);
			for(auto age : NHANES::AgeCat::_values())
			{
				for(auto edu : NHANES::Edu::_values())
				{
					std::string person_type2 = getNHANESpersonType(org, sex, age, edu);
					nhanesPool.push_back(person_type2);

					for(size_t rs = 1; rs <= NUM_RISK_STRATA; ++rs)
					{
						std::string person_type3 = std::to_string(rs) + person_type2;
						m_nhanesPool[person_type1].push_back(person_type3);
					}
				}
			}
		}
	}
}

/*
* @brief Returns the NHANES person type by race, gender, age category and educational attainment
* @param race NHANES race type
* @param sex NHANES gender type
* @param age NHANES age category
* @param edu NHANES educational attainment
* @return person type by race, gender, age cat and education cat
*/
std::string Parameters::getNHANESpersonType(const char *race, const char *sex, const char *age, const char *edu)
{
	std::string race_ = std::to_string(NHANES::Org::_from_string(race));
	std::string sex_ = std::to_string(NHANES::Sex::_from_string(sex));

	//std::string age_cat_ = std::to_string(NHANES::AgeCat3::_from_string(age));
	std::string age_cat_ = std::to_string(NHANES::AgeCat::_from_string(age));

	std::string edu_cat_ = std::to_string(NHANES::Edu::_from_string(edu));

	return (race_+sex_+age_cat_+edu_cat_);
}

std::string Parameters::getNHANESpersonType(const char *race, const char *sex, const char *age_cat)
{
	std::string race_ = std::to_string(NHANES::Org::_from_string(race));
	std::string sex_ = std::to_string(NHANES::Sex::_from_string(sex));
	std::string age_cat_ = std::to_string(NHANES::AgeCat3::_from_string(age_cat));
	
	return (race_+sex_+age_cat_);
}

std::string Parameters::getNHANESpersonType(int race, int sex, int age, int edu)
{
	return (std::to_string(race)+std::to_string(sex)+std::to_string(age)+std::to_string(edu));
}


bool Parameters::isEmpty(const char *vars[], int size)
{
	std::vector<bool>emptyStrings;

	for(int i = 0; i < size; ++i)
	{
		std::string s_vars(vars[i]);
		if(s_vars.empty())
			emptyStrings.push_back(true);
	}

	if(emptyStrings.size() > 0)
		return true;
	else
		return false;
}

/**
*	@brief Pairs ACS codes with their respective definitions
*	@param var is PUMS variable type (race, education, hhIncome,...)
*	@return map paired with PUMS ACS codes and its definition
*/
Parameters::MapInt Parameters::createCodeBookMap(ACS::PumsVar var)
{
	MapInt map;
	std::string str = var._to_string();

	auto its = m_codeBook.equal_range(str);

	for(auto it = its.first; it != its.second; ++it)
	{
		std::string key = it->second.back();
		int val = std::stoi(it->second.front());
		map.insert(std::make_pair(key, val));
	}

	m_codeBook.erase(str);

	return map;
}

/**
*	@param CSVfileName is name of file to be imported
*	@return result is complete file path for the file to be imported
*/
const char* Parameters::getFilePath(const char *CSVfileName)
{
	size_t bufferSize = strlen(inputDir) + strlen(CSVfileName) + 1;

	char *temp = new char[bufferSize];
	strcpy(temp, inputDir);
	strcat(temp, CSVfileName);

	char *result = temp;
	temp = NULL;
	delete [] temp;

	return result;
	
}

Parameters::Rows Parameters::readCSVFile(const char* file)
{
	Columns col;
	Rows row;

	std::ifstream ifs;
	ifs.open(file, std::ios::in);

	if(!ifs.is_open())
	{
		std::cout << "Error: Cannot open " << file << std::endl;
		exit(EXIT_SUCCESS);
	}

	std::string line;
	while(std::getline(ifs, line))
	{
		Tokenizer temp(line);
		col.assign(temp.begin(), temp.end());
		row.push_back(col);
	}

	return row;
}


