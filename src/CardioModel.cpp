/*
* This is a CHD model assess equity and efficiency among White and Black 
* non-hispanic population in United States. It creates a realistic synthetic
* population representative of US states. NHANES dataset is used to assign 
* risk factors to the population. Framingham Heart Study and SCORE equations
* has been used to predict 10-year total and fatal CHD. Smoking Tax intervention,
* Statins intervention, and early education intervention is used in this model to
* compare and contrast equity and efficiency of each intervention.
*/


#include "CardioModel.h"
#include "Area.h"
#include "PersonPums.h"
#include "HouseholdPums.h"
#include "ACS.h"
#include "Random.h"
#include "CardioCounter.h"
#include "ElapsedTime.h"
#include "CardioParams.h"

/*
* @brief Default class constructor
*/
CardioModel::CardioModel() 
{
	
}

/*
* @brief Overloaded class constructor. 
* @param inDir Input file directory path
* @param outDir Output file directory path
* @param simType Simulaton type
* @param geoLvl Geographic orientation (MSAs or US states)
*/
CardioModel::CardioModel(const char *inDir, const char *outDir, int simType, int geoLvl) 
	: PopBrewer(new CardioParams(inDir, outDir, simType, geoLvl))
{
	import();
}

/*
* @brief Class destructor
*/
CardioModel::~CardioModel()
{
	delete count;
	delete random;
}

void CardioModel::start()
{

}

/*
* @brief Starts the simulation (population generation and model execution).
*        Runs simulation for n trials and outputs the results of the simulation.
* @param stateID State's geo ID
*/
void CardioModel::start(int stateID)
{
	if(parameters != NULL)
	{
		count = new CardioCounter(parameters);
		random = new Random;
	}
	else{
		std::cout << "Error: Parameters are not initialized!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	int num_trials = parameters->getCardioParam()->num_trials;
	
	Area<CardioParams> *state = &m_geoAreas.at(std::to_string(stateID));

	std::string state_name = state->getAreaName();
	
	for(size_t i = 0; i < num_trials; ++i)
	{
		std::cout << std::endl << "Simulation #" << i+1 << std::endl;

		createPopulation(state);
		
		runModel();
		clearList();
	}

	count->output(state->getAreaAbbreviation());
}

/*
* @brief Invokes a method to:
*        1. Create a population for a given state.
*        2. Compute a percenatge to reduce education gap between white and black non-hispanic 
*           population
*        3. Set NHANES risk factors
*/
void CardioModel::createPopulation(Area<CardioParams> *area)
{
	std::cout << "Creating Population for " << area->getAreaName() << std::endl;
	area->createAgents(this);

	computeEducationDifference();
	setRiskFactors();
}

/*
* @brief Computes the percentage of black NH population attending High School 
*		 that needs to be shifted to higher education category (Some college or more)
*        to reduce education gap.
*/
void CardioModel::computeEducationDifference()
{
	std::string whiteMale, whiteFemale, blackMale, blackFemale, someCollege, highSchool;
	MapDbl perEduDiff;
	double diff = 0;

	whiteMale = concatenate(NHANES::Org::WhiteNH, NHANES::Sex::Male);
	whiteFemale = concatenate(NHANES::Org::WhiteNH, NHANES::Sex::Female);

	blackMale = concatenate(NHANES::Org::BlackNH, NHANES::Sex::Male);
	blackFemale = concatenate(NHANES::Org::BlackNH, NHANES::Sex::Female);

	someCollege = concatenate(NHANES::Edu::some_coll_);
	highSchool = concatenate(NHANES::Edu::HS_or_less);

	double perWhiteMaleCollege = (double)getPopulation("edu" + concatenate(whiteMale, someCollege))/getPopulation(whiteMale);
	double perWhiteFemaleCollege = (double)getPopulation("edu" + concatenate(whiteFemale, someCollege))/getPopulation(whiteFemale);

	double perBlackMaleCollege = (double)getPopulation("edu" + concatenate(blackMale, someCollege))/getPopulation(blackMale);
	double perBlackFemaleCollege = (double)getPopulation("edu" + concatenate(blackFemale, someCollege))/getPopulation(blackFemale);

	double perBlackMaleHS = (double)getPopulation("edu" + concatenate(blackMale, highSchool))/getPopulation(blackMale);
	double perBlackFemaleHS = (double)getPopulation("edu" + concatenate(blackFemale, highSchool))/getPopulation(blackFemale);

	//Compute percentage of Black NH male with High School that needs to be shifted to reduce education gap
	diff = (perWhiteMaleCollege - perBlackMaleCollege);
	perEduDiff[blackMale] = (diff > 0) ? diff/perBlackMaleHS : 0; 

	//Compute percentage of Black NH female with High School that needs to be shifted to reduce education gap
	diff = (perWhiteFemaleCollege - perBlackFemaleCollege);
	perEduDiff[blackFemale] = (diff > 0) ? diff/perBlackFemaleHS : 0;

	parameters->setPercentEduDifference(perEduDiff);
}

/*
* @brief Placeholder method for adding household
* @param h PUMS household from PUMS file
* @param countHH Household counter
*/
void CardioModel::addHousehold(const HouseholdPums<CardioParams> *h, int countHH)
{
	// Do nothing 
}

/*
* @brief Adds agents with age 45-64 to the list of agents.
*        Adds agents to the map based on race, gender, age cat and edu cat
* @param p PUMS person from Person-level PUMS file for a given state
*/
void CardioModel::addAgent(const PersonPums<CardioParams> *p)
{
	//Including ARIC study cohort age range (45-54)
	std::string agent_type;

	if(p->getAge() >= 45 && p->getAge() < 65)
	{
		if(p->getOrigin() == ACS::Origin::WhiteNH || p->getOrigin() == ACS::Origin::BlackNH)
		{
			CardioAgent *agent = new CardioAgent(p, parameters, count, random);

			//agent_type = agent->getAgentType1();
			agent_type = agent->getAgentType5();

			agentList.push_back(agent);
			agentsPtrMap.insert(std::make_pair(agent_type, agent));
		}
	}
}

/*
* @brief Assigns NHANES risk factors to agents based on their proportions.
*        Each NHANES risk factor has age, cholesterol levels (Total, HDL, LDL, Triglycerides),
*        Systolic blood pressure, smoking status, statin medication status, hypertension medication status.
*        Risk factors are stratified by agent type (by race, gender, age cat and edu cat) and risk strata.
*        Risk strata are identified based on the presence or absence of individual risk factors.
*/
void CardioModel::setRiskFactors()
{
	std::cout << "Assigning NHANES Risk Factors....\n" << std::endl;

	//Clear risk factor every simulation trial
	count->clearRiskFactor();

	//NHANES risk factors by agent type
	AgentStrataRiskMap m_riskFactors(*parameters->getRiskFactorMap());

	//Tuple consisting of weight, strata and risk factor
	WeightStrataRiskTuple risk_tuple;
	std::vector<WeightStrataRiskTuple> v_riskFactors;
	
	double agents_per_strata = 0;
	short int risk_strata = 0;

	EET::RiskFactors risk_factors;

	for(auto map = m_riskFactors.begin(); map != m_riskFactors.end(); ++map)
	{
		std::string agent_type = map->first;

		double agent_pop = agentsPtrMap.count(agent_type);
		if(agent_pop == 0)
			continue;

		double adj = 0;
		
		std::cout << "Risk factor assignment for person type: " << agent_type << std::endl;

		//Compute weight (number of agents with a given risk strata and risk factor)
		for(auto risk = map->second.begin(); risk != map->second.end(); ++risk)
		{
			int risk_strata = risk->first;
			for(auto risk_vec = risk->second.begin(); risk_vec != risk->second.end(); ++risk_vec)
			{
				double weight = risk_vec->first * agent_pop;
				EET::RiskFactors risks = risk_vec->second;

				if( weight > 0)
					v_riskFactors.push_back(std::make_tuple(weight, risk_strata, risks));
			}

		}

		//Round-off the weights
		double totWeights = rounding(v_riskFactors, adj);

		if(totWeights != agent_pop)
		{
			std::cout << "Error: Total weights and agent population doesn't match!" << std::endl;
			exit(EXIT_SUCCESS);
		}

		boost::range::random_shuffle(v_riskFactors);

		auto agents = agentsPtrMap.equal_range(agent_type);
		int idx = 0;

		risk_tuple = v_riskFactors.at(idx);

		agents_per_strata = std::get<0>(risk_tuple);
		risk_strata = std::get<1>(risk_tuple);
		risk_factors = std::get<2>(risk_tuple);

		for(auto agent = agents.first; agent != agents.second; ++agent)
		{
			if(agents_per_strata == 0)
			{
				while(true)
				{
					idx++;
					if(idx < v_riskFactors.size())
					{
						risk_tuple = v_riskFactors.at(idx);

						agents_per_strata = std::get<0>(risk_tuple);
						if(agents_per_strata > 0)
						{
							risk_strata = std::get<1>(risk_tuple);
							risk_factors = std::get<2>(risk_tuple);

							break;
						}
					}
					else if(idx >= v_riskFactors.size())
					{
						break;
					}
				}
					
			}
			
			if(agents_per_strata > 0)
			{
				agent->second->setRiskFactors(risk_strata, risk_factors);
				agents_per_strata--;
			}
		}

		v_riskFactors.clear();
	}

	//Computes mean of risk factors before the intervention
	count->computeMeanRisk(parameters->beforeIntervention());

	std::cout << "\nAssignment Complete!\n " << std::endl;
}

/*
* @brief Executes different intervention scenarios;:
*        1. Early education intervention
*        2. Smoking Tax intervention
*        3. Statins Intervention
*        4. Smoking Tax + Statins Intervention
*/
void CardioModel::runModel()
{
	bool isEducationPresent = false;
	for(auto eduInterventionId : EET::EduIntervention::_values())
	{
		if(eduInterventionId == (int)EET::EduIntervention::Education)
		{
			isEducationPresent = true;
			educationIntervention(eduInterventionId, isEducationPresent);
		}

		for(auto interventionId : EET::Interventions::_values())
		{
			switch(interventionId)
			{
			case EET::Interventions::None:
				if(isEducationPresent)
					break;
				noIntervention(interventionId, isEducationPresent);
				break;
			case EET::Interventions::Tax:
				taxIntervention(interventionId, isEducationPresent);
				break;
			case EET::Interventions::Statins:
				statinsIntervention(interventionId, isEducationPresent);
				break;
			case EET::Interventions::Tax_Statins:
				taxStatinsIntervention(interventionId, isEducationPresent);
				break;
			default:
				break;
			}
		}
	}
}

//void CardioModel::runModel()
//{
//	std::string s_eduIntervention = "_";
//	std::string s_otherIntervention = "";
//
//	bool isEducationPresent = false;
//
//	for(auto eduInterventionID : EET::EduIntervention::_values())
//	{
//		if(eduInterventionID == (int)EET::EduIntervention::Education)
//		{
//			isEducationPresent = true;
//			s_eduIntervention += std::string(eduInterventionID._to_string());
//			runSubModels(s_eduIntervention, eduInterventionID, 1,  isEducationPresent);
//		}
//		
//		for(auto otherInterventionID : EET::OtherIntervention::_values())
//		{
//			if(isEducationPresent && otherInterventionID == (int)EET::OtherIntervention::No_Intervention)
//				continue;
//
//			s_otherIntervention = s_eduIntervention + std::string(otherInterventionID._to_string());
//			runSubModels(s_otherIntervention, otherInterventionID, 2,  isEducationPresent);
//		}
//	}
//}

//void CardioModel::runModel()
//{
//	std::string sep = "_";
//	std::string s_otherIntervention = "";
//
//	bool isEducationPresent = false;
//
//	for(auto otherInterventionID : EET::OtherIntervention::_values())
//	{
//		s_otherIntervention = sep + std::string(otherInterventionID._to_string());
//		runSubModels(s_otherIntervention, otherInterventionID, 2,  isEducationPresent);
//	}
//	
//}

/*
* @brief Executes the sub-models for education intervention
* @param id Intervention id
* @param isEducationPresent True if education intervention is present
*/
void CardioModel::educationIntervention(int id, bool isEducationPresent)
{
	std::string eduInterventionName = getEducationInterventionName(id);
	std::cout << "\nStarting " << eduInterventionName << " Intervention\n" << std::endl;

	runSubModels(eduInterventionName, id, 0, 0, 1, isEducationPresent);
}

/*
* @brief Executes the sub-models for no intervention scenario
* @param id Id for no intervention scenario
* @param isEducationPresent True if education intervention is present
*/
void CardioModel::noIntervention(int id, bool isEducationPresent)
{
	if(!isEducationPresent) {
		std::string interventionName = getInterventionName(id, isEducationPresent, "No Intervention");
		std::cout << "\nStarting " << interventionName << " Intervention\n" << std::endl;

		runSubModels(interventionName, id, 0, 0, 2, isEducationPresent);
	}
}

/*
* @brief Executes the sub-models of smoking tax intervention
* @param id Intervention index for smoking tax intervention
* @param isEducationPresent True if education intervention is present
*/
void CardioModel::taxIntervention(int id, bool isEducationPresent)
{
	if(!isEducationPresent) {

		for(auto taxType : EET::TaxType::_values())
		{
			std::string interventionName = getInterventionName(id, isEducationPresent, taxType._to_string());
			std::cout << "\nStarting " << interventionName << " Intervention\n" << std::endl;

			runSubModels(interventionName, id, taxType, 0, 2, isEducationPresent);

		}

		count->computeSmokeChange();
	}
}

/*
* @brief Executes the sub-models of statins intervention
* @param id Intervention index for statins intervention
* @param isEducationPresent True if education intervention is present
*/
void CardioModel::statinsIntervention(int id, bool isEducationPresent)
{
	if(!isEducationPresent) {
		for(auto statinsType : EET::StatinsType::_values())
		{
			std::string interventionName = getInterventionName(id, isEducationPresent, statinsType._to_string());
			std::cout << "\nStarting " << interventionName << " Intervention\n" << std::endl;

			runSubModels(interventionName, id, 0, statinsType, 2, isEducationPresent);
		}
	}
}

/*
* @brief Executes the sub-models of smoking tax + statins intervention
* @param id Intervention index for smoking tax + statins intervention
* @param isEducationPresent True if education intervention is present
*/
void CardioModel::taxStatinsIntervention(int id, bool isEducationPresent)
{
	for(auto statinsType : EET::StatinsType::_values())
	{
		std::string s_statinsType = statinsType._to_string();
		for(auto taxType : EET::TaxType::_values())
		{
			std::string s_taxType = taxType._to_string();

			if(statinsType == (int)EET::StatinsType::Strong && taxType == (int)EET::TaxType::Two_Dollar)
			{
				std::string interventionName = getInterventionName(id, isEducationPresent, s_statinsType + "+" + s_taxType);
				std::cout << "\nStarting " << interventionName << " Intervention\n" << std::endl;

				runSubModels(interventionName, id, taxType, statinsType, 2, isEducationPresent);
			}
		}
	}
}

/*
* @brief Sub-models of CHD model
*        1. Compute 10 Year risk of CHD (Total and Fatal) before intervention
*        2. Execute intervention for 2 years
*        3. Compute 10 Year risk of CHD after the intervention
*        4. Process the CHD events based on predicted 10-year risk
*        5. Reset the risk factor attributes
* @param interventionName Name of the intervention
* @param interventionId Intervention index
* @param taxType Type of the tax (2, 3, 4, or 5 dollar tax)
* @param statinsType Type of statins (Weak, Strong, Stronger, Strongest)
* @param interventionYears Duration of intervention
* @param isEducationPresent True if education intervention is present
*/
void CardioModel::runSubModels(std::string interventionName, int interventionId, int taxType, int statinsType, int interventionYears, bool isEducationPresent)
{
	computeTenYearCHDRisk(parameters->beforeIntervention());
	for(int currYear = 0; currYear < interventionYears; ++currYear)
	{
		std::cout << "Year " << currYear + 1 << std::endl;
		executeIntervention(interventionId, taxType, statinsType, currYear, isEducationPresent);
	}

	computeTenYearCHDRisk(parameters->afterIntervention());
	processChdEvents(interventionId, taxType, statinsType, isEducationPresent);
	resetAttributes();

	count->sumOutcomes(interventionName);

}

/*
* @brief Executes a method to compute 10-year risk of CHD for each agent
* @param timeFrame Before or after intervention time period
*/
void CardioModel::computeTenYearCHDRisk(std::string timeFrame)
{
	double waitTime = 4000; //4 seconds wait time
	ElapsedTime timer;

	std::cout << "Computing 10 year CHD risk " << timeFrame << "....\n" << std::endl;
	count->clearCHDRisks(timeFrame);

	int countAgent = 0;
	for(auto agent : agentList)
	{
		countAgent++;

		agent->computeTenYearCHDRisk(timeFrame);
		timer.stop();
		if(timer.elapsed_ms() > waitTime)
		{
			std::cout << "CHD Risk Score computed for " << countAgent << " agents!" << std::endl;
			timer.start();
		}
	}

	std::cout << "Complete!\n" << std::endl;
}

/*
* @brief Executes a method to update risk factors of each agent in response to the intervention
* @param interventionId Index of intervention
* @param taxType Two, Three, Four or Five Dollar Smoking tax intervention
* @param statinsType Type of statins intervention (Weak, Strong, Stronger or Strongest)
* @param currentYear Intervention year (year 1 and year 2)
* @param isEducationPresent True if education intervention is present
*/
void CardioModel::executeIntervention(int interventionId, int taxType, int statinsType, int currentYear, bool isEducationPresent)
{
	//std::cout << "Starting " << intervention_name << "_Intervention\n" << std::endl;
	std::cout << "Running intervention....\n" << std::endl;
	count->clearRiskFactor();

	double waitTime = 4000; //4 seconds wait time
	ElapsedTime timer;
	
	int countAgent = 0;

	if(interventionId == EET::EduIntervention::Education)
		resetPersonCounter();

	for(auto agent : agentList)
	{
		agent->update(interventionId, taxType, statinsType, currentYear, isEducationPresent);
		
		countAgent++;
		timer.stop();
		if(timer.elapsed_ms() > waitTime)
		{
			std::cout << "Risk factors updated for " << countAgent << " agents!" << std::endl;
			timer.start();
		}
	}

	std::cout << "Complete!\n" << std::endl;
	
	if(interventionId == EET::EduIntervention::Education)
		count->computeMeanRisk(parameters->afterEducation());

	count->computeMeanRisk(parameters->afterIntervention());
}

/*
* @brief Executes a method to process CHD event (CHD related death) over 10 year period based on 10-year 
*        risk of Fatal CHD.
* @param interventionId Index of intervention
* @param taxType Two, Three, Four or Five Dollar Smoking tax intervention
* @param statinsType Type of statins intervention (Weak, Strong, Stronger or Strongest)
* @param isEducationPresent True if education intervention is present
*/
void CardioModel::processChdEvents(int interventionId, int taxType, int statinsType, bool isEducationPresent)
{
	if(!isEducationPresent)
	{
		std::string interventionType = parameters->getInterventionType(interventionId, statinsType, taxType, isEducationPresent);

		count->clearCHDdeaths();

		double deathsPerYear;
		double totalDeaths = 0;

		int num_years = parameters->getCardioParam()->num_years;

		std::cout << std::endl;
		for(int year = 1; year <= num_years; ++year)
		{
			int alive = 0;

			std::cout << "Processing Fatal CHD events: Year " << year << std::endl;
			for(auto agent : agentList)
			{
				if(agent->isDead())
					continue;
				
				agent->processChdEvent(interventionId, statinsType, taxType, year, isEducationPresent);

				if(!agent->isDead())
					alive++;
			}

			deathsPerYear = count->getChdDeaths(interventionType, year);
			totalDeaths += deathsPerYear;

			std::cout << "CHD deaths=" << deathsPerYear << "," << "Pop=" << alive << std::endl << std::endl;
		}

		std::cout << "Total CHD deaths=" << totalDeaths << std::endl;
	}
}

/*
* @brief Executes a method to reset agent's attributes to initial state (original attributes before intervention)
*/
void CardioModel::resetAttributes()
{
	std::cout << "\nResetting agent attributes.." << std::endl;
	
	for(auto agent : agentList)
	{
		agent->resetAttributes();
	}

	std::cout << "Resetting Complete!" << std::endl;
}

/*
* @brief Executes a method to reset a person type (person type by race, gender and education)
*/
void CardioModel::resetPersonCounter()
{
	std::string agent_type;
	for(auto org : NHANES::Org::_values())
	{
		for(auto sex : NHANES::Sex::_values())
		{
			for(auto edu : NHANES::Edu::_values())
			{
				agent_type = "edu" + std::to_string(org) + std::to_string(sex) + std::to_string(edu);
				count->resetPersonCount(agent_type);
			}
		}
	}
}

/*
* @brief Performs the rounding of the population count to the nearset lowest or highest number
*        depending on the difference between the count, its floor value, and adjustment 
*        factor.
* @param popCount Vector of population count
* @param adj Adjustment factor
*/
void CardioModel::rounding(std::vector<PairDD> &popCount, double & adj)
{
	//double adj = 0;
	for(auto it = popCount.begin(); it != popCount.end(); ++it)
	{
		double diff = adj+(it->first-floor(it->first));
		 if(diff >= 0.5){
			 adj = diff-1;
			 it->first = ceil(it->first);
		 }
		 else{
			 adj = diff;
			 it->first = floor(it->first);
		 }
	}
}

/*
* @brief Performs the rounding of the risk factor weights to the nearset lowest or highest number
*        depending on the difference between the weight, its floor value, and adjustment 
*        factor.
* @param v_tuple Vector of risk factor tuples (weight, risk strata, risk factor)
* @param adj Adjustment factor
* @return Returns the sum of all the weights
*/
double CardioModel::rounding(std::vector<WeightStrataRiskTuple> &v_tuple, double & adj)
{
	//double adj = 0;
	double totWeight = 0;
	for(size_t i = 0; i < v_tuple.size(); ++i)
	{
		double weight = std::get<0>(v_tuple[i]);
		double diff = adj+(weight-floor(weight));
		 if(diff >= 0.5){
			 adj = diff-1;
			 weight = ceil(weight);
		 }
		 else{
			 adj = diff;
			 weight = floor(weight);
		 }

		 totWeight += weight;
		 std::get<0>(v_tuple[i]) = weight;
	}

	return totWeight;
}

/*
* @brief Concatenates two integers into a string and returns it.
* @param value1 Integer 1
* @param value2 Integer 2
* @return Returns the concatenated string of value1 and value2
*/
std::string CardioModel::concatenate(int value1, int value2)
{
	return std::to_string(value1) + std::to_string(value2);
}

/*
* @brief Returns string type of a given integer
*/
std::string CardioModel::concatenate(int value1)
{
	return std::to_string(value1);
}

/*
* @brief Concatenates two strings and returns the result
*/
std::string CardioModel::concatenate(std::string value1, std::string value2)
{
	return value1 + value2;
}

/*
* @brief Returns the population for a given agentType
*/
int CardioModel::getPopulation(std::string agentType)
{
	return count->getPersonCount(agentType);
}

/*
* @brief Returns the name of the given intervention based its id, type and presence of education intervention
*/
std::string CardioModel::getInterventionName(int id, bool isEducationPresent, std::string type)
{
	std::string intervention = EET::Interventions::_from_integral(id)._to_string();
	std::string sep = "+";

	if(isEducationPresent)
	{
		std::string education = getEducationInterventionName(EET::EduIntervention::Education);
		return education + sep + intervention + sep + type;
	}
	else
	{
		return intervention + sep + type;
	}
		
}

/*
* @brief Returns the name of education intervention based on its index
*/
std::string CardioModel::getEducationInterventionName(int id)
{
	std::string eduInterventionType = EET::EduIntervention::_from_integral(id)._to_string();
	return eduInterventionType;
}

/*
* @brief Returns a pointer to the CardioCounter object
*/
CardioCounter * CardioModel::getCounter() const
{
	return count;
}

/*
* @brief Reserves a memory for storing agents
*/
void CardioModel::setSize(int pop)
{
	agentList.reserve(pop);
}

/*
* @brief Clears agent list and agent map
*/
void CardioModel::clearList()
{
	for(auto agent : agentList)
		delete agent;

	agentList.clear();
	agentList.shrink_to_fit();

	agentsPtrMap.clear();
}