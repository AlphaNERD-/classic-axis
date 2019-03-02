#pragma once

#include "CPathFind.h"

class CAutoPilot {
public:
	CNodeAddress         m_currentAddress;
	CNodeAddress         m_startingRouteNode;
	CNodeAddress field_8;
	int field_C;
	unsigned int         m_nSpeedScaleFactor;
	CCarPathLinkAddress  m_nCurrentPathNodeInfo;
	CCarPathLinkAddress  m_nNextPathNodeInfo;
	CCarPathLinkAddress  m_nPreviousPathNodeInfo;
	char field_1A[2];
	int         field_1C;
	unsigned int         m_dwTimeToStartMission;
	unsigned int m_dwTimeSwitchedToRealPhysics;
private:
	char pad_0;
public:
	unsigned char _smthcurr;
	unsigned char _smthNext;
	char m_nCuurentLane;
	char m_nNextLane;
	char m_nDrivingStyle;
	char m_nCarMission;
	char m_nTempAction;
	unsigned int m_dwTempActionTime;
	float m_fMaxTrafficSpeed;
	unsigned char m_nCruiseSpeed;
	char unknown[11];
	CVector m_vecDestinationCoors;
	CNodeAddress m_aPathFindNodesInfo[8];
	short m_wPathFindNodesCount;
private:
	char _pad[2];
};

VALIDATE_SIZE(CAutoPilot, 0x74);