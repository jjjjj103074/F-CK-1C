#pragma once

#include <array>
#include <cstddef>

namespace Common
{
template <std::size_t SegmentCount>
class SegmentedIntegrity final
{
public:
	SegmentedIntegrity()
	{
		reset();
	}

	void reset()
	{
		segments_.fill(1.0);
		integrity_ = 1.0;
	}

	void apply(std::size_t segment, double integrity)
	{
		if (segment >= SegmentCount)
		{
			return;
		}
		segments_[segment] = integrity;
		refresh();
	}

	double value() const
	{
		return integrity_;
	}

private:
	void refresh()
	{
		integrity_ = 1.0;
		for (double segment : segments_)
		{
			integrity_ *= segment;
		}
	}

	std::array<double, SegmentCount> segments_;
	double integrity_ = 1.0;
};
}
