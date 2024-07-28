/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (Drawing Routines)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

LRESULT CWlanWizard::OnMeasureItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PMEASUREITEMSTRUCT pmis = reinterpret_cast<PMEASUREITEMSTRUCT>(lParam);
    switch (pmis->CtlID)
    {
        case IDC_WLANWIZ_LISTBOX:
            pmis->itemHeight = 56;
            break;
    }
    return FALSE;
}

LRESULT CWlanWizard::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps = { 0 };
    HDC hDC = BeginPaint(&ps);

    /* Paint the sidebar, if we have theming enabled */
    if (this->hTheme)
    {
        RECT rcEtchVert = { 0 };
        ATL::CWindow hEtchVert = GetDlgItem(IDC_WLANWIZ_ETCHEDVERT);
        hEtchVert.GetClientRect(&rcEtchVert);
        hEtchVert.MapWindowPoints(hEtchVert.GetParent(), &rcEtchVert);

        rcEtchVert.left = 0;
        rcEtchVert.top = 0;

        DrawThemeBackground(this->hTheme, hDC, 0, 0, &rcEtchVert, NULL);
    }

    EndPaint(&ps);
    return FALSE;
}


LRESULT CWlanWizard::OnDrawItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PDRAWITEMSTRUCT pdis = reinterpret_cast<PDRAWITEMSTRUCT>(lParam);

    switch (pdis->itemAction)
    {
        case ODA_FOCUS:
        case ODA_SELECT:
        case ODA_DRAWENTIRE:
        {
            ATL::CStringW cswWindowText;
            SetBkMode(pdis->hDC, TRANSPARENT);

            if (pdis->CtlID != IDC_WLANWIZ_LISTBOX)
            {
                /* Bounding box for button text */
                RECT rcBtnText = pdis->rcItem;
                rcBtnText.left += 24;
                rcBtnText.top += 3;

                /* Draw button caption and icon on other DC when themed */
                HBITMAP hbmBtn = CreateCompatibleBitmap(pdis->hDC, pdis->rcItem.right - pdis->rcItem.left, pdis->rcItem.bottom - pdis->rcItem.top);
                HDC hDCBtn = CreateCompatibleDC(pdis->hDC);
                HGDIOBJ hOld = INVALID_HANDLE_VALUE;

                GetDlgItemTextW(static_cast<int>(wParam), cswWindowText);

                /* Step 1: Prepare drawing surface */
                HBRUSH hbrRect = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
                
                if (this->hTheme)
                {
                    SetBkMode(hDCBtn, TRANSPARENT);
                    SelectObject(hDCBtn, hbmBtn);

                    COLORREF crFill = 0;
                    GetThemeColor(this->hTheme, EBP_NORMALGROUPBACKGROUND, 0, TMT_FILLCOLOR, &crFill);
                    hbrRect = CreateSolidBrush(crFill);
                }

                FillRect(!this->hTheme ? pdis->hDC : hDCBtn,
                         &pdis->rcItem,
                         hbrRect);

                DeleteObject(hbrRect);

                /* Step 2: Draw related icon to the sidebar button
                 * The +20 offset originates from resource.h, the IDs are laid out in a way
                 * they can be safely batch processed in loops, as the range is fixed. */
                if (wParam >= IDC_WLANWIZ_SCAN_NETWORKS && wParam <= IDC_WLANWIZ_ADVANCED_SETTINGS)
                    DrawIconEx(this->hTheme ? hDCBtn : pdis->hDC,
                               pdis->rcItem.left + 2,
                               pdis->rcItem.top + 2,
                               LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCEW(wParam + 20)),
                               16,
                               16,
                               NULL,
                               NULL,
                               DI_NORMAL);

                /* Step 3: Draw button text */
                HFONT hfBtnCaption = NULL;
                LOGFONTW lfBtnCaption = { 0 };
                
                if (this->hTheme)
                {
                    COLORREF crBtnColor = 0;
                    /* According to msstyles, the items do not have transparent background,
                     * so they have been filled by color to blend in. Same as in wzcdlg. */
                    GetThemeColor(this->hTheme, EBP_NORMALGROUPBACKGROUND, 0, TMT_TEXTCOLOR, &crBtnColor);

                    /* We can't use DrawThemeText directly as it's not aware of underline style? */
                    GetThemeFont(this->hTheme, hDCBtn, EBP_NORMALGROUPBACKGROUND, 0, TMT_FONT, &lfBtnCaption);
                    SetTextColor(hDCBtn, crBtnColor);
                }
                else
                    lfBtnCaption = this->lfCaption; /* Fall back to default caption font */

                lfBtnCaption.lfUnderline = this->bMouseOverButtons && pdis->CtlID == this->wPrevCtlID;
                hfBtnCaption = CreateFontIndirectW(&lfBtnCaption);

                hOld = SelectObject(this->hTheme ? hDCBtn : pdis->hDC, hfBtnCaption);

                DrawTextW(this->hTheme ? hDCBtn : pdis->hDC,
                          cswWindowText,
                          cswWindowText.GetLength(),
                          &rcBtnText,
                          DT_WORDBREAK);

                SelectObject(this->hTheme ? hDCBtn : pdis->hDC, hOld);
                DeleteObject(hfBtnCaption);

                /* Step 4: Combine themed buttons with item DC
                 * This step is executed only when visual styles are applied. */
                if (this->hTheme)
                    BitBlt(pdis->hDC, 0, 0, pdis->rcItem.right - pdis->rcItem.left, pdis->rcItem.bottom - pdis->rcItem.top, hDCBtn, 0, 0, SRCCOPY);

                DeleteObject(hbmBtn);
                DeleteDC(hDCBtn);
            }

            /* Draw listbox content, if the network list is not empty */
            if (this->lstWlanNetworks && this->lstWlanNetworks->dwNumberOfItems > 0 && pdis->CtlID == IDC_WLANWIZ_LISTBOX)
            {
                COLORREF cr3dFace = 0;
                COLORREF crWnd = 0;
                GRADIENT_RECT gRect = { 0 };
                TRIVERTEX tvRect[2] = { 0 };

                if (this->hTheme)
                {
                    GetThemeColor(this->hTheme, EBP_NORMALGROUPBACKGROUND, 0, TMT_FILLCOLOR, &cr3dFace);
                    GetThemeColor(this->hTheme, EBP_NORMALGROUPBACKGROUND, 0, TMT_BORDERCOLOR, &crWnd);
                }
                else
                {
                    cr3dFace = GetSysColor(COLOR_3DFACE);
                    crWnd = GetSysColor(COLOR_WINDOW);
                }
            
                tvRect[0].x     = pdis->rcItem.right;
                tvRect[0].y     = pdis->rcItem.bottom;
                tvRect[0].Red   = GetRValue(cr3dFace) << 8;
                tvRect[0].Green = GetGValue(cr3dFace) << 8;
                tvRect[0].Blue  = GetBValue(cr3dFace) << 8;
                tvRect[0].Alpha = 0x0000;

                tvRect[1].x     = pdis->rcItem.left;
                tvRect[1].y     = pdis->rcItem.top;
                tvRect[1].Red   = GetRValue(crWnd) << 8;
                tvRect[1].Green = GetGValue(crWnd) << 8;
                tvRect[1].Blue  = GetBValue(crWnd) << 8;
                tvRect[1].Alpha = 0x0000;

                gRect.LowerRight = 1;

                GradientFill(pdis->hDC, tvRect, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

                /* Draw network SSID */
                UINT uSSIDLength = static_cast<UINT>(SendDlgItemMessageW(pdis->CtlID, LB_GETTEXTLEN, pdis->itemID, NULL));
                UINT uItemRealID = static_cast<UINT>(SendDlgItemMessageW(pdis->CtlID, LB_GETITEMDATA, pdis->itemID, NULL));
                UINT uRSSI = this->lstWlanNetworks->Network[uItemRealID].wlanSignalQuality;
            
                cswWindowText.Empty();
                cswWindowText.Preallocate(uSSIDLength);

                SendDlgItemMessageW(pdis->CtlID, LB_GETTEXT, pdis->itemID, reinterpret_cast<LPARAM>(cswWindowText.GetBuffer()));
                cswWindowText.ReleaseBuffer();

                /* SSID is drawn with bold font */
                this->lfCaption.lfWeight = FW_BOLD;
                this->lfCaption.lfUnderline = FALSE;

                HFONT hfCaption = CreateFontIndirectW(&this->lfCaption);
                HGDIOBJ hOld = SelectObject(pdis->hDC, hfCaption);

                TextOutW(pdis->hDC, 4, pdis->rcItem.top + 4, cswWindowText, cswWindowText.GetLength());
            
                SelectObject(pdis->hDC, hOld);
                DeleteObject(hfCaption);

                this->lfCaption.lfWeight = FW_NORMAL;

                /* Choose signal level icon depending on signal strength */
                int iRSSIResIcon = IDI_WLANICON;

                if (16 <= uRSSI && uRSSI < 36)
                    iRSSIResIcon = IDI_WLANICON_20;
                else if(36 <= uRSSI && uRSSI < 56)
                    iRSSIResIcon = IDI_WLANICON_40;
                else if(56 <= uRSSI && uRSSI < 76)
                    iRSSIResIcon = IDI_WLANICON_60;
                else if(76 <= uRSSI && uRSSI < 96)
                    iRSSIResIcon = IDI_WLANICON_80;
                else if (uRSSI >= 96)
                    iRSSIResIcon = IDI_WLANICON_100;

                DrawIconEx(pdis->hDC,
                           pdis->rcItem.right - 36,
                           pdis->rcItem.bottom - 32,
                           LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCEW(iRSSIResIcon)),
                           32,
                           32,
                           NULL,
                           NULL,
                           DI_NORMAL);

            }

            if (pdis->itemState & ODS_FOCUS)
                DrawFocusRect(pdis->hDC, &pdis->rcItem);

            break;
        }
    }

    return FALSE;
}

LRESULT CWlanWizard::OnThemeChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CloseThemeData(this->hTheme);
    this->hTheme = OpenThemeData(m_hWnd, L"ExplorerBar");

    return FALSE;
}