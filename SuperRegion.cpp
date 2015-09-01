//project
#include "SuperRegion.h"

SuperRegion::SuperRegion()
	: reward(0)
{
}

SuperRegion::SuperRegion(const int& pReward)
	: reward(pReward)
{
}

SuperRegion::~SuperRegion()
{
}

void SuperRegion::addRegion(const int& region)
{
	regions.push_back(region);
}

bool SuperRegion::hasIn(int value)
{
	return regions.end() == find( regions.begin(), regions.end(), value );
}
