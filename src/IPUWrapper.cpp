/*
* @Description IPUWrapper class handles the import of Household and Person 
*              level PUMS dataset. It also performs IPF (Iterative Proportional Fitting)
*              to compute ACS estimates (household and person level). And, it initiates
*              IPU (Iterative Propertional Updating) to compute household selection probabilities.
*              
*/

#include "IPUWrapper.h"
#include "ViolenceParams.h"
#include "CardioParams.h"
#include "DepressionParams.h"
#include "County.h"
#include "IPF.h"
#include "IPU.h"
#include "NDArray.h"
#include "csv.h"
#include "ElapsedTime.h"
//#include <ctime>
#include <boost/algorithm/string.hpp>


#ifdef _WIN32
#include <Windows.h>
#endif

//Define these method prototypes of all the models
template class IPUWrapper<ViolenceParams>;
template class IPUWrapper<CardioParams>;
template class IPUWrapper<DepressionParams>;

/*
* @brief Class constructor
* @param param Shared pointer to parameters
* @param m_acsEst Multimap to store 2015 ACS estimates 
* @param mapCountyPuma Mapping between PUMA codes and counties
*/
template<class GenericParams>
IPUWrapper<GenericParams>::IPUWrapper(std::shared_ptr<GenericParams>param, ACSEstimates *m_acsEst, CountyMap *mapCountyPuma) : 
	parameters(param), m_acsEstimates(m_acsEst), m_pumaCounty(mapCountyPuma)
{
}

/*
* @brief Class destructor. Deletes IPU object.
*/
template<class GenericParams>
IPUWrapper<GenericParams>::~IPUWrapper()
{
	delete ipu;
}

/*
* @brief This method invokes method to:
*        1. Import Household and Person-level PUMS dataset for each state
*        2. Compute ACS estimates with IPF
*        3. Refine Household PUMS dataset
*        4. Execute the IPU
*/
template<class GenericParams>
void IPUWrapper<GenericParams>::startIPU(std::string geoID, std::string areaAbbv, int tot_pop, bool run)
{
	this->geoID = geoID;
	this->totalPop = tot_pop;
	this->areaAbbv = areaAbbv;

	Columns states(getStateList());
	for(size_t i = 0; i < states.size(); ++i)
	{
		importHouseholdPUMS(states[i]);
		importPersonPUMS(states[i]);
	}

	computeHouseholdEst();
	computePersonEst();

	refineHHPumsList();

	std::cout << "Starting IPU...\n" << std::endl;

	if(run){
		ipu = new IPU<GenericParams>(&m_householdPUMS, ipuCons, true);
		ipu->start();
	}
	else{
		std::cout << "Error: Cannot start IPU! " << std::endl;
		exit(EXIT_SUCCESS);
	}

}

/*
* @brief Returns true if IPU is successfully completed
*/
template<class GenericParams>
bool IPUWrapper<GenericParams>::successIPU()
{
	return ipu->success();
}

//template<class GenericParams>
//typename const IPUWrapper<GenericParams>::ProbMap * IPUWrapper<GenericParams>::getHouseholdProbability() const
//{
//	return ipu->getHHProbability();
//}

/*
* @brief Returns household selection probability from IPU
*/
template<class GenericParams>
const typename IPUWrapper<GenericParams>::ProbMap * IPUWrapper<GenericParams>::getHouseholdProbability() const
{
	return ipu->getHHProbability();
}

//template<class GenericParams>
//typename const IPUWrapper<GenericParams>::HouseholdsMap * IPUWrapper<GenericParams>::getHouseholds() const
//{
//	return &m_householdPUMS;
//}

/*
* @brief Returns household level PUMS dataset
*/
template<class GenericParams>
const typename IPUWrapper<GenericParams>::HouseholdsMap * IPUWrapper<GenericParams>::getHouseholds() const
{
	return &m_householdPUMS;
}

/*
* @brief Returns the count of households by type, size and income
* @param type Household (by type, size and income). Type is family household, single household etc
*/
template<class GenericParams>
double IPUWrapper<GenericParams>::getHouseholdCount(std::string type) const
{
	return ipu->getHHCount(type);
}

//template<class GenericParams>
//typename const IPUWrapper<GenericParams>::Marginal * IPUWrapper<GenericParams>::getConstraints() const
//{
//	return &ipuCons;
//}

