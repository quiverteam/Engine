/*

unitlib2.h

Valve's unitlib is great, but it requires linking to an extra static library
Rather than using static libraries, this just gets included into a translation unit and bam

*/
#pragma once

#ifdef _WIN32
#include <intrin.h>
#endif

#include <immintrin.h>

#ifdef POSIX
#include <x86intrin.h>
#include <unistd.h>
#endif

#include <vector>
#include <list>
#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <math.h>

#define DECLARE_TEST_SUITE(var) extern CUnitTestSuite* var;

#define DEFINE_TEST_SUITE(var, name) CUnitTestSuite* var = new CUnitTestSuite(name)

#define BEGIN_UNIT_TEST(name, suite)\
{	\
	auto __unitlib2__internal__test = CUnitTest(name);

#define TEST_CASE(cond) __unitlib2__internal__test.eval(cond)

#define TEST_ACCURACY(value, expected, max_dev) __unitlib2__internal__test.within_accuracy(value, expected, max_dev)

#define END_UNIT_TEST(suite) \
	suite->insert_test(__unitlib2__internal__test);\
}

class CUnitTest
{
	bool bPassed;
	bool bCompleted;
	std::string Message;
	std::string Name;

public:
	CUnitTest(std::string Name)
	{
		bPassed = false;
		Message = "";
		this->Name = Name;
		bCompleted = false;
	}

	CUnitTest(const CUnitTest& other)
	{
		this->bPassed = other.bPassed;
		this->bCompleted = other.bCompleted;
		this->Name = other.Name;
		this->Message = other.Message;
	}

	CUnitTest(CUnitTest&& other)
	{
		this->bPassed = other.bPassed;
		this->bCompleted = other.bCompleted;
		this->Name = other.Name;
		this->Message = other.Message;
		other.bCompleted = false;
		other.bPassed = false;
		other.Message = "";
		other.Name = "";
	}

	CUnitTest& operator=(const CUnitTest& other)
	{
		this->bPassed = other.bPassed;
		this->bCompleted = other.bCompleted;
		this->Name = other.Name;
		this->Message = other.Message;
		return *this;
	}

	bool passed() const { return bPassed; };

	void eval(bool b) 
	{
		if(bCompleted)
			bPassed = bPassed && b;
		else
			bPassed = b;
	};

	template<class T>
	void within_range(T expected, T val, T deviation)
	{
		T v = val - expected;
		if(bCompleted)
			bPassed = (v <= 0.0 ? (+v) < deviation : v < deviation) && bPassed;
		else
			bPassed = v <= 0.0 ? (+v) < deviation : v < deviation;

		if(!bPassed)
			Message += "Test FAILED: The value " + std::to_string(expected) + " was not within " + std::to_string(deviation)\
			 + " of " + std::to_string(val) + "\nThe value was within " + std::to_string(v) + "\n";
		else
			Message += "Test COMPLETED: The value " + std::to_string(expected) + " was within " +std::to_string(deviation)\
			 + " of " + std::to_string(val) + "\nThe value was within " + std::to_string(v) + "\n"; 
	}

	// Specialization for float
	void within_accuracy(float expected, float val, float percent_dev)
	{
		within_accuracy((long double)expected, (long double)val, (long double)percent_dev);
	}


	// Specialization for double
	void within_accuracy(double expected, double val, double percent_dev)
	{
		within_accuracy((long double)expected, (long double)val, (long double)percent_dev);
	}

	// Tests accuracy of numerical things. Uses long double for best precision
	void within_accuracy(long double expected, long double val, long double percent_dev)
	{
		long double v = (long double)((expected-val) / val) * 100.0;
		if(bCompleted)
			bPassed = (v <= 0.0 ? (+v) < percent_dev : v < percent_dev) && bPassed;
		else
			bPassed = v <= 0.0 ? (+v) < percent_dev : v < percent_dev;
		if(!bPassed)
			Message += "Test FAILED: The value " + std::to_string(expected) + " was not within " + std::to_string(percent_dev * 100.0f)\
			 + "% of " + std::to_string(val) + "\nThe value was within " + std::to_string(fabs(v)) + "%\n";
		else
			Message += "Test COMPLETED: The value " + std::to_string(expected) + " was within " +std::to_string(percent_dev * 100.0f)\
			 + "% of " + std::to_string(val) + "\nThe value was within " + std::to_string(fabs(v)) + "%\n"; 
	}

	// Tests accuracy of integers, this won't have percision issues
	void within_accuracy(long long expected, long long val, long double percent_dev)
	{
		long double v = (long double)((expected-val) / val) * 100.0;
		if(bCompleted)
			bPassed = (v <= 0.0 ? (+v) < percent_dev : v < percent_dev) && bPassed;
		else
			bPassed = v <= 0.0 ? (+v) < percent_dev : v < percent_dev;
		if(!bPassed)
			Message += "Test FAILED: The value " + std::to_string(expected) + " was not within " + std::to_string(percent_dev * 100.0f) + "% of " + std::to_string(val)\
			 + "\nThe value was within " + std::to_string(fabs(v)) + "%\n";
		else
			Message += "Test COMPLETED: The value " + std::to_string(expected) + " was within " +std::to_string(percent_dev * 100.0f) + "% of " + std::to_string(val)\
			 + "\nThe value was within " + std::to_string(fabs(v)) + "%\n"; 
	}

	std::string get_name() const
	{
		return this->Name;
	}

	bool has_message() const
	{
		return this->Message != "";
	}

	std::string get_message() const
	{
		return this->Message;
	}

	virtual void print(std::ostream stream)
	{
		
	}

};

class CUnitTestSuite
{
private:
	std::vector<CUnitTest> Tests;
	std::string Name;
public:
	CUnitTestSuite(const CUnitTestSuite& other)
	{
		this->Name = other.Name;
		this->Tests = other.Tests;
	}

	CUnitTestSuite(CUnitTestSuite&& other)
	{
		this->Name = other.Name;
		this->Tests = other.Tests;
		other.Name = "";
		other.Tests = std::vector<CUnitTest>();
	}

	CUnitTestSuite(std::string name) 
	{
		Name = name;
	}

	// Returns a list of tests
	std::vector<CUnitTest> get_tests() const { return Tests; };

	// Inserts a new test
	void insert_test(CUnitTest test) { Tests.push_back(test); };

	// Returns the number of tests that passed
	int get_pass_count() const
	{
		int passed = 0;
		for(auto x : Tests)
		{
			if(x.passed())
				passed++;
		}
		return passed;
	}

	// Returns the number of tests that failed
	int get_fail_count() const
	{
		int passed = 0;
		for(auto x : Tests)
		{
			if(!x.passed())
				passed++;
		}
		return passed;
	}

	// Returns the total number of tests
	int get_test_count() const { return Tests.size(); };

	// Returns a list of passed tests
	std::vector<CUnitTest> get_passed_tests() const
	{
		std::vector<CUnitTest> passed;
		for(auto x : Tests)
		{
			if(x.passed())
				passed.push_back(x);
		}
		return passed;
	}

	// Returns a list of failed tests
	std::vector<CUnitTest> get_failed_tests() const
	{
		std::vector<CUnitTest> failed;
		for(auto x : Tests)
		{
			if(!x.passed())
				failed.push_back(x);
		}
		return failed;
	}

	// Print test results to stdout
	void print_results() const
	{
		using namespace std;
		printf("=================================\n");
		printf("Results for suite:\n%s\n", this->Name.c_str());
		printf("=================================\n");
		
		for(auto x : Tests)
		{
			printf("\n%s: ", x.get_name().c_str());
			if(x.passed())
				printf("PASSED\n");
			else
				printf("FAILED\n");
			if(x.has_message())
				printf("%s\n", x.get_message().c_str());
		}

		printf("\n=================================\n");
		if(this->get_test_count() == 0)
		{
			printf("No tests completed.\n");
		}
		else
		{
			printf("%i out of %i tests passed.\n", this->get_pass_count(), this->get_test_count());
			printf("%i failed.\n", this->get_fail_count());
			printf("%f%% completed.\n", (float)(this->get_pass_count() / this->get_test_count() * 100.0f));
		}
		printf("=================================\n");
	}

	// Writes the unit tests to and XML file
	void write_xml(std::ostream stream)
	{

	}
};

#define DECLARE_PERF_TEST_SUITE(var) extern CPerfTestSuite* var

#define DEFINE_PERF_TEST_SUITE(var, name) CPerfTestSuite* var = new CPerfTestSuite(name)

//
// Multistep perf tests take multiple values from tests and averages them together.
// Optionally, the test suite can prune outliers that might mess up the measurement.
// Outliers could be caused by context switches, etc.
// This macro will not actually sample anything.
//
#define BEGIN_MULTISTEP_PERF_TEST(name, suite)\
{\
	CSteppedPerfTest __unitlib2__internal__stepped = CSteppedPerfTest(name);

//
// Begins a perf test step
// This will sample things
//
#define BEGIN_PERF_TEST_STEP()\
{\
	unsigned long long __unitlib2__t1 = __rdtsc();

//
// Ends a perf test step
//
#define END_PERF_TEST_STEP()\
	unsigned long long __unitlib2__t2 = __rdtsc();\
	__unitlib2__internal_stepped.add_step(t2-t1);\
}

//
// Ends a multisample performance test
//
#define END_MULTISTEP_PERF_TEST(name, suite)\
	suite->insert_test(__unitlib2__internal__stepped);\
}

//
// Begins a new perf test
//
#define BEGIN_PERF_TEST(name, suite)\
{\
	CPerfTest __unitlib2__internal__perftest = CPerfTest(name);\
	unsigned long long __unitlib2__t1 = __rdtsc();

//
// Ends the perf test
//
#define END_PERF_TEST(suite) \
	unsigned long long __unitlib2__t2 = __rdtsc();\
	__unitlib2__internal_perftest.set_cycles(__unitlib2__t2-__unitlib2__t1);\
	suite->insert_test(__unitlib2__internal__perftest);\
}

class CPerfTest;

class CPerfTest
{
private:
	unsigned long long Cycles;
	std::string Name;

public:
	CPerfTest(std::string name)
	{
		Name = name;
		Cycles = 0;
	}

	CPerfTest(const CPerfTest& other)
	{
		this->Cycles = other.Cycles;
		this->Name = other.Name;
	}

	CPerfTest(CPerfTest&& other)
	{
		this->Cycles = other.Cycles;
		this->Name = other.Name;
		other.Cycles = 0;
		other.Name = "";
	}

	CPerfTest& operator=(const CPerfTest& other)
	{
		this->Cycles = other.Cycles;
		this->Name = other.Name;
		return *this;
	}

	void set_cycles(unsigned long long cycles)
	{
		this->Cycles = cycles;
	}

	unsigned long long get_cycles() const
	{
		return Cycles;
	}

	std::string get_name() const
	{
		return Name;
	}

	virtual void print(std::ostream& stream) const
	{
		stream << "\n";
		stream << this->get_name() << " completed in " << get_cycles() << " cycles.\n";
	}

	virtual void to_xml(std::ostream& stream) const
	{
		stream << "<perftest type=\"normal\">";
		stream << this->get_cycles();
		stream << "</perftest>\n";
	}
};

//
// A type of performance test with multiple steps 
//
class CSteppedPerfTest : public CPerfTest
{
private:
	std::list<unsigned long long> Times;
public:

	CSteppedPerfTest(std::string name) :
		CPerfTest(name)
	{

	}

	// Prunes tests with abnormal times
	// Deviation is a percentage different from the average that something can be
	void prune(float deviation = 50.0f)
	{
		double favg = 0.0f;
		for(auto x : Times)
		{
			favg += x;
		}
		favg /= Times.size();

		for(auto x : Times)
		{
			if(x/favg > (1.0f + deviation/100.0f))
				Times.remove(x);
		}
		// TODO: Make this better
		this->set_cycles((unsigned long long)get_average_time());
	}

	int get_step_count() const
	{
		return Times.size();
	}

	std::list<unsigned long long> get_steps() const
	{
		return Times;
	}

	void add_step(unsigned long long duration)
	{
		Times.push_back(duration);
		// TODO: Make this better
		this->set_cycles((unsigned long long)get_average_time());
	}

	float get_average_time() const
	{
		double avg = 0.0f;
		for(auto x : Times)
		{
			avg += x;
		}
		avg /= get_step_count();
		return avg;
	}

	virtual void print(std::ostream& stream) const
	{
		stream << "\n";
		stream << this->get_name() << " completed in " << get_average_time() << " cycles.\n";
		stream << "Test had a total of " << this->get_step_count() << " steps.\n";
	}

	virtual void to_xml(std::ostream& stream) const
	{
		stream << "<perftest type=\"stepped\" steps=\"" << this->get_step_count() << "\">";
		stream << this->get_average_time();
		stream << "</perftest>\n";
	}
};

class CPerfTestSuite
{
private:
	std::vector<CPerfTest> Tests;
	std::string Name;

public:
	CPerfTestSuite(std::string Name)
	{
		this->Name = Name;
	}

	CPerfTestSuite(const CPerfTestSuite& other)
	{
		this->Name = other.Name;
		this->Tests = other.Tests;
	}

	CPerfTestSuite(CPerfTestSuite&& other)
	{
		this->Name = other.Name;
		this->Tests = other.Tests;
		other.Name = "";
	}

	void insert_test(const CPerfTest& test)
	{
		Tests.push_back(test);
	}

	int get_test_count() const
	{
		return Tests.size();
	}

	std::vector<CPerfTest> get_tests() const
	{
		return Tests;
	}

	// Small util function for getting CPU frequency
	// Note: This is likely not that accurate
	// Note2: Returns frequency in GHz
	static float get_cpu_frequency()
	{
		unsigned long long t1 = __rdtsc();
		#ifdef _WIN32
		#error Not implemented :(
		#else
		usleep(5);
		#endif
		unsigned long long t2 = __rdtsc();
		return ((t2-t1) / (float)0.000005) / 1000000000.0f;
	}

	// Prints results to stdout
	void print_results() const
	{
		using namespace std;
		printf("=================================\n");
		printf("Performance test results for:\n%s", this->Name.c_str());
		printf("=================================\n");
		float ghz = CPerfTestSuite::get_cpu_frequency();
		printf("\nCPU Frequency is at: %f\n%llu clocks per second.\n\n", ghz, ghz * 1000000000.0f);

		for(auto x : this->Tests)
		{
			x.print(std::cout);
			//printf("\nTest %s completed in: %llu clock cycles.\n\n", x.get_name(), x.get_cycles());
		}

		printf("=================================\n");
		printf("END Performance test results for:\n%s", this->Name.c_str());
		printf("=================================\n");
	}

	void print_results(std::ostream& stream)
	{
		stream << "\n=================================\n";
		stream << "Performance test results for:\n" << Name << "\n";
		stream << "=================================\n";
		float ghz = CPerfTestSuite::get_cpu_frequency();
		printf("\nCPU Frequency is at: %f\n%llu clocks per second.\n\n", ghz, ghz * 1000000000.0f);
		stream << "\nCPU Frequency is at: " << ghz << "\n" << (unsigned long long)ghz * 1000000000 << " clocks per second.\n\n";

		for(auto x : this->Tests)
		{
			x.print(stream);
			//printf("\nTest %s completed in: %llu clock cycles.\n\n", x.get_name(), x.get_cycles());
		}

		stream << "\n=================================\n";
		stream << "END Performance test results for: " << Name;
		stream << "\n=================================\n";
	}

	void to_xml(std::ostream& stream)
	{
		stream << "<perftestsuite name=\"" << this->Name << "\" test_count=\"" << this->get_test_count() << "\">\n";
		for(auto x : this->Tests)
		{
			x.to_xml(stream);
		}
		stream << "</perftestsuite>\n";
	}
};
