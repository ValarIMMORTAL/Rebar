#include "_ustation.h"
#include "CmdGalleryWallAndSlab.h"
#include "CmdGallerySingleWall.h"
#include "WallHelper.h"
#include "SlabHelper.h"
#include "GallerySettingsDialog.h"

NAMESPACE_GALLERY_WALL_AND_SLAB_BEGIN

void cmd(WCharCP){
    // 显示配筋设置窗口
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    auto dialog = new Gallery::SettingsDialog(CWnd::FromHandle(MSWIND));
    dialog->on_ok = [=](const GallerySettings &settings, const ElementAgenda &agenda)
    {
        // 该函数在执行前已经将配筋参数写入到墙板中
        // 直接使用墙板中存储的配筋参数生成钢筋即可
        for(auto &entry : agenda)
        {
            auto &elem = const_cast<EditElementHandleR>(static_cast<EditElementHandleCR>(entry));
            if(WallHelper::is_wall(elem))
            {
                // 执行墙配筋
                SingleWall::execute_make_rebar(elem);
            }
            else if(SlabHelper::is_slab(elem))
            {
                // 执行板配筋
                // todo
            }
        }

        // 配筋结束后关闭对话框
        dialog->SendMessage(WM_CLOSE);
        delete dialog;
    };
    dialog->Create(IDD_DIALOG_Gallery_Settings);
    dialog->ShowWindow(SW_SHOW);
    // 隐藏“选择模型”按钮
    dialog->GetDlgItem(IDC_SELECT_MODEL)->ShowWindow(SW_HIDE);
}

NAMESPACE_GALLERY_WALL_AND_SLAB_END