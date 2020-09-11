#ifndef __ViolenceParams_h__
#define __ViolenceParams_h__

#include "Parameters.h"

//Violence Model Parameters
namespace MVS
{
	struct Violence
	{
		int sim_case;
		int inner_draws, outer_draws;
		int min_households_puma;
		int mean_friends_size;
		int num_teachers;
		int min_age;
		int age_diff_students, age_diff_others;
		double p_val_student, p_val_teacher, p_val_origin, p_val_edu, p_val_age, p_val_gender;
		int aff_students, aff_teachers;
		double prev_students_tot;
		double prev_teachers_female;
		double prev_teachers_male;
		double prev_fam_female;
		double	prev_fam_male;
		double	prev_comm_female;
		double prev_comm_male;
		double ptsd_cutoff;
		double sec_coeff, ter_coeff;
		int screening_time;
		double sensitivity;
		double specificity;
		int tot_steps;
		int treatment_time;
		int num_trials;
		int cbt_dur_non_cases;
		int max_cbt_sessions;
		int max_spr_sessions;
		double cbt_coeff;
		double spr_coeff;
		int cbt_cost, spr_cost;
		double nd_coeff;
		double percent_nd;
		int nd_dur;
		double ptsdx_relapse;
		int time_relapse;
		int num_relapse;
		double percent_relapse;
		double dw_mild, dw_moderate, dw_severe;
		double discount;
		//std::map<int, double> p_tv_age, p_online_news_age;
		std::map<int, std::vector<std::pair<double, int>>> news_source_dist;
		std::vector<std::pair<double, int>> p_tv_time, p_social_media_hours;
		std::map<std::string, double> ptsd_prev_tv_time, ptsd_prev_social_media, odd_ratio;
		
	};
}

class ViolenceParams : public Parameters
{
public:
	ViolenceParams();
	ViolenceParams(const char*, const char*, int, int);
	virtual ~ViolenceParams();

	MapInt getSchoolDemographics();
	PairTuple *getPtsdSymptoms();

	const MVS::Violence *getViolenceParam();

private:

	//Mass-violence model
	void readSchoolDemograhics();
	void readMassViolenceInputs();
	void readPtsdSymptoms();
	void setViolenceParams(MapDbl *);

	void setMediaDistParams(MapDbl *);
	void setTvCoverageDistParams(MapDbl *);
	void setSocialMediaHoursDistParams(MapDbl *);
	void setPrevalenceDistParams(MapDbl *);
	void setPrevalenceDistSocialMediaParams(MapDbl *);
	void setOddRatioParams(MapDbl *);

	MapInt m_schoolDemo;
	MVS::Violence vParams;
	PairTuple m_ptsdx_;
};
#endif