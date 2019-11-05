#include "TerranBot.h"
#include "..//include/BWAPI/Game.h"

TerranBot::TerranBot(BWAPI::GameWrapper* Broodwar) {
}

TerranBot::~TerranBot() {
}

void TerranBot::manageWorker(BWAPI::Unit& u) {

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

	if (u->isIdle()) {
		u->gather(u->getClosestUnit(BWAPI::Filter::IsMineralField));
	}
}

void TerranBot::manageCommandCenter(BWAPI::Unit& u) {
	
	int nMinerals = (*Broodwar)->self()->minerals();
	int scvMineralPrice = BWAPI::UnitType(BWAPI::UnitTypes::Enum::Terran_SCV).mineralPrice();

	if (u->isIdle()) {
		if (u->isFlying()) {

		}
		else {
			if ((*Broodwar)->self()->completedUnitCount(BWAPI::UnitTypes::Enum::Terran_SCV) <= (20 + 3)) {
				if (nMinerals >= scvMineralPrice) {
					if (u->train(BWAPI::UnitTypes::Terran_SCV)) {
						(*Broodwar)->printf("Command center %d began training a worker", u->getID());
					}
				}
			}
		}
	}

}
