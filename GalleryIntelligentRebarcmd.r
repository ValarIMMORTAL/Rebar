/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/RebarSDKExamplecmd.r $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
| TagsExample command resource
+-----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
|                                                                       |
|    Include Files                                                      |
|                                                                       |
+----------------------------------------------------------------------*/
#pragma suppressREQCmds

#include <Mstn\MdlApi\rscdefs.r.h>
#include <Mstn\MdlApi\cmdclass.r.h>

#include "GalleryIntelligentRebarIds.h"

/*----------------------------------------------------------------------+
|                                                                       |
|  Register Application and DLL                                         |
|                                                                       |
+----------------------------------------------------------------------*/
#define DLLAPP_REBARSDKEXAMPLE 1

DllMdlApp DLLAPP_REBARSDKEXAMPLE =
    {
        L"GALLERYINTELLIGENTREBAR", L"GalleryIntelligentRebar" // taskid, dllName
}

/*----------------------------------------------------------------------+
|                                                                       |
|   Immediate Defines                                                   |
|                                                                       |
+----------------------------------------------------------------------*/
#define CT_NONE 0
#define CT_MAIN 1
#define CT_REBAR 2
#define CT_PLACE 3
#define CT_EDIT 4
#define CT_CREATE 5
#define CT_GALLERY 6
#define CT_SETTINGS 7
#define CT_SINGLE_WALL 8
#define CT_SINGLE_FLOOR 9
#define CT_WALL_AND_FLOOR 10
#define CT_COMBINE_FLOOR 11
#define CT_FaceRebar 12
#define CT_EdageRebar 13
#define CT_COMBINE_WALL 14
/*----------------------------------------------------------------------+
|                                                                       |
|   Command Tables                                                      |
|                                                                       |
+----------------------------------------------------------------------*/
CommandTable CT_MAIN =
{
    {1, CT_REBAR, MANIPULATION, REQ, "LDREBAR"},
};

CommandTable CT_REBAR =
{
    {1, CT_PLACE, INHERIT, REQ, "PLACE"},
    {2, CT_GALLERY, INHERIT, REQ, "GALLERY"},
    {3, CT_EDIT, INHERIT, REQ, "EDIT"}
};

CommandTable CT_PLACE =
{
    {1, CT_NONE, INHERIT, DEF, "WALLREBAR"},
    {2, CT_NONE, INHERIT, DEF, "SLABREBAR"},
};

CommandTable CT_EDIT =
{
 {1, CT_NONE, INHERIT, DEF, "REFRESHDATA"},
 {2, CT_NONE, INHERIT, DEF, "DELETEPROSTHESES"},
};

// �ȵ����
CommandTable CT_GALLERY =
{
    // �������(ѡ���ȵ�+����)
    {1, CT_SETTINGS, INHERIT, DEF, "SETTINGS"},
    // �ȵ�ǽ�������
    {2, CT_SINGLE_WALL, INHERIT, DEF, "SINGLE_WALL"},
	// �ȵ��嵥�����
    {3, CT_SINGLE_FLOOR, INHERIT, DEF, "SINGLE_FLOOR"}, 
    // ǽ���������
    {4, CT_WALL_AND_FLOOR, INHERIT, DEF, "WALL_AND_FLOOR"}, 
    // �ϲ������
    {5, CT_COMBINE_FLOOR, INHERIT,DEF, "COMBINE_FLOOR"},
	// ���ô����ê�̳���
	{6,CT_NONE, INHERIT, DEF, "SET_LAEANDLA0"},
	// ֧�����
	{7,CT_NONE, INHERIT, DEF, "SINGLE_BUTTRESS"},
	//�����
	{8, CT_FaceRebar, INHERIT,DEF, "FaceRebar"},
	//������
	{9,CT_NONE, INHERIT, DEF, "FACETIEREBAR"},
	//�ر߼�ǿ��
	{10, CT_EdageRebar, INHERIT,DEF, "EdageRebar"},
	//¥�����
	{11, CT_NONE, INHERIT,DEF, "StairsRebar"},
	// �׶���ǿ��
	{12, CT_NONE, INHERIT,DEF, "HOLEREINFORCINGREBAR"},
	//�ϲ�ǽ���
	{13, CT_NONE, INHERIT,DEF, "CombineWallRebar"},
	//��ˮ�����
	{14, CT_NONE, INHERIT,DEF, "CatchpitRebar"},
	//����һ���޸Ŀ��Ա���ͨ�����Լ�д���뱨��
};
