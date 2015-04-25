#ifndef _INCLUDED_ASW_PROP_DYNAMIC_H
#define _INCLUDED_ASW_PROP_DYNAMIC_H

#include "props.h"


class CASW_Prop_Dynamic : public CDynamicProp
{
public:
	DECLARE_CLASS( CASW_Prop_Dynamic, CDynamicProp );

protected:
	virtual int		OnTakeDamage( const CTakeDamageInfo &inputInfo );
};


#endif // _INCLUDED_ASW_PROP_DYNAMIC_H