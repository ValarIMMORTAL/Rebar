#pragma once
#include <RebarElements.h>

namespace PIT
{
	class PITRebar :public RebarElement
	{
	public:
		virtual 	ElementId DrawRefLine(DgnModelRefP modelRef) { return 0; }
		virtual		RebarElement*  Create(RebarSetR rebarSet) { return nullptr; }
	};
}
class RebarMakerFactory
{
public:
	virtual RebarSetTag* MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL) = 0;
};

