#include "CardioParams.h"

CardioParams::CardioParams() {}

CardioParams::CardioParams(const char *inDir, const char *outDir, const int simModel, const int geoLvl)
	: Parameters(inDir, outDir, simModel, geoLvl)
{
	readNHANESRiskFactors();
	readFraminghamCoefficients();
}

CardioParams::~CardioParams()
{
}

//void CardioParams::readNHANESRiskFactors()
//{
//	readRiskFactorMatrix();
//
//	io::CSVReader<15>risk_factors(getFilePath("risk_factors/nhanes_risk_factors_new.csv"));
//
//	risk_factors.read_header(io::ignore_extra_column, "Risk_Strata", "Race", "Gender",
//		"Age_Cat3", "Edu_Cat", "Weight", "HDL", "Tchols", "HTN", "Triglyc",
//		"LDL", "SysBP", "Smoking_Stat", "Diff", "Statin");
//
//	const char* risk_strata = NULL;
//	const char* race = NULL; const char* gender = NULL; const char* age_cat3 = NULL;
//	const char* edu = NULL;
//	const char* weight = NULL;
//
//	const char* hdl = NULL; const char* tchols = NULL; const char* htn = NULL;
//	const char* triglyc = NULL; const char* ldl = NULL; const char* bp = NULL;
//	const char* smoking = NULL;
//	const char *diff = NULL;
//	const char *statin = NULL;
//
//	double _weight, _diff;
//	int _risk_strata;
//	EET::RiskFactors risks;
//	WeightRiskPair weight_risks;
//
//	std::map<std::string, std::vector<double>> weightByAgentType;
//
//	const double bottom = -0.656;
//	const double top = 1.136;
//
//	while(risk_factors.read_row(risk_strata, race, gender, age_cat3, edu,
//		weight, hdl, tchols, htn, triglyc, ldl, bp, smoking, diff, statin))
//	{
//		_diff = std::stod(diff);
//
//		const char *risk_vars [] = {edu, weight, hdl, tchols, htn, triglyc, ldl, bp, smoking, statin};
//		int size = sizeof(risk_vars)/sizeof(risk_vars[0]);
//		
//		if(!isEmpty(risk_vars, size) && _diff >= bottom && _diff <= top)
//		{
//			std::string agent_type = getNHANESpersonType(race, gender, age_cat3, edu);
//		
//			_weight = std::stod(weight);
//
//			_risk_strata = (!std::stoi(htn)) ? std::stoi(risk_strata) : std::stoi(risk_strata)+NUM_RISK_STRATA/2;
//
//			risks.tchols = std::make_pair(std::stod(tchols), m_riskMatrix[_risk_strata].tchols);
//			risks.hdlChols = std::make_pair(std::stod(hdl), m_riskMatrix[_risk_strata].hdl);
//			risks.systolicBp = std::make_pair(std::stod(bp), m_riskMatrix[_risk_strata].sysBp);
//
//			risks.ldlChols = std::stod(ldl);
//			risks.triglyceride = std::stod(triglyc);
//			risks.curSmokeStat = std::stoi(smoking);
//			risks.htnMed = std::stoi(htn);
//			risks.onStatin = std::stoi(statin);
//
//			weight_risks = std::make_pair(_weight, risks);
//			m_riskFactors[agent_type][_risk_strata].push_back(weight_risks);
//			
//			weightByAgentType[agent_type].push_back(_weight);
//
//		}
//	}
//
//	VectorCumulativeProbability cumulativeProbByStrata;
//
//	std::vector<WeightRiskPair> cumulativeSum;
//
//	for(auto map = m_riskFactors.begin(); map != m_riskFactors.end(); ++map)
//	{
//		std::string agent_type = map->first;
//		
//		double tot_weight = std::accumulate(weightByAgentType[agent_type].begin(), weightByAgentType[agent_type].end(), 0.0);
//		double sumRiskStrataWeights = 0;
//
//		cumulativeProbByStrata.clear();
//		for(auto sub_map = map->second.begin(); sub_map != map->second.end(); ++sub_map)
//		{
//			int risk_strata = sub_map->first;
//			cumulativeSum.clear();
//			for(auto risk_vec = sub_map->second.begin(); risk_vec != sub_map->second.end(); ++risk_vec)
//			{
//				risk_vec->first /= tot_weight;
//				sumRiskStrataWeights += risk_vec->first;
//
//				cumulativeSum.push_back(std::make_pair(sumRiskStrataWeights, risk_vec->second));
//			}
//
//			cumulativeProbByStrata.push_back(std::make_pair(risk_strata, cumulativeSum));
//		}
//
//		m_pRiskStrataEdu[agent_type] = cumulativeProbByStrata;
//	}
//}

