#pragma once

#include <DgnView/DgnElementSetTool.h>
#include <functional>
#include <vector>

using namespace Bentley;
using namespace Bentley::DgnPlatform;

/// @brief ��ѡԪ�صĹ���
/// @details ���Ԫ�ؼ�ѡ������հ״��ύѡ���Ҽ�ȡ��ѡ��
class PickElementTool : public DgnElementSetTool {
private:
    using CompleteCallback = std::function<void(const ElementAgenda &)>;
    using FilterCallback = std::function<bool(EditElementHandleCR)>;
    using CancelCallback = std::function<void(void)>;

public:
    // PickElementTool(int tool_id, CompleteCallback on_complete);
    /// @brief ����һ��ѡ�񹤾�
    /// @param on_filter ���ڹ���Ԫ�أ�����true��Ԫ�ر�ʾ����ѡ��
    /// @param on_complete ���ѡ��ʱ����
    PickElementTool(FilterCallback on_filter, CompleteCallback on_complete, CancelCallback on_cancel);

protected:
    virtual bool _DoGroups () override { return false; }
    virtual bool _WantAccuSnap () override { return false; }
    virtual bool _WantDynamics () override { return false; }
    virtual UsesSelection _AllowSelection () override { return USES_SS_Check; }
    virtual UsesDragSelect _AllowDragSelect () override { return USES_DRAGSELECT_Box; }
    // ����true��ʾ����ѡ���������հ�λ���ύ
    virtual bool _NeedAcceptPoint () override {
        auto elm_src = this->_GetElemSource();
        return SOURCE_Pick == this->_GetElemSource();
    }
    // virtual bool _NeedAcceptPoint () override { return true; }
    virtual bool _NeedPointForSelection () override { return true; }
    virtual bool _OnModifierKeyTransition (bool wentDown, int key) override { return this->OnModifierKeyTransitionHelper(wentDown, key); }
    virtual bool _WantAdditionalLocate(DgnButtonEventCP ev) override { return true; }
    virtual size_t _GetAdditionalLocateNumRequired() override { return this->m_required; } 
    // ѡ������е�Ԫ�ر����������
    virtual bool  _IsModifyOriginal () override { return false; }
    
    virtual StatusInt _OnElementModify(EditElementHandleR) { return ERROR; }
    virtual bool _OnPostLocate (HitPathCP path, WStringR cantAcceptReason) override;
    virtual void _OnPostInstall() override;
    // virtual void _SetupAndPromptForNextAction () override;
    virtual void _OnRestartTool () override;
    virtual bool _OnResetButton(DgnButtonEventCR ev) override;
    virtual bool _OnDataButton(DgnButtonEventCR ev) override;
public:
    /// @brief ���ÿ�ѡԪ�ص�������Ҫ��Ĭ��Ϊ0����ʾ����
    /// @param count 
    void set_required(size_t count) {
        this->m_required = count;
    }

    /// @brief ������̬����
    /// @details ����������������Ƶ�element��ʱ�ͻ�ִ��on_filter���������ܻᵼ����������, Ĭ�Ͽ���
    void dynamic_filter(bool enabled) {
        this->m_enable_dynamic_filter = enabled;        
    }

private:
    FilterCallback m_on_filter;
    CompleteCallback m_on_complete;
    CancelCallback m_on_cancel;
    /// @brief �ϴ�ѡ�������
    size_t m_last_selection_count;
    /// @brief ������Ҫѡ������������Ϊ1���ǵ�ѡ�� 0Ϊ����ѡ
    size_t m_required;
    /// @brief �Ƿ�����̬����
    bool m_enable_dynamic_filter;
};
