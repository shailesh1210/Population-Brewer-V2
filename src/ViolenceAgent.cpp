#include "ViolenceAgent.h"
#include "PersonPums.h"
#include "ACS.h"
#include "ViolenceParams.h"
#include "Random.h"
#include "ViolenceCounter.h"

ViolenceAgent::ViolenceAgent()
{
}

ViolenceAgent::ViolenceAgent(std::shared_ptr<ViolenceParams> param, const PersonPums<ViolenceParams> *p, Random *rand, ViolenceCounter *count, int hhCount, int countPersons) 
	: parameters(param), random(rand), counter(count)
{
	this->householdID = hhCount;
	this->puma = p->getPumaCode();

	this->age = p->getAge();
	this->sex = p->getSex();
	this->origin = p->getOrigin();

	this->education = p->getEducation();
	this->income = p->getIncome();

	this->agentIdx = "Agent"+std::to_string(hhCount)+std::to_string(countPersons);
	
	for(int i = 0; i < NUM_TREATMENT; ++i)
	{
		this->ptsdx[i] = 0.0;
		this->ptsdTime[i] = 0;
		this->durMild[i] = 0;
		this->durMod[i] = 0;
		this->durSevere[i] = 0;

		this->curCBT[i] = false;
		this->curSPR[i] = false;
		this->sessionsCBT[i] = 0;
		this->sessionsSPR[i] = 0;

		this->isResolved[i] = false;
		this->resolvedTime[i] = 0;
		this->numRelapse[i] = 0;
		this->relapseTimer[i] = 0;
	}
	
	this->initPtsdx = ptsdx[STEPPED_CARE];
	
	this->ptsdCase = false;
	this->priPTSD = false;
	this->secPTSD = false;
	this->terPTSD = false;

	this->priorCBT = 0;
	this->priorSPR = 0;

	this->schoolName = "N/A";
	this->hoursWatched = 0;
	this->socialMediaHours = 0;
	this->newsSource = 0;

	setAgeCat();
	setAgeCat2();
	setNewOrigin();
	setDummyVariables();
}

ViolenceAgent::~ViolenceAgent()
{
	
}

void ViolenceAgent::excecuteRules(int tick)
{
	if(initPtsdx != 0.0)
	{
		priorTreatmentUptake(tick);
		ptsdScreeningSteppedCare(tick);

		for(int i = 0; i < NUM_TREATMENT; ++i)
		{
			provideTreatment(tick, i);
			symptomResolution(tick, i);
			symptomRelapse(i);
		}
	}
}

void ViolenceAgent::priorTreatmentUptake(int tick)
{
	if(tick == 0)
	{
		//Random random;
		short int male = (sex == Violence::Sex::Male) ? 1 : 0;

		double logCBT = 0;
		double logSPR = 0;

		if(initPtsdx < getPtsdCutOff())
		{
			logCBT = -1.3360 + (-0.0881*male) + (-0.7250*black) + (-0.3133*hispanic) + (-0.7122*other) + (0.6201*age2) + (-0.6444*age3);
			logSPR = -1.095 + (-0.2119*male) + (-0.5343*black) + (-0.2377*hispanic) + (-0.7551*other) + (0.5693*age2) + (-0.5413*age3);
		}
		else if(initPtsdx >= getPtsdCutOff())
		{
			logCBT = 0.2887 + (-0.1323*male) + (-0.4647*black) + (-0.4502*hispanic) + (-1.2979*other) + (0.1119*age2) + (-0.1129*age3);
			logSPR = 0.6048 + (-0.3331*male) + (-0.5914*black) + (-0.4878*hispanic) + (-0.8119*other) + (0.2076*age2) + (-0.0720*age3);
		}

		double pCBT = exp(logCBT)/(1+exp(logCBT));
		double pSPR = exp(logSPR)/(1+exp(logSPR));

		double randomCBT = random->uniform_real_dist();
		priorCBT = (randomCBT < pCBT) ? 1 : 0;

		double randomSPR = random->uniform_real_dist();
		priorSPR = (randomSPR < pSPR) ? 1 : 0;
	}
}

