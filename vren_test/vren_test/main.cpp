#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include "app.hpp"

#ifdef VREN_TEST_UNIT_TESTS
int main(int argc, char* argv[])
{
    std::srand(std::time(NULL));

	::testing::GTEST_FLAG(catch_exceptions) = false;

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
#endif

#ifdef VREN_TEST_BENCHMARK
int main(int argc, char** argv)
{
    std::srand(std::time(NULL));

    ::benchmark::Initialize(&argc, argv);
    
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    {
        return 1;
    }

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;                                                          
}
#endif