template<class GenericParams>
const typename IPUWrapper<GenericParams>::Marginal * IPUWrapper<GenericParams>::getConstraints() const
{
	return &ipuCons;
}

/*
* @brief Returns the population size scaled by 1.05
*/
template<class GenericParams>
int IPUWrapper<GenericParams>::getPopSize() const
{
	return (int)(1.05*totalPop);
}

/*
* @brief Returns the list of states
*/
template<class GenericParams>
typename IPUWrapper<GenericParams>::Columns IPUWrapper<GenericParams>::getStateList()
{
	Columns states, tok;
	int state_count = 0;

	//If geographic level is State level
	if(parameters->isStateLevel())
	{
		std::string lower = areaAbbv;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		states.push_back(lower);

		return states;
	}
	
	//If geographic level is MSA
	for(auto county = m_pumaCounty->begin(); county != m_pumaCounty->end(); ++county)
	{
		std::string countyName = county->second.getCountyName();
		std::transform(countyName.begin(), countyName.end(), countyName.begin(), ::tolower);

		boost::split(tok, countyName, boost::is_any_of(" "));
		
		if(states.size() == 0)
		{
			states.push_back(tok.back());
		}
		else
		{
			state_count = std::count(states.begin(), states.end(), tok.back());
			if(state_count == 0)
				states.push_back(tok.back());
		}	
	}

	return states;
}

/*
* @brief Imports Household level PUMS dataset for each state
* @param state US state
*/
template<class GenericParams>
void IPUWrapper<GenericParams>::importHouseholdPUMS(std::string state)
{
	std::string state_upper_case = state;
	std::transform(state_upper_case.begin(), state_upper_case.end(), state_upper_case.begin(), ::toupper);

	std::cout << "Importing Household PUMS file for: " << state_upper_case << " state..." << std::endl;
	
	double waitTime = 2000; //2 seconds wait time
	ElapsedTime timer, benchmark;

	benchmark.start();
	
	io::CSVReader<7>householdPumsFile(parameters->getHouseholdPumsFile(state));
	householdPumsFile.read_header(io::ignore_extra_column, "SERIALNO", "ADJINC", "PUMA10", "NP", "HHT", "HINCP", "NRC");

	std::string hhIdx = ""; 
	std::string adjinc = "";
	std::string puma = "";
	std::string hhSize = ""; 
	std::string hhType = "";
	std::string hhIncome = "";
	std::string numChild = "";

	int countHH = 0;
	while(householdPumsFile.read_row(hhIdx, adjinc, puma, hhSize, hhType, hhIncome, numChild))
	{
		if(isValidPUMA(std::stoi(puma)))
		{
			HouseholdPums<GenericParams> *hhPums = new HouseholdPums<GenericParams>(parameters);

			hhPums->setPUMA(puma);
			hhPums->setHouseholds(hhIdx, hhType, hhSize, hhIncome, numChild, adjinc);

			if(hhPums->getHouseholdSize() > 0)
			{
				short int type = hhPums->getHouseholdType();
				short int incCat = hhPums->getHouseholdIncCat();

				if((type > 0 && incCat > 0) || (type < 0 && incCat < 0))
				{
					puma = std::to_string(hhPums->getPUMA());
					hhType = std::to_string(hhPums->getHouseholdType());
					hhSize = std::to_string(hhPums->getHouseholdSize());
					hhIncome = std::to_string(hhPums->getHouseholdIncCat());

					m_householdPUMS.insert(std::make_pair(hhPums->getHouseholdIndex(), *hhPums));
					
					m_pumsHHCount.insert(std::make_pair(puma+hhType+hhSize, true));
					m_pumsHHCount.insert(std::make_pair(puma+hhType+hhSize+hhIncome, true));
			
					++countHH;
					timer.stop();

					if(timer.elapsed_ms() > waitTime)
					{
						std::cout << countHH << " households are added to the list!" << std::endl;
						timer.start();
					}
				}
			}

			delete hhPums;
		}
	}

	std::cout << m_householdPUMS.size() << " households are added to the list!" << std::endl;  
	
	benchmark.stop();

	std::cout << "Time elapsed: " << benchmark.elapsed_ms()/1000 << " seconds!" << std::endl;
	std::cout << "Import Successful!\n" << std::endl;
}


