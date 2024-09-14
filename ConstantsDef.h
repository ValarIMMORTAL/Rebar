#pragma once
#include "_ustation.h"

const std::list<CString> g_listRebarDir = { _T("水平") ,_T("竖向") };

const std::list<CString> g_listFloorRebarDir = { _T("横向") ,_T("纵向") };

const std::list<CString> g_listRebarRadiateDir = { _T("环向") ,_T("径向") };

const std::list<CString> RebartypeDir = { _T("外表皮长度") ,_T("中心线长度") };

const std::list<CString> g_listSlabRebarDir = { _T("横向") ,_T("纵向") };

const std::list<CString> g_listRebarShape = { _T("圆形") ,_T("弧型") };

const std::list<CString> g_listCoverSlabRebarDir = { _T("长") ,_T("宽") };

const std::list<CString> g_listCNCutVec = { _T("正向") ,_T("反向") };

const std::list<CString> g_listRebarSize = { _T("6mm") ,_T("8mm"),_T("10mm"),_T("12mm"),_T("14mm"),_T("16mm"),_T("18mm"),_T("20mm"),_T("22mm"),_T("25mm"),_T("28mm"),_T("32mm"),_T("36mm"),_T("40mm"),_T("50mm") };

const std::list<CString> g_listRebarPose = { _T("外侧面"),_T("中间"), _T("内侧面")};

const std::list<CString> g_listRebartype = { _T("横筋"),_T("点筋") };

const std::list<CString> g_listfloorRebarPose = { _T("板底"),_T("中间"), _T("板顶") };

const std::list<CString> g_listRotateAngle = { _T("向内"), _T("向外"), _T("角度") };

const std::list<CString> g_listRebarPosition = {_T("上方"), _T("下方"), _T("左侧"), _T("右侧")};

const std::list<CString> g_listRebarLapOption = { _T("机械连接"),_T("搭接") };

const std::list<CString> g_listNoYes = { _T("否"),_T("是") };

const std::list<CString> g_listEndType = { _T("无") ,_T("弯曲"),_T("吊钩"),_T("折线"),_T("90度弯钩"),_T("135度弯钩"),_T("180度弯钩"),_T("直锚"),_T("用户") };

const std::list<CString> g_listInsertType = { _T("直锚"),_T("90度弯钩")};

const std::list<CString> g_listACRelation = { _T("同层墙") ,_T("上层墙"),_T("下层墙"),_T("上部板"),_T("下部板") };

const std::list<CString> g_listACRebarRelation = { _T("忽略") ,_T("锚入"),_T("被锚入") };

//const std::list<CString> g_listACMethod = { _T("①") ,_T("②"),_T("③"),_T("④"),_T("⑤"),_T("⑥"),_T("⑦"),_T("⑧"),_T("⑨"),_T("⑩") };
const std::list<CString> g_listACMethod = { _T("墙墙_L1") ,_T("墙墙_L2"),_T("墙墙_L3"),_T("墙墙_T4")/*_T("墙墙_十5"),_T("墙墙_G6"),_T("墙板_7"),_T("墙板_8"),_T("墙板_9"),_T("墙板_10") */};

const std::list<CString> g_listTieRebarStyle = { _T("无拉筋") ,_T("直拉"),_T("斜拉") };

const std::list<CString> g_listReinFStyle = { _T("水平1L+竖直3L") ,_T("水平1L+竖直4L"),_T("水平2L+竖直3L"),_T("水平1L+竖直4L"),_T("全部边") };

const std::list<CString> g_listRebarAnchorInType = { _T("直锚"),_T("弯锚")};

const std::list<CString> g_listStairsRebarStyle = { _T("预制") ,_T("现浇")};

const std::list<CString> g_listDownFace = { _T("矩形"),_T("圆形")};

const std::list<CString> g_listCustomizeDir = { _T("X") ,_T("Y"),_T("Z")};

const std::list<CString> g_listSlabRebarMethod = { _T("默认") ,_T("正交"),_T("放射") };

const std::list<CString> g_RebarLinkMethod = { _T("搭接连接") ,_T("机械连接") };

const std::list<CString> g_listDomeRebarDir = { _T("环向"),_T("径向") };

const std::list<CString> g_listDomeRebarDirXY = { _T("X向"),_T("Y向") };

const std::list<CString> g_listDomeRebarLayout = { _T("XY正交"),_T("环径正交") };

const std::list<CString> g_listAnchorinStyle = { _T("弯锚"), _T("直锚"), _T("直锚带端板") };