void ViolenceAgent::ptsdScreeningSteppedCare(int tick)
{
	int screening_time = parameters->getViolenceParam()->screening_time;
	if(tick == screening_time)
	{
		if(initPtsdx >= getPtsdCutOff())
		{
			double randomP1 = random->uniform_real_dist();
			double sensitivity = parameters->getViolenceParam()->sensitivity;
			if(randomP1 < sensitivity)
			{
				cbtReferred = true;
				sprReferred = false;
			}
			else
			{
				cbtReferred = false;
				sprReferred = true;
			}
		}
		else if(initPtsdx < getPtsdCutOff())
		{
			double randomP2 = random->uniform_real_dist();
			double specificity = 1 - (parameters->getViolenceParam()->specificity);
			if(randomP2 < specificity)
			{
				cbtReferred = true;
				sprReferred = false;

				counter->addCbtReferredNonPtsd();
			}
			else
			{
				cbtReferred = false;
				sprReferred = true;
			}
		}
	}
}

void ViolenceAgent::provideTreatment(int tick, int treatment)
{
	int screening_time = parameters->getViolenceParam()->screening_time;
	if(tick >= screening_time)
	{
		if(treatment == STEPPED_CARE)
		{
			if(cbtReferred && !sprReferred)
				provideCBT(tick, STEPPED_CARE);
			else if(!cbtReferred && sprReferred)
				provideSPR(tick, STEPPED_CARE);
		}
		else if(treatment == USUAL_CARE)
		{
			provideSPR(tick, USUAL_CARE);
		}
	}
}

void ViolenceAgent::symptomResolution(int tick, int treatment)
{
	double strength = 0;
	int max_sessions = -1;
	
	if(curCBT[treatment] && !curSPR[treatment])
	{
		strength = parameters->getViolenceParam()->cbt_coeff;
		max_sessions = parameters->getViolenceParam()->max_cbt_sessions;
	
		sessionsCBT[treatment] += 1;
		curCBT[treatment] = false;

		counter->addCbtCount(this, tick, treatment);
	}
	else if(!curCBT[treatment] && curSPR[treatment])
	{
		strength = parameters->getViolenceParam()->spr_coeff;
		max_sessions =  parameters->getViolenceParam()->max_spr_sessions;

		sessionsSPR[treatment] += 1;
		curSPR[treatment] = false;

		counter->addSprCount(this, tick, treatment);
	}
	else if(!curCBT[treatment] && !curSPR[treatment])
	{
		double pNatDecay = parameters->getViolenceParam()->percent_nd;
		double randomP = random->uniform_real_dist();

		max_sessions = parameters->getViolenceParam()->nd_dur;
		if(randomP < pNatDecay)
		{
			if(ptsdx[treatment] >= getPtsdCutOff() && treatment == STEPPED_CARE)
				counter->addNaturalDecayCount(tick);
			strength = parameters->getViolenceParam()->nd_coeff;
		}
	}

	if(ptsdx[treatment] > 0)
	{
		if(ptsdx[treatment] >= getPtsdCutOff())
			counter->addPtsdCount(treatment, getPtsdType(), tick);

		ptsdx[treatment] = ptsdx[treatment] - (strength/max_sessions)*ptsdx[treatment];
		if(ptsdx[treatment] < 0)
			ptsdx[treatment] = 0;

		if(initPtsdx >= getPtsdCutOff())
		{
			if(ptsdx[treatment] >= getPtsdCutOff())
			{
				ptsdTime[treatment] += 1;
				isResolved[treatment] = false;

				//counter to keep track of time-spent by an agent at different levels of PTSDx
				if(ptsdx[treatment] <= MILD_PTSDX_MAX)
					durMild[treatment]++;
				else if(ptsdx[treatment] <= MOD_PTSDX_MAX)
					durMod[treatment]++;
				else if(ptsdx[treatment] <= MAX_PTSDX)
					durSevere[treatment]++;

			}
			else if(ptsdx[treatment] < getPtsdCutOff() && !isResolved[treatment])
			{
				isResolved[treatment] = true;
			}

			if(isResolved[treatment])
			{
				counter->addPtsdResolvedCount(treatment, getPtsdType(), tick);
				resolvedTime[treatment] += 1;
			}
		}
		
	}
}

