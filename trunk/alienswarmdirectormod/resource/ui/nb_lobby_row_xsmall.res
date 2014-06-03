"Resource/UI/NB_Lobby_Row_XSmall.res"
{
	"AvatarImage"
	{
		"fieldName"		"AvatarImage"
		"xpos"		"9"
		"ypos"		"0"
		"wide"		"8"
		"tall"		"8"
		"ControlName"		"CAvatarImagePanel"
		"scaleImage"		"1"
		"zpos"		"2"
	}
	"ClassImage"
	{
		"fieldName"		"ClassImage"
		"xpos"		"112"
		"ypos"		"0"
		"wide"		"8"
		"tall"		"8"
		"ControlName"		"ImagePanel"
		"scaleImage"		"1"
		"zpos"		"2"
	}
// 	"NameLabel"
// 	{
// 		"fieldName"		"NameLabel"
// 		"xpos"		"18"
// 		"ypos"		"0"
// 		"wide"		"42"
// 		"tall"		"7"
// 		"font"		"DefaultVerySmall"
// 		"labelText"		"NameLabel"
// 		"textAlignment"		"west"
// 		"ControlName"		"Label"
// 		"zpos"		"2"
// 		"fgcolor_override"		"83 148 192 255"
// 	}
	"DrpNameLabel"
	{
		"ControlName"			"DropDownMenu"
		"fieldName"			"DrpNameLabel"
		"xpos"		"18"
		"ypos"		"-2"
		"wide"		"51"
		"tall"		"12"
		"visible"			"1"
		"enabled"			"1"
		"zpos"			"2"
		"tabPosition"			"0"

		"BtnDropButton"
		{
			"ControlName"			"BaseModHybridButton"
			"fieldName"			"BtnDropButton"
			"xpos"				"0"
			"ypos" 				"0"
			"tall"				"12"
			"wide"				"51"
			"visible"			"1"
			"enabled"			"1"
			"tabPosition"		"0"
			"tooltiptext"		""
			"style"				"AlienSwarmDefault"
			"command"			"PlayerFlyout"
			"ShowDropDownIndicator"	"0"
		}
	}
	"LevelLabel"
	{
		"fieldName"		"LevelLabel"
		"xpos"		"72"
		"ypos"		"0"
		"wide"		"33"
		"tall"		"7"
		"font"		"DefaultVerySmall"
		"labelText"		"LevelLabel"
		"textAlignment"		"west"
		"ControlName"		"Label"
		"zpos"		"2"
		"fgcolor_override"		"83 148 192 255"
	}
	"PromotionIcon"
	{
		"fieldName"		"PromotionIcon"
		"ControlName"		"ImagePanel"
		"xpos"		"66"
		"ypos"		"1"
		"scaleImage"		"1"
		"wide"		"5"
		"tall"		"5"
		"zpos"		"1"
	}
	"XPBar"
	{
		"fieldName"		"XPBar"
		"xpos"		"23"
		"ypos"		"75"
		"wide"		"146"
		"tall"		"3"
		"ControlName"		"StatsBar"
		"visible"		"0"
		"zpos"		"2"
	}
	"PortraitButton"
	{
		"fieldName"		"PortraitButton"
		"xpos"		"10"
		"ypos"		"8"
		"wide"		"19"
		"tall"		"19"
		"ControlName"		"CBitmapButton"
		"zpos"		"2"
	}
	"WeaponButton0"
	{
		"fieldName"		"WeaponButton0"
		"xpos"		"30"
		"ypos"		"8"
		"wide"		"36"
		"tall"		"19"
		"ControlName"		"CBitmapButton"
		"zpos"		"2"
	}
	"WeaponButton1"
	{
		"fieldName"		"WeaponButton1"
		"xpos"		"66"
		"ypos"		"8"
		"wide"		"36"
		"tall"		"19"
		"ControlName"		"CBitmapButton"
// 		"pin_to_sibling"		"WeaponButton0"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"1"
		"zpos"		"2"
	}
	"WeaponButton2"
	{
		"fieldName"		"WeaponButton2"
		"xpos"		"102"
		"ypos"		"8"
		"wide"		"19"
		"tall"		"19"
		"ControlName"		"CBitmapButton"
// 		"pin_to_sibling"		"WeaponButton1"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"1"
		"zpos"		"2"
	}
	"ClassLabel"
	{
		"fieldName"		"ClassLabel"
		"xpos"		"79"
		"ypos"		"0"
		"wide"		"33"
		"tall"		"7"
		"font"		"DefaultVerySmall"
		"labelText"		"ClassLabel"
		"textAlignment"		"east"
		"ControlName"		"Label"
		"zpos"		"2"
		"fgcolor_override"		"83 148 192 255"
		"visible"	"0"
	}
	"Background"
	{
		"fieldName"		"Background"
		"visible"		"0"
		"ControlName"		"ImagePanel"
	}
	"BackroundPlain"
	{
		"fieldName"		"BackroundPlain"
		"xpos"		"9"
		"ypos"		"0"
		"wide"		"112"
		"tall"		"28"
		"ControlName"		"Panel"
		"PaintBackgroundEnabled"		"1"
		"PaintBackgroundType"		"2"
		"bgcolor_override"		"23 43 65 255"
	}
	"BackgroundWeapon0"
	{
		"fieldName"		"BackgroundWeapon0"
		"xpos"		"30"
		"ypos"		"8"
		"wide"		"35"
		"tall"		"18"
		"ControlName"		"Panel"
		"PaintBackgroundType"		"2"
		"bgcolor_override"		"16 32 49 255"	
// 		"pin_to_sibling"		"WeaponButton0"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"zpos"		"1"
	}
	"BackgroundWeapon1"
	{
		"fieldName"		"BackgroundWeapon1"
		"xpos"		"66"
		"ypos"		"8"
		"wide"		"36"
		"tall"		"18"
		"ControlName"		"Panel"
// 		"pin_to_sibling"		"WeaponButton1"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"PaintBackgroundType"		"2"
		"bgcolor_override"		"16 32 49 255"
		"zpos"		"1"
	}
	"BackgroundWeapon2"
	{
		"fieldName"		"BackgroundWeapon2"
		"xpos"		"102"
		"ypos"		"8"
		"wide"		"18"
		"tall"		"18"
		"ControlName"		"Panel"
// 		"pin_to_sibling"		"WeaponButton2"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"PaintBackgroundType"		"2"
		"bgcolor_override"		"16 32 49 255"
		"zpos"		"1"
	}
	"BackgroundInnerWeapon0"
	{
		"fieldName"		"BackgroundInnerWeapon0"
		"xpos"		"30"
		"ypos"		"9"
		"wide"		"34"
		"tall"		"17"
		"ControlName"		"Panel"
		"PaintBackgroundType"		"2"
		"bgcolor_override"		"32 57 82 255"
// 		"pin_to_sibling"		"BackgroundWeapon0"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"zpos"		"2"
	}
	"BackgroundInnerWeapon1"
	{
		"fieldName"		"BackgroundInnerWeapon1"
		"xpos"		"66"
		"ypos"		"9"
		"wide"		"34"
		"tall"		"17"
		"ControlName"		"Panel"
// 		"pin_to_sibling"		"BackgroundWeapon1"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"PaintBackgroundType"		"2"
		"bgcolor_override"		"32 57 82 255"
		"zpos"		"2"
	}
	"BackgroundInnerWeapon2"
	{
		"fieldName"		"BackgroundInnerWeapon2"
		"xpos"		"102"
		"ypos"		"9"
		"wide"		"17"
		"tall"		"17"
		"ControlName"		"Panel"
// 		"pin_to_sibling"		"BackgroundWeapon2"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"PaintBackgroundType"		"2"
		"bgcolor_override"		"32 57 82 255"
		"zpos"		"2"
	}
	"SilhouetteWeapon0"
	{
		"fieldName"		"SilhouetteWeapon0"
// 		"xpos"		"-4"
// 		"ypos"		"-4"
		"xpos"		"32"
		"ypos"		"10"
		"wide"		"32"
		"tall"		"16"
		"ControlName"		"ImagePanel"
// 		"pin_to_sibling"		"BackgroundWeapon0"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"scaleImage"	"1"
		"image"		"briefing/weapon_primary_silhouette"
		"zpos"		"3"
		"visible"	"1"
	}
	"SilhouetteWeapon1"
	{
		"fieldName"		"SilhouetteWeapon1"
// 		"xpos"		"-4"
// 		"ypos"		"-4"
		"xpos"		"68"
		"ypos"		"10"
		"wide"		"32"
		"tall"		"16"
		"ControlName"		"ImagePanel"
// 		"pin_to_sibling"		"BackgroundWeapon1"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"scaleImage"	"1"
		"image"		"briefing/weapon_primary_silhouette"
		"zpos"		"3"
	}
	"SilhouetteWeapon2"
	{
		"fieldName"		"SilhouetteWeapon2"
// 		"xpos"		"-4"
// 		"ypos"		"-4"
		"xpos"		"104"
		"ypos"		"10"
		"wide"		"15"
		"tall"		"15"
		"ControlName"		"ImagePanel"
// 		"pin_to_sibling"		"BackgroundWeapon2"
// 		"pin_corner_to_sibling"		"0"
// 		"pin_to_sibling_corner"		"0"
		"scaleImage"	"1"
		"image"		"briefing/weapon_extra_silhouette"
		"zpos"		"3"
	}
	"ReadyCheckImage"
	{
		"fieldName"		"ReadyCheckImage"
		"ControlName"		"ImagePanel"
		"xpos"		"1"
		"ypos"		"0"
		"scaleImage"		"1"
		"wide"		"6"
		"tall"		"6"
		"zpos"		"1"
	}
	"VoiceIcon"
	{
		"fieldName"		"VoiceIcon"
		"ControlName"		"ImagePanel"
		"xpos"		"2"
		"ypos"		"8"
		"scaleImage"		"1"
		"wide"		"6"
		"tall"		"6"
		"zpos"		"1"
		"image"	"../voice/voice_icon_hud"
	}
	// changing cyclers
	"ChangingSlot0"
	{
		"fieldName"		"ChangingSlot0"
		"xpos"		"12"
		"ypos"		"10"
		"wide"		"16"
		"tall"		"16"
		"ControlName"		"ImagePanel"
		"scaleImage"	"1"
		"image"		"common/swarm_cycle_anim"
		"zpos"		"5"
		"drawcolor"	"169 213 255 64"
	}
	"ChangingSlot1"
	{
		"fieldName"		"ChangingSlot1"
		"xpos"		"-9"
		"ypos"		"-1"
		"wide"		"16"
		"tall"		"16"
		"ControlName"		"ImagePanel"
		"pin_to_sibling"		"BackgroundWeapon0"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"0"
		"scaleImage"	"1"
		"image"		"common/swarm_cycle_anim"
		"zpos"		"5"
		"drawcolor"	"169 213 255 64"
	}
	"ChangingSlot2"
	{
		"fieldName"		"ChangingSlot2"
		"xpos"		"-9"
		"ypos"		"-1"
		"wide"		"16"
		"tall"		"16"
		"ControlName"		"ImagePanel"
		"pin_to_sibling"		"BackgroundWeapon1"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"0"
		"scaleImage"	"1"
		"image"		"common/swarm_cycle_anim"
		"zpos"		"5"
		"drawcolor"	"169 213 255 64"
	}
	"ChangingSlot3"
	{
		"fieldName"		"ChangingSlot3"
		"xpos"		"-1"
		"ypos"		"-1"
		"wide"		"15"
		"tall"		"15"
		"ControlName"		"ImagePanel"
		"pin_to_sibling"		"BackgroundWeapon2"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"0"
		"scaleImage"	"1"
		"image"		"common/swarm_cycle_anim"
		"zpos"		"5"
		"drawcolor"	"169 213 255 64"
	}
}