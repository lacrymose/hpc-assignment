/**
 * @file Source.cpp
 * @brief Main program flow.
 */

#include <iostream>
#include <fstream>
#include <algorithm>

#include "ConfigurationLoader.h"
#include "Configuration.h"

#include "EigenCrankNicolsonDiscretizator.h"
#include "DiscretizationResult.h"
#include "Exception.h"
#include "AnalyticalFunctions.h"
#include "ConfigurationLoadingException.h"
#include "MPIWrapper.h"

#define PARAMETER_COUNT 4

#define CONFIGURATION_FILE_INDEX 1
#define WAVE_FILE_INDEX 2
#define NORM_FILE_INDEX 3

/**
* @brief Main entry point for application.
* Requires 4 additional parameters to run:
*			- Name of configuration file.
*			- Name for output wave file.
*			- Name for output norm file.
* @param argc Parameter count.
* @param argv Array of arguments.
* @return int Zero if program executed correctly.
*/
int main(int argc, char * argv[])
{
	try
	{
		if (argc != PARAMETER_COUNT)
		{
			std::cout << "Invalid amount of parameters." << std::endl;
			return -1;
		}

		MPIWrapper::init(&argc, &argv);
		double begin, end;
		long coreId = MPIWrapper::getCoreId();
		long coresQuantity = MPIWrapper::getQuantityOfCores();
		char * configurationFile = argv[CONFIGURATION_FILE_INDEX];
		char * wavesFile = argv[WAVE_FILE_INDEX];
		char * normsFile = argv[NORM_FILE_INDEX];

		ConfigurationLoader configurationLoader;
		Configuration * configuration = configurationLoader.loadFromFile(configurationFile);

		double dx = (configuration->upperBound - configuration->lowerBound) / configuration->numberOfPoints;
		double dt = configuration->cfl / configuration->acceleration * dx;


		EigenCrankNicolsonDiscretizator discretizator(new DiscretizationParameters(
			configuration->lowerBound,
			configuration->upperBound,
			configuration->acceleration,
			configuration->numberOfPoints,
			AnalyticalFunctions::expFunction,
			NULL,
			configuration->timeLevels,
			dt,
			dx
			));

		begin = MPIWrapper::getTime();

		DiscretizationResult * result = discretizator.discretize();

		end = MPIWrapper::getTime();

		if(coreId == 0)
		{
			std::cout << "Calculated in: " << (end - begin) << std::endl;
			std::fstream ws;
			ws.open(wavesFile, std::fstream::out | std::fstream::trunc);
			WavesSummary * pointsAtTimeT = result->getValuesAtTimeT();
			ws << *pointsAtTimeT;
			ws.close();

			std::fstream ns;
			ns.open(normsFile, std::fstream::out | std::fstream::trunc);
			NormSummary * norms = result->getNorms();
			std::cout << *norms;
			ns << *norms << std::endl;
			ns << "Calculated in: " << (end - begin) << std::endl;
			ns.close();
			delete result;
			delete norms;
		}

		delete configuration;
	}
	catch (Exception e)
	{
		std::cout << "Exception" << std::endl;
	}

	MPIWrapper::finalize();
	return 0;
}
