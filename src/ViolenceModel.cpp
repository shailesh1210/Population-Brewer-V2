#include "ViolenceModel.h"
#include "PersonPums.h"
#include "HouseholdPums.h"
#include "ViolenceParams.h"
#include "ViolenceCounter.h"
#include "Area.h"
#include "County.h"
#include "ACS.h"
#include "Random.h"
#include "ElapsedTime.h"

ViolenceModel::ViolenceModel() : schoolName("Stoneman HS")
{
	
}

ViolenceModel::ViolenceModel(const char *inDir, const char *outDir, int simType, int geoLvl)
	:PopBrewer(new ViolenceParams(inDir, outDir, simType, geoLvl)), schoolName("Stoneman HS")
{
	import();
}

ViolenceModel::~ViolenceModel()
{
	delete count;
	delete random;
}

void ViolenceModel::start(std::string modelNumber)
{
	if(parameters->getGeoType() == Geography::Level::STATE)
	{
		std::cout << "Error: Cannot Run Violence Model in State Level!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	if(parameters != NULL)
	{
		count = new ViolenceCounter(parameters);
		random = new Random;
	}
	else
	{
		std::cout << "Error: Parameters are not initialized!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	int num_trials = parameters->getViolenceParam()->num_trials;
	int sim_case = parameters->getViolenceParam()->sim_case;

	for(int i = 0; i < num_trials; ++i)
	{
		std::cout << "Simulation no: " << i+1 << "," << sim_case << std::endl;

		createPopulation();
		if(modelNumber == "Model1")
		{
			groupAffectedAgents(modelNumber);
			distributePtsdStatus();
			runModel();
		}
		else if(modelNumber == "Model2")
		{
			AgentListMap m_tvWatchersPreIntervention, m_tvWatchersPostIntervention, m_socialMediaUsersPre, m_socialMediaUsersPost;

			groupAffectedAgents(modelNumber); 
			assignNewsSource(&m_tvWatchersPreIntervention, &m_socialMediaUsersPre);

			for(auto scenario : Violence::Scenario::_values())
			{
				std::cout << "\nScenario : " << scenario._to_string() << ".." << std::endl;
				std::string s_preIntervention = preIntervention();

				distributePtsdStatus(&m_tvWatchersPreIntervention, &m_socialMediaUsersPre, s_preIntervention, scenario); 
				resetPtsdStatus(&m_tvWatchersPreIntervention, &m_socialMediaUsersPre);

				if(scenario == (int)Violence::Scenario::First)
				{
					processTvWatchers(&m_tvWatchersPreIntervention, &m_tvWatchersPostIntervention, scenario);
					processSocialMediaUsers(&m_socialMediaUsersPre, &m_socialMediaUsersPost, scenario);
				}
			}
		}
		
		clearList();
	}

	if(parameters->writeToFile())
		count->output("miami", modelNumber);
}

void ViolenceModel::addHousehold(const HouseholdPums<ViolenceParams> *hh, int countHH)
{
	if(pumaHouseholds.size() == 0)
		exit(EXIT_SUCCESS);

	int puma_code = hh->getPUMA();
	std::vector<PersonPums<ViolenceParams>> tempPersons;
	int countPersons;

	if(hh->getHouseholdType() >= ACS::HHType::MarriedFam)
	{
		Household tempHH;
		tempHH.reserve(hh->getHouseholdSize());

		tempPersons = hh->getPersons();
		countPersons = 0;
		for(auto pp = tempPersons.begin(); pp != tempPersons.end(); ++pp)
		{
			ViolenceAgent *agent = new ViolenceAgent(parameters, &(*pp), random, count, countHH, countPersons);
			
			agent->setFriendSize(random->poisson_dist(getMeanFriendSize()));
			agent->setPTSDcase(false);

			tempHH.push_back(*agent);

			countPersons++;
			delete agent;
		}

		if(pumaHouseholds.count(puma_code) > 0)
			pumaHouseholds[puma_code].push_back(tempHH);
	}
}

void ViolenceModel::addAgent(const PersonPums<ViolenceParams> *p)
{
	//do nothing here
}

void ViolenceModel::createPopulation()
{
	Area<ViolenceParams> *metro = &m_geoAreas.at("33100");
	//Area<ViolenceParams> *metro = &m_geoAreas.at("10180");
	
	initializeHouseholdMap(PARKLAND);
	//initializeHouseholdMap(TAYLOR);

	std::cout << std::endl;
	std::cout << "Creating Population for " << metro->getAreaName() << std::endl;
	metro->createAgents(this);

	createSchool(&pumaHouseholds[PARKLAND]);
	//createSchool(&pumaHouseholds[TAYLOR]);
}

void ViolenceModel::groupAffectedAgents(std::string modelNum)
{
	//pool of primarily exposed agents during shooting (Students and teachers)
	poolPrimaryRiskAgents(&studentsMap, &affectedStudents, STUDENT); 
	poolPrimaryRiskAgents(&teachersMap, &affectedTeachers, TEACHER);

	//pool of secondarily exposed agents - friends and families of primary
	poolSecondaryRiskAgents();

	//pool of community members indirectly affected by shooting
	poolTertiaryRiskAgents(modelNum);

}

void ViolenceModel::distributePtsdStatus()
{
	distPrimaryPtsd();
	distSecondaryPtsd();
	distTertiaryPtsd();
}


void ViolenceModel::distributePtsdStatus(AgentListMap *m_tvWatchers, AgentListMap *m_socialMediaUsers, std::string period, int scenario)
{
	distributePtsdStatus(m_tvWatchers, period, scenario, Violence::Source::TV);
	distributePtsdStatus(m_socialMediaUsers, period, scenario, Violence::Source::SocialMedia);
}


void ViolenceModel::distributePtsdStatus(AgentListMap *m_agents, std::string period, int scenario, int source)
{
	MapDbl m_prevalence;
	if(source == (int)Violence::Source::TV)
	{
		m_prevalence = parameters->getViolenceParam()->ptsd_prev_tv_time;
	}
	else if(source == (int)Violence::Source::SocialMedia)
	{
		m_prevalence = parameters->getViolenceParam()->ptsd_prev_social_media;
	}

	setPtsdStatus(m_agents, m_prevalence, source, scenario, period);
	//resetPtsdStatus(m_agents, source);
}

void ViolenceModel::runModel()
{
	std::cout << std::endl;
	std::cout << "Running mass violence model..." << std::endl;
	int curTick = 0;
	int countPersons;
	
	VecDbls prevalence;
	AgentListPtr tempAgents;
	std::vector<Household>*households;
	
	for(auto map = pumaHouseholds.begin(); map != pumaHouseholds.end(); ++map)
	{
		households = &map->second;
		//count->reset();
		while(curTick < getMaxWeeks())
		{
			countPersons = 0;
			for(auto hh = households->begin(); hh != households->end(); ++hh)
			{
				for(auto pp = hh->begin(); pp != hh->end(); ++pp)
				{
					ViolenceAgent *agent = (&(*pp));
					if(agent->getAge() < getMinAge())
						continue;

					agent->excecuteRules(curTick);
					
					if(curTick == getMaxWeeks()-1)
						tempAgents.push_back(agent);

					countPersons++;
				}
			}

			prevalence = count->getPrevalence(curTick, countPersons);
			count->computeOutcomes(curTick, countPersons);

			curTick++;
		
			std::cout << std::fixed;
			std::cout << "Tick: " << curTick  << std::setprecision(4) 
				<< ", Pr(SC): " << prevalence[STEPPED_CARE] <<", Pr(UC): " << prevalence[USUAL_CARE] << ", Pr(NT): " << prevalence[NO_TREATMENT]
				<< " | " << "CBT: " << count->getCbtUptake(curTick-1) << "," << "SPR: " << count->getSprUptake(curTick-1) 
				<< "," << "ND: " << count->getNaturalDecayUptake(curTick-1) << std::endl;
		}

		count->computeCostEffectiveness(&tempAgents);
	}

}

/**
*	@brief Creates a population for High School based on its demographics. Households are
*	added to the list if they have agents who are less than 18 years old and attends high school
*	(9th to 12th grade).
*	@param households is vector containing list of households in designated PUMA area
*	@return void
*/
void ViolenceModel::createSchool(std::vector<Household>*households)
{
	MapInt schoolDemoMap = parameters->getSchoolDemographics();
	resizeHouseholds();

	std::random_shuffle(households->begin(), households->end());

	std::string student_type;
	int students_per_hh, num_teachers;
	num_teachers = getNumTeachers();

	int countPersons = 0;
	for(auto hh = households->begin(); hh != households->end(); ++hh)
	{
		students_per_hh = getNumStudents(*hh, countPersons);
		if(students_per_hh > 0)
		{
			std::vector<ViolenceAgent>prosp_students;
			for(auto pp = hh->begin(); pp != hh->end(); ++pp)
			{
				if(pp->isStudent())
				{
					student_type =std::to_string(pp->getSex())+std::to_string(pp->getOrigin());
					//m_students.insert(std::make_pair(student_type, *pp));
					if(schoolDemoMap.count(student_type) > 0)
					{
						int student_count = schoolDemoMap.at(student_type);
						if(student_count > 0)
						{
							prosp_students.push_back(*pp);
							student_count--;
							schoolDemoMap.at(student_type) = student_count;
						}
					}
				}
			}

			if(students_per_hh == prosp_students.size())
			{
				for(auto pp = hh->begin(); pp != hh->end(); ++pp)
				{
					if(pp->getAge() < getMinAge())
						continue;

					if(pp->isStudent())
					{
						pp->setSchoolName(schoolName);
						createAgentHashMap(&studentsMap, &(*pp), IN_SCHOOL_NETWORK);
					}
					else
					{
						createAgentHashMap(&othersMap, &(*pp), OUT_SCHOOL_NETWORK);
					}
				}
				schoolHouseholds.push_back(&(*hh));
			}
			else
			{
				for(auto pp = hh->begin(); pp != hh->end(); ++pp)
				{
					if(pp->getAge() >= getMinAge())
					{
						createAgentHashMap(&othersMap, &(*pp), OUT_SCHOOL_NETWORK);
					}
				}

				for(auto pp = prosp_students.begin(); pp != prosp_students.end(); ++pp)
				{
					student_type = std::to_string(pp->getSex())+std::to_string(pp->getOrigin());
					schoolDemoMap.at(student_type)++;
				}
			}
		}
		else
		{
			if(num_teachers == 0)
			{
				for(auto pp = hh->begin(); pp != hh->end(); ++pp)
				{
					if(pp->getAge() >= getMinAge())
					{
						createAgentHashMap(&othersMap, &(*pp), OUT_SCHOOL_NETWORK);
					}
				}
			}
			else
			{
				std::random_shuffle(hh->begin(), hh->end());
				int count_teacher = 0;
				for(auto pp = hh->begin(); pp != hh->end(); ++pp)
				{
					if(pp->getAge() < getMinAge())
						continue;

					if(pp->isTeacher() && count_teacher == 0)
					{
						pp->setSchoolName(schoolName);
						createAgentHashMap(&teachersMap, &(*pp), IN_SCHOOL_NETWORK);
						count_teacher++;
						//num_teachers--;
					}
					else
					{
						createAgentHashMap(&othersMap, &(*pp), OUT_SCHOOL_NETWORK);
					}
				}

				if(count_teacher > 0 && num_teachers > 0){
					schoolHouseholds.push_back(&(*hh));
					num_teachers--;
				}
			}
		}
	}

	schoolDemographics();
	createSocialNetwork(households, countPersons);
}

void ViolenceModel::createAgentHashMap(AgentListMap *agentsMap, ViolenceAgent *a, int network_type)
{
	std::string key = "";
	if(network_type == OUT_SCHOOL_NETWORK)
		key = std::to_string(a->getOrigin())+std::to_string(a->getEducation());
	else if(network_type == IN_SCHOOL_NETWORK)
		key = std::to_string(a->getOrigin());

	std::vector<ViolenceAgent*>tempAgentPtr;
	if(agentsMap->count(key) == 0)
	{
		tempAgentPtr.push_back(a);
		agentsMap->insert(std::make_pair(key, tempAgentPtr));
	}
	else
	{
		agentsMap->at(key).push_back(a);
	}
}


/**
*	@brief Creates a social network of friends and families for agents who are 14 years or older.
*	Matching is done based on age, race, gender and education of agents. Each agent is assigned a 
*	predefined friends size from a poisson distribution with a mean friend size of 3.0. Social network
*	is built for students, teachers and other agents. Students can have friends from same/different schools. 
*	Teachers can have friends from school or other agents. 
*	@param househoulds is vector containing list of households
*	@param totalPersons is total number of agents who are 14 years or older
*	@return void
*/
void ViolenceModel::createSocialNetwork(std::vector<Household>*households, int totalPersons)
{
	std::cout << std::endl;
	std::cout << "Building Social Network...\n" << std::endl;
	
	int count, draws;
	double pSelection, waitTime;
	count = 0;
	waitTime = 2000; //milliseconds

	ElapsedTime timer;
	for(auto hh = households->begin(); hh != households->end(); ++hh)
	{
		for(auto pp = hh->begin(); pp != hh->end(); ++pp)
		{
			ViolenceAgent *a = (&(*pp));
			if(a->getAge() < getMinAge())
				continue;

			draws = 0;
			while(a->getTotalFriends() < a->getFriendSize() && draws < getOuterDraws())
			{
				draws++;
				if(a->getSchoolName() == schoolName)
				{
					if(a->isStudent())
					{
						pSelection = random->uniform_real_dist();
						if(pSelection > getPval(STUDENT))
							findFriends(&studentsMap, a, IN_SCHOOL_NETWORK);
						else
							findFriends(&othersMap, a, OUT_SCHOOL_NETWORK);
					}
					else if(a->isTeacher())
					{
						pSelection = random->uniform_real_dist();
						if(pSelection > getPval(TEACHER))
							findFriends(&teachersMap, a, IN_SCHOOL_NETWORK);
						else
							findFriends(&othersMap, a, OUT_SCHOOL_NETWORK);
					}
				}
				else
				{
					findFriends(&othersMap, a, OUT_SCHOOL_NETWORK);
				}
			}

			timer.stop();
			if(timer.elapsed_ms() > waitTime)
			{
				std::cout << "Social network building progress: " << 100*(double)count/totalPersons << "% completed!" << std::endl;
				timer.start();
			}

			++count;
		}
	}

	std::cout << "Social network building progress: " << 100*(double)count/totalPersons << "% completed!\n" << std::endl;
	
	std::cout << "Total Pop (14 or older): " << totalPersons << std::endl;
	std::cout << std::endl;
	//networkAnalysis();
}


void ViolenceModel::findFriends(AgentListMap *agentsMap, ViolenceAgent *a, int network_type)
{
	bool ageMatch, genderMatch;
	double ageChoice, genderChoice;
	int origin, edu;
	std::string new_key, old_key;

	new_key = old_key = "";

	ViolenceAgent *b = NULL;

	bool match_found = false;
	int inner_draws = 0;
	while(!match_found)
	{
		inner_draws++;
		if(inner_draws >= getInnerDraws())
			break;

		origin = getOriginKey(a);
		edu = getEducationKey(a);

		if(origin < 0 || edu < 0)
			exit(EXIT_SUCCESS);
		
		if(network_type == IN_SCHOOL_NETWORK)
			new_key = std::to_string(origin);
		else if(network_type == OUT_SCHOOL_NETWORK)
			new_key = std::to_string(origin)+std::to_string(edu);

		if(old_key == new_key && !old_key.empty() && !new_key.empty())
			break;

		if(agentsMap->count(new_key) > 0)
		{
			std::random_shuffle(agentsMap->at(new_key).begin(), agentsMap->at(new_key).end());
			for(size_t i = 0; i < agentsMap->at(new_key).size(); ++i)
			{
				b = agentsMap->at(new_key).at(i);
				if(!(a->isCompatible(b)))
					continue;
			
				ageMatch = genderMatch = true;
				if(a->isStudent() || b->isStudent())
				{
					if(abs(a->getAge()-b->getAge()) > getAgeDiffStudents())
						ageMatch = false;
				}
				else
				{
					ageChoice = random->uniform_real_dist();
					if(ageChoice > getPval(AGE))
					{
						if(abs(a->getAge()-b->getAge()) > getAgeDiffOthers())
							ageMatch = false;
					}
				}

				genderChoice = random->uniform_real_dist();
				if(genderChoice > getPval(GENDER))
				{
					if(a->getSex() != b->getSex())
						genderMatch = false;
				}

				if(ageMatch && genderMatch)
				{
					a->setFriend(b);
					b->setFriend(a);

					match_found = true;
					break;
				}
			}

			old_key = new_key;
		}
	}
}


/**
*	@brief Draws directly affected agents (students and teachers) from list of students/teachers 
*	within school based on prevalence of affected agents and assigns PTSD status agents.
*	@param none
*	@return void
*/
void ViolenceModel::distPrimaryPtsd()
{
	std::cout << "Distributing primary PTSD status!" << std::endl; 
	setPtsdStatus(&affectedStudents, 0, getPrevalence(PREVAL_STUDENTS), PRIMARY);
	
	//poolPrimaryRiskAgents(&teachersMap, &aff_teachers, TEACHER);
	for(auto sex : ACS::Sex::_values())
	{
		int a_sex = sex._to_integral();
		double preval =  (a_sex == ACS::Sex::Male) ? getPrevalence(PREVAL_TEACHERS_MALE) :
			getPrevalence(PREVAL_TEACHERS_FEMALE);

		setPtsdStatus(&affectedTeachers, sex, preval, PRIMARY);
	}

	//primary += primaryRiskPool.size();

	std::cout << "At primary risk: " << primaryRiskPool.size() << std::endl;
	std::cout << "Distribution complete!\n" << std::endl;
}

/**
*	@brief Creates a pool of friends and families of agents affected by shooting and assigns	
*	secondary PTSD status to agents based on the prevalence of PTSD.
*	@param none
*	@return void
*/
void ViolenceModel::distSecondaryPtsd()
{
	std::cout << "Distributing Secondary PTSD status!" << std::endl;
	
	//assign secondary PTSD status to close friends and families of directly affected agents
	for(auto sex : ACS::Sex::_values())
	{
		int a_sex = sex._to_integral();
		double preval =  (a_sex == ACS::Sex::Male) ? getPrevalence(PREVAL_FAM_MALE) :
			getPrevalence(PREVAL_FAM_FEMALE);

		setPtsdStatus(&affectedFamFriends, sex, preval, SECONDARY);
	}

	//secondary += secondaryRiskPool.size();
	std::cout << "At secondary risk: " << secondaryRiskPool.size() << std::endl;
	std::cout << "Distribution complete!\n" << std::endl;
}

void ViolenceModel::distTertiaryPtsd()
{
	std::cout << "Distributing Tertiary PTSD status!" << std::endl;

	for(auto sex : ACS::Sex::_values())
	{
		int a_sex = sex._to_integral();
		double preval =  (a_sex == ACS::Sex::Male) ? getPrevalence(PREVAL_COMM_MALE) :
			getPrevalence(PREVAL_COMM_FEMALE);

		setPtsdStatus(&communityMembers, sex, preval, TERTIARY);
	}

	//tertiary += tertiaryRiskPool.size();

	std::cout << "At Tertiary risk: " << tertiaryRiskPool.size() << std::endl;
	std::cout << "Distribution complete!\n" << std::endl;
}



void ViolenceModel::assignNewsSource(AgentListMap *m_tvWatchers, AgentListMap *m_socialMediaUsers)
{
	AgentListPtr tempAgents;

	for(auto time : Violence::Coverage::_values())
		m_tvWatchers->insert(std::make_pair(std::to_string(time), tempAgents));

	for(auto time : Violence::SocialMediaHours::_values())
		m_socialMediaUsers->insert(std::make_pair(std::to_string(time), tempAgents));

	VecPairs vMediaDist;
	AgentListPtr *agentList = NULL;

	for(auto map = communityMembers.begin(); map != communityMembers.end(); ++map)
	{
		std::string s_age_cat = map->first;
		if(s_age_cat == std::to_string(Violence::AgeCat2::Age_14_17))
			continue;

		agentList = &(map->second);
		int totPop = agentList->size();

		std::cout << "Assigning news source for age category " << s_age_cat << "..." << std::endl;
		
		if(totPop > 0)
		{
			int ageCategory = std::stoi(s_age_cat);
			vMediaDist = parameters->getViolenceParam()->news_source_dist.at(ageCategory);
			
			for(auto agent = agentList->begin(); agent != agentList->end(); ++agent)
			{
				ViolenceAgent *affectedAgent = *agent;
				affectedAgent->setNewsSource(&vMediaDist);

				if(affectedAgent->getNewsSource() == Violence::Source::TV)
				{
					std::string s_hoursWatched = std::to_string(affectedAgent->getNumberOfTvHours());
					m_tvWatchers->at(s_hoursWatched).push_back(affectedAgent);
				}
				else if(affectedAgent->getNewsSource() == Violence::Source::SocialMedia)
				{
					std::string s_socialMediaHours = std::to_string(affectedAgent->getSocialMediaHours());
					m_socialMediaUsers->at(s_socialMediaHours).push_back(affectedAgent);
				}
			}
		}
	}

	displayMediaDist();
}

void ViolenceModel::processTvWatchers(AgentListMap *m_tvWatchersPre, AgentListMap *m_tvWatchersPost, int scenario)
{
	int source = Violence::Source::TV;
	std::string s_analysis, s_postIntervention;
	for(auto analysis : Violence::SensitivityAnalysisTV::_values())
	{
		s_analysis = Violence::SensitivityAnalysisTV::_from_integral(analysis)._to_string();
		s_postIntervention = postIntervention() + " " + s_analysis;

		changeMediaHours(m_tvWatchersPre, m_tvWatchersPost, analysis, source);
		distributePtsdStatus(m_tvWatchersPost, s_postIntervention, scenario, source);
		resetAttributes(m_tvWatchersPre, source);
	}
}

void ViolenceModel::processSocialMediaUsers(AgentListMap *m_socialMediaUsersPre, AgentListMap *m_socialMediaUsersPost, int scenario)
{
	int source = Violence::Source::SocialMedia;
	std::string s_analysis, s_postIntervention;

	for(auto analysis : Violence::SensitivityAnalysisSocialMedia::_values())
	{
		s_analysis = Violence::SensitivityAnalysisSocialMedia::_from_integral(analysis)._to_string();
		s_postIntervention = postIntervention() + " " + s_analysis;

		changeMediaHours(m_socialMediaUsersPre, m_socialMediaUsersPost, analysis, source);
		distributePtsdStatus(m_socialMediaUsersPost, s_postIntervention, scenario, source);
		resetAttributes(m_socialMediaUsersPre, source);
	}
}

void ViolenceModel::changeMediaHours(AgentListMap * m_agentsPre, AgentListMap *m_agentsPost, int analysis, int source)
{
	std::cout << "\n\Changing media hours for " <<  Violence::Source::_from_integral(source)._to_string() << "..." << std::endl;
	
	if(source == (int)Violence::Source::TV)
	{
		std::cout << "\t\nAnalysis type: " << Violence::SensitivityAnalysisTV::_from_integral(analysis)._to_string() << std::endl << std::endl;
	}
	else if(source == (int)Violence::Source::SocialMedia)
	{
		std::cout << "\t\nAnalysis type: " << Violence::SensitivityAnalysisSocialMedia::_from_integral(analysis)._to_string() << std::endl << std::endl;
	}

	m_agentsPost->clear();
	mergeAgentList(m_agentsPre, m_agentsPost, analysis, source);

	for(auto map = m_agentsPost->begin(); map != m_agentsPost->end(); ++map)
	{
		int hours = std::stoi(map->first);
		for(auto agent = map->second.begin(); agent != map->second.end(); ++agent)
		{
			ViolenceAgent *a = *agent;
			if(source == (int)Violence::Source::TV)
			{
				a->setNumberofTvHours(hours);
			}
			else if(source == (int)Violence::Source::SocialMedia)
			{
				a->setSocialMediaHours(hours);
			}
		}
	}

	std::cout << "Change complete!" << std::endl;
	displayMediaDist();
}

void ViolenceModel::mergeAgentList(AgentListMap * m_agentsPre, AgentListMap *m_agentsPost, int analysis, int source)
{
	AgentListPtr tempAgents;
	if(source == (int)Violence::Source::TV)
	{
		for(auto hours : Violence::Coverage::_values())
			m_agentsPost->insert(std::make_pair(std::to_string(hours), tempAgents));
	}
	else if(source == (int)Violence::Source::SocialMedia)
	{
		for(auto hours : Violence::SocialMediaHours::_values())
			m_agentsPost->insert(std::make_pair(std::to_string(hours), tempAgents));
	}

	int numHoursPre, numHoursPost;
	std::string s_hours;

	for(auto pre = m_agentsPre->begin(); pre != m_agentsPre->end(); ++pre)
	{
		numHoursPre = std::stoi(pre->first);
		if(source == (int)Violence::Source::TV)
		{
			if(analysis == (int)Violence::SensitivityAnalysisTV::TV_Hours_Less4)
			{
				numHoursPost = (int)Violence::Coverage::Hours_Less_4;
				if(numHoursPre >= numHoursPost)
				{
					s_hours = std::to_string(numHoursPost);
				}
			}
			else if(analysis == (int)Violence::SensitivityAnalysisTV::TV_Hours_8_12)
			{
				numHoursPost = (int)Violence::Coverage::Hours_8_11;
				if(numHoursPre < numHoursPost)
				{
					s_hours = std::to_string(numHoursPre+2);
				}
				else
				{
					s_hours = std::to_string(numHoursPre);
				}

			}
			else if(analysis == (int)Violence::SensitivityAnalysisTV::TV_Hours_0_7)
			{
				numHoursPost = (int)Violence::Coverage::Hours_4_7;
				if(numHoursPre > numHoursPost)
				{
					s_hours = std::to_string(numHoursPre-2);
				}
				else
				{
					s_hours = std::to_string(numHoursPre);
				}
			}
		}

		if(source == (int)Violence::Source::SocialMedia)
		{
			if(analysis == (int)Violence::SensitivityAnalysisSocialMedia::No_SocialMedia)
			{
				numHoursPost = (int)Violence::SocialMediaHours::None;
				s_hours = std::to_string(numHoursPost);
			}
			else if(analysis == (int)Violence::SensitivityAnalysisSocialMedia::SocialMediaHours_Less_2)
			{
				numHoursPost = (int)Violence::SocialMediaHours::Hours_2_Less;
				s_hours = std::to_string(numHoursPost);
			}
			else if(analysis == (int)Violence::SensitivityAnalysisSocialMedia::SocialMediaHours_More_2)
			{
				numHoursPost = (int)Violence::SocialMediaHours::Hours_2_More;
				s_hours = std::to_string(numHoursPost);
			}
		}

		if(!s_hours.empty())
		{
			tempAgents = m_agentsPost->at(s_hours);
			tempAgents.insert(tempAgents.end(), pre->second.begin(), pre->second.end());

			m_agentsPost->at(s_hours) = tempAgents;
		}
	}
}

void ViolenceModel::poolPrimaryRiskAgents(AgentListMap *agentsMap, AgentListMap *aff_agents, int agent_type)
{
	VecInts origins;
	for(auto map = agentsMap->begin(); map != agentsMap->end(); ++map)
		origins.push_back(std::stoi(map->first));

	int affected_count = 0;
	if(agent_type == STUDENT)
		affected_count = getAffectedStudents();
	else if(agent_type == TEACHER)
		affected_count = getAffectedTeachers();

	if(affected_count <= 0)
	{
		std::cout << "Error: Directly affected agents cannot be less than or equal to zero!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	int totalSize = 0;
	for(auto it = agentsMap->begin(); it != agentsMap->end(); ++it)
		totalSize += it->second.size();

	if(totalSize < affected_count)
	{
		std::cout << "Error: Insufficient agents in the container (total agents < affected agents)!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	while(affected_count > 0)
	{
		std::string rand_origin = std::to_string(origins[random->random_int(0, origins.size()-1)]);
		if(agentsMap->count(rand_origin) > 0)
		{
			std::random_shuffle(agentsMap->at(rand_origin).begin(), agentsMap->at(rand_origin).end());

			int size = agentsMap->at(rand_origin).size();
			int idx = random->random_int(0, size-1);

			ViolenceAgent *agent = agentsMap->at(rand_origin).at(idx);
			std::string key_gender = (agent_type == TEACHER) ? std::to_string(agent->getSex()) : "0";

			if(agent->isPrimaryRisk(&primaryRiskPool))
				continue;

			if(aff_agents != NULL)
			{
				if(aff_agents->count(key_gender) > 0)
				{
					aff_agents->at(key_gender).push_back(agent);
				}
				else
				{
					AgentListPtr temp;
					temp.push_back(agent);
					aff_agents->insert(std::make_pair(key_gender, temp));
				}
			}
			
			primaryRiskPool.insert(std::make_pair(agent->getAgentIdx(), true));
			affected_count--;
		}
	}
}

void ViolenceModel::poolSecondaryRiskAgents()
{
	AgentListPtr tempAgents;
	for(auto sex : Violence::Sex::_values())
		affectedFamFriends.insert(std::make_pair(std::to_string(sex), tempAgents));

	std::string key_gender = "";
	//secondary risk group - Households of students 
	for(auto hh = schoolHouseholds.begin(); hh != schoolHouseholds.end(); ++hh)
	{
		Household *household = *hh; 
		//Agents in same school but not present at school during mass-shooting are at secondary risk
		if(!isAffectedHousehold(household))
		{
			for(auto pp = household->begin(); pp != household->end(); ++pp)
			{
				ViolenceAgent *agent = (&(*pp));
				if(agent->getAge() < getMinAge())
					continue;

				key_gender = std::to_string(agent->getSex());
				if(!agent->isPrimaryRisk(&primaryRiskPool)  && !agent->isSecondaryRisk(&secondaryRiskPool))
				{
					if(agent->getSchoolName() == schoolName)
					{
						affectedFamFriends[key_gender].push_back(agent);
						secondaryRiskPool.insert(std::make_pair(agent->getAgentIdx(), true));
					}
				}
			}
		}
		else 
		{
			for(auto pp = household->begin(); pp != household->end(); ++pp)
			{
				ViolenceAgent *agent = (&(*pp));
				if(agent->getAge() < getMinAge())
					continue;

				//friends of primary affected agents are at secondary risk 
				if(agent->isPrimaryRisk(&primaryRiskPool))
				{
					AgentListPtr *friendList = agent->getFriendList();
					for(auto ff = friendList->begin(); ff != friendList->end(); ++ff)
					{
						ViolenceAgent *frnd = *ff;
						key_gender = std::to_string(frnd->getSex());
						if(!frnd->isPrimaryRisk(&primaryRiskPool) && !frnd->isSecondaryRisk(&secondaryRiskPool))
						{
							affectedFamFriends[key_gender].push_back(frnd);
							secondaryRiskPool.insert(std::make_pair(frnd->getAgentIdx(), true));
						}
					}
				}
				else
				{
					key_gender = std::to_string(agent->getSex());
					if(!agent->isSecondaryRisk(&secondaryRiskPool))
					{
						affectedFamFriends[key_gender].push_back(agent);
						secondaryRiskPool.insert(std::make_pair(agent->getAgentIdx(), true));
					}
				}
			}
		}
	}
}

void ViolenceModel::poolTertiaryRiskAgents(std::string modelNum)
{
	bool isAgeCat = (modelNum == "Model2") ? true : false;

	AgentListPtr tempAgents;
	if(isAgeCat)
	{
		for(auto ageCat : Violence::AgeCat2::_values())
			communityMembers.insert(std::make_pair(std::to_string(ageCat), tempAgents));
	}
	else
	{
		for(auto sex : Violence::Sex::_values())
			communityMembers.insert(std::make_pair(std::to_string(sex), tempAgents));
	}

	std::string key = "";
	for(auto map = othersMap.begin(); map != othersMap.end(); ++map)
	{
		AgentListPtr agentList = map->second;
		for(auto agent = agentList.begin(); agent != agentList.end(); ++agent)
		{
			ViolenceAgent *a = *agent;
			key = (isAgeCat) ? std::to_string(a->getAgeCat2()) : std::to_string(a->getSex());
			if(!a->isPrimaryRisk(&primaryRiskPool) && !a->isSecondaryRisk(&secondaryRiskPool))
			{
				communityMembers[key].push_back(a);
				tertiaryRiskPool.insert(std::make_pair(a->getAgentIdx(), true));
			}
		}
	}
}

void ViolenceModel::schoolDemographics()
{
	std::multimap<std::string, bool> demoStudents, demoTeachers;
	for(auto hh = schoolHouseholds.begin(); hh != schoolHouseholds.end(); ++hh)
	{
		Household *person = *hh;
		for(auto pp = person->begin(); pp != person->end(); ++pp)
		{
			if(pp->getSchoolName() == schoolName)
			{
				if(pp->isStudent())
					demoStudents.insert(std::make_pair(std::to_string(pp->getSex())+std::to_string(pp->getOrigin()), true));
				else if(pp->isTeacher())
					demoTeachers.insert(std::make_pair(std::to_string(pp->getSex())+std::to_string(pp->getOrigin()), true));
			}
		}
	}

	std::cout << "School Demo: " << std::endl;
	for(auto sex : ACS::Sex::_values())
	{
		for(auto origin : ACS::Origin::_values())
		{
			std::string key_demo = std::to_string(sex)+std::to_string(origin);
			std::cout <<  sex << "," << origin << "," << demoStudents.count(key_demo)
				<<"," << demoTeachers.count(key_demo) << std::endl;
		}
		std::cout << std::endl;
	}
}


void ViolenceModel::networkAnalysis()
{
	typedef std::vector<ViolenceAgent *> FriendsPtr;

	std::cout << std::endl;
	std::cout << "Social Network Analysis: " << std::endl;
	int count = 0;
	std::vector<Household>*parkland(&pumaHouseholds[TAYLOR]);

	for(auto hh = parkland->begin(); hh != parkland->end(); ++hh)
	{
		for(auto pp1 = hh->begin(); pp1 != hh->end(); ++pp1)
		{
			ViolenceAgent *agent1 = &(*pp1);
			if(agent1->getTotalFriends() != agent1->getFriendSize())
				std::cout << ++count << "," << agent1->getAgentIdx() << "," << agent1->getTotalFriends() << "," << agent1->getFriendSize() << std::endl;

			FriendsPtr *friends = agent1->getFriendList();
			for(auto pp2 = friends->begin(); pp2 != friends->end(); ++pp2) 
			{
				ViolenceAgent *agent2 = *pp2;
				if(agent1->getAgentIdx() == agent2->getAgentIdx())
				{
					std::cout << "Error: Cannot have self as a friend!" << std::endl;
					exit(EXIT_SUCCESS);
				}

				if(agent1->getHouseholdID() == agent2->getHouseholdID())
				{
					std::cout << "Error: Cannot have family member as a friend!" << std::endl;
					exit(EXIT_SUCCESS);
				}

				if(agent1->isStudent())
				{
					if(abs(agent1->getAge()-agent2->getAge()) > getAgeDiffStudents())
						std::cout << agent1->getAgentIdx() << std::endl;
				}

			}
		}
	}

}

void ViolenceModel::initializeHouseholdMap(int puma_area)
{
	std::vector<Household>vecHouseholds;
	pumaHouseholds.insert(std::make_pair(puma_area, vecHouseholds));
	pumaHouseholds[puma_area].reserve(getMinHouseholdsPuma());
}

void ViolenceModel::resizeHouseholds()
{
	for(auto it = pumaHouseholds.begin(); it != pumaHouseholds.end(); ++it)
		pumaHouseholds[it->first].shrink_to_fit();
}


bool ViolenceModel::isAffectedHousehold(Household *hh)
{
	bool affected = false;
	for(auto pp = hh->begin(); pp != hh->end(); ++pp)
	{
		ViolenceAgent *agent = &(*pp);
		if(agent->isPrimaryRisk(&primaryRiskPool))
		{
			affected = true;
			break;
		}
	}
	return affected;
}

void ViolenceModel::displayMediaDist()
{
	for(auto ageCat : Violence::AgeCat2::_values())
	{
		if(ageCat == (int)Violence::AgeCat2::Age_14_17)
			continue;

		std::cout << "\n\nMedia coverage summary for " << ageCat._to_string() << std::endl;

		int pop = count->getPersonCount("AgeCat2" + std::to_string(ageCat));
		for(auto source : Violence::Source::_values())
		{
			int numMediaUsers = count->getPersonCount("SourceNews" + std::to_string(ageCat) + std::to_string(source));
			std::cout << "\n% " << source._to_string() << "(Pop:" << numMediaUsers << ") = " << 100*(double)numMediaUsers/pop << std::endl;

			if(source == (int)Violence::Source::TV)
			{
				for(auto time : Violence::Coverage::_values())
				{
					int hoursWatched = count->getPersonCount("HoursWatched" + std::to_string(ageCat) + std::to_string(time));
					if(hoursWatched > 0)
					{
						std::cout << time._to_string() << " = " << 100*(double)hoursWatched/numMediaUsers << "," << hoursWatched << std::endl;
					}
				}
			}
			else if(source == (int)Violence::Source::SocialMedia)
			{
				for(auto hours : Violence::SocialMediaHours::_values())
				{
					int socialMediaHours = count->getPersonCount("SocialMediaHours" + std::to_string(ageCat) + std::to_string(hours));
					if(socialMediaHours > 0)
					{
						std::cout << hours._to_string() << " = " << 100*(double)socialMediaHours/numMediaUsers << "," << socialMediaHours <<  std::endl;
					}
				}
			}
		}
	}

	std::cout << std::endl;
}

void ViolenceModel::computePrevalence(int newsSource, bool display, std::string period, int scenario)
{
	int ptsdCases, totPop;
	double prevalence;

	PairDD ptsdTotals;
	MapPair m_ptsdTotals;

	std::string s_scenarioByPeriod = period + " " + Violence::Scenario::_from_integral(scenario)._to_string();
	std::string s_newsSource = Violence::Source::_from_integral(newsSource)._to_string();

	for(auto ageCat : Violence::AgeCat2::_values())
	{
		if(ageCat == (int)Violence::AgeCat2::Age_14_17)
			continue;

		std::string agentType  = "SourceNews" + std::to_string(ageCat) + std::to_string(newsSource);

		ptsdCases = count->getPersonCount("Ptsd" + agentType);
		totPop = count->getPersonCount("AgeCat2" + std::to_string(ageCat));

		ptsdTotals.first = ptsdCases;
		ptsdTotals.second = totPop;

		prevalence = (double)ptsdCases/totPop;

		m_ptsdTotals.insert(std::make_pair(std::to_string(ageCat), ptsdTotals));

		if(display)
		{
			std::cout << "Prevalence PTSD (" << ageCat._to_string() << "," << ptsdCases << "," << totPop << ") = " << prevalence << std::endl;
		}
	}

	count->addPrevalence(&m_ptsdTotals, s_scenarioByPeriod, s_newsSource);

}

void ViolenceModel::resetPersonCounter(int source)
{
	for(auto ageCat : Violence::AgeCat2::_values())
	{
		std::string agentType = "SourceNews" + std::to_string(ageCat) + std::to_string(source);
		count->resetPersonCount("Ptsd" + agentType);

		if(source == (int)Violence::Source::TV)
		{
			for(auto time : Violence::Coverage::_values())
			{
				count->resetPersonCount("HoursWatched" + std::to_string(ageCat) + std::to_string(time));
			}
		}
		else if(source == (int)Violence::Source::SocialMedia)
		{
			for(auto time : Violence::SocialMediaHours::_values())
			{
				count->resetPersonCount("SocialMediaHours" + std::to_string(ageCat) + std::to_string(time));
			}
		}
	}
}

std::string ViolenceModel::preIntervention()
{
	return "Pre-Intervention";
}

std::string ViolenceModel::postIntervention()
{
	return "Post-Intervention";
}

ViolenceCounter * ViolenceModel::getCounter() const
{
	return count;
}


int ViolenceModel::getNumStudents(const std::vector<ViolenceAgent>&agents, int &countPersons) const
{
	int num_students = 0; 
	for(auto pp = agents.begin(); pp != agents.end(); ++pp)
	{
		if(pp->isStudent())
			num_students++;

		if(pp->getAge() >= getMinAge())
			countPersons++;
	}

	return num_students;
}

int ViolenceModel::getOriginKey(const ViolenceAgent *a) const
{
	int origin = -1;
	double originChoice = random->uniform_real_dist();
	if(originChoice > getPval(ORIGIN))
	{
		origin = a->getOrigin();
	}
	else
	{
		origin = random->random_int(ACS::Origin::Hisp, ACS::Origin::TwoNH);
		if(origin == a->getOrigin())
		{
			while(true)
			{
				origin = random->random_int(ACS::Origin::Hisp, ACS::Origin::TwoNH);
				if(origin != a->getOrigin())
					break;
			}
		}
	}

	return origin;
}

int ViolenceModel::getEducationKey(const ViolenceAgent *a) const
{
	int edu = -1;
	double eduChoice = random->uniform_real_dist();
	if(eduChoice > getPval(EDUCATION))
	{
		edu = a->getEducation();
	}
	else
	{
		edu = random->random_int(ACS::Education::Less_9th_Grade, ACS::Education::Graduate_Degree);
		if(edu == a->getEducation())
		{
			while(true)
			{
				edu = random->random_int(ACS::Education::Less_9th_Grade, ACS::Education::Graduate_Degree);
				if(edu != a->getEducation())
					break;
			}
		}
	}

	return edu;
}

int ViolenceModel::getInnerDraws() const
{
	return parameters->getViolenceParam()->inner_draws;
}

int ViolenceModel::getOuterDraws() const
{
	return parameters->getViolenceParam()->outer_draws;
}

int ViolenceModel::getMinHouseholdsPuma() const
{
	return parameters->getViolenceParam()->min_households_puma;
}

int ViolenceModel::getMeanFriendSize() const
{
	return parameters->getViolenceParam()->mean_friends_size;
}

int ViolenceModel::getNumTeachers() const
{
	return parameters->getViolenceParam()->num_teachers;
}

int ViolenceModel::getMinAge() const
{
	return parameters->getViolenceParam()->min_age;
}

int ViolenceModel::getAgeDiffStudents() const
{
	return parameters->getViolenceParam()->age_diff_students;
}

int ViolenceModel::getAgeDiffOthers() const
{
	return parameters->getViolenceParam()->age_diff_others;
}

int ViolenceModel::getAffectedStudents() const
{
	return parameters->getViolenceParam()->aff_students;
}

int ViolenceModel::getAffectedTeachers() const
{
	return parameters->getViolenceParam()->aff_teachers;
}

int ViolenceModel::getMaxWeeks() const
{
	return parameters->getViolenceParam()->tot_steps;
}

double ViolenceModel::getPval(int type) const
{
	switch(type)
	{
	case STUDENT:
		return parameters->getViolenceParam()->p_val_student;
		break;
	case TEACHER:
		return parameters->getViolenceParam()->p_val_teacher;
		break;
	case ORIGIN:
		return parameters->getViolenceParam()->p_val_origin;
		break;
	case EDUCATION:
		return parameters->getViolenceParam()->p_val_edu;
		break;
	case AGE:
		return parameters->getViolenceParam()->p_val_age;
		break;
	case GENDER:
		return parameters->getViolenceParam()->p_val_gender;
		break;
	default:
		return -1;
		break;
	}
}

double ViolenceModel::getPrevalence(int p_type) const
{
	switch(p_type)
	{
	case PREVAL_STUDENTS:
		return parameters->getViolenceParam()->prev_students_tot;
		break;
	case PREVAL_TEACHERS_FEMALE:
		return  parameters->getViolenceParam()->prev_teachers_female;
		break;
	case PREVAL_TEACHERS_MALE:
		return parameters->getViolenceParam()->prev_teachers_male;
		break;
	case PREVAL_FAM_FEMALE:
		return parameters->getViolenceParam()->prev_fam_female;
		break;
	case PREVAL_FAM_MALE:
		return parameters->getViolenceParam()->prev_fam_male;
		break;
	case PREVAL_COMM_FEMALE:
		return parameters->getViolenceParam()->prev_comm_female;
		break;
	case PREVAL_COMM_MALE:
		return  parameters->getViolenceParam()->prev_comm_male;
		break;
	default:
		return -1;
		break;
	}
}


void ViolenceModel::setSize(int pop)
{
	//do nothing here
}

void ViolenceModel::setPtsdStatus(AgentListMap *affectedAgents, int sex, double preval, int type)
{
	if(preval < 0)
		exit(EXIT_SUCCESS);

	//MapPair *m_ptsdx = parameters->getPtsdSymptoms();
	MapQuadraple *m_ptsdx_ = parameters->getPtsdSymptoms();

	std::string key_sex = std::to_string(sex);
	AgentListPtr *agentList = &affectedAgents->at(key_sex);
	
	ViolenceAgent *affAgent = NULL;

	//PTSD symptoms assignment for cases
	size_t prev_count = boost::math::round(preval*agentList->size());
	while(prev_count > 0)
	{
		std::random_shuffle(agentList->begin(), agentList->end());

		int sample_size = agentList->size();
		int randIdx = random->random_int(0, sample_size-1);

		affAgent = agentList->at(randIdx);
		if(affAgent->getPtsdCase() == PTSD_CASE)
			continue;

		affAgent->setPTSDstatus(true, type);
		affAgent->setPTSDcase(true);
		affAgent->setPTSDx(m_ptsdx_);
		
		prev_count--;
	}

	//PTSD symptoms assignment for non-cases
	for (auto agent = agentList->begin(); agent != agentList->end(); ++agent)
	{
		affAgent = *agent;
		if(affAgent->getPtsdCase() == PTSD_CASE)
			continue;

		affAgent->setPTSDstatus(true, type);
		affAgent->setPTSDx(m_ptsdx_);
	}
}


void ViolenceModel::setPtsdStatus(AgentListMap *affAgents, MapDbl m_prevalence, int newsSource, int scenario, std::string period)
{
	std::cout << "\nDistributing PTSD prevalence among " << Violence::Source::_from_integral(newsSource)._to_string() << "..." << std::endl;

	AgentListPtr *agentList = NULL;
	ViolenceAgent *agent = NULL;

	int total_population = 0;
	double oddRatio = (newsSource ==(int) Violence::Source::TV) ? parameters->getViolenceParam()->odd_ratio.at(std::to_string(scenario)) : 1;

	for(auto map = affAgents->begin(); map != affAgents->end(); ++map)
	{
		std::string key = map->first;
		double preval = oddRatio * m_prevalence.at(key);

		agentList = &(map->second);
		int pop = agentList->size();

		total_population += pop;

		size_t ptsd_count = boost::math::round(preval*pop);
		int num_ptsd = 0;
		
		std::random_shuffle(agentList->begin(), agentList->end());
		
		while(ptsd_count > 0)
		{
			int randIdx = random->random_int(0, pop-1);

			agent = agentList->at(randIdx);
			if(agent->getPtsdCase() == PTSD_CASE)
				continue;

			agent->setPTSDstatus(true, TERTIARY);
			agent->setPTSDcase(true);

			ptsd_count--;
			num_ptsd++;
		}
	}

	computePrevalence(newsSource, true, period, scenario);
}

void ViolenceModel::resetPtsdStatus(AgentListMap *m_tvWatchers, AgentListMap *m_socialMediaUsers)
{
	resetPtsdStatus(m_tvWatchers, Violence::Source::TV);
	resetPtsdStatus(m_socialMediaUsers, Violence::Source::SocialMedia);
}

void ViolenceModel::resetPtsdStatus(AgentListMap *m_agents, int source)
{
	resetPersonCounter(source);

	std::cout << "\nRe-setting PTSD status..." << std::endl;
	for(auto map = m_agents->begin(); map != m_agents->end(); ++map)
	{
		for(auto agent = map->second.begin(); agent != map->second.end(); ++agent)
		{
			ViolenceAgent *a = *agent;
			if(a->getPtsdCase() == PTSD_CASE)
				a->setPTSDcase(false);
		}
	}

	std::cout << "Reset complete!" << std::endl;
}

void ViolenceModel::resetAttributes(AgentListMap *m_agents, int source)
{
	std::cout << "\nRe-setting Hours..." << std::endl;
	for(auto map = m_agents->begin(); map != m_agents->end(); ++map)
	{
		int hours = std::stoi(map->first);
		for(auto agent = map->second.begin(); agent != map->second.end(); ++agent)
		{
			ViolenceAgent *a = *agent;
			if(source == (int)Violence::Source::TV)
			{
				a->setNumberofTvHours(hours);
			}
			else if(source == (int)Violence::Source::SocialMedia)
			{
				a->setSocialMediaHours(hours);
			}
		}
	}

	resetPtsdStatus(m_agents, source);
	//resetPersonCounter(source);
	std::cout << "Reset complete!" << std::endl;
}

void ViolenceModel::clearList()
{
	for(auto it = pumaHouseholds.begin(); it != pumaHouseholds.end(); ++it)
	{
		it->second.clear();
		it->second.shrink_to_fit();
	}

	schoolHouseholds.clear();

	studentsMap.clear();
	teachersMap.clear();
	othersMap.clear();

	affectedStudents.clear();
	affectedTeachers.clear();
	affectedFamFriends.clear();
	communityMembers.clear();

	primaryRiskPool.clear();
	secondaryRiskPool.clear();
	tertiaryRiskPool.clear();
}

