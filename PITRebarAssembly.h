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
		* @description: 从混凝土假体中获取类名为assemblyname的rebarassembly指针
		* @param	concreteid		IN		混凝土假体ID
		* @param	assemblyname	IN		类名字符串(Example:	class PIT::ACCSTWallRebarAssembly)
		*
		* @return	待配筋构件类型
		* @author	LiuXiang
		* @Time		2021/04/21
		*/
		static RebarAssembly* GetRebarAssembly(ElementId concreteid, string assemblyname);

		static void DeleteRebarsFromAssembly(ElementId concreteid, string assemblyname);
	};
}
