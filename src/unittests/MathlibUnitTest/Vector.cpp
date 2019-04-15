// Vector tests
#include "unitlib2/unitlib2.h"

#include "Tests.h"

#include <vector.h>
#include <mathlib.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>


#ifdef _WIN32
#include <intrin.h>
#endif

#define TEST_COUNT 256

DEFINE_TEST_SUITE(VectorTestSuite, "Vector Tests");
DEFINE_TEST_SUITE(MatrixTestSuite, "Matrix Tests");
DEFINE_TEST_SUITE(QuatTestSuite, "Quaternion Tests");

DEFINE_PERF_TEST_SUITE(VectorPerfTestSute, "Vector Tests");
DEFINE_PERF_TEST_SUITE(MatrixPerfTestSute, "Matrix Tests");
DEFINE_PERF_TEST_SUITE(QuatPerfTestSute, "Quat Tests");

void PrintVec(const Vector& v)
{
	printf("Vector: x=%f y=%f z=%f\n", v.x, v.y, v.z);
}

void RunVectorTests()
{
	//
	// Vector normalization test 1
	//
	BEGIN_UNIT_TEST("Vector Normalization Test #1", VectorTestSuite)

		Vector v = Vector(232.3f, 320.0f, 302.0f);

		float m = v.NormalizeInPlace();

		// Normalized vector should be within a certain range of this:
		TEST_ACCURACY(v.x, 0.4669f, 0.05f);
		TEST_ACCURACY(v.y, 0.6431f, 0.05);
		TEST_ACCURACY(v.z, 0.607f, 0.05);
		TEST_ACCURACY(m, 497.6f, 0.05);

	END_UNIT_TEST(VectorTestSuite)

	//
	// Vector normalization test 2
	//
	BEGIN_UNIT_TEST("Vector Normalization Test #2", VectorTestSuite)

		Vector v = Vector(13.239f, 1020.059f, 0.1f);

		float m = v.NormalizeInPlace();

		// Normalized vector should be within a certain range of this:
		TEST_ACCURACY(v.x, 0.012978f, 0.05f);
		TEST_ACCURACY(v.y, 1.0f, 0.05);
		TEST_ACCURACY(v.z, 0.00009803f, 0.05);
		TEST_ACCURACY(m, 1020.1f, 0.05);

	END_UNIT_TEST(VectorTestSuite)

}