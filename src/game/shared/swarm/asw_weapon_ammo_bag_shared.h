#ifndef _INCLUDED_ASW_WEAPON_AMMO_BAG_H
#define _INCLUDED_ASW_WEAPON_AMMO_BAG_H
#pragma once

#ifdef CLIENT_DLL
#include "c_asw_weapon.h"
#define CASW_Weapon C_ASW_Weapon
#define CASW_Weapon_Ammo_Bag C_ASW_Weapon_Ammo_Bag
#define CASW_Marine C_ASW_Marine
#else
#include "asw_weapon.h"
#include "npc_combine.h"
#endif

#include "asw_shareddefs.h"
#include "basegrenade_shared.h"

// ammo bag stores clips for a number of different weapons
//   holding marine can reload from the bag
//   holding marine can give ammo clips to nearby marines

class CASW_Weapon_Ammo_Bag : public CASW_Weapon
{
public:
	DECLARE_CLASS( CASW_Weapon_Ammo_Bag, CASW_Weapon );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CASW_Weapon_Ammo_Bag();

	#ifndef CLIENT_DLL
	DECLARE_DATADESC();

	virtual void Spawn();
	#endif

	CNetworkArray( int, m_AmmoCount, ASW_AMMO_BAG_SLOTS );
	float m_fBurnDamage;

	// Classification
	virtual Class_T		Classify( void ) { return (Class_T) CLASS_ASW_AMMO_BAG; }
};


#endif /* _INCLUDED_ASW_WEAPON_AMMO_BAG_H */
