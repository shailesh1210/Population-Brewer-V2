#include "CardioAgent.h"
#include "ACS.h"
#include "PersonPums.h"
#include "Random.h"
//#include "Parameters.h"
#include "CardioCounter.h"

CardioAgent::CardioAgent()
{
}

CardioAgent::CardioAgent(const PersonPums<CardioParams> *person, std::shared_ptr<CardioParams> param, CardioCounter *count, Random *rand) : parameters(param)
{
	if(count != NULL && rand != NULL)
	{
		counter = count;
		random = rand;
	}
	else
		exit(EXIT_SUCCESS);

	this->householdID = person->getPUMSID();
	this->puma = person->getPumaCode();

	this->age = person->getAge();
	this->initAge = this->age;

	this->sex = person->getSex();

	this->origin = person->getOrigin();
	this->nhanes_org = (origin == ACS::Origin::WhiteNH) ? NHANES::Org::WhiteNH : NHANES::Org::BlackNH;;

	this->education = person->getEducation();
	this->nhanes_edu = this->init_edu = (education <= ACS::Education::High_School) ? NHANES::Edu::HS_or_less : NHANES::Edu::some_coll_;

	this->income = person->getIncome();

	this->rfStrata = -1;

	this->pTotalCHD = 0;
	this->pFatalCHD = 0;

	this->dead = false;
	this->deathYear = -1;

	setNHANESAgeCat();

	counter->addPersonCount(nhanes_org, sex);
	//count agent by race, gender and education - used to calculate education difference
	counter->addPersonCount(getAgentType4()); 

}

CardioAgent::~CardioAgent()
{
}

void CardioAgent::setNHANESAgeCat()
{
	if(age >=35 && age <= 44)
	{
		this->nhanes_ageCat = NHANES::AgeCat::Age_35_44;
		this->nhanes_ageCat3 = NHANES::AgeCat3::Age_35_44;
	}
	else if(age >=45 && age <= 64)
	{
		this->nhanes_ageCat3 = NHANES::AgeCat3::Age_45_64;
		if(age <= 54)
		{
			this->nhanes_ageCat = NHANES::AgeCat::Age_45_54;
		}
		else
		{
			this->nhanes_ageCat = NHANES::AgeCat::Age_55_64;
		}
	}
	else if(age >= 65)
	{
		this->nhanes_ageCat3 = NHANES::AgeCat3::Age_65_;
		if(age <= 74)
		{
			this->nhanes_ageCat = NHANES::AgeCat::Age_65_74;
		}
		else
		{
			this->nhanes_ageCat = NHANES::AgeCat::Age_75_;
		}
	}
	else
	{
		this->nhanes_ageCat = -1;
		this->nhanes_ageCat3 = -1;
	}
	
}


void CardioAgent::setRiskFactors(short int risk_strata, EET::RiskFactors risk_factors)
{
	this->initRiskStrata = this->rfStrata = risk_strata;
	this->initChart = this->chart = risk_factors;

	counter->addRiskFactors(this, getAgentType2(), getAgentType4());
}

void CardioAgent::update(int intervention_type, int taxType, int statinsType, int currentYear, bool isEducationPresent)
{
	//updateAge();
	updateRiskFactors(intervention_type, taxType, statinsType, currentYear, isEducationPresent);
}

void CardioAgent::computeTenYearCHDRisk(std::string timeFrame)
{
	computeFraminghamCHDRisk(timeFrame);
	computeScoreFatalCHDRisk(timeFrame);
	computeRiskFactorDifference(timeFrame);
}

void CardioAgent::processChdEvent(int id, int statinsType, int taxType, int year, bool isEducationPresent)
{
	deathFatalChd(parameters->getInterventionType(id, statinsType, taxType, isEducationPresent), year);
	//updateAge();

	if(!isDead())
	{
		counter->addStatinsUsage(this, getAgentType2(), parameters->afterIntervention(year+1), id, statinsType, taxType, isEducationPresent);	
	}
}

void CardioAgent::resetAttributes()
{
	this->dead = false;
	this->deathYear = -1;

	resetAge();
	resetRiskFactors();
}

void CardioAgent::computeFraminghamCHDRisk(std::string timeFrame)
{
	MapDbl weighted_sum;
	for(auto type : NHANES::WeightedSum::_values())
		weighted_sum.insert(std::make_pair(type, getWeightedSumFramingham(type)));

	double e = exp(weighted_sum[NHANES::WeightedSum::Individual] - weighted_sum[NHANES::WeightedSum::Mean]);

	double survival_rate = getTenYrTotalCHDSurvivalARIC();
	this->pTotalCHD = 1 - pow(survival_rate, e);

	counter->addTenYearTotalCHDRisk(timeFrame, getAgentType2(), getAgentType(), this->pTotalCHD);
}

void CardioAgent::computeScoreFatalCHDRisk(std::string timeFrame)
{
	double e = exp(getWeightedSumScore());
	double fatal_survival_rate = getTenYrFatalCHDSurvivalARIC();

	this->pFatalCHD = 1 - pow(fatal_survival_rate, e);

	if(timeFrame == parameters->afterIntervention())
	{
		double randomP = random->uniform_real_dist();
		if(randomP < pFatalCHD)
		{
			this->deathYear = random->random_int(1, num_years);
		}
	}

	counter->addTenYearFatalCHDRisk(timeFrame, getAgentType2(), getAgentType(), this->pFatalCHD);
	
}

void CardioAgent::computeRiskFactorDifference(std::string timeFrame)
{
	std::string agentType = getAgentType2();
	double difference;
	for(auto risk : NHANES::RiskFac::_values())
	{
		if(risk == (int)NHANES::RiskFac::SmokingStat || risk == (int)NHANES::RiskFac::HyperTension)
			continue;

		if(risk == (int)NHANES::RiskFac::Age)
		{
			difference = initAge - getMeanAge(timeFrame);
		}
		else if(risk == (int)NHANES::RiskFac::totalChols)
		{
			difference = chart.tchols.first - getMeanTChols(timeFrame);
		}
		else if(risk == (int)NHANES::RiskFac::HdlChols)
		{
			difference = chart.hdlChols.first - getMeanHDL(timeFrame);
		}
		else if(risk == (int)NHANES::RiskFac::SystolicBp)
		{
			difference = chart.systolicBp.first - getMeanSysBp(timeFrame);
		}

		counter->addRiskFactorDifference(timeFrame, agentType, risk, difference);
	}
}

void CardioAgent::updateAge()
{
	if(!dead)
	{
		this->age += 1;
		setNHANESAgeCat();
	}
}

//void CardioAgent::updateRiskFactors(int interventionId, int currentYear, bool isEducationPresent)
//{
//	switch(interventionId)
//	{
//	case EET::EduIntervention::Education:
//		educationIntervention();
//		break;
//	case EET::OtherIntervention::Dollar_Tax:
//		smokingTaxIntervention(interventionId, isEducationPresent, EET::TaxType::One_Dollar, currentYear);
//		break;
//	case EET::OtherIntervention::Two_Dollar_Tax:
//		smokingTaxIntervention(interventionId, isEducationPresent, EET::TaxType::Two_Dollar, currentYear);
//		break;
//	case EET::OtherIntervention::Five_Dollar_Tax:
//		smokingTaxIntervention(interventionId, isEducationPresent, EET::TaxType::Five_Dollar, currentYear);
//		break;
//	case EET::OtherIntervention::Statins:
//		statinIntervention(interventionId, isEducationPresent, currentYear);
//		break;
//	case EET::OtherIntervention::Weak_Statins:
//		statinIntervention(interventionId, isEducationPresent, currentYear);
//		break;
//	case EET::OtherIntervention::Dollar_Tax_Statins:
//		smokingTaxIntervention(interventionId, isEducationPresent, EET::TaxType::One_Dollar, currentYear);
//		statinIntervention(interventionId, isEducationPresent, currentYear);
//		break;
//	case EET::OtherIntervention::Two_Dollar_Tax_Statins:
//		smokingTaxIntervention(interventionId, isEducationPresent, EET::TaxType::Two_Dollar, currentYear);
//		statinIntervention(interventionId, isEducationPresent, currentYear);
//		break;
//	default:
//		break;
//	}
//
//	//counter->addRiskFactors(this, getAgentType2());
//	counter->addRiskFactors(this, getAgentType2(), getAgentType4());
//}

void CardioAgent::updateRiskFactors(int interventionId, int taxType, int statinsType, int currentYear, bool isEducationPresent)
{
	switch(interventionId)
	{
	case EET::EduIntervention::Education:
		educationIntervention();
		break;
	case EET::Interventions::Tax:
		smokingTaxIntervention(isEducationPresent, taxType, currentYear);
		break;
	case EET::Interventions::Statins:
		statinIntervention(interventionId, taxType, statinsType, isEducationPresent, currentYear);
		break;
	case EET::Interventions::Tax_Statins:
		statinIntervention(interventionId, taxType, statinsType, isEducationPresent, currentYear);
		smokingTaxIntervention(isEducationPresent, taxType, currentYear);
		break;
	default:
		break;
	}

	//counter->addRiskFactors(this, getAgentType2());
	counter->addRiskFactors(this, getAgentType2(), getAgentType4());
}

void CardioAgent::resetAge()
{
	this->age = this->initAge;
}

void CardioAgent::resetEducation()
{
	if(this->nhanes_edu != this->init_edu)
		this->nhanes_edu = this->init_edu;
}

void CardioAgent::resetRiskFactors()
{
	this->rfStrata = this->initRiskStrata;
	this->chart = this->initChart;
}


void CardioAgent::setRiskStrata(short int rs)
{
	this->rfStrata = rs; 
}


//void CardioAgent::smokingTaxIntervention(bool isEducationPresent, int tax_type, int currentYear)
//{
//	double randP = random->uniform_real_dist();
//	double pChangeSmoking = getPercentChangeSmoking(tax_type);
//
//	if(chart.curSmokeStat && pChangeSmoking < 0)
//	{
//		double pQuitSmoke = getSmokingProbability(isEducationPresent, pChangeSmoking, currentYear);
//		if(randP < pQuitSmoke)
//		{
//			chart.curSmokeStat = 0;
//			setRiskStrata(getRiskStrata(true));
//			updateRisksTaxIntervention();
//		}
//	}
//	else if(!chart.curSmokeStat && pChangeSmoking > 0)
//	{
//		double pStartSmoke = getSmokingProbability(isEducationPresent, pChangeSmoking, currentYear);
//		if(randP < pStartSmoke)
//		{
//			chart.curSmokeStat = 1;
//			setRiskStrata(getRiskStrata(true));
//			updateRisksTaxIntervention();
//			
//		}
//	}
//}

/*
* @brief Implements the smoking tax intervention by computing probability of quitting smoking.
* @param isEducationPresent True if education intervention is present
* @param taxType Type of tax intervention (Dollar, Two Dollar taxation)
* @param currentYear Current time-step (year) 
*/
void CardioAgent::smokingTaxIntervention(bool isEducationPresent, int tax_type, int currentYear)
{
	double randP = random->uniform_real_dist();
	double pChangeSmoking = getPercentChangeSmoking(tax_type);

	if(chart.isSmoker && pChangeSmoking < 0)
	{
		double pQuitSmoke = getSmokingProbability(isEducationPresent, pChangeSmoking, currentYear);
		if(randP < pQuitSmoke)
		{
			chart.isSmoker = 0;

			setRiskStrata(getRiskStrata(true));
			updateRisksTaxIntervention(tax_type);
		}
	}
	else if(!chart.isSmoker && pChangeSmoking > 0)
	{
		double pStartSmoke = getSmokingProbability(isEducationPresent, pChangeSmoking, currentYear);
		if(randP < pStartSmoke)
		{
			chart.isSmoker = 1;

			setRiskStrata(getRiskStrata(true));
			updateRisksTaxIntervention(tax_type);
			
		}
	}
}

void CardioAgent::statinIntervention(int id, int taxType, int statinsType, bool isEducationPresent, int currYear)
{
	if(age >= statin_age_min && age <= statin_age_max)
	{
		if(currYear == 0)
			counter->addStatinsUsage(this, getAgentType2(), parameters->beforeIntervention(), id, statinsType, taxType, isEducationPresent);

		if(isStatinQualified() && !isOnStatin())
		{
			//updateRisksStatinsIntervention(id, statinsType, taxType, isEducationPresent);
			updateRisksStatinsIntervention(id, statinsType, taxType, currYear, isEducationPresent);
			counter->addStatinsUsage(this, getAgentType2(), parameters->afterIntervention(), id, statinsType, taxType, isEducationPresent);
		}

		counter->addStatinsUsage(this, getAgentType2(), parameters->afterIntervention(currYear), id, statinsType, taxType, isEducationPresent);			
	}
}

void CardioAgent::educationIntervention()
{
	counter->addStatinsUsage(this, getAgentType2(), parameters->beforeIntervention(), EET::EduIntervention::Education, 0, 0, true);

	if(nhanes_org == NHANES::Org::BlackNH && nhanes_edu == NHANES::Edu::HS_or_less)
	{
		double pEduDiff = getPercentEduDifference();

		if(pEduDiff > 0)
		{
			int count = 0;
			double randP = random->uniform_real_dist();
			if(randP < pEduDiff)
			{
				this->nhanes_edu = NHANES::Edu::some_coll_;
				updateRisksEducationIntervention(randP);

				this->initRiskStrata = this->rfStrata;
				this->initChart = this->chart;
			}
		}
	}
	counter->addStatinsUsage(this, getAgentType2(), parameters->afterIntervention(), EET::EduIntervention::Education, 0, 0, true);
	counter->addPersonCount(getAgentType4());
	
}

/*
* @brief Updates the risk factors of agents for smoking tax intervention. 
*        Gets the list of new risk factors after the change in smoking status
*        and risk strata. Randomly select from the list of risk factors until
*		 new risk factor is less than old risk factor (if smoking status is changed
*        from smoker to non-smoker (or former smoker).
*/
void CardioAgent::updateRisksTaxIntervention(int tax_type)
{
	//const VectorWeightRisks *new_risks = parameters->getRiskFactorByStrata(getAgentType1(), rfStrata);
	const VectorWeightRisks *new_risks = parameters->getRiskFactorByStrata(getAgentType5(), rfStrata);
	int count = 0;

	bool wasOnStatins = isOnStatin();

	EET::RiskFactors oldRiskFactor = this->chart;

	while(true)
	{
		if(new_risks->size() == 0)
			break;

		int idx = random->random_int(0, new_risks->size()-1);

		EET::RiskFactors newRiskFactor = new_risks->at(idx).second;

		if(chart.isSmoker)
		{
			if(isNewRiskGreater(newRiskFactor))
			{
				this->chart = newRiskFactor;
				break;
			}
			else
			{
				if(count == new_risks->size())
				{
					this->chart = newRiskFactor;
					break;
				}

				count++;
				continue;
			}
		}
		else if(!chart.isSmoker)
		{ 
			if (!isNewRiskGreater(newRiskFactor)) 
			{
				if (newRiskFactor.curSmokeStat == NHANES::SmokingStatus::FormerSmoker)
				{
					this->chart = newRiskFactor;
					break;
				}
				else if(newRiskFactor.curSmokeStat == NHANES::SmokingStatus::NonSmoker)
				{
					if (count == new_risks->size())
					{
						this->chart = newRiskFactor;
						break;
					}
				}

			}
			else
			{
				if (count > new_risks->size()) 
				{
					this->chart = newRiskFactor;
					break;
				}
			}
			
			count++;
		}
	}

	//Agents on statins will continue to be in on statins
	this->chart.onStatin = 0;

	if(wasOnStatins && !isOnStatin())
	{
		this->chart.onStatin = 1;
	}

	//Counter for smoking change
	if (oldRiskFactor.curSmokeStat == NHANES::SmokingStatus::CurrentSmoker) 
	{
		if (chart.curSmokeStat == NHANES::SmokingStatus::FormerSmoker || chart.curSmokeStat == NHANES::SmokingStatus::NonSmoker)
		{
			PairInts smokeChange = std::make_pair(oldRiskFactor.curSmokeStat, chart.curSmokeStat);
			counter->addSmokingChange(std::to_string(tax_type), getAgentType2(), smokeChange);
		}

		counter->addSmokers(std::to_string(tax_type), getAgentType2());
	}
}

//void CardioAgent::updateRisksStatinsIntervention(int id, int statinsType, int taxType, bool isEducationPresent)
//{
//	double statinUptake;
//	if(statinsType == (int)EET::StatinsType::Weak)
//	{
//		//statinUptake = 0.5*getStatinUptakeProbability();
//		statinUptake = 1.5*getStatinUptakeProbability();
//	}
//	else if(statinsType == (int)EET::StatinsType::Strong)
//	{
//		//statinUptake = getStatinUptakeProbability();
//		statinUptake = 2.0*getStatinUptakeProbability();
//	}
//	else if(statinsType == (int)EET::StatinsType::Stronger)
//	{
//		//statinUptake = 1.5*getStatinUptakeProbability();
//		statinUptake = 3.0*getStatinUptakeProbability();
//	}
//	else if(statinsType == (int)EET::StatinsType::Strongest) 
//	{
//		statinUptake = 4.0*getStatinUptakeProbability();
//	}
//
//	double randP = random->uniform_real_dist();
//	if(randP < statinUptake)
//	{
//		chart.onStatin = 1;
//
//		chart.ldlChols -= chart.ldlChols*getLDLReductionPercent();
//		chart.hdlChols.first += chart.hdlChols.first*getHDLReductionPercent();
//		chart.triglyceride -= chart.triglyceride*getTriglycerideReductionPercent();
//
//		chart.tchols.first = chart.ldlChols + chart.hdlChols.first + 0.2*chart.triglyceride;
//
//		//counter->addStatinsUsage(this, getAgentType2(), parameters->afterIntervention(), id, statinsType, taxType, isEducationPresent);
//	}
//}

void CardioAgent::updateRisksStatinsIntervention(int id, int statinsType, int taxType, int currentYear, bool isEducationPresent)
{
	double initialUptake = getStatinUptakeProbability();
	double estimatedStatinsUsePostIntervention;

	int increment = 1;
	double statinUptake;
	if(statinsType == (int)EET::StatinsType::Weak)
	{
		increment = 2;
		estimatedStatinsUsePostIntervention = getEstimatedStatinsUse(initialUptake, increment);
	}
	else if(statinsType == (int)EET::StatinsType::Strong)
	{
		increment = 3;
		estimatedStatinsUsePostIntervention = getEstimatedStatinsUse(initialUptake, increment);
	}
	else if(statinsType == (int)EET::StatinsType::Stronger)
	{
		increment = 4;
		estimatedStatinsUsePostIntervention = getEstimatedStatinsUse(initialUptake, increment);
	}
	else if(statinsType == (int)EET::StatinsType::Strongest) 
	{
		estimatedStatinsUsePostIntervention = 1;
	}

	statinUptake = estimatedStatinsUsePostIntervention/2;

	if(currentYear == 1)
	{
		statinUptake = (statinUptake) / (1-statinUptake);
	}

	double randP = random->uniform_real_dist();
	if(randP < statinUptake)
	{
		chart.onStatin = 1;

		chart.ldlChols -= chart.ldlChols*getLDLReductionPercent();
		chart.hdlChols.first += chart.hdlChols.first*getHDLReductionPercent();
		chart.triglyceride -= chart.triglyceride*getTriglycerideReductionPercent();

		chart.tchols.first = chart.ldlChols + chart.hdlChols.first + 0.2*chart.triglyceride;

		//counter->addStatinsUsage(this, getAgentType2(), parameters->afterIntervention(), id, statinsType, taxType, isEducationPresent);
	}
}

void CardioAgent::updateRisksEducationIntervention(double randP)
{
	randP = random->uniform_real_dist();

	const VectorCumulativeProbability *m_pRiskByEdu = parameters->getRiskStrataCumulativeProbability(getAgentType1());
	for(auto p = m_pRiskByEdu->begin(); p != m_pRiskByEdu->end(); ++p)
	{
		double pRiskStrataByEdu = p->second.back().first;
		if(randP < pRiskStrataByEdu)
		{
			int risk_strata = p->first;
			this->rfStrata = risk_strata;

			for(auto p1 = p->second.begin(); p1 != p->second.end(); ++p1)
			{
				double pRiskFactor = p1->first;
				if(randP < pRiskFactor)
				{
					this->chart = p1->second;
					break;
				}
			}
			break;
		}
	}
}

void CardioAgent::deathFatalChd(std::string interventionType, int year)
{
	//double randomP = random->uniform_real_dist();
	double yll = 0;

	std::string s_year = std::to_string(year);

	//if(randomP < this->pFatalCHD)
	if(year == this->deathYear)
	{
		this->dead = true;
		
		counter->addChdDeaths(interventionType, s_year, getAgentType2());
		counter->addYearsLifeLost(interventionType, s_year, getAgentType2(), getYLL());
	}
}

bool CardioAgent::isNewRiskGreater(EET::RiskFactors new_risks)
{
	if(getSumRisks(chart) < getSumRisks(new_risks))
		return true;
	else
		return false;
}

bool CardioAgent::isStatinQualified() const
{
	bool isAtRisk = false;

	if(chart.ldlChols > ldl_thres) {
		isAtRisk = true;
	}
	else if(chart.hdlChols.first < hdl_thres) {
		isAtRisk = true;
	}
	else if(chart.systolicBp.second) {
		isAtRisk = true;
	}
	else if(chart.curSmokeStat) {
		isAtRisk = true;
	}
	else {
		isAtRisk = false;
	}

	if(isAtRisk && this->pTotalCHD > 0.1) {
		return true;
	}
	else {
		return false;
	}

}


double CardioAgent::getWeightedSumFramingham(int type)
{
	double sum = 0;
	short int age_ = this->initAge;

	EET::Framingham b = getBetaFHS();
	switch(type)
	{
	case NHANES::WeightedSum::Individual:
		{
			if(sex == NHANES::Sex::Male && age_ > 70)
				age_ = 70;
			else if(sex == NHANES::Sex::Female && age_ > 78)
				age_ = 78;

			sum = b.age*log(age_) + b.tchols*log(chart.tchols.first) + b.hdl*log(chart.hdlChols.first) 
				+ b.sbp*log(chart.systolicBp.first) + b.htn*chart.htnMed + b.smoker*chart.curSmokeStat
				+ b.age_tchols*log(age_)*log(chart.tchols.first) + b.age_smoker*log(age_)*chart.curSmokeStat
				+ b.sq_age*log(age_)*log(age_);
			break;
		}
	case NHANES::WeightedSum::Mean:
		{
			double meanAge = getMeanAge(parameters->beforeIntervention());
			double meanTchols = getMeanTChols(parameters->beforeIntervention());
			double meanHDL = getMeanHDL(parameters->beforeIntervention());
			double meanSBP = getMeanSysBp(parameters->beforeIntervention());
			double percentHTN = getPercentHTN(parameters->beforeIntervention());
			double percentSmoking = getPercentSmoking(parameters->beforeIntervention());

			sum = b.age*log(meanAge) + b.tchols*log(meanTchols) + b.hdl*log(meanHDL) 
				+ b.sbp*log(meanSBP) + b.htn*percentHTN + b.smoker*percentSmoking
				+ b.age_tchols*log(meanAge)*log(meanTchols) + b.age_smoker*log(meanAge)*percentSmoking
				+ b.sq_age*log(meanAge)*log(meanAge);
			break;
		}
	default:
		break;
	}

	return sum;
}

double CardioAgent::getWeightedSumScore()
{
	const double chols_conversion = 0.02586;
	double sum = 0;

	EET::Score beta = getBetaScore();
	sum = beta.tchols*(chols_conversion*(chart.tchols.first-getMeanTChols(parameters->beforeIntervention()))) 
		+ beta.sbp*(chart.systolicBp.first-getMeanSysBp(parameters->beforeIntervention())) 
		+ beta.smoking*(chart.curSmokeStat);
	return sum;
}

double CardioAgent::getSumRisks(EET::RiskFactors risk)
{
	return risk.tchols.first + risk.systolicBp.first;
}


EET::Framingham CardioAgent::getBetaFHS()
{
	if(sex == NHANES::Sex::Male)
		return parameters->getCardioParam()->male_coeff;
	else
		return parameters->getCardioParam()->female_coeff;
}

EET::Score CardioAgent::getBetaScore()
{
	return parameters->getCardioParam()->score_beta_coeff;
}

double CardioAgent::getTenYrSurvivalFramingham()
{
	if(sex == NHANES::Sex::Male)
		return parameters->getCardioParam()->chd_survival_male_fhs;
	else
		return parameters->getCardioParam()->chd_survival_female_fhs;
}

double CardioAgent::getTenYrTotalCHDSurvivalARIC()
{
	if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Male)
		return parameters->getCardioParam()->chd_survival_white_male;
	else if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Female)
		return parameters->getCardioParam()->chd_survival_white_female;
	else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Male)
		return parameters->getCardioParam()->chd_survival_black_male;
	else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Female)
		return parameters->getCardioParam()->chd_survival_black_female;
	else
		return -1;
	
}

double CardioAgent::getTenYrFatalCHDSurvivalARIC()
{
	if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Male)
		return parameters->getCardioParam()->fatal_chd_survival_white_male;
	else if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Female)
		return parameters->getCardioParam()->fatal_chd_survival_white_female;
	else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Male)
		return parameters->getCardioParam()->fatal_chd_survival_black_male;
	else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Female)
		return parameters->getCardioParam()->fatal_chd_survival_black_female;
	else
		return -1;
}

double CardioAgent::getPercentChangeSmoking(int tax_type)
{
	if(nhanes_edu == NHANES::Edu::HS_or_less)
	{
		if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Male)
			return tax_type*parameters->getCardioParam()->pReductionSmokingWMHS;
		else if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Female)
			return tax_type*parameters->getCardioParam()->pReductionSmokingWFHS;
		else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Male)
			return tax_type*parameters->getCardioParam()->pReductionSmokingBMHS;
		else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Female)
			return tax_type*parameters->getCardioParam()->pReductionSmokingBFHS;
		else
			return -1;
	}
	else if(nhanes_edu == NHANES::Edu::some_coll_)
	{
		if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Male)
			return tax_type*parameters->getCardioParam()->pReductionSmokingWMSC;
		else if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Female)
			return tax_type*parameters->getCardioParam()->pReductionSmokingWFSC;
		else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Male)
			return tax_type*parameters->getCardioParam()->pReductionSmokingBMSC;
		else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Female)
			return tax_type*parameters->getCardioParam()->pReductionSmokingBFSC;
		else
			return -1;
	}
	else
	{
		return -1;
	}
}

double CardioAgent::getSmokingProbability(bool isEducationPresent, double percentChangeSmoking, int currentYear)
{
	double baselinePrevalence = getPercentSmoking(isEducationPresent, currentYear);
	double pSmoking = -1;

	if(percentChangeSmoking != 0 && baselinePrevalence != 0)
	{
		if(percentChangeSmoking > 0)
		{
			pSmoking = percentChangeSmoking/(1-baselinePrevalence);
		}
		else
		{
			pSmoking = abs(percentChangeSmoking)/baselinePrevalence;
		}
	}

	return pSmoking;
}

double CardioAgent::getStatinUptakeProbability()
{
	if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Male)
		return parameters->getCardioParam()->statin_uptake_WM;
	else if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Female)
		return parameters->getCardioParam()->statin_uptake_WF;
	else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Male)
		return parameters->getCardioParam()->statin_uptake_BM;
	else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Female)
		return parameters->getCardioParam()->statin_uptake_BF;
	else
		return -1;
}

double CardioAgent::getEstimatedStatinsUse(double initialUptake, int increment) 
{
	return (increment * initialUptake) / (1 - initialUptake);
}

double CardioAgent::getLDLReductionPercent()
{
	double change_ldl = parameters->getCardioParam()->ldl_change/100;
	return change_ldl;
}

double CardioAgent::getHDLReductionPercent()
{
	double change_hdl = parameters->getCardioParam()->hdl_change/100;
	return change_hdl;
}

double CardioAgent::getTriglycerideReductionPercent()
{
	double change_tgl = parameters->getCardioParam()->trigly_change/100;
	return change_tgl;
}

double CardioAgent::getPercentEduDifference()
{
	std::string agent_type = getAgentType2();
	return parameters->getPercentEduDifference(agent_type);
}

double CardioAgent::getYLL()
{
	double yll = 0;
	const double discount = 0.03;

	double lifeExpectancy = getLifeExpectancy();
	if(age < lifeExpectancy)
	{
		yll = lifeExpectancy - age;
	}
	else
	{
		yll = 0;
	}

	yll = (1/discount)*(1-exp(-discount*yll));
	return yll;
}

double CardioAgent::getLifeExpectancy()
{
	if(nhanes_edu == NHANES::Edu::HS_or_less)
	{
		if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Male)
		{
			return parameters->getCardioParam()->lifeExpectancyWMHS;
		}
		else if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Female)
		{
			return parameters->getCardioParam()->lifeExpectancyWFHS;
		}
		else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Male)
		{
			return parameters->getCardioParam()->lifeExpectancyBMHS;
		}
		else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Female)
		{
			return parameters->getCardioParam()->lifeExpectancyBFHS;
		}
		else
		{
			return -1;
		}
		
	}
	else if(nhanes_edu == NHANES::Edu::some_coll_)
	{
		if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Male)
		{
			return parameters->getCardioParam()->lifeExpectancyWMSC;
		}
		else if(nhanes_org == NHANES::Org::WhiteNH && sex == NHANES::Sex::Female)
		{
			return parameters->getCardioParam()->lifeExpectancyWFSC;
		}
		else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Male)
		{
			return parameters->getCardioParam()->lifeExpectancyBMSC;
		}
		else if(nhanes_org == NHANES::Org::BlackNH && sex == NHANES::Sex::Female)
		{
			return parameters->getCardioParam()->lifeExpectancyBFSC;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

short int CardioAgent::getNHANESAgeCat() const
{
	return nhanes_ageCat;
}

short int CardioAgent::getNHANESOrigin() const
{
	return nhanes_org;
}

short int CardioAgent::getNHANESEduCat() const
{
	return nhanes_edu;
}

short int CardioAgent::getRiskStrata() const
{
	return rfStrata;
}

short int CardioAgent::getRiskStrata(bool flag) const
{
	return (short int)parameters->getRiskStrata(getRiskState());
}

/**
*	@brief returns agentType by risk strata, origin, sex, age and edu
*	@param none
*	@return string
*/
std::string CardioAgent::getAgentType() const
{
	std::string agentType = std::to_string(rfStrata)+std::to_string(nhanes_org)
		+std::to_string(sex)+std::to_string(nhanes_ageCat3)+std::to_string(nhanes_edu);

	return agentType;
}


/**
*	@brief returns agentType by origin, sex, age cat 3 and edu
*	@param none
*	@return string
*/
std::string CardioAgent::getAgentType1() const
{
	std::string agentType = std::to_string(nhanes_org) +std::to_string(sex)+std::to_string(nhanes_ageCat3)
		+std::to_string(nhanes_edu);

	return agentType;
}

/**
*	@brief returns agentType by origin and sex
*	@param none
*	@return string
*/
std::string CardioAgent::getAgentType2() const
{
	return std::to_string(nhanes_org) + std::to_string(sex);
}

/**
*	@brief returns agentType by race, gender, age cat3
*	@param none
*	@return string
*/
std::string CardioAgent::getAgentType3() const
{
	std::string agentType = std::to_string(nhanes_org) + std::to_string(sex)
		+ std::to_string(nhanes_ageCat3);
	return agentType;
}

/**
*	@brief returns agentType by race, gender, education
*	@param none
*	@return string
*/
std::string CardioAgent::getAgentType4() const
{
	std::string agentType = "edu" + std::to_string(nhanes_org) + std::to_string(sex)
		+ std::to_string(nhanes_edu);
	return agentType;
}

/**
*	@brief returns agentType by race, gender, age cat (45-54, 55-64..), and education cat
*	@param none
*	@return string
*/
std::string CardioAgent::getAgentType5() const 
{
	std::string agentType = std::to_string(nhanes_org) +std::to_string(sex)+std::to_string(nhanes_ageCat)
		+std::to_string(nhanes_edu);

	return agentType;
}

std::string CardioAgent::getRiskState() const
{
	return std::to_string(chart.tchols.second) + std::to_string(chart.hdlChols.second) 
		+ std::to_string(chart.systolicBp.second) + std::to_string(chart.isSmoker) 
		+ std::to_string(chart.htnMed);
}


double CardioAgent::getRiskFactor(int risk) const
{
	double risk_val = -1;
	switch(risk)
	{
	case NHANES::RiskFac::Age:
		risk_val = initAge;
		break;
	case NHANES::RiskFac::totalChols:
		risk_val = chart.tchols.first;
		break;
	case NHANES::RiskFac::LdlChols:
		risk_val = chart.ldlChols;
		break;
	case NHANES::RiskFac::HdlChols:
		risk_val = chart.hdlChols.first;
		break;
	case NHANES::RiskFac::SystolicBp:
		risk_val = chart.systolicBp.first;
		break;
	case NHANES::RiskFac::SmokingStat:
		//risk_val = (chart.curSmokeStat) ? 1 : 0;
		risk_val = (chart.isSmoker) ? 1 : 0;
		break;
	case NHANES::RiskFac::HyperTension:
		risk_val = (chart.htnMed) ? 1 : 0;
		break;
	default:
		break;
	}

	return risk_val;
}


double CardioAgent::getMeanAge(std::string timeFrame) const
{
	return (counter->getMeanRiskFactor(timeFrame, getAgentType2(), NHANES::RiskFac::Age));
}

double CardioAgent::getMeanHDL(std::string timeFrame) const
{
	return (counter->getMeanRiskFactor(timeFrame, getAgentType2(), NHANES::RiskFac::HdlChols));
}

double CardioAgent::getMeanTChols(std::string timeFrame) const
{
	return (counter->getMeanRiskFactor(timeFrame, getAgentType2(), NHANES::RiskFac::totalChols));
}

double CardioAgent::getMeanSysBp(std::string timeFrame) const
{
	return (counter->getMeanRiskFactor(timeFrame, getAgentType2(), NHANES::RiskFac::SystolicBp));
}

double CardioAgent::getPercentSmoking(std::string timeFrame) const
{
	return counter->getMeanRiskFactor(timeFrame, getAgentType2(), NHANES::RiskFac::SmokingStat);
}

double CardioAgent::getPercentSmoking(bool isEducationPresent, int currentYear) const
{
	if(!isEducationPresent)
	{
		if(currentYear == 0)
		{
			return counter->getMeanRiskFactor(parameters->beforeIntervention(), getAgentType4(), NHANES::RiskFac::SmokingStat);
		}
		else
		{
			return counter->getMeanRiskFactor(parameters->afterIntervention(), getAgentType4(), NHANES::RiskFac::SmokingStat);
		}
	}
	else
	{
		if(currentYear == 0)
		{
			return counter->getMeanRiskFactor(parameters->afterEducation(), getAgentType4(), NHANES::RiskFac::SmokingStat);
		}
		else
		{
			return counter->getMeanRiskFactor(parameters->afterIntervention(), getAgentType4(), NHANES::RiskFac::SmokingStat);
		}
		
	}
}

double CardioAgent::getPercentHTN(std::string timeFrame) const
{
	return counter->getMeanRiskFactor(timeFrame, getAgentType2(), NHANES::RiskFac::HyperTension);
}


bool CardioAgent::isDead() const
{
	return this->dead;
}

bool CardioAgent::isOnStatin() const
{
	if(chart.onStatin)
		return true;
	else
		return false;
}


