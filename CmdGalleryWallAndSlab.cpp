#include "_ustation.h"
#include "CmdGalleryWallAndSlab.h"
#include "CmdGallerySingleWall.h"
#include "WallHelper.h"
#include "SlabHelper.h"
#include "GallerySettingsDialog.h"

NAMESPACE_GALLERY_WALL_AND_SLAB_BEGIN

void cmd(WCharCP){
    // ��ʾ������ô���
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    auto dialog = new Gallery::SettingsDialog(CWnd::FromHandle(MSWIND));
    dialog->on_ok = [=](const GallerySettings &settings, const ElementAgenda &agenda)
    {
        // �ú�����ִ��ǰ�Ѿ���������д�뵽ǽ����
        // ֱ��ʹ��ǽ���д洢�����������ɸֽ��
        for(auto &entry : agenda)
        {
            auto &elem = const_cast<EditElementHandleR>(static_cast<EditElementHandleCR>(entry));
            if(WallHelper::is_wall(elem))
            {
                // ִ��ǽ���
                SingleWall::execute_make_rebar(elem);
            }
            else if(SlabHelper::is_slab(elem))
            {
                // ִ�а����
                // todo
            }
        }

        // ��������رնԻ���
        dialog->SendMessage(WM_CLOSE);
        delete dialog;
    };
    dialog->Create(IDD_DIALOG_Gallery_Settings);
    dialog->ShowWindow(SW_SHOW);
    // ���ء�ѡ��ģ�͡���ť
    dialog->GetDlgItem(IDC_SELECT_MODEL)->ShowWindow(SW_HIDE);
}

NAMESPACE_GALLERY_WALL_AND_SLAB_END