/*
* @brief Imports Person level PUMS dataset for each state
* @param state US state
*/
template<class GenericParams>
void IPUWrapper<GenericParams>::importPersonPUMS(std::string state)
{
	std::string state_upper_case = state;
	std::transform(state_upper_case.begin(), state_upper_case.end(), state_upper_case.begin(), ::toupper);

	std::cout << "Importing Person PUMS file for : " << state_upper_case << " state..." << std::endl;

	double waitTime = 2000; //2 seconds wait time
	ElapsedTime timer, benchmark;

	benchmark.start();

	io::CSVReader<10>personPumsFile(parameters->getPersonPumsFile(state));
	personPumsFile.read_header(io::ignore_extra_column, "SERIALNO", "ADJINC", "AGEP", "SEX", "HISP", "RAC1P", "SCHL", "MAR", "PINCP", "PUMA10");

	std::string idx = "";
	std::string adj_inc = "";
	std::string age = "", ageCat = "", sex = "", hisp = "", race = "", origin = "";
	std::string edu = "", eduAge = "";
	std::string marital_status = "";
	std::string p_income = "";
	std::string puma = "";

	int countPersons = 0;
	while(personPumsFile.read_row(idx, adj_inc, age, sex, hisp, race, edu, marital_status, p_income, puma))
	{
		if(isValidPUMA(std::stoi(puma)))
		{
			double pumsIdx = std::stod(idx);
			if(m_householdPUMS.count(pumsIdx) > 0)
			{
				PersonPums<GenericParams> *pumsAgent = new PersonPums<GenericParams>(parameters);

				pumsAgent->setDemoCharacters(puma, idx, age, sex, hisp, race);
				pumsAgent->setSocialCharacters(edu, marital_status, p_income, adj_inc);

				puma = std::to_string(pumsAgent->getPumaCode());
				sex = std::to_string(pumsAgent->getSex());
				origin = std::to_string(pumsAgent->getOrigin());

				if(pumsAgent->getAge() >= 18)
				{
					eduAge = std::to_string(pumsAgent->getEduAgeCat());
					edu = std::to_string(pumsAgent->getEducation());
					m_pumsPerCount.insert(std::make_pair(puma+sex+eduAge+origin+edu, true));
				}
			
				m_householdPUMS.at(pumsIdx).addPersons(*pumsAgent);
				
				++countPersons;
				timer.stop();
				
				if(timer.elapsed_ms() > waitTime)
				{
					std::cout << countPersons << " persons are added to the list!" << std::endl;
					timer.start();
				}

				delete pumsAgent;
			}
		}
	}
	
	std::cout << countPersons << " persons are added to the list!" << std::endl;

	benchmark.stop();

	std::cout << "Time elapsed: " << benchmark.elapsed_ms()/1000 << " seconds!" << std::endl;
	std::cout << "Import Successful!\n" << std::endl;
}

/*
* @brief Computes household level ACS estimates with IPF for:
*        1. Household size by household type
*        2. Household income by household type and size
*/
template<class GenericParams>
void IPUWrapper<GenericParams>::computeHouseholdEst()
{
	//Step 1: extraction of group quarters ACS estimates
	std::map<int, Marginal> m_grpQtrs;

	Marginal grpQuarters = getEstimatesVector(ACS::Estimates::estGQ, "Group Quarters");

	m_grpQtrs.insert(std::make_pair(ACS::Estimates::estGQ, grpQuarters));
	addConstraints(m_grpQtrs);

	//Step 2: extraction of household type, size and income ACS estimates for IPF
	std::cout << "Starting IPF to compute household level estimates...\n" << std::endl;

	//Get estimates for household type
	Marginal famType = getEstimatesVector(ACS::Estimates::estHHType, "Household Type");

	//Get estimates for household size
	Marginal famSize = getEstimatesVector(ACS::Estimates::estHHSize, "Household Size");

	adjustHHSizeEstimates(famSize, grpQuarters.front());

	Marginal nonFamSize(famSize.begin() + ACS::HHSize::_size(), famSize.end());

	//Remove non-family households
	famSize.erase(famSize.begin() + ACS::HHSize::_size(), famSize.end());

	std::cout << "Running IPF for household size by household type...\n" << std::endl;

	std::map<int, Marginal> m_hhSizeByType;

	m_hhSizeByType = getIPFestimates(famType, famSize, famType.size(), famSize.size(), ACS::Estimates::estHHType, 0, 0);
	m_hhSizeByType.insert(std::make_pair(ACS::HHType::NonFamily, nonFamSize));

	Marginal hhIncome = getEstimatesVector(ACS::Estimates::estHHIncome, "Household Income");
	Marginal hhSizeByType;

	for(auto type = m_hhSizeByType.begin(); type != m_hhSizeByType.end(); ++type)
	{
		std::vector<double> estHHsize(type->second);
		for(size_t i = 0; i < estHHsize.size(); ++i)
			hhSizeByType.push_back(estHHsize.at(i));
	}

	std::cout << "Running IPF for household income by household type and size...\n" << std::endl;

	std::map<int, Marginal> m_hhIncbyTypebySize;
	m_hhIncbyTypebySize = getIPFestimates(hhSizeByType, hhIncome, hhSizeByType.size(), hhIncome.size(), ACS::Estimates::estHHIncome, 0, 0);
	addConstraints(m_hhIncbyTypebySize);

	std::cout << "IPF complete!\n" << std::endl;

	m_pumsHHCount.clear();
}

