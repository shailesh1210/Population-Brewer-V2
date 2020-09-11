#ifndef __ACS_h__
#define __ACS_h__

#include <iostream>
#include "enum.h"

namespace ACS
{
	BETTER_ENUM(Counts, int, countAge, countOrigin, countEdu, countHHSize, countHHIncome);
	BETTER_ENUM(Estimates, int, estRace, estEducation, estHHType, estHHSize, estHHIncome, estGQ);
	BETTER_ENUM(PumsVar, int, SERIALNO,SPORDER,PUMA,ST,ADJINC, AGEP,SEX,HISP,RAC1P,SCHL, HHT, HINCP);
	BETTER_ENUM(Index, int, Household_GQ, Child, Adult);
	//BETTER_ENUM(Index, int, Household, Person);
	
	//BETTER ENUMS: Person-level marginal variables

	//List of variables extracted from ACS race estimate file
	BETTER_ENUM(RaceMarginalVar, int, ORG,
		POP_M_0_4,POP_M_5_9,POP_M_10_14,POP_M_15_17,POP_M_18_19,POP_M_20,POP_M_21,POP_M_22_24,
		POP_M_25_29,POP_M_30_34,
		POP_M_35_39,POP_M_40_44,
		POP_M_45_49,POP_M_50_54,POP_M_55_59,POP_M_60_61,POP_M_62_64,
		POP_M_65_66,POP_M_67_69,POP_M_70_74,POP_M_75_79,POP_M_80_84,POP_M_85_150,
		POP_F_0_4,POP_F_5_9,POP_F_10_14,POP_F_15_17,POP_F_18_19,POP_F_20,POP_F_21,POP_F_22_24,
		POP_F_25_29,POP_F_30_34,
		POP_F_35_39,POP_F_40_44,
		POP_F_45_49,POP_F_50_54,POP_F_55_59,POP_F_60_61,POP_F_62_64,
		POP_F_65_66,POP_F_67_69,POP_F_70_74,POP_F_75_79,POP_F_80_84,POP_F_85_150);

	//List of variables extracted from ACS Educational attainment estimate file
	BETTER_ENUM(EduMarginalVar1, int, 
		POP_M_18_24_E1,POP_M_18_24_E2,POP_M_18_24_E3,POP_M_18_24_E4,POP_M_18_24_E5,POP_M_18_24_E6,POP_M_18_24_E7, 
		POP_M_25_34_E1,POP_M_25_34_E2,POP_M_25_34_E3,POP_M_25_34_E4,POP_M_25_34_E5,POP_M_25_34_E6,POP_M_25_34_E7,
		POP_M_35_44_E1,POP_M_35_44_E2,POP_M_35_44_E3,POP_M_35_44_E4,POP_M_35_44_E5,POP_M_35_44_E6,POP_M_35_44_E7,
		POP_M_45_64_E1,POP_M_45_64_E2,POP_M_45_64_E3,POP_M_45_64_E4,POP_M_45_64_E5,POP_M_45_64_E6,POP_M_45_64_E7,
		POP_M_65_OVER_E1, POP_M_65_OVER_E2, POP_M_65_OVER_E3, POP_M_65_OVER_E4, POP_M_65_OVER_E5, POP_M_65_OVER_E6, POP_M_65_OVER_E7);

	
	//List of variables extracted from ACS Educational attainment estimate file
	BETTER_ENUM(EduMarginalVar2, int,
		POP_F_18_24_E1 = EduMarginalVar1::POP_M_65_OVER_E7+1,
		POP_F_18_24_E2,POP_F_18_24_E3,POP_F_18_24_E4,POP_F_18_24_E5,POP_F_18_24_E6,POP_F_18_24_E7,
		POP_F_25_34_E1,POP_F_25_34_E2,POP_F_25_34_E3,POP_F_25_34_E4,POP_F_25_34_E5,POP_F_25_34_E6,POP_F_25_34_E7,
		POP_F_35_44_E1,POP_F_35_44_E2,POP_F_35_44_E3,POP_F_35_44_E4,POP_F_35_44_E5,POP_F_35_44_E6,POP_F_35_44_E7,
		POP_F_45_64_E1,POP_F_45_64_E2,POP_F_45_64_E3,POP_F_45_64_E4,POP_F_45_64_E5,POP_F_45_64_E6,POP_F_45_64_E7,
		POP_F_65_OVER_E1, POP_F_65_OVER_E2, POP_F_65_OVER_E3, POP_F_65_OVER_E4, POP_F_65_OVER_E5, POP_F_65_OVER_E6, POP_F_65_OVER_E7); 

