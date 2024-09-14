#pragma once

#include <DgnView/DgnElementSetTool.h>
#include <functional>
#include <vector>

using namespace Bentley;
using namespace Bentley::DgnPlatform;

/// @brief 点选元素的工具
/// @details 点击元素加选，点击空白处提交选择，右键取消选择
class PickElementTool : public DgnElementSetTool {
private:
    using CompleteCallback = std::function<void(const ElementAgenda &)>;
    using FilterCallback = std::function<bool(EditElementHandleCR)>;
    using CancelCallback = std::function<void(void)>;

public:
    // PickElementTool(int tool_id, CompleteCallback on_complete);
    /// @brief 创建一个选择工具
    /// @param on_filter 用于过滤元素，返回true的元素表示可以选择
    /// @param on_complete 完成选择时调用
    PickElementTool(FilterCallback on_filter, CompleteCallback on_complete, CancelCallback on_cancel);

protected:
    virtual bool _DoGroups () override { return false; }
    virtual bool _WantAccuSnap () override { return false; }
    virtual bool _WantDynamics () override { return false; }
    virtual UsesSelection _AllowSelection () override { return USES_SS_Check; }
    virtual UsesDragSelect _AllowDragSelect () override { return USES_DRAGSELECT_Box; }
    // 返回true表示：多选结束后点击空白位置提交
    virtual bool _NeedAcceptPoint () override {
        auto elm_src = this->_GetElemSource();
        return SOURCE_Pick == this->_GetElemSource();
    }
    // virtual bool _NeedAcceptPoint () override { return true; }
    virtual bool _NeedPointForSelection () override { return true; }
    virtual bool _OnModifierKeyTransition (bool wentDown, int key) override { return this->OnModifierKeyTransitionHelper(wentDown, key); }
    virtual bool _WantAdditionalLocate(DgnButtonEventCP ev) override { return true; }
    virtual size_t _GetAdditionalLocateNumRequired() override { return this->m_required; } 
    // 选择参照中的元素必须重载这个
    virtual bool  _IsModifyOriginal () override { return false; }
    
    virtual StatusInt _OnElementModify(EditElementHandleR) { return ERROR; }
    virtual bool _OnPostLocate (HitPathCP path, WStringR cantAcceptReason) override;
    virtual void _OnPostInstall() override;
    // virtual void _SetupAndPromptForNextAction () override;
    virtual void _OnRestartTool () override;
    virtual bool _OnResetButton(DgnButtonEventCR ev) override;
    virtual bool _OnDataButton(DgnButtonEventCR ev) override;
public:
    /// @brief 设置可选元素的最少需要，默认为0，表示无限
    /// @param count 
    void set_required(size_t count) {
        this->m_required = count;
    }

    /// @brief 开启动态过滤
    /// @details 如果开启这个，鼠标移到element上时就会执行on_filter函数，可能会导致性能问题, 默认开启
    void dynamic_filter(bool enabled) {
        this->m_enable_dynamic_filter = enabled;        
    }

private:
    FilterCallback m_on_filter;
    CompleteCallback m_on_complete;
    CancelCallback m_on_cancel;
    /// @brief 上次选择的数量
    size_t m_last_selection_count;
    /// @brief 最少需要选的数量，设置为1就是单选， 0为无限选
    size_t m_required;
    /// @brief 是否开启动态过滤
    bool m_enable_dynamic_filter;
};
