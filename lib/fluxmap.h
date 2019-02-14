#ifndef FLUXMAP_H
#define FLUXMAP_H

class Fluxmap
{
public:
    uint8_t operator[](int index) const
    {
        return _intervals.at(index);
    }

    nanoseconds_t duration() const { return _duration; }
    int bytes() const { return _intervals.size(); }

    const uint8_t* ptr() const
	{
		if (!_intervals.empty())
			return &_intervals.at(0);
		return NULL;
	}

    Fluxmap& appendIntervals(const std::vector<uint8_t>& intervals);
    Fluxmap& appendIntervals(const uint8_t* ptr, size_t len);

    Fluxmap& appendInterval(uint8_t interval)
    {
        return appendIntervals(&interval, 1);
    }

    nanoseconds_t guessClock() const;
	std::vector<bool> decodeToBits(nanoseconds_t clock_period) const;

	Fluxmap& appendBits(const std::vector<bool>& bits, nanoseconds_t clock);

	void precompensate(int threshold_ticks, int amount_ticks);

private:
    nanoseconds_t _duration = 0;
    int _ticks = 0;
    std::vector<uint8_t> _intervals;
};

#endif