/*
* @brief Reads NHANES risk factors (Total Cholesterol, HDL, LDL, Triglycerides, 
*        Systolic BP), hypertension medication, Smoking status, Statins intake.
*		 Computes selection probability of risk factors by person type 
*        (race, gender, age cat, and edu)
*/
void CardioParams::readNHANESRiskFactors()
{
	readRiskFactorMatrix();

	io::CSVReader<14>risk_factors(getFilePath("risk_factors/nhanes_final.csv"));

	risk_factors.read_header(io::ignore_extra_column, "Risk_Strata", "Race", "Gender",
		"Age_Cat", "Edu_Cat", "Weight", "HDL", "Tchols", "HTN", "Triglyc",
		"LDL", "SysBP", "Smoking_Stat", "Statin");

	const char* risk_strata = NULL;
	const char* race = NULL; const char* gender = NULL; const char* age_cat = NULL;
	const char* edu = NULL;
	const char* weight = NULL;

	const char* hdl = NULL; const char* tchols = NULL; const char* htn = NULL;
	const char* triglyc = NULL; const char* ldl = NULL; const char* bp = NULL;
	const char* smoking = NULL;
	const char *statin = NULL;

	double _weight;
	int _risk_strata;

	EET::RiskFactors risks;

	WeightRiskPair weight_risks;
	RiskFactorsMap2 m_tempRiskFactors;
	
	WeightsByAgentType m_weightByAgentType;
	PairDD ldl_trig_pair;

	while(risk_factors.read_row(risk_strata, race, gender, age_cat, edu,
		weight, hdl, tchols, htn, triglyc, ldl, bp, smoking, statin))
	{
		const char *risk_vars [] = {edu, weight, hdl, tchols, htn, triglyc, ldl, bp, smoking, statin};
		int size = sizeof(risk_vars)/sizeof(risk_vars[0]);
		
		if(!isEmpty(risk_vars, size))
		{
			std::string agent_type = getNHANESpersonType(race, gender, age_cat, edu);

			_weight = std::stod(weight);
			_risk_strata = std::stoi(risk_strata);

			risks.tchols = std::make_pair(std::stod(tchols), m_riskMatrix[_risk_strata].tchols);
			risks.hdlChols = std::make_pair(std::stod(hdl), m_riskMatrix[_risk_strata].hdl);
			risks.systolicBp = std::make_pair(std::stod(bp), m_riskMatrix[_risk_strata].sysBp);

			risks.ldlChols = std::stod(ldl);
			risks.triglyceride = std::stod(triglyc);

			risks.curSmokeStat = std::stoi(smoking);
			risks.htnMed = std::stoi(htn);
			risks.onStatin = std::stoi(statin);

			if (risks.curSmokeStat == NHANES::SmokingStatus::CurrentSmoker) {
				risks.isSmoker = 1;
			}
			else {
				risks.isSmoker = 0;
			}

			ldl_trig_pair = std::make_pair(risks.ldlChols, risks.triglyceride);

			weight_risks = m_tempRiskFactors[agent_type][_risk_strata][ldl_trig_pair];

			weight_risks.first += _weight;
			weight_risks.second = risks;

			m_tempRiskFactors[agent_type][_risk_strata][ldl_trig_pair] = weight_risks;
			
		}
	}

	addRiskFactors(m_tempRiskFactors, m_weightByAgentType); 
	computeRiskFactorSelectionProbability(m_weightByAgentType);
	
}



/*
* @brief Adds risk factors and their associated weights to the risk factor map
* @param m_tempRiskFactors Temporary storage for risk factors by agent type, strata, (ldl, triglyceriedes)
* @param m_weightsAgents Collection of weights by agent type (race, gender, age cat, and education)
*/
void CardioParams::addRiskFactors(RiskFactorsMap2 & m_tempRiskFactors, WeightsByAgentType &m_weightByAgentType) {
	
	WeightRiskPair weight_risks;

	for (auto map1 = m_tempRiskFactors.begin(); map1 != m_tempRiskFactors.end(); ++map1) {
		std::string agentType = map1->first;

		for (auto map2 = map1->second.begin(); map2 != map1->second.end(); ++map2) {
			int strata = map2->first;

			for (auto map3 = map2->second.begin(); map3 != map2->second.end(); ++map3) {

				weight_risks = map3->second;

				m_riskFactors[agentType][strata].push_back(weight_risks);
				m_weightByAgentType[agentType].push_back(weight_risks.first);
			}
		}
	}
}

/*
* @brief Computes selection probability of risk factors each strata and agent type
* @param m_weightByAgentType Collection of risk factor weights by agent type
*/
void CardioParams::computeRiskFactorSelectionProbability( WeightsByAgentType &m_weightByAgentType) {
	VectorCumulativeProbability cumulativeProbByStrata;
	std::vector<WeightRiskPair> cumulativeSum;

	for(auto map = m_riskFactors.begin(); map != m_riskFactors.end(); ++map)
	{
		std::string agent_type = map->first;
		
		double tot_weight = std::accumulate(m_weightByAgentType[agent_type].begin(), m_weightByAgentType[agent_type].end(), 0.0);
		double sumRiskStrataWeights = 0;

		cumulativeProbByStrata.clear();
		for(auto sub_map = map->second.begin(); sub_map != map->second.end(); ++sub_map)
		{
			int risk_strata = sub_map->first;
			cumulativeSum.clear();

			for(auto risk_vec = sub_map->second.begin(); risk_vec != sub_map->second.end(); ++risk_vec)
			{
				//Compute selection probability of risk factor (weight/total_weight)
				double weight = risk_vec->first;
				double selectionProb = weight / tot_weight;

				risk_vec->first = selectionProb;
				//risk_vec->first /= tot_weight;

				sumRiskStrataWeights += risk_vec->first;
				cumulativeSum.push_back(std::make_pair(sumRiskStrataWeights, risk_vec->second));
			}

			cumulativeProbByStrata.push_back(std::make_pair(risk_strata, cumulativeSum));
		}

		m_pRiskStrataEdu[agent_type] = cumulativeProbByStrata;
	}
}

/*
* @brief Reads "risk_matrix.csv" file containing the different combinations of 
*        risk factors (Total Cholesterol, HDL, Systolic BP, Smoking Status, and Hypertension)
*		 that makes up the risk strata. Risk factors for each strata is represented by either
*	     0 or 1. 1 means risk factor is present for that strata.
*/
void CardioParams::readRiskFactorMatrix()
{
	io::CSVReader<6>risk_matrix(getFilePath("risk_factors/risk_matrix.csv"));

	risk_matrix.read_header(io::ignore_extra_column, "Risk_strata", "Tchols", "HdlChols", 
		"SystolicBp", "SmokingStat", "HyperTension");

	const char *risk_strata = NULL;
	const char *tchols = NULL;
	const char *hdl = NULL;
	const char *sysBP = NULL;
	const char *smokeStat = NULL;
	const char *htn = NULL;

	EET::RiskState risk;
	while(risk_matrix.read_row(risk_strata, tchols, hdl, sysBP, smokeStat, htn))
	{
		risk.tchols = std::stoi(tchols);
		risk.hdl = std::stoi(hdl);
		risk.sysBp = std::stoi(sysBP);
		risk.smokingStat = std::stoi(smokeStat);
		risk.htn = std::stoi(htn);

		m_riskMatrix.insert(std::make_pair(std::stoi(risk_strata), risk));
		m_riskStrata.insert(std::make_pair(getRiskFactorCombination(risk), std::stoi(risk_strata)));

	}
}

void CardioParams::readFraminghamCoefficients()
{
	io::CSVReader<3>framingham_file(getFilePath("risk_factors/framingham_params.csv"));
	framingham_file.read_header(io::ignore_extra_column, "Variable", "Value1", "Value2");

	const char* var = NULL;
	const char* value1 = NULL;
	const char* value2 = NULL;

	PairMap m_framingham;
	while(framingham_file.read_row(var, value1, value2))
	{
		PairDD value;
		value.first = std::stod(value1);
		value.second = std::stod(value2);

		m_framingham.insert(std::make_pair(var, value));
	}

	setCardioParams(&m_framingham);
}


void CardioParams::setCardioParams(PairMap *m_params)
{
	cardioParams.num_years = (int)m_params->at("num_years").first;
	cardioParams.num_trials = (int)m_params->at("num_trials").first;

	cardioParams.male_coeff.age = m_params->at("beta_age_fhs").first;
	cardioParams.female_coeff.age = m_params->at("beta_age_fhs").second;

	cardioParams.male_coeff.tchols = m_params->at("beta_tchols_fhs").first;
	cardioParams.female_coeff.tchols = m_params->at("beta_tchols_fhs").second;

	cardioParams.male_coeff.hdl = m_params->at("beta_hdl_fhs").first;
	cardioParams.female_coeff.hdl = m_params->at("beta_hdl_fhs").second;

	cardioParams.male_coeff.sbp = m_params->at("beta_sbp_fhs").first;
	cardioParams.female_coeff.sbp = m_params->at("beta_sbp_fhs").second;

	cardioParams.male_coeff.htn = m_params->at("beta_trt_sbp_fhs").first;
	cardioParams.female_coeff.htn = m_params->at("beta_trt_sbp_fhs").second;

	cardioParams.male_coeff.smoker = m_params->at("beta_smoker_fhs").first;
	cardioParams.female_coeff.smoker = m_params->at("beta_smoker_fhs").second;

	cardioParams.male_coeff.age_tchols = m_params->at("beta_age_tchols_fhs").first;
	cardioParams.female_coeff.age_tchols = m_params->at("beta_age_tchols_fhs").second;

	cardioParams.male_coeff.age_smoker = m_params->at("beta_age_smoker_fhs").first;
	cardioParams.female_coeff.age_smoker = m_params->at("beta_age_smoker_fhs").second;

	cardioParams.male_coeff.sq_age = m_params->at("beta_sq_age_fhs").first;
	cardioParams.female_coeff.sq_age = m_params->at("beta_sq_age_fhs").second;

	cardioParams.score_beta_coeff.tchols =  m_params->at("beta_tchols_score").first;
	cardioParams.score_beta_coeff.sbp = m_params->at("beta_sbp_score").first;
	cardioParams.score_beta_coeff.smoking = m_params->at("beta_smoker_score").first;

	cardioParams.chd_survival_male_fhs = m_params->at("chd_survival_fhs").first;
	cardioParams.chd_survival_female_fhs = m_params->at("chd_survival_fhs").second;

	cardioParams.chd_survival_white_male = m_params->at("chd_survival_white_aric").first;
	cardioParams.chd_survival_white_female = m_params->at("chd_survival_white_aric").second;

	cardioParams.chd_survival_black_male = m_params->at("chd_survival_black_aric").first;
	cardioParams.chd_survival_black_female = m_params->at("chd_survival_black_aric").second;

	cardioParams.fatal_chd_survival_white_male = m_params->at("fatal_chd_survival_white_aric").first;
	cardioParams.fatal_chd_survival_white_female = m_params->at("fatal_chd_survival_white_aric").second;

	cardioParams.fatal_chd_survival_black_male = m_params->at("fatal_chd_survival_black_aric").first;
	cardioParams.fatal_chd_survival_black_female = m_params->at("fatal_chd_survival_black_aric").second;

	cardioParams.pReductionSmokingWMHS = m_params->at("preval_change_smoking_white_HS").first;
	cardioParams.pReductionSmokingWFHS = m_params->at("preval_change_smoking_white_HS").second;

	cardioParams.pReductionSmokingWMSC = m_params->at("preval_change_smoking_white_SC").first;
	cardioParams.pReductionSmokingWFSC = m_params->at("preval_change_smoking_white_SC").second;

	cardioParams.pReductionSmokingBMHS = m_params->at("preval_change_smoking_black_HS").first;
	cardioParams.pReductionSmokingBFHS = m_params->at("preval_change_smoking_black_HS").second;

	cardioParams.pReductionSmokingBMSC = m_params->at("preval_change_smoking_black_SC").first;
	cardioParams.pReductionSmokingBFSC = m_params->at("preval_change_smoking_black_SC").second;

	cardioParams.lifeExpectancyWMHS = m_params->at("life_exp_white_HS").first;
	cardioParams.lifeExpectancyWFHS = m_params->at("life_exp_white_HS").second;

	cardioParams.lifeExpectancyWMSC = m_params->at("life_exp_white_SC").first;
	cardioParams.lifeExpectancyWFSC = m_params->at("life_exp_white_SC").second;

	cardioParams.lifeExpectancyBMHS = m_params->at("life_exp_black_HS").first;
	cardioParams.lifeExpectancyBFHS = m_params->at("life_exp_black_HS").second;

	cardioParams.lifeExpectancyBMSC = m_params->at("life_exp_black_SC").first;
	cardioParams.lifeExpectancyBFSC = m_params->at("life_exp_black_SC").second;

	cardioParams.statin_uptake_WM = m_params->at("statin_uptake_white").first;
	cardioParams.statin_uptake_WF = m_params->at("statin_uptake_white").second;

	cardioParams.statin_uptake_BM = m_params->at("statin_uptake_black").first;
	cardioParams.statin_uptake_BF = m_params->at("statin_uptake_black").second;

	cardioParams.statin_cost1 = m_params->at("statin_cost").first;
	cardioParams.statin_cost2 = m_params->at("statin_cost").second;

	cardioParams.ldl_change = m_params->at("ldl_change_statin").first;
	//cardioParams.ldl_change.second = m_params->at("ldl_change_statin").second;

	cardioParams.hdl_change = m_params->at("hdl_change_statin").first;
	//cardioParams.hdl_change.second = m_params->at("hdl_change_statin").second;

	cardioParams.trigly_change = m_params->at("trigly_change_statin").first;
	//cardioParams.trigly_change.second = m_params->at("trigly_change_statin").second;

}

CardioParams::ProbMap CardioParams::getRiskStrataProbability() const
{
	return m_riskStrataProb;
}

std::string CardioParams::beforeIntervention() const
{
	return "Before Intervention";
}

std::string CardioParams::beforeIntervention(int year) const
{
	return beforeIntervention() + "-Year-" + std::to_string(year);
}

std::string CardioParams::afterIntervention() const
{
	return "After Intervention";
}

std::string CardioParams::afterIntervention(int year) const
{
	return afterIntervention() + "-Year-" + std::to_string(year);
}

std::string CardioParams::beforeEducation() const
{
	return "Before Education";
}

std::string CardioParams::afterEducation() const
{
	return "After Education";
}

const CardioParams::VectorCumulativeProbability * CardioParams::getRiskStrataCumulativeProbability(std::string agent_type)
{
	if(m_pRiskStrataEdu.count(agent_type) > 0)
		return &m_pRiskStrataEdu[agent_type];
	else
	{
		std::cout <<"Error: Agent type " << agent_type << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}


const CardioParams::AgentStrataRiskMap * CardioParams::getRiskFactorMap()
{
	return &m_riskFactors;
}

const CardioParams::VectorWeightRisks * CardioParams::getRiskFactorByStrata(std::string agent_type, int strata)
{
	if(m_riskFactors.count(agent_type) > 0)
	{
		if(m_riskFactors[agent_type].count(strata) > 0)
		{
			return &m_riskFactors[agent_type][strata];
		}
		else
		{
			VectorWeightRisks temp;
			m_riskFactors[agent_type].insert(std::make_pair(strata, temp));

			return &m_riskFactors[agent_type][strata];
		}
	}
	else
	{
		std::cout <<"Error: Agent type " << agent_type << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}


const EET::RiskState * CardioParams::getRiskMatrix(int risk_strata)
{
	if(m_riskMatrix.count(risk_strata) > 0)
		return &m_riskMatrix[risk_strata];
	else{
		std::cout << "Error: Risk Strata = " << risk_strata << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

const int CardioParams::getRiskStrata(std::string risk_matrix)
{
	if(m_riskStrata.count(risk_matrix) > 0)
		return m_riskStrata[risk_matrix];
	else{
		std::cout << "Error: Risk matrix " << risk_matrix << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

const double CardioParams::getPercentEduDifference(std::string agent_type)
{
	if(percentEduDifference.count(agent_type) > 0)
		return percentEduDifference[agent_type];
	else
	{
		std::cout << "Error: Agent type " <<  agent_type << " doesn't exist!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

const EET::Cardio* CardioParams::getCardioParam()
{
	if(simType == EQUITY_EFFICIENCY)
	{
		return &cardioParams;
	}
	else
	{
		std::cout << "Error: Wrong simulation model selected!" << std::endl;
		exit(EXIT_SUCCESS);
	}
}

std::string CardioParams::getInterventionType(int id, int statinsType, int taxType, bool isEduPresent)
{
	std::string interventionType = std::to_string(id) + std::to_string(statinsType) + std::to_string(taxType);
	if(!isEduPresent) {
		return interventionType;
	}
	else {
		return "Edu+" + interventionType;
	}
}

std::string CardioParams::getRiskFactorCombination(EET::RiskState & risk)
{
	return std::to_string(risk.tchols) + std::to_string(risk.hdl) 
		+ std::to_string(risk.sysBp) + std::to_string(risk.smokingStat) + std::to_string(risk.htn);
}

void CardioParams::setPercentEduDifference(MapDbl perEduDiff)
{
	this->percentEduDifference.clear();
	this->percentEduDifference = perEduDiff;
}
