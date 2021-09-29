#include "ExampleAIModule.h" 

#include <algorithm>

using namespace BWAPI;

bool analyzed;
bool analysis_just_finished;
BWTA::Region* home;
BWTA::Region* enemy_base;

//This is the startup method. It is called once
//when a new game has been started with the bot.
///																																	
void ExampleAIModule::onStart()
{
	//Enable flags
	Broodwar->enableFlag(Flag::UserInput);
	//Uncomment to enable complete map information
	//Broodwar->enableFlag(Flag::CompleteMapInformation);

	// Start possition flag
	m_startAtTopLeft = Broodwar->self()->getStartLocation().x < Broodwar->mapWidth() / 2 && Broodwar->self()->getStartLocation().y < Broodwar->mapHeight() / 2;

	
	

	//Start analyzing map data
	BWTA::readMap();
	analyzed = false;
	analysis_just_finished = false;
	//CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeThread, NULL, 0, NULL); //Threaded version
	AnalyzeThread();
	Broodwar->setLocalSpeed(5);
	


	m_allocatedMinerals = 0;
	m_allocatedGas = 0;
	m_nrOfGasWorkers = 0;

	// Find the initial command center
	BWAPI::Unitset initialBuildings = Broodwar->getUnitsOnTile(Broodwar->self()->getStartLocation(), BWAPI::Filter::IsBuilding);
	BWAPI::Unit commandCenter;
	commandCenter = *initialBuildings.begin();

	if (m_startAtTopLeft)
	{
		m_armyRallypoints.push_back(BWAPI::Position(2750, 650));  // top side 1st troop rally
		m_armyRallypoints.push_back(BWAPI::Position(1600, 1650)); // top side 2nd troop rally
		//m_armyRallypoints.push_back(BWAPI::Position(2360, 1400)); // top side 3rd troop rally
		m_armyRallypoints.push_back(BWAPI::Position(2300, 2230)); // outside enemy base
		m_armyRallypoints.push_back(BWAPI::Position(2600, 2340)); // outskirt enemy base 1
		m_armyRallypoints.push_back(BWAPI::Position(2990, 2340)); // outskirt enemy base 2
		m_armyRallypoints.push_back(BWAPI::Position(3300, 2440)); // enemy base

		// continue when home base is defeated
		m_armyRallypoints.push_back(BWAPI::Position(250, 1900)); // left base
		m_armyRallypoints.push_back(BWAPI::Position(318, 2790)); // bottom left base
		m_armyRallypoints.push_back(BWAPI::Position(3750, 1100)); // right base
		m_armyRallypoints.push_back(BWAPI::Position(3700, 220)); // top right base
		m_armyRallypoints.push_back(BWAPI::Position(2990, 2340)); // enemy base


		m_miningSites.push_back({ tileToPosition(Broodwar->self()->getStartLocation()), commandCenter});
		m_miningSites.push_back({ BWAPI::Position(3750, 1100) }); // right base
		m_miningSites.push_back({ BWAPI::Position(2000, 1500)}); // center base
		m_miningSites.push_back({ BWAPI::Position(3700, 220)}); // top right base


		m_buildingPositions.push_back(BWAPI::Position(2600, 720)); // missile turret
		m_buildingPositions.push_back(BWAPI::Position(2600, 650));
		m_buildingPositions.push_back(BWAPI::Position(2750, 550));
	}
	else
	{
		m_armyRallypoints.push_back(BWAPI::Position(1450, 2180)); // bot side 1st troop rally
		m_armyRallypoints.push_back(BWAPI::Position(2360, 1400)); // bot side 2nd troop rally
		//m_armyRallypoints.push_back(BWAPI::Position(1600, 1650)); // bot side 3rd troop rally
		m_armyRallypoints.push_back(BWAPI::Position(1700, 650));  // outside enemy base
		m_armyRallypoints.push_back(BWAPI::Position(1400, 650));  // outskirt enemy base 1
		m_armyRallypoints.push_back(BWAPI::Position(1100, 650));  // outskirt enemy base 2
		m_armyRallypoints.push_back(BWAPI::Position(800, 550));  // enemy base

		// continue when home base is defeated
		m_armyRallypoints.push_back(BWAPI::Position(3800, 180)); // top right base ONLY OFFENSIVE TARGET
		m_armyRallypoints.push_back(BWAPI::Position(3750, 1100)); // right base
		m_armyRallypoints.push_back(BWAPI::Position(250, 1900)); // left base
		m_armyRallypoints.push_back(BWAPI::Position(318, 2790)); // bottom left base
		m_armyRallypoints.push_back(BWAPI::Position(1100, 650));  // enemy base


		m_miningSites.push_back({ tileToPosition(Broodwar->self()->getStartLocation()), commandCenter});
		m_miningSites.push_back({ BWAPI::Position(250, 1900) }); // left base
		m_miningSites.push_back({ BWAPI::Position(2000, 1500)}); // center base
		m_miningSites.push_back({ BWAPI::Position(318, 2790)}); // bottom left base


		m_buildingPositions.push_back(BWAPI::Position(1550, 2280)); // missile turret
		m_buildingPositions.push_back(BWAPI::Position(2600, 650)); 
		m_buildingPositions.push_back(BWAPI::Position(2750, 550));
	}

	/// SCOUTING ///
	int tmpCounter = 0;
	BWAPI::Unitset units = Broodwar->self()->getUnits();
	for (auto u : units)
	{
		if (u->getType() == BWAPI::UnitTypes::Terran_SCV)
		{
			tmpCounter++;
			if (tmpCounter >= 4)
			{
				u->move(m_armyRallypoints[4]);
				u->move(tileToPosition(Broodwar->self()->getStartLocation()), true);
			}
		}
	}

	m_miningSites[0].isBuildingRefinery = true;
	m_furthestArmyRallyPoint = 0;

	/// Stages ///

	m_currentStageIndex = 0;

	int nrOf_Supply_depot = 0;
	//int nrOf_Refineries = 0;
	int nrOf_Barracks = 0;
	/*int nrOf_Marines = 0;
	int nrOf_Medics = 0;
	int nrOf_Firebats = 0;
	int nrOf_Tanks = 0;
	int nrOf_Wraiths = 0;
	int nrOf_Factories = 0;
	int nrOf_CommandCenters = 1;
	int nrOf_EngineeringBay = 0;
	int nrOf_Academy = 0;
	int nrOf_Bunkers = 0;
	int nrOf_Starports = 0;
	int nrOf_MissileTurrets = 0;*/

	StageVector* stages = m_stageStorage.getStages();
	//m_NormalSpeedAfterStage = 8;
	
	stages->push_back(std::vector<StageGoal>());
	stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 1, 110));

	stages->push_back(std::vector<StageGoal>());
	stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 1));
	stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 1, 160));
	////////////////////////////////////////////////////////////////////////////////////////////////////////


}

//Called when a game is ended.
//No need to change this.
void ExampleAIModule::onEnd(bool isWinner)
{
	if (isWinner)
	{
		Broodwar->sendText("I won!");
	}
}

//Finds a guard point around the home base.
//A guard point is the center of a chokepoint surrounding
//the region containing the home base.
Position ExampleAIModule::findGuardPoint()
{
	//Get the chokepoints linked to our home region
	std::set<BWTA::Chokepoint*> chokepoints = home->getChokepoints();
	double min_length = 10000;
	BWTA::Chokepoint* choke = NULL;

	//Iterate through all chokepoints and look for the one with the smallest gap (least width)
	for (std::set<BWTA::Chokepoint*>::iterator c = chokepoints.begin(); c != chokepoints.end(); c++)
	{
		double length = (*c)->getWidth();
		if (length < min_length || choke == NULL)
		{
			min_length = length;
			choke = *c;
		}
	}

	return choke->getCenter();
}




///																																	
void ExampleAIModule::onFrame()
{

	//Call every Frame
	frames++;
	for (int i = 0; i < m_buildingTimerList.size(); i++)
	{
		m_buildingTimerList[i].m_frame++;
	}
	updateBuildingTimer();
	///			

	if (frames > 6000)
	{
		m_flagExpand = true;
	}

	if (!m_flagStartBuildOfWorkerPos)
	{
		if (m_currentStageIndex > 6)
		{
			m_flagStartBuildOfWorkerPos = true;
		}
	}

	if (!m_flagStrategyChoice)
	{
		if (Broodwar->enemy()->getRace() == BWAPI::Races::Protoss || Broodwar->enemy()->getRace() == BWAPI::Races::Terran || Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
		{
			StageVector* stages = m_stageStorage.getStages();

			int nrOf_Supply_depot = 2;
			int nrOf_Refineries = 0;
			int nrOf_Barracks = 1;
			int nrOf_Marines = 0;
			int nrOf_Medics = 0;
			int nrOf_Firebats = 0;
			int nrOf_Tanks = 0;
			int nrOf_Wraiths = 0;
			int nrOf_Factories = 0;
			int nrOf_CommandCenters = 1;
			int nrOf_EngineeringBay = 0;
			int nrOf_Academy = 0;
			int nrOf_Bunkers = 0;
			int nrOf_Starports = 0;
			int nrOf_MissileTurrets = 0;
			int nrOf_Armory = 0;
			int nrOf_ScienceFacility = 0;

			/*	if (Broodwar->enemy()->getRace() == BWAPI::Races::Protoss || Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
			{
			Broodwar->leaveGame();
			}*/

			if (Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
			{
				stages->push_back(std::vector<StageGoal>());
				//stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 4, 90));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Refinery, nrOf_Refineries += 1, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Academy, nrOf_Academy += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 1, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 1, 150));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Firebat, nrOf_Firebats += 2));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Engineering_Bay, nrOf_EngineeringBay += 1));

				stages->push_back(std::vector<StageGoal>());
				//stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Factory, nrOf_Factories += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 2, 160));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Firebat, nrOf_Firebats += 7));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 5));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Missile_Turret, nrOf_MissileTurrets += 1, 0, 0, m_buildingPositions[0]));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 8));
				//stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 2));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 1, 150));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Medic, nrOf_Medics += 3));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 1));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 150));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 12));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Medic, nrOf_Medics += 2));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 150));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 20));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Medic, nrOf_Medics += 2));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Command_Center, nrOf_CommandCenters += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Starport, nrOf_Starports += 1, 200));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Factory, nrOf_Factories += 1));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Wraith, nrOf_Wraiths += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Factory, nrOf_Factories += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 7, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 3, 120));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 14, 150));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Medic, nrOf_Medics += 3));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Science_Facility, nrOf_ScienceFacility += 1));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Command_Center, nrOf_CommandCenters += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 4, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 120));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Wraith, nrOf_Wraiths += 1));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Command_Center, nrOf_CommandCenters += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 8, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 120));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 10, 100));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 100, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 10, 1000));

			}
			else if (Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
			{

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 4, 90));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Refinery, nrOf_Refineries += 1, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 1, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 1, 150));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Academy, nrOf_Academy += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Factory, nrOf_Factories += 1, 210));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 1, 200));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 1, 190));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 10));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Medic, nrOf_Medics += 2));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 4, 110));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 1, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 1, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Factory, nrOf_Factories += 1, 400, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 5, 200));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 7, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 3, 120));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 16, 150));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Medic, nrOf_Medics += 6, 150));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Command_Center, nrOf_CommandCenters += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 4, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 120));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 8, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 120));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Command_Center, nrOf_CommandCenters += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 10, 100));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 100, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 10, 1000));
			}
			else if (Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
			{

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 4, 90));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Refinery, nrOf_Refineries += 1, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 130));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 1, 150));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Academy, nrOf_Academy += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Factory, nrOf_Factories += 1, 210));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 1, 200));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 1, 190));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 10));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Medic, nrOf_Medics += 2));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 1, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 3, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Factory, nrOf_Factories += 1, 200, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 8));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Engineering_Bay, nrOf_EngineeringBay += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Barracks, nrOf_Barracks += 1, 220));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Command_Center, nrOf_CommandCenters += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 7, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 3, 120));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Marine, nrOf_Marines += 16, 150));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Medic, nrOf_Medics += 6, 150));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Command_Center, nrOf_CommandCenters += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 4, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 120));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 8, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 2, 120));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Command_Center, nrOf_CommandCenters += 1));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 10, 100));

				stages->push_back(std::vector<StageGoal>());
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, nrOf_Tanks += 100, 100));
				stages->back().push_back(StageGoal(BWAPI::UnitTypes::Terran_Supply_Depot, nrOf_Supply_depot += 10, 1000));

			}

			m_flagStrategyChoice = true;
		}
	}

	//Broodwar->setScreenPosition(BWAPI::Position(m_armyRallypoints[m_furthestArmyRallyPoint].x - 320, m_armyRallypoints[m_furthestArmyRallyPoint].y - 200));
	///			

	//Call every n:th frame
	if (Broodwar->getFrameCount() % 5 == 0)
	{
		m_currentStageIndex = 0;
		bool isStageComplete = true;


		StageVector* stages = m_stageStorage.getStages();
		for (int i = 0; i < stages->size(); i++)
		{
			Stage& stage = stages->operator[](i);

			for (int j = 0; j < stage.size(); j++)
			{
				if (Broodwar->self()->completedUnitCount(stage[j].type) < stage[j].n)
				{
					isStageComplete = false;
					break;
				}
			}

			if (!isStageComplete)
			{
				break;
			}
			else
			{
				m_currentStageIndex = std::min(m_currentStageIndex + 1, static_cast<int>(stages->size()) - 1);
			}
		}
		///																																	
		if (m_currentStageIndex >= m_NormalSpeedAfterStage)
		{
			Broodwar->setLocalSpeed(10);
		}

		// used to check which workers actually does their job.
		int nrOfWorkingWorkers = 0;
		m_nrOfGasWorkers = 0;

		BWAPI::Unitset units = Broodwar->self()->getUnits();

		int i_commandCenter = 0;



		/// THIS IF-SATS IS FOR HANDELING RUSH STRATEGIES ///

		if (m_baseState == SAFE)
		{

			for (auto u : units)
			{
				if (u->isBeingConstructed())
				{
					continue;
				}

				BWAPI::UnitType type = u->getType();
				if (type.isWorker())
				{
					manageWorker(u);
					if (u->isConstructing())
					{
						nrOfWorkingWorkers++;
					}
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Command_Center)
				{
					manageCommandCenter(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Barracks)
				{
					manageBarrack(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Marine)
				{
					manageMarine(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Firebat)
				{
					manageFirebat(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Medic)
				{
					manageMedic(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode || type == BWAPI::UnitTypes::Enum::Terran_Siege_Tank_Siege_Mode)
				{
					manageTank(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Wraith)
				{
					manageWraith(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Factory)
				{
					manageFactory(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Starport)
				{
					manageFactory(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Academy)
				{
					manageAcademy(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Engineering_Bay)
				{
					manageEngineeringBay(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Machine_Shop)
				{
					manageMachineShop(u);
				}
			} // for

			if (Broodwar->getUnitsInRadius(tileToPosition(Broodwar->self()->getStartLocation()), 14 * 32, BWAPI::Filter::IsEnemy).size() > 1)
			{
				if (Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
				{
					m_baseState = ATTACKED_BY_ZERG;

					for (auto u : units)
					{
						if (u->getType().isWorker() || u->getType() == BWAPI::UnitTypes::Terran_Marine)
						{
							u->stop();
						}
					}
				}
				
			}
		}
		else if (m_baseState == ATTACKED_BY_ZERG)
		{
			//Broodwar->setLocalSpeed(10); /////////////////////////////////////////////////////////////////////?

			for (auto u : units)
			{
				BWAPI::UnitType type = u->getType();
				if (type == BWAPI::UnitTypes::Terran_Marine || type == BWAPI::UnitTypes::Terran_SCV)
				{
					if (u->isIdle())
					{
						int variansX = rand() % 200 - 100;
						int variansY = rand() % 200 - 100;
						u->attack(BWAPI::Position(tileToPosition(Broodwar->self()->getStartLocation()).x + variansX, tileToPosition(Broodwar->self()->getStartLocation()).y + variansY));
					}
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Command_Center)
				{
					manageCommandCenter(u);
				}
				else if (type == BWAPI::UnitTypes::Enum::Terran_Barracks)
				{
					manageBarrack(u);
				}
			}

			if (Broodwar->getUnitsInRadius(tileToPosition(Broodwar->self()->getStartLocation()), 14 * 32, BWAPI::Filter::IsEnemy).size() == 0)
			{
				m_baseState = SAFE;
			}

		}
		
		
		manageTroops(units);


		// THIS IS IMPORTANT
		int firstSiteToEvaluate = 1;


		for (int i = firstSiteToEvaluate; i < m_miningSites.size(); i++)
		{
			if (m_miningSites[i].commandCenter)
			{
				if (m_miningSites[i].commandCenter->exists())
				{
					if (m_miningSites[i].commandCenter->getTilePosition() == positionToTile(m_miningSites[i].position))
					{
						if (!m_miningSites[i].commandCenter->isFlying())
						{
							BWAPI::Unitset workers = Broodwar->getUnitsInRadius(m_miningSites[i].position, 8 * 32, BWAPI::Filter::IsWorker);
							int n_mineralFields = Broodwar->getUnitsInRadius(m_miningSites[i].position, 8 * 32, BWAPI::Filter::IsMineralField).size();

							if (workers.size() < n_mineralFields * 3)
							{
								if (m_miningSites[i].commandCenter->isIdle())
								{

									int mineralPrice = BWAPI::UnitTypes::Terran_SCV.mineralPrice();
									if (Broodwar->self()->minerals() - m_allocatedMinerals >= mineralPrice)
									{
										m_allocatedMinerals += mineralPrice;
										m_miningSites[i].commandCenter->train(BWAPI::UnitTypes::Enum::Terran_SCV);
									}
								}
							}

							if (m_miningSites[i].refinery)
							{
								if (m_miningSites[i].refinery->exists())
								{
									if (m_miningSites[i].refinery->getResources() > 0)
									{
										if (workers.size() > 7)
										{
											
											/*BWAPI::Unitset x = Broodwar->self()->getUnits();
											for (auto y : x)
											{
												if (y->getPosition().getApproxDistance(m_miningSites[i].position) < 500)
												{
													if (y->getType().isWorker())
													{
														y->getClosestUnit(BWAPI::Filter::IsRefinery);
													}
												}
											}*/

											auto it = workers.begin();
											for (int j = 0; j < 2; j++)
											{
												if ((*it)->isGatheringMinerals() || (*it)->isIdle())
												{
													(*it)->gather((*it)->getClosestUnit(BWAPI::Filter::IsRefinery));
													it++;
												}
											}
										}
									}
								}
								else
								{
									m_miningSites[i].refinery = nullptr;
								}
							}
							else
							{
								if (!m_miningSites[i].isBuildingRefinery)
								{
									if (workers.size() > 7)
									{
										if ((*workers.begin())->isGatheringMinerals())
										{
											int mineralPrice = BWAPI::UnitTypes::Terran_Refinery.mineralPrice();
											if (Broodwar->self()->minerals() - m_allocatedMinerals >= mineralPrice)
											{
												m_allocatedMinerals += mineralPrice;
												(*workers.begin())->build(BWAPI::UnitTypes::Enum::Terran_Refinery, Broodwar->getBuildLocation(BWAPI::UnitTypes::Enum::Terran_Refinery, positionToTile(m_miningSites[i].position)));
												m_miningSites[i].isBuildingRefinery = true;
											}
										}
									}
								}
							}
						}
					}
				}
				else
				{
					m_miningSites[i].commandCenter = nullptr;
				}
			}
		}

	}

	if (Broodwar->getFrameCount() % 200 == 0)
	{
		BWAPI::Unitset units = Broodwar->self()->getUnits();
		for (auto u : units)
		{
			if (!u->isCompleted())
			{
				if (u->getType().isBuilding())
				{
					if (!u->getBuildUnit())
					{
						for (auto v : units)
						{
							if (v->getType().isWorker())
							{
								if (v->isGatheringMinerals())
								{
									v->rightClick(u);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	


	//Draw lines around regions, chokepoints etc.
	if (analyzed)
	{
		drawTerrainData();
	}

	// Marks the Rally point spot
	for (int i = 0; i < m_armyRallypoints.size(); i++)
	{
		Broodwar->drawCircleMap(m_armyRallypoints[i], 10, BWAPI::Colors::Red, true);
	}

	// Marks the Mining site
	for (int i = 0; i < m_miningSites.size(); i++)
	{
		Broodwar->drawCircleMap(m_miningSites[i].position, 10, BWAPI::Colors::Green, true);
	}

	// Marks the building locations
	for (int i = 0; i < m_buildingPositions.size(); i++)
	{
		Broodwar->drawCircleMap(m_buildingPositions[i], 10, BWAPI::Colors::Blue, true);
	}
	
	// indicates the worker attack zone for rushes.
	Broodwar->drawCircleMap(tileToPosition(Broodwar->self()->getStartLocation()), 14 * 32, BWAPI::Colors::Red);

	Broodwar->drawTextScreen(70, 1, "Allocated: %d:%d", m_allocatedMinerals, m_allocatedGas);
	Broodwar->drawTextScreen(250, 1, "Construction workers: %d", m_constructingWorkers.size());
	Broodwar->drawTextScreen(5, 1, "Stage: %d/%d", (m_currentStageIndex + 1), m_stageStorage.getStages()->size());
	Broodwar->drawTextScreen(250, 10, "Enemy: %s |<>| fail guys: %d", Broodwar->enemy()->getRace().c_str(), failures);
	Broodwar->drawTextScreen(250, 20, "Frame timer: %d", frames);

	//Broodwar->drawTextScreen(500, 15, "Score       : %d", Broodwar->self()->getUnitScore());
	//Broodwar->drawTextScreen(500, 25, "Enemy Score: %d", Broodwar->enemy()->getCustomScore());

	Stage& stage = m_stageStorage.getStages()->operator[](m_currentStageIndex);
	for (int i = 0; i < stage.size(); i++)
	{
		int nUnitsOfThisType = Broodwar->self()->completedUnitCount(stage[i].type);
		int nUnitsOfThisTypeDemand = stage[i].n;
		Broodwar->drawTextScreen(5, 1 + 10 * (i + 1), "%s: %d/%d", stage[i].type.getName().c_str(), nUnitsOfThisType, nUnitsOfThisTypeDemand);
	}

	for (int i = 0; i < stage.size(); i++)
	{
		Broodwar->drawTextScreen(250, 30 + 10 * i, "%s: %d",stage[i].type.getName().c_str(), stage[i].nBeingConstructed);
	}

	for (int i = 0; i < m_buildingTimerList.size(); i++)
	{
		Broodwar->drawTextScreen(0, 100 + 10 * i, "Timer %d: %d", i, m_buildingTimerList[i].m_frame);
	}

	std::string troopState;
	switch (m_troopState)
	{
	case ExampleAIModule::RETREAT:
		troopState = "RETREAT";
		break;
	case ExampleAIModule::HOLD:
		troopState = "HOLD";
		break;
	case ExampleAIModule::ADVANCE:
		troopState = "ADVANCE";
		break;
	default:
		break;
	}

	std::string baseState;
	switch (m_baseState)
	{
	case ExampleAIModule::SAFE:
		baseState = "SAFE";
		break;
	case ExampleAIModule::ATTACKED_BY_ZERG:
		baseState = "ATTACKED BY ZERG";
		break;
	default:
		break;
	}

	Broodwar->drawTextScreen(25, 300, "Troop state: %s", troopState.c_str());
	Broodwar->drawTextScreen(25, 290, "Base  state: %s", baseState.c_str());
}


//Is called when text is written in the console window.
//Can be used to toggle stuff on and off.
void ExampleAIModule::onSendText(std::string text)
{
	if (text == "/show players")
	{
		showPlayers();
	}
	else if (text == "/show forces")
	{
		showForces();
	}
	else
	{
		Broodwar->printf("You typed '%s'!", text.c_str());
		Broodwar->sendText("%s", text.c_str());
	}
}

//Called when the opponent sends text messages.
//No need to change this.
void ExampleAIModule::onReceiveText(BWAPI::Player player, std::string text)
{
	Broodwar->printf("%s said '%s'", player->getName().c_str(), text.c_str());
}

//Called when a player leaves the game.
//No need to change this.
void ExampleAIModule::onPlayerLeft(BWAPI::Player player)
{
	Broodwar->sendText("%s left the game.", player->getName().c_str());
}

//Called when a nuclear launch is detected.
//No need to change this.
void ExampleAIModule::onNukeDetect(BWAPI::Position target)
{
	if (target != Positions::Unknown)
	{
		Broodwar->printf("Nuclear Launch Detected at (%d,%d)", target.x, target.y);
	}
	else
	{
		Broodwar->printf("Nuclear Launch Detected");
	}
}

//No need to change this.
void ExampleAIModule::onUnitDiscover(BWAPI::Unit unit)
{
	//Broodwar->sendText("A %s [%x] has been discovered at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x,unit->getPosition().y);
}

//No need to change this.
void ExampleAIModule::onUnitEvade(BWAPI::Unit unit)
{
	//Broodwar->sendText("A %s [%x] was last accessible at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x,unit->getPosition().y);
}

//No need to change this.
void ExampleAIModule::onUnitShow(BWAPI::Unit unit)
{
	//Broodwar->sendText("A %s [%x] has been spotted at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x,unit->getPosition().y);
}

//No need to change this.
void ExampleAIModule::onUnitHide(BWAPI::Unit unit)
{
	//Broodwar->sendText("A %s [%x] was last seen at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x,unit->getPosition().y);
}

//Called when a new unit has been created.
//Note: The event is called when the new unit is built, not when it
//has been finished.
///																																	
void ExampleAIModule::onUnitCreate(BWAPI::Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		//Broodwar->sendText("A %s [%x] has been created at (%d,%d)", unit->getType().getName().c_str(), unit, unit->getPosition().x, unit->getPosition().y);

		m_allocatedMinerals -= unit->getType().mineralPrice();
		m_allocatedGas -= unit->getType().gasPrice();

		m_allocatedMinerals = std::max(m_allocatedMinerals, 0);
		m_allocatedGas = std::max(m_allocatedGas, 0);

		Stage& currentStage = m_stageStorage.getStages()->operator[](m_currentStageIndex);
		for (int i = 0; i < currentStage.size(); i++)
		{
			if (currentStage[i].type == unit->getType())
			{
				currentStage[i].nBeingConstructed--;
				currentStage[i].nBeingConstructed = std::max(currentStage[i].nBeingConstructed, 0);
			}
		}

		if (unit->getType() == BWAPI::UnitTypes::Terran_Barracks)
		{
			//BWTA::Region r = BWTA::getNearestChokepoint(tileToPosition(Broodwar->self()->getStartLocation()))->;
			//unit->setRallyPoint(m_armyRallypoints[m_furthestArmyRallyPoint]);
		}
		if (unit->getType() == BWAPI::UnitTypes::Terran_Machine_Shop)
		{
			m_upgradingFactories.erase(
				std::remove(
					m_upgradingFactories.begin(),
					m_upgradingFactories.end(),
					unit->getBuildUnit()->getID()
				),
				m_upgradingFactories.end()
			);
			//unit->getBuildUnit()->setRallyPoint(m_armyRallypoints[m_furthestArmyRallyPoint]);
		}
		
		// Updating the building timer vector
		if (unit->getBuildUnit() != nullptr && unit->getBuildUnit()->getType() == BWAPI::UnitTypes::Terran_SCV)
		{
			for (int i = 0; i < m_buildingTimerList.size(); i++)
			{
				
				if (unit->getBuildUnit()->getID() == m_buildingTimerList[i].m_workerID)
				{
					m_buildingTimerList.erase(std::remove(m_buildingTimerList.begin(), m_buildingTimerList.end(), m_buildingTimerList[i]), m_buildingTimerList.end());
					i = m_buildingTimerList.size();
				}
				
			}
		}
		

	}
}

//Called when a unit has been destroyed.
void ExampleAIModule::onUnitDestroy(BWAPI::Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{

		for (int i = 0; i < m_constructingWorkers.size(); i++)
		{
			if (m_constructingWorkers[i].workerID == unit->getID())
			{
				Stage& stage = (*m_stageStorage.getStages())[m_currentStageIndex];
				for (int j = 0; j < stage.size(); j++)
				{
					if (stage[j].type == m_constructingWorkers[i].buildingType)
					{
						stage[j].nBeingConstructed--;
						stage[i].nBeingConstructed = std::max(stage[i].nBeingConstructed, 0);
						break;

					}
				}
				m_constructingWorkers[i] = m_constructingWorkers.back();
				m_constructingWorkers.pop_back();
				break;
			}
		}


		//Broodwar->sendText("My unit %s [%x] has been destroyed at (%d,%d)", unit->getType().getName().c_str(), unit, unit->getPosition().x, unit->getPosition().y);
		

	}
	else
	{
		//Broodwar->sendText("Enemy unit %s [%x] has been destroyed at (%d,%d)", unit->getType().getName().c_str(), unit, unit->getPosition().x, unit->getPosition().y);
	}
}

//Only needed for Zerg units.
//No need to change this.
void ExampleAIModule::onUnitMorph(BWAPI::Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		m_allocatedMinerals -= unit->getType().mineralPrice();
		m_allocatedGas -= unit->getType().gasPrice();

		m_allocatedMinerals = std::max(m_allocatedMinerals, 0);
		m_allocatedGas = std::max(m_allocatedGas, 0);

		Stage& currentStage = m_stageStorage.getStages()->operator[](m_currentStageIndex);
		for (int i = 0; i < currentStage.size(); i++)
		{
			if (currentStage[i].type == unit->getType())
			{
				currentStage[i].nBeingConstructed--;
				currentStage[i].nBeingConstructed = std::max(currentStage[i].nBeingConstructed, 0);
			}
		}

		if (unit->getType().isRefinery())
		{
			int site_index = Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Command_Center) - 1;
			if (m_miningSites[site_index].refinery == nullptr)
			{
				m_miningSites[site_index].refinery = unit;
			}
		}


		// Updating the building timer vector
		if (unit->getBuildUnit() != nullptr && unit->getBuildUnit()->getType() == BWAPI::UnitTypes::Terran_SCV)
		{
			for (int i = 0; i < m_buildingTimerList.size(); i++)
			{

				if (unit->getBuildUnit()->getID() == m_buildingTimerList[i].m_workerID)
				{
					m_buildingTimerList.erase(std::remove(m_buildingTimerList.begin(), m_buildingTimerList.end(), m_buildingTimerList[i]), m_buildingTimerList.end());
					i = m_buildingTimerList.size();
				}

			}
		}



	} // if




}

//No need to change this.
void ExampleAIModule::onUnitRenegade(BWAPI::Unit unit)
{
	//Broodwar->sendText("A %s [%x] is now owned by %s",unit->getType().getName().c_str(),unit,unit->getPlayer()->getName().c_str());
}

//No need to change this.
void ExampleAIModule::onSaveGame(std::string gameName)
{
	Broodwar->printf("The game was saved to \"%s\".", gameName.c_str());
}

///																																	
void ExampleAIModule::onUnitComplete(BWAPI::Unit unit)
{
	//Broodwar->sendText("A %s [%x] has been completed at (%d,%d)",unit->getType().getName().c_str(),unit,unit->getPosition().x,unit->getPosition().y);

	///			
	if (unit->getType() == BWAPI::UnitTypes::Terran_Command_Center && m_flagExpand)
	{
		unit->lift();
	}
	///			

	if (unit->getType() == BWAPI::UnitTypes::Terran_Marine)
	{
		Broodwar->printf("Marine at TIME : %d", frames);
	}

	for (int i = 0; i < m_constructingWorkers.size(); i++)
	{
		if (!Broodwar->getUnit(m_constructingWorkers[i].workerID)->isConstructing())
		{
			//Broodwar->printf("Worker %d has finished building a %s", m_constructingWorkers[i].workerID, m_constructingWorkers[i].buildingType.getName().c_str());

			m_constructingWorkers.erase(
				std::remove(
				m_constructingWorkers.begin(),
				m_constructingWorkers.end(),
				m_constructingWorkers[i]
				),
				m_constructingWorkers.end()
				);
			break;
		}
	} // for
}

//Analyzes the map.
//No need to change this.
DWORD WINAPI AnalyzeThread()
{
	BWTA::analyze();

	//Self start location only available if the map has base locations
	if (BWTA::getStartLocation(BWAPI::Broodwar->self()) != NULL)
	{
		home = BWTA::getStartLocation(BWAPI::Broodwar->self())->getRegion();
	}
	//Enemy start location only available if Complete Map Information is enabled.
	if (BWTA::getStartLocation(BWAPI::Broodwar->enemy()) != NULL)
	{
		enemy_base = BWTA::getStartLocation(BWAPI::Broodwar->enemy())->getRegion();
	}
	analyzed = true;
	analysis_just_finished = true;
	return 0;
}

//Prints some stats about the units the player has.
//No need to change this.
void ExampleAIModule::drawStats()
{
	BWAPI::Unitset myUnits = Broodwar->self()->getUnits();
	Broodwar->drawTextScreen(5, 0, "I have %d units:", myUnits.size());
	std::map<UnitType, int> unitTypeCounts;
	for (auto u : myUnits)
	{
		if (unitTypeCounts.find(u->getType()) == unitTypeCounts.end())
		{
			unitTypeCounts.insert(std::make_pair(u->getType(), 0));
		}
		unitTypeCounts.find(u->getType())->second++;
	}
	int line = 1;
	for (std::map<UnitType, int>::iterator i = unitTypeCounts.begin(); i != unitTypeCounts.end(); i++)
	{
		Broodwar->drawTextScreen(5, 16 * line, "- %d %ss", i->second, i->first.getName().c_str());
		line++;
	}
}

//Draws terrain data aroung regions and chokepoints.
//No need to change this.
void ExampleAIModule::drawTerrainData()
{
	//Iterate through all the base locations, and draw their outlines.
	for (auto bl : BWTA::getBaseLocations())
	{
		TilePosition p = bl->getTilePosition();
		Position c = bl->getPosition();
		//Draw outline of center location
		Broodwar->drawBox(CoordinateType::Map, p.x * 32, p.y * 32, p.x * 32 + 4 * 32, p.y * 32 + 3 * 32, Colors::Blue, false);
		//Draw a circle at each mineral patch
		for (auto m : bl->getStaticMinerals())
		{
			Position q = m->getInitialPosition();
			Broodwar->drawCircle(CoordinateType::Map, q.x, q.y, 30, Colors::Cyan, false);
		}
		//Draw the outlines of vespene geysers
		for (auto v : bl->getGeysers())
		{
			TilePosition q = v->getInitialTilePosition();
			Broodwar->drawBox(CoordinateType::Map, q.x * 32, q.y * 32, q.x * 32 + 4 * 32, q.y * 32 + 2 * 32, Colors::Orange, false);
		}
		//If this is an island expansion, draw a yellow circle around the base location
		if (bl->isIsland())
		{
			Broodwar->drawCircle(CoordinateType::Map, c.x, c.y, 80, Colors::Yellow, false);
		}
	}
	//Iterate through all the regions and draw the polygon outline of it in green.
	for (auto r : BWTA::getRegions())
	{
		BWTA::Polygon p = r->getPolygon();
		for (int j = 0; j<(int)p.size(); j++)
		{
			Position point1 = p[j];
			Position point2 = p[(j + 1) % p.size()];
			Broodwar->drawLine(CoordinateType::Map, point1.x, point1.y, point2.x, point2.y, Colors::Green);
		}
	}
	//Visualize the chokepoints with red lines
	for (auto r : BWTA::getRegions())
	{
		for (auto c : r->getChokepoints())
		{
			Position point1 = c->getSides().first;
			Position point2 = c->getSides().second;
			Broodwar->drawLine(CoordinateType::Map, point1.x, point1.y, point2.x, point2.y, Colors::Red);
		}
	}
}

//Show player information.
//No need to change this.
void ExampleAIModule::showPlayers()
{
	for (auto p : Broodwar->getPlayers())
	{
		Broodwar->printf("Player [%d]: %s is in force: %s", p->getID(), p->getName().c_str(), p->getForce()->getName().c_str());
	}
}

//Show forces information.
//No need to change this.
void ExampleAIModule::showForces()
{
	for (auto f : Broodwar->getForces())
	{
		BWAPI::Playerset players = f->getPlayers();
		Broodwar->printf("Force %s has the following players:", f->getName().c_str());
		for (auto p : players)
		{
			Broodwar->printf("  - Player [%d]: %s", p->getID(), p->getName().c_str());
		}
	}
}

void ExampleAIModule::updateBuildingTimer()
{
	for (int i = 0; i < m_buildingTimerList.size(); i++)
	{
		if (m_buildingTimerList[i].m_frame > m_buildingTimerList[i].m_marginFrames)
		{

			/// 1 avalokera
			m_allocatedMinerals -= m_buildingTimerList[i].m_type.mineralPrice();
			m_allocatedGas -= m_buildingTimerList[i].m_type.gasPrice();
			m_allocatedMinerals = std::max(m_allocatedMinerals, 0);
			m_allocatedGas = std::max(m_allocatedGas, 0);

			/// 2 rerequest the task
			Stage& currentStage = m_stageStorage.getStages()->operator[](m_currentStageIndex);
			for (int j = 0; j < currentStage.size(); j++)
			{
				if (currentStage[j].type == m_buildingTimerList[i].m_type)
				{
					currentStage[j].nBeingConstructed--;
					currentStage[j].nBeingConstructed = std::max(currentStage[j].nBeingConstructed, 0);
				}
			}

			/// 3 reset worker
			if (Broodwar->getUnit(m_buildingTimerList[i].m_workerID))
			{
				Broodwar->getUnit(m_buildingTimerList[i].m_workerID)->stop();
			}
			



			//Broodwar->setLocalSpeed(10);

			failures++;
			Broodwar->printf("||||||||||||||||||||||| Failed to build a %s ||||||||||||||||||||||||", m_buildingTimerList[i].m_type.getName().c_str());
			m_buildingTimerList.erase(std::remove(m_buildingTimerList.begin(), m_buildingTimerList.end(), m_buildingTimerList[i]), m_buildingTimerList.end());
		}
	}
	
}
///																																	
void ExampleAIModule::manageWorker(BWAPI::Unit& u)
{
	// Ignore the unit if it no longer exists
	// Make sure to include this block when handling any Unit pointer!
	if (!u->exists())
		return;

	// Ignore the unit if it has one of the following status ailments
	if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
		return;

	// Ignore the unit if it is in one of the following states
	if (u->isLoaded() || !u->isPowered() || u->isStuck())
		return;

	// Ignore the unit if it is incomplete or busy constructing
	if (!u->isCompleted())
		return;

	

	for (int i = 0; i < m_constructingWorkers.size(); i++)
	{
		if (m_constructingWorkers[i].workerID == u->getID())
		{
			if (!u->isConstructing())
			{
				/*bool canBuild = u->build(
					m_constructingWorkers[i].buildingType,
					Broodwar->getBuildLocation(
						m_constructingWorkers[i].buildingType,
						u->getTilePosition()
					)
				);*/

				bool canBuild = u->build(
					m_constructingWorkers[i].buildingType,
					getBuildLocation(
						m_constructingWorkers[i].buildingType,
						(m_flagStartBuildOfWorkerPos ? u->getTilePosition() : Broodwar->self()->getStartLocation())
					)
				);

				if (canBuild)
				{
					std::string str = m_constructingWorkers[i].buildingType.getName();
					Broodwar->printf("Worker %d is searching for a new location for a %s", u->getID(), str.c_str());
				}
			}
			break;
		}
	}

	if (u->isConstructing())
		return;


	
	if (Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Refinery) > 0)
	{
		if (u->getClosestUnit(BWAPI::Filter::IsRefinery)->getResources() > 0)
		{
			if (u->isGatheringGas())
			{
				m_nrOfGasWorkers++;
				if (m_nrOfGasWorkers > 3 * Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Refinery))
				{
					u->stop();
					m_nrOfGasWorkers--;
				}
			}
			else
			{
				if (m_nrOfGasWorkers < 3)
				{
					u->gather(u->getClosestUnit(BWAPI::Filter::IsRefinery));
					m_nrOfGasWorkers++;
					return;
				}
			}
		}
		else if (u->getPosition().getApproxDistance(u->getClosestUnit(BWAPI::Filter::IsRefinery)->getPosition()) < 300)
		{
			if (u->isGatheringGas())
			{
				u->stop();
				m_nrOfGasWorkers--;
			}
		}
	}



	

	Stage& stage = m_stageStorage.getStages()->operator[](m_currentStageIndex);

	int nMinerals = Broodwar->self()->minerals();
	int nGas = Broodwar->self()->gas();

	int n = stage.size();
	for (int i = 0; i < n; i++)
	{
		if (Broodwar->self()->allUnitCount(stage[i].type) + stage[i].nBeingConstructed < stage[i].n)
		{
			if (u->canBuild(stage[i].type))
			{
				if (nMinerals - m_allocatedMinerals >= stage[i].nReqMineral && nGas - m_allocatedGas >= stage[i].nReqGas)
				{
					int mineralPrice = stage[i].type.mineralPrice();
					int gasPrice = stage[i].type.gasPrice();

					if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
					{
						BWAPI::TilePosition tilePos;
						if (stage[i].suggestedPosition == BWAPI::Position(-1, -1))
						{
							tilePos = (m_flagStartBuildOfWorkerPos ? u->getTilePosition() : Broodwar->self()->getStartLocation());
							tilePos = Broodwar->getBuildLocation(stage[i].type, tilePos, rand() % (64 - 32) + 32);

							//BWAPI::TilePosition tmpTilePos = u->getTilePosition();
							/*tmpTilePos.x += rand() % 5 - 2;
							tmpTilePos.y += rand() % 5 - 2;*/
						}
						else
						{
							tilePos = positionToTile(stage[i].suggestedPosition);
						}

						bool canBuild = u->build(stage[i].type, tilePos);

						if (canBuild)
						{
							std::string str = stage[i].type.getName();
							//Broodwar->printf("Worker %d began building a %s", u->getID(), str.c_str());
							m_allocatedMinerals += mineralPrice;
							m_allocatedGas += gasPrice;
							stage[i].nBeingConstructed++;
							
							///							

							m_buildingTimerList.push_back(BuildingTimer(u->getID(), stage[i].type));


							///							


							m_constructingWorkers.push_back({ stage[i].type, u->getID() });

							return;
						}
					}
				}
			}
		}
	} // for

	if (u->isIdle())
	{
		u->gather(u->getClosestUnit(BWAPI::Filter::IsMineralField));
	}

}
void ExampleAIModule::manageCommandCenter(BWAPI::Unit& u)
{
	int nMinerals = Broodwar->self()->minerals();
	int scvMineralPrice = BWAPI::UnitType(BWAPI::UnitTypes::Enum::Terran_SCV).mineralPrice();
	if (u->isIdle())
	{
		if (u->isFlying())
		{
			for (int i = 0; i < m_miningSites.size(); i++)
			{
				if (m_miningSites[i].commandCenter == nullptr)
				{
					m_miningSites[i].commandCenter = u;
					break;
				}
				else if (m_miningSites[i].commandCenter->getID() == u->getID())
				{
					if (u->getPosition() == m_miningSites[i].position)
					{
						u->land(positionToTile(m_miningSites[i].position));
					}
					else
					{
						u->move(m_miningSites[i].position);
					}
					break;
				}
			}
		}
		else
		{
			if (Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Enum::Terran_SCV) <= (20 + 3))
			{
				if (nMinerals - m_allocatedMinerals >= scvMineralPrice)
				{
					if (u->train(BWAPI::UnitTypes::Terran_SCV))
					{
						//Broodwar->printf("Command center %d began training a worker", u->getID());
						m_allocatedMinerals += scvMineralPrice;
					}
				}
			}
		}
	}
}
void ExampleAIModule::manageMarine(BWAPI::Unit& u)
{
	if (u->isIdle())
	{
		u->attack(m_armyRallypoints[m_furthestArmyRallyPoint]);
	}
	if (u->isAttacking() && u->getHitPoints() > 30)
	{
		if (u->canUseTechWithoutTarget(BWAPI::TechTypes::Stim_Packs))
		{
			u->useTech(BWAPI::TechTypes::Stim_Packs);
		}
	}
}
void ExampleAIModule::manageFirebat(BWAPI::Unit& u)
{
	if (u->isIdle())
	{
		u->attack(m_armyRallypoints[m_furthestArmyRallyPoint]);
	}
	if (u->isAttacking() && u->getHitPoints() > 40)
	{
		if (u->canUseTechWithoutTarget(BWAPI::TechTypes::Stim_Packs))
		{
			u->useTech(BWAPI::TechTypes::Stim_Packs);
		}
	}
}
void ExampleAIModule::manageMedic(BWAPI::Unit& u)
{
	if (u->isIdle())
	{
		u->move(m_armyRallypoints[m_furthestArmyRallyPoint]);
	}
}
void ExampleAIModule::manageTank(BWAPI::Unit& u)
{
	if (u->isIdle())
	{
		if (m_troopState == HOLD)
		{
			if (u->getPosition().getApproxDistance(m_armyRallypoints[m_furthestArmyRallyPoint]) < 150)
			{
				if (u->canSiege())
				{
					u->siege();
				}
			}
			else
			{
				u->attack(m_armyRallypoints[m_furthestArmyRallyPoint]);
			}
			//u->canSiege();
			
		}
		else if (m_troopState == ADVANCE || m_troopState == RETREAT)
		{
			if (u->canUnsiege())
			{
				u->unsiege();
			}
			else
			{
				u->attack(m_armyRallypoints[m_furthestArmyRallyPoint]);
			}
			
		}
	}
}
void ExampleAIModule::manageWraith(BWAPI::Unit & u)
{
	if (u->isIdle())
	{
		u->attack(m_armyRallypoints[m_furthestArmyRallyPoint]);
	}
}
void ExampleAIModule::manageBarrack(BWAPI::Unit& u)
{
	if (u->isIdle())
	{
		Stage& stage = m_stageStorage.getStages()->operator[](m_currentStageIndex);

		int nMinerals = Broodwar->self()->minerals();
		int nGas = Broodwar->self()->gas();

		int n = stage.size();
		for (int i = 0; i < n; i++)
		{
			if (Broodwar->self()->allUnitCount(stage[i].type) + stage[i].nBeingConstructed < stage[i].n)
			{
				if (u->canTrain(stage[i].type))
				{
					//Broodwar->printf("Barracks:::::::::::::: %s", stage[i].type.c_str());
					if (nMinerals - m_allocatedMinerals >= stage[i].nReqMineral && nGas - m_allocatedGas >= stage[i].nReqGas)
					{
						int mineralPrice = BWAPI::UnitType(stage[i].type).mineralPrice();
						int gasPrice = BWAPI::UnitType(stage[i].type).gasPrice();

						if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
						{
							bool canTrain = u->train(stage[i].type);

							if (canTrain)
							{
								std::string str = stage[i].type.getName();
								//Broodwar->printf("Barracks %d began training a %s", u->getID(), str.c_str());
								m_allocatedMinerals += mineralPrice;
								m_allocatedGas += gasPrice;
								stage[i].nBeingConstructed++;
								break;
							}
						}
					}
				}
			}
			
		} // for

		if (m_currentStageIndex > 5 && u->getPosition().getApproxDistance(tileToPosition(Broodwar->self()->getStartLocation())) < 900)
		{
			u->lift();
		}
		if (u->isFlying())
		{
			BWAPI::Position tmpPos = m_miningSites[2].position;

			tmpPos.x += rand() % 15 * 32 - 10 * 32;
			tmpPos.y += rand() % 15 * 32 - 10 * 32;

			u->land(positionToTile(tmpPos));
		}
	}
}
void ExampleAIModule::manageFactory(BWAPI::Unit& u)
{
	int nMinerals = Broodwar->self()->minerals();
	int nGas = Broodwar->self()->gas();

	if (u->isIdle())
	{
		BWAPI::TilePosition pos = u->getTilePosition();

		pos.x += (rand() % 14) - 7;
		pos.y += (rand() % 14) - 7;

		if (u->getAddon() == nullptr)
		{
			//Broodwar->printf("FACTORY-------------------");
			if (u->canBuild(BWAPI::UnitTypes::Terran_Machine_Shop, pos))
			{
				int mineralPrice = BWAPI::UnitType(BWAPI::UnitTypes::Terran_Machine_Shop).mineralPrice();
				int gasPrice = BWAPI::UnitType(BWAPI::UnitTypes::Terran_Machine_Shop).gasPrice();

				if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
				{
					bool canBuild = u->build(BWAPI::UnitTypes::Terran_Machine_Shop, pos);
					if (canBuild)
					{
						for (int i = 0; i < m_upgradingFactories.size(); i++)
						{
							if (m_upgradingFactories[i] == u->getID())
							{
								return;
							}
						}

						m_allocatedMinerals += BWAPI::UnitType(BWAPI::UnitTypes::Terran_Machine_Shop).mineralPrice();
						m_allocatedGas += BWAPI::UnitType(BWAPI::UnitTypes::Terran_Machine_Shop).gasPrice();

						m_upgradingFactories.push_back(u->getID());
					}
				}
			}
		}
		///		

		Stage& stage = m_stageStorage.getStages()->operator[](m_currentStageIndex);

		int nMinerals = Broodwar->self()->minerals();
		int nGas = Broodwar->self()->gas();

		int n = stage.size();
		for (int i = 0; i < n; i++)
		{
			if (Broodwar->self()->allUnitCount(stage[i].type) + stage[i].nBeingConstructed < stage[i].n)
			{
				if (u->canTrain(stage[i].type))
				{
					if (nMinerals - m_allocatedMinerals >= stage[i].nReqMineral)
					{
						int mineralPrice = BWAPI::UnitType(stage[i].type).mineralPrice();

						if (nMinerals - m_allocatedMinerals >= mineralPrice)
						{
							bool canTrain = u->train(stage[i].type);

							if (canTrain)
							{
								std::string str = stage[i].type.getName();
								//Broodwar->printf("Factory %d began training a %s", u->getID(), str.c_str());
								m_allocatedMinerals += mineralPrice;
								stage[i].nBeingConstructed++;
								break;
							}
						}
					}
				}
			} // if

		} // for

		///		
	}
}
void ExampleAIModule::manageStarport(BWAPI::Unit & u)
{
	if (u->isIdle())
	{
		Stage& stage = m_stageStorage.getStages()->operator[](m_currentStageIndex);

		int nMinerals = Broodwar->self()->minerals();
		int nGas = Broodwar->self()->gas();

		int n = stage.size();
		for (int i = 0; i < n; i++)
		{
			if (Broodwar->self()->allUnitCount(stage[i].type) + stage[i].nBeingConstructed < stage[i].n)
			{
				if (u->canTrain(stage[i].type))
				{
					//Broodwar->printf("Barracks:::::::::::::: %s", stage[i].type.c_str());
					if (nMinerals - m_allocatedMinerals >= stage[i].nReqMineral && nGas - m_allocatedGas >= stage[i].nReqGas)
					{
						int mineralPrice = BWAPI::UnitType(stage[i].type).mineralPrice();
						int gasPrice = BWAPI::UnitType(stage[i].type).gasPrice();

						if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
						{
							bool canTrain = u->train(stage[i].type);

							if (canTrain)
							{
								std::string str = stage[i].type.getName();
								//Broodwar->printf("Starport %d began training a %s", u->getID(), str.c_str());
								m_allocatedMinerals += mineralPrice;
								m_allocatedGas += gasPrice;
								stage[i].nBeingConstructed++;
								break;
							}
						}
					}
				}
			}

		}
	}
}
void ExampleAIModule::manageMachineShop(BWAPI::Unit& u)
{
	if (m_currentStageIndex >= 5)
	{
		int nMinerals = Broodwar->self()->minerals();
		int nGas = Broodwar->self()->minerals();

		if (u->canResearch(BWAPI::TechTypes::Tank_Siege_Mode))
		{
			int mineralPrice = BWAPI::TechTypes::Tank_Siege_Mode.mineralPrice();
			int gasPrice = BWAPI::TechTypes::Tank_Siege_Mode.gasPrice();

			if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
			{
				m_allocatedMinerals += mineralPrice;
				u->research(BWAPI::TechTypes::Tank_Siege_Mode);
			}
		}
	}
}
void ExampleAIModule::manageAcademy(BWAPI::Unit& u)
{
	int nMinerals = Broodwar->self()->minerals();
	int nGas = Broodwar->self()->minerals();

	
	if (u->canUpgrade(BWAPI::UpgradeTypes::U_238_Shells))
	{
		int mineralPrice = BWAPI::UpgradeTypes::U_238_Shells.mineralPrice();
		int gasPrice = BWAPI::UpgradeTypes::U_238_Shells.gasPrice();

		if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
		{
			m_allocatedMinerals += mineralPrice;
			u->upgrade(BWAPI::UpgradeTypes::U_238_Shells);
		}
	}

	if (m_currentStageIndex > 4)
	{
		if (u->canResearch(BWAPI::TechTypes::Stim_Packs))
		{
			int mineralPrice = BWAPI::TechTypes::Stim_Packs.mineralPrice();
			int gasPrice = BWAPI::TechTypes::Stim_Packs.gasPrice();

			if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
			{
				m_allocatedMinerals += mineralPrice;
				u->research(BWAPI::TechTypes::Stim_Packs);
			}
		}
	}

	
}
void ExampleAIModule::manageEngineeringBay(BWAPI::Unit& u)
{
	if (Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
	{
		if (m_currentStageIndex >= 4)
		{
			int nMinerals = Broodwar->self()->minerals();
			int nGas = Broodwar->self()->minerals();

			if (u->canUpgrade(BWAPI::UpgradeTypes::Terran_Infantry_Weapons))
			{
				int mineralPrice = BWAPI::UpgradeTypes::Terran_Infantry_Weapons.mineralPrice();
				int gasPrice = BWAPI::UpgradeTypes::Terran_Infantry_Weapons.gasPrice();

				if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
				{
					m_allocatedMinerals += mineralPrice;
					u->upgrade(BWAPI::UpgradeTypes::Terran_Infantry_Weapons);
				}
			}

			if (u->canUpgrade(BWAPI::UpgradeTypes::Terran_Infantry_Armor))
			{
				int mineralPrice = BWAPI::UpgradeTypes::Terran_Infantry_Armor.mineralPrice();
				int gasPrice = BWAPI::UpgradeTypes::Terran_Infantry_Armor.gasPrice();

				if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
				{
					m_allocatedMinerals += mineralPrice;
					u->upgrade(BWAPI::UpgradeTypes::Terran_Infantry_Armor);
				}
			}
		}
	}
	else
	{
		if (m_currentStageIndex >= 5)
		{
			int nMinerals = Broodwar->self()->minerals();
			int nGas = Broodwar->self()->minerals();

			if (u->canUpgrade(BWAPI::UpgradeTypes::Terran_Infantry_Weapons))
			{
				int mineralPrice = BWAPI::UpgradeTypes::Terran_Infantry_Weapons.mineralPrice();
				int gasPrice = BWAPI::UpgradeTypes::Terran_Infantry_Weapons.gasPrice();

				if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
				{
					m_allocatedMinerals += mineralPrice;
					u->upgrade(BWAPI::UpgradeTypes::Terran_Infantry_Weapons);
				}
			}

			if (u->canUpgrade(BWAPI::UpgradeTypes::Terran_Infantry_Armor))
			{
				int mineralPrice = BWAPI::UpgradeTypes::Terran_Infantry_Armor.mineralPrice();
				int gasPrice = BWAPI::UpgradeTypes::Terran_Infantry_Armor.gasPrice();

				if (nMinerals - m_allocatedMinerals >= mineralPrice && nGas - m_allocatedGas >= gasPrice)
				{
					m_allocatedMinerals += mineralPrice;
					u->upgrade(BWAPI::UpgradeTypes::Terran_Infantry_Armor);
				}
			}
		}
	}
	
	
}

bool ExampleAIModule::isBuildableByWorker(BWAPI::UnitType type)
{
	bool isTrue = false;
	if (type == BWAPI::UnitTypes::Terran_Academy ||
		type == BWAPI::UnitTypes::Terran_Armory ||
		type == BWAPI::UnitTypes::Terran_Barracks ||
		type == BWAPI::UnitTypes::Terran_Bunker ||
		type == BWAPI::UnitTypes::Terran_Command_Center ||
		type == BWAPI::UnitTypes::Terran_Comsat_Station ||
		type == BWAPI::UnitTypes::Terran_Control_Tower ||
		type == BWAPI::UnitTypes::Terran_Engineering_Bay ||
		type == BWAPI::UnitTypes::Terran_Factory ||
		type == BWAPI::UnitTypes::Terran_Machine_Shop ||
		type == BWAPI::UnitTypes::Terran_Missile_Turret ||
		type == BWAPI::UnitTypes::Terran_Nuclear_Silo ||
		type == BWAPI::UnitTypes::Terran_Physics_Lab ||
		type == BWAPI::UnitTypes::Terran_Refinery ||
		type == BWAPI::UnitTypes::Terran_Science_Facility ||
		type == BWAPI::UnitTypes::Terran_Supply_Depot
		)
	{
		isTrue = true;
	}

	return isTrue;
}
bool ExampleAIModule::isTrainableByBarracks(BWAPI::UnitType type)
{
	bool isTrue = false;
	if (type == BWAPI::UnitTypes::Terran_Civilian ||
		type == BWAPI::UnitTypes::Terran_Ghost ||
		type == BWAPI::UnitTypes::Terran_Goliath ||
		type == BWAPI::UnitTypes::Terran_Marine ||
		type == BWAPI::UnitTypes::Terran_Medic
		)
	{
		isTrue = true;
	}

	return isTrue;
}

void ExampleAIModule::manageTroops(BWAPI::Unitset units)
{
	/// TROOP MANAGER ///
	int enemyBaseStart = 2;
	int enemyBaseEnd = 5;

	if (m_troopState == RETREAT)
	{
		if (frames - m_begunRetreatFrame < 5000)
		{
			BWAPI::Unitset unitsOnRallyPoint = Broodwar->getUnitsInRadius(m_armyRallypoints[m_furthestArmyRallyPoint], 8 * 32, BWAPI::Filter::IsAlly);
			if (unitsOnRallyPoint.size() >= 8)
			{
				Broodwar->printf("HOLD");
				m_troopState = HOLD;
			}
		}
		else
		{
			Broodwar->printf("RETREAT FURTHER");

			m_furthestArmyRallyPoint = std::max(m_furthestArmyRallyPoint - 1, 0);
			m_begunRetreatFrame = frames;
		}
	}
	else if (m_troopState == HOLD)
	{
		BWAPI::Unitset unitsOnRallyPoint = Broodwar->getUnitsInRadius(m_armyRallypoints[m_furthestArmyRallyPoint], 8 * 32, BWAPI::Filter::IsAlly);
		if (unitsOnRallyPoint.size() >= 30)
		{
			if (m_furthestArmyRallyPoint < m_armyRallypoints.size() - 1)
			{
				Broodwar->printf("MOVE OUT");
				m_furthestArmyRallyPoint++;

				m_begunAdvanceFrame = frames;
				m_troopState = ADVANCE;
			}
			else if (m_furthestArmyRallyPoint == m_armyRallypoints.size() - 1)
			{
				m_furthestArmyRallyPoint = enemyBaseStart;
			}
		}
		else if (unitsOnRallyPoint.size() < 4)
		{
			if (m_furthestArmyRallyPoint > 0)
			{
				Broodwar->printf("RETREAT");
				m_troopState = RETREAT;

				if (m_furthestArmyRallyPoint >= enemyBaseStart && m_furthestArmyRallyPoint <= enemyBaseEnd)
				{
					m_furthestArmyRallyPoint = 1;
				}
				else
				{
					m_furthestArmyRallyPoint = std::max(m_furthestArmyRallyPoint - 1, 0);
				}

				for (auto u : units)
				{
					if (u->getType() == BWAPI::UnitTypes::Enum::Terran_Marine ||
						u->getType() == BWAPI::UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode ||
						u->getType() == BWAPI::UnitTypes::Enum::Terran_Medic ||
						u->getType() == BWAPI::UnitTypes::Enum::Terran_Firebat ||
						u->getType() == BWAPI::UnitTypes::Enum::Terran_Wraith)
					{
						u->stop();
					}
				}
				m_begunRetreatFrame = frames;
			}
		}
	}
	else if (m_troopState == ADVANCE)
	{
		if (frames - m_begunAdvanceFrame < 2000)
		{
			BWAPI::Unitset unitsOnRallyPoint = Broodwar->getUnitsInRadius(m_armyRallypoints[m_furthestArmyRallyPoint], 8 * 32, BWAPI::Filter::IsAlly);
			if (unitsOnRallyPoint.size() >= 20)
			{
				Broodwar->printf("HOLD");
				m_troopState = HOLD;
			}
		}
		else
		{
			if (m_furthestArmyRallyPoint > 0)
			{
				if (m_furthestArmyRallyPoint <= enemyBaseEnd)
				{
					Broodwar->printf("RETREAT");
					m_troopState = RETREAT;

					if (m_furthestArmyRallyPoint >= enemyBaseStart && m_furthestArmyRallyPoint <= enemyBaseEnd)
					{
						m_furthestArmyRallyPoint = 1;
					}
					else
					{
						m_furthestArmyRallyPoint = std::max(m_furthestArmyRallyPoint - 1, 0);
					}

					for (auto u : units)
					{
						if (u->getType() == BWAPI::UnitTypes::Enum::Terran_Marine ||
							u->getType() == BWAPI::UnitTypes::Enum::Terran_Siege_Tank_Tank_Mode)
						{
							u->stop();
						}
					}
					m_begunRetreatFrame = frames;
				}
				else
				{
					m_begunAdvanceFrame = frames;
				}
			}
		}
	}
}

BWAPI::TilePosition ExampleAIModule::getBuildLocation(BWAPI::UnitType type, BWAPI::TilePosition tilePos)
{
	BWAPI::TilePosition tile = Broodwar->getBuildLocation(type, tilePos);

	for (int i = -3; i < 3; i += 2)
	{
		for (int j = -3; j < 3; j += 2)
		{
			if (Broodwar->canBuildHere(BWAPI::TilePosition(tile.x + i, tile.y + j), type))
			{
				tile.x += i;
				tile.y += j;
				return tile;
			}
		}
	}
	return tile;
}

BWAPI::TilePosition ExampleAIModule::positionToTile(BWAPI::Position position)
{
	BWAPI::TilePosition pos;
	pos.x = position.x / 32;
	pos.y = position.y / 32;
	return pos; 
}
BWAPI::Position ExampleAIModule::tileToPosition(BWAPI::TilePosition tilePosition)
{
	BWAPI::Position pos;
	pos.x = tilePosition.x * 32;
	pos.y = tilePosition.y * 32;
	return pos;
}