#include "_ustation.h"
#include "PITRebarAssembly.h"

RebarAssembly* PIT::PITRebarAssembly::GetRebarAssembly(ElementId concreteid, string assemblyname)
{
	RebarAssemblies area;
	REA::GetRebarAssemblies(concreteid, area);
	for (int i = 0; i < area.GetSize(); i++)
	{
		RebarAssembly* rebaras = area.GetAt(i);
		string tesname = typeid(*rebaras).name();
		if (tesname == assemblyname)
		{
			return rebaras;
		}
	}
	return nullptr;
}

void PIT::PITRebarAssembly::DeleteRebarsFromAssembly(ElementId concreteid, string assemblyname)
{
	RebarAssembly* rebaras = GetRebarAssembly(concreteid, assemblyname);

	if (rebaras != nullptr)//墙本来有钢筋就删除，避免重复生成
	{
		RebarSets rebar_sets;
		rebaras->GetRebarSets(rebar_sets, ACTIVEMODEL);
		for (int i = 0; i < rebar_sets.GetSize(); i++)
		{
			RebarSetP rebarSet = &rebar_sets.At(i);
			ElementId id = (int)rebarSet->GetElementId();
			EditElementHandle editEh(id, ACTIVEMODEL);
			editEh.DeleteFromModel();
		}
	}
}