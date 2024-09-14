/*--------------------------------------------------------------------------------------+
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
+--------------------------------------------------------------------------------------*/
#pragma once

#include "RebarDetailElement.h"

class SDK_RebarDetailer : public RebarDetailer
{
public:

    // called before ProConcrete 2d drawing. if return true, ProConcrete will not draw any 2d elements
    virtual bool IsUserDefined(BarImplementor *theBar, TDrawCommand drawCommand, int calcMode) const override;

    // always called after ProConcrete 2d drawing is processed
    virtual bool DrawUserBar(BarImplementor *theBar, TDrawCommand drawCommand, int calcMode) const override;
};