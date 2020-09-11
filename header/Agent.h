#ifndef __Agent_h__
#define __Agent_h__

template<class GenericParams>
class PersonPums;


template<class GenericParams>
class Agent
{
public:
	Agent();
	Agent(const PersonPums<GenericParams> *p);
	~Agent();

	void setHouseholdID(double);
	void setPUMA(int);
	void setAge(short int);
	void setSex(short int);
	void setOrigin(short int);
	void setEducation(short int);
	void setIncome(double);

	double getHouseholdID() const;
	int getPUMA() const;
	short int getAge() const;
	short int getSex() const;
	short int getOrigin() const;
	short int getEducation() const;
	double getIncome() const;

protected:
	double householdID;
	int puma;
	short int age, sex;
	short int origin, education;
	double income;
};

#endif