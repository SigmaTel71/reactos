/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WM_DRAWITEM, m_ListboxWLAN)
 * COPYRIGHT:   Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

void
CWlanWizard::OnListboxDrawItem(_In_ UINT parentID, _In_ PDRAWITEMSTRUCT pdis)
{
    if (!m_lstWlanNetworks || m_lstWlanNetworks->dwNumberOfItems == 0)
        return;

    if (!(pdis->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
        return;

    ATL::CStringW cswWindowText;
    SetBkMode(pdis->hDC, TRANSPARENT);

    UINT uSSIDLength = static_cast<UINT>(SendDlgItemMessageW(pdis->CtlID, LB_GETTEXTLEN, pdis->itemID, NULL));
    DWORD uItemRealID = static_cast<DWORD>(SendDlgItemMessageW(pdis->CtlID, LB_GETITEMDATA, pdis->itemID, NULL));

    PWLAN_AVAILABLE_NETWORK pWlanNetwork = &m_lstWlanNetworks->Network[uItemRealID];

    /* Step 1: draw listbox item's graphics, starting with the background */
    if (!(pdis->itemState & ODS_SELECTED))
    {
        COLORREF cr3dFace = 0;
        COLORREF crWnd = 0;
        GRADIENT_RECT gRect = {0, 1};
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

        if (m_hThemeEB)
        {
            GetThemeColor(m_hThemeEB, EBP_NORMALGROUPBACKGROUND, 0, TMT_FILLCOLOR, &cr3dFace);
            GetThemeColor(m_hThemeEB, EBP_NORMALGROUPBACKGROUND, 0, TMT_BORDERCOLOR, &crWnd);
        }
        else
        {
            cr3dFace = GetSysColor(COLOR_3DFACE);
            crWnd = GetSysColor(COLOR_WINDOW);
        }

        tvRect[0].Red = GetRValue(cr3dFace) << 8;
        tvRect[0].Green = GetGValue(cr3dFace) << 8;
        tvRect[0].Blue = GetBValue(cr3dFace) << 8;

        tvRect[1].Red = GetRValue(crWnd) << 8;
        tvRect[1].Green = GetGValue(crWnd) << 8;
        tvRect[1].Blue = GetBValue(crWnd) << 8;

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
    WLAN_SIGNAL_QUALITY ulSQ = pWlanNetwork->wlanSignalQuality;
    DWORD dwSQIconID = IDI_WLANICON;

    if (16 <= ulSQ && ulSQ < 36)
        dwSQIconID = IDI_WLANICON_20;
    else if (36 <= ulSQ && ulSQ < 56)
        dwSQIconID = IDI_WLANICON_40;
    else if (56 <= ulSQ && ulSQ < 76)
        dwSQIconID = IDI_WLANICON_60;
    else if (76 <= ulSQ && ulSQ < 96)
        dwSQIconID = IDI_WLANICON_80;
    else if (ulSQ >= 96)
        dwSQIconID = IDI_WLANICON_100;

    DrawIconEx(pdis->hDC, pdis->rcItem.right - 36, pdis->rcItem.top + 24, m_MListboxIcons.Lookup(dwSQIconID), 32, 32, NULL, NULL, DI_NORMAL);

    /* Network type */
    DWORD dwBSSIconID = IDI_BSS_INFRA;

    if (pWlanNetwork->dot11BssType == dot11_BSS_type_independent)
        dwBSSIconID = IDI_BSS_ADHOC;

    DrawIconEx(pdis->hDC, pdis->rcItem.left + 2, pdis->rcItem.top + 3, m_MListboxIcons.Lookup(dwBSSIconID), 48, 48, NULL, NULL, DI_NORMAL);

    /* Authentication standard strength */
    if (pWlanNetwork->bSecurityEnabled)
    {
        DrawIconEx(pdis->hDC, 52, pdis->rcItem.top + 36, m_MListboxIcons.Lookup(IDI_SHELL32_LOCK), 16, 16, NULL, NULL, DI_NORMAL);

        /* WEP is not secure for decades */
        if (pWlanNetwork->dot11DefaultAuthAlgorithm < DOT11_AUTH_ALGO_WPA)
        {
            DrawIconEx(pdis->hDC,
                        54, pdis->rcItem.top + 37,
                        m_MListboxIcons.Lookup(IDI_AP_DHCP_FAILED),
                        16, 16,
                        NULL, NULL,
                        DI_NORMAL);
        }
    }

    /* Step 2: fill the listbox item with text */
    COLORREF crItemText = SetTextColor(pdis->hDC, GetSysColor(pdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

    /* SSID */
    m_lfCaption.lfWeight = FW_BOLD;
    m_lfCaption.lfUnderline = FALSE;

    HFONT hfCaption = CreateFontIndirectW(&m_lfCaption);
    HGDIOBJ hOld = SelectObject(pdis->hDC, hfCaption);

    cswWindowText = ATL::CStringW(L"", uSSIDLength);
    SendDlgItemMessageW(pdis->CtlID, LB_GETTEXT, pdis->itemID, reinterpret_cast<LPARAM>(cswWindowText.GetBuffer()));

    TextOutW(pdis->hDC, 52, pdis->rcItem.top + 4, cswWindowText, cswWindowText.GetLength());
    m_lfCaption.lfWeight = FW_NORMAL;

    SelectObject(pdis->hDC, hOld);
    DeleteObject(hfCaption);

    /* Authentication standard */
    ATL::CStringW cswNetworkSecurityAlgo = L"";
    ATL::CStringW cswNetworkSecurity = L"";

    if (pWlanNetwork->bSecurityEnabled)
    {
        switch (pWlanNetwork->dot11DefaultAuthAlgorithm)
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

    if (pWlanNetwork->dot11BssType == dot11_BSS_type_infrastructure)
        cswNetworkSecurity.LoadStringW(pWlanNetwork->bSecurityEnabled ? IDS_WLANWIZ_ENCRYPTED_AP : IDS_WLANWIZ_UNENCRYPTED_AP);
    else if (pWlanNetwork->dot11BssType == dot11_BSS_type_independent)
        cswNetworkSecurity.LoadStringW(pWlanNetwork->bSecurityEnabled ? IDS_WLANWIZ_ENCRYPTED_IBSS : IDS_WLANWIZ_UNENCRYPTED_IBSS);

    cswNetworkSecurity += cswNetworkSecurityAlgo;

    TextOutW(pdis->hDC,
             pWlanNetwork->bSecurityEnabled ? 72 : 52, pdis->rcItem.top + 38,
             cswNetworkSecurity, cswNetworkSecurity.GetLength());

    /* Connection state or preference as set in saved profile */
    bool hasProfile = wcslen(pWlanNetwork->strProfileName);
    bool preferAutoConnect = false;

    if (hasProfile)
    {
        preferAutoConnect = PreferAutoConnection(pWlanNetwork);
        ATL::CStringW cswConnState(preferAutoConnect ? (LPCWSTR)IDS_WLANWIZ_CONNECTED_AUTO : (LPCWSTR)IDS_WLANWIZ_CONNECTED_MANU);

        RECT rConnMode =
        {
            .left = pdis->rcItem.right - 100,
            .top = pdis->rcItem.top + 4,
            .right = pdis->rcItem.right - 25,
            .bottom = pdis->rcItem.top + 22
        };

        RECT rCalc = rConnMode;

        /* Network relation had to be drawn bold and painted in COLOR_3DFACE
         * out of queue and this breaks the pipeline. Refactoring help needed! */
        COLORREF oldRelationColor = SetTextColor(pdis->hDC, GetSysColor(pdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT));
        m_lfCaption.lfWeight = FW_BOLD;
        hfCaption = CreateFontIndirectW(&m_lfCaption);

        hOld = SelectObject(pdis->hDC, hfCaption);

        DrawTextW(pdis->hDC,
                  cswConnState, cswConnState.GetLength(),
                  &rCalc,
                  DT_CALCRECT | DT_RIGHT | DT_NOCLIP | DT_WORDBREAK);
        DrawTextW(pdis->hDC,
                  cswConnState, cswConnState.GetLength(),
                  &rConnMode,
                  DT_RIGHT | DT_NOCLIP | DT_WORDBREAK);

        SelectObject(pdis->hDC, hOld);
        DeleteObject(hfCaption);

        SetTextColor(pdis->hDC, oldRelationColor);

        DrawIconEx(pdis->hDC,
                   pdis->rcItem.right - 20, pdis->rcItem.top + 3,
                   m_MListboxIcons.Lookup(IDI_SHELL32_FAVORITES),
                   16, 16,
                   0, 0,
                   DI_NORMAL);
    }

    if (pdis->rcItem.bottom - pdis->rcItem.top == 56 && (pdis->itemState & ODS_SELECTED))
    {
        SetTextColor(pdis->hDC, crItemText);
        return;
    }

    /* Expanded text */
    if (pdis->itemState & ODS_SELECTED)
    {
        ATL::CStringW cswExpandedText = L"";

        if (pWlanNetwork->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
            cswExpandedText.LoadStringW(IDS_WLANWIZ_EXPAND_CONNECTED);
        else
        {
            if (pWlanNetwork->bSecurityEnabled)
            {
                pWlanNetwork->dot11DefaultAuthAlgorithm < DOT11_AUTH_ALGO_WPA
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

        DrawTextW(pdis->hDC,
                    cswExpandedText, cswExpandedText.GetLength(),
                    &rcExpandedText,
                    DT_CALCRECT | DT_WORDBREAK | DT_LEFT);
        DrawTextW(pdis->hDC,
                    cswExpandedText, cswExpandedText.GetLength(),
                    &rcExpandedText,
                    DT_WORDBREAK | DT_LEFT);

        /* We cannot host real controls, so visual emulation with collision checks has to be done. */
        if (!(pWlanNetwork->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED))
        {
            SIZE sCheckbox;

            if (m_hThemeButton)
            {
                GetThemePartSize(m_hThemeButton,
                                    pdis->hDC,
                                    BP_CHECKBOX, CBS_CHECKEDNORMAL,
                                    NULL,
                                    TS_DRAW, &sCheckbox);
            }
            else
            {
                sCheckbox.cx = GetSystemMetrics(SM_CXMENUCHECK);
                sCheckbox.cy = GetSystemMetrics(SM_CYMENUCHECK);
            }

            RECT rcCheckbox =
            {
                .left = 52,
                .top = rcExpandedText.bottom + 8,
                .right = rcCheckbox.left + sCheckbox.cx,
                .bottom = rcCheckbox.top + sCheckbox.cy
            };

            RECT rcCheckboxText = rcCheckbox;
            rcCheckboxText.left += sCheckbox.cx + 4;

            ATL::CStringW szAutoconn((LPCWSTR)IDS_WLANWIZ_EXPAND_AUTOCONNECT);

            DrawTextW(pdis->hDC,
                        szAutoconn, szAutoconn.GetLength(),
                        &rcCheckboxText,
                        DT_CALCRECT | DT_LEFT | DT_VCENTER);
            DrawTextW(pdis->hDC,
                        szAutoconn, szAutoconn.GetLength(),
                        &rcCheckboxText,
                        DT_LEFT | DT_VCENTER);

            if (m_hThemeButton)
            {
                CHECKBOXSTATES cbs = !hasProfile || preferAutoConnect ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL;

                DrawThemeBackground(m_hThemeButton,
                                    pdis->hDC,
                                    BP_CHECKBOX, cbs, &rcCheckbox, NULL);
            }
            else
            {
                UINT uState = !hasProfile || preferAutoConnect ? DFCS_CHECKED : 0;
                DrawFrameControl(pdis->hDC, &rcCheckbox, DFC_BUTTON, DFCS_BUTTONCHECK | uState);
            }
        }
    }

    SetTextColor(pdis->hDC, crItemText);
}
