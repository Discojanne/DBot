#include "StageStorage.h"

StageStorage::StageStorage()
{
}

StageStorage::~StageStorage()
{
}

StageVector* StageStorage::getStages()
{
	return &m_stages;
}