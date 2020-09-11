/**
*	@file	 PopBrewer.cpp	
*	@author	 Shailesh Tamrakar
*	@date	 7/23/2018
*	@version 1.0
*
*	@section DESCRIPTION
*	This source file includes methods/functions of PopBrewer class to -  
*	1. Create and map US Metropolitan Statistical Areas (MSAs) (or US states) with
*	their counties and PUMA codes by using PUMA-County Crosswalk.
*	2. Import household-level estimates of household type by household 
*	size and household income.
*	3. Import person-level estimates of education by age and sex, race by 
*	age and sex. 
*	
*	The population and household estimates are derived from ACS 2011-2015 
*	and is publicly available at "www.factfinder.census.gov".
*/

#include "PopBrewer.h"
#include "ViolenceParams.h"
#include "CardioParams.h"
#include "DepressionParams.h"
#include "County.h"
#include "Area.h"
//#include "csv.h"

template class PopBrewer<ViolenceParams>;
template class PopBrewer<CardioParams>;
template class PopBrewer<DepressionParams>;

/*
* @brief Default class constructor
*/
template <class GenericParams>
PopBrewer<GenericParams>::PopBrewer(){}


/*
* @brief Overloaded class constructor
* @param param CHD model input parameters
*/
template <class GenericParams>
PopBrewer<GenericParams>::PopBrewer(GenericParams *param) : parameters(param)
{
	
}

/*
* @brief Class destructor
*/
template <class GenericParams>
PopBrewer<GenericParams>::~PopBrewer()
{
	std::cout << "Pop Brewer destructor!" << std::endl;
}

/*
* @brief Sets CHD model input parameters as shared pointer
* @brief param CHD model input parameters
*/
template <class GenericParams>
void PopBrewer<GenericParams>::setParameters(const GenericParams &param)
{
	parameters = std::make_shared<GenericParams>(param);
}

/*
* @brief Calls method to import list of MSAs or US states. 
*        Calls method to import ACS 2015 estimates
*/
template <class GenericParams>
void PopBrewer<GenericParams>::import()
{
	importArea(parameters->getGeoType());
	importEstimates();
}

