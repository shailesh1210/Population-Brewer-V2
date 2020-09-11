#include "Area.h"
#include "DepressionModel.h"
#include "PersonPums.h"
#include "HouseholdPums.h"
#include "DepressionParams.h"
#include "DepressionCounter.h"
#include "DepressionHousehold.h"
#include "Random.h"
#include "ElapsedTime.h"

DepressionModel::DepressionModel()
{
}

DepressionModel::DepressionModel(const char *inDir, const char *outDir, int simType, int geoLvl)
	:PopBrewer(new DepressionParams(inDir, outDir, simType, geoLvl))
{
	import();
}

DepressionModel::~DepressionModel()
{
	delete count;
	delete random;
}

void DepressionModel::start(int stateID)
{
	if(parameters != NULL)
	{
		count = new DepressionCounter(parameters);
		random = new Random;
	}
	else
	{
		std::cout << "Error: Parameters are not initialized!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	int num_trials = 1;
	Area<DepressionParams> *state = &m_geoAreas.at(std::to_string(stateID));
	std::string state_name = state->getAreaAbbreviation();
	
	for(size_t i = 0; i < num_trials; ++i)
	{
		std::cout << std::endl << "Simulation #" << i+1 << std::endl;

		createPopulation(state);
		setDepressionType(&m_agents, "Before Intervention");
		execute(state_name);

		clearList();
	}

	count->output(state_name);
}

void DepressionModel::addHousehold(const HouseholdPums<DepressionParams> *hh, int countHH)
{
	std::vector<PersonPums<DepressionParams>> tempPersons;
	int countPersons;
	
	if(hh->getHouseholdType() >= ACS::HHType::MaleHHFam)
	{
		DepressionHousehold *household = new DepressionHousehold(hh);
		tempPersons = hh->getPersons();

		countPersons = 0;
		for(auto pp = tempPersons.begin(); pp != tempPersons.end(); ++pp)
		{
			DepressionAgent *agent = new DepressionAgent(parameters, &(*pp), random, count, countHH, countPersons);
			household->addMemebers(agent);

			countPersons++;

			delete agent;
		}

		household->setIncomeToPovertyRatio();

		if(household->getHouseholdType() == ACS::HHType::NonFamily)
		{
			for(auto member = household->getMembers()->begin(); member != household->getMembers()->end(); ++member)
			{
				DepressionAgent *agent = new DepressionAgent;
				*agent = **member;

				if(agent->getAge() >= 18)
				{
					m_agents[agent->getAgentType1()].push_back(agent);
				}
			}
		}

		delete household;
		//households.push_back(household);
	}
}

void DepressionModel::addAgent(const PersonPums<DepressionParams> *p)
{
}

void DepressionModel::createPopulation(Area<DepressionParams> *area)
{
	std::cout << "Creating Population for " << area->getAreaName() << std::endl;

	area->createAgents(this);
	resize();
	computeWageGap();
	//households.shrink_to_fit();
}

void DepressionModel::computeWageGap()
{
	VecPairDblInt pWageGap;
	MapVecPair m_pWageGap;

	std::string sex_age_prefix = prefixBySexAge();
	std::string ip_ratio_prefix = prefixByIPratio();

	std::string sex_age_type, sex_age_ip_type;
	for(auto sex : ACS::Sex::_values())
	{
		for(auto ageCat : Depression::AgeCat::_values())
		{
			sex_age_type = std::to_string(sex) + std::to_string(ageCat);
			double cumulative = 0;
			for(auto ipRatio : ACS::IncomeToPovertyRatio::_values())
			{
				sex_age_ip_type = sex_age_type + std::to_string(ipRatio);
				double p_ipRatio = (double)getPopulation(ip_ratio_prefix+sex_age_ip_type)/getPopulation(sex_age_prefix+sex_age_type);
				if(sex == (int)ACS::Sex::Male)
				{
					cumulative += p_ipRatio;
					pWageGap.push_back(std::make_pair(cumulative, ipRatio));
				}

				std::cout << sex._to_string() << ", " << ageCat._to_string() << ", " << ipRatio._to_string() << ", " << p_ipRatio << std::endl;
			}

			if(sex == (int)ACS::Sex::Male)
			{
				m_pWageGap[std::to_string(ageCat)] = pWageGap;
				pWageGap.clear();
			}

			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	parameters->setWageGapProbabilityDist(m_pWageGap);

}

void DepressionModel::setDepressionType(AgentMap *agents, std::string timeFrame)
{
	std::string quartiles [] = {"1Q", "2Q", "3Q", "4Q"};

	for(auto map = agents->begin(); map != agents->end(); ++map)
	{
		std::string agent_type = map->first;
		std::cout << "\nAssigning depression attributes to person type " << agent_type << std::endl;

		const VecPairDblInt *depression_preval_pair = parameters->getDepressionPrevalence(agent_type);
		bool check_fit = false;
		int num_draws = 0;

		while(!check_fit)
		{
			resetPersonCount(quartiles, agent_type);

			std::cout << "\nAttempt: " << ++num_draws << " Pop: " << map->second.size() << std::endl;
			for(auto agent : map->second)
			{
				double randP = random->uniform_real_dist();
				for(auto prevalence = depression_preval_pair->begin(); prevalence != depression_preval_pair->end(); ++prevalence)
				{
					double depression_prevalence = prevalence->first;
					if(randP < depression_prevalence)
					{
						agent->setDepressionType(prevalence->second);
						break;
					}
				}
			}
			
			check_fit = checkFit(depression_preval_pair, quartiles, agent_type, map->second.size(), num_draws);
		}
	}

	count->computeOutcomes(timeFrame);
}

void DepressionModel::execute(std::string state_name)
{
	std::cout << "\nReducing Wage-Gender Gap...\n" << std::endl;

	int female = ACS::Sex::Female;

	resetPersonCount(female);

	std::string prefix_ip_ratio = prefixByIPratio();
	std::string female_type;

	AgentMap female_agents;
	for(auto ageCat : Depression::AgeCat::_values())
	{
		for(auto ipRatio : ACS::IncomeToPovertyRatio::_values())
		{
			female_type = prefix_ip_ratio + std::to_string(female) + std::to_string(ageCat) + std::to_string(ipRatio);
			std::cout << "Working on person type" << female_type << "...." << std::endl;

			if(m_agents.count(female_type) > 0)
			{
				for(auto agent : m_agents[female_type])
				{
					agent->update();
					female_agents[agent->getAgentType1()].push_back(agent);
				}
			}
		}
	}

	setDepressionType(&female_agents, "After Intervention");

	//updating list of female agents after wage-gap reduction
	for(auto map : m_agents)
	{
		std::string agent_type = map.first;
		if(female_agents.count(agent_type) == 0)
			continue;

		m_agents[agent_type] = female_agents[agent_type];
	}

	std::cout << std::endl;

	resize();
	//computeWageGap();
}

bool DepressionModel::checkFit(const VecPairDblInt *vec_prevalence, std::string quartiles[], std::string agent_type, int agent_pop, int num_draws)
{
	int idx = 0;
	double ref_preval = 0;

	//std::string quartiles [] = {"1Q", "2Q", "3Q", "4Q"};
	double quartile_dist [] = {0.25, 0.25, 0.25, 0.25};

	std::vector<std::pair<double, double>> v_diff_preval, v_diff_quartiles;
	std::vector<bool>v_chi_sq_tests;
	
	std::string prefix = prefixByDepression();
	std::string depression_agent_type;

	int pop = 0;
	for(auto depression_type : Depression::DepressionType::_values())
	{
		//int countDepression = count->getPersonCount(std::to_string(depression_type) + agent_type);
		depression_agent_type = prefix + std::to_string(depression_type) + agent_type;

		int countDepression = getPopulation(depression_agent_type);
		pop += countDepression;

		double computed_preval = 100*(double)countDepression/agent_pop;

		if(idx == 0)
		{
			ref_preval = 100*vec_prevalence->at(idx).first;
		}
		else
		{
			ref_preval = 100*(vec_prevalence->at(idx).first - vec_prevalence->at(idx-1).first);
		}

		double diff_preval = ref_preval - computed_preval;
		std::cout << std::setprecision(5) << depression_type._to_string() << " Depression: " << computed_preval 
			<< "," << ref_preval << ", Pop: " << countDepression <<  std::endl; 

		if(ref_preval != 0)
			v_diff_preval.push_back(std::make_pair(diff_preval, ref_preval));

		for(int i = 0; i < 4; i++)
		{
			int countSymptoms = getPopulation(depression_agent_type + quartiles[i]);

			std::cout << quartiles[i] << " = ";
			if(countDepression != 0)
			{
				double symptom_dist = 100*(double)countSymptoms/countDepression * Depression::fourth_q;
				double diff_quartiles = 100*quartile_dist[i] - symptom_dist;

				v_diff_quartiles.push_back(std::make_pair(diff_quartiles, 100*quartile_dist[i]));

				std::cout << std::setprecision(6) << symptom_dist << ", ";
			}
			else
			{
				std::cout << std::setprecision(6) << 0 << ", ";
			}
		}

		std::cout << std::endl;
		if(v_diff_quartiles.size() != 0)
			v_chi_sq_tests.push_back(chiSquareTest(v_diff_quartiles, num_draws, countDepression));

		idx++;
		v_diff_quartiles.clear();
	}

	std::cout << std::endl;

	if(agent_pop != pop)
	{
		std::cout << "Error: Agent population for agent type " << agent_type << " doesn't match!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	v_chi_sq_tests.push_back(chiSquareTest(v_diff_preval, num_draws, agent_pop));

	bool success = true;
	for(auto test : v_chi_sq_tests)
	{
		if(!test)
		{
			success = false;
			break;
		}
	}

	//resetPersonCount(quartiles, agent_type);
	return success;
}

bool DepressionModel::chiSquareTest(const std::vector<std::pair<double, double>> v_diff, int num_draws, int population)
{
	bool fit = false;
	//std::cout << std::endl;
	double sum_chi_sq = 0;
	for(auto diff : v_diff)
	{
		sum_chi_sq += ((diff.first * diff.first)/diff.second);
	}

	boost::math::chi_squared dist(v_diff.size()-1);
	double p_val = 1-(boost::math::cdf(dist, sum_chi_sq));

	if(population < 10)
	{
		fit = true;
	}
	else if(population >= 10 && population < 100)
	{
		if(p_val > (1 - 10*parameters->getAlpha()))
			fit = true;
	}
	else if(population >= 100 && population < 1000)
	{
		if(p_val > (1 - 4*parameters->getAlpha()))
			fit = true;
	}
	else
	{
		if(p_val > (1 - parameters->getAlpha()))
			fit = true;
	}

	std::cout << std::setprecision(6) << "P-value: " << p_val << " | df: " << v_diff.size() << " | chi-val: " << sum_chi_sq << std::endl << std::endl;

	return fit;
}

void DepressionModel::resetPersonCount(int sex)
{
	std::string prefix = prefixByIPratio();
	for(auto ageCat : Depression::AgeCat::_values())
	{
		for(auto ipRatio : ACS::IncomeToPovertyRatio::_values())
		{
			std::string person_type = prefix + std::to_string(sex) + std::to_string(ageCat) + std::to_string(ipRatio);
			count->resetPersonCount(person_type);
		}
	}
}

void DepressionModel::resetPersonCount(std::string quartiles[], std::string agent_type)
{
	for(auto depression_type : Depression::DepressionType::_values())
	{
		std::string agent_type2 = prefixByDepression() + std::to_string(depression_type) + agent_type;
		count->resetPersonCount(agent_type2);
			
		for(int i = 0; i < 4; i++)
		{
			count->resetPersonCount(agent_type2 + quartiles[i]);
		}
	}
}

std::string DepressionModel::prefixByDepression()
{
	return "Type";
}

std::string DepressionModel::prefixByIPratio()
{
	return "IP_Ratio";
}

std::string DepressionModel::prefixBySexAge()
{
	return "Sex_Age_Cat";
}

DepressionCounter * DepressionModel::getCounter() const
{
	return count;
}

int DepressionModel::getPopulation(std::string agent_type)
{
	return count->getPersonCount(agent_type);
}

void DepressionModel::setSize(int pop)
{

}

void DepressionModel::resize()
{
	for(auto map = m_agents.begin(); map != m_agents.end(); ++map)
		m_agents[map->first].shrink_to_fit();
}

void DepressionModel::clearList()
{
	for(auto map = m_agents.begin(); map != m_agents.end(); ++map)
	{
		m_agents[map->first].shrink_to_fit();

		for(auto agent : map->second)
			delete agent;

		m_agents[map->first].clear();
		m_agents[map->first].shrink_to_fit();
	}

	m_agents.clear();
}


