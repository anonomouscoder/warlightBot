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
	// START HERE!
	if( regions.size() < 50 ) 
	{
		state = RAID;
		debug << "Choosing AI: RAID"<< std::endl;
	}
	else if( regions.size() >= 50 ) 
	{
		state = EMPIRE;
		debug << "Choosing AI: EMPIRE"<< std::endl;
	}
	if( state == RAID )
	{	
		std::cout << startingRegionsreceived.front() << std::endl;
	}
	if( state == EMPIRE )
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
	debug << "Calculating threat levels" << std::endl;
	debug << ".giving enemy's regions threat = 100" << std::endl;

	std::vector<Region>::iterator it2;
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
	for( it2 = regions.begin(); it2 != regions.end(); it2++)
	{
		debug << "region: " << it2->getId() << " threat: " << it2->threat << std::endl;
	}


	if( state == RAID )
	{
		unsigned region = std::rand() % owned.size();
		debug << "placing " << armiesLeft << " in " << owned[region] << std::endl;
		std::cout << botName << " place_armies " << owned[region] << " " << armiesLeft
				<< std::endl;
		addArmies(owned[region], armiesLeft);
	}
	if( state == EMPIRE )
	{
		round++;
		debug << "Round " << round << std::endl;
		unsigned target = 999;

		debug << "Placing Armies("<< timebank <<"): " << std::endl;
		debug << ".Choosing 2 from: [";
		for ( int i = 0; i < owned.size(); ++i) debug << owned[i] << ",";
		debug << "]" << std::endl;

		std::vector<std::pair<unsigned,unsigned>> weights;

		unsigned maxChecks = 100;
		std::vector<unsigned> unitsNeededList;
		for ( unsigned ownedIndex = 0; 
			  ownedIndex < std::min((unsigned)owned.size(),maxChecks); 
	 		  ++ownedIndex)
		{
			Region thisOwnedRegion = regions[owned[ownedIndex]];
			unsigned numberOfNeighbors = thisOwnedRegion.getNbNeighbors();
			bool regionHasEnemy = false;
			bool regionHasNeutral = false;
			unsigned weight = 1;
			unsigned unitsNeeded = 1;
			debug << ".Checking neighbors of " << owned[ownedIndex] << std::endl;
			for ( unsigned l = 0; l < std::min(numberOfNeighbors, maxChecks); ++l)
			{
				//debug << "..l = " << l << ", numberOfNeighbors = " << numberOfNeighbors << std::endl;
				//debug << "..weight of " << owned[ownedIndex] << " : " << weight << std::endl;
				Region thisNeighborRegion = regions[thisOwnedRegion.getNeighbor(l)];
				SuperRegion thisNeighborSuperRegion = superRegions[thisNeighborRegion.getSuperRegion()];

				//count how many regions of this superregion that I don't have
				std::vector<int>::iterator it;
				std::vector<int> regionsOfThisSuperRegion = superRegions[thisNeighborRegion.getSuperRegion()].getRegions();
				int countOfOwnedRegions = 0;
				for( it=regionsOfThisSuperRegion.begin(); it!=regionsOfThisSuperRegion.end(); it++)
				{
					if( regions[*it].getOwner() == ME ) countOfOwnedRegions++;
				}
	
				if( (unsigned)superRegions[thisNeighborRegion.getSuperRegion()].size() - countOfOwnedRegions == 1 )
				{
					debug << ", but this is the last region in this super region, weight +1000";
					weight += 1000;
				}

				if (thisNeighborRegion.getOwner() == ENEMY && !regionHasEnemy)
				{
					weight += 50;
					regionHasEnemy = true;
					debug << "+ 50 (enemy) ";
				}
				if (thisNeighborRegion.getOwner() == NEUTRAL && !regionHasNeutral)
				{
					weight += 100;
					regionHasNeutral = true;
					debug << "+ 100 (neutral) ";
				}
				if (thisNeighborRegion.getOwner() == NEUTRAL || thisNeighborRegion.getOwner() == ENEMY)
				{
					unitsNeeded += whatCanWin(thisNeighborRegion.getArmies()) +1;				
					debug << " unitsNeeded: " << unitsNeeded;
					if ( thisNeighborRegion.getSuperRegion() == thisOwnedRegion.getSuperRegion() )
					{
						weight += thisNeighborSuperRegion.getReward()*100 / thisNeighborSuperRegion.size();
						debug  << "..weight = " << thisNeighborSuperRegion.getReward()*100 / ( thisNeighborSuperRegion.size() - countOfOwnedRegions );
						debug << "(" << thisNeighborSuperRegion.getReward() 
							  << " / " << thisNeighborSuperRegion.size() << ") ";
					}

				}
			
			}
			debug << std::endl << ".done Checking neighbors of " << owned[ownedIndex] << " weight = " << weight << std::endl;
			if (thisOwnedRegion.getArmies() > 50) 
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
}

void Bot::makeMoves()
{
	// START HERE!
	if( state == RAID )
	{
		std::vector<std::string> moves;
		for (size_t j = 0; j < owned.size(); ++j)
		{
			std::stringstream move;
			int i = owned[j];
			if (regions[i].getArmies() <= 1)
				continue;

			int target = 1;
			// prefer highest threat
			int highestThreatSeen = 0;
			int highestThreatTarget = regions[i].getNeighbor(std::rand() % regions[i].getNbNeighbors());
			for ( unsigned k = 0; k < 5; ++k)
			{
				target = regions[i].getNeighbor(std::rand() % regions[i].getNbNeighbors());
				if( highestThreatSeen < regions[target].threat && canThisWin(regions[i].getArmies() - 1,regions[target].getArmies()) )
				{
					highestThreatSeen = regions[target].threat;
					highestThreatTarget = target;
				}
			}
			target = highestThreatTarget;
			debug << "moving " << (regions[i].getArmies() - 1) << " from " << i << " to "  << target << std::endl;
			move << botName << " attack/transfer " << i << " "
					<< target << " "
					<< (regions[i].getArmies() - 1);
			moves.push_back(move.str());
		}

		std::cout << string::join(moves) << std::endl;		
	}
	if( state == EMPIRE )
	{
		debug << "Making moves("<< timebank <<")" << std::endl;
	
		std::vector<std::string> moves;
		unsigned maxChecks = 100;
		std::vector<bool> attackedRegions = std::vector<bool>(regions.size(),false);
		for (unsigned ownedIndex = 0; ownedIndex < std::min((unsigned)owned.size(),maxChecks); ++ownedIndex)
		{
			Region thisOwnedRegion = regions[owned[ownedIndex]];
			//if this territory only has 1-4 army, skip it
			if (thisOwnedRegion.getArmies() <= 4)
				continue;
			std::stringstream move;
			debug << ".Looking at region " << owned[ownedIndex] 
				  << ": str= " << thisOwnedRegion.getArmies() 
				  << std::endl;
			unsigned numberOfNeighbors = thisOwnedRegion.getNbNeighbors();
			std::vector<std::pair<unsigned,unsigned>> weightedPossibleMoves;

			unsigned maxNeighborChecks = 100;
			bool hasBadNeighbor = false;
			bool saveForFinalRegionInSuperRegion = false;

			for ( unsigned neighborIndex = 0; neighborIndex < std::min(numberOfNeighbors,maxNeighborChecks); ++neighborIndex)
			{
				unsigned weight = 1;
				Region thisNeighborRegion = regions[thisOwnedRegion.getNeighbor(neighborIndex)];
				debug << "..neighbor " << thisOwnedRegion.getNeighbor(neighborIndex) 
		              << ": str= " << thisNeighborRegion.getArmies() 
		              << std::endl;
				switch(thisNeighborRegion.getOwner())
				{
					case ENEMY:
						debug << "...owned by enemy, weight == 20";
						weight = 20;
					case NEUTRAL: //or enemy
						if( weight != 20)
						{
							debug << "...owned by neutral, weight == 10";
							weight = 10;
						}
						if( thisNeighborRegion.getSuperRegion() == thisOwnedRegion.getSuperRegion() )
						{
							weight += 100;
							debug << ", in same superregion. weight + 100";	
							weight += weightedSuperRegionIndexList[thisNeighborRegion.getSuperRegion()].first;
							debug << " + "+weightedSuperRegionIndexList[thisNeighborRegion.getSuperRegion()].second;

							//count how many regions of this superregion that I don't have
							std::vector<int>::iterator it;
							std::vector<int> regionsOfThisSuperRegion = superRegions[thisNeighborRegion.getSuperRegion()].getRegions();
							int countOfOwnedRegions = 0;
							for( it=regionsOfThisSuperRegion.begin(); it!=regionsOfThisSuperRegion.end(); it++)
							{
								if( regions[*it].getOwner() == ME ) countOfOwnedRegions++;
							}
					
							if( (unsigned)superRegions[thisNeighborRegion.getSuperRegion()].size() - countOfOwnedRegions == 1 )
							{
								debug << ", but this is the last region in this super region, don't move troops from thisOwnedRegion";
								saveForFinalRegionInSuperRegion = true;
							}
						}
						if( canThisWin( thisOwnedRegion.getArmies()-1, thisNeighborRegion.getArmies() ) )
						{
							weight += 1000;
							debug << ", conquerable. weight + 1000";
							//no need to save, i can kill it
							saveForFinalRegionInSuperRegion = false;
						}
					
						if( attackedRegions[thisNeighborRegion.getId()] )
						{
							debug << ", but this region is already being attacked, so treat it like it's already mine";
							weight -= 50;
						}
						debug << std::endl;
						hasBadNeighbor = true;
						break;
					default:
					case ME:
						weight = 500;
						debug << "...owned by me, weight == 500 ";
						if( (thisNeighborRegion.getArmies()-1) > (thisOwnedRegion.getArmies() - 1) )
						{
							weight += 15;
							debug << " + 15 because it's stronger " << std::endl;
						}
					
						break;
				}
			
				weightedPossibleMoves.push_back( std::pair<unsigned,unsigned>(thisOwnedRegion.getNeighbor(neighborIndex),weight) );

			}
			//if a region has no bad neighbor, add the threat of each region to the moves to that region
			//so it should move it's armies toward the highest threat (i.e. the enemy)
			std::vector<std::pair<unsigned,unsigned>>::iterator it;
			if( !hasBadNeighbor ) 
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
				debug << it->first << ":" << it->second << ", ";
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
				if( !hasBadNeighbor ) weightToMove = 0;

				// don't add a move unless a good one is found
				if (target != 0 && weight > weightToMove && !saveForFinalRegionInSuperRegion)
				{
					debug << ".target has enough weight to move there"<<std::endl;
					if( regions[target].getOwner() != ME) attackedRegions[target] = true;
					unsigned defenseStr = regions[target].getArmies();
			
					unsigned strengthNeeded = whatCanWin(defenseStr) + 1;
					unsigned possibleStrengthNeeded = whatCanWin(defenseStr + 8) + 1;
					if( possibleStrengthNeeded <= attackingArmy ) strengthNeeded = possibleStrengthNeeded;

					debug << ".stengthNeeded: " << strengthNeeded << ",attackers: " << attackingArmy << std::endl;
					debug << ".moving " << std::min( strengthNeeded,attackingArmy) << " to " << target << std::endl;
					move <<" "<< botName << " attack/transfer " << owned[ownedIndex] << " "
							<< target << " "
							<< std::min( strengthNeeded, attackingArmy);
					moves.push_back(move.str());
					debug << ".taking: " << std::min( strengthNeeded, attackingArmy) << " off of attackingArmy" << std::endl;
					attackingArmy -= std::min( strengthNeeded, attackingArmy);
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
			debug <<"." << string::join(moves) << std::endl;
			std::cout << string::join(moves) << std::endl;
		}
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

