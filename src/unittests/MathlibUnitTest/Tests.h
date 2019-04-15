/*

Stuff!

*/
#pragma once

#include "unitlib2/unitlib2.h"

DECLARE_TEST_SUITE(VectorTestSuite);
DECLARE_TEST_SUITE(MatrixTestSuite);
DECLARE_TEST_SUITE(QuatTestSuite);

DECLARE_PERF_TEST_SUITE(VectorPerfTestSute);
DECLARE_PERF_TEST_SUITE(MatrixPerfTestSute);
DECLARE_PERF_TEST_SUITE(QuatPerfTestSute);

void RunMatrixTests();

void RunMatrixPerfTests();

void RunVectorTests();

void RunVectorPerfTests();

void RunQuatTests();

void RunQuatPerfTests();