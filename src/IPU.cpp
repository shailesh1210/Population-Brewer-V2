#include "IPU.h"
#include "ACS.h"
#include "ViolenceParams.h"
#include "CardioParams.h"
#include "DepressionParams.h"

#define MAX_ITERATIONS 4000

template class IPU<ViolenceParams>;
template class IPU<CardioParams>;
template class IPU<DepressionParams>;

template<class GenericParams>
IPU<GenericParams>::IPU(HouseholdsMap *m_hhPUMS, const std::vector<double>& ipuCons, bool print) : 
	m_households(m_hhPUMS), cons(ipuCons), eps(1e-3), printOutput(print), ipu_success(false)
{
}

template<class GenericParams>
IPU<GenericParams>::~IPU()
{

}

template<class GenericParams>
void IPU<GenericParams>::start()
{
	initialize();
	solve(freqMatrix.n_rows, freqMatrix.n_cols);
	computeProbabilities();
	roundWeights(m_hhCount);
	clear();
}

template<class GenericParams>
bool IPU<GenericParams>::success()
{
	return ipu_success;
}

template<class GenericParams>
const typename IPU<GenericParams>::ProbMap *IPU<GenericParams>::getHHProbability() const
{
	return &m_hhProbs;
}

template<class GenericParams>
double IPU<GenericParams>::getHHCount(std::string hhType) const
{
	if(m_hhCount.count(hhType) > 0)
		return m_hhCount.at(hhType);
	else
		return -1;
}

template<class GenericParams>
void IPU<GenericParams>::clearMap()
{
	m_hhCount.clear();
	m_hhProbs.clear();
	m_idx.clear();
}

template<class GenericParams>
void IPU<GenericParams>::initialize()
{
	int num_rows, num_cols;
	num_rows = m_households->size();
	mapIndexByType(num_cols);

	//freqMatrix = zeros<mat>(num_rows, num_cols);

	freqMatrix.set_size(num_rows, num_cols);

	weights.set_size(num_rows);
	weights.fill(1);

	int rowIdx = 0;
	std::string hhType, hhSize, hhIncCat;
	std::string sex, ageCat, origin, edu;
	int hhColIdx, perColIdx;

	std::vector<PersonPums<GenericParams>>personList;
	
	for(auto hh = m_households->begin(); hh != m_households->end(); ++hh)
	{
		hhType = std::to_string(hh->second.getHouseholdType());
		hhSize = std::to_string(hh->second.getHouseholdSize());
		hhIncCat = std::to_string(hh->second.getHouseholdIncCat());

		hhColIdx = m_idx.at(ACS::Index::Household_GQ).at(hhType+hhSize+hhIncCat);
		freqMatrix(rowIdx, hhColIdx) = 1.0;
	
		personList = hh->second.getPersons();
		for(auto pp = personList.begin(); pp != personList.end(); ++pp)
		{
			sex = std::to_string(pp->getSex());
			
			origin = std::to_string(pp->getOrigin());
			edu = std::to_string(pp->getEducation());

			if(pp->getAge() < 18)
			{
				ageCat = std::to_string(pp->getAgeCat());
				perColIdx = m_idx.at(ACS::Index::Child).at(sex+ageCat+origin);
			}
			else 
			{
				ageCat = std::to_string(pp->getEduAgeCat());
				perColIdx = m_idx.at(ACS::Index::Adult).at(sex+ageCat+origin+edu);
			}

			freqMatrix(rowIdx, perColIdx) += 1;
		}

		rowIdx++;
	}	

	mapNonZeroRowIndex(num_cols);

}

template<class GenericParams>
void IPU<GenericParams>::solve(int row_size, int col_size)
{
	if(col_size != cons.size()){
		std::cout << "Error: Column size of freq. matrix doesn't match constraints size!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	if(row_size != weights.size()){
		std::cout << "Error: Row size of freq. matrix doesn't match weights size!" << std::endl;
		exit(EXIT_SUCCESS);
	}

	vec gamma_vals(col_size);
	vec gamma_vals_new(col_size);
	
	double gamma, gamma_new, delta;
	double col_weighted_sum;
	std::vector<double>colSum(col_size);

	int non_zeros = 0;
	double sum_gamma = 0;
	for(int i = 0; i < col_size; ++i)
	{
		//col_weighted_sum = sum(freqMatrix.col(i)%weights);
		col_weighted_sum = getColWeightSum(i);
		colSum[i] = col_weighted_sum;
		if(col_weighted_sum != 0) //&& cons[i] > 0.01)
		{
			gamma_vals[i] = (fabs(col_weighted_sum-cons[i]))/cons[i];
			sum_gamma += gamma_vals[i];
			non_zeros++;
		}
		else
			gamma_vals[i] = 0;
	}

	//gamma = mean(gamma_vals);
	std::cout << "Total number of non-zero columns: " << non_zeros << std::endl;
	gamma = sum_gamma/non_zeros;

	bool run_ipu = true;
	int iterations = 0;

	std::vector<int>rowsIdxList;

	while(run_ipu && iterations <= MAX_ITERATIONS)
	{
		iterations++;
		for(int j = 0; j < col_size; ++j)
		{
			//col_weighted_sum = sum(freqMatrix.col(j)%weights);
			col_weighted_sum = getColWeightSum(j);
			colSum[j] = col_weighted_sum;
			if(col_weighted_sum != 0)
			{
				double ratio = cons[j]/col_weighted_sum;
				rowsIdxList = m_nonZeroIdx.at(j);

				for(size_t k = 0; k < rowsIdxList.size(); ++k)
					weights(rowsIdxList[k]) = ratio*weights(rowsIdxList[k]);
			}
		}

		
		double sum_gamma_new = 0;
		for(int i = 0; i < col_size; ++i)
		{
			//col_weighted_sum = sum(freqMatrix.col(i)%weights);
			col_weighted_sum = getColWeightSum(i);
			if(col_weighted_sum != 0) //&& cons[i] > 0.01)
			{
				gamma_vals_new[i] = (fabs(col_weighted_sum-cons[i]))/cons[i];
				sum_gamma_new += gamma_vals_new[i];
			}
			else
				gamma_vals_new[i] = 0;
		}

		gamma_new = sum_gamma_new/non_zeros;
		//gamma_new = mean(gamma_vals_new);
		
		delta = fabs(gamma_new-gamma);

		if(printOutput)
			std::cout << "Improvement run in " << iterations << ":" << std::setprecision(8) 
			<< "|gamma_new = " << gamma_new << "|gamma = " << gamma << std::endl;


		if(gamma_new < eps)
		{
			if(printOutput)
				std::cout << "Ipu completed after " << iterations << " iterations!\n" << std::endl; 
			run_ipu = false;
			ipu_success = true;
		}
		else if(delta < eps/1000)
		{
			std::cout << std::endl;
			std::cout << "Corner solution reached!\n" << std::endl;
				
			int new_col_size = m_idx.at(ACS::Index::Household_GQ).size();
			vec hh_cons(new_col_size);
			for(int i = 0; i < new_col_size; ++i)
				hh_cons(i) = cons(i);

			cons.resize(new_col_size);
			cons = hh_cons;

			solve(row_size, new_col_size);
			run_ipu = false;
		}
		else{
			for(int i = 0; i < col_size; ++i)
				gamma_vals[i] = gamma_vals_new[i];

			gamma = gamma_new;
		}

		if(iterations > MAX_ITERATIONS)
			std::cout << "WARNING: Convergence not achieved!\n" << std::endl;
	}

}

template<class GenericParams>
void IPU<GenericParams>::mapIndexByType(int &num_cols)
{
	int idx = 0;
	
	std::map<std::string, int> m_hhIdx, m_childIdx, m_adultIdx;

	//mapping between household/GQ types and freq. matrix index
	std::string gqType = "-1";
	std::string gqSize = "1";
	std::string gqInc = "-1";

	m_hhIdx.insert(std::make_pair(gqType+gqSize+gqInc, idx++));

	for(auto hhType : ACS::HHType::_values())
		for(auto hhSize : ACS::HHSize::_values())
			for(auto hhInc : ACS::HHIncome::_values())
				m_hhIdx.insert(std::make_pair(std::to_string(hhType)+std::to_string(hhSize)+std::to_string(hhInc), idx++));

	m_idx.insert(std::make_pair(ACS::Index::Household_GQ, m_hhIdx));

	//mapping between person (children) types and freq. matrix index;
	for(auto sex : ACS::Sex::_values())
		for(size_t ageCat = ACS::AgeCat::Age_0_4; ageCat <= ACS::AgeCat::Age_15_17; ++ageCat)
			for(auto org : ACS::Origin::_values())
				m_childIdx.insert(std::make_pair(std::to_string(sex)+std::to_string(ageCat)+std::to_string(org), idx++));

	m_idx.insert(std::make_pair(ACS::Index::Child, m_childIdx));

	//mapping between person (adults) types and freq. matrix index;
	for(auto sex : ACS::Sex::_values())
		for(auto eduAgeCat : ACS::EduAgeCat::_values())
			for(auto org : ACS::Origin::_values())
				for(auto edu : ACS::Education::_values())
					m_adultIdx.insert(std::make_pair(std::to_string(sex)+std::to_string(eduAgeCat)+std::to_string(org)+std::to_string(edu), idx++));

	m_idx.insert(std::make_pair(ACS::Index::Adult, m_adultIdx));
	num_cols = (m_hhIdx.size()+m_childIdx.size()+m_adultIdx.size());
}

template<class GenericParams>
void IPU<GenericParams>::mapNonZeroRowIndex(int num_cols)
{
	std::vector<int>v_idx(0);
	for(int i = 0; i < num_cols; ++i)
		m_nonZeroIdx.insert(std::make_pair(i, v_idx));

	sp_mat::const_iterator start = freqMatrix.begin();
	sp_mat::const_iterator end   = freqMatrix.end();

	for(sp_mat::const_iterator it = start; it != end; ++it)
	{
		if(m_nonZeroIdx.count(it.col()) > 0)
			m_nonZeroIdx.at(it.col()).push_back(it.row());
	}
}

template<class GenericParams>
double IPU<GenericParams>::getColWeightSum(int colIdx)
{
	double sum = 0; 

	std::vector<int> rowsIdxList(m_nonZeroIdx.at(colIdx));
	for(size_t i = 0; i < rowsIdxList.size(); ++i)
		sum += (freqMatrix(rowsIdxList[i], colIdx) * weights(rowsIdxList[i]));
	
	return sum;
}

template<class GenericParams>
void IPU<GenericParams>::computeProbabilities()
{
	std::vector<PairDD> weight_hhId;
	std::map<std::string, std::vector<PairDD>> m_hhWeights;

	std::string gqType = "-1";
	std::string gqSize = "1";
	std::string gqIncCat = "-1";

	m_hhWeights.insert(std::make_pair(gqType+gqSize+gqIncCat, weight_hhId));

	for(auto hhType : ACS::HHType::_values())
	{
		for(auto hhSize : ACS::HHSize::_values())
		{
			for(auto hhInc : ACS::HHIncome::_values())
			{
				std::string type = std::to_string(hhType)+std::to_string(hhSize)+std::to_string(hhInc);
				m_hhWeights.insert(std::make_pair(type, weight_hhId));
			}
		}
	}

	int idx = 0;
	double hhIdx;
	std::string hhTypeStr, hhSizeStr, hhIncCatStr;
	for(auto hh = m_households->begin(); hh != m_households->end(); ++hh)
	{
		hhIdx = hh->first;
		hhTypeStr = std::to_string(hh->second.getHouseholdType());
		hhSizeStr = std::to_string(hh->second.getHouseholdSize());
		hhIncCatStr = std::to_string(hh->second.getHouseholdIncCat());

		auto range = m_hhWeights.equal_range(hhTypeStr+hhSizeStr+hhIncCatStr);
		for(auto wt = range.first; wt != range.second; ++wt)
			wt->second.push_back(PairDD(weights(idx), hhIdx));

		idx++;
	}

	double sum_weights = 0;

	int start = 5;
	int end = 100;

	std::map<double, std::vector<PairDD>> hhProbHash;
	std::vector<PairDD> tempHHPair;
	double d_hash = 0;
	
	//adds household probabilties to buckets
	for(auto p_vec = m_hhWeights.begin(); p_vec != m_hhWeights.end();)
	{
		if(p_vec->second.size() == 0)
		{
			p_vec = m_hhWeights.erase(p_vec);
			continue;
		}

		std::vector<double>tempWts;
		for(size_t i = 0; i < p_vec->second.size(); ++i)
			tempWts.push_back(p_vec->second.at(i).first);

		sum_weights = std::accumulate(tempWts.begin(), tempWts.end(), 0.0);
		m_hhCount.insert(std::make_pair(p_vec->first, sum_weights));

		for(size_t j = 0; j < tempWts.size(); ++j)
			tempWts.at(j) = (sum_weights != 0) ? tempWts.at(j)/sum_weights : 0.0;

		std::partial_sum(tempWts.begin(), tempWts.end(), tempWts.begin());

		for(size_t k = 0; k < p_vec->second.size(); ++k)
		{
			p_vec->second.at(k).first = tempWts.at(k);
			for(int hash = start; hash <= end; hash += start)
			{
				d_hash = (double)hash/100;
				hhProbHash.insert(std::make_pair(d_hash, tempHHPair));
				if(p_vec->second.at(k).first <= d_hash)
				{
					hhProbHash[d_hash].push_back(p_vec->second.at(k));
					break;
				}
			}
		}

		for(auto hash = hhProbHash.begin(); hash != hhProbHash.end();)
		{
			if(hash->second.size() == 0)
				hash = hhProbHash.erase(hash);
			else
				++hash;
		}

		m_hhProbs.insert(std::make_pair(p_vec->first, hhProbHash));
		++p_vec;	

		hhProbHash.clear();
	}

}

template<class GenericParams>
void IPU<GenericParams>::roundWeights(std::map<std::string, double> &m_hhCount)
{
	double adj = 0;
	for(auto hh = m_hhCount.begin(); hh != m_hhCount.end(); ++hh)
	{
		 double diff = adj+(hh->second-floor(hh->second));
		 if(diff >= 0.5){
			 adj = diff-1;
			 hh->second = ceil(hh->second);
		 }
		 else{
			 adj = diff;
			 hh->second = floor(hh->second);
		 }
	}
}

template<class GenericParams>
void IPU<GenericParams>::clear()
{
	freqMatrix.clear();
	cons.clear();
	weights.clear();
	m_idx.clear();
}



