#include "error.hpp"
#include "config.hpp"
#include "connection.hpp"
#include "query.hpp"
#include "benchmark/benchmark.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>

enum class ConnectionSuccess {
    SUCCESS,
	PRIMARY_TRANSIENT_FAILURE,
    PRIMARY_PERMANENT_FAILURE,
};

void program(benchmark::State& state, std::string configFilePath, std::string queryFilePath, int queryExecuteCount, ConnectionSuccess connectionSuccess = ConnectionSuccess::SUCCESS, int failureCount = 0) {
    try {
        AppConfig appConfig = ConfigLoader::loadConfig(configFilePath);

        ConnectionManager connectionManager = ConnectionManager(appConfig);
        connectionManager.establishConnection();
        QueryEngine queryEngine = QueryEngine(connectionManager);

		std::vector<Query> baseQueries = queryEngine.parseQueriesFromFile(queryFilePath);
        std::vector<Query> queriesToRun;
        for (int i = 0; i < queryExecuteCount; ++i) {
           for (const auto& query : baseQueries) {
                queriesToRun.push_back(query);
		   }
		}

        if (connectionSuccess == ConnectionSuccess::PRIMARY_TRANSIENT_FAILURE) {
            connectionManager.setSimulatedFailureMode("primary", failureCount, true); // Simulate transient failure
        } else if (connectionSuccess == ConnectionSuccess::PRIMARY_PERMANENT_FAILURE) {
            connectionManager.setSimulatedFailureMode("primary", failureCount, false); // Simulate permanent failure
		}

        for (auto _ : state) {
            if (connectionManager.isConnected()) {
                std::vector<QueryResult> results = queryEngine.executeQueries(queriesToRun);
			    /*for (const auto& result : results) {
				    result.print();
			    }*/
            }
        }

    }
    catch (const ParseError& e) {
        std::cerr << "FATAL [Main]: Configuration Parse Error - " << e.what() << std::endl;
    }
    catch (const ValidationError& e) {
        std::cerr << "FATAL [Main]: Configuration Validation Error - " << e.what() << std::endl;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "FATAL [Main]: Runtime Error - " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL [Main]: Unexpected Exception - " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "FATAL [Main]: Unknown Exception" << std::endl;
    }
}

BENCHMARK_CAPTURE(program, success100_100, "configs/example_primary.cfg", "queries/success100.txt", 25);
BENCHMARK_CAPTURE(program, success75_100, "configs/example_primary.cfg", "queries/success75.txt", 25);
BENCHMARK_CAPTURE(program, success50_100, "configs/example_primary.cfg", "queries/success50.txt", 25);
BENCHMARK_CAPTURE(program, success25_100, "configs/example_primary.cfg", "queries/success25.txt", 25);
BENCHMARK_CAPTURE(program, success0_100, "configs/example_primary.cfg", "queries/success0.txt", 25);

BENCHMARK_CAPTURE(program, success100_500, "configs/example_primary.cfg", "queries/success100.txt", 125);
BENCHMARK_CAPTURE(program, success75_500, "configs/example_primary.cfg", "queries/success75.txt", 125);
BENCHMARK_CAPTURE(program, success50_500, "configs/example_primary.cfg", "queries/success50.txt", 125);
BENCHMARK_CAPTURE(program, success25_500, "configs/example_primary.cfg", "queries/success25.txt", 125);
BENCHMARK_CAPTURE(program, success0_500, "configs/example_primary.cfg", "queries/success0.txt", 125);

BENCHMARK_CAPTURE(program, success100_1000, "configs/example_primary.cfg", "queries/success100.txt", 250);
BENCHMARK_CAPTURE(program, success75_1000, "configs/example_primary.cfg", "queries/success75.txt", 250);
BENCHMARK_CAPTURE(program, success50_1000, "configs/example_primary.cfg", "queries/success50.txt", 250);
BENCHMARK_CAPTURE(program, success25_1000, "configs/example_primary.cfg", "queries/success25.txt", 250);
BENCHMARK_CAPTURE(program, success0_1000, "configs/example_primary.cfg", "queries/success0.txt", 250);
BENCHMARK_MAIN();