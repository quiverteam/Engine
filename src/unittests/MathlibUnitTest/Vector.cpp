// Vector tests
#include "Tests.h"

#include <vector.h>
#include <mathlib.h>
#include <stdlib.h>

#include <intrin.h>

#define TEST_COUNT 256

void PrintVec(const Vector& v)
{
	printf("Vector: x=%f y=%f z=%f\n", v.x, v.y, v.z);
}

void RunVectorTests()
{
	MathLib_Init();

	Vector v1 = Vector(rand(), rand(), rand());

	printf("Before Normalization:\n");
	PrintVec(v1);

	v1.NormalizeInPlace();

	printf("After Normalization:\n");
	PrintVec(v1);

	uint64 testtimes[TEST_COUNT];

	for (int i = 0; i < TEST_COUNT; i++)
	{
		Vector v2 = Vector(rand(), rand(), rand());

		uint64 n1 = __rdtsc();

		v2.NormalizeInPlace();

		uint64 n2 = __rdtsc();

		testtimes[i] = n2 - n1;
	}

	float avg = 0.0f;
	for (int i = 0; i < TEST_COUNT; i++)
		avg += testtimes[i];
	avg /= TEST_COUNT;

	printf("Normalization avg cycles: %f\n", avg);
}