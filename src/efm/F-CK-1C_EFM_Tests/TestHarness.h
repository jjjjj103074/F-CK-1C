#pragma once

#include <cmath>
#include <cstdio>

namespace Tests
{
struct CheckLocation
{
	const char* expression;
	const char* file;
	int line;
};

struct NearCheck
{
	double tolerance;
	CheckLocation location;
};

class Context
{
public:
	void expect(bool condition, const CheckLocation& location)
	{
		++checks_;
		if (condition)
		{
			return;
		}

		++failures_;
		std::printf("FAIL %s:%d: %s\n", location.file, location.line, location.expression);
	}

	void expect_near(double actual, double expected, const NearCheck& check)
	{
		++checks_;
		if (std::fabs(actual - expected) <= check.tolerance)
		{
			return;
		}

		++failures_;
		std::printf(
			"FAIL %s:%d: %s (actual=%.12f expected=%.12f tolerance=%.12f)\n",
			check.location.file,
			check.location.line,
			check.location.expression,
			actual,
			expected,
			check.tolerance);
	}

	int finish() const
	{
		std::printf("EFM tests: %d checks, %d failures\n", checks_, failures_);
		return failures_ == 0 ? 0 : 1;
	}

private:
	int checks_ = 0;
	int failures_ = 0;
};
}

#define TEST_EXPECT(context, expression) \
	(context).expect((expression), { #expression, __FILE__, __LINE__ })

#define TEST_EXPECT_NEAR(context, actual, expected, tolerance) \
	(context).expect_near( \
		(actual), \
		(expected), \
		{ (tolerance), { #actual " ~= " #expected, __FILE__, __LINE__ } })
