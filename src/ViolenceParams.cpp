#include "ViolenceParams.h"

ViolenceParams::ViolenceParams(){}

ViolenceParams::ViolenceParams(const char *inDir, const char *outDir, int simType, int geoLvl)
	: Parameters(inDir, outDir, simType, geoLvl)
{
	readSchoolDemograhics();
	readMassViolenceInputs();
	readPtsdSymptoms();
}

ViolenceParams::~ViolenceParams()
{
}

/**
*	@brief reads demographics of students by gender and origin for Mass-violence model
*	@param none
*	@return none
*/
void ViolenceParams::readSchoolDemograhics()
{
	io::CSVReader<3>school_demo(getFilePath("mass_violence/stoneman_demo.csv"));
	//io::CSVReader<3>school_demo(getFilePath("mass_violence/cooper_hs_demo.csv"));
	school_demo.read_header(io::ignore_extra_column, "Gender", "Origin", "Count");

	const char *gender = NULL;
	const char *origin = NULL;
	const char *count = NULL;

	while(school_demo.read_row(gender, origin, count))
	{
		int i_gender = ACS::Sex::_from_string(gender);
		int i_origin = ACS::Origin::_from_string(origin);
		
		std::string key_school_demo = std::to_string(i_gender)+std::to_string(i_origin);
		m_schoolDemo.insert(std::make_pair(key_school_demo, std::stoi(count)));
	}
}

void ViolenceParams::readMassViolenceInputs()
{
	io::CSVReader<2>social_network_params(getFilePath("mass_violence/mass_violence_input.csv"));
	social_network_params.read_header(io::ignore_extra_column, "Variable", "Value");

	const char* var = NULL;
	const char* val = NULL;

	MapDbl m_massViolenceParams;
	while(social_network_params.read_row(var, val))
		m_massViolenceParams.insert(std::make_pair(var, std::stod(val)));

	setViolenceParams(&m_massViolenceParams);
}

void ViolenceParams::readPtsdSymptoms()
{
	io::CSVReader<8>ptsdx_file(getFilePath("mass_violence/ptsdx_strata.csv"));
	ptsdx_file.read_header(io::ignore_extra_column, 
		"Gender", "Age_Cat", "ptsd_type", "ptsd_case", 
		"first_ptsdx", "second_ptsdx", "third_ptsdx", "fourth_ptsdx");

	const char* s_gender = NULL;
	const char* s_age = NULL;
	const char* s_ptsd_type = NULL;
	const char* s_ptsd_case = NULL;
	const char* s_ptsdx_first = NULL;
	const char* s_ptsdx_second = NULL;
	const char* s_ptsdx_third = NULL;
	const char* s_ptsdx_fourth = NULL;
	
	std::string key = "";
	//PairDD ptsdx;
	Tuple ptsdx;
	while(ptsdx_file.read_row(s_gender, s_age, s_ptsd_type, s_ptsd_case, 
		s_ptsdx_first, s_ptsdx_second, s_ptsdx_third, s_ptsdx_fourth))
	{
		int i_gender = Violence::Sex::_from_string(s_gender);
		int i_age = Violence::AgeCat::_from_string(s_age);

		key = std::to_string(i_gender)+std::to_string(i_age)+s_ptsd_type+s_ptsd_case;
		ptsdx = std::make_tuple(std::stod(s_ptsdx_first), std::stod(s_ptsdx_second), 
			std::stod(s_ptsdx_third), std::stod(s_ptsdx_fourth));

		m_ptsdx_.insert(std::make_pair(key, ptsdx));
	}
}

