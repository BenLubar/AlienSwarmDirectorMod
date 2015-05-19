#ifndef _INLCUDE_C_ASW_DRONE_ADVANCED_H
#define _INLCUDE_C_ASW_DRONE_ADVANCED_H

#include "c_asw_alien.h"
#include "interpolatedvar.h"

class C_ASW_Drone_Advanced : public C_ASW_Alien
{
public:
	DECLARE_CLASS( C_ASW_Drone_Advanced, C_ASW_Alien );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

					C_ASW_Drone_Advanced();
	virtual			~C_ASW_Drone_Advanced();

	Class_T		Classify( void ) { return (Class_T) CLASS_ASW_DRONE; }

	float GetRunSpeed();
	virtual void ClientThink();
	virtual void OnDataChanged( DataUpdateType_t updateType );
	void UpdatePoseParams();	
	virtual const Vector& GetAimTargetPos(const Vector &vecFiringSrc, bool bWeaponPrefersFlatAiming);
	virtual void GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM]);
	virtual void FireEvent(const Vector& origin, const QAngle& angles, int event, const char *options);

	CNetworkHandle( C_BaseEntity, m_hAimTarget );
	CNetworkVar( bool, m_bIsSummoner );

private:
	C_ASW_Drone_Advanced( const C_ASW_Drone_Advanced & ); // not defined, not accessible
	float m_flCurrentTravelYaw;
	float m_flCurrentTravelSpeed;
	float m_flClientPoseParameter[MAXSTUDIOPOSEPARAM];
	bool m_bWasJumping;
};

#endif /* _INLCUDE_C_ASW_DRONE_ADVANCED_H */