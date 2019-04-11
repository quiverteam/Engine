/*

Stuff!

*/
#pragma once

#include "unitlib2/unitlib2.h"

DECLARE_TEST_SUITE(VectorTestSuite, "Vector Tests");
DECLARE_TEST_SUITE(MatrixTestSuite, "Matrix Tests");
DECLARE_TEST_SUITE(QuatTestSuite, "Quaternion Tests");

DECLARE_PERF_TEST_SUITE(VectorPerfTestSute, "Vector Perf Tests");
DECLARE_PERF_TEST_SUITE(MatrixPerfTestSute, "Matrix Perf Tests");
DECLARE_PERF_TEST_SUITE(QuatPerfTestSute, "Quaternion Perf Tests");

void RunMatrixTests();

void RunMatrixPerfTests();

void RunVectorTests();

void RunVectorPerfTests();

void RunQuatTests();

void RunQuatPerfTests();