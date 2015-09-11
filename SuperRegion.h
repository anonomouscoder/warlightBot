#ifndef SUPERREGION_H_INCLUDED
#define SUPERREGION_H_INCLUDED

// stl
#include <vector>
#include <algorithm>
// project
#include "main.h"
#include "Region.h"

class SuperRegion
{
public:
	SuperRegion();
	SuperRegion(const int& pReward);
	inline int getReward() const { return reward; }
	virtual ~SuperRegion();
	void addRegion(const int& region);
	inline size_t size() const { return regions.size(); }
	inline std::vector<int> const getRegions() { return regions; }
	bool hasIn(int value);
	bool hasWasteland;
private:
	std::vector<int> regions;
	int reward;
};

#endif // SUPERREGION_H_INCLUDED