void ViolenceAgent::symptomRelapse(int treatment)
{
	double relapse_ptsdx = parameters->getViolenceParam()->ptsdx_relapse;
	if(isResolved[treatment] && initPtsdx > relapse_ptsdx)
	{
		double relapse_time = parameters->getViolenceParam()->time_relapse;
		double max_relapses = parameters->getViolenceParam()->num_relapse;

		relapseTimer[treatment] += 1;
		if(relapseTimer[treatment] > relapse_time && numRelapse[treatment] < max_relapses)
		{
			double randomP = random->uniform_real_dist();
			double percent_relapse = parameters->getViolenceParam()->percent_relapse;

			if(randomP < percent_relapse)
			{
				ptsdx[treatment] = 0.8*initPtsdx;
			
				curCBT[treatment] = false;
				curSPR[treatment] = false;

				isResolved[treatment] = false;
				resolvedTime[treatment] = 0;

				numRelapse[treatment] += 1;
			}
		}
	}
}

void ViolenceAgent::provideCBT(int tick, int treatment)
{
	double pCBT = getTrtmentUptakeProbability(treatment, CBT);
	if(pCBT > 0.0)
	{
		double randomP = random->uniform_real_dist();
		int maxCBTsessions = parameters->getViolenceParam()->max_cbt_sessions;
	
		if(randomP < pCBT && tick < getMaxCbtTime())
		{
			curSPR[treatment] = false;
			if(sessionsCBT[treatment] < 2*maxCBTsessions)
				curCBT[treatment] = true;
			else
				curCBT[treatment] = false;

			if(treatment == STEPPED_CARE)
			{
				//if(sessionsCBT[treatment] == 0 && sessionsSPR[treatment] == 0 && ptsdx[treatment] >= getPtsdCutOff())
					//counter->addCbtReach(treatment, tick);

				if(ptsdx[treatment] >= getPtsdCutOff())
					counter->addCbtReach(treatment, tick);
			}
		}
		else
		{
			if(initPtsdx < getPtsdCutOff() && tick == getMaxCbtTime())
			{
				cbtReferred = false;
				sprReferred = true;
			}
			provideSPR(tick, treatment);
		}
	}
	else
	{
		curCBT[treatment] = false;
	}
}

void ViolenceAgent::provideSPR(int tick, int treatment)
{
	curCBT[treatment] = false;

	double pSPR = getTrtmentUptakeProbability(treatment, SPR);
	if(pSPR > 0)
	{
		double randomP = random->uniform_real_dist();
		int maxSPRsessions = parameters->getViolenceParam()->max_spr_sessions;

		if(randomP < pSPR && tick < getMaxSprTime())
		{
			if(sessionsSPR[treatment] < maxSPRsessions)
				curSPR[treatment] = true;
			else
				curSPR[treatment] = false;

			//if(sessionsCBT[treatment] == 0 && sessionsSPR[treatment] == 0 && ptsdx[treatment] >= getPtsdCutOff())
				//counter->addSprReach(treatment, tick);
			if(ptsdx[treatment] >= getPtsdCutOff())
				counter->addSprReach(treatment, tick);
		}
		else
		{
			curSPR[treatment] = false;
		}
	}
	else
	{
		curSPR[treatment] = false;
	}
}

double ViolenceAgent::getTrtmentUptakeProbability(int treatment, int type)
{
	double log = 0.0; 
	double p = 0.0;
	
	int male = (sex == Violence::Sex::Male) ? 1 : 0;

	if(initPtsdx >= getPtsdCutOff() && ptsdx[treatment] >= getPtsdCutOff())
	{
		if(type == CBT)
			log = -2.11 + (-0.1941*male) + (-0.5237*black) + (-1.0845*hispanic) + (-0.2653*other) + (-0.1139*age2) + (0.2455*age3) + (1.8377*priorCBT);
		else if(type == SPR)
			log = -1.7778 + (0.00136*male) + (-0.6774*black) + (-0.8136*hispanic) + (-0.3118*other) + (-0.0549*age2) + (0.3746*age3) + (1.4076*priorSPR);
	}
	else if(initPtsdx < getPtsdCutOff() && ptsdx[treatment] != 0)
	{
		if(type == CBT)
			log = -4.1636 + (-0.6422*male) + (-0.8040*black) + (-0.6795*hispanic) + (0.1976*other) + (0.1453*age2) + (-0.4203*age3) + (2.4109*priorCBT);
		else if(type == SPR)
			log = -3.7355 + (-0.5319*male) + (-1.2562*black) + (-0.4632*hispanic) + (0.1427*other) + (0.1703*age2) + (-0.5289*age3) + (2.0696*priorSPR);
	}
	
	p = (log != 0.0) ? exp(log)/(1+exp(log)) : 0.0; 
	return p;
}

