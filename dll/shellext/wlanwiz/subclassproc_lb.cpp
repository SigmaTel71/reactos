/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN ListBox Subclassed Procedures)
 * COPYRIGHT:   Copyright 2024-2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

LRESULT
CWlanWizard::OnPaintLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HBRUSH hbrWhite = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    PAINTSTRUCT psLB = { 0 };
    HDC hDC = m_ListboxWLAN.BeginPaint(&psLB);

    RECT rc = { 0 };
    m_ListboxWLAN.GetClientRect(&rc);

    if (!m_ListboxWLAN.IsWindowEnabled())
    {
        ATL::CStringW cswWindowText = L"";
        FillRect(hDC, &rc, hbrWhite);

        /* This will help aligning error or suggestion messages inside listbox area */
        RECT rCalc = rc;
        rc.top = rc.bottom / 2;

        /* Painted text changes depending on scan status or result */
        if(m_uScanStatus == STATUS_SCANNING)
            cswWindowText.LoadStringW(IDS_WLANWIZ_SCANNING_NETWORKS);
        else
        {
            if(!m_lstWlanNetworks || m_lstWlanNetworks->dwNumberOfItems == 0)
                cswWindowText.LoadStringW(IDS_WLANWIZ_NO_APS_DISCOVERED);
        }

        m_lfCaption.lfUnderline = FALSE;

        HFONT hCaptionFont = CreateFontIndirectW(&m_lfCaption);
        HGDIOBJ hOld = SelectObject(hDC, hCaptionFont);

        rc.top -= DrawTextW(hDC, cswWindowText, -1, &rCalc, DT_BOTTOM | DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
        DrawTextW(hDC, cswWindowText, -1, &rc, DT_BOTTOM | DT_CENTER | DT_WORDBREAK);
        SelectObject(hDC, hOld);
        DeleteObject(hCaptionFont);
    }
    else
    {
        m_ListboxWLAN.DefWindowProcW(nMsg, reinterpret_cast<WPARAM>(hDC), lParam);
    }

    m_ListboxWLAN.EndPaint(&psLB);
    DeleteObject(hbrWhite);
	
    return FALSE;
}

LRESULT
CWlanWizard::OnLButtonDownLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    bool bClickedOutside;
    int idx = (int)m_ListboxWLAN.SendMessageW(LB_ITEMFROMPOINT, 0, lParam);

    if (idx >= 0)
    {
        RECT rcItem;

        LRESULT res = m_ListboxWLAN.SendMessageW(LB_GETITEMRECT, idx, (LPARAM)&rcItem);
        if (res == LB_ERR || rcItem.bottom - rcItem.top == 56)
            goto pass;

        /* Calculate the collision box for emulated checkbox. */
        AutoconnectCheckboxCollision(&rcItem);
    }
pass:
    bHandled = FALSE;
    return 0;
}
