#ifndef __ViolenceAgent_h__
#define __ViolenceAgent_h__

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <tuple>
#include "Agent.h"

#define NUM_PTSD 3
#define PRIMARY 0
#define SECONDARY 1
#define TERTIARY 2

#define NUM_CASES 2
#define PTSD_NON_CASE 0
#define PTSD_CASE 1

//PTSDx quartiles
#define FIRST_Q 0.25
#define SECOND_Q 0.50
#define THIRD_Q 0.75
#define FOURTH_Q 1.0

//PTSD treatments
#define NUM_TREATMENT 3
#define STEPPED_CARE 0
#define USUAL_CARE 1
#define NO_TREATMENT 2

#define CBT 0
#define SPR 1

#define MIN_PTSDX 0
#define MAX_PTSDX 17

#define MILD_PTSDX_MAX 8.0
#define MOD_PTSDX_MAX 11.0

class ViolenceParams;
class Random;
class ViolenceCounter;

template<class GenericParams>
class PersonPums;

class ViolenceAgent : public Agent<ViolenceParams>
{
public:
	typedef std::vector<ViolenceAgent*>FriendsPtr;
	typedef std::vector<std::pair<double, int>> VecPairs;
	typedef std::pair<double, double> PairDD;
	typedef std::tuple<double, double, double, double> Tuple;
	typedef std::map<std::string, PairDD> MapPair;
	typedef std::map<std::string, Tuple> MapTuple;
	typedef std::map<std::string, double> MapDbl;
	typedef std::map<std::string, bool> MapBool;

	ViolenceAgent();
	ViolenceAgent(std::shared_ptr<ViolenceParams>, const PersonPums<ViolenceParams> *, Random *, ViolenceCounter *, int, int);

	virtual ~ViolenceAgent();

	void excecuteRules(int);

	void setAgentIdx(std::string);
	void setAgeCat();
	void setAgeCat2();
	void setNewOrigin();
	//void setPTSDx(MapPair *, bool);
	void setPTSDx(MapTuple *);
	void setPTSDstatus(bool, int);
	void setPTSDcase(bool);
	void setSchoolName(std::string);
	void setFriendSize(int);
	void setFriend(ViolenceAgent *);
	void setNumberOfTvHours();
	void setNumberofTvHours(int);
	void setSocialMediaHours();
	void setSocialMediaHours(int);
	void setNewsSource(VecPairs *);
	void setDummyVariables();

	std::string getAgentIdx() const;
	short int getAgeCat2() const;
	double getInitPTSDx() const;
	double getPTSDx(int) const;
	//double getPTSDx(PairDD) const;
	double getPTSDx(Tuple, std::string) const;
	double getPtsdCutOff() const;
	bool getPTSDstatus(int) const;
	bool getCBTReferred() const;
	bool getSPRReferred() const;
	short int getCBTsessions(int) const;
	short int getSPRsessions(int) const;
	int getPtsdType() const;
	int getPtsdCase() const;
	int getDurationMild(int) const;
	int getDurationModerate(int) const;
	int getDurationSevere(int) const;
	int getResolvedTime(int) const;
	int getNumberOfTvHours() const;
	int getSocialMediaHours() const;
	int getNewsSource() const;

	std::string getAgentType1() const;
	std::string getAgentType2() const;
	std::string getAgentType3() const;
	std::string getAgentType4() const;

	std::string getSchoolName() const;
	FriendsPtr *getFriendList();
	int getFriendSize() const;
	int getTotalFriends() const;
	int getMaxCbtTime() const;
	int getMaxSprTime() const;

	bool isStudent() const;
	bool isTeacher() const;
	bool isCompatible(const ViolenceAgent *) const;
	bool isFriend(const ViolenceAgent *) const;
	bool isPrimaryRisk(const MapBool *) const;
	bool isSecondaryRisk(const MapBool *) const;
	
private:
	
	void priorTreatmentUptake(int);
	void ptsdScreeningSteppedCare(int);
	void provideTreatment(int, int);
	void symptomResolution(int, int);
	void symptomRelapse(int);

	void provideCBT(int, int);
	void provideSPR(int, int);
	double getTrtmentUptakeProbability(int, int);

	bool watchesNews(double);

	Random *random;
	ViolenceCounter *counter;
	std::shared_ptr<ViolenceParams>parameters;

	std::string schoolName, agentIdx;
	short int ageCat, ageCat2, newOrigin;
	int friendSize;
	FriendsPtr friendList;

	//ptsd variables
	double initPtsdx, ptsdx[NUM_TREATMENT];
	bool priPTSD, secPTSD, terPTSD;
	bool ptsdCase;
	short int ptsdTime[NUM_TREATMENT];
	short int durMild[NUM_TREATMENT], durMod[NUM_TREATMENT], durSevere[NUM_TREATMENT];

	//treatment variables
	short int priorCBT, priorSPR;
	bool cbtReferred, sprReferred;
	bool curCBT[NUM_TREATMENT], curSPR[NUM_TREATMENT];
	short int sessionsCBT[NUM_TREATMENT], sessionsSPR[NUM_TREATMENT];
	
	//relapse variables
	short int numRelapse[NUM_TREATMENT], relapseTimer[NUM_TREATMENT];

	//resolution variables
	bool isResolved[NUM_TREATMENT];
	short int resolvedTime[NUM_TREATMENT];

	//media exposure variable
	int hoursWatched, socialMediaHours;;
	int newsSource;

	//dummy variables
	short int age1, age2, age3;
	short int white, black, hispanic, other;
};
#endif