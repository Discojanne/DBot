#pragma once

#include <vector>
#include <BWAPI.h>



struct BuildingTimer
{
	BuildingTimer(int _workerID = -1, BWAPI::UnitType _type = BWAPI::UnitTypes::Enum::Terran_Supply_Depot, int _marginFrames = 500) :
		m_workerID(_workerID), m_type(_type), m_marginFrames(_marginFrames)
	{
	}
	void operator=(const BuildingTimer& original)
	{
		m_workerID = original.m_workerID;
		m_marginFrames = original.m_marginFrames;
		m_frame = original.m_frame;
		m_type = original.m_type;
	}
	bool operator==(const BuildingTimer& original)
	{
		if (m_workerID == original.m_workerID && m_marginFrames == original.m_marginFrames && m_frame == original.m_frame && m_type == original.m_type)
		{
			return true;
		}
		return false;
	}

	int m_workerID = -1;
	int m_marginFrames;
	int m_frame = 0;
	BWAPI::UnitType m_type; // What type or unit that is supposed to be constructed
};