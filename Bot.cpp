// stl
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <math.h>
//project
#include "Bot.h"

//tools
#include "tools/StringManipulation.h"

Bot::Bot() :
		armiesLeft(0), timebank(0), timePerMove(0), maxRounds(0), parser(this), phase(NONE), state(EMPIRE)
{
	debug.open("debuga.txt", std::fstream::out);
	debug << "debugstarted" << std::endl;
	loadGeneticValues();
}

void Bot::loadGeneticValues()
{
	std::ifstream geneticFileHandler("/home/adtran/Desktop/ai/genetics.txt");
	std::string line;
	if( geneticFileHandler.is_open() )
	{
		debug << "genetics file is open" << std::endl;
		std::vector<int> geneticInputs;
		while( std::getline(geneticFileHandler, line) ) 
		{
			debug << "line: '" << line << "'" << std::endl;
			geneticInputs.push_back(std::stoi(line));
			debug << "line is pushed " << std::endl;
		}
		debug << "closing genetics file" << std::endl;
		geneticFileHandler.close();

		aiStateCutoff = geneticInputs[0];
		Empire.lastSuperWeight = geneticInputs[1];
		Empire.firstSuperWeight = geneticInputs[2];
		Raid.lastSuperWeight = geneticInputs[3];
		Raid.firstSuperWeight = geneticInputs[4];
		
	}
	
	debug << aiStateCutoff << ", " << Empire.lastSuperWeight << ", " << Empire.firstSuperWeight << ", " << Raid.lastSuperWeight << ", " << Raid.firstSuperWeight << std::endl;
	return;
}
Bot::~Bot()
{
	debug.close();
}

void Bot::playGame()
{
	parser.parseInput();
}

void Bot::pickStartingRegion()
{
	state = EMPIRE;
	// START HERE!
	if( state == EMPIRE  || state == RAID )
	{
		//sort the superregions by weight
		auto compareFunc = [](std::pair<unsigned,unsigned> a, std::pair<unsigned,unsigned> b) { return a.second < b.second; };
		std::sort ( weightedSuperRegionIndexList.begin(), weightedSuperRegionIndexList.end(), compareFunc);
	
		round = 0;
		debug << "Choosing Regions("<< timebank <<"): " << std::endl;
		unsigned target = 999;
		std::vector<std::pair<unsigned,unsigned>> weights;
		unsigned maxChecks = 10;
	
		for ( unsigned startRegionIndex = 0; 
			  startRegionIndex < std::min((unsigned)startingRegionsreceived.size(),maxChecks); 
	 		  ++startRegionIndex)
		{
			Region thisStartRegion = regions[startingRegionsreceived[startRegionIndex]];
			SuperRegion thisStartSuperRegion = superRegions[thisStartRegion.getSuperRegion()];

			unsigned weight = 1000;
			weight -= weightedSuperRegionIndexList[thisStartRegion.getSuperRegion()].second;
		
			debug << "Region "<< startingRegionsreceived[startRegionIndex] <<", weight = " << weight << std::endl;	
			weights.push_back(std::pair<unsigned,unsigned>(startRegionIndex,weight) );
		}
		//sort in reverse order
		auto reversecompareFunc = [](std::pair<unsigned,unsigned> a, std::pair<unsigned,unsigned> b) { return a.second < b.second; };
		std::sort ( weights.begin(), weights.end(), reversecompareFunc);
		target = weights[0].first;
		debug << ".Choosing "<< regions[target].getId()  << " , superregion: " << regions[target].getSuperRegion() << std::endl;
		std::cout << startingRegionsreceived[target] << std::endl;
	}
}

