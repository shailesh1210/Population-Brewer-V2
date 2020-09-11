/**
*	@file	 main.cpp	
*	@author	 Shailesh Tamrakar
*	@date	 7/23/2018
*	@version 1.0
*
*	@section DESCRIPTION
*	The main purpose of this program is to generate a synthetic population
*	to assess the impact of tax intervention (soda-tax) on Cardio-Vascular
*	Disease (CVD) among economically derived population across US MSAs.
*	It imports publicily available PUMS (Public Use Microdata Sample) 
*	dataset as well as ACS (American Community Survey) estimates of socio-
*	demographic variables as input parameters, followed by execution
*	of IPU algorithm to create a synthetic population by simultaneously 
*	matching both household and person-level attributes/estimates.
*   This model has been extended to include two versions of Mass-violence 
*   simulation model. 
*   
*	 
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include "CardioModel.h"
#include "ViolenceModel.h"
#include "DepressionModel.h"
#include "csv.h"
#include "IPU.h"
#include "ACS.h"

#include "DepressionParams.h"


int main(int argc, const char* argv[])
{
	std::cout << "*******************************************************************************\n";
	std::cout << "************Welcome to Population Brewer***************************************\n";
	std::cout << "This software generates synthetic population for US Metropolitan Areas(States)\n";
	std::cout << "*******************************************************************************\n";

	std::cout << std::endl;

	std::vector<const char*> arguments;
	const int NUM_ARGUMENTS = 6;

	if(argc < NUM_ARGUMENTS)
	{
		std::cout << "Program usage format\n";
		std::cout << "Program name[Synthetic Pop] Input Directory[input] Output Directory[output] Simulation Type[PMH=3 or MVS=2 or EET=1] Geo Level[MSA=1/State=2] Interactive[0 or 1] state ID(1-51)"
			<< std::endl;
		exit(EXIT_SUCCESS);
	}

	for(int i = 0; i < argc; i++) {
		arguments.push_back(argv[i]);
	}

	int simType, geoLevel, stateID;
	geoLevel = std::stoi(arguments[4]);

	bool interactive = (std::stoi(arguments[5]) != 0) ? true : false;

	if(geoLevel == 2)
		stateID = std::stoi(arguments.back());
	else
		stateID = -1;

	if(interactive)
	{
		std::cout << "****Available simulation models****" << std::endl;

		std::cout << "1. Equity Efficiency Model" << std::endl;
		std::cout << "2. Mass Violence Model" << std::endl;
		std::cout << "3. Population Mental Health Model \n" << std::endl;

		std::cout << "Please select simulation model (Enter 1, 2 or 3) : ";

		std::cin >> simType; 

		if(std::cin.eof())
			exit(EXIT_SUCCESS);

		while(!std::cin.eof() && !std::cin.good() || simType < EQUITY_EFFICIENCY || simType > POP_MENTAL_HEALTH)
		{
			std::cout << "Invalid input!" << std::endl;
			std::cin.clear();
			std::cin.ignore(256,'\n');

			std::cout << "Please re-enter valid value: ";
			std::cin >> simType;

			if(std::cin.eof())
			exit(EXIT_SUCCESS);
		}
	}
	else
	{
		simType = std::stoi(arguments[3]);
	}

	std::cout << std::endl;

	switch(simType)
	{
	case EQUITY_EFFICIENCY:
		{
			CardioModel *cvdModel = new CardioModel(arguments[1], arguments[2], simType, geoLevel);
			cvdModel->start(stateID);

			delete cvdModel;
			break;
		}

	case MASS_VIOLENCE:
		{
			ViolenceModel *massViolence = new ViolenceModel(arguments[1], arguments[2], simType, geoLevel);
			massViolence->start("Model2");

			delete massViolence;
			break;
		}
	case POP_MENTAL_HEALTH:
		{
			DepressionModel *mentalHealthModel = new DepressionModel(arguments[1], arguments[2], simType, geoLevel);
			mentalHealthModel->start(stateID);

			delete mentalHealthModel;
			break;
		}
	default:
		break;
	}
	
	//delete param;

}