/*
* @brief Calls method to import list of MSAs or list of US states
* @param geoType Type of geographical orientation (MSA or US state)
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importArea(int geoType)
{
	switch(geoType)
	{
		case Geography::Level::MSA:
			importMetroArea();
			break;
		case Geography::Level::STATE:
			importState();
		default:
			break;
	}

}

/**
*	@brief Imports, iterates through list of MSA and pairs MSAs with their counties 
*	and PUMA codes. 
*   Steps: 1. Map MSA to counties
*          2. Map Counties to PUMA code.
*          3. Iterate through list of MSAs. For each county in MSA,
*	@param none
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importMetroArea()
{
	std::cout << "Importing Metropolitan Statistical Areas..." << std::endl;

	//mapping between metro area and their respective counties 
	std::multimap<std::string, std::string> msa_county_map;
	mapMetroToCounties(parameters->getCountiesListFile(), msa_county_map);

	//mapping between counties and their respective puma codes
	std::multimap<std::string, County> county_puma_map;
	mapCountiesToPUMA(parameters->getPUMAListFile(), county_puma_map);

	io::CSVReader<3> msaList(parameters->getMSAListFile());
	msaList.read_header(io::ignore_extra_column, "GEO_ID_2", "MSA_NAME", "TOT_POP");

	std::string geoID = "";
	std::string msaName = "";
	std::string abbv = "";
	std::string totPop = "";

	int num_counties = 0;
	
	while(msaList.read_row(geoID, msaName, totPop))
	{
		Area<GenericParams> *metro = new Area<GenericParams>(parameters);

		metro->setAreaIDandName(geoID, msaName);
		metro->setPopulation(std::stoi(totPop));

		//Get a list of county names in a MSA
		auto range_msa = msa_county_map.equal_range(msaName);

		if(range_msa.first == range_msa.second)
		{
			std::cout << msaName << " doesn't exist in County List File!" << std::endl;
			exit(EXIT_SUCCESS);
		}

		for(auto countyByMSA = range_msa.first ; countyByMSA != range_msa.second; ++countyByMSA)
		{
			std::string county_msa = countyByMSA->second;

			//Get list of counties in a MSA
			auto range_county = county_puma_map.equal_range(county_msa);

			if(range_county.first == range_county.second)
			{
				std::cout << county_msa << " of " << msaName << " doesn't exist in PUMA List file! " << std::endl;
				exit(EXIT_SUCCESS);
			}

			for(auto pumaByCounty = range_county.first; pumaByCounty != range_county.second; ++pumaByCounty)
			{
				metro->setCounties(pumaByCounty->second);
			}

			num_counties += (metro->getCountyNum(county_msa) >=1) ? 1 : 0;

			if(metro->getCountyNum(county_msa) == 0)
			{
				std::cout << "No counties in " << msaName << " !" << std::endl;
			}

			county_puma_map.erase(county_msa);
		}


		msa_county_map.erase(msaName);
		m_geoAreas.insert(std::make_pair(metro->getGeoID(), *metro));
		
		delete metro;
	}

	
	county_puma_map.clear();
	msa_county_map.clear();

	std::cout << "Successfully imported!\n" << std::endl;
}

/*
* @brief Import list of states from the input file
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importState()
{
	std::cout << "Importing US States..." << std::endl;

	io::CSVReader<4> stateList(parameters->getUSStateListFile());
	stateList.read_header(io::ignore_extra_column, "GEO_ID_2", "STATE", "ABBV", "TOT_POP");

	std::string geoID = "";
	std::string state_name = "";
	std::string abbv = "";
	std::string totPop = "";
	
	while(stateList.read_row(geoID, state_name, abbv, totPop))
	{
		Area<GenericParams> *state = new Area<GenericParams>(parameters);

		state->setAreaIDandName(geoID, state_name);
		state->setAreaAbbreviation(abbv);
		state->setPopulation(std::stoi(totPop));

		m_geoAreas.insert(std::make_pair(state->getGeoID(), *state));
		
		delete state;
	}

	std::cout << "Successfully imported!\n" << std::endl;
}

/*
* @brief Calls methods to import:
*        1. ACS race estimates
		 2. ACS Educational attainment estimates
		 3. ACS Household type estimates
		 4. ACS Household size estimates
		 5. ACS Household income estimates
		 6. ACS Group Quarter estimates
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importEstimates()
{
	importRaceEstimates();
	importEducationEstimates();

	importHHTypeEstimates();
	importHHSizeEstimates();
	importHHIncomeEstimates();

	importGQEstimates();
}


/**
*	@brief Imports and sets race/origin estimates by age and sex
*	@param none
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importRaceEstimates()
{
	std::cout << "Importing Race Estimates by Age and Sex...." << std::endl;
	
	Rows raceEst = readCSVFile(parameters->getRaceMarginalFile());

	//1. Extract and remove headers from input
	Columns raceHdr = raceEst.front();
	raceEst.pop_front();

	std::list<ACS::RaceMarginalVar> raceVarsListMale;

	for(auto var : ACS::RaceMarginalVar::_values())
		raceVarsListMale.push_back(var);

	//2. Map column index with header names
	std::map<int, int> m_raceIdx(getColumnIndexMap(&raceVarsListMale, &raceHdr));

	//3. Set estimates
	setEstimates(raceEst, m_raceIdx, ACS::RaceMarginalVar::_size(), ACS::Estimates::estRace);
	
	std::cout << "Import Complete!\n" << std::endl;
}

/**
*	@brief Imports and sets education estimates by age and sex
*	@param none
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importEducationEstimates()
{
	std::cout << "Importing Education Estimates by Age and Sex..." << std::endl;

	Rows eduEst = readCSVFile(parameters->getEducationMarginalFile());

	Columns eduHdr = eduEst.front();
	eduEst.pop_front();

	std::list<ACS::EduMarginalVar1>eduVarListMale;

	for(auto val_male : ACS::EduMarginalVar1::_values())
		eduVarListMale.push_back(val_male);

	std::list<ACS::EduMarginalVar2> eduVarListFemale;

	for(auto val_female : ACS::EduMarginalVar2::_values())
		eduVarListFemale.push_back(val_female);

	std::map<int, int> m_eduIdx(getColumnIndexMap(&eduVarListMale, &eduHdr));
	std::map<int, int> m_female_eduIdx(getColumnIndexMap(&eduVarListFemale, &eduHdr));

	m_eduIdx.insert(m_female_eduIdx.begin(), m_female_eduIdx.end());

	setEstimates(eduEst, m_eduIdx, ACS::EduMarginalVar1::_size() +ACS::EduMarginalVar2::_size(), ACS::Estimates::estEducation);

	std::cout << "Import Complete!\n" << std::endl;
}

/**
*	@brief Imports and sets household type estimates
*	@param none
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importHHTypeEstimates()
{
	std::cout << "Importing Household Type Estimates..." << std::endl;

	Rows hhTypeEst = readCSVFile(parameters->getHHTypeMarginalFile());

	Columns hhTypeHdr = hhTypeEst.front();
	hhTypeEst.pop_front();

	std::list<ACS::HHTypeMarginalVar> hhTypeVarList;

	for(auto val : ACS::HHTypeMarginalVar::_values())
		hhTypeVarList.push_back(val);
	
	std::map<int, int> m_hhTypeIdx(getColumnIndexMap(&hhTypeVarList, &hhTypeHdr));

	setEstimates(hhTypeEst, m_hhTypeIdx, ACS::HHTypeMarginalVar::_size(), ACS::Estimates::estHHType);

	std::cout << "Import Complete!\n" << std::endl;
}

/**
*	@brief Imports and sets household size estimates
*	@param none
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importHHSizeEstimates()
{
	std::cout << "Importing Household Size Estimates..." << std::endl;

	Rows hhSizeEst = readCSVFile(parameters->getHHSizeMarginalFile());

	Columns hhSizeHdr = hhSizeEst.front();
	hhSizeEst.pop_front();

	std::list<ACS::HHSizeMarginalVar>hhSizeVarList;

	for(auto val : ACS::HHSizeMarginalVar::_values())
		hhSizeVarList.push_back(val);

	std::map<int, int> m_hhSizeIdx(getColumnIndexMap(&hhSizeVarList, &hhSizeHdr));

	setEstimates(hhSizeEst, m_hhSizeIdx, ACS::HHSizeMarginalVar::_size(), ACS::Estimates::estHHSize);

	std::cout << "Import Complete!\n" << std::endl;
}

/**
*	@brief Imports and sets household income estimates
*	@param none
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importHHIncomeEstimates()
{
	std::cout << "Importing Household Income Estimates..." << std::endl;

	Rows hhIncEst = readCSVFile(parameters->getHHIncomeMarginalFile());

	Columns hhIncHdr = hhIncEst.front();
	hhIncEst.pop_front();

	std::list<ACS::HHIncMarginalVar>hhIncVarList;

	for(auto val : ACS::HHIncMarginalVar::_values())
		hhIncVarList.push_back(val);

	std::map<int, int> m_hhIncIdx(getColumnIndexMap(&hhIncVarList, &hhIncHdr));

	setEstimates(hhIncEst, m_hhIncIdx, ACS::HHIncMarginalVar::_size(), ACS::Estimates::estHHIncome);

	std::cout << "Import Complete!\n" << std::endl;
}

/**
*	@brief Imports and sets Group Quarter(GQ) estimates
*	@param none
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::importGQEstimates()
{
	std::cout << "Importing Group Quarter Estimates..." << std::endl;

	Rows grpQtrEst = readCSVFile(parameters->getGQMarginalFile());

	Columns grpQtrHdr = grpQtrEst.front();
	grpQtrEst.pop_front();

	std::list<ACS::GQMarginalVar> gqVarList;

	gqVarList.push_back(ACS::GQMarginalVar::GQ_POP);

	std::map<int, int> m_gqIdx(getColumnIndexMap(&gqVarList, &grpQtrHdr));

	setEstimates(grpQtrEst, m_gqIdx, ACS::GQMarginalVar::_size(), ACS::Estimates::estGQ);

	std::cout << "Import Complete!\n" << std::endl;
}

/**
*	@brief Sets population/household estimates by variable type (education, race, hhIncome...)
*	@param rows is M X N matrix containing pop/household estimates of MSAs, where M = MSA, N = no.of variables (1...n)
*	@param m_idx is a map containing variable's column indexes
*	@param size is number of enumerated variables in person/household level estimates defined in "ACS.h"
*	@param type is enumerated variable by type of pop/household estimates (ACS::Estimates::...) defined in "ACS.h"
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::setEstimates(const Rows &rows, const std::map<int, int>&m_idx, const size_t size, int type)
{
	for(auto curRow = rows.begin(); curRow != rows.end(); ++curRow)
	{
		std::string geoID = curRow->front();
		if(m_geoAreas.count(geoID) == 0)
			continue;

		Area<GenericParams> *area = &m_geoAreas.at(geoID);

		Columns est;
		for(size_t i = 0; i < size; ++i)
			est.push_back(curRow->at(m_idx.at(i)));

		if (est.size() == 0)
		{
			std::cout << "Error: No Estimates added!" << std::endl;
			exit(EXIT_SUCCESS);
		}

		area->setEstimates(est, type);
	}
}

/**
*	@brief Maps MSA with their respective counties
*	@param countyFile is a file containing list MSAs and their counties
*	@param m_map is a multimap to store MSA-County pairs
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::mapMetroToCounties(const char* countyFile, std::multimap<std::string, std::string> &m_map)
{	
	//const int num_vars = 2;
	io::CSVReader<2> countyList(countyFile);
	countyList.read_header(io::ignore_extra_column, "MSA", "Counties");

	std::string metro = "";
	std::string county = "";
	while(countyList.read_row(metro, county))
		m_map.insert(std::make_pair(metro, county));
}

/**
*	@brief Maps US counties with PUMA codes and calculates population
*	weight of counties within a PUMA region
*	@param pumaFile is a file list of all US counties, PUMA codes and 
*	population count
*	@param m_map is a multimap to store PUMA-County pairs
*	@return void
*/
template <class GenericParams>
void PopBrewer<GenericParams>::mapCountiesToPUMA(const char* pumaFile, std::multimap<std::string, County> &m_map)
{
	io::CSVReader<4> pumaList(pumaFile);
	pumaList.read_header(io::ignore_extra_column, "cntyname", "State", "PUMA10", "Pop14");

	std::string county = "";
	std::string state = ""; 
	std::string puma_code = "";
	std::string population = "";

	std::multimap<std::string, County> temp_puma_county_map;

	//Map PUMA code with counties
	while(pumaList.read_row(county, state, puma_code, population))
	{

		County tempCnty;
		tempCnty.setCountyName(county);
		tempCnty.setPumaCode(std::stoi(puma_code));
		tempCnty.setPopulation(std::stoi(population));

		temp_puma_county_map.insert(std::make_pair(state+puma_code, tempCnty));
	}

	//Compute population weight for each county in a PUMA region.
	for(auto puma = temp_puma_county_map.begin(); puma != temp_puma_county_map.end();)
	{
		auto puma_range = temp_puma_county_map.equal_range(puma->first);

		int sum_pop = 0;
		for(auto it2 = puma_range.first; it2 != puma_range.second; ++it2)
		{
			sum_pop += it2->second.getPopulation();
		}

		
		for(auto it3 = puma_range.first; it3 != puma_range.second; ++it3)
		{
			double weight = ((double)it3->second.getPopulation()/sum_pop);
			it3->second.setPopulationWeight(weight);

			m_map.insert(std::make_pair(it3->second.getCountyName(), it3->second));
		}

		puma = puma_range.second;
	}
}

