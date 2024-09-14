#include "_ustation.h"
#include "PITRebarEndType.h"

// bool PITRebarEndType::GenerateArc(BeArcSegR seg, CVector3D rebarVec, CVector3D endNormal, double angle,double len) const
// {
// 	double bendRadius = seg.GetRadius();
// 	DPoint3d vec = GetptOrgin();
// 	vec.Normalize();
// 	vec.Scale(bendRadius);
// 
// 	CPoint3D ptBend, ptMid, ptEnd;
// 
// 	ptMid = GetptOrgin();
// 	ptEnd = ptMid;
// 	ptEnd.Add(vec);
// // 	DPoint3d pt1 = endNormal;
// // 	pt1.Scale(bendRadius + len);
// // 	pt1.Add(ptMid);
// 
// 
// 	CPoint3D ptNormal = endNormal;
// 	ptNormal.Scale(bendRadius);
// 	ptBend = ptMid;
// 	ptBend.Add(ptNormal);
// 
// 	CPoint3D ptCenter = endNormal;
// 	ptCenter.Scale(bendRadius);
// 	ptCenter.Add(ptEnd);
// 
// 	DEllipse3d delp = DEllipse3d::FromArcCenterStartEnd(ptCenter, ptBend, ptEnd);
// 	delp.Evaluate(ptMid, sin(angle*0.5), cos(angle*0.5));
// 	seg.mid = ptMid;
// 	seg.start = ptBend;
// 	seg.end = ptEnd;
// 	return true;
// }
// 
// bool PITRebarEndType::GetBendVertices(std::vector<CPoint3D>& vex, BeArcSegCR seg, CVector3D endNormal, double len) const
// {
// 	double bendRadius = seg.GetRadius();
// 	DPoint3d vec = GetptOrgin();
// 	vec.Normalize();
// 	vec.Scale(bendRadius);
// 
// 	CPoint3D ptBend, ptMid, ptEnd;
// 
// 	ptMid = GetptOrgin();
// 	ptEnd = seg.end;
// 	DPoint3d pt1 = endNormal;
// 	pt1.Scale(bendRadius + len);
// 	pt1.Add(ptMid);
// 
// 
// // 	CPoint3D ptNormal = endNormal;
// // 	ptNormal.Scale(bendRadius);
// // 	ptBend = ptMid;
// // 	ptBend.Add(ptNormal);
// 
// 	ptMid = seg.GetMid();
// 	ptBend = seg.start;
// 
// 	//		RebarVertexP vex;
// // 	CPoint3D ptCenter = endNormal;
// // 	ptCenter.Scale(bendRadius);
// // 	ptCenter.Add(ptEnd);
// // 	vex = &rebar.PopVertices().NewElement();
// // 	vex->SetIP(ptMid);
// // 	vex->SetType(RebarVertex::kIP);
// // 	vex->SetRadius(bendRadius);
// // 	vex->SetCenter(ptCenter);
// //	DEllipse3d delp = DEllipse3d::FromArcCenterStartEnd(ptCenter, ptBend, ptEnd);
// 	//		ptMid = ptNormal;
// 	//		ptMid.Scale((1 - sin(45)) * bendRadius);
// 	//		ptMid.Add(ptEnd);
// //	delp.Evaluate(ptMid, sin(45), cos(45));
// 
// // 	vex->SetArcPt(0, ptBend);
// // 	vex->SetArcPt(1, ptMid);
// // 	vex->SetArcPt(2, ptEnd);
// 
// 	vex = { pt1,ptBend,ptMid,ptEnd };
// 	return true;
// }

long PIT::PITRebarEndType::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarObject::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarObject::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

double PIT::PITRebarCode::GetBarDiameter(BrStringCR sizeKey, double rebarThread, DgnModelRefP modelRef)
{
	return RebarCode::GetBarDiameter(sizeKey,modelRef) + rebarThread;
}

double PIT::PITRebarCode::GetPinRadius(BrStringCR sizeKey, double rebarThread, DgnModelRefP modelRef, bool isStirrup)
{
	return RebarCode::GetPinRadius(sizeKey, modelRef, isStirrup) + rebarThread;
}

double PIT::PITRebarCode::GetBendLength(BrStringCR sizeKey, const PITRebarEndType & endType, DgnModelRefP modelRef)
{
	RebarEndType rebarEndType;
	rebarEndType.SetType((RebarEndType::Type)endType.GetType());
	return RebarCode::GetBendLength(sizeKey, rebarEndType, modelRef);
}

bool PIT::PITRebarCode::IsCenterLineLength()
{
	return RebarCode::IsCenterLineLength();
}