/*
* @brief Computes person level ACS estimates with IPF for:
*        1. Educational attainment by sex, age, and origin (race/eth) for 18 years and over
*/
template<class GenericParams>
void IPUWrapper<GenericParams>::computePersonEst()
{
	std::cout << "Starting IPF to compute person level estimates...\n" << std::endl;
	
	//Calculation of estimates for educational attainment by sex, age and origin for population 18 years and over
	Marginal origin = getEstimatesVector(ACS::Estimates::estRace, "Race");
	Marginal edu = getEstimatesVector(ACS::Estimates::estEducation, "Education");

	size_t num_origins = ACS::Origin::_size();
	size_t num_education = ACS::Education::_size();

	int estType = ACS::Estimates::estEducation;

	for(auto sex : ACS::Sex::_values())
	{
		for(auto eduAge : ACS::EduAgeCat::_values())
		{
			std::cout << "Running IPF for Sex: " << sex._to_string() << " and Age Category: " << eduAge._to_string() << "..\n" << std::endl;
			addConstraints(getIPFestimates(origin, edu, num_origins, num_education, estType, sex, eduAge));
		}
	}

	std::cout << "IPF completed!\n" << std::endl;

	m_pumsPerCount.clear();
}

/*
* @brief Removes invalid person types (which doesn't have a valid frequency(count < 0.01) 
*        from Household PUMS dataset.
*        
*/
template<class GenericParams>
void IPUWrapper<GenericParams>::refineHHPumsList()
{
	std::map<std::string, double> m_personCons(getConstraintsMap());
	std::vector<PersonPums<GenericParams>> hhPersons;

	std::cout << "PUMS households before refinement: " << m_householdPUMS.size() <<  std::endl;

	std::string sex, ageCat, origin, edu;
	std::string dummy = "0";

	bool valid_person;

	for(auto hh = m_householdPUMS.begin(); hh != m_householdPUMS.end();)
	{
		hhPersons = hh->second.getPersons();
		for(auto pp = hhPersons.begin(); pp != hhPersons.end(); ++pp)
		{
			sex = std::to_string(pp->getSex());
			origin = std::to_string(pp->getOrigin());
			if(pp->getAge() < 18)
			{
				ageCat = std::to_string(pp->getAgeCat());
				if(m_personCons[dummy+sex+ageCat+origin] > 0.01)
					valid_person = true;
				else
					valid_person = false;
			}
			else
			{
				ageCat = std::to_string(pp->getEduAgeCat());
				edu = std::to_string(pp->getEducation());

				if(m_personCons[sex+ageCat+origin+edu] > 0.01)
					valid_person = true;
				else
					valid_person = false;
			}

			if(!valid_person)
				break;
		}

		if(!valid_person)
			hh = m_householdPUMS.erase(hh);
		else
			++hh;
	}
	
	std::cout << "PUMS households after refinement: " << m_householdPUMS.size() << std::endl << std::endl;

}