bool ViolenceAgent::watchesNews(double pNewsSource)
{
	double randomP = random->uniform_real_dist();
	if(randomP < pNewsSource)
		return true;
	else
		return false;
}

void ViolenceAgent::setAgentIdx(std::string idx)
{
	this->agentIdx = idx;
}

void ViolenceAgent::setAgeCat()
{
	if(age >= 14 && age <= 34)
		ageCat = Violence::AgeCat::Age_14_34;
	else if(age >= 35 && age <= 64)
		ageCat = Violence::AgeCat::Age_35_64;
	else if(age >= 65)
		ageCat = Violence::AgeCat::Age_65_;
	else 
		ageCat = -1;
}

void ViolenceAgent::setAgeCat2()
{
	if(age >= 14 && age <= 17)
		ageCat2 = Violence::AgeCat2::Age_14_17;
	else if(age >= 18 && age <= 29)
		ageCat2 = Violence::AgeCat2::Age_18_29;
	else if(age >= 30 && age <= 49)
		ageCat2 = Violence::AgeCat2::Age_30_49;
	else if(age >= 50 && age <= 64)
		ageCat2 = Violence::AgeCat2::Age_50_64;
	else if(age >= 65)
		ageCat2 = Violence::AgeCat2::Age_65_;
	else
		ageCat2 = -1;
}

void ViolenceAgent::setNewOrigin()
{
	if(origin != ACS::Origin::Hisp && origin != ACS::Origin::WhiteNH && origin != ACS::Origin::BlackNH)
		newOrigin = Violence::Origin::OtherNH;
	else
		newOrigin = origin;
}

//void ViolenceAgent::setPTSDx(MapPair *m_ptsdx, bool isPtsd)
//{
//	if(age >= 14)
//	{
//		std::string ptsd_status = (isPtsd) ? "1" : "0";
//		std::string key_ptsd = std::to_string(sex)+std::to_string(ageCat)+ptsd_status;
//	
//		if(m_ptsdx->count(key_ptsd) > 0)
//		{
//			PairDD pair_ptsdx = m_ptsdx->at(key_ptsd);
//			double ptsdx_ = getPTSDx(pair_ptsdx);
//
//			for(int i = 0; i < NUM_TREATMENT; ++i)
//				this->ptsdx[i] = ptsdx_;
//
//			this->initPtsdx = ptsdx[STEPPED_CARE];
//		}
//		else
//		{
//			std::cout << "Error: Strata " << key_ptsd << " doesn't exist!" << std::endl;
//			exit(EXIT_SUCCESS);
//		}
//	}
//
//}

void ViolenceAgent::setPTSDx(MapTuple *m_ptsdx)
{
	if(age >= 14)
	{
		std::string key_ptsd = std::to_string(sex) + std::to_string(ageCat) + std::to_string(getPtsdType()) 
								+ std::to_string(getPtsdCase());
	
		if(m_ptsdx->count(key_ptsd) > 0)
		{
			Tuple tup_ptsdx = m_ptsdx->at(key_ptsd);
			double ptsdx_ = getPTSDx(tup_ptsdx, key_ptsd);

			for(int i = 0; i < NUM_TREATMENT; ++i)
				this->ptsdx[i] = ptsdx_;

			this->initPtsdx = ptsdx[STEPPED_CARE];
		}
		else
		{
			std::cout << "Error: Strata " << key_ptsd << " doesn't exist!" << std::endl;
			exit(EXIT_SUCCESS);
		}
	}

}

