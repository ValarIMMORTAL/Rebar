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

	BE_DATA_VALUE(int,					shape);			// 0: ����   1: Բ��
	BE_DATA_VALUE(double,				length);		// ����
	BE_DATA_VALUE(double,				width);			// ���
	BE_DATA_VALUE(double,				heigth);		// �߶�
	BE_DATA_VALUE(double,				columeCover);	// ��������
	BE_DATA_VALUE(int,					longNum);		// ��������
	BE_DATA_VALUE(int,					shortNum);		// ��������
	BE_DATA_VALUE(double,				embedLength);	// ���ó���
	BE_DATA_VALUE(double,				expandLength);	// ��չ����
	BE_DATA_VALUE(int,					endType);		// �˲���ʽ����
	BE_DATA_VALUE(int,					cornerType);	// �乳����
	BE_DATA_VALUE(double,				rotateAngle);	// ��ת��

	BE_DATA_VALUE(BrString,				verticalSize);	// �ݽ�ߴ�
	BE_DATA_VALUE(int,					verticalType);	// �ݽ�����

	BE_DATA_VALUE(double,				verticalLen);	// �ݽ�ߴ�
	BE_DATA_VALUE(double,				hoopRebarLen);	// ����ߴ�

	BE_DATA_VALUE(vector<ElementId>,	vecSetId);

public:
	bool MakeRebars(DgnModelRefP modelRef);

	RebarSetTag* MakeRebars(int nSideRebarNum, int nFrontRebarNum, bool isFlag, vector<CVector3D>& vTransform, DgnModelRefP modelRef);

	void SetInsertRebarData(InsertRebarInfo& stInserRebarInfo);

	void CalcColumnLengthAndWidth(ElementHandleCR eh); // �������ĳ��ȺͿ��

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