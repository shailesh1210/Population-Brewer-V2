#ifndef __DepressionParams_h__
#define __DepressionParams_h__

#include "Parameters.h"
#include <map>
#include <string>

class DepressionParams : public Parameters
{
public:
	typedef std::map<int, std::map<int, int>> IntMap;
	typedef std::map<int, IntMap> HashMap;
	typedef std::pair<double, int> PairDblInt;
	typedef std::vector<PairDblInt> VecPairDblInt;
	typedef std::map<std::string, VecPairDblInt> MapVecPair;

	DepressionParams();
	DepressionParams(const char *, const char *, int, int);

	virtual ~DepressionParams();

	void setWageGapProbabilityDist(MapVecPair);

	int getPovertyThreshold(int, int, int);
	const VecPairDblInt *getIncomePovertyRatioTag() const;
	const VecPairDblInt *getDepressionPrevalence(std::string) const;
	const Tuple *getDepressionSymptoms(std::string) const;
	const VecPairDblInt *getWageGapProbabilityDist(std::string) const;

private:
	void readPovertyThreshold();
	void readDepressionPrevalence();
	void readDepressionSymptoms();

	void tagIncomePovertyRatio();

	std::string getPersonTypeByIPRatio(const char *, const char *, const char *);
	std::string getPersonTypeByDepressionType(const char *, const char *, const char *, const char*);

	HashMap m_povertyThreshhold;
	MapVecPair m_depressionPrevalence, m_pWageGap; 
	PairTuple m_depressionSymptoms;
	VecPairDblInt v_ipRatio_ipTag;
};
#endif