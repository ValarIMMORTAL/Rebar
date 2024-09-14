#pragma once

#include <Bentley/Bentley.h>
#include "GallerySettings.h"
#include "GallerySettings.h"

#define BEGIN_NAMESPACE_GALLERY_SINGLE_WALL namespace Gallery { namespace SingleWall {
#define END_NAMESPACE_GALLERY_SINGLE_WALL }}

BEGIN_NAMESPACE_GALLERY_SINGLE_WALL

// �ȵ�ǽ�������
void cmd(WCharCP);

// @brief ִ��ǽ���
// @details �ú�����ȡ�洢��ǽ�е�gallery settings��
// @param wall Ҫ����ǽ
void execute_make_rebar(EditElementHandleR wall);

using namespace Gallery;
/// ����õ����������Ŀ׶����� useHoleehs
/// ���׶���С���ڸֽ���ʱ����������
void CalculateMyUseHoles(vector<EditElementHandle*> &useHoleehs, vector<EditElementHandle*> Holeehs, Gallery::GallerySettings gallery_settings, DgnModelRefP modelRef);
// @brief ���������ǽ������
// @details �ٺ����⴦��gallery settings����Ȼ���ٴ������
// @param wall Ҫ����ǽ
void execute_make_rebar(EditElementHandleR wall, GallerySettings &gallery_settings);

END_NAMESPACE_GALLERY_SINGLE_WALL
