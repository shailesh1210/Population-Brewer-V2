#include "CardioCounter.h"
#include "CardioAgent.h"
#include "CardioParams.h"


CardioCounter::CardioCounter()
{
}

CardioCounter::CardioCounter(std::shared_ptr<CardioParams> p) : param(p)
{
}

CardioCounter::~CardioCounter()
{
}

void CardioCounter::initialize()
{
	initHouseholdCounter(param);
	initPersonCounter(param);
	initRiskFacCounter();
}

void CardioCounter::clearRiskFactor()
{
	clearRiskFactorCounter();
}

void CardioCounter::clearCHDRisks(std::string timeFrame)
{
	clearCHDRiskCounter(timeFrame);
}

void CardioCounter::clearCHDdeaths()
{
	m_totChdDeathsPerYear.clear();
}

void CardioCounter::output(std::string geoID)
{
	outputMeanRiskFactor(geoID);
	outputTenYearTotalCHDRisk(geoID);
	outputTenYearFatalCHDRisk(geoID);
	outputStatinsUsage(geoID);
	outputCostEffectiveness(geoID);
	outputPercentSmokeChange(geoID);
	
}

void CardioCounter::initRiskFacCounter()
{
	clearMap(m_riskFacCount);
	clearRiskFactorCounter();

	//m_incomeByRace.clear(); //temp
	
	std::vector<bool> riskFacCounts;
	riskFacCounts.reserve(0);

	const Pool *nhanesPool = param->getNhanesPool();
	for(size_t nh = 0; nh < nhanesPool->size(); ++nh)
		for(size_t rf = 1; rf <= NUM_RISK_STRATA; ++rf)
			m_riskFacCount.insert(std::make_pair(std::to_string(rf)+nhanesPool->at(nh), riskFacCounts));	
}

void CardioCounter::addRiskFactorCount(std::string rfType)
{
	if(m_riskFacCount.count(rfType) > 0)
		m_riskFacCount[rfType].push_back(true);
	
}

void CardioCounter::addRiskFactorDifference(std::string timeFrame, std::string agentType, int riskType, double diff)
{
	m_sumDiffRiskFac[timeFrame][agentType][riskType] += (diff * diff);
}

void CardioCounter::addTenYearTotalCHDRisk(std::string timeFrame, std::string agentType1, std::string agentType2, double p)
{
	m_totalChdRiskScore[timeFrame][agentType1][agentType2].push_back(p);
}

void CardioCounter::addTenYearFatalCHDRisk(std::string timeFrame, std::string agentType1, std::string agentType2, double p)
{
	m_fatalChdRiskScore[timeFrame][agentType1][agentType2].push_back(p);
}

void CardioCounter::addIncome(std::string race, double income)
{
	m_incomeByRace[race].push_back(income);
}

void CardioCounter::addRiskFactors(CardioAgent *agent, std::string agentType1, std::string agentType2)
{
	accumulateRiskFacs(agent, agentType1, agentType2);
}

void CardioCounter::addStatinsUsage(CardioAgent *agent, std::string agentType, std::string timeFrame, int intervention, int statinsType, int taxType, bool isEduPresent)
{
	if((intervention == (int)EET::Interventions::Statins || intervention == (int)EET::Interventions::Tax_Statins) || intervention == (int)EET::EduIntervention::Education)
	{
		std::string s_intervention = param->getInterventionType(intervention, statinsType, taxType, isEduPresent);
		if(timeFrame == param->beforeIntervention())
		{
			if(agent->isStatinQualified()) {
				std::string onStatin = (agent->isOnStatin()) ? "Yes" : "No";
				m_statinEligibles[s_intervention][agentType][onStatin] += 1;
			}
		}
		
		if(agent->isOnStatin())
			m_statinUsage[s_intervention][agentType][timeFrame] += 1;
	}
}

/*
* @brief Counter for CHD deaths for a given intervention by year and agent type
* @param intervention Intervention type
* @param year Current intervention year
* @param agentType agent type by race gender
*/
void CardioCounter::addChdDeaths(std::string intervention, std::string year, std::string agentType)
{
	m_totChdDeathsPerYear[intervention][year] += 1;
	m_totChdDeathsByRaceGender[intervention][agentType] += 1;
}

/*
* @brief Counter for Years of Life Lost (YLL) for a given intervention by year and agent type
* @param intervention Intervention type
* @param year Current intervention year
* @param agentType agent type by race gender
* @param yll Years of Life Lost
*/
void CardioCounter::addYearsLifeLost(std::string intervention, std::string year, std::string agentType, double yll)
{
	m_totYLL[intervention][agentType][year] += yll;
}

/*
* @brief Counter for change in smoking status from - Smoker to former smoker, Smoker to non smoker
* @param intervention Intervention type
* @param agentType agent type by race gender
* @param smokeChange Pair of Integers containing former and current smoking status
*/
void CardioCounter::addSmokingChange(std::string intervention, std::string agentType, PairInts smokeChange)
{
	proportionSmokeChange[intervention][agentType][smokeChange] += 1;
}

/*
* @brief Counter for smokers by intervention and agent type
* @param intervention Intervention type
* @param agentType agent type by race gender
*/
void CardioCounter::addSmokers(std::string intervention, std::string agentType) 
{
	countSmokers[intervention][agentType] += 1;
}

/*
* @brief Compute percentage of smoke status change (from smokers to former
*        and from smokers to non smokers)
*/
void CardioCounter::computeSmokeChange() 
{
	for (auto map1 = proportionSmokeChange.begin(); map1 != proportionSmokeChange.end(); ++map1)
	{
		std::string intervention = map1->first;
		for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
		{
			std::string agentType = map2->first;
			double smokerCount = countSmokers[intervention][agentType];

			for(auto map3 = map2->second.begin(); map3 != map2->second.end(); ++map3)
			{
				PairDouble smokeStatus = map3->first;

				double pSmokeChange = map3->second/smokerCount;
				map3->second = pSmokeChange;

				avgPropSmokeChange[intervention][agentType][smokeStatus] += pSmokeChange;
			}
		}
	}
}

/*
* @brief Calls a method to compute mean of risk factors
* @param timeFrame Before or after intervention period
*/
void CardioCounter::computeMeanRisk(std::string timeFrame)
{
	m_meanRiskFac[timeFrame].clear();
	m_meanRiskFac[timeFrame] = computeMean();
}

void CardioCounter::sumOutcomes(std::string intervention)
{
	sumMeanRiskFactors(intervention);
	sumStdDevRiskFactors(intervention);
	sumTenYearTotalCHDRisk(intervention);
	sumTenYearFatalCHDRisk(intervention);
}


void CardioCounter::accumulateRiskFacs(CardioAgent *agent, std::string agentType1, std::string agentType2)
{
	double risk_factor = -1;
	for(auto risk : NHANES::RiskFac::_values())
	{
		risk_factor = agent->getRiskFactor(risk);

		m_sumRiskFac[agentType1][risk] += risk_factor;
		m_sumRiskFac[agentType2][risk] += risk_factor;

	}
}

CardioCounter::RiskFacMap CardioCounter::computeMean()
{
	RiskFacMap meanRisk;
	for(auto org : NHANES::Org::_values())
	{
		for(auto sex : NHANES::Sex::_values())
		{
			std::string agent_type1 = std::to_string(org) + std::to_string(sex);
			computeMean(meanRisk, agent_type1);

			for(auto edu : NHANES::Edu::_values())
			{
				std::string agent_type2 =  "edu" + agent_type1 + std::to_string(edu);
				computeMean(meanRisk, agent_type2);
			}
		}

	}

	return meanRisk;
}

void CardioCounter::computeMean(RiskFacMap &mean_risks, std::string agent_type)
{
	int agent_pop = m_personCount[agent_type].size();
	if(m_personCount.count(agent_type) > 0 && m_sumRiskFac.count(agent_type) > 0)
	{
		for(auto risk : NHANES::RiskFac::_values())
		{
			mean_risks[agent_type][risk] = m_sumRiskFac[agent_type][risk]/agent_pop;
		}
	}
	else
	{
		std::cout << "Error: Agent Type " << agent_type << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

void CardioCounter::sumMeanRiskFactors(std::string intervention)
{
	std::string before = param->beforeIntervention();
	std::string after = param->afterIntervention();
	std::string agent_type = "";

	std::map<std::string, bool> m_raceGender;
	for(auto org : NHANES::Org::_values())
	{
		for(auto sex : NHANES::Sex::_values())
		{
			agent_type = std::to_string(org) + std::to_string(sex);
			m_raceGender[agent_type] = true;
		}
	}

	RiskFacMap meanRisksBeforeIntervention(m_meanRiskFac[before]), meanRisksAfterIntervention(m_meanRiskFac[after]);

	for(auto map1 = meanRisksBeforeIntervention.begin(); map1 != meanRisksBeforeIntervention.end(); ++map1)
	{
		agent_type = map1->first;

		if(m_raceGender.count(agent_type) > 0)
		{
			if(m_personCount.count(agent_type) > 0)
				m_popTotalsByRaceGender[intervention][agent_type] += m_personCount[agent_type].size();

			for(auto edu : NHANES::Edu::_values())
			{
				std::string agentTypeByEdu = "edu" + agent_type + std::to_string(edu);
				if(m_personCount.count(agentTypeByEdu) > 0)
					m_popTotalsRaceGenderEdu[intervention][agentTypeByEdu] += m_personCount[agentTypeByEdu].size();
			}

			for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
			{
				int risk_type = map2->first;
				m_totalMeanRiskFactors[intervention][before][agent_type][risk_type] += map2->second;
			}
		}
	}

	for(auto map1 = meanRisksAfterIntervention.begin(); map1 != meanRisksAfterIntervention.end(); ++map1)
	{
		agent_type = map1->first;
		if(m_raceGender.count(agent_type) > 0)
		{
			for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
			{
				int risk_type = map2->first;
				m_totalMeanRiskFactors[intervention][after][agent_type][risk_type] += map2->second;
			}
		}
	}
}

void CardioCounter::sumStdDevRiskFactors(std::string intervention)
{
	for(auto map1 = m_sumDiffRiskFac.begin(); map1 != m_sumDiffRiskFac.end(); ++map1)
	{
		std::string timeFrame = map1->first;
		for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
		{
			std::string agentType = map2->first;
			int pop = m_personCount[agentType].size();
			
			for(auto risk : NHANES::RiskFac::_values())
			{
				if(risk == (int)NHANES::RiskFac::SmokingStat || risk == (int)NHANES::RiskFac::HyperTension)
				{
					double percent = getMeanRiskFactor(timeFrame, agentType, risk);
					double std_err = sqrt(((percent*(1-percent))/pop));

					m_stdDevRiskFac[intervention][timeFrame][agentType][risk] += std_err;

					continue;
				}

				double sum_sq_diff = map2->second.at(risk);
				double std_dev = sqrt((sum_sq_diff/pop));

				m_stdDevRiskFac[intervention][timeFrame][agentType][risk] += std_dev;
			}
		}
	}
}

void CardioCounter::sumTenYearTotalCHDRisk(std::string intervention)
{
	for(auto mapTotalCHD = m_totalChdRiskScore.begin(); mapTotalCHD != m_totalChdRiskScore.end(); ++mapTotalCHD)
	{
		std::string timeframe = mapTotalCHD->first;
		sumTenYearCHDRisk(mapTotalCHD->second, m_totalFraminghamRisk, intervention, timeframe);
	}
}

void CardioCounter::sumTenYearFatalCHDRisk(std::string intervention)
{
	for(auto mapFatalCHD = m_fatalChdRiskScore.begin(); mapFatalCHD != m_fatalChdRiskScore.end(); ++mapFatalCHD)
	{
		std::string timeframe = mapFatalCHD->first;
		sumTenYearCHDRisk(mapFatalCHD->second, m_totalSCORERisk, intervention, timeframe);
	}
}

void CardioCounter::sumTenYearCHDRisk(std::map<std::string, CHDRiskScores> & m_tenYearCHDRisk, TotalCHDScores &m_total, std::string intervention, std::string timeFrame)
{
	double weightedMean;
	for(auto map = m_tenYearCHDRisk.begin();  map != m_tenYearCHDRisk.end(); ++map)
	{
		std::string agent_type = map->first;
		CHDRiskScores chd_risk_score(map->second);
		
		double sum = 0;
		int popCount = 0;

		for(auto sub_map = chd_risk_score.begin(); sub_map != chd_risk_score.end(); ++sub_map)
		{
			sum += std::accumulate(sub_map->second.begin(), sub_map->second.end(), 0.0);
			popCount += sub_map->second.size();
		}

		weightedMean = sum/popCount;
		m_total[intervention][timeFrame][agent_type] += weightedMean;
	}
}

double CardioCounter::getMeanRiskFactor(std::string timeFrame, std::string personType, int risk_type)
{
	if(m_meanRiskFac[timeFrame].count(personType) > 0)
		return m_meanRiskFac[timeFrame][personType][risk_type];
	else
	{
		std::cout << "Error: Cannot return risk# " << risk_type << " for person type " << personType << std::endl;
		exit(EXIT_SUCCESS);
	}
}

double CardioCounter::getChdDeaths(std::string interventionType, int year)
{
	std::string s_id = interventionType;
	std::string s_year = std::to_string(year);

	if(m_totChdDeathsPerYear.count(s_id) > 0)
	{
		return m_totChdDeathsPerYear[s_id][s_year];
	}
	else
	{
		//std::cout << "Error: Intervention: " << EET::OtherIntervention::_from_integral(id)._to_string() << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

//void CardioCounter::outputIncome(std::string geoID)
//{
//	std::ofstream file;
//	file.open(parameters->getOutputDir()+ "income.csv", std::ios::app);
//
//	if(!file.is_open())
//		exit(EXIT_SUCCESS);
//
//	std::vector<double>indvIncomes;
//
//	file << geoID << ",";
//	for(auto race : ACS::Race::_values())
//	{
//		std::string agentRace = std::to_string(race);
//		if(m_incomeByRace.count(agentRace) > 0)
//		{
//			indvIncomes = m_incomeByRace[agentRace];
//			double sumIncome = std::accumulate(indvIncomes.begin(), indvIncomes.end(), 0.0);
//			int pop = indvIncomes.size();
//
//			file << sumIncome/pop << ",";
//		}
//		else
//		{
//			file << "0" << ",";
//		}
//	}
//
//	file << std::endl;
//
//	m_incomeByRace.clear();
//}

void CardioCounter::outputMeanRiskFactor(std::string geoID)
{
	int num_trials = param->getCardioParam()->num_trials;
	std::ofstream file;
	file.open(param->getOutputDir()+ "/Cardio/Mean/Mean_Risk_" + geoID + ".csv", std::ios::app);
	if(!file.is_open())
		exit(EXIT_SUCCESS);

	file << "State,Intervention,Time,Race_Gender,Age,Tchols,LDL,HDL,SBP,Smoking,HTN,Pop, Pop_HS, Pop_SC" << std::endl;
	for(auto intervention = m_totalMeanRiskFactors.begin(); intervention != m_totalMeanRiskFactors.end(); ++intervention)
	{
		std::string intervention_name = intervention->first;

		for(auto time = intervention->second.begin(); time != intervention->second.end(); ++time)
		{
			std::string timeframe = time->first;
			for(auto race_gender = time->second.begin(); race_gender != time->second.end(); ++race_gender)
			{
				std::string race_gender_ = race_gender->first;
				double avg_pop = (double)m_popTotalsByRaceGender[intervention_name][race_gender_]/num_trials;

				file << geoID << "," << intervention_name << "," << timeframe << "," << race_gender_ << ",";

				for(auto risk = race_gender->second.begin(); risk != race_gender->second.end(); ++risk)
				{
					int risk_type = risk->first;

					double avg_risk_val = (double)risk->second/num_trials;
					double stdErr = m_stdDevRiskFac[intervention_name][timeframe][race_gender_][risk_type]/num_trials;
					
					if(risk_type == (int)NHANES::RiskFac::SmokingStat || risk_type == (int)NHANES::RiskFac::HyperTension)
					{
						double low = avg_risk_val - 1.96*stdErr;
						double up = avg_risk_val + 1.96*stdErr;

						file << avg_risk_val << "(" << low << " - " << up << "), ";
					}
					else
					{
						file << avg_risk_val << "(" << stdErr << "), ";
					}

				}

				file << avg_pop << ",";

				double avg_pop_edu = 0;

				for(auto edu : NHANES::Edu::_values())
				{
					std::string agentByEdu = "edu" + race_gender_ + std::to_string(edu);
					if(intervention_name == "Education" && timeframe == "Before Intervention") {
						avg_pop_edu = m_popTotalsRaceGenderEdu["None+No Intervention"][agentByEdu]/num_trials;
					}
					else {
						avg_pop_edu = m_popTotalsRaceGenderEdu[intervention_name][agentByEdu]/num_trials;	
					}

					if (avg_pop_edu != 0)
						file <<  avg_pop_edu << ",";
				}

				file << std::endl;
			}
		}
	}
}

void CardioCounter::outputTenYearTotalCHDRisk(std::string geoID)
{
	outputTenYearCHDRisk(m_totalFraminghamRisk, "/Cardio/Total/Ten_Year_Total_CHD", geoID);
}

void CardioCounter::outputTenYearFatalCHDRisk(std::string geoID)
{
	outputTenYearCHDRisk(m_totalSCORERisk, "/Cardio/Fatal/Ten_Year_Fatal_CHD", geoID);
}

void CardioCounter::outputTenYearCHDRisk(TotalCHDScores &m_totalRisk, std::string chd_type, std::string geoID)
{
	int num_trials = param->getCardioParam()->num_trials;
	std::string filename = chd_type + "_" + geoID + ".csv";
	std::ofstream file(param->getOutputDir()+filename, std::ios::app);

	if(!file.is_open())
		exit(EXIT_SUCCESS);

	file << "State,Intervention,Time,Race_Gender, Ten_year_CHD, Pop" << std::endl;
	for(auto intervention = m_totalRisk.begin(); intervention != m_totalRisk.end(); ++intervention)
	{
		std::string intervention_name = intervention->first;

		for(auto time = intervention->second.begin(); time != intervention->second.end(); ++time)
		{
			std::string timeframe = time->first;
			//file << geoID << "," << intervention_name << "," << timeframe << ",";
			for(auto race_gender = time->second.begin(); race_gender != time->second.end(); ++race_gender)
			{
				std::string race_gender_ = race_gender->first;

				double avg_chd_risk = (double)race_gender->second/num_trials;
				double avg_pop = (double)m_popTotalsByRaceGender[intervention_name][race_gender_]/num_trials;

				file << geoID << "," << intervention_name << "," << timeframe << "," << race_gender_ << "," << avg_chd_risk << "," << avg_pop << std::endl;
			}

			//file << std::endl;
		}
	}
}

void CardioCounter::outputStatinsUsage(std::string geoID)
{
	std::ofstream file;
	file.open(param->getOutputDir()+ "/Cardio/Statin_Usage/Statins_Use_" + geoID + ".csv", std::ios::app);
	if(!file.is_open())
		exit(EXIT_SUCCESS);

	
	std::string sep = "_";
	int num_trials = param->getCardioParam()->num_trials;

	MapStringDbl avgPopByRaceGender;
	for(auto map1 = m_popTotalsByRaceGender.begin(); map1 != m_popTotalsByRaceGender.end(); ++map1)
	{
		std::string intervention = map1->first;
		for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
		{
			std::string raceGender = map2->first;
			avgPopByRaceGender[raceGender] = map2->second/num_trials;
		}

		break;
	}

	std::string onStatins [] = {"Yes", "No"};

	std::string beforeIntervention = "Before Intervention";
	std::string afterIntervention = "After Intervention";

	//file << "State, Intervention, Time, Race_Gender, Pop, Statins_Usage, Statins_Eligible, Percent_Statins_Use" << std::endl;
	file << "State, Intervention, RaceGender, Pop, TotalStatinsEligiblePre, OnStatinsPre, NotOnStatinsPre, BaselineStatinsUse, StatinsUsePost, RemainingUsePost" << std::endl;

	for(auto map1 = m_statinUsage.begin(); map1 != m_statinUsage.end(); ++map1)
	{
		std::string interventionType = map1->first;
		for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
		{
			std::string raceGender = map2->first;

			std::string statinsUsers = onStatins[0];
			std::string nonUsers = onStatins[1];
			
			double avgPop = avgPopByRaceGender[raceGender];

			double avgStatinsEligibleNonUsers = (double)m_statinEligibles[interventionType][raceGender][nonUsers]/num_trials;
			double avgStatinsEligibleUsers = (double)m_statinEligibles[interventionType][raceGender][statinsUsers]/num_trials;

			double avgStatinsUseBeforeIntervention = (double)m_statinUsage[interventionType][raceGender][beforeIntervention]/num_trials;
			double avgStatinsUseAfterIntervention = (double)m_statinUsage[interventionType][raceGender][afterIntervention]/num_trials;

			file << geoID << "," << interventionType << ","  << raceGender << "," << avgPop << "," 
				<< (avgStatinsEligibleUsers + avgStatinsEligibleNonUsers) << "," << avgStatinsUseBeforeIntervention << ","
				<< avgStatinsEligibleNonUsers << "," << avgStatinsUseBeforeIntervention << "," 
				<< avgStatinsUseAfterIntervention << "," << avgStatinsEligibleNonUsers - avgStatinsUseAfterIntervention << std::endl;

			/**
			for(auto map3 = map2->second.begin(); map3 != map2->second.end(); ++map3)
			{
				std::string timeFrame = map3->first;
				double avgStatinsUse = (double)map3->second/num_trials;
				
				if(timeFrame == param->beforeIntervention())
				{
					//file << geoID << "," << interventionType << "," << timeFrame << "," << raceGender << "," << avgPop << "," << avgStatinsUse 
						//<< "," << avgStatinsEligibleNonUsers << "," << (double)avgStatinsUse/avgPop << std::endl; 
				}
				else if(timeFrame == param->afterIntervention())
				{ 
					//file << geoID << "," << interventionType << "," << timeFrame << "," << raceGender << "," << avgPop << "," << avgStatinsUse 
						//<< "," << avgStatinsEligibleNonUsers - avgStatinsUse << "," << (double)avgStatinsUse/avgPop << std::endl; 
				}
			}
			*/
		}

		std::cout << std::endl;
	}
}

void CardioCounter::outputCostEffectiveness(std::string geoID)
{
	std::ofstream file;
	file.open(param->getOutputDir()+ "/Cardio/CostEffectiveness/Cost_" + geoID + ".csv", std::ios::app);

	if(!file.is_open())
		exit(EXIT_SUCCESS);

	int num_trials = param->getCardioParam()->num_trials;
	int num_years = param->getCardioParam()->num_years;

	PairDouble totCost; 
	double totDalys, chdDeaths;

	StatinsUseByYear statinsUseByYear;

	int id, taxType, statinsType;
	std::string interventionType, agentType, s_taxType, s_statinsType, s_id;

	char ref = '0';

	file << "State, Intervention, Statin_Type, Tax_Type, Race_Gender, Total_Cost1(millions), Total_Cost2(millions), DALYs(years), CHD_Deaths,";

	for(int year = 1; year <= num_years + 2; ++year)
	{
		file << "Year" + std::to_string(year) << ",";
	}

	file << std::endl;
	
	for(auto map1 = m_totChdDeathsByRaceGender.begin(); map1 != m_totChdDeathsByRaceGender.end(); ++map1)
	{
		interventionType = map1->first;

		id = interventionType.at(0) - ref;
		statinsType = interventionType.at(1) - ref;
		taxType = interventionType.at(2) - ref;

		s_id = EET::Interventions::_from_integral(id)._to_string();
		if(statinsType != 0)
		{
			s_statinsType = EET::StatinsType::_from_integral(statinsType)._to_string();
		}
		else
		{
			s_statinsType = "None";
		}

		if(taxType != 0)
		{
			s_taxType = EET::TaxType::_from_integral(taxType)._to_string();
		}
		else
		{
			s_taxType = "None";
		}

		std::cout << "Intervention: " << s_id << "," << " ,Statins Type: " << s_statinsType << ", Tax Type: " << s_taxType << std::endl;

		for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
		{
			statinsUseByYear.clear();

			agentType = map2->first;

			chdDeaths = map2->second/num_trials;

			totCost = computeTotalCost(interventionType, agentType, statinsUseByYear);
			totDalys = computeDALYs(interventionType, agentType);

			file << geoID << "," << s_id << "," << s_statinsType << "," << s_taxType << "," << agentType << "," 
				<< totCost.first/pow(10, 6) << "," << totCost.second/pow(10, 6) << "," <<  totDalys << "," << chdDeaths << ",";
				

			std::cout << "Agent Type: " << agentType << ", Cost(mills): " << totCost.first/pow(10,6) << ", DALYs(yrs): " << totDalys 
				<< ", Deaths: " << chdDeaths << std::endl;

			if (statinsUseByYear.size() > 0) 
			{
				for(double statinsUse : statinsUseByYear) 
				{
					file << statinsUse << ",";
				}
			}
			else {
				for(int year = 1; year <= num_years + 2; ++year) 
				{
					file << 0 << ",";
				}
			}

			file << std::endl;
		}
	}
}

/*
* @brief Outputs the average of change in smoking status by tax intervention type
*		 and agent type (by race gender).
* @param geoId State's geo ID
*/
void CardioCounter::outputPercentSmokeChange(std::string geoId)
{
	std::ofstream file;
	file.open(param->getOutputDir()+ "/Cardio/SmokeStatus/Smoking_" + geoId + ".csv", std::ios::app);

	if(!file.is_open())
		exit(EXIT_SUCCESS);

	int num_trials = param->getCardioParam()->num_trials;

	for (auto map1 = avgPropSmokeChange.begin(); map1 != avgPropSmokeChange.end(); ++map1)
	{
		std::string taxType = map1->first;
		for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
		{
			std::string agentType = map2->first;
			for(auto map3 = map2->second.begin(); map3 != map2->second.end(); ++map3)
			{
				PairDouble smokeStatus = map3->first;
				double pSmokeChange = map3->second;

				std::cout << taxType << "," << agentType << "," << smokeStatus.first << "," << smokeStatus.second << "," << pSmokeChange/num_trials << std::endl;

				file << geoId << "," << taxType << "," << agentType << "," << smokeStatus.first << "," << smokeStatus.second << "," << pSmokeChange/num_trials << std::endl;
			}
		}
	}

	file.close();
}

CardioCounter::PairDouble CardioCounter::computeTotalCost(std::string interventionType, std::string agentType, StatinsUseByYear & statinsUseByYear)
{
	int id = interventionType.at(0) - '0';
	PairDouble totCost;

	if(id == (int)EET::Interventions::Statins || id == (int)EET::Interventions::Tax_Statins)
	{
		totCost = computeTotalCostStatins(interventionType, agentType, statinsUseByYear);
	}
	else
	{
		totCost.first = 0;
		totCost.second = 0;
	}

	return totCost;
}

double CardioCounter::computeDALYs(std::string interventionType, std::string agentType)
{
	double dalys = 0;
	double yll = 0;
	
	int num_trials = param->getCardioParam()->num_trials;
	for(auto map = m_totYLL[interventionType][agentType].begin(); map !=  m_totYLL[interventionType][agentType].end(); ++map)
	{
		std::string year = map->first;
		
		yll = (double)map->second/num_trials;
		dalys += yll;
		
	}
	return dalys;
}

CardioCounter::PairDouble CardioCounter::computeTotalCostStatins(std::string interventionType, std::string agentType, StatinsUseByYear & statinsUseByYear)
{
	int num_trials = param->getCardioParam()->num_trials;
	const double discount = 0.03;

	std::string sep = "-";
	std::string timeFrame = "";

	double totCost1 = 0;
	double totCost2 = 0;

	double cost1PerYear, cost2PerYear, numStatinsUsePerYear;

	double statinCost1 = param->getCardioParam()->statin_cost1;
	double statinCost2 = param->getCardioParam()->statin_cost2;

	int year;

	PairDouble totCost;

	for(auto map = m_statinUsage[interventionType][agentType].begin(); map != m_statinUsage[interventionType][agentType].end(); ++map)
	{
		timeFrame = map->first;
		if(timeFrame != param->beforeIntervention() && timeFrame != param->afterIntervention())
		{
			year = std::stoi(getYear(timeFrame, sep));

			numStatinsUsePerYear = (double)map->second/num_trials;

			statinsUseByYear.push_back(numStatinsUsePerYear);

			cost1PerYear = statinCost1 * numStatinsUsePerYear;
			cost2PerYear = statinCost2 * numStatinsUsePerYear;

			if(year > 0)
			{
				cost1PerYear = (cost1PerYear)/pow(1+discount, year);
				cost2PerYear = (cost2PerYear)/pow(1+discount, year);
			}

			totCost1 += cost1PerYear;
			totCost2 += cost2PerYear;
		}
	}

	totCost.first = totCost1;
	totCost.second = totCost2;

	return totCost;
}

std::string CardioCounter::getYear(std::string timeFrame, std::string sep)
{
	std::string s = timeFrame;
	std::string delimiter = sep;

	size_t pos = 0;
	std::string token;

	while ((pos = s.find(delimiter)) != std::string::npos) 
	{
		token = s.substr(0, pos);
		s.erase(0, pos + delimiter.length());
	}

	return s;
}


void CardioCounter::clearRiskFactorCounter()
{
	m_sumRiskFac.clear();
}

void CardioCounter::clearCHDRiskCounter(std::string timeFrame)
{
	m_sumDiffRiskFac[timeFrame].clear();
	m_totalChdRiskScore[timeFrame].clear();
	m_fatalChdRiskScore[timeFrame].clear();
}