#pragma once
class FacesRebarAssembly;
#include "FacesRebarAssembly.h"

class PlaneRebarAssembly : public FacesRebarAssembly
{
	BE_DATA_VALUE(PIT::LineSegment, LineSeg1)	//��׼�߶Σ���ƽ�洹ֱ��XOYƽ�棬��Ϊ�ױߣ���ƽ��ƽ����XOYƽ�棬��Ϊ���
		BE_DATA_VALUE(PIT::LineSegment, LineSeg2)	//��׼�߶Σ�ƽ���ڹ�LineSeg1����㴹ֱ��LineSeg�ķ�����߶�
		BE_DATA_VALUE(STWallGeometryInfo, STwallData)
		//	bool				_bAnchor;
		unsigned short		_anchorPos = 0;			//ֱêλ��
	ElementHandle		_ehCrossPlanePre;
	ElementHandle		_ehCrossPlaneNext;
	unsigned short		_bendPos = 0;			//90������λ��

public:
	PlaneRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~PlaneRebarAssembly();

private:
	//�����ڲ����ȥ����ǽ��֮���ƫ�ƾ���
	double InsideFace_OffsetLength(DPoint3dCR RebarlineMidPt);
	//��ȡrebarcurve,��Ҫ�Ǹֽ������������ʵ���ཻ��Ȼ���ڸֽ�����������������Ķ˲���ʽ
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebar, const PIT::PITRebarEndTypes& endTypes);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Plane; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:
	RebarSetTag* MakeRebars(ElementId& rebarSetId, PIT::LineSegment rebarLine, PIT::LineSegment vec, int dir, BrStringCR sizeKey, double xLen, double spacing, double startOffset, double endOffset, int	level, int grade, int DataExchange, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, DgnModelRefP modelRef);

	void CalculateUseHoles(DgnModelRefP modelRef);


public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(PlaneRebarAssembly, RebarAssembly)

public:
	//��Ҫ����������������ֽ������
	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
	//	void SetAnchor(bool anchor) { _bAnchor = anchor; }
	void SetCrossPlanePre(ElementHandleCR eh) { _ehCrossPlanePre = eh; }
	void SetCrossPlaneNext(ElementHandleCR eh) { _ehCrossPlaneNext = eh; }
	//�����ǰ廹��ǽ������ǰ����������񣬵��棬����
	virtual bool	AnalyzingFloorData(ElementHandleCR eh);
	//����ǽ�����Ϣ����������ǽ�ֽ�ê����Ϣ
	void AnalyzeFloorAndWall(EditElementHandleCR eeh, int i, MSElementDescrP EdpFace);
	void DealWintHoriRebar(PIT::LineSegment& lineSeg1, PIT::LineSegment& linesegment2, int rebarnum);
	void GetHoriEndType(PIT::LineSegment lineSeg1, int rebarnum);
	//�����������
	int GetFaceType(MSElementDescrP face, MSElementDescrP upface[40], int upfacenum, MSElementDescrP downface[40], int downfacenum, int i, DVec3d vecRebar);
	void ChangeRebarLine(PIT::LineSegment& lineSeg);
	//bisSumps�Ƿ��Ǽ�ˮ��
	void CreateAnchorBySelf(vector<MSElementDescrP> tmpAnchordescrs, PIT::LineSegment Lineseg, double benrandis, double la0, double lae, double diameter, double diameterPre, int irebarlevel, bool isInface = true, bool bisSumps = false);
	//�ֽ������׶�ʱ������˲�ê����Ϣ���������ȣ�����
	bool GetHoleRebarAnchor(Dpoint3d ptstart, Dpoint3d ptend, Dpoint3d curPt, PIT::PITRebarEndType& endtype);

	//���õ�ǰ�����
	virtual void SetCurentFace(EditElementHandle* face)
	{
		m_CurentFace = face;
	}
	/*
	* @desc:	���ݿ׶��ضϸֽ�ê�̳��ȵõ��µ�ê�̳���
	* @param[in] ptStr �ֽ�ê�̲��ֵ���ʼ��
	* @param[in] vecAnchor ê�̷�����������ê�̽�����
	* @param[in] rebarLevel ��ǰ�ֽ�㣬���ptStr�ǵ�ǰ����ʼ�㣬����0�����ptStr�ǵ�һ����ʼ�㣬���ǰ��
	* @param[in\out] dAnchorleng ê�̳��ȣ���������ê�̽����㣬���ڽضϺ����Ϊ�µĳ���
	* @author	Hong ZhuoHui
	* @Date:	2024/3/1
	*/
	void CutRebarAnchorLeng(Dpoint3d ptStr, CVector3D vecAnchor, int rebarLevel, double& dAnchorleng);
	/*
	* @desc:		������ΧԪ�غ͸ֽ��ߵ�λ�ü���ֽ��Ƿ�Ϸ�
	* @param[in]	originPt  �ֽ���ʼ��
	* @param[in]	dSideCover ���򱣻�����
	* @param[in]	alleehs  ����ʵ�弯��
	* @param[in\out]    tmpEndType  �ֽ��ĩ������
	* @author	LiuSilei
	* @Date:	2024/11/15
	*/
	// �ֽ�ê�̼���봦����
	void PlaneRebarAssembly::CheckRebarAnchorage(
		DPoint3d& originPt,    // �ֽ���ʼ��
		double dSideCover,  // ���򱣻�����
		vector<EditElementHandle*> alleehs, // ����ʵ�弯��
		PIT::PITRebarEndType& tmpEndType // �ֽ��ĩ������
	);
	//��ˮ��
	void CreateCatchpitBySelf(vector<MSElementDescrP> tmpAnchordescrs, PIT::LineSegment Lineseg, double benrandis, double la0, double lae, double diameter, int irebarlevel, bool isInface = true, bool bisSumps = false, bool isYdir = false);
	/*
	* @desc:		����ֽ����Z�Ͱ�ĸֽ��������������������з��������۳��̶���Ҫ�ڹս�����ƽ��
	* @param[in]	sectionPairs  �ֽ����
	* @param[in]	parafaces ƽ��ǽ
	* @param[in]	minP  ��С��
	* @param[in]	maxP  ����
	* @param[in]    isXDir  �ֽ�ֲ�����
	* @author	LiuSilei
	* @Date:	2024/10/30
	*/
	bool CalculateZCorner(map<int, int>& sectionPairs, vector<MSElementDescrP>& parafaces, DPoint3d& minP, DPoint3d& maxP, bool isXDir = true);
	//��������ֽ���Ϣ�������˲���ʽ��
	void CalculateInSideData(MSElementDescrP face/*��ǰ�����*/,
		MSElementDescrP tmpupfaces[40],
		MSElementDescrP tmpdownfaces[40],
		int i,
		DVec3d rebarVec);
	//��������ֽ���Ϣ�������˲���ʽ��
	void CalculateOutSideData(MSElementDescrP face/*��ǰ�����*/,
		MSElementDescrP tmpupfaces[40],
		MSElementDescrP tmpdownfaces[40],
		int i,
		DVec3d rebarVec);

	// @brief ɨ��range�ڵ�����Ԫ��
	// @param range 
	// @param filter ���˺���
	// @return 
	std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);

	/*
	* @desc:	�жϸ�Ԫ���Ƿ���ǽԪ��
	* @param[in] element Ԫ��
	*/
	bool is_Wall(const ElementHandle &element);

	/*
	* @desc:	���øֽ�����յ�����ƫ�� ǽ�������һ��������ֽ��ֱ��
	* @param[in] RebarPt ͨ���õ�����ĳ��ǽ�ķ�Χ�л�ȡ��ǽ������
	*/
	double WallRebars_OffsetLength(DPoint3dCR RebarPt);

	//ȥ���ظ���
	static void RemoveRepeatPoint(vector<DPoint3d>& vecPoint);
	static double GetLae();

	//�ȵ����Ӧ����
	struct LDFloorData
	{
		DPoint3d oriPt = { 0 };//�ȵ�����С��
		double Xlenth = 0;//ת����XOYƽ������X����
		double Ylenth = 0;
		double Zlenth = 0;//���
		DVec3d Vec = DVec3d::From(0, 0, 1);//Ĭ��ȡZ�᷽��
		MSElementDescrP facedes = nullptr;//����
		MSElementDescrP facedesUp = nullptr;//����
		MSElementDescrP upfaces[40];//��¼�嶥ǽ����ཻ����
		int upnum = 0;//��¼����ǽ�����
		MSElementDescrP downfaces[40];//��¼���ǽ����ཻ����
		int downnum = 0;//��¼����ǽ�����
	}m_ldfoordata;
	enum  SideType//���������
	{
		Nor = 0,//������
		Out,//�����
		In  //�ڲ���
	}m_sidetype;

	struct InSideQuJianInfo
	{
		int str = 0;
		int end = 0;
		bool addstr = false;//��ʼλ���Ƿ���Ҫ�Ӹֽ�
		bool addend = false;//��ֹλ���Ƿ���Ҫ�Ӹֽ�
		int strval = 0;
		int endval = 0;
	};

	struct InSideFaceInfo//�ڲ������õ��������Ϣ��ÿһ�������
	{
		InSideQuJianInfo  pos[20] = { 0 };//��ֽ���ƽ�еı��������䣨����ֽ�ʱΪZ����ֵ������ֽ�ʱΪX����ֵ��
		int  posnum = 0;//��ʶ��������
		bool Verstr = false;//��ʼ�д�ֱǽ
		bool Verend = false;//β���д�ֱǽ
		double calLen = 0;//�д�ֱǽʱ����ȥ�ĸ��ֽ�ֱ��
		int strval = 0;
		int endval = 0;
		PIT::EndType endtype = { 0 };
		PIT::EndType strtype = { 0 };
		bool bStartAnhorsel = false;
		bool bEndAnhorsel = false;
		double dStartanchoroffset = 0;
		double dEndanchoroffset = 0;
		bool m_bStartIsSlefAhol = false;//��ǰ�乳����ѡ��ê��
		bool m_bEndIsSlefAhol = false;//��ǰ�乳����ѡ��ê��
		void ClearData()
		{
			for (int i = 0; i < 20; i++)
			{
				pos[i].str = 0;
				pos[i].end = 0;
				pos[i].addstr = false;
				pos[i].addend = false;
				pos[i].strval = 0;
				pos[i].endval = 0;
			}
			posnum = 0;
			Verstr = false;
			Verend = false;
			calLen = 0;
			strval = 0;
			endval = 0;
			endtype = { 0 };
			strtype = { 0 };
			bStartAnhorsel = false;
			bEndAnhorsel = false;
			dStartanchoroffset = 0;
			dEndanchoroffset = 0;
			m_bStartIsSlefAhol = false;
			m_bEndIsSlefAhol = false;

		}
	}m_insidef;
	bool m_bStartAnhorselslantedFace = false; //ê��б��
	bool m_bEndAnhorselslantedFace = false;

	struct OutSideQuJianInfo
	{
		int str = 0;//�������
		int end = 0;//�����յ�
		bool addstr = false;//��ʼλ���Ƿ���Ҫ�Ӹֽ�
		bool addend = false;//��ֹλ���Ƿ���Ҫ�Ӹֽ�
		int strval = 0;//��������ľ���
		int endval = 0;//�յ������ľ���
	};
	struct OutSideFaceInfo//��������õ��������Ϣ��ÿһ�������
	{
		OutSideQuJianInfo  pos[20] = { 0 };//��ֽ���ƽ�еı��������䣨����ֽ�ʱΪZ����ֵ������ֽ�ʱΪX����ֵ��
		int  posnum = 0;//��ʶ��������
		bool isdelstr = false;//�Ƿ�ɾ����ʼ�ֽΪ�����ǽ�ֽ�����
		bool isdelend = false;//�Ƿ�ɾ��β���ֽΪ�����ǽ�ֽ�����
		bool Verstr = false;//��ʼ�д�ֱǽ
		bool Verend = false;//β���д�ֱǽ
		double calLen = 0;//�жϵ�ǰ�ֽ�㣬�ֽ�ǰ�滹�в㣬��㲿�ֳ�����С1���ֽ�ֱ����û�в�������С���ֽ�ê�봦��
		int strval = 0;
		int endval = 0;
		PIT::EndType endtype = { 0 };
		PIT::EndType strtype = { 0 };
		bool bStartAnhorsel = false;
		bool bEndAnhorsel = false;
		double dStartanchoroffset = 0;
		double dEndanchoroffset = 0;
		bool m_bStartIsSlefAhol = false;
		bool m_bEndIsSlefAhol = false;
		void ClearData()
		{
			for (int i = 0; i < 20; i++)
			{
				pos[i].str = 0;
				pos[i].end = 0;
				pos[i].addstr = false;
				pos[i].addend = false;
				pos[i].strval = 0;
				pos[i].endval = 0;
			}
			posnum = 0;
			isdelstr = false;
			isdelend = false;
			Verstr = false;
			Verend = false;
			calLen = 0;
			strval = 0;
			endval = 0;
			endtype = { 0 };
			strtype = { 0 };
			bStartAnhorsel = false;
			bEndAnhorsel = false;
			dStartanchoroffset = 0;
			dEndanchoroffset = 0;
			m_bStartIsSlefAhol = false;
			m_bEndIsSlefAhol = false;
		}
	}m_outsidef;

	struct NorSideQuJianInfo
	{
		int str = 0;
		int end = 0;
		bool addstr = false;//��ʼλ���Ƿ���Ҫ�Ӹֽ�
		bool addend = false;//��ֹλ���Ƿ���Ҫ�Ӹֽ�
		int strval = 0;
		int endval = 0;
	};
	struct NorSideFaceInfo//��ֱ�����õ��������Ϣ��ÿһ����ֱ��
	{
		NorSideQuJianInfo  pos[20] = { 0 };//��ֽ���ƽ�еı��������䣨����ֽ�ʱΪZ����ֵ������ֽ�ʱΪX����ֵ��
		int  posnum = 0;//��ʶ��������
		double calLen = 0;//�жϵ�ǰ�ֽ�㣬�ֽ�ǰ�滹�в㣬��㲿�ֳ�����С1���ֽ�ֱ����û�в�������С���ֽ�ê�봦��
		int strval = 0;
		int endval = 0;
		PIT::EndType endtype = { 0 };
		PIT::EndType strtype = { 0 };
		bool bStartAnhorsel = false;
		bool bEndAnhorsel = false;
		double dStartanchoroffset = 0;
		double dEndanchoroffset = 0;
		bool m_bStartIsSlefAhol = false;
		bool m_bEndIsSlefAhol = false;
		void ClearData()
		{
			for (int i = 0; i < 20; i++)
			{
				pos[i].str = 0;
				pos[i].end = 0;
				pos[i].addstr = false;
				pos[i].addend = false;
				pos[i].strval = 0;
				pos[i].endval = 0;
			}
			posnum = 0;
			calLen = 0;
			strval = 0;
			endval = 0;
			endtype = { 0 };
			strtype = { 0 };
			bStartAnhorsel = false;
			bEndAnhorsel = false;
			dStartanchoroffset = 0;
			dEndanchoroffset = 0;
			m_bStartIsSlefAhol = false;
			m_bEndIsSlefAhol = false;
		}
	}m_norsidef;

	struct LDSlabGeometryInfo//������Ϣ
	{
		DPoint3d ptStart;
		DPoint3d ptEnd;
		double length;
		double width;    //���
		double height;
		DVec3d vecZ;

	}m_STslabData;

	struct verSlabFaceInfo
	{
		PIT::EndType endtype = { 0 };
		PIT::EndType strtype = { 0 };
		bool bStartAnhorsel = false;
		bool bEndAnhorsel = false;
		double dStartanchoroffset = 0;
		double dEndanchoroffset = 0;
		void ClearData()
		{
			endtype = { 0 };
			strtype = { 0 };
			bStartAnhorsel = false;
			bEndAnhorsel = false;
			dStartanchoroffset = 0;
			dEndanchoroffset = 0;
		}
	}m_verSlabFaceInfo;

	struct HoleRebarInfo
	{
		bool bIsUpFace = false;
		BrString brstring = "";

		void ClearData()
		{
			bIsUpFace = false;
			brstring = "";
		}

	}m_holeRebarInfo;

	CVector3D m_vecEndNormalStart;//ê��б��ʱ�ã�ֱ�����endnormal
	CVector3D m_vecEndNormalEnd;



	PIT::EndType m_topEndinfo = { 0 };
	double m_dUpSlabThickness = 0;
	bool m_bUpIsStatr = false;
	PIT::EndType m_bottomEndinfo = { 0 };
	double m_dbottomSlabThickness = 0;

	int m_wallTopFaceType = -1; //0:���棬1������
	int m_wallBottomFaceType = -1; //0:���棬1������
	double m_dTopOffset = 0;
	double m_dBottomOffset = 0;

	int m_iCurRebarDir = -1;
	std::vector<ElementHandle> m_associatedWall;

	PIT::EndType m_HorEndType = { 0 };
	PIT::EndType m_HorStrType = { 0 };
	int m_bStartType = -1;//0:���棬1������
	int m_bEndInType = -1;//0:���棬1������
	double m_dStartOffset = 0;
	double m_dEndOffset = 0;
	MSElementDescrP m_TopSlabEdp = nullptr;
	MSElementDescrP m_BottomSlabEdp = nullptr;
	vector<MSElementDescrP> m_vecWallEdp;

	int m_nowvecDir;//��ǰ�ֽ��0��ʾx�ᣬ1��ʾz��
	EditElementHandle* m_pOldElm = NULL;  // ����׶��İ�

	double m_curreDiameter;//��ǰֱ��
	double m_twoFacesDistance = 0.0;//��ˮ���ڲ����������֮��ľ���
	int m_curLevel;//��ǰ����
	EditElementHandle* m_CurentFace = nullptr;//��ǰ�������
	double m_slabThickness = 0.0;//�������ѡ��İ�ĺ��
	EditElementHandle* m_zCorner = nullptr;//Z�Ͱ�ս���Ҫ���ö���ĸֽ�

	vector<ElementHandle> m_Allwalls;//������߸������е�ǽ
	vector<EditElementHandle*> m_allEehs;//�����Լ���������ʵ��
	bool m_strDelete = false;//�����ֽ��ǽ�����ʼ�ֽ��Ƿ�ɾ�������ǽ�ֽ�����

	bool m_endDelete = false;//�����ֽ���ʼ�ֽ��Ƿ�ɾ�������ǽ�ֽ�����
};


