#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	PITRebarEndType
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/04/21
	Version:		V1.0
*	Description:	rebarEndType
*	History:
*	1. Date:		2021/04/21
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include "Bstream.h"
#include "RebarCatalog.h"
#include "RebarDetailElement.h"
#include "RebarModel.h"
namespace PIT
{
	class PITRebarEndType : public RebarObject
	{
	public:
		enum Type
		{
			kNone = 0,
			kBend,          // 90
			kCog,           // 135
			kHook,          // 180
			kLap,
			kCustom,
			kSplice,
			kTerminator,         // Mechanical Device
			kTie			//100
		};

	private:
			BE_DATA_VALUE(Type, Type)
			BE_DATA_VALUE(DPoint3d, ptOrgin)
			BE_DATA_REFER(double, angle)
			BE_DATA_REFER(CVector3D, endNormal)
			BE_DATA_REFER(double, bendLen)
			BE_DATA_REFER(double, bendRadius)
			BE_DATA_REFER(double, straightAnchorLen)
			BE_DATA_REFER(RebarEndDeviceData, DeviceData);
	public:
		PITRebarEndType()
			: m_Type(kNone)
		{}
		BE_DECLARE_VMS(PITRebarEndType, RebarObject)
	};

	struct PITRebarEndTypes
	{
		PITRebarEndType beg;
		PITRebarEndType end;
	};


	class PITRebarCode
	{
	public:
		static double GetBarDiameter(BrStringCR sizeKey,double rebarThread, DgnModelRefP modelRef);
		static double GetPinRadius(BrStringCR sizeKey, double rebarThread, DgnModelRefP modelRef, bool isStirrup);
		static double GetBendLength(BrStringCR sizeKey, const PITRebarEndType &endType, DgnModelRefP modelRef);
		static bool IsCenterLineLength();
	};
}