void Bot::placeArmies()
{
	// START HERE!
	int enemyWeight = 0;
	int neutralWeight = 0;
	int sameSuperWeight = 0;
	int lastRegionOfSuper = 0;
	int firstRegionOfSuper = 0;
	int conquerableWeight = 0;
	int maxArmiesOnRegion = 50;

	int fullyOwnedSupers = calculateOwnedSupers();
	
	if( fullyOwnedSupers < aiStateCutoff ) 
	{
		state = EMPIRE;
	}
	else
	{
		state = RAID;
	}
	
	if( state == RAID )
	{
		enemyWeight = 50;
		neutralWeight = 100;
		sameSuperWeight = 100;
		conquerableWeight = 1000;

		lastRegionOfSuper = Raid.lastSuperWeight;
		firstRegionOfSuper = Empire.firstSuperWeight;
	}
	if( state == EMPIRE )
	{
		enemyWeight = 50;
		neutralWeight = 100;
		sameSuperWeight = 100;
		conquerableWeight = 1000;

		lastRegionOfSuper = Empire.lastSuperWeight;
		firstRegionOfSuper = Empire.firstSuperWeight;
	}

	calculateThreatLevels();

	round++;
	debug << "Round " << round << std::endl;
	unsigned target = 999;

	debug << "Placing Armies("<< timebank <<"): " << std::endl;
	debug << ".Choosing 2 from: [";
	for ( int i = 0; i < owned.size(); ++i) debug << owned[i] << ",";
	debug << "]" << std::endl;

	std::vector<std::pair<unsigned,unsigned>> weights;

	std::vector<unsigned> unitsNeededList;
	for ( unsigned ownedIndex = 0; ownedIndex < owned.size(); ++ownedIndex)
	{
		Region thisOwnedRegion = regions[owned[ownedIndex]];
		unsigned numberOfNeighbors = thisOwnedRegion.getNbNeighbors();
		bool regionHasEnemy = false;
		bool regionHasNeutral = false;
		unsigned weight = 1;
		unsigned unitsNeeded = 1;
		debug << ".Checking neighbors of " << owned[ownedIndex] << std::endl;
		for ( unsigned l = 0; l < numberOfNeighbors; ++l)
		{
			Region thisNeighborRegion = regions[thisOwnedRegion.getNeighbor(l)];
			SuperRegion thisNeighborSuperRegion = superRegions[thisNeighborRegion.getSuperRegion()];

			//count how many regions of this superregion that I don't have
			int countOfOwnedRegions = countOwnedRegionsOfSuper(thisNeighborRegion.getSuperRegion());

			if( (unsigned)superRegions[thisNeighborRegion.getSuperRegion()].size() - countOfOwnedRegions == 1 )
			{
				debug << ", but this is the last region in this super region, weight +"<<lastRegionOfSuper;
				weight += lastRegionOfSuper;
			}
			if( countOfOwnedRegions == 0 ) 
			{
				debug << ", i don't have anything in this super region weight + "<<firstRegionOfSuper;
				weight += firstRegionOfSuper;
			}
			if (thisNeighborRegion.getOwner() == ENEMY && !regionHasEnemy)
			{
				weight += enemyWeight;
				regionHasEnemy = true;
				debug << "+ "<<enemyWeight<<" (enemy) ";
			}
			if (thisNeighborRegion.getOwner() == NEUTRAL && !regionHasNeutral)
			{
				weight += neutralWeight;
				regionHasNeutral = true;
				debug << "+ "<<neutralWeight<<" (neutral) ";
			}
			if (thisNeighborRegion.getOwner() == NEUTRAL || thisNeighborRegion.getOwner() == ENEMY)
			{
				unitsNeeded += whatCanWin(thisNeighborRegion.getArmies()) +1;				
				debug << " unitsNeeded: " << unitsNeeded;
				if ( thisNeighborRegion.getSuperRegion() == thisOwnedRegion.getSuperRegion() )
				{
					weight += thisNeighborSuperRegion.getReward()*100 / ( thisNeighborSuperRegion.size() - countOfOwnedRegions );
					debug  << "..weight = " << thisNeighborSuperRegion.getReward()*100 / ( thisNeighborSuperRegion.size() - countOfOwnedRegions );
					debug << "(" << thisNeighborSuperRegion.getReward() 
						  << " / " << thisNeighborSuperRegion.size() << " - " << countOfOwnedRegions << ") ";
				}

			}
		
		}
		debug << std::endl << ".done Checking neighbors of " << owned[ownedIndex] << " weight = " << weight << std::endl;
		if (thisOwnedRegion.getArmies() > maxArmiesOnRegion) 
			weight = 1;
		weights.push_back(std::pair<unsigned,unsigned>(ownedIndex,weight) );
		unitsNeededList.push_back(unitsNeeded);
		//std::vector<std::pair<unsigned,unsigned>>::iterator it;
		//debug << ".Size of weights list: " <<  std::distance(weights.begin(),weights.end()) << std::endl;
		//debug << ".Current weights list: [";
		//for (it=weights.begin(); it!=weights.end(); it++)
		//	debug << it->first << ":" << it->second << ", ";
		//debug << "]" << std::endl;
	}
	std::vector<std::pair<unsigned,unsigned>>::iterator it;
	//debug << ".Final weights list: [";
	//for (it=weights.begin(); it!=weights.end(); it++)
	//	debug << it->first << ":" << it->second << ", ";
	//debug << "]" << std::endl;

	auto compareFunc = [](std::pair<unsigned,unsigned> a, std::pair<unsigned,unsigned> b) { return a.second > b.second; };
	std::sort ( weights.begin(), weights.end(), compareFunc);
	debug << ".Sorted weights list: [";
	for (it=weights.begin(); it!=weights.end(); it++)
		debug << it->first << ":" << it->second << ", ";
	debug << "]" << std::endl;
	target = weights[0].first;
	debug << ".Target chosen: " << owned[target] << std::endl;


	debug << ".choosing troop sizes: " << std::endl;
	std::vector<unsigned> troops;
	unsigned reserve = armiesLeft;
	for (it=weights.begin(); it!=weights.end(); it++)
	{
		target = it->first;
		unsigned unitsNeeded;
		if (regions[owned[target]].getArmies() > unitsNeededList[target])
			unitsNeeded = 0;
		else
			unitsNeeded = unitsNeededList[target] - regions[owned[target]].getArmies();
		debug << "..region " << owned[target] << " needs " << unitsNeeded << " troops (" << reserve << " left)" << std::endl;
		if (unitsNeeded < reserve)
		{
			debug << "..giving this region " << unitsNeeded << " troops." << std::endl;
			troops.push_back(unitsNeeded);
			reserve -= unitsNeeded;
		} else
		{
			debug << "..this region needs all that i can give: giving " << reserve << " troops." << std::endl;
			troops.push_back(reserve);
			reserve = 0;
			break;
		}
	}
	if (reserve != 0)
	{
		debug << "..giving the highest priority more troops, as I ran out of places to put them: " << reserve << " troops." << std::endl;
		troops[0] += reserve;
	}
	std::vector<std::string> placements;
	for(int i=0;i<troops.size();i++)
	{
		if (troops[i] > 0)
		{
			target = weights[i].first;
			std::stringstream placement;
			placement <<" "<< botName << " place_armies " << owned[target] << " " << troops[i];
			placements.push_back(placement.str());
			addArmies(owned[target], troops[i]);
		}
	}
	std::cout << string::join(placements,',') << std::endl;
	debug << string::join(placements) << std::endl;


}

void Bot::makeMoves()
{
	// START HERE!
	int enemyWeight = 0;
	int neutralWeight = 0;
	int sameSuperWeight = 0;
	int conquerableWeight = 0;
	int alreadyAttackedWeight = 0;

	int ownedRegionBaseWeight = 0;
	int strongerOwnedRegionWeight = 0;
	if( state == RAID )
	{
		enemyWeight = 200;
		neutralWeight = 100;
		sameSuperWeight = 10;
		conquerableWeight = 1000;
		alreadyAttackedWeight = -50;

		ownedRegionBaseWeight = 500;
		strongerOwnedRegionWeight = 15;
	}
	if( state == EMPIRE )
	{
		enemyWeight = 20;
		neutralWeight = 10;
		sameSuperWeight = 100;
		conquerableWeight = 1000;
		alreadyAttackedWeight = -50;

		ownedRegionBaseWeight = 500;
		strongerOwnedRegionWeight = 15;
	}
	debug << "Making moves("<< timebank <<")" << std::endl;

	std::vector<std::string> moves;
	std::vector<bool> attackedRegions = std::vector<bool>(regions.size(),false);
	for (unsigned ownedIndex = 0; ownedIndex < owned.size(); ++ownedIndex)
	{
		Region thisOwnedRegion = regions[owned[ownedIndex]];
		//if this territory only has 1-4 army, skip it
		if (thisOwnedRegion.getArmies() <= 3)
			continue;
		debug << ".Looking at region " << owned[ownedIndex] 
			  << ": str= " << thisOwnedRegion.getArmies() 
			  << std::endl;
		unsigned numberOfNeighbors = thisOwnedRegion.getNbNeighbors();
		unsigned numberOfBadNeighbors = 0;
		
		std::vector<std::pair<unsigned,unsigned>> weightedPossibleMoves;

		bool saveForFinalRegionInSuperRegion = false;

		for( unsigned neighborIndex = 0; neighborIndex < numberOfNeighbors; ++neighborIndex )
		{
			unsigned weight = 1;
			Region thisNeighborRegion = regions[thisOwnedRegion.getNeighbor(neighborIndex)];
			debug << "..neighbor " << thisOwnedRegion.getNeighbor(neighborIndex) 
	              << ": str= " << thisNeighborRegion.getArmies() 
	              << std::endl;
			switch( thisNeighborRegion.getOwner() )
			{
				case ENEMY:
					debug << "...owned by enemy, weight == " << enemyWeight;
					weight = enemyWeight;
				case NEUTRAL: //or enemy
					if( weight != enemyWeight)
					{
						debug << "...owned by neutral, weight == " << neutralWeight;
						weight = neutralWeight;
					}
					if( thisNeighborRegion.getSuperRegion() == thisOwnedRegion.getSuperRegion() )
					{
						weight += sameSuperWeight;
						debug << ", in same superregion. weight + " << sameSuperWeight;	
						weight += weightedSuperRegionIndexList[thisNeighborRegion.getSuperRegion()].first;
						debug << " + " +weightedSuperRegionIndexList[thisNeighborRegion.getSuperRegion()].second;

						//count how many regions of this superregion that I don't have
						int countOfOwnedRegionsInSuper = countOwnedRegionsOfSuper(thisNeighborRegion.getSuperRegion());
				
						if( (unsigned)superRegions[thisNeighborRegion.getSuperRegion()].size() - countOfOwnedRegionsInSuper == 1 )
						{
							debug << ", last region in super";
							saveForFinalRegionInSuperRegion = true;
						}
					}
					if( canThisWin( thisOwnedRegion.getArmies()-1, thisNeighborRegion.getArmies() ) )
					{
						weight += conquerableWeight;
						debug << ", conquerable. weight + 1000";
						//no need to save, i can kill it
						saveForFinalRegionInSuperRegion = false;
					}
				
					if( attackedRegions[thisNeighborRegion.getId()] )
					{
						debug << ", already attacked - " << alreadyAttackedWeight;
						weight -= alreadyAttackedWeight;
					}
					debug << std::endl;
					++numberOfBadNeighbors;
					break;
				default:
				case ME:
					weight = ownedRegionBaseWeight;
					debug << "...owned by me, weight == " << ownedRegionBaseWeight;
					if( (thisNeighborRegion.getArmies()-1) > (thisOwnedRegion.getArmies() - 1) )
					{
						weight += strongerOwnedRegionWeight;
						debug << " + " << strongerOwnedRegionWeight;
					}
				
					break;
			}
		
			weightedPossibleMoves.push_back( std::pair<unsigned,unsigned>(thisOwnedRegion.getNeighbor(neighborIndex),weight) );
			debug << std::endl;
		}
		//if a region has no bad neighbor, add the threat of each region to the moves to that region
		//so it should move it's armies toward the highest threat (i.e. the enemy)
		std::vector<std::pair<unsigned,unsigned>>::iterator it;
		if( numberOfBadNeighbors == 0 ) 
		{
			for ( it=weightedPossibleMoves.begin(); it!=weightedPossibleMoves.end(); it++)
			{
				it->second += regions[it->first].threat;
			}
		}

		//sort the weighted list:
		auto compareFunc = [](std::pair<unsigned,unsigned> a, std::pair<unsigned,unsigned> b) { return a.second > b.second; };
		std::sort ( weightedPossibleMoves.begin(), weightedPossibleMoves.end(), compareFunc);

		debug << ".Sorted weights list: [";
		for (it=weightedPossibleMoves.begin(); it!=weightedPossibleMoves.end(); it++)
		{		
			debug << it->first << ":" << it->second << ", ";
		}
		debug << "]" << std::endl;

		unsigned attackingArmy  = thisOwnedRegion.getArmies() - 1;
		int count = 0;
		while( attackingArmy > 0 && count < weightedPossibleMoves.size() -1)
		{
			debug << ".have " << attackingArmy << " troops left, looking to move them" << std::endl;
			unsigned target = weightedPossibleMoves[count].first;
			unsigned weight = weightedPossibleMoves[count].second;
			debug << ".target[ " << count << "]=" << target << ", weight["<<count<<"]="<<weight<<std::endl;
	
			//if everyone around me is friendly, allow friendly movements
			unsigned weightToMove = 500;
			if( numberOfBadNeighbors == 0 ) weightToMove = 0;

			// don't add a move unless a good one is found
			if (target != 0 && weight > weightToMove && !saveForFinalRegionInSuperRegion)
			{
				debug << ".target has enough weight to move there"<<std::endl;
				if( regions[target].getOwner() != ME) attackedRegions[target] = true;
				unsigned defenseStr = regions[target].getArmies();
		
				unsigned strengthNeeded = whatCanWin(defenseStr);
				//unsigned possibleStrengthNeeded = whatCanWin(defenseStr * 3 / 2) + 1;

				//if( possibleStrengthNeeded <= attackingArmy ) strengthNeeded = possibleStrengthNeeded;

				//if you only have 1 bad neighbor, attack with everything
				if( numberOfBadNeighbors == 1 ) strengthNeeded = attackingArmy;
				if( strengthNeeded <= attackingArmy ) //only move if you can kill them
				{
					std::stringstream move;
					debug << ".stengthNeeded: " << strengthNeeded << ",attackers: " << attackingArmy << std::endl;
					debug << ".moving " << std::min( strengthNeeded,attackingArmy ) << " to " << target << std::endl;
					move <<" "<< botName << " attack/transfer " << owned[ownedIndex] << " "
							<< target << " "
							<< std::min( strengthNeeded, attackingArmy);
					moves.push_back(move.str());
					debug << move.str() << std::endl;

					debug << ".taking: " << std::min( strengthNeeded, attackingArmy) << " off of attackingArmy" << std::endl;
					attackingArmy -= std::min( strengthNeeded, attackingArmy);
				}
				count++;
			}
			else
			{
				debug << ".target either doesn't have enough weight to move there, or it's saving for a final region in super region"<<std::endl;
				break;
			}
		}
		debug << ".# of moves saved: " << moves.size() << std::endl;
	}
	if (moves.size() < 1) // if no moves are found, send back no moves
	{
		debug <<".No moves" << std::endl;
		std::cout << "No moves" << std::endl;
	} else 
	{
		std::vector<std::string>::iterator it_str;
		debug << "one move per line:" << std::endl;
		for( it_str=moves.begin(); it_str != moves.end(); ++it_str )
			debug <<"." << *it_str << std::endl;
		debug << "exactly what I send:" << std::endl;
		debug << string::join(moves) << std::endl;
		std::cout << string::join(moves) << std::endl;
	}
}
bool Bot::canThisWin(int attackers, int defenders)
{
	if( attackers == 1) return false;
	if( attackers == 2 && defenders == 1) return true;
	if( attackers == 2) return false;
 
	//debug << "attackers: " << attackers << " defenders: " << defenders << std::endl;
	float baseDefendersKilled= attackers*0.6*0.84;
	//debug << "base killed: "<< baseDefendersKilled << std::endl;
	
	float luckDefendersKilled= 0;
	for( int i = 0; i < attackers; i++)
	{
		luckDefendersKilled += i * (std::pow(1-0.6,attackers-i)) * (std::pow(0.6,i));
		//debug << ".iteration " << i << " kills: "<< i * (std::pow(1-0.6,attackers-i)) * (std::pow(0.6,i)) << std::endl;
	}
	luckDefendersKilled = luckDefendersKilled * 160 / 1000;
	//debug << "luck killed: "<< luckDefendersKilled << std::endl;
	
	int totalDefendersKilled = std::floor(baseDefendersKilled + luckDefendersKilled * 160 / 1000 + 0.5);
	//debug << "total killed: "<< totalDefendersKilled << std::endl;
	if( totalDefendersKilled >= defenders ) return true;
	else return false;
}
int Bot::whatCanWin(int defenders)
{
	for( int i = defenders; i < 10*defenders; i++)
	{
		if( canThisWin(i,defenders) ) return i;
	}
}

void Bot::calculateThreatLevels()
{
	debug << "Calculating threat levels" << std::endl;
	debug << ".giving enemy's regions threat = 100" << std::endl;

	std::vector<Region> lastThreatLevelRegions = std::vector<Region>();
	std::vector<Region> thisThreatLevelRegions = std::vector<Region>();
	int threatLevel = 100;
	int thisAppliedThreatCount = 0;
	for( int i = 1; i < regions.size(); i++)
	{
		if( regions[i].getOwner() == ENEMY )
		{
			regions[i].threat = threatLevel;
			thisThreatLevelRegions.push_back(regions[i]);
			thisAppliedThreatCount++;
		}else regions[i].threat = 0;
	}
	debug << ".filling out threat map (neighbors of a region with threat x get threat x-1)" << std::endl;

	while( thisAppliedThreatCount > 0)
	{
		threatLevel--;
		debug << "..threatLevel: " << threatLevel << std::endl;
		lastThreatLevelRegions = thisThreatLevelRegions;
		thisThreatLevelRegions = std::vector<Region>();
		std::vector<Region>::iterator it;
		thisAppliedThreatCount = 0;

		for( it=lastThreatLevelRegions.begin(); it != lastThreatLevelRegions.end(); it++)
		{
			debug << "...region " << it->getId() << ", number of neighbors: " << it->getNbNeighbors() << std::endl;
			for( int i = 0; i < it->getNbNeighbors(); i++)
			{

				Region neighbor = regions[it->getNeighbor(i)];
				debug << "....neighbor: " << neighbor.getId() << "(" << neighbor.threat << ")";	
				if( neighbor.threat == 0)
				{
					regions[it->getNeighbor(i)].threat = threatLevel;
					thisThreatLevelRegions.push_back(neighbor);
					debug << "->(" << threatLevel << ")" << std::endl;	
					thisAppliedThreatCount++;
				}else debug << std::endl;
			}
		}
	}
	std::vector<Region>::iterator it2;
	for( it2 = regions.begin(); it2 != regions.end(); it2++)
	{
		debug << "region: " << it2->getId() << " threat: " << it2->threat << std::endl;
	}
}
int Bot::calculateOwnedSupers()
{
	int count = 0;
	for( int i = 0; i < superRegions.size(); ++i )
	{
		if( countOwnedRegionsOfSuper(i) == superRegions[i].size() ) ++count;
	}
	return count;
}
int Bot::countOwnedRegionsOfSuper(int superRegionIndex)
{
	std::vector<int>::iterator it;
	std::vector<int> regionsOfThisSuperRegion = superRegions[superRegionIndex].getRegions();
	int countOfOwnedRegions = 0;
	for( it=regionsOfThisSuperRegion.begin(); it!=regionsOfThisSuperRegion.end(); it++)
	{
		if( regions[*it].getOwner() == ME ) countOfOwnedRegions++;
	}
	return countOfOwnedRegions;

}

void Bot::addRegion(const unsigned& noRegion, const unsigned& noSuperRegion)
{
	while (regions.size() <= noRegion)
	{
		regions.push_back(Region());
	}
	regions[noRegion] = Region(noRegion, noSuperRegion);
	superRegions[noSuperRegion].addRegion(noRegion);

}
void Bot::addNeighbors(const unsigned& noRegion, const unsigned& neighbors)
{
	regions[noRegion].addNeighbor(neighbors);
	regions[neighbors].addNeighbor(noRegion);
}
void Bot::addWasteland(const unsigned &noRegion)
{
	wastelands.push_back(noRegion);
	unsigned noSuperRegion = regions[noRegion].getSuperRegion();
	weightedSuperRegionIndexList[noSuperRegion].second = superRegions[noSuperRegion].getReward() * 50 / (unsigned)superRegions[noSuperRegion].size();
}
void Bot::addSuperRegion(const unsigned& noSuperRegion, const int&reward)
{
	while (superRegions.size() <= noSuperRegion)
	{
		superRegions.push_back(SuperRegion());
		weightedSuperRegionIndexList.push_back(std::pair<unsigned,unsigned>(noSuperRegion,0));
	}
	superRegions[noSuperRegion] = SuperRegion(reward);
}

void Bot::setBotName(const std::string& name)
{
	botName = name;
}
void Bot::setOpponentBotName(const std::string& name)
{
	opponentBotName = name;
}
void Bot::setArmiesLeft(const int& nbArmies)
{
	armiesLeft = nbArmies;
}
void Bot::setTimebank(const int &newTimebank)
{
	timebank = newTimebank;
}
void Bot::setTimePerMove(const int &newTimePerMove)
{
	timePerMove = newTimePerMove;
}
void Bot::setMaxRounds(const int &newMaxRounds)
{
	maxRounds = newMaxRounds;
}

void Bot::clearStartingRegions()
{
	startingRegionsreceived.clear();
}

void Bot::addStartingRegion(const unsigned& noRegion)
{
	startingRegionsreceived.push_back(noRegion);
}

void Bot::addOpponentStartingRegion(const unsigned& noRegion)
{
	opponentStartingRegions.push_back(noRegion);
}
void Bot::opponentPlacement(const unsigned & noRegion, const int & nbArmies)
{
	// suppress unused variable warnings
	(void) noRegion;
	(void) nbArmies;

	// TODO: STUB
}
void Bot::opponentMovement(const unsigned &noRegion, const unsigned &toRegion, const int &nbArmies)
{
	// suppress unused variable warnings
	(void) noRegion;
	(void) toRegion;
	(void) nbArmies;

	// TODO: STUB
}

void Bot::startDelay(const int& delay)
{
	// suppress unused variable warnings
	(void) delay;
	// TODO: STUB
}
void Bot::setPhase(const Bot::Phase pPhase)
{
	phase = pPhase;
}
void Bot::executeAction()
{
	if (phase == NONE)
		return;
	if (phase == Bot::PICK_STARTING_REGION)
	{
		pickStartingRegion();
	}
	else if (phase == Bot::PLACE_ARMIES)
	{
		placeArmies();
	}
	else if (phase == Bot::ATTACK_TRANSFER)
	{
		makeMoves();
	}
	phase = NONE;
}

void Bot::updateRegion(const unsigned& noRegion, const  std::string& playerName, const int& nbArmies)
{
	Player owner;
	if (playerName == botName)
		owner = ME;
	else if (playerName == opponentBotName)
		owner = ENEMY;
	else
		owner = NEUTRAL;
	regions[noRegion].setArmies(nbArmies);
	regions[noRegion].setOwner(owner);
	if (owner == ME)
		owned.push_back(noRegion);
}
void Bot::addArmies(const unsigned& noRegion, const int& nbArmies)
{
	regions[noRegion].setArmies(regions[noRegion].getArmies() + nbArmies);
}
void Bot::moveArmies(const unsigned& noRegion, const unsigned& toRegion, const int& nbArmies)
{
	if (regions[noRegion].getOwner() == regions[toRegion].getOwner() && regions[noRegion].getArmies() > nbArmies)
	{
		regions[noRegion].setArmies(regions[noRegion].getArmies() - nbArmies);
		regions[toRegion].setArmies(regions[toRegion].getArmies() + nbArmies);
	}
	else if (regions[noRegion].getArmies() > nbArmies)
	{
		regions[noRegion].setArmies(regions[noRegion].getArmies() - nbArmies);
		if (regions[toRegion].getArmies() - std::round(nbArmies * 0.6) <= 0)
		{
			regions[toRegion].setArmies(nbArmies - std::round(regions[toRegion].getArmies() * 0.7));
			regions[toRegion].setOwner(regions[noRegion].getOwner());
		}
		else
		{
			regions[noRegion].setArmies(
					regions[noRegion].getArmies() + nbArmies - std::round(regions[toRegion].getArmies() * 0.7));
			regions[toRegion].setArmies(regions[toRegion].getArmies() - std::round(nbArmies * 0.6));
		}
	}
}

void Bot::resetRegionsOwned()
{
	owned.clear();
}

