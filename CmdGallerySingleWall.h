#pragma once

#include <Bentley/Bentley.h>
#include "GallerySettings.h"
#include "GallerySettings.h"

#define BEGIN_NAMESPACE_GALLERY_SINGLE_WALL namespace Gallery { namespace SingleWall {
#define END_NAMESPACE_GALLERY_SINGLE_WALL }}

BEGIN_NAMESPACE_GALLERY_SINGLE_WALL

// 廊道墙单独配筋
void cmd(WCharCP);

// @brief 执行墙配筋
// @details 该函数读取存储在墙中的gallery settings中
// @param wall 要配筋的墙
void execute_make_rebar(EditElementHandleR wall);

using namespace Gallery;
/// 计算得到符合条件的孔洞集合 useHoleehs
/// 当孔洞大小大于钢筋间距时即符合条件
void CalculateMyUseHoles(vector<EditElementHandle*> &useHoleehs, vector<EditElementHandle*> Holeehs, Gallery::GallerySettings gallery_settings, DgnModelRefP modelRef);
// @brief 联合配筋中墙配筋入口
// @details 再函数外处理gallery settings数据然后再传入配筋
// @param wall 要配筋的墙
void execute_make_rebar(EditElementHandleR wall, GallerySettings &gallery_settings);

END_NAMESPACE_GALLERY_SINGLE_WALL