/*
* @brief Returns a vector containing ACS estimates depending on the type of the estimate.
* @param type Type of the ACS estimate
* @param typeName Name of the ACS estimate type
*/
template<class GenericParams>
typename IPUWrapper<GenericParams>::Marginal IPUWrapper<GenericParams>::getEstimatesVector(int type, std::string typeName)
{
	if(!isValidGeoID(type, typeName))
		exit(EXIT_SUCCESS);

	std::map<int, std::vector<double>> m_estimates;
	std::map<std::string, int> all_origins;

	if(type == ACS::Estimates::estRace)
	{
		all_origins = parameters->getOriginMapping();
		for(auto org = all_origins.begin(); org != all_origins.end(); ++org)
		{
			std::vector<double> est;
			for(size_t i = 0; i < ACS::RaceMarginalVar::_size()-1; i++)
				est.push_back(0);

			m_estimates.insert(std::make_pair(org->second, est));
		}
	}
	
	if(m_acsEstimates->count(type) == 0){
		std::cout << "Error: Estimates for type-" << typeName << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
	
	auto acs_type = m_acsEstimates->equal_range(type);
	for(auto it = acs_type.first; it != acs_type.second; ++it) 
	{
		auto area_est = it->second.equal_range(geoID);
		for(auto row = area_est.first; row != area_est.second; ++row)
		{
			Columns cols = row->second;
			std::vector<double>values;
			if(type == ACS::Estimates::estRace)
			{
				std::string origin = cols.front();
				for(size_t i = 1; i < cols.size(); ++i)
					values.push_back(std::stod(cols.at(i)));

				int origin_idx = all_origins.at(origin);
				if(m_estimates.count(origin_idx) > 0)
				{
					auto itr = m_estimates.find(origin_idx);
					itr->second = values;
				}

			
				if(origin_idx == ACS::Origin::WhiteNH || origin_idx == ACS::Origin::BlackNH)
				{
					Marginal temp_est(m_estimates[origin_idx]);
					setPopulationSize(temp_est, origin_idx);
				}
			}
			else
			{
				if(type == ACS::Estimates::estHHSize)
					values.push_back(0);

				for(size_t i = 0; i < cols.size(); ++i)
					values.push_back(std::stod(cols.at(i)));

				m_estimates.insert(std::make_pair(type, values));
			}
		}
	}

	if(type == ACS::Estimates::estRace){
		Marginal raceMarginal;
		extractRaceEstimates(raceMarginal, m_estimates);
		return raceMarginal;
	}
	else if(type == ACS::Estimates::estHHIncome){
		extractHHIncEstimates(m_estimates.at(type));
		return m_estimates.at(type);
	}
	else{
		return m_estimates.at(type);
	}
}

/*
*
*/
template<class GenericParams>
void IPUWrapper<GenericParams>::extractRaceEstimates(Marginal &raceMarginals, const std::map<int, std::vector<double>> &m_raceEst)
{
	//Extract race/origin estimates by age and sex for age range 0-4, 5-9, 10-14 and 15-17
	std::vector<int> under18Idx;
	for(size_t i = ACS::RaceMarginalVar::POP_M_0_4; i <= ACS::RaceMarginalVar::POP_M_15_17; ++i)
		under18Idx.push_back(i-1);
	
	for(size_t j = ACS::RaceMarginalVar::POP_F_0_4; j <= ACS::RaceMarginalVar::POP_F_15_17; ++j)
		under18Idx.push_back(j-1);

	std::map<int, Marginal>m_under18;
	for(size_t k = 0; k < under18Idx.size(); ++k)
	{
		Marginal under18;
		for(auto org = m_raceEst.begin(); org != m_raceEst.end(); ++org)
			under18.push_back(org->second.at(under18Idx[k]));

		m_under18.insert(std::make_pair(k, under18));
	}
	
	addConstraints(m_under18);

	//Extract race/origin estimates by age and sex to match age range (18 and over) of education estimates
	std::multimap<int, int>m_ageGender(parameters->getVariableMap(ACS::Estimates::estEducation));
	
	if(m_ageGender.size() == 0){
		std::cout <<"Error: Invalid data type : " << ACS::Estimates::estEducation << " ! " << std::endl;
		exit(EXIT_SUCCESS);
	}

	for(auto sex : ACS::Sex::_values())
	{
		for(auto age : ACS::EduAgeCat::_values())
		{
			int key = 10*sex+age; 
			for(auto org = m_raceEst.begin(); org != m_raceEst.end(); ++org)
			{
				double sum = 0;
				for(auto idx = m_ageGender.lower_bound(key); idx != m_ageGender.upper_bound(key); ++idx)
					sum += (org->second.at(idx->second));
				
				raceMarginals.push_back(sum);
			}
		}
	}
}

template<class GenericParams>
void IPUWrapper<GenericParams>::extractHHIncEstimates(Marginal &hhIncMarginals)
{
	//std::unordered_multimap<int, int> m_hhIncome(parameters->getVariableMap(ACS::Estimates::estHHIncome));
	std::multimap<int, int> m_hhIncome(parameters->getVariableMap(ACS::Estimates::estHHIncome));
	if(m_hhIncome.size() == 0){
		std::cout <<"Error: Invalid data type : " << ACS::Estimates::estHHIncome << " ! " << std::endl;
		exit(EXIT_SUCCESS);
	}

	Marginal tempHHIncMar(hhIncMarginals);
	hhIncMarginals.clear();

	for(auto hhIncCat : ACS::HHIncome::_values())
	{
		double sum = 0; 
		auto inc_idx_range = m_hhIncome.equal_range(hhIncCat);
		for(auto idx = inc_idx_range.first; idx != inc_idx_range.second; ++idx)
			sum += tempHHIncMar.at(idx->second);

		hhIncMarginals.push_back(sum);
	}

}

template<class GenericParams>
void IPUWrapper<GenericParams>::adjustHHSizeEstimates(Marginal &hhSizeEst, double pop_gqs)
{
	std::vector<double>family(hhSizeEst.begin(), hhSizeEst.begin()+ACS::HHSize::_size());
	std::vector<double>non_family(hhSizeEst.begin()+ACS::HHSize::_size(), hhSizeEst.end());

	std::vector<double> pHHsize(ACS::HHSize::_size());

	double tot_hh = std::accumulate(hhSizeEst.begin(), hhSizeEst.end(), 0.0);
	double pop_tot_given, pop_tot_eq, hhld_diff, weight_sum_hh;

	pop_tot_given = 0.995*totalPop-pop_gqs;
	pop_tot_eq = 0;
	weight_sum_hh = 0;

	for(size_t i = 0; i < pHHsize.size(); ++i)
	{
		double sum_hh_size = family[i]+non_family[i];

		pHHsize[i] = sum_hh_size/tot_hh;

		weight_sum_hh += pHHsize[i]*(i+1);
		pop_tot_eq += (i+1)*sum_hh_size;

	}

	hhld_diff = (pop_tot_given-pop_tot_eq)/weight_sum_hh;
	double new_hh_tots = 0;
	
	for(size_t j = 0; j < ACS::HHSize::_size(); ++j)
		new_hh_tots += (family[j]+non_family[j]) + hhld_diff*pHHsize[j];

	for(size_t k = 0; k < hhSizeEst.size(); ++k)
		hhSizeEst[k] = (hhSizeEst[k]/tot_hh)*new_hh_tots;

}

//Note: Arguments definition in getIPFEstimates(.....)
//		1. row_mar = Row Marginals (Origin, Family Type etc.)
//		2. col_mar= Column Marginals (Education, Family Size, Household income etc.)
//		3. row_size = Size of row marginals
//		4. col_size = Size of column marginals
//		5. type = ACS::Estimates::...
//		6. row1var = first level row variables
//		7. col1var = first level column variables
template<class GenericParams>
typename IPUWrapper<GenericParams>::MarginalMap IPUWrapper<GenericParams>::getIPFestimates(Marginal &row_mar, Marginal &col_mar, size_t row_size, size_t col_size, int type, int row1var, int col1var)
{
	createSeedMatrix(row1var, col1var, row_size, col_size, type);

	setMarginals(row_mar, row_size);
	setMarginals(col_mar, col_size);
	
	adjustMarginals(marginals[0], marginals[1]);

	m_size.push_back(marginals[0].size());
	m_size.push_back(marginals[1].size());

	NDArray<double>seed1D(m_size);
	seed1D.assign(seed);
	deprecated::IPF ipf(seed1D, marginals);
	
	MarginalMap m_estimates(ipf.solve(seed1D));

	clear();

	return m_estimates;
}

/*
* @brief Adds constraints (estimates) to the list.
* @param m_marginal ACS estimates.  
*/
template<class GenericParams>
void IPUWrapper<GenericParams>::addConstraints(const std::map<int, Marginal> &m_marginal)
{
	for(auto row = m_marginal.begin(); row != m_marginal.end(); ++row)
	{
		for(size_t i = 0; i < row->second.size(); ++i)
		{
			if(row->second.at(i) != 0){
				ipuCons.push_back(row->second.at(i));
			}
			else
				ipuCons.push_back(0.01);
		}
	}		
}

/*
* @brief Returns person-level constraints (for age < 18 and age >= 18) by person type
*/
template<class GenericParams>
typename IPUWrapper<GenericParams>::ConsPersonMap IPUWrapper<GenericParams>::getConstraintsMap()
{
	ConsPersonMap m_cons;

	int tot_hh_type = ACS::HHType::_size()*ACS::HHSize::_size()*ACS::HHIncome::_size()+1;

	//Get person-level constraints
	Marginal personCons(ipuCons.begin()+tot_hh_type, ipuCons.end());

	int idx = 0;
	std::string dummy = "0";

	//Person-level constraints for age < 18
	for(auto sex : ACS::Sex::_values())
	{
		for(size_t ageCat = ACS::AgeCat::Age_0_4; ageCat <= ACS::AgeCat::Age_15_17; ++ageCat)
		{
			for(auto origin : ACS::Origin::_values())
			{
				m_cons.insert(std::make_pair(dummy+std::to_string(sex)+std::to_string(ageCat)+std::to_string(origin), personCons[idx]));
				idx++;
			}
		}
	}

	//Person level constraints for age > 18
	for(auto sex : ACS::Sex::_values())
	{
		for(auto eduAge : ACS::EduAgeCat::_values())
		{
			for(auto origin : ACS::Origin::_values())
			{
				for(auto edu : ACS::Education::_values())
				{
					m_cons.insert(std::make_pair(std::to_string(sex)+std::to_string(eduAge)+std::to_string(origin)+std::to_string(edu), personCons[idx]));
					idx++;
				}
			}
		}
	}

	return m_cons;
}

//Note: Arguments definition in createSeedMatrix(.....)
//		4. row1var = first level row variables
//		5. col1var = first level column variables
//		1. row_size = Size of row marginals
//		2. col_size = Size of column marginals
//		3. type = ACS::Estimates::...
template<class GenericParams>
void IPUWrapper<GenericParams>::createSeedMatrix(int row1var, int col1var, size_t row_size, size_t col_size, int type)
{
	for(size_t row2var = 1; row2var <= row_size; ++row2var)
	{
		for(size_t col2var = 1; col2var <= col_size; ++col2var)
		{
			double freq = getFrequency(row1var, col1var, row2var, col2var, type);
			if(freq == 0)
			{
				seed.push_back(0.001);
			}
			else
				seed.push_back(freq);
		}
	}
}

template<class GenericParams>
void IPUWrapper<GenericParams>::setMarginals(Marginal &mar, int size)
{
	int count = 0;
	std::vector<double> temp_marginal;
	for(auto it1 = mar.begin(); it1 != mar.begin()+size; ++it1)
	{
		temp_marginal.push_back(*it1);
		count++;
	}
	
	marginals.push_back(temp_marginal);
	mar.erase(mar.begin(), mar.begin()+count);
}

template<class GenericParams>
void IPUWrapper<GenericParams>::adjustMarginals(Marginal &mar1, Marginal &mar2)
{
	double pop_mar1 = std::accumulate(mar1.begin(), mar1.end(), 0.0);
	double pop_mar2 = std::accumulate(mar2.begin(), mar2.end(), 0.0);

	if(pop_mar1 == pop_mar2)
		return;

	if(pop_mar1 > pop_mar2)
	{
		double sum2 = 0;
		for(size_t i = 0; i < mar2.size(); ++i)
		{
			mar2[i] = mar2[i]*pop_mar1/pop_mar2;
			sum2 += mar2[i]; 
		}

	}
	else
	{
		double sum1 = 0;
		for(size_t j = 0; j < mar1.size(); ++j)
		{
			mar1[j] = mar1[j]*pop_mar2/pop_mar1;
			sum1 += mar1[j];
		}
	}
}

template<class GenericParams>
void IPUWrapper<GenericParams>:: clear()
{
	seed.clear();
	seed.shrink_to_fit();

	marginals.clear();
	marginals.shrink_to_fit();

	m_size.clear();
	m_size.shrink_to_fit();
}

template<class GenericParams>
void IPUWrapper<GenericParams>::clearHHPums()
{
	for(auto hh = m_householdPUMS.begin(); hh != m_householdPUMS.end(); ++hh)
		hh->second.clearPersonList();

	m_householdPUMS.clear();

	ipuCons.clear();
	ipuCons.shrink_to_fit();

	ipu->clearMap();
}

template<class GenericParams>
void IPUWrapper<GenericParams>::setPopulationSize(Marginal &est, int origin)
{
	if(parameters->getSimType() == EQUITY_EFFICIENCY)
	{
		if(origin == ACS::Origin::WhiteNH)
			totalPop = 0;

		double male_pop = 0;
		double female_pop = 0;

		for(size_t maleIdx = ACS::RaceMarginalVar::POP_M_45_49-1; maleIdx < ACS::RaceMarginalVar::POP_M_62_64; ++maleIdx)
			male_pop += est[maleIdx];

		for(size_t femaleIdx = ACS::RaceMarginalVar::POP_F_45_49-1; femaleIdx < ACS::RaceMarginalVar::POP_F_62_64; ++femaleIdx)
			female_pop += est[femaleIdx];

		totalPop += ((int)male_pop + (int)female_pop);
	}
}

template<class GenericParams>
bool IPUWrapper<GenericParams>::isValidGeoID(int type, std::string str)
{
	bool flag = true;
	
	auto acs_type = m_acsEstimates->equal_range(type);
	for(auto it = acs_type.first; it != acs_type.second; ++it)
	{
		if(it->second.count(geoID) == 0)
		{
			std::cout << "GEO ID: "<< geoID << " doesn't exist in the " << str << " estimates list! " << std::endl;
			std::cout << "Please re-check!" << std::endl;
		
			flag = false;
		}
	}

	return flag;
}

template<class GenericParams>
bool IPUWrapper<GenericParams>::isValidPUMA(int puma_code)
{
	bool isValid = false;
	if(parameters->isStateLevel())
	{
		if(puma_code > 0)
			isValid = true;
	}
	else
	{
		if(m_pumaCounty->count(puma_code) > 0)
			isValid = true;
	}

	return isValid;
}

template<class GenericParams>
double IPUWrapper<GenericParams>::getFrequency(int row1var, int col1var, int row2var, int col2var, int type)
{
	double frequency = 0;
	if(parameters->isStateLevel())
	{
		const int puma_code = 100;
		frequency = (double)getCount(row1var, col1var, row2var, col2var, puma_code, type); 
	}
	else
	{
		for(auto cnty = m_pumaCounty->begin(); cnty != m_pumaCounty->end(); ++cnty)
		{
			int pumaCode = cnty->first;
			int count = getCount(row1var, col1var, row2var, col2var, pumaCode, type); 
			frequency += (cnty->second.getPopulationWeight()*count);
		}
	}
	return frequency;
}

//Note: Arguments definition in method getCount(....)
//		1. row1var : first level row variables
//		2. col1var : first level column variables
//		3. row2var : second level row variables
//		4. col2var : second level column variables
template<class GenericParams>
int IPUWrapper<GenericParams>::getCount(int row1var, int col1var, int row2var, int col2var, int pumaCode, int type)
{
	int count = 0;
	std::string var_type = "";
	std::string puma = "", sex = "", eduAge = "", origin = "", edu = "";
	std::string hhType = "", hhSize = "", hhInc = "";

	switch(type)
	{
	case ACS::Estimates::estEducation:
		{
			puma = std::to_string(pumaCode);
			sex = std::to_string(row1var);
			eduAge = std::to_string(col1var);
			origin = std::to_string(row2var);
			edu = std::to_string(col2var);

			var_type = puma+sex+eduAge+origin+edu;
			count = m_pumsPerCount.count(var_type);
			break;
		}
	case ACS::Estimates::estHHType:
		{
			puma = std::to_string(pumaCode);
			hhType = std::to_string(row2var);
			hhSize = std::to_string(col2var);

			var_type = puma+hhType+hhSize;
			count = m_pumsHHCount.count(var_type);

			break;
		}
	case ACS::Estimates::estHHIncome:
		{
			for(auto val : ACS::HHSize::_values())
			{
				if((row2var-val)%ACS::HHSize::_size() == 0)
				{
					int hh_type_val = ((row2var-val)/ACS::HHSize::_size())+1;
					hhType = std::to_string(hh_type_val);
					hhSize = std::to_string(val);
					break;
				}
			}

			puma = std::to_string(pumaCode);
			hhInc = std::to_string(col2var);
			var_type = puma+hhType+hhSize+hhInc;

			count = m_pumsHHCount.count(var_type);
			
			break;
		}
	default:
		break;
	}

	return count;
}


