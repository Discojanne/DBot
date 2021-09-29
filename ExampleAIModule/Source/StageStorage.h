#pragma once
#include <vector>
#include <BWAPI.h>

//struct StageGoal
//{
//	BWAPI::UnitType unitType;
//	int creatorUnit;
//	int nRequiredMinerals;
//	int nRequiredGas;
//};

struct StageGoal
{
	StageGoal(BWAPI::UnitType _type = BWAPI::UnitTypes::Enum::Terran_SCV, int _n = 1, int _nReqMineral = 0, int _nReqGas = 0, BWAPI::Position position = BWAPI::Position(-1, -1)) :
	type(_type), suggestedPosition(position), n(_n), nReqMineral(_nReqMineral), nReqGas(_nReqGas), nBeingConstructed(0)
	{
	}
	BWAPI::UnitType type;
	BWAPI::Position suggestedPosition;
	int n;
	int nReqMineral;
	int nReqGas;
	int nBeingConstructed;

};

typedef std::vector<StageGoal> Stage;
typedef std::vector<Stage> StageVector;

class StageStorage
{
public:
	StageStorage();
	~StageStorage();

	StageVector* getStages();

private:
	StageVector m_stages;
};