	//@BETTER ENUMS: Household level marginal

	//List of variables extracted from ACS Household type file
	BETTER_ENUM(HHTypeMarginalVar, int,
		HH_MAR_FAM,HH_MALE_FAM,HH_FEMALE_FAM);

	//List of variables extracted from ACS Household size estimate file
	BETTER_ENUM(HHSizeMarginalVar, int, 
		HH_FAM_2P, HH_FAM_3P,HH_FAM_4P,HH_FAM_5P,HH_FAM_6P,HH_FAM_7P,	
		HH_NON_FAM_1P,HH_NON_FAM_2P,HH_NON_FAM_3P,HH_NON_FAM_4P,HH_NON_FAM_5P,HH_NON_FAM_6P,HH_NON_FAM_7P);

	//List of variables extracted from ACS Household income estimate file
	BETTER_ENUM(HHIncMarginalVar, int, 
		HH_INC1,HH_INC2, HH_INC3, HH_INC4, HH_INC5, HH_INC6, HH_INC7, HH_INC8,  
		HH_INC9,HH_INC10, HH_INC11, HH_INC12, HH_INC13, HH_INC14, HH_INC15, HH_INC16);

	//List of variables extracted from ACS Group Quarters estimate file
	BETTER_ENUM(GQMarginalVar, int, GQ_POP);


	//@BETTER_ENUMS: Person level attributes
	BETTER_ENUM(AgeCat, int, 
		Age_0_4=1, Age_5_9, Age_10_14, Age_15_17, Age_18_19, Age_20, Age_21, Age_22_24, 
		Age_25_29, Age_30_34, 
		Age_35_39, Age_40_44, 
		Age_45_49, Age_50_54, Age_55_59, Age_60_61, Age_62_64, 
		Age_65_66, Age_67_69, Age_70_74, Age_75_79, Age_80_84, Age_85_100);

	//BETTER_ENUM(AgeCatRf, int, Rf_Age_35_44 = 1, Rf_Age_45_54, Rf_Age_55_64, Rf_Age_65_74, Rf_Age_75_Over);

	BETTER_ENUM(ChildAgeCat, int, Age_0_4=1, Age_5_9, Age_10_14, Age_15_17);
	BETTER_ENUM(Sex, int, Male=1, Female);
	BETTER_ENUM(Race, int, White=1, Black, American_Indian_Alaska_Native, Asian, Hawaiian_Pacific, Some_Other, Two_Or_More);
	BETTER_ENUM(Ethnicity, int, Not_Hispanic=1, Hispanic);
	BETTER_ENUM(Origin, int, Hisp=1, WhiteNH, BlackNH, AmericanAlaskanNH, AsianNH, HawaiianNH, SomeOtherNH, TwoNH);
	BETTER_ENUM(Education, int, Less_9th_Grade=1, _9th_To_12th_Grade, High_School, Some_College, Associate_Degree, Bachelors_Degree, Graduate_Degree);
	BETTER_ENUM(EduAgeCat, int, Age_18_24=1, Age_25_34, Age_35_44, Age_45_64, Age_65_Over);

	//@BETTER_ENUMS: Household level attributes
	BETTER_ENUM(HHType, int, MarriedFam=1, MaleHHFam, FemaleHHFam, NonFamily);  
	BETTER_ENUM(HHSize, int, HHSize1=1, HHSize2, HHsize3, HHsize4, HHsize5, HHsize6, HHsize7); 
	BETTER_ENUM(HHIncome, int, HHInc1=1, HHInc2, HHInc3, HHInc4, HHInc5, HHInc6, HHInc7, HHInc8, HHInc9, HHInc10);

	BETTER_ENUM(HHSizeByAgeCat, int, One_Person_Under65=1, One_Person_Over65, Two_Person_Under65, Two_Person_Over65, 
		Three_Person, Four_Person, Five_Person, Six_Person, Seven_Person, Eight_Person, Nine_Person);
	BETTER_ENUM(NumChild, int, Zero_Child, One_Child, Two_Child, Three_Child, Four_Child, Five_Child,
		Six_Child, Seven_Child, Eight_Child);
	BETTER_ENUM(TotalPersons, int, One=1, Two, Three, Four, Five, Six, Seven, Eight, Nine);
	BETTER_ENUM(AgeCat1, int, All=1, Under65, Over65);

	BETTER_ENUM(IncomeToPovertyRatio, int, IP_0_50=1, IP_50_100, IP_100_150, IP_150_200, IP_200_250, IP_250_300,
		IP_300_350, IP_350_400, IP_400_450, IP_450_500, IP_500_);
}

/*Enumeration of NHANES related demographic variables for Cardio Model*/
namespace NHANES
{
	BETTER_ENUM(Sex, int, Male=1, Female);
	BETTER_ENUM(AgeCat, int, Age_35_44=1, Age_45_54, Age_55_64, Age_65_74, Age_75_);
	BETTER_ENUM(AgeCat3, int, Age_35_44=1, Age_45_64, Age_65_);
	BETTER_ENUM(Org, int, WhiteNH=1, BlackNH);
	BETTER_ENUM(Edu, int, HS_or_less=1, some_coll_);
	BETTER_ENUM(RiskFac, int, Age, totalChols, LdlChols, HdlChols, SystolicBp, SmokingStat, HyperTension);
	BETTER_ENUM(SmokingStatus, int, NonSmoker=0, FormerSmoker, CurrentSmoker);
	BETTER_ENUM(WeightedSum, int, Individual=1, Mean);
}

namespace Violence
{
	BETTER_ENUM(Sex, int, Male=1, Female);
	BETTER_ENUM(AgeCat, int, Age_14_34=1, Age_35_64, Age_65_);
	BETTER_ENUM(AgeCat2, int, Age_14_17=1, Age_18_29=2, Age_30_49, Age_50_64, Age_65_);
	BETTER_ENUM(Origin, int, Hisp=1, WhiteNH, BlackNH, OtherNH);
	BETTER_ENUM(Exposure, int, Primary=0,Secondary,Tertiary);
	BETTER_ENUM(Ptsd_case, int, NonCase=0, Case);
	BETTER_ENUM(Coverage, int, Hours_Less_4=1, Hours_4_7, Hours_8_11, Hours_12_More);
	BETTER_ENUM(Source, int, Other=0, TV, SocialMedia, TV_SocialMedia);
	BETTER_ENUM(SocialMediaHours, int, None=0, Hours_2_Less, Hours_2_More);
	BETTER_ENUM(Scenario, int, First=1, Second, Third);
	BETTER_ENUM(SensitivityAnalysisTV, int, TV_Hours_Less4=1, TV_Hours_8_12, TV_Hours_0_7); 
	BETTER_ENUM(SensitivityAnalysisSocialMedia, int, No_SocialMedia=1, SocialMediaHours_Less_2, SocialMediaHours_More_2); 
}

namespace Depression
{
	BETTER_ENUM(AgeCat, int, Age_18_44=1, Age_45_64, Age_65_);
	BETTER_ENUM(DepressionType, int, None=1, Other, Major);
	BETTER_ENUM(DepressionCase, int, NonCase=0, Case);
}

namespace Geography
{
	BETTER_ENUM(Level, int, MSA=1, STATE=2);
}


#endif __ACS_h__