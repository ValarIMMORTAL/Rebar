#pragma once

//�ֽ�xml�ļ���Ϣ
struct RebarXmlInfo
{
	WChar xmlName[MAX_LEVEL_NAME_LENGTH];
};
/**
* brief ������ֵ����
* return �޷���ֵ
*/
void savePathResultDlgParams(void);

/**
* @��������  void
* @����˵��  ���ز���ֵ
* @��������  void
**/
void loadPathResultDlgParams(void);

