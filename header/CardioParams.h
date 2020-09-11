#ifndef __CardioParams_h__
#define __CardioParams_h__

#include "Parameters.h"

namespace EET
{
	//BETTER_ENUM(TaxType, int, One_Dollar=1, Two_Dollar=2, Three_Dollar=3);
	BETTER_ENUM(TaxType, int, Two_Dollar=2, Three_Dollar=3, Four_Dollar=4, Five_Dollar=5);
	//BETTER_ENUM(StatinsType, int, Strong=1, Weak, Stronger);
	BETTER_ENUM(StatinsType, int, Strong=1, Weak, Stronger, Strongest);
	//BETTER_ENUM(OtherIntervention, int, No_Intervention=0, Dollar_Tax, Two_Dollar_Tax, Five_Dollar_Tax, Statins, Weak_Statins, Dollar_Tax_Statins, Two_Dollar_Tax_Statins);
	BETTER_ENUM(Interventions, int, None=0, Tax, Statins, Tax_Statins);
	BETTER_ENUM(EduIntervention, int, WithoutEducation=8, Education);

	struct Framingham
	{
		double age, tchols, hdl;
		double sbp, htn;
		double smoker, age_tchols;
		double age_smoker, sq_age;
	};

	struct Score
	{
		double tchols, sbp, smoking;
	};

	struct Cardio
	{
		int num_years;
		int num_trials;

		Framingham male_coeff, female_coeff;
		Score score_beta_coeff;

		double chd_survival_male_fhs, chd_survival_female_fhs;

		double chd_survival_white_male, chd_survival_white_female;
		double chd_survival_black_male, chd_survival_black_female;

		double fatal_chd_survival_white_male, fatal_chd_survival_white_female;
		double fatal_chd_survival_black_male, fatal_chd_survival_black_female;

		double pReductionSmokingWMHS, pReductionSmokingWFHS;
		double pReductionSmokingWMSC, pReductionSmokingWFSC;

		double pReductionSmokingBMHS, pReductionSmokingBFHS;
		double pReductionSmokingBMSC, pReductionSmokingBFSC;

		double lifeExpectancyWMHS, lifeExpectancyWFHS;
		double lifeExpectancyWMSC, lifeExpectancyWFSC;

		double lifeExpectancyBMHS, lifeExpectancyBFHS;
		double lifeExpectancyBMSC, lifeExpectancyBFSC;

		double statin_uptake_WM, statin_uptake_WF;
		double statin_uptake_BM, statin_uptake_BF;
		double statin_cost1, statin_cost2;

		double ldl_change, hdl_change, trigly_change;

	};

	/*Collection of risk factor states, if present coded 1, else 0*/
	struct RiskState
	{
		short int tchols, hdl, sysBp; 
		short int smokingStat, htn;
	};

	/*Collection of risk factors*/
	struct RiskFactors
	{
		std::pair<double, short int> tchols, hdlChols, systolicBp;

		double ldlChols, triglyceride;
		short int curSmokeStat, htnMed;
		short int onStatin;

		short int isSmoker;
	};

	
}

class CardioParams : public Parameters
{
public:

	typedef std::map<std::string, std::vector<PairDD>> ProbMap;
	typedef std::map<std::string, PairDD> PairMap;
	typedef std::map<int,std::map<std::string, PairDD>> RiskFacMap;
	typedef std::map<int, EET::RiskState> RiskMatrix;
	typedef std::pair<double, EET::RiskFactors> WeightRiskPair;
	typedef std::pair<int, std::vector<WeightRiskPair>> PairIntVector;
	typedef std::vector<WeightRiskPair> VectorWeightRisks;
	typedef std::vector<PairIntVector> VectorCumulativeProbability;
	typedef std::map<int, VectorWeightRisks> StrataRiskMap;
	typedef std::map<std::string, StrataRiskMap> AgentStrataRiskMap;
	typedef std::map<std::string, VectorCumulativeProbability> RiskStrataCumulativeProbability;
	
	typedef std::map<std::string, std::map<int, std::map<PairDD, WeightRiskPair>>> RiskFactorsMap2;
	typedef std::map<std::string, std::vector<double>> WeightsByAgentType;

	CardioParams();
	CardioParams(const char*, const char*, const int, const int);
	virtual ~CardioParams();

	ProbMap getRiskStrataProbability() const;
	
	std::string beforeIntervention() const;
	std::string beforeIntervention(int) const;
	std::string afterIntervention() const;
	std::string afterIntervention(int) const;

	std::string beforeEducation() const;
	std::string afterEducation() const;

	const VectorCumulativeProbability *getRiskStrataCumulativeProbability(std::string);
	const AgentStrataRiskMap *getRiskFactorMap();
	const VectorWeightRisks *getRiskFactorByStrata(std::string, int);
	const EET::RiskState *getRiskMatrix(int);
	const int getRiskStrata(std::string);
	const double getPercentEduDifference(std::string);
	const EET::Cardio* getCardioParam();
	std::string getInterventionType(int, int, int, bool);

	void setPercentEduDifference(MapDbl);

private:

	//Equity-efficiency model
	void readNHANESRiskFactors();
	void addRiskFactors(RiskFactorsMap2 &, WeightsByAgentType &);
	void computeRiskFactorSelectionProbability(WeightsByAgentType &);

	void readRiskFactorMatrix();
	void readFraminghamCoefficients(); 
	void setCardioParams(PairMap *);

	std::string getRiskFactorCombination(EET::RiskState &);

	//EET 
	ProbMap m_riskStrataProb;
	RiskStrataCumulativeProbability m_pRiskStrataEdu;
	AgentStrataRiskMap m_riskFactors;
	RiskMatrix m_riskMatrix;
	MapInt m_riskStrata;
	EET::Cardio cardioParams;
	MapDbl percentEduDifference;

};
#endif __CardioParams_h__
