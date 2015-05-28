#include "cbase.h"
#include "asw_weapon_ammo_bag_shared.h"
#include "in_buttons.h"
#include "ammodef.h"
#ifdef CLIENT_DLL
#include "c_asw_player.h"
#include "c_asw_weapon.h"
#include "c_asw_marine.h"
#include "c_asw_game_resource.h"
#include "c_asw_marine_resource.h"
#include "asw_hud_crosshair.h"
#include "asw_input.h"
#define CASW_Marine_Resource C_ASW_Marine_Resource
#else
#include "asw_pickup.h"
#include "asw_marine_resource.h"
#include "asw_marine.h"
#include "asw_player.h"
#include "asw_weapon.h"
#include "asw_game_resource.h"
#include "npcevent.h"
#include "asw_sentry_base.h"
#include "world.h"
#include "te_effect_dispatch.h"
#include "asw_util_shared.h"
#include "asw_marine_profile.h"
#include "asw_equipment_list.h"
#include "asw_weapon_parse.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( ASW_Weapon_Ammo_Bag, DT_ASW_Weapon_Ammo_Bag )

BEGIN_NETWORK_TABLE( CASW_Weapon_Ammo_Bag, DT_ASW_Weapon_Ammo_Bag )
#ifdef CLIENT_DLL
	// recvprops
	RecvPropArray3		( RECVINFO_ARRAY(m_AmmoCount), RecvPropInt( RECVINFO(m_AmmoCount[0])) ),
#else
	// sendprops
	SendPropArray3	( SENDINFO_ARRAY3(m_AmmoCount), SendPropInt( SENDINFO_ARRAY(m_AmmoCount), ASW_AMMO_BAG_SLOTS, SPROP_UNSIGNED ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CASW_Weapon_Ammo_Bag )
	
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( asw_weapon_ammo_bag, CASW_Weapon_Ammo_Bag );
PRECACHE_WEAPON_REGISTER(asw_weapon_ammo_bag);

#define THROW_INTERVAL 0.5

#ifndef CLIENT_DLL

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CASW_Weapon_Ammo_Bag )
	DEFINE_ARRAY( m_AmmoCount, FIELD_INTEGER, ASW_AMMO_BAG_SLOTS ),
	DEFINE_FIELD( m_fBurnDamage, FIELD_FLOAT ),
END_DATADESC()

#endif /* not client */

CASW_Weapon_Ammo_Bag::CASW_Weapon_Ammo_Bag()
{
}

#ifdef GAME_DLL
void CASW_Weapon_Ammo_Bag::Spawn()
{
	BaseClass::Spawn();

	SetThink( &CASW_Weapon_Ammo_Bag::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
}
#endif