void ViolenceParams::setViolenceParams(MapDbl *m_param)
{
	vParams.sim_case = (int)m_param->at("case");
	vParams.inner_draws = (int)m_param->at("inner_draws");
	vParams.outer_draws = (int)m_param->at("outer_draws");
	vParams.min_households_puma = (int)m_param->at("min_households_puma");
	vParams.mean_friends_size = (int)m_param->at("mean_friends_size");
	vParams.num_teachers = (int)m_param->at("num_teachers");
	vParams.min_age = (int)m_param->at("min_age");
	vParams.age_diff_students =(int)m_param->at("age_diff_students");
	vParams.age_diff_others = (int)m_param->at("age_diff_others");
	vParams.p_val_student = m_param->at("p_val_student");
	vParams.p_val_teacher = m_param->at("p_val_teacher");
	vParams.p_val_origin = m_param->at("p_val_origin");
	vParams.p_val_edu = m_param->at("p_val_edu");
	vParams.p_val_age = m_param->at("p_val_age");
	vParams.p_val_gender = m_param->at("p_val_gender");
	vParams.aff_students = (int)m_param->at("aff_students");
	vParams.aff_teachers = (int)m_param->at("aff_teachers");
	vParams.prev_students_tot = m_param->at("prev_students_tot");
	vParams.prev_teachers_female = m_param->at("prev_teachers_female");
	vParams.prev_teachers_male = m_param->at("prev_teachers_male");
	vParams.prev_fam_female = m_param->at("prev_fam_female");
	vParams.prev_fam_male = m_param->at("prev_fam_male");
	vParams.prev_comm_female = m_param->at("prev_comm_female");
	vParams.prev_comm_male = m_param->at("prev_comm_male");
	vParams.ptsd_cutoff = m_param->at("ptsd_cutoff");
	vParams.sec_coeff = m_param->at("sec_coeff");
	vParams.ter_coeff = m_param->at("ter_coeff");
	vParams.screening_time = (int)m_param->at("screening_time");
	vParams.sensitivity = m_param->at("sensitivity");
	vParams.specificity = m_param->at("specificity");
	vParams.tot_steps = (int)m_param->at("tot_steps");
	vParams.treatment_time = (int)m_param->at("treatment_time");
	vParams.num_trials = (int)m_param->at("num_trials");
	vParams.cbt_dur_non_cases = (int)m_param->at("cbt_dur_non_cases");
	vParams.max_cbt_sessions = (int)m_param->at("max_cbt_sessions");
	vParams.max_spr_sessions = (int)m_param->at("max_spr_sessions");
	vParams.cbt_coeff = m_param->at("cbt_coeff");
	vParams.spr_coeff = m_param->at("spr_coeff");
	vParams.nd_coeff = m_param->at("nd_coeff");
	vParams.cbt_cost = (int)m_param->at("cbt_cost");
	vParams.spr_cost = (int)m_param->at("spr_cost");
	vParams.percent_nd = m_param->at("percent_nd");
	vParams.nd_dur = (int)m_param->at("nd_dur");
	vParams.ptsdx_relapse = m_param->at("ptsdx_relapse");
	vParams.time_relapse = (int)m_param->at("time_relapse");
	vParams.num_relapse = (int)m_param->at("num_relapse");
	vParams.percent_relapse = m_param->at("percent_relapse");
	vParams.dw_mild = m_param->at("dw_mild");
	vParams.dw_moderate = m_param->at("dw_moderate");
	vParams.dw_severe = m_param->at("dw_severe");
	vParams.discount = m_param->at("discount");

	setMediaDistParams(m_param);
	setTvCoverageDistParams(m_param);
	setSocialMediaHoursDistParams(m_param);
	setPrevalenceDistParams(m_param);
	setPrevalenceDistSocialMediaParams(m_param);
	setOddRatioParams(m_param);
}

void ViolenceParams::setMediaDistParams(MapDbl * m_param)
{
	std::string ageGroups [] = {"18_29", "30_49", "50_64", "65_over"};
	std::string newsSource [] =	{"other_news", "tv_news", "online_news", "tv_online_news"};
	
	std::vector<std::pair<double, int>> v_mediaDist;
	std::string keyNewsSource = "";

	int ageIdx = 0;
	double pMediaDist;

	for(auto ageCat : Violence::AgeCat2::_values())
	{
		if(ageCat == (int)Violence::AgeCat2::Age_14_17)
			continue;

		for(auto source : Violence::Source::_values())
		{
			keyNewsSource = "p_" + newsSource[source] + "_" + ageGroups[ageIdx];
			pMediaDist = m_param->at(keyNewsSource);

			v_mediaDist.push_back(std::make_pair(pMediaDist, source));
		}

		vParams.news_source_dist[ageCat] = v_mediaDist;
		v_mediaDist.clear();

		ageIdx++;
	}
}

void ViolenceParams::setTvCoverageDistParams(MapDbl * m_param)
{
	vParams.p_tv_time.push_back(std::make_pair(m_param->at("p_tv_coverage_4hrs"), Violence::Coverage::Hours_Less_4));
	vParams.p_tv_time.push_back(std::make_pair(m_param->at("p_tv_coverage_4_7hrs"), Violence::Coverage::Hours_4_7));
	vParams.p_tv_time.push_back(std::make_pair(m_param->at("p_tv_coverage_8_11hrs"), Violence::Coverage::Hours_8_11));
	vParams.p_tv_time.push_back(std::make_pair(m_param->at("p_tv_coverage_12hrs"), Violence::Coverage::Hours_12_More));
}

void ViolenceParams::setSocialMediaHoursDistParams(MapDbl *m_param)
{
	vParams.p_social_media_hours.push_back(std::make_pair(m_param->at("p_social_media_none"), Violence::SocialMediaHours::None));
	vParams.p_social_media_hours.push_back(std::make_pair(m_param->at("p_social_media_less_2hrs"), Violence::SocialMediaHours::Hours_2_Less));
	vParams.p_social_media_hours.push_back(std::make_pair(m_param->at("p_social_media_more_2hrs"), Violence::SocialMediaHours::Hours_2_More));
}

void ViolenceParams::setPrevalenceDistParams(MapDbl * m_param)
{
	vParams.ptsd_prev_tv_time[std::to_string(Violence::Coverage::Hours_Less_4)] = m_param->at("ptsd_prev_tv_4hrs");
	vParams.ptsd_prev_tv_time[std::to_string(Violence::Coverage::Hours_4_7)] = m_param->at("ptsd_prev_tv_4_7hrs");
	vParams.ptsd_prev_tv_time[std::to_string(Violence::Coverage::Hours_8_11)] = m_param->at("ptsd_prev_tv_8_11hrs");
	vParams.ptsd_prev_tv_time[std::to_string(Violence::Coverage::Hours_12_More)] = m_param->at("ptsd_prev_tv_12hrs");
}

void ViolenceParams::setPrevalenceDistSocialMediaParams(MapDbl *m_param)
{
	vParams.ptsd_prev_social_media[std::to_string(Violence::SocialMediaHours::None)] = m_param->at("ptsd_prev_social_media_none");
	vParams.ptsd_prev_social_media[std::to_string(Violence::SocialMediaHours::Hours_2_Less)] = m_param->at("ptsd_prev_social_media_2hrs_less");
	vParams.ptsd_prev_social_media[std::to_string(Violence::SocialMediaHours::Hours_2_More)] = m_param->at("ptsd_prev_social_media_2hrs_more");
}

void ViolenceParams::setOddRatioParams(MapDbl *m_param)
{
	vParams.odd_ratio[std::to_string(Violence::Scenario::First)] = m_param->at("odd_ratio_tv_only");
	//vParams.odd_ratio[std::to_string(Violence::Scenario::Social_Media_Only)] = m_param->at("odd_ratio_social_media_only");
	vParams.odd_ratio[std::to_string(Violence::Scenario::Second)] = m_param->at("odd_ratio_tv_social_media_posts");
	vParams.odd_ratio[std::to_string(Violence::Scenario::Third)] = m_param->at("odd_ratio_tv_video_sharing"); 
}


Parameters::MapInt ViolenceParams::getSchoolDemographics() 
{
	return m_schoolDemo;
}


Parameters::PairTuple * ViolenceParams::getPtsdSymptoms() 
{
	return &m_ptsdx_;
}


const MVS::Violence* ViolenceParams::getViolenceParam()
{
	return &vParams;
}
