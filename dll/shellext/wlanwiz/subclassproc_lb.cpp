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
    PAINTSTRUCT psLB = { 0 };
    HDC hDC = m_ListboxWLAN.BeginPaint(&psLB);

    if (!m_ListboxWLAN.IsWindowEnabled())
    {
        ATL::CStringW cswWindowText = L"";
        RECT rc = { 0 };
        HBRUSH hbrWhite = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));

        m_ListboxWLAN.GetClientRect(&rc);
        FillRect(hDC, &rc, hbrWhite);
        DeleteObject(hbrWhite);

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
	
    return FALSE;
}

LRESULT
CWlanWizard::OnMouseMoveLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!m_lstWlanNetworks->dwNumberOfItems || !IsThemeActive())
    {
        bHandled = TRUE;
        return 0;
    }

    RECT rcCurrentItem;
    LRESULT itemID = m_ListboxWLAN.SendMessageW(LB_GETCURSEL);

    if (itemID == LB_ERR)
    {
        bHandled = TRUE;
        return 0;
    }

    m_ListboxWLAN.SendMessageW(LB_GETITEMRECT, itemID, (LPARAM)&rcCurrentItem);

    POINT ps;
    GetCursorPos(&ps);
    m_ListboxWLAN.ScreenToClient(&ps);

    LONG itemHeight = rcCurrentItem.bottom - rcCurrentItem.top;

    if (itemHeight == 56)
    {
        bHandled = TRUE;
        return 0;
    }

    bool bLastHot = m_lastCBS == CBS_UNCHECKEDHOT || m_lastCBS == CBS_CHECKEDHOT;
    bool bLastNormal = m_lastCBS == CBS_CHECKEDNORMAL || m_lastCBS == CBS_UNCHECKEDNORMAL;

    m_bMouseOverAutoconnect = (ps.x >= m_rcCbCollision.left && ps.x <= m_rcCbCollision.right) &&
                              (ps.y >= m_rcCbCollision.top && ps.y <= m_rcCbCollision.bottom);

    if ((!m_bMouseOverAutoconnect && bLastHot) || (m_bMouseOverAutoconnect && bLastNormal))
        m_ListboxWLAN.InvalidateRect(&m_rcCheckbox, FALSE);

    return 0;
}

LRESULT
CWlanWizard::OnMouseWheelLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (m_PasswordWnd == GetFocus())
        return 0;

    if (m_PasswordWnd.IsWindowVisible())
        m_PasswordWnd.ShowWindow(SW_HIDE);

    return m_ListboxWLAN.DefWindowProcW(nMsg, wParam, lParam);
}

LRESULT
CWlanWizard::OnLButtonDownLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT itemID = m_ListboxWLAN.SendMessageW(LB_GETCURSEL);

    if (itemID == LB_ERR)
    {
        bHandled = FALSE;
        return 0;
    }

    PLB_ITEMDATA plbItemData = reinterpret_cast<PLB_ITEMDATA>(m_ListboxWLAN.SendMessageW(LB_GETITEMDATA, itemID));

    if (m_bMouseOverAutoconnect)
        plbItemData->bShouldAutoconnect = !plbItemData->bShouldAutoconnect;

    bHandled = FALSE;
    return 0;
}

LRESULT
CWlanWizard::OnEditChangeLB(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    if (!m_PasswordWnd.IsWindowVisible())
        return 0;

    int charCount = m_PasswordWnd.SendMessageW(WM_GETTEXTLENGTH);

    /* For WPA networks the minimum password length is 8 characters */
    if (charCount < 8 && m_ConnectButton.IsWindowEnabled())
    {
        m_ConnectButton.EnableWindow(FALSE);
    }
    else if (charCount >= 8 && !m_ConnectButton.IsWindowEnabled())
    {
        m_ConnectButton.EnableWindow();
    }

    return 0;
}
