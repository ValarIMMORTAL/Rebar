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
    /// 这里是鼠标移上去的高亮
    if (!DgnElementSetTool::_OnPostLocate(path, cantAcceptReason))
    {
        return false;
    }
    
    if(!this->m_enable_dynamic_filter)
    {
        // 不开启动态过滤功能的话，允许所有
        return true;
    }

    EditElementHandle eh(path->GetHeadElem()->GetElementId(), path->GetRoot());
    return this->m_on_filter(eh);
}

void PickElementTool::_OnPostInstall()
{
    // 清除之前的选择
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
        // 点到了空白的地方
        if(required > 0 && manager.NumSelected() < required)
        {
            // 还没有选够数量
            return false;
        }

        // 完成选择
        ElementAgenda agenda;
        manager.BuildAgenda(agenda);

        this->m_on_complete(agenda);

        this->_ExitTool();
        return true;
    }

    // 需要继续选择
    // 获得选到的第一个元素
    auto dgn_model_ptr = hit_path_ptr->GetRoot();
    auto first_ptr = hit_path_ptr->GetHeadElem();

    // 检查是否已经选中，如果选中了就取消掉选择
    if(manager.RemoveElement(first_ptr, dgn_model_ptr) == SUCCESS)
    {
        // 成功删除选择 ,说明已经选过了
        return false;
    }

    // 加入选择集
    EditElementHandle eeh(first_ptr, dgn_model_ptr);
    // 判断是否可以选择
    if(!this->m_on_filter(eeh))
    {
        return false;
    }
        
    // 可以选择
    // 加入选择集
    manager.AddElement(first_ptr, dgn_model_ptr);

    // 判断是否达到选择的上限
    if(required > 0 && manager.NumSelected() == required)
    {
        // 达到上限了，结束选择
        ElementAgenda agenda;
        manager.BuildAgenda(agenda);

        this->m_on_complete(agenda);

        this->_ExitTool();
        return true;
    }

    return false;
}
