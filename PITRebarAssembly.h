#pragma once
#include "_ustation.h"
#include <SelectionRebar.h>

namespace PIT
{
	class PITRebarAssembly :public RebarAssembly
	{
	public:
		PITRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :RebarAssembly(id, modelRef) {}
		virtual ~PITRebarAssembly() {}
		/*
		* @description: �ӻ����������л�ȡ����Ϊassemblyname��rebarassemblyָ��
		* @param	concreteid		IN		����������ID
		* @param	assemblyname	IN		�����ַ���(Example:	class PIT::ACCSTWallRebarAssembly)
		*
		* @return	����������
		* @author	LiuXiang
		* @Time		2021/04/21
		*/
		static RebarAssembly* GetRebarAssembly(ElementId concreteid, string assemblyname);

		static void DeleteRebarsFromAssembly(ElementId concreteid, string assemblyname);
	};
}
