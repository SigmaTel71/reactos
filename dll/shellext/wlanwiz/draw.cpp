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
            bHandled = TRUE;
            break;
    }
    return TRUE;
}

LRESULT CWlanWizard::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps = { 0 };
    HDC hDC = BeginPaint(&ps);

    /* Paint the sidebar, if we have theming enabled */
    if (this->hThemeEB)
    {
        RECT rcEtchVert = { 0 };
        ATL::CWindow hEtchVert = GetDlgItem(IDC_WLANWIZ_ETCHEDVERT);
        hEtchVert.GetClientRect(&rcEtchVert);
        hEtchVert.MapWindowPoints(hEtchVert.GetParent(), &rcEtchVert);

        rcEtchVert.left = 0;
        rcEtchVert.top = 0;

        DrawThemeBackground(this->hThemeEB, hDC, 0, 0, &rcEtchVert, NULL);
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
                HGDIOBJ hOld = NULL;

                GetDlgItemTextW(static_cast<int>(wParam), cswWindowText);

                /* Step 1: Prepare drawing surface */
                HBRUSH hbrRect = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
                
                if (this->hThemeEB)
                {
                    SetBkMode(hDCBtn, TRANSPARENT);
                    SelectObject(hDCBtn, hbmBtn);

                    COLORREF crFill = 0;
                    GetThemeColor(this->hThemeEB, EBP_NORMALGROUPBACKGROUND, 0, TMT_FILLCOLOR, &crFill);
                    hbrRect = CreateSolidBrush(crFill);
                }

                FillRect(!this->hThemeEB ? pdis->hDC : hDCBtn,
                         &pdis->rcItem,
                         hbrRect);

                DeleteObject(hbrRect);

                /* Step 2: Draw related icon to the sidebar button
                 * The +20 offset originates from resource.h, the IDs are laid out in a way
                 * they can be safely batch processed in loops, as the range is fixed. */
                if (wParam >= IDC_WLANWIZ_SCAN_NETWORKS && wParam <= IDC_WLANWIZ_ADVANCED_SETTINGS)
                {
                    HICON hSidebarIcon = NULL;

                    wParam == IDC_WLANWIZ_PREFERRED_APS
                        ? hSidebarIcon = LoadIconW(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCE(44)) /* 'Favorites' icon */
                        : hSidebarIcon = LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCEW(wParam + 20));

                    DrawIconEx(this->hThemeEB ? hDCBtn : pdis->hDC,
                               pdis->rcItem.left + 2,
                               pdis->rcItem.top + 2,
                               hSidebarIcon,
                               16,
                               16,
                               NULL,
                               NULL,
                               DI_NORMAL);
                }

                /* Step 3: Draw button text */
                HFONT hfBtnCaption = NULL;
                LOGFONTW lfBtnCaption = { 0 };
                
                if (this->hThemeEB)
                {
                    COLORREF crBtnColor = 0;
                    /* According to msstyles, the items do not have transparent background,
                     * so they have been filled by color to blend in. Same as in wzcdlg. */
                    GetThemeColor(this->hThemeEB, EBP_NORMALGROUPBACKGROUND, 0, TMT_TEXTCOLOR, &crBtnColor);

                    /* We can't use DrawThemeText directly as it's not aware of underline style? */
                    GetThemeFont(this->hThemeEB, hDCBtn, EBP_NORMALGROUPBACKGROUND, 0, TMT_FONT, &lfBtnCaption);
                    SetTextColor(hDCBtn, crBtnColor);
                }
                else
                    lfBtnCaption = this->lfCaption; /* Fall back to default caption font */

                lfBtnCaption.lfUnderline = this->bMouseOverButtons && pdis->CtlID == this->wPrevCtlID;
                hfBtnCaption = CreateFontIndirectW(&lfBtnCaption);

                hOld = SelectObject(this->hThemeEB ? hDCBtn : pdis->hDC, hfBtnCaption);

                DrawTextW(this->hThemeEB ? hDCBtn : pdis->hDC,
                          cswWindowText,
                          cswWindowText.GetLength(),
                          &rcBtnText,
                          DT_WORDBREAK);

                SelectObject(this->hThemeEB ? hDCBtn : pdis->hDC, hOld);
                DeleteObject(hfBtnCaption);

                /* Step 4: Combine themed buttons with item DC
                 * This step is executed only when visual styles are applied. */
                if (this->hThemeEB)
                    BitBlt(pdis->hDC, 0, 0, pdis->rcItem.right - pdis->rcItem.left, pdis->rcItem.bottom - pdis->rcItem.top, hDCBtn, 0, 0, SRCCOPY);

                DeleteObject(hbmBtn);
                DeleteDC(hDCBtn);
            }

            /* Draw listbox content, if the network list is not empty */
            if (this->lstWlanNetworks && this->lstWlanNetworks->dwNumberOfItems > 0 && pdis->CtlID == IDC_WLANWIZ_LISTBOX)
            {
                UINT uSSIDLength = static_cast<UINT>(SendDlgItemMessageW(pdis->CtlID, LB_GETTEXTLEN, pdis->itemID, NULL));
                DWORD uItemRealID = static_cast<DWORD>(SendDlgItemMessageW(pdis->CtlID, LB_GETITEMDATA, pdis->itemID, NULL));

                WLAN_AVAILABLE_NETWORK wlanNetwork = this->lstWlanNetworks->Network[uItemRealID];

                /* Step 1: draw listbox item's graphics, starting with the background */
                if (!(pdis->itemState & ODS_SELECTED))
                {
                    COLORREF cr3dFace = 0;
                    COLORREF crWnd = 0;
                    GRADIENT_RECT gRect = { 0, 1 };
                    TRIVERTEX tvRect[2] =
                    { 
                        {
                            .x = pdis->rcItem.right,
                            .y = pdis->rcItem.bottom,
                            .Alpha = 0x0000,
                        },
                        {
                            .x = pdis->rcItem.left,
                            .y = pdis->rcItem.top,
                            .Alpha = 0x0000
                        }
                    };

                    if (this->hThemeEB)
                    {
                        GetThemeColor(this->hThemeEB, EBP_NORMALGROUPBACKGROUND, 0, TMT_FILLCOLOR, &cr3dFace);
                        GetThemeColor(this->hThemeEB, EBP_NORMALGROUPBACKGROUND, 0, TMT_BORDERCOLOR, &crWnd);
                    }
                    else
                    {
                        cr3dFace = GetSysColor(COLOR_3DFACE);
                        crWnd = GetSysColor(COLOR_WINDOW);
                    }

                    tvRect[0].Red   = GetRValue(cr3dFace) << 8;
                    tvRect[0].Green = GetGValue(cr3dFace) << 8;
                    tvRect[0].Blue  = GetBValue(cr3dFace) << 8;

                    tvRect[1].Red   = GetRValue(crWnd) << 8;
                    tvRect[1].Green = GetGValue(crWnd) << 8;
                    tvRect[1].Blue  = GetBValue(crWnd) << 8;

                    GradientFill(pdis->hDC, tvRect, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
                }
                else
                {
                    /* Themed listbox items appeared in Windows Vista, so no DrawThemeBackground here */
                    HBRUSH hbrSelectedBg = GetSysColorBrush(COLOR_HIGHLIGHT);
                    FillRect(pdis->hDC, &pdis->rcItem, hbrSelectedBg);
                    DeleteObject(hbrSelectedBg);
                }

                /* Signal quality bar */
                WLAN_SIGNAL_QUALITY ulSQ = wlanNetwork.wlanSignalQuality;
                int iSQResIcon = IDI_WLANICON;

                if (16 <= ulSQ && ulSQ < 36)
                    iSQResIcon = IDI_WLANICON_20;
                else if (36 <= ulSQ && ulSQ < 56)
                    iSQResIcon = IDI_WLANICON_40;
                else if (56 <= ulSQ && ulSQ < 76)
                    iSQResIcon = IDI_WLANICON_60;
                else if (76 <= ulSQ && ulSQ < 96)
                    iSQResIcon = IDI_WLANICON_80;
                else if (ulSQ >= 96)
                    iSQResIcon = IDI_WLANICON_100;

                HICON hicnRes = LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCEW(iSQResIcon));

                DrawIconEx(pdis->hDC,
                           pdis->rcItem.right - 36, pdis->rcItem.top + 24,
                           hicnRes,
                           32, 32,
                           NULL,
                           NULL,
                           DI_NORMAL);

                DestroyIcon(hicnRes);

                /* Network type */
                int iBSSIcon = IDI_BSS_INFRA;

                if (wlanNetwork.dot11BssType == dot11_BSS_type_independent)
                    iBSSIcon = IDI_BSS_ADHOC;

                hicnRes = static_cast<HICON>(LoadImageW(wlanwiz_hInstance, MAKEINTRESOURCEW(iBSSIcon), IMAGE_ICON, 48, 48, LR_LOADTRANSPARENT));

                DrawIconEx(pdis->hDC,
                           pdis->rcItem.left + 2, pdis->rcItem.top + 3,
                           hicnRes,
                           48, 48,
                           NULL,
                           NULL,
                           DI_NORMAL);

                DestroyIcon(hicnRes);

                /* Authentication standard strength */
                if (wlanNetwork.bSecurityEnabled)
                {
                    HICON hicnSecurity = LoadIconW(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCEW(48)); /* lock icon */

                    DrawIconEx(pdis->hDC,
                               52, pdis->rcItem.top + 36,
                               hicnSecurity,
                               16, 16,
                               NULL,
                               NULL,
                               DI_NORMAL);

                    /* WEP is not secure for decades */
                    if (wlanNetwork.dot11DefaultAuthAlgorithm < DOT11_AUTH_ALGO_WPA)
                    {
                        DrawIconEx(pdis->hDC,
                                   54, pdis->rcItem.top + 37,
                                   LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCE(IDI_AP_DHCP_FAILED)),
                                   16, 16,
                                   NULL,
                                   NULL,
                                   DI_NORMAL);
                    }

                    DestroyIcon(hicnSecurity);
                }

                /* Step 2: fill the listbox item with text */
                COLORREF crItemText = SetTextColor(pdis->hDC, GetSysColor(pdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

                /* SSID */
                this->lfCaption.lfWeight = FW_BOLD;
                this->lfCaption.lfUnderline = FALSE;

                HFONT hfCaption = CreateFontIndirectW(&this->lfCaption);
                HGDIOBJ hOld = SelectObject(pdis->hDC, hfCaption);

                cswWindowText = ATL::CStringW(L"", uSSIDLength);
                SendDlgItemMessageW(pdis->CtlID, LB_GETTEXT, pdis->itemID, reinterpret_cast<LPARAM>(cswWindowText.GetBuffer()));

                TextOutW(pdis->hDC, 52, pdis->rcItem.top + 4, cswWindowText, cswWindowText.GetLength());
                this->lfCaption.lfWeight = FW_NORMAL;
                
                SelectObject(pdis->hDC, hOld);
                DeleteObject(hfCaption);

                /* Authentication standard */
                ATL::CStringW cswNetworkSecurityAlgo = L"";
                ATL::CStringW cswNetworkSecurity = L"";

                if (wlanNetwork.bSecurityEnabled)
                {
                    switch (wlanNetwork.dot11DefaultAuthAlgorithm)
                    {
                    case DOT11_AUTH_ALGO_80211_OPEN:
                    case DOT11_AUTH_ALGO_80211_SHARED_KEY:
                        cswNetworkSecurityAlgo = L" (WEP)";
                        break;
                    case DOT11_AUTH_ALGO_WPA:
                    case DOT11_AUTH_ALGO_WPA_PSK:
                        cswNetworkSecurityAlgo = L" (WPA)";
                        break;
                    case DOT11_AUTH_ALGO_RSNA:
                    case DOT11_AUTH_ALGO_RSNA_PSK: /* Possible as of NT 6.0 on ad hoc */
                        cswNetworkSecurityAlgo = L" (WPA2)";
                        break;
                    case DOT11_AUTH_ALGO_WPA3_ENT_192:
                    case DOT11_AUTH_ALGO_WPA3_SAE:
                    case DOT11_AUTH_ALGO_WPA3_ENT:
                        cswNetworkSecurityAlgo = L" (WPA3)";
                        break;
                    default:
                        break;
                    }
                }

                if (wlanNetwork.dot11BssType == dot11_BSS_type_infrastructure)
                    cswNetworkSecurity.LoadStringW(wlanNetwork.bSecurityEnabled ? IDS_WLANWIZ_ENCRYPTED_AP : IDS_WLANWIZ_UNENCRYPTED_AP);
                else if (wlanNetwork.dot11BssType == dot11_BSS_type_independent)
                    cswNetworkSecurity.LoadStringW(wlanNetwork.bSecurityEnabled ? IDS_WLANWIZ_ENCRYPTED_IBSS : IDS_WLANWIZ_UNENCRYPTED_IBSS);

                cswNetworkSecurity += cswNetworkSecurityAlgo;

                TextOutW(pdis->hDC,
                         wlanNetwork.bSecurityEnabled ? 72 : 52,
                         pdis->rcItem.top + 38,
                         cswNetworkSecurity, cswNetworkSecurity.GetLength());


                /* Expanded text */
                if (pdis->itemState & ODS_SELECTED)
                {
                    ATL::CStringW cswExpandedText = L"";
                    ATL::CStringW cswConnectButtonText = L"";

                    if (wlanNetwork.dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
                    {
                        cswConnectButtonText.LoadStringW(IDS_WLANWIZ_DISCONNECT);
                        cswExpandedText.LoadStringW(IDS_WLANWIZ_EXPAND_CONNECTED);
                    }
                    else
                    {
                        cswConnectButtonText.LoadStringW(IDS_WLANWIZ_CONNECT);
                        if (wlanNetwork.bSecurityEnabled)
                        {
                            wlanNetwork.dot11DefaultAuthAlgorithm < DOT11_AUTH_ALGO_WPA
                                ? cswExpandedText.LoadStringW(IDS_WLANWIZ_EXPAND_ENCRYPTED_OBSOLETE)
                                : cswExpandedText.LoadStringW(IDS_WLANWIZ_EXPAND_ENCRYPTED);
                        }
                        else
                            cswExpandedText.LoadStringW(IDS_WLANWIZ_EXPAND_UNENCRYPTED);
                    }

                    RECT rcExpandedText =
                    {
                        .left = 52,
                        .top = pdis->rcItem.top + 60,
                        .right = pdis->rcItem.right - 4,
                        .bottom = pdis->rcItem.bottom
                    };

                    DrawTextW(pdis->hDC, cswExpandedText, cswExpandedText.GetLength(), &rcExpandedText, DT_WORDBREAK | DT_LEFT);
                    GetDlgItem(IDC_WLANWIZ_MAINBUTTON).SetWindowTextW(cswConnectButtonText);
                }

                SetTextColor(pdis->hDC, crItemText);
                
                /* Shrink focus rectangle for listbox items */
                pdis->rcItem.left += 1;
                pdis->rcItem.top += 1;
                pdis->rcItem.right -= 1;
                pdis->rcItem.bottom -= 1;
            }

            /* FIXME: The focus rectangle is not drawn inside listbox */
            if (pdis->itemState & ODS_FOCUS)
                DrawFocusRect(pdis->hDC, &pdis->rcItem);

            break;
        }
    }

    return FALSE;
}

LRESULT CWlanWizard::OnThemeChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CloseThemeData(this->hThemeEB);
    this->hThemeEB = OpenThemeData(m_hWnd, L"ExplorerBar");

    return FALSE;
}