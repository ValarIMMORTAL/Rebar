#pragma once
#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "WallRebarAssembly.h"

class CParapetDlg;
class ParapetRebarAssembly : public STWallRebarAssembly
{
public:
	ParapetRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		STWallRebarAssembly(id, modelRef)
	{
		pParapetDoubleRebarDlg = nullptr;
	}
	virtual ~ParapetRebarAssembly()
	{
	}
protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Parapet; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ParapetWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ParapetWall Rebar"; }
	virtual bool        OnDoubleClick() override;
public:
	BE_DECLARE_VMS(ParapetRebarAssembly, RebarAssembly)
	double _height;
	CParapetDlg * pParapetDoubleRebarDlg;
public:
	void InitRebarParam(double ulenth);
};