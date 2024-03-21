/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (Sidebar GroupBox Subclassed Procedures)
 * COPYRIGHT:   Copyright 2024-2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

LRESULT
CWlanWizard::OnEraseBkgndGroupBoxBtns(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return FALSE;
}

LRESULT
CWlanWizard::OnPaintGroupBox(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int ctrlIDs[2] = { IDC_WLANWIZ_MAINGROUP, IDC_WLANWIZ_RELATEDGROUP };

    for (UINT i = 0; i < _countof(ctrlIDs); ++i)
    {
        PAINTSTRUCT ps = { 0 };
        RECT rc = { 0 };

        ATL::CWindow hWnd = GetDlgItem(ctrlIDs[i]);
        HDC hDC = hWnd.BeginPaint(&ps);

        hWnd.GetClientRect(&rc);

        if (m_hThemeEB)
        {
            /* Theme the sidebar groupbox controls */
            RECT rcGroupBoxHeader = { 0 };
            RECT rcGroupBoxBg = { 0 };
            MARGINS mHeader = { 0 };
            LOGFONTW lfGroupBoxCaption = { 0 };

            GetThemeMargins(m_hThemeEB, hDC, EBP_NORMALGROUPHEAD, 0, TMT_CONTENTMARGINS, &rc, &mHeader);
            GetThemeFont(m_hThemeEB, hDC, EBP_NORMALGROUPHEAD, 0, TMT_FONT, &lfGroupBoxCaption);

            rcGroupBoxHeader = rc;
            rcGroupBoxHeader.bottom = -lfGroupBoxCaption.lfHeight + mHeader.cyBottomHeight + mHeader.cyTopHeight + rcGroupBoxHeader.top;

            DrawThemeBackground(m_hThemeEB, hDC, EBP_NORMALGROUPHEAD, 0, &rcGroupBoxHeader, NULL);
            DrawThemeEdge(m_hThemeEB, hDC, EBP_NORMALGROUPHEAD, 0, &rc, 0, BF_ADJUST, &rc);

            ATL::CStringW cswGBCaption;
            hWnd.GetWindowTextW(cswGBCaption);

            /* Add content margins to text rectangle */
            rcGroupBoxHeader.top += mHeader.cyTopHeight / 2;
            rcGroupBoxHeader.right -= mHeader.cxRightWidth;
            rcGroupBoxHeader.left += mHeader.cxLeftWidth;

            DrawThemeText(m_hThemeEB, hDC, EBP_NORMALGROUPHEAD, 0, cswGBCaption, cswGBCaption.GetLength(), DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER, 0, &rcGroupBoxHeader);

            /* Draw groupbox content */
            rcGroupBoxBg = rc;
            rcGroupBoxBg.top = rcGroupBoxHeader.bottom;

            DrawThemeBackground(m_hThemeEB, hDC, EBP_NORMALGROUPBACKGROUND, 0, &rcGroupBoxBg, NULL);
            DrawThemeEdge(m_hThemeEB, hDC, EBP_NORMALGROUPBACKGROUND, 0, &rcGroupBoxBg, 0, BF_ADJUST, &rc);
        }
        else
        {
            /* Draw classic GroupBox borders */
            FillRect(hDC, &rc, reinterpret_cast<HBRUSH>(COLOR_WINDOW));
            
            switch (ctrlIDs[i])
            {
                case IDC_WLANWIZ_MAINGROUP:
                    m_SidebarGroupMain.DefWindowProcW(nMsg, reinterpret_cast<WPARAM>(hDC), lParam);
                    break;
                
                case IDC_WLANWIZ_RELATEDGROUP:
                    m_SidebarGroupRelated.DefWindowProcW(nMsg, reinterpret_cast<WPARAM>(hDC), lParam);
                    break;
            }
        }

        hWnd.EndPaint(&ps);
    }
    
    m_SidebarButtonPA.InvalidateRect(NULL);
    m_SidebarButtonAS.InvalidateRect(NULL);
    m_SidebarButtonSN.InvalidateRect(NULL);
    m_SidebarButtonIW.InvalidateRect(NULL);

    return FALSE;
}
