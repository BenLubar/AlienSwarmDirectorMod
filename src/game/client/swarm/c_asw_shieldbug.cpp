#include "cbase.h"
#include "C_ASW_Shieldbug.h"
#include "engine/IVDebugOverlay.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar asw_shieldbug_death_force( "asw_shieldbug_death_force", "65000" , 0, "this sets the custom death force for the exploding shieldbug");

IMPLEMENT_CLIENTCLASS_DT(C_ASW_Shieldbug, DT_ASW_Shieldbug, CASW_Shieldbug)
	
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_ASW_Shieldbug )

END_PREDICTION_DATA()

C_ASW_Shieldbug::C_ASW_Shieldbug()
{
}

C_ASW_Shieldbug::~C_ASW_Shieldbug()
{
}

Vector C_ASW_Shieldbug::GetCustomDeathForce()
{
	Vector deathForce;
	deathForce.z = asw_shieldbug_death_force.GetInt();
	return deathForce;
}

// plays alien type specific footstep sound
void C_ASW_Shieldbug::DoAlienFootstep(Vector &vecOrigin, float fvol)
{
	CSoundParameters params;
	if ( !CBaseEntity::GetParametersForSound( "ASW_ShieldBug.StepLight", params, NULL ) )
		return;

	CLocalPlayerFilter filter;

	// do the alienfleshy foot sound
	EmitSound_t ep2;
	ep2.m_nChannel = CHAN_AUTO;
	ep2.m_pSoundName = params.soundname;
	ep2.m_flVolume = fvol * 0.2f;
	ep2.m_SoundLevel = params.soundlevel;
	ep2.m_nFlags = 0;
	ep2.m_nPitch = params.pitch;
	ep2.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep2 );
}

const Vector& C_ASW_Shieldbug::GetAimTargetRadiusPos(const Vector &vecFiringSrc)
{
	// We can't get the hitboxes on the client, but it just so happens that the eyes attachment and the attach_brain attachment are in the right place for us to do some sneaky math.
	static Vector aim_pos;
	Vector vecEyes, vecBrain;
	if (!GetAttachment("eyes", vecEyes))
	{
		Assert(0);
		return GetAbsOrigin();
	}
	if (!GetAttachment("attach_brain", vecBrain))
	{
		Assert(0);
		return GetAbsOrigin();
	}
	aim_pos = vecBrain * 2 - vecEyes;
	return aim_pos;
}


const Vector& C_ASW_Shieldbug::GetAimTargetPos(const Vector &vecFiringSrc, bool bWeaponPrefersFlatAiming)
{
	return GetAimTargetRadiusPos(vecFiringSrc);
}
