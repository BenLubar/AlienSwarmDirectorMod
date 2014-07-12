//====== Copyright (c) 1996-2009, Valve Corporation, All rights reserved. =====
//
// Purpose: Alien Swarm Director Controller.
//
//=============================================================================
#ifndef ASW_DIRECTOR_CONTROL_H
#define ASW_DIRECTOR_CONTROL_H
#ifdef _WIN32
#pragma once
#endif

class CASW_Marine;

// Allows the level designer to send and recieve hints with the director

class CASW_Director_Control : public CLogicalEntity
{
public:

	DECLARE_CLASS( CASW_Director_Control, CLogicalEntity );
	DECLARE_DATADESC();

	CASW_Director_Control()
	{
		m_bWanderersStartEnabled = false;
		m_bHordesStartEnabled = false;
		m_bDirectorControlsSpawners = false;
		m_flPreSpawnAliens = 0.0f;
	}

	virtual void Precache();

	virtual void OnEscapeRoomStart( CASW_Marine *pMarine );			// marine has entered the escape room with all objectives complete

	bool m_bWanderersStartEnabled;
	bool m_bHordesStartEnabled;
	bool m_bDirectorControlsSpawners;
	float m_flPreSpawnAliens;

private:
	void InputEnableHordes( inputdata_t &inputdata );
	void InputDisableHordes( inputdata_t &inputdata );
	void InputEnableWanderers( inputdata_t &inputdata );
	void InputDisableWanderers( inputdata_t &inputdata );
	void InputStartFinale( inputdata_t &inputdata );

	COutputEvent m_OnEscapeRoomStart;
};

#endif // ASW_DIRECTOR_CONTROL_H