/*
* @brief Reads and tokenizes CSV file containing ACS estimates.
* @param file Path to the csv file
* @return M X N vector containing population/household estimates 
*/
//source:https://stackoverflow.com/questions/904472/templated-member-function-with-typedef-return-value
template <class GenericParams>
typename PopBrewer<GenericParams>::Rows PopBrewer<GenericParams>::readCSVFile(const char *file)
{
	Columns col;
	Rows row;

	std::ifstream ifs;
	
	ifs.open(file, std::ios::in);
	if(!ifs.is_open())
	{
		std::cout << "Error: Cannot open " << file << " file!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	std::string line;
	while(std::getline(ifs, line))
	{
		TokenizerCSVFile temp(line);
		col.assign(temp.begin(), temp.end());
		row.push_back(col);
	}

	return row;
}

/*
* @brief Maps and returns the index of ACS variables to be extracted from the input estimate file.
* @param varList List of variables to be extracted from the file
* @param col List of variables in input estimate file
* @return Map with column name and column index (index of variable in the input file)
*/
template <class GenericParams>
template<class T>
std::map<int, int> PopBrewer<GenericParams>::getColumnIndexMap(std::list<T> *varList, Columns *col)
{
	std::map<int, int> m_temp;
	for(auto it = varList->begin(); it != varList->end(); ++it)
	{
		std::string varStr = it->_to_string();
		int colIdx = getColumnIndex(col, varStr);

		if(colIdx < 0)
			exit(EXIT_SUCCESS);

		m_temp.insert(std::make_pair(it->_to_integral(), colIdx));
	}
	
	return m_temp;
}

/*
* @brief Returns the column index of ACS estimate variable
* @param col List of ACS estimate variable in the input file
* @param varName2 Variable to be extracted from the input file
*/
template <class GenericParams>
int PopBrewer<GenericParams>::getColumnIndex(Columns *col, std::string varName2)
{
	int idx = 0;
	
	std::string varName1;
	for(auto itr = col->begin(); itr != col->end(); ++itr)
	{
		varName1 = *itr;

		if (varName1.compare(varName2) == 0)
			return idx;
		idx++;
			
	}
	
	std::cout <<"Error: " <<varName2 << "," << varName2.size() << " does not exist in the Column!" << std::endl;
	
	return -1;
}