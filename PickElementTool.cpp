#include "_ustation.h"
#include <string>
#include "PickElementTool.h"
#include "ScanIntersectTool.h"
#include "Log.h"



// PickElementTool::PickElementTool(int tool_id, PickElementTool::CompleteCallback on_complete)
PickElementTool::PickElementTool(
    PickElementTool::FilterCallback on_filter, 
    PickElementTool::CompleteCallback on_complete,
    PickElementTool::CancelCallback on_cancel)
// : DgnElementSetTool(tool_id),
: DgnElementSetTool(),
m_on_filter(on_filter),
m_on_complete(on_complete),
m_on_cancel(on_cancel),
m_last_selection_count(0),
m_required(1)
{ }

bool PickElementTool::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
{
    /// �������������ȥ�ĸ���
    if (!DgnElementSetTool::_OnPostLocate(path, cantAcceptReason))
    {
        return false;
    }
    
    if(!this->m_enable_dynamic_filter)
    {
        // ��������̬���˹��ܵĻ�����������
        return true;
    }

    EditElementHandle eh(path->GetHeadElem()->GetElementId(), path->GetRoot());
    return this->m_on_filter(eh);
}

void PickElementTool::_OnPostInstall()
{
    // ���֮ǰ��ѡ��
    SelectionSetManager::GetManager().EmptyAll();
}

void PickElementTool::_OnRestartTool()
{
    this->_ExitTool();
}

bool PickElementTool::_OnResetButton(DgnButtonEventCR ev)
{
    this->m_on_cancel();

    this->_ExitTool();

    return true;
}

bool PickElementTool::_OnDataButton(DgnButtonEventCR ev)
{
    auto required = this->_GetAdditionalLocateNumRequired();
    auto &manager = SelectionSetManager::GetManager();
    auto hit_path_ptr = this->_DoLocate(ev, true, ComponentMode::Innermost);
    if(hit_path_ptr == nullptr)
    {
        // �㵽�˿հ׵ĵط�
        if(required > 0 && manager.NumSelected() < required)
        {
            // ��û��ѡ������
            return false;
        }

        // ���ѡ��
        ElementAgenda agenda;
        manager.BuildAgenda(agenda);

        this->m_on_complete(agenda);

        this->_ExitTool();
        return true;
    }

    // ��Ҫ����ѡ��
    // ���ѡ���ĵ�һ��Ԫ��
    auto dgn_model_ptr = hit_path_ptr->GetRoot();
    auto first_ptr = hit_path_ptr->GetHeadElem();

    // ����Ƿ��Ѿ�ѡ�У����ѡ���˾�ȡ����ѡ��
    if(manager.RemoveElement(first_ptr, dgn_model_ptr) == SUCCESS)
    {
        // �ɹ�ɾ��ѡ�� ,˵���Ѿ�ѡ����
        return false;
    }

    // ����ѡ��
    EditElementHandle eeh(first_ptr, dgn_model_ptr);
    // �ж��Ƿ����ѡ��
    if(!this->m_on_filter(eeh))
    {
        return false;
    }
        
    // ����ѡ��
    // ����ѡ��
    manager.AddElement(first_ptr, dgn_model_ptr);

    // �ж��Ƿ�ﵽѡ�������
    if(required > 0 && manager.NumSelected() == required)
    {
        // �ﵽ�����ˣ�����ѡ��
        ElementAgenda agenda;
        manager.BuildAgenda(agenda);

        this->m_on_complete(agenda);

        this->_ExitTool();
        return true;
    }

    return false;
}
