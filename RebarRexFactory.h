#pragma once 

#include <RebarElements.h>

class RebarRexFactory : public RexFactory {
public:
	virtual RebarExtendedElementP Make(BrStringCR rex_name) const override;
};