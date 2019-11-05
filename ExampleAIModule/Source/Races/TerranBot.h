#pragma once

#include <BWAPI.h>
#include <BWTA.h>

class TerranBot {
public:
	TerranBot(BWAPI::GameWrapper* Broodwar);
	~TerranBot();

	void manageWorker(BWAPI::Unit& u);
	void manageCommandCenter(BWAPI::Unit& u);

private:
	BWAPI::GameWrapper Broodwar;
};