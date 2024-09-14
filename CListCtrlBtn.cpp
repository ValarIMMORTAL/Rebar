#include "_ustation.h"
#include "CListCtrlBtn.h"

IMPLEMENT_DYNAMIC(CListCtrlBtn, CButton)
CListCtrlBtn::CListCtrlBtn()
{
}

CListCtrlBtn::CListCtrlBtn(int type, int nItem, int nSubItem, CRect rect, HWND hParent)    //-
{

	m_type = type;
	m_inItem = nItem;
	m_inSubItem = nSubItem;
	m_rect = rect;
	m_hParent = hParent;
	bEnable = TRUE;
}

CListCtrlBtn::~CListCtrlBtn()
{
}

BEGIN_MESSAGE_MAP(CListCtrlBtn, CButton)
	ON_CONTROL_REFLECT(BN_CLICKED, &CListCtrlBtn::OnBnClicked)
END_MESSAGE_MAP()

void CListCtrlBtn::OnBnClicked()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_type == 0)//表示端点属性
		::SendMessage(m_hParent, WM_ENDPROP_CLICK, m_inItem, m_inSubItem);
	if (m_type == 1)//表示清除
		::SendMessage(m_hParent, WM_CLEAR_CLICK, m_inItem, m_inSubItem);
}
void CListCtrlBtn::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	ASSERT(lpDrawItemStruct);

	COLORREF crText = m_bSelected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_WINDOWTEXT);
	COLORREF crTextBkgrnd = m_bSelected ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW);

	CString sCaption;
	CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);	// get device context
	RECT r = lpDrawItemStruct->rcItem;					// context rectangle
	int cx = r.right - r.left;						// get width
	int cy = r.bottom - r.top;						// get height
	// get text box position
	RECT tr = { r.left + 2, r.top, r.right - 2, r.bottom };

	GetWindowText(sCaption);							// get button text
	pDC->SetBkMode(TRANSPARENT);

	// Select the correct skin 
	if (lpDrawItemStruct->itemState & ODS_DISABLED)//鼠标禁用
	{
		// no skin selected for disabled state -> standard button 
		pDC->FillSolidRect(&r, GetSysColor(COLOR_BTNFACE));

		pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));//)m_TextColor灰色字体  文字偏移
		OffsetRect(&tr, -1, -1);
		pDC->DrawText(sCaption, &tr, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
	}
	else
	{										// SELECTED (DOWN) BUTTON
		if ((lpDrawItemStruct->itemState & ODS_SELECTED))//当按下按钮时的处理
		{
			// no skin selected for selected state -> standard buttonRGB(255,152,0)
			pDC->FillSolidRect(&r, RGB(39, 169, 241));//GetSysColor(COLOR_BTNFACE)

		}
		else
		{											// DEFAULT BUTTON
			pDC->FillSolidRect(&r, GetSysColor(COLOR_BTNFACE));//
		}
		// paint the enabled button text
		//pDC->SetTextColor(RGB(255,255,255));
		CFont nFont, *nOldFont;
		nFont.CreatePointFont(90, _T("微软雅黑"));//创建字体 
		nOldFont = pDC->SelectObject(&nFont);

		DrawText(lpDrawItemStruct->hDC, sCaption, sCaption.GetLength(), &tr, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
		pDC->SelectStockObject(SYSTEM_FONT);
		//pDC->DrawText(sCaption, &tr, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
	}
}