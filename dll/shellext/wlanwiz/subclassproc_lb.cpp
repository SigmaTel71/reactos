/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN ListBox Subclassed Procedures)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

LRESULT CWlanWizard::OnPaintLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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
        if(this->uScanStatus == STATUS_SCANNING)
            cswWindowText.LoadStringW(IDS_WLANWIZ_SCANNING_NETWORKS);
        else
        {
            if(!this->lstWlanNetworks || this->lstWlanNetworks->dwNumberOfItems == 0)
                cswWindowText.LoadStringW(IDS_WLANWIZ_NO_APS_DISCOVERED);
        }

        this->lfCaption.lfUnderline = FALSE;

        HFONT hCaptionFont = CreateFontIndirectW(&this->lfCaption);
        HGDIOBJ hOld = SelectObject(hDC, hCaptionFont);

        rc.top -= DrawTextW(hDC, cswWindowText, -1, &rCalc, DT_BOTTOM | DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
        DrawTextW(hDC, cswWindowText, -1, &rc, DT_BOTTOM | DT_CENTER | DT_WORDBREAK);
        SelectObject(hDC, hOld);
        DeleteObject(hCaptionFont);
    }
    else
    {
        HDC hDCMem = CreateCompatibleDC(hDC);
        HBITMAP hbmMem = CreateCompatibleBitmap(hDC, rc.right - rc.left, rc.bottom - rc.top);

        HGDIOBJ hOld = SelectObject(hDCMem, hbmMem);

        FillRect(hDCMem, &rc, hbrWhite);

        m_ListboxWLAN.DefWindowProcW(nMsg, reinterpret_cast<WPARAM>(hDCMem), lParam);
        
        BitBlt(hDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, hDCMem, 0, 0, SRCCOPY);

        SelectObject(hDCMem, hOld);
        DeleteObject(hbmMem);
        DeleteDC(hDCMem);
    }

    m_ListboxWLAN.EndPaint(&psLB);
    DeleteObject(hbrWhite);
	
    return FALSE;
}