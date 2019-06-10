// Vector tests
#include "unitlib2/unitlib2.h"

#include "Tests.h"

#include <vector.h>
#include <mathlib.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

//#include "mathlib/ssemath.h"
#include "mathlib/mathlib.h"
#include "../mathlib/sse.h"

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
	srand(time(0));

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

	//
	// Vector add
	//
	BEGIN_UNIT_TEST("Vector Add Test #1", VectorTestSuite)
		Vector v1 = Vector(rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f);
		Vector v2 = Vector(rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f);
		Vector res = v1 + v2;

		// result should be 

		TEST_ACCURACY(res.x, v1.x + v2.x, 0.05f);
		TEST_ACCURACY(res.y, v1.y + v2.y, 0.05f);
		TEST_ACCURACY(res.z, v1.z + v2.z, 0.05f);

	END_UNIT_TEST(VectorTestSuite)

	//
	// Vector sub
	//
	BEGIN_UNIT_TEST("Vector Sub Test #1", VectorTestSuite)
		Vector v1 = Vector(rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f);
		Vector v2 = Vector(rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f);
		Vector res = v1 - v2;

		// result should be 

		TEST_ACCURACY(res.x, v1.x - v2.x, 0.05f);
		TEST_ACCURACY(res.y, v1.y - v2.y, 0.05f);
		TEST_ACCURACY(res.z, v1.z - v2.z, 0.05f);

	END_UNIT_TEST(VectorTestSuite)

	//
	// Vector mul
	//
	BEGIN_UNIT_TEST("Vector Mul Test #1", VectorTestSuite)
		Vector v1 = Vector(rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f);
		double c = rand() / 1000.0f;
		Vector res = v1 * c;

		// result should be 

		TEST_ACCURACY(res.x, v1.x * c, 0.05f);
		TEST_ACCURACY(res.y, v1.y * c, 0.05f);
		TEST_ACCURACY(res.z, v1.z * c, 0.05f);

	END_UNIT_TEST(VectorTestSuite)

	//
	// Vector mul
	//
	BEGIN_UNIT_TEST("Vector Div Test #1", VectorTestSuite)
		Vector v1 = Vector(rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f);
		double c = rand() / 1000.0f;
		Vector res = v1 / c;

		// result should be 

		TEST_ACCURACY(res.x, v1.x / c, 0.05f);
		TEST_ACCURACY(res.y, v1.y / c, 0.05f);
		TEST_ACCURACY(res.z, v1.z / c, 0.05f);

	END_UNIT_TEST(VectorTestSuite)

	//
	// Sqrt test 
	//
	BEGIN_UNIT_TEST("Sqrt Test #1", VectorTestSuite)
		float v = 1293.3202f;

		// sqrt v = 35.96276129554014098995381727835384380016333734135153232355
		TEST_ACCURACY(_SSE_Sqrt(v), 35.96276129554014098995381727835384380016333734135153232355, 0.05f);
	END_UNIT_TEST(VectorTestSuite)

	//
	//
	//

}