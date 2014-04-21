#ifndef MOD_MARKER_H
#define MOD_MARKER_H
#ifdef _WIN32
#pragma once
#endif

//#include "baseentity.h"
#include "asw_marker.h"


// This class is used for marking world positions of objectives


class CMOD_Marker : public CASW_Marker
{
public:
	DECLARE_CLASS( CMOD_Marker, CASW_Marker );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	virtual void SetComplete( bool bComplete );

	virtual bool IsObjectiveComplete() { return m_bComplete; }

	void InputSetComplete( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

private:

	CNetworkString( m_ObjectiveName, 256 );
	CNetworkVar( int, m_nMapWidth );
	CNetworkVar( int, m_nMapHeight );
	CNetworkVar( bool, m_bComplete );
	CNetworkVar( bool, m_bEnabled );
	bool m_bStartDisabled;

};

#endif /* MOD_MARKER_H */