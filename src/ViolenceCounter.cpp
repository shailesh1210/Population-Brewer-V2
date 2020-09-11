#include "ViolenceCounter.h"
#include "ViolenceParams.h"

ViolenceCounter::ViolenceCounter()
{
}

ViolenceCounter::ViolenceCounter(std::shared_ptr<ViolenceParams> p) : param(p), totalPrevalence(0)
{
}

ViolenceCounter::~ViolenceCounter()
{
}

void ViolenceCounter::initialize()
{
	initHouseholdCounter(param);
	initPersonCounter(param);
	initPtsdCounter();

}

void ViolenceCounter::output(std::string geoID, std::string modelNumber)
{
	//outputHouseholdCounts(param, geoID);
	//outputPersonCounts(param, geoID);

	if(modelNumber == "Model1")
	{
		outputHealthOutcomes();
		outputReach();
		outputCostEffectiveness();
	}
	else if(modelNumber == "Model2")
	{
		outputPrevalence();
	}
}

void ViolenceCounter::initPtsdCounter()
{
	clearCounter();
	
	nonPtsdCountSC = 0;
	int num_steps = param->getViolenceParam()->tot_steps;
	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		for(int j = 0; j < NUM_PTSD; ++j)
		{
			for(int k = 0; k < num_steps; ++k)
			{
				m_ptsdCount[i][j].insert(std::make_pair(k, 0));
				m_ptsdResolvedCount[i][j].insert(std::make_pair(k, 0));

				if(j == 0)
				{
					m_cbtReach[i].insert(std::make_pair(k, 0));
					m_sprReach[i].insert(std::make_pair(k, 0));

					if(totalReach[i].size() < num_steps)
						totalReach[i].push_back(0);
				}
			}	
		}
	}

	initTreatmentCounter(num_steps);
	//initOutcomeCounter(num_steps);
}

void ViolenceCounter::initTreatmentCounter(int steps)
{
	for(int i = 0; i < steps; ++i)
	{
		m_cbtCount.insert(std::make_pair(i, 0));
		m_sprCount.insert(std::make_pair(i, 0));
		m_ndCount.insert(std::make_pair(i, 0));

	}

	for(int i = 0; i < steps/WEEKS_IN_YEAR; ++i)
	{
		//m_totCbt.insert(std::make_pair(i, 0));
		for(int j = 0; j < NUM_TREATMENT; ++j)
		{
			for(int k = 0; k < NUM_CASES; ++k)
			{
				m_totCbt[j][k].insert(std::make_pair(i, 0));
				m_totSpr[j][k].insert(std::make_pair(i, 0));
			}
		}
	}

}

void ViolenceCounter::addPtsdCount(int treatment, int ptsd_type, int tick)
{
	if(m_ptsdCount[treatment][ptsd_type].count(tick) > 0)
		m_ptsdCount[treatment][ptsd_type].at(tick) += 1;
	else
	{
		std::cout << "Error: Cannot add PTSD count!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

void ViolenceCounter::addPrevalence(int population)
{
	totalPrevalence += (double)getPersonCount("Positive")/population;
}

void ViolenceCounter::addPrevalence(MapPair *m_ptsdTotals, std::string scenarioByTime, std::string source)
{
	std::string ageCat = "";
	double prevalence;
	double ptsdCases, totPop;
	for(auto map = m_ptsdTotals->begin(); map != m_ptsdTotals->end(); ++map)
	{
		ageCat = map->first;

		ptsdCases = map->second.first;
		totPop = map->second.second;

		prevalence = (double)ptsdCases/totPop;

		m_totalPrevalence2[scenarioByTime][source][ageCat] += prevalence;
		m_totalPtsdCases[scenarioByTime][source][ageCat] += ptsdCases;
		m_totalPopulation[scenarioByTime][source][ageCat] += totPop;
	}
}

void ViolenceCounter::addCbtReferredNonPtsd()
{
	nonPtsdCountSC++;
}

void ViolenceCounter::addPtsdResolvedCount(int treatment, int ptsd_type, int tick)
{
	if(m_ptsdResolvedCount[treatment][ptsd_type].count(tick) > 0)
		m_ptsdResolvedCount[treatment][ptsd_type].at(tick) += 1;
	else
	{
		std::cout << "Error: Cannot add PTSD resolved count!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

void ViolenceCounter::addCbtReach(int treatment, int tick)
{
	if(m_cbtReach[treatment].count(tick) > 0)
		m_cbtReach[treatment].at(tick) += 1;
	else
	{
		std::cout << "Error: Cannot add CBT reach!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

void ViolenceCounter::addSprReach(int treatment, int tick)
{
	if(m_sprReach[treatment].count(tick) > 0)
		m_sprReach[treatment].at(tick) += 1;
	else
	{
		std::cout << "Error: Cannot add SPR reach!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

void ViolenceCounter::addCbtCount(ViolenceAgent *agent, int tick, int treatment)
{
	if(treatment == STEPPED_CARE)
	{
		if(agent->getPTSDx(treatment) >= agent->getPtsdCutOff())
		{
			if(m_cbtCount.count(tick) > 0)
				m_cbtCount[tick] += 1;
		}

		if(agent->getCBTReferred())
		{
			int week = (tick+1);
			int year = (week % WEEKS_IN_YEAR == 0) ? ((week/WEEKS_IN_YEAR)-1) : (week/WEEKS_IN_YEAR);

			//counts number of CBT sessions received by PTSD cases and non-cases(screened incorrectly as cases)
			if(agent->getInitPTSDx() >= agent->getPtsdCutOff())
				m_totCbt[treatment][PTSD_CASE].at(year) += 1;
			else if(agent->getInitPTSDx() < agent->getPtsdCutOff())
				m_totCbt[treatment][PTSD_NON_CASE].at(year) += 1;
		}
	}
}

void ViolenceCounter::addSprCount(ViolenceAgent *agent, int tick, int treatment)
{
	int week = (tick+1);
	int year = (week % WEEKS_IN_YEAR == 0) ? ((week/WEEKS_IN_YEAR)-1) : (week/WEEKS_IN_YEAR);

	if(treatment == STEPPED_CARE)
	{
		if(agent->getPTSDx(treatment) >= agent->getPtsdCutOff())
		{
			if(m_sprCount.count(tick) > 0)
				m_sprCount[tick] += 1;
		}

		if(agent->getCBTReferred())
		{
			//counts number of SPR sessions received by PTSD cases and non-cases(screened incorrectly as cases)
			if(agent->getInitPTSDx() >= agent->getPtsdCutOff())
				m_totSpr[treatment][PTSD_CASE].at(year) += 1;
			else if(agent->getInitPTSDx() < agent->getPtsdCutOff())
				m_totSpr[treatment][PTSD_NON_CASE].at(year) += 1;
		}
		else if(agent->getSPRReferred())
		{
			if(agent->getInitPTSDx() >= agent->getPtsdCutOff())
				m_totSpr[treatment][PTSD_CASE].at(year) += 1;
		}
	}
	else if(treatment == USUAL_CARE)
	{
		if(agent->getInitPTSDx() >= agent->getPtsdCutOff())
			m_totSpr[treatment][PTSD_CASE].at(year) += 1;
	}
	
}

void ViolenceCounter::addNaturalDecayCount(int tick)
{
	if(m_ndCount.count(tick) > 0)
		m_ndCount[tick] += 1;
	else
	{
		std::cout << "Error: Cannot add Natural Decay count!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

void ViolenceCounter::computeOutcomes(int tick, int totPop)
{
	computePrevalence(tick, totPop);
	computeRecovery(tick, totPop);
	computeReach(tick);
	
}

void ViolenceCounter::computeCostEffectiveness(AgentListPtr *agentList)
{
	computeDALYs(agentList);
	computeTotalCost();
	computeAverageCost();
}

void ViolenceCounter::computePrevalence(int tick, int totPop)
{
	Outcomes prevalence;
	double ptsd_count[NUM_TREATMENT];
	double non_ptsd_count[NUM_TREATMENT];

	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		ptsd_count[i] = 0;
		non_ptsd_count[i] = 0;
	}

	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		for(int j = 0; j < NUM_PTSD; ++j)
		{
			double pts_count = m_ptsdCount[i][j].at(tick);
			double prev = (double)pts_count/totPop;

			if(m_totPrev[i][j].count(tick) > 0)
				m_totPrev[i][j].at(tick) += prev;
			else
				m_totPrev[i][j].insert(std::make_pair(tick, prev));

			ptsd_count[i] += pts_count;
		}
		
		non_ptsd_count[i] = (double)totPop - ptsd_count[i];
		prevalence.value[i] = ptsd_count[i]/totPop;
	}

	Pair stdErr(computeError(ptsd_count, non_ptsd_count));

	prevalence.diff = computeDiff(prevalence.value, stdErr.first);
	prevalence.ratio = computeRatio(prevalence.value, stdErr.second);

	if(m_prevalence.count(tick) > 0)
	{
		for(int i = 0; i < NUM_TREATMENT; ++i)
			m_prevalence[tick].value[i] += prevalence.value[i];

		m_prevalence[tick].diff += prevalence.diff;
		m_prevalence[tick].ratio += prevalence.ratio;
	}
	else
	{
		m_prevalence.insert(std::make_pair(tick, prevalence));
	}
	
}

void ViolenceCounter::computeRecovery(int tick, int totPop)
{
	Outcomes recovery;
	double ptsd_count[NUM_TREATMENT];
	double ptsd_resolved[NUM_TREATMENT];

	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		ptsd_count[i] = 0;
		ptsd_resolved[i] = 0;
	}

	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		for(int j = 0; j < NUM_PTSD; ++j)
		{
			double pts_count = m_ptsdCount[i][j].at(tick);
			double res_count = m_ptsdResolvedCount[i][j].at(tick);
			double recov = pts_count/(pts_count+res_count);

			if(m_totRecovery[i][j].count(tick) > 0)
				m_totRecovery[i][j].at(tick) += recov;
			else
				m_totRecovery[i][j].insert(std::make_pair(tick, recov));

			ptsd_count[i] += pts_count;
			ptsd_resolved[i] += res_count;
		}

		recovery.value[i] = ptsd_count[i]/(ptsd_count[i]+ptsd_resolved[i]);
	}

	Pair stdErr(computeError(ptsd_resolved, ptsd_count));
	recovery.diff = computeDiff(recovery.value, stdErr.first);
	recovery.ratio = computeRatio(recovery.value, stdErr.second);

	if(m_recovery.count(tick) > 0)
	{
		for(int i = 0; i < NUM_TREATMENT; ++i)
			m_recovery[tick].value[i] += recovery.value[i];

		m_recovery[tick].diff += recovery.diff;
		m_recovery[tick].ratio += recovery.ratio;
	}
	else
	{
		m_recovery.insert(std::make_pair(tick, recovery));
	}
	
}

void ViolenceCounter::computeReach(int tick)
{
	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		double ptsd_cases = 0;
		for(int j = 0; j < NUM_PTSD; ++j)
			ptsd_cases += m_ptsdCount[i][j].at(tick);

		if(i == STEPPED_CARE)
		{
			if(ptsd_cases > 0)
				totalReach[i][tick] += 10000*(double)(m_cbtReach[i].at(tick) + m_sprReach[i].at(tick))/ptsd_cases;
		}
		else if(i == USUAL_CARE)
		{
			if(ptsd_cases > 0)
				totalReach[i][tick] += 10000*(double)(m_sprReach[i].at(tick))/ptsd_cases;
		}
	}
}

void ViolenceCounter::computeDALYs(AgentListPtr *agentList)
{
	double dw_mild, dw_mod, dw_sev;

	dw_mild = param->getViolenceParam()->dw_mild;
	dw_mod = param->getViolenceParam()->dw_moderate;
	dw_sev = param->getViolenceParam()->dw_severe;

	for(auto pp = agentList->begin(); pp != agentList->end(); ++pp)
	{
		ViolenceAgent *agent = *pp;
		for(int i = 0; i < NUM_TREATMENT; ++i)
		{
			double daly = getYLD(dw_mild, agent->getDurationMild(i))
				+ getYLD(dw_mod, agent->getDurationModerate(i)) + getYLD(dw_sev, agent->getDurationSevere(i));

			if(m_totDalys.count(i) > 0)
				m_totDalys[i] += daly;
			else
				m_totDalys.insert(std::make_pair(i, daly));

			if(agent->getInitPTSDx() >= agent->getPtsdCutOff())
			{
				if(m_totPtsdFreeWeeks.count(i) > 0)
					m_totPtsdFreeWeeks[i] += agent->getResolvedTime(i);
				else
					m_totPtsdFreeWeeks.insert(std::make_pair(i, agent->getResolvedTime(i)));
			}
		}
	}
}

void ViolenceCounter::computeTotalCost()
{
	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		
		double tot_cost = getTotalCost(i);
		if(m_totCost.count(i) > 0)
			m_totCost.at(i) += tot_cost;
		else
			m_totCost.insert(std::make_pair(i, tot_cost));
	}
}

void ViolenceCounter::computeAverageCost()
{
	MapInts ptsd_cases;
	MapDbls m_cbtVisits[NUM_TREATMENT], m_sprVisits[NUM_TREATMENT];
	
	double avg_cbt_visits, avg_spr_visits; 
	
	int cbt_cost = param->getViolenceParam()->cbt_cost;
	int spr_cost = param->getViolenceParam()->spr_cost;
	
	int num_years = (param->getViolenceParam()->tot_steps)/WEEKS_IN_YEAR;
	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		int num_ptsd = 0;
		double avg_cost = 0;

		for(int j = 0; j < NUM_PTSD; ++j)
			num_ptsd += m_ptsdCount[i][j].at(0);

		//ptsd_cases.insert(std::make_pair(i, num_ptsd));
		for(int k1 = 0; k1 < NUM_CASES; ++k1)
		{
			double tot_cbt_visits = 0;
			double tot_spr_visits = 0;

			for(int k2 = 0; k2 < num_years; ++k2)
			{
				tot_cbt_visits += m_totCbt[i][k1].at(k2);
				tot_spr_visits += m_totSpr[i][k1].at(k2);
			}

			m_cbtVisits[i].insert(std::make_pair(k1, tot_cbt_visits));
			m_sprVisits[i].insert(std::make_pair(k1, tot_spr_visits));
		}

		if(i==STEPPED_CARE)
		{
			avg_cbt_visits = (double)m_cbtVisits[i][PTSD_CASE]/num_ptsd + (double)m_cbtVisits[i][PTSD_NON_CASE]/nonPtsdCountSC;
			avg_spr_visits = (double)m_sprVisits[i][PTSD_CASE]/num_ptsd + (double)m_sprVisits[i][PTSD_NON_CASE]/nonPtsdCountSC;

			avg_cost = cbt_cost*avg_cbt_visits + spr_cost*avg_spr_visits;
		}
		else if(i==USUAL_CARE)
		{
			avg_spr_visits = (double)m_sprVisits[i][PTSD_CASE]/num_ptsd;
			avg_cost = spr_cost*avg_spr_visits;
		}

		if(m_avgCost.count(i) > 0)
			m_avgCost[i] += avg_cost;
		else
			m_avgCost.insert(std::make_pair(i, avg_cost));
	}	

}

Risk ViolenceCounter::computeDiff(double *arr, double stdErr)
{
	Risk diff;

	diff.val = arr[STEPPED_CARE]-arr[USUAL_CARE];
	diff.lowLim = diff.val - 1.96*stdErr;
	diff.upLim = diff.val + 1.96*stdErr;
	
	return diff;
}

Risk ViolenceCounter::computeRatio(double *arr, double stdErr)
{
	Risk ratio;

	ratio.val = arr[STEPPED_CARE]/arr[USUAL_CARE];

	ratio.lowLim = exp(log(ratio.val) - 1.96*stdErr);
	ratio.upLim =  exp(log(ratio.val) + 1.96*stdErr);
	
	return ratio;
}


ViolenceCounter::Pair ViolenceCounter::computeError(double *cases, double *non_cases)
{
	Pair stdErr;
	double a = cases[STEPPED_CARE];
	double b = non_cases[STEPPED_CARE];

	double c = cases[USUAL_CARE];
	double d = non_cases[USUAL_CARE];
	
	stdErr.first = sqrt(((a*b)/(pow((a+b),3))) + ((c*d)/(pow((c+d),3)))); //RD standard error
	stdErr.second = (a != 0 && c != 0) ? sqrt((1/a)-(1/(a+b))+(1/c)-(1/(c+d))) : 0; //RR standard error

	return stdErr;
}


ViolenceCounter::VectorDbls ViolenceCounter::getPrevalence(int tick, int totPop)
{
	VectorDbls prevalence;
	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		double p = 0;
		for(int j = 0; j < NUM_PTSD; ++j)
			p += 100*(double)m_ptsdCount[i][j].at(tick)/totPop;
		prevalence.push_back(p);
	}

	return prevalence;
}

double ViolenceCounter::getTotalPrevalence()
{
	return totalPrevalence;
}

double ViolenceCounter::getCbtUptake(int tick)
{
	int tot_ptsd = 0;
	for(int j = 0; j < NUM_PTSD; ++j)
		tot_ptsd += m_ptsdCount[STEPPED_CARE][j].at(tick);

	if (tot_ptsd != 0)
		return 100*(double)m_cbtCount[tick]/tot_ptsd;
	else 
		return 0;
}

double ViolenceCounter::getSprUptake(int tick)
{
	int tot_ptsd = 0;
	for(int j = 0; j < NUM_PTSD; ++j)
		tot_ptsd += m_ptsdCount[STEPPED_CARE][j].at(tick);

	if(tot_ptsd != 0)
		return 100*(double)m_sprCount[tick]/tot_ptsd;
	else
		return 0;
}

double ViolenceCounter::getNaturalDecayUptake(int tick)
{
	int tot_ptsd = 0;
	for(int j = 0; j < NUM_PTSD; ++j)
		tot_ptsd += m_ptsdCount[STEPPED_CARE][j].at(tick);

	if(tot_ptsd != 0)
		return 100*(double)m_ndCount[tick]/tot_ptsd;
	else
		return 0;
}

double ViolenceCounter::getYLD(double dw, int durPTSD)
{
	double dis = param->getViolenceParam()->discount;
	double durPTSD_yrs = durPTSD/WEEKS_IN_YEAR;

	//years lost due to disability
	return (dw*(1/dis)*(1-exp(-dis*durPTSD_yrs))); 
}

double ViolenceCounter::getTotalCost(int trtment)
{
	if(trtment == NO_TREATMENT)
		return 0;

	int cbt_cost, spr_cost;
	double discount;

	cbt_cost = param->getViolenceParam()->cbt_cost;
	spr_cost = param->getViolenceParam()->spr_cost;
	discount = param->getViolenceParam()->discount;
	
	double tot_cost, new_cost;
	tot_cost = 0;
	
	int num_years = (param->getViolenceParam()->tot_steps)/WEEKS_IN_YEAR;
	for(int year = 0; year < num_years; ++year)
	{
		new_cost = 0;
		for(int status = 0; status < NUM_CASES; ++status)
		{
			if(trtment == STEPPED_CARE)
				new_cost += (cbt_cost*m_totCbt[trtment][status].at(year) + spr_cost*m_totSpr[trtment][status].at(year));
			else if(trtment == USUAL_CARE)
				new_cost += spr_cost*m_totSpr[trtment][status].at(year);
		}

		if(year > 0)
			new_cost = (new_cost/pow(1+discount, year));

		tot_cost += new_cost;
	}

	return tot_cost;
}

void ViolenceCounter::outputHealthOutcomes()
{
	std::string case_sim = std::to_string(param->getViolenceParam()->sim_case);

	std::ofstream file1, file2, file3, file4;
	std::string outcome_file1 = "overall_prevalence_"+case_sim+".csv";
	std::string outcome_file2 = "overall_recovery_"+case_sim+".csv";
	std::string outcome_file3 = "prevalence_ptsd_type_"+case_sim+".csv";
	std::string outcome_file4 = "recovery_ptsd_type_"+case_sim+".csv";

	file1.open(param->getOutputDir()+outcome_file1);
	file2.open(param->getOutputDir()+outcome_file2);
	file3.open(param->getOutputDir()+outcome_file3);
	file4.open(param->getOutputDir()+outcome_file4);
	
	if(!file1.is_open() && !file2.is_open() && !file3.is_open() && !file4.is_open())
	{
		std::cout << "Error: Cannot open file!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	int num_steps = param->getViolenceParam()->tot_steps;
	int num_trials = param->getViolenceParam()->num_trials;

	for(int i = 0; i < num_steps; ++i)
	{
		for(int j = 0; j < NUM_TREATMENT; ++j)
		{
			m_prevalence[i].value[j] /= num_trials;
			m_recovery[i].value[j] /= num_trials;

			for(int k = 0; k < NUM_PTSD; ++k)
			{
				m_totPrev[j][k].at(i) /= num_trials;
				m_totRecovery[j][k].at(i) /= num_trials;

				file3 << i+1 << "," << m_totPrev[j][k].at(i) << ",";
				file4 << i+1 << "," << m_totRecovery[j][k].at(i) << ",";
			}
		}

		file3 << std::endl;
		file4 << std::endl;

		m_prevalence[i].diff /= num_trials;
		m_recovery[i].diff /= num_trials;

		m_prevalence[i].ratio /= num_trials;
		m_recovery[i].ratio /= num_trials;
		

		file1 << i << "," << m_prevalence[i].value[STEPPED_CARE] << "," << m_prevalence[i].value[USUAL_CARE] << "," 
			<< m_prevalence[i].diff << "," << m_prevalence[i].ratio << std::endl;

		file2 << i << "," << m_recovery[i].value[STEPPED_CARE] << "," << m_recovery[i].value[USUAL_CARE] << "," 
			<< m_recovery[i].diff << "," << m_recovery[i].ratio << std::endl;
	}
}

void ViolenceCounter::outputReach()
{
	std::string case_sim = std::to_string(param->getViolenceParam()->sim_case);
	std::string fileName = "treatment_reach_"+case_sim+".csv";
	std::ofstream file;

	file.open(param->getOutputDir()+fileName);

	if(!file.is_open())
	{
		std::cout << "Error: Cannot open file!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	int num_trials = param->getViolenceParam()->num_trials;
	int treatment_time = param->getViolenceParam()->treatment_time;

	for(int i = 0; i < NUM_TREATMENT-1; ++i)
	{
		for(int j = 0; j < treatment_time; ++j)
			file << i << "," << j << "," << totalReach[i][j]/num_trials << std::endl;

		file << std::endl;
	}
}

void ViolenceCounter::outputCostEffectiveness()
{
	std::string case_sim = std::to_string(param->getViolenceParam()->sim_case);
	std::string fileName = "cost_effectiveness_"+case_sim+".csv";
	std::ofstream file;

	file.open(param->getOutputDir()+fileName);

	if(!file.is_open())
	{
		std::cout << "Error: Cannot open file!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	file << "Scenario, Mean Cost/person($), Total Cost($), DALYS avoided, PTSD-Free Days, ICER($ per DALY averted), ICER($ per PTSD-Free Day)" << std::endl;
	file << "CBT cost= $" << param->getViolenceParam()->cbt_cost << "/session" << std::endl;
	file << "SPR cost= $" << param->getViolenceParam()->spr_cost << "/session" << std::endl;

	std::cout << std::endl;
	std::cout << "Cost Effectivess Analysis in progress..." << std::endl;

	int num_trials = param->getViolenceParam()->num_trials;
	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		m_totDalys[i] /= num_trials;
		m_totPtsdFreeWeeks[i] /= num_trials;
		m_totCost[i] /= num_trials;
		m_avgCost[i] /= num_trials;

		std::cout << i << "," << "DALYs(years)=" << m_totDalys[i] << ", Total cost(millions)=" << m_totCost[i]/pow(10,6) 
			<< ", Average cost=" << m_avgCost[i] << ",Ptsd-free days=" << DAYS_IN_WEEK*m_totPtsdFreeWeeks[i] << std::endl;  
	}

	double icer_daly = 0; 
	double icer_ptsd_free = 0;

	for(int i = NO_TREATMENT; i >= STEPPED_CARE; --i)
	{
		if(i != NO_TREATMENT)
		{
			double daly_avoided = fabs(m_totDalys[NO_TREATMENT]-m_totDalys[i]);
			double ptsd_free = DAYS_IN_WEEK*m_totPtsdFreeWeeks[i];

			file << i << "," << m_avgCost[i] << "," << m_totCost[i] << "," << daly_avoided << "," 
				<< ptsd_free << "," << icer_daly << "," << icer_ptsd_free <<  std::endl;
			
			if(i > 0)
			{
				icer_daly = fabs((m_totCost[i-1]-m_totCost[i])/(m_totDalys[i-1]-m_totDalys[i]));
				icer_ptsd_free = fabs((m_totCost[i-1]-m_totCost[i])/(DAYS_IN_WEEK*(m_totPtsdFreeWeeks[i-1]-m_totPtsdFreeWeeks[i])));
			}
		}

	}

	file.close();
	std::cout << "Analysis complete!" << std::endl;
}

void ViolenceCounter::outputPtsdDistribution()
{
	std::cout << "PTSD symptoms distribution by quartile.." << std::endl;
	for(auto sex : Violence::Sex::_values())
	{
		for(auto ageCat : Violence::AgeCat::_values())
		{

			for(auto exp : Violence::Exposure::_values())
			{
				for (auto pcase : Violence::Ptsd_case::_values())
				{
					std::string key = std::to_string(sex)+std::to_string(ageCat)+std::to_string(exp)+std::to_string(pcase);

					std::cout << key << "," << m_personCount[key+"1Q"].size() << "," << m_personCount[key+"2Q"].size()
						<< "," << m_personCount[key+"3Q"].size() << "," << m_personCount[key+"4Q"].size() << std::endl;
				}
			}
		}
	}

}

void ViolenceCounter::outputPrevalence()
{
	std::string filename = "prevalence_by_media.csv";
	std::ofstream file;

	file.open(param->getOutputDir() + filename);

	if(!file.is_open())
	{
		std::cout << "Error: Cannot open file!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	int num_trials = param->getViolenceParam()->num_trials;
	for(auto map1 = m_totalPrevalence2.begin(); map1 != m_totalPrevalence2.end(); ++map1)
	{
		std::string time = map1->first;
		file << "\nComputing Overall Prevalence - " << time << ".." << std::endl;

		for(auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2)
		{
			std::string source = map2->first;
			for(auto map3 = map2->second.begin(); map3 != map2->second.end(); ++map3)
			{
				std::string ageCat = map3->first;

				double prev = (double)(map3->second)/num_trials;
				double ptsdCases = (double)m_totalPtsdCases[time][source][ageCat]/num_trials;
				double pop = (double)m_totalPopulation[time][source][ageCat]/num_trials;

				file << "Prevalence( " <<  Violence::AgeCat2::_from_integral(std::stoi(ageCat))._to_string() << "), " <<
					source << "," << prev << "," << ptsdCases << "," << pop << std::endl;
			}

			file << std::endl;
		}
	}
}

void ViolenceCounter::clearCounter()
{
	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		for(int j = 0; j < NUM_PTSD; ++j)
		{
			m_ptsdCount[i][j].clear();
			m_ptsdResolvedCount[i][j].clear();
		}

		for(int k = 0; k < NUM_CASES; ++k)
		{
			m_totCbt[i][k].clear();
			m_totSpr[i][k].clear();
		}

		m_cbtReach[i].clear();
		m_sprReach[i].clear();

		//totalReach[i].clear();
	}

	
	m_sprCount.clear();
	m_cbtCount.clear();
	m_ndCount.clear();

	//m_totCbt.clear();
	
}