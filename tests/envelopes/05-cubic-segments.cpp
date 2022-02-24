#include <test/tests.h>
#include "../common.h"

#include "envelopes.h"

#include <array>
#include <vector>

TEST("Cubic segments (example)", example) {
	using Curve = signalsmith::envelopes::CubicSegmentCurve<float>;
	struct Point{
		float x, y;
		Point(float x, float y) : x(x), y(y) {}
	};
	std::vector<Point> points;
	
	points.emplace_back(0, 0.5);
	points.emplace_back(1, 1);
	points.emplace_back(1.5, 3);
	points.emplace_back(2.5, 1.5);
	points.emplace_back(2.5, 1.5);
	points.emplace_back(3.5, 1.8);
	points.emplace_back(4, 3);
	points.emplace_back(4, 0.25);
	points.emplace_back(5, 2);
	points.emplace_back(6, 2.25);
	points.emplace_back(6.5, 1.5);
	
	Curve curveSmooth, curveMonotonic, curveLinear;
	for (auto &p : points) {
		curveSmooth.add(p.x, p.y);
		curveMonotonic.add(p.x, p.y);
		curveLinear.add(p.x, p.y);
		curveLinear.add(p.x, p.y);
	}
	curveSmooth.update();
	curveMonotonic.update(true);
	curveLinear.update();
	
	CsvWriter csv("cubic-segments-example");
	csv.line("x", "smooth", "monotonic", "linear");
	
	for (double x = -1; x < 7.5; x += 0.01) {
		csv.line(x, curveSmooth(x), curveMonotonic(x), curveLinear(x));
	}
	return test.pass();
}
TEST("Cubic segments (gradient)", segment_gradient) {
	using Segment = signalsmith::envelopes::CubicSegment<double>;
	double x0 = test.random(-1, 1);

	Segment s(
		x0,
		test.random(-1, 1),
		test.random(-1, 1),
		test.random(-1, 1),
		test.random(-1, 1)
	);
	Segment grad = s.dx();
	
	for (int r = 0; r < 100; ++r) {
		double x = test.random(x0, x0 + 2);
		double v = s(x);
		double dx = 1e-10;
		double v2 = s(x + dx);
		double approxGrad = (v2 - v)/dx;
		double g = grad(x);

		// Just ballpark correct
		double diff = std::abs(approxGrad - g);
		TEST_ASSERT(diff < 1e-4);
	}
}

TEST("Cubic segments (hermite)", segment_hermite) {
	using Segment = signalsmith::envelopes::CubicSegment<double>;

	for (int r = 0; r < 100; ++r) {
		double x0 = test.random(-1, 1);
		double x1 = x0 + test.random(0.01, 2);
		double y0 = test.random(-10, 10);
		double y1 = test.random(-10, 10);
		double g0 = test.random(-5, 5);
		double g1 = test.random(-5, 5);

		Segment s = Segment::hermite(x0, x1, y0, y1, g0, g1);
		Segment grad = s.dx();
		TEST_ASSERT(std::abs(s(x0) - y0) < 1e-6);
		TEST_ASSERT(std::abs(s(x1) - y1) < 1e-6);
		TEST_ASSERT(std::abs(grad(x0) - g0) < 1e-6);
		TEST_ASSERT(std::abs(grad(x1) - g1) < 1e-6);
	}
}

TEST("Cubic segments (known)", segment_known) {
	using Segment = signalsmith::envelopes::CubicSegment<double>;

	{
		Segment s = Segment::smooth(0, 1, 2, 3, 0, 1, 2, 3);
		TEST_ASSERT(std::abs(s(0) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(0) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s(1.5) - 1.5) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1.5) - 1) < 1e-6);
	}
	{
		Segment s = Segment::smooth(0, 1, 2, 3, 0, 1, 0, 1);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 0) < 1e-6);
	}

	{ // monotonic
		Segment s = Segment::smooth(0, 1, 2, 3, -1, 1, 0, 2, true);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 0) < 1e-6);
	}
}

TEST("Cubic segments (random)", segment_random) {
	using Segment = signalsmith::envelopes::CubicSegment<double>;
	
	for (int r = 0; r < 1000; ++r) {
		bool monotonic = r%2;
		
		std::array<double, 5> x, y;
		x[0] = test.random(-1, 1);
		for (int i = 1; i < 5; ++i) x[i] = x[i - 1] + test.random(1e-10, 2);
		for (auto &v : y) v = test.random(-10, 10) - 7.5;
		
		Segment sA = Segment::smooth(x[0], x[1], x[2], x[3], y[0], y[1], y[2], y[3], monotonic);
		Segment sB = Segment::smooth(x[1], x[2], x[3], x[4], y[1], y[2], y[3], y[4], monotonic);

		// The points agree
		TEST_ASSERT(std::abs(sA(x[1]) - y[1]) < 1e-6);
		TEST_ASSERT(std::abs(sA(x[2]) - y[2]) < 1e-6);
		TEST_ASSERT(std::abs(sB(x[2]) - y[2]) < 1e-6);
		TEST_ASSERT(std::abs(sB(x[3]) - y[3]) < 1e-6);

		// Test smoothness - their gradients agree at x2
		TEST_ASSERT(std::abs(sA.dx()(x[2]) - sB.dx()(x[2])) < 1e-6);
		
		if (monotonic) {
			double dt = 1e-3;
			if (y[1] >= y[2]) { // decreasing
				double min = sA(x[1]);
				for (double t = x[1]; t <= x[2]; t += dt) {
					double y = sA(t);
					TEST_ASSERT(y <= min + 1e-6);
					min = std::min(y, min);
				}
			}
			if (y[1] <= y[2]) { // increasing
				double max = sA(x[1]);
				for (double t = x[1]; t <= x[2]; t += dt) {
					double y = sA(t);
					TEST_ASSERT(y >= max - 1e-6);
					max = std::max(y, max);
				}
			}
		}
	}
}

TEST("Cubic segments (duplicate points)", duplicate_points) {
	using Segment = signalsmith::envelopes::CubicSegment<double>;

	{ // Duplicate left point means it continues existing curve (straight here)
		Segment s = Segment::smooth(1, 1, 2, 3, 1, 1, 2, 3);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 2) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 1) < 1e-6);
	}

	{ // Duplicate left point means it continues existing curve (quadratic here)
		Segment s = Segment::smooth(1, 1, 2, 3, 1, 1, 0, 1);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) + 2) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 0) < 1e-6);
	}

	{ // Vertical also continues the curve
		Segment s = Segment::smooth(1, 1, 2, 3, 0, 1, 2, 3);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 2) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 1) < 1e-6);
	}

	{ // or flat, if it's a min/max
		Segment s = Segment::smooth(1, 1, 2, 3, 2, 1, 2, 3);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 2) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 1) < 1e-6);
	}


	{ // Duplicate right point means it continues existing curve (straight here)
		Segment s = Segment::smooth(0, 1, 2, 2, 0, 1, 2, 2);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 2) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 1) < 1e-6);
	}

	{ // Duplicate right point means it continues existing curve (quadratic here)
		Segment s = Segment::smooth(0, 1, 2, 2, 0, 1, 0, 0);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) + 2) < 1e-6);
	}

	{ // Vertical also continues the curve
		Segment s = Segment::smooth(0, 1, 2, 2, 0, 1, 2, 3);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 2) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 1) < 1e-6);
	}

	{ // or flat, if it's a min/max
		Segment s = Segment::smooth(0, 1, 2, 2, 0, 1, 2, 1);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 2) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 0) < 1e-6);
	}
}