void ViolenceAgent::setPTSDstatus(bool ptsdStatus, int type)
{
switch(type)
	{
	case PRIMARY:
		this->priPTSD = ptsdStatus;
		break;
	case SECONDARY:
		this->secPTSD = ptsdStatus;
		break;
	case TERTIARY:
		this->terPTSD = ptsdStatus;
		break;
	default:
		break;
	}
}

void ViolenceAgent::setPTSDcase(bool ptsd_case)
{
	this->ptsdCase = ptsd_case;
	if(this->ptsdCase)
	{
		counter->addPersonCount("Ptsd" + getAgentType3()); 
	}
}

void ViolenceAgent::setSchoolName(std::string name)
{
	this->schoolName = name;
}

void ViolenceAgent::setFriendSize(int size)
{
	if(size > 0 && age >= 14)
	{
		this->friendSize = size;
		friendList.reserve(size);
	}
	else
	{
		this->friendSize = 0;
		friendList.reserve(0);
	}
}

void ViolenceAgent::setFriend(ViolenceAgent *frnd)
{
	if((int)friendList.size() <= friendSize)
		friendList.push_back(frnd);
}

void ViolenceAgent::setNumberOfTvHours()
{
	//std::vector<std::pair<double, int>> distTvHours(parameters->getViolenceParam()->p_tv_time);
	double pHoursNews = 0;
	double randP = random->uniform_real_dist();

	for(auto vec : parameters->getViolenceParam()->p_tv_time)
	{
		pHoursNews += vec.first;
		int hoursWatched = vec.second;

		if(randP < pHoursNews)
		{
			this->hoursWatched = hoursWatched;
			break;
		}
	}

	//counter->addPersonCount(getAgentType1());
	counter->addPersonCount(getAgentType2());
}

void ViolenceAgent::setNumberofTvHours(int numHours)
{
	this->hoursWatched = numHours;
	counter->addPersonCount(getAgentType2());
}

void ViolenceAgent::setSocialMediaHours()
{
	double pSocialMediaHours = 0;
	double randP = random->uniform_real_dist();

	for(auto vec : parameters->getViolenceParam()->p_social_media_hours)
	{
		pSocialMediaHours += vec.first;
		int socialMedialHrs = vec.second;

		if(randP < pSocialMediaHours)
		{
			this->socialMediaHours = socialMedialHrs;
			break;
		}
	}

	counter->addPersonCount(getAgentType4());
}

void ViolenceAgent::setSocialMediaHours(int numHours)
{
	this->socialMediaHours = numHours;
	counter->addPersonCount(getAgentType4());
}

void ViolenceAgent::setNewsSource(VecPairs *vMediaDist)
{
	int mediaType;
	double pMediaDist = 0;
	
	double randomP = random->uniform_real_dist();
	for(auto it = vMediaDist->begin(); it != vMediaDist->end(); ++it)
	{
		pMediaDist += it->first;
		mediaType = it->second;

		if(randomP < pMediaDist)
		{
			this->newsSource = mediaType;
			break;
		}
	}

	if(this->newsSource == Violence::Source::TV)
		setNumberOfTvHours();

	if(this->newsSource == Violence::Source::SocialMedia)
		setSocialMediaHours();
	
	counter->addPersonCount(getAgentType3());
	counter->addPersonCount(getAgentType1());
}

void ViolenceAgent::setDummyVariables()
{
	age1 = age2 = age3 = 0;
	if(ageCat == Violence::AgeCat::Age_14_34)
		age1 = 1;
	else if(ageCat == Violence::AgeCat::Age_35_64)
		age2 = 1;
	else if(ageCat == Violence::AgeCat::Age_65_)
		age3 = 1;

	white = black = hispanic = other = 0;
	if(newOrigin == Violence::Origin::WhiteNH)
		white = 1;
	else if(newOrigin == Violence::Origin::BlackNH)
		black = 1;
	else if(newOrigin == Violence::Origin::Hisp)
		hispanic = 1;
	else if(newOrigin == Violence::Origin::OtherNH)
		other = 1;
}

std::string ViolenceAgent::getAgentIdx() const
{
	return agentIdx;
}

short int ViolenceAgent::getAgeCat2() const
{
	return ageCat2;
}

double ViolenceAgent::getInitPTSDx() const
{
	return initPtsdx;
}

double ViolenceAgent::getPTSDx(int treatment) const
{
	switch(treatment)
	{
	case STEPPED_CARE:
		return ptsdx[STEPPED_CARE];
		break;
	case USUAL_CARE:
		return ptsdx[USUAL_CARE];
		break;
	default:
		return -1;
		break;
	}
}

//double ViolenceAgent::getPTSDx(PairDD pair_ptsdx) const
//{
//	bool ptsd_status = (priPTSD || secPTSD || terPTSD) ? true : false;
//	double ptsdx_ = random->normal_dist(pair_ptsdx.first, pair_ptsdx.second);
//
//	double sec_coeff = parameters->getViolenceParam()->sec_coeff;
//	double ter_coeff = parameters->getViolenceParam()->ter_coeff;
//	
//	if(secPTSD)
//		ptsdx_ = sec_coeff*ptsdx_;
//	else if(terPTSD)
//		ptsdx_ = ter_coeff*ptsdx_;
//
//	//bounds checking for ptsd symptoms
//	if(ptsdx_ >= getPtsdCutOff())
//	{
//		if(ptsdx_ >  MAX_PTSDX)
//			ptsdx_ = MAX_PTSDX;
//
//		if(!ptsd_status)
//			ptsdx_ = getPtsdCutOff()-1;
//	}
//	else
//	{
//		if(ptsdx_ < MIN_PTSDX)
//			ptsdx_ = MIN_PTSDX;
//
//		if(ptsd_status)
//		{
//			int count = 0;
//			while(true && count < 10)
//			{
//				ptsdx_ = random->normal_dist(pair_ptsdx.first, pair_ptsdx.second);
//				if(ptsdx_ > MAX_PTSDX)
//					ptsdx_ = MAX_PTSDX;
//				
//				if(secPTSD)
//					ptsdx_ = sec_coeff*ptsdx_;
//				else if(terPTSD)
//					ptsdx_ = ter_coeff*ptsdx_;
//
//				if(ptsdx_ >= getPtsdCutOff())
//					break;
//
//				count++;
//				if(count >= 10 && ptsdx_ < getPtsdCutOff())
//					ptsdx_ = getPtsdCutOff();
//			}
//		}
//	}
//	
//	return ptsdx_;
//}

double ViolenceAgent::getPTSDx(Tuple tup_ptsdx, std::string key) const
{
	double randP = random->uniform_real_dist();
	double ptsdx_ = -1;
	if (randP < FIRST_Q)
	{
		ptsdx_ = std::get<0>(tup_ptsdx);
		counter->addPersonCount(key, "1Q");
	}
	else if(randP < SECOND_Q)
	{
		ptsdx_ = std::get<1>(tup_ptsdx);
		counter->addPersonCount(key, "2Q");
	}
	else if(randP < THIRD_Q)
	{
		ptsdx_ = std::get<2>(tup_ptsdx);
		counter->addPersonCount(key, "3Q");
	}
	else if(randP < FOURTH_Q)
	{
		ptsdx_ = std::get<3>(tup_ptsdx);
		counter->addPersonCount(key, "4Q");
	}

	return ptsdx_;
}

double ViolenceAgent::getPtsdCutOff() const
{
	return parameters->getViolenceParam()->ptsd_cutoff;
}

bool ViolenceAgent::getPTSDstatus(int type) const
{
	switch(type)
	{
	case PRIMARY:
		return priPTSD;
		break;
	case SECONDARY:
		return secPTSD;
		break;
	case TERTIARY:
		return terPTSD;
		break;
	default:
		return false;
		break;
	}
}

bool ViolenceAgent::getCBTReferred() const
{
	return cbtReferred;
}

bool ViolenceAgent::getSPRReferred() const
{
	return sprReferred;
}

short int ViolenceAgent::getCBTsessions(int treatment) const
{
	return sessionsCBT[treatment];
}

short int ViolenceAgent::getSPRsessions(int treatment) const
{
	return sessionsSPR[treatment];
}

int ViolenceAgent::getPtsdType() const
{
	int ptsd_type = -1;

	if(priPTSD)
		ptsd_type = PRIMARY;
	else if(secPTSD)
		ptsd_type = SECONDARY;
	else if(terPTSD)
		ptsd_type = TERTIARY;

	return ptsd_type;
}

int ViolenceAgent::getPtsdCase() const
{
	if(ptsdCase)
		return PTSD_CASE;
	else
		return PTSD_NON_CASE;
}

int ViolenceAgent::getDurationMild(int treatment) const
{
	return durMild[treatment];
}

int ViolenceAgent::getDurationModerate(int treatment) const
{
	return durMod[treatment];
}

int ViolenceAgent::getDurationSevere(int treatment) const
{
	return durSevere[treatment];
}

int ViolenceAgent::getResolvedTime(int treatment) const
{
	return resolvedTime[treatment];
}

int ViolenceAgent::getNumberOfTvHours() const
{
	return hoursWatched;
}

int ViolenceAgent::getSocialMediaHours() const
{
	return socialMediaHours;
}

int ViolenceAgent::getNewsSource() const
{
	return newsSource;
}

std::string ViolenceAgent::getSchoolName() const
{
	return schoolName;
}

ViolenceAgent::FriendsPtr * ViolenceAgent::getFriendList() 
{
	return &friendList;
}

int ViolenceAgent::getFriendSize() const
{
	return friendSize;
}

int ViolenceAgent::getTotalFriends() const
{
	return friendList.size();
}

int ViolenceAgent::getMaxCbtTime() const
{
	int screening_time = parameters->getViolenceParam()->screening_time;
	if(initPtsdx >= getPtsdCutOff())
		return parameters->getViolenceParam()->treatment_time;
	else
		return screening_time+parameters->getViolenceParam()->cbt_dur_non_cases;

}

int ViolenceAgent::getMaxSprTime() const
{
	return parameters->getViolenceParam()->treatment_time;
}

//return agent type by age category2
std::string ViolenceAgent::getAgentType1() const
{
	return "AgeCat2" + std::to_string(ageCat2);
}

//returns agent type by age category2 and hours of tv watched
std::string ViolenceAgent::getAgentType2() const
{
	return "HoursWatched" + std::to_string(ageCat2) + std::to_string(hoursWatched);
}

//return agent type by age category 2 and the source of the news
std::string ViolenceAgent::getAgentType3() const
{
	return "SourceNews" + std::to_string(ageCat2) + std::to_string(newsSource);
}

//return agent type by age cat2 and the social media hours
std::string ViolenceAgent::getAgentType4() const
{
	return "SocialMediaHours" + std::to_string(ageCat2) + std::to_string(socialMediaHours);
}

bool ViolenceAgent::isStudent() const
{
	if(age >=14 && age <= 18 && (education == ACS::Education::_9th_To_12th_Grade))
		return true;
	else
		return false;
}

bool ViolenceAgent::isTeacher() const
{
	if(age >= 25 && age < 65 && education >= ACS::Education::Bachelors_Degree)
		return true;
	else
		return false;
}

bool ViolenceAgent::isCompatible(const ViolenceAgent *b) const
{
	bool valid = true;
	if(this->householdID == b->householdID || this->agentIdx == b->agentIdx || b->getTotalFriends() == b->friendSize || this->isFriend(b))
		valid = false;

	return valid;
}

bool ViolenceAgent::isFriend(const ViolenceAgent *b) const
{
	bool isFrnd = false;
	for(auto ff = friendList.begin(); ff != friendList.end(); ++ff)
	{
		ViolenceAgent *ref = *ff;
		if(ref->getAgentIdx() == b->getAgentIdx())
		{
			isFrnd = true;
			break;
		}
	}

	return isFrnd;
}

bool ViolenceAgent::isPrimaryRisk(const MapBool *primaryRiskPool) const
{
	if(primaryRiskPool->count(agentIdx) > 0)
		return true;
	else
		return false;
}

bool ViolenceAgent::isSecondaryRisk(const MapBool *secondRiskPool) const
{
	if(secondRiskPool->count(agentIdx) > 0)
		return true;
	else
		return false;
}


