#pragma once

/*--------------------------------------------------------------------------------------+
|
|     $Source: CInsertRebarAssemblyColumn.h $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include <RebarElements.h>
#include "CommonFile.h"

class CInsertRebarMainDlg;
class CInsertRebarAssemblyColumn : public RebarAssembly
{
	BE_DATA_REFER(BeMatrix,				Placement);

	BE_DATA_VALUE(int,					shape);			// 0: 方形   1: 圆形
	BE_DATA_VALUE(double,				length);		// 长度
	BE_DATA_VALUE(double,				width);			// 宽度
	BE_DATA_VALUE(double,				heigth);		// 高度
	BE_DATA_VALUE(double,				columeCover);	// 柱保护层
	BE_DATA_VALUE(int,					longNum);		// 长面数量
	BE_DATA_VALUE(int,					shortNum);		// 短面数量
	BE_DATA_VALUE(double,				embedLength);	// 埋置长度
	BE_DATA_VALUE(double,				expandLength);	// 拓展长度
	BE_DATA_VALUE(int,					endType);		// 端部样式类型
	BE_DATA_VALUE(int,					cornerType);	// 弯钩方向
	BE_DATA_VALUE(double,				rotateAngle);	// 旋转角

	BE_DATA_VALUE(BrString,				verticalSize);	// 纵筋尺寸
	BE_DATA_VALUE(int,					verticalType);	// 纵筋类型

	BE_DATA_VALUE(double,				verticalLen);	// 纵筋尺寸
	BE_DATA_VALUE(double,				hoopRebarLen);	// 箍筋尺寸

	BE_DATA_VALUE(vector<ElementId>,	vecSetId);

public:
	bool MakeRebars(DgnModelRefP modelRef);

	RebarSetTag* MakeRebars(int nSideRebarNum, int nFrontRebarNum, bool isFlag, vector<CVector3D>& vTransform, DgnModelRefP modelRef);

	void SetInsertRebarData(InsertRebarInfo& stInserRebarInfo);

	void CalcColumnLengthAndWidth(ElementHandleCR eh); // 计算柱的长度和宽度

	bool MakeRebarCurve(vector<RebarCurve>& rebars, double xPos, double height, double bendRadius, double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat);
	
	void CalculateTransform(vector<CVector3D>& vTransform, DgnModelRefP modelRef, bool isFlag);

	static bool IsSmartSmartFeature(EditElementHandle& eeh);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 3; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Insert Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Insert Rebar"; }
	virtual bool		OnDoubleClick() override;
	virtual bool		Rebuild() override;

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CInsertRebarAssemblyColumn, RebarAssembly)

public:
	CInsertRebarAssemblyColumn(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CInsertRebarAssemblyColumn() {};

private:

	void Init();
	
	DPoint3d m_ptStart;
	DPoint3d m_ptEnd;
	std::vector<EditElementHandle*> vecNegs;

	CInsertRebarMainDlg*		m_pInsertRebarMainDlg;

};