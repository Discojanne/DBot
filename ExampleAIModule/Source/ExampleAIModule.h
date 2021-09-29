#pragma once
#include <BWAPI.h>

#include <BWTA.h>
#include <windows.h>

#include "StageStorage.h"

#include "BuildingTimer.h"

extern bool analyzed;
extern bool analysis_just_finished;
extern BWTA::Region* home;
extern BWTA::Region* enemy_base;
DWORD WINAPI AnalyzeThread();

using namespace BWAPI;
using namespace BWTA;

class ExampleAIModule : public BWAPI::AIModule
{
public:
	//Methods inherited from BWAPI:AIModule
	virtual void onStart();
	virtual void onEnd(bool isWinner);
	virtual void onFrame();
	virtual void onSendText(std::string text);
	virtual void onReceiveText(BWAPI::Player player, std::string text);
	virtual void onPlayerLeft(BWAPI::Player player);
	virtual void onNukeDetect(BWAPI::Position target);
	virtual void onUnitDiscover(BWAPI::Unit unit);
	virtual void onUnitEvade(BWAPI::Unit unit);
	virtual void onUnitShow(BWAPI::Unit unit);
	virtual void onUnitHide(BWAPI::Unit unit);
	virtual void onUnitCreate(BWAPI::Unit unit);
	virtual void onUnitDestroy(BWAPI::Unit unit);
	virtual void onUnitMorph(BWAPI::Unit unit);
	virtual void onUnitRenegade(BWAPI::Unit unit);
	virtual void onSaveGame(std::string gameName);
	virtual void onUnitComplete(BWAPI::Unit unit);

	//Own methods
	void drawStats();
	void drawTerrainData();
	void showPlayers();
	void showForces();
	Position findGuardPoint();

	/*
	Functions added by student
	*/
	void updateBuildingTimer();
	void manageWorker(BWAPI::Unit& u);
	void manageCommandCenter(BWAPI::Unit& u);
	void manageMarine(BWAPI::Unit& u);
	void manageFirebat(BWAPI::Unit& u);
	void manageMedic(BWAPI::Unit& u);
	void manageTank(BWAPI::Unit& u);
	void manageWraith(BWAPI::Unit& u);
	void manageBarrack(BWAPI::Unit& u);
	void manageFactory(BWAPI::Unit& u);
	void manageStarport(BWAPI::Unit& u);
	void manageMachineShop(BWAPI::Unit& u);
	void manageAcademy(BWAPI::Unit& u);
	void manageEngineeringBay(BWAPI::Unit& u);
	bool isBuildableByWorker(BWAPI::UnitType type);
	bool isTrainableByBarracks(BWAPI::UnitType type);
	void manageTroops(BWAPI::Unitset units);

	BWAPI::TilePosition getBuildLocation(BWAPI::UnitType type, BWAPI::TilePosition tilePos);

	BWAPI::TilePosition positionToTile(BWAPI::Position position);
	BWAPI::Position tileToPosition(BWAPI::TilePosition tilePosition);
	
private:
	/*
	Variables added by student
	*/

	struct ConstructingWorker
	{
		BWAPI::UnitType buildingType;
		int workerID;

		bool operator==(const ConstructingWorker& right)
		{
			return buildingType == right.buildingType && workerID == right.workerID;
		}
	};

	struct MiningSite
	{
		MiningSite(BWAPI::Position _position, BWAPI::Unit _commandCenter = nullptr) : position(_position), commandCenter(_commandCenter), refinery(nullptr), isBuildingRefinery(false)
		{
		}
		BWAPI::Position position;
		BWAPI::Unit commandCenter;
		BWAPI::Unit refinery;
		bool isBuildingRefinery;
	};

	StageStorage m_stageStorage;
	int m_currentStageIndex;
	int m_allocatedMinerals;
	int m_allocatedGas;

	bool m_startAtTopLeft;

	int m_furthestArmyRallyPoint;
	std::vector<BWAPI::Position> m_armyRallypoints;
	std::vector<MiningSite> m_miningSites;

	int m_nrOfGasWorkers;
	std::vector<ConstructingWorker> m_constructingWorkers;
	std::vector<int> m_upgradingFactories;


	///					
	int frames = 0;
	int failures = 0;

	std::vector<BuildingTimer> m_buildingTimerList;


	// progress bools
	bool m_flagExpand = false;


	enum TroopState
	{
		RETREAT, 
		HOLD,
		ADVANCE
	};

	enum BaseState
	{
		SAFE,
		ATTACKED_BY_ZERG
	};


	TroopState m_troopState = HOLD;
	BaseState m_baseState = SAFE;
	int m_begunRetreatFrame;
	int m_begunAdvanceFrame;


	std::vector<BWAPI::Position> m_buildingPositions;

	//bool m_flagFirstMoveTroops = false;
	bool m_flagStrategyChoice = false;
	bool m_flagStartBuildOfWorkerPos = false;
	
	int m_stageAtWhereToBuildFromWorkerPosition = 7;
	int m_NormalSpeedAfterStage = 9;

	///					
};
