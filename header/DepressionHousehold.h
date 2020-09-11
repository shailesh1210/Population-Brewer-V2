#ifndef __DepressionHousehold_h__
#define __DepressionHousehold_h__

#include <vector>
#include "DepressionAgent.h"

template <class GenericParams>
class HouseholdPums;

class DepressionParams;

class DepressionHousehold
{
public:
	typedef std::vector<DepressionAgent *> FamilyMembers;

	DepressionHousehold();
	DepressionHousehold(const HouseholdPums<DepressionParams> *);

	virtual ~DepressionHousehold();

	void addMemebers(DepressionAgent *);

	void setIncomeToPovertyRatio();

	short int getHouseholdSize() const;
	short int getHouseholdType() const;
	double getHouseholdIncome() const;
	short int getNumChildren() const;
	short int getTotalPersons() const;

	const FamilyMembers *getMembers() const;
	
	
private:
	int getHouseholderAgeCat() const;

	void setIncomeToPovertyNonFamily();
	void setIncomeToPovertyFamily();

	short int hhSize, hhType, numChildren;
	short int totalPersons;
	double hhIncome;

	FamilyMembers members;
};
#endif