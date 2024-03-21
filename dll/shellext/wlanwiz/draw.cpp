/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (Drawing Routines)
 * COPYRIGHT:   Copyright 2024-2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

LRESULT
CWlanWizard::OnMeasureItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

LRESULT
CWlanWizard::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps = { 0 };
    HDC hDC = BeginPaint(&ps);

    /* Paint the sidebar, if we have theming enabled */
    if (m_hThemeEB)
    {
        RECT rcEtchVert = { 0 };
        ATL::CWindow hEtchVert = GetDlgItem(IDC_WLANWIZ_ETCHEDVERT);
        hEtchVert.GetClientRect(&rcEtchVert);
        hEtchVert.MapWindowPoints(hEtchVert.GetParent(), &rcEtchVert);

        rcEtchVert.left = 0;
        rcEtchVert.top = 0;

        DrawThemeBackground(m_hThemeEB, hDC, 0, 0, &rcEtchVert, NULL);
    }

    EndPaint(&ps);
    return FALSE;
}


LRESULT
CWlanWizard::OnDrawItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PDRAWITEMSTRUCT pdis = reinterpret_cast<PDRAWITEMSTRUCT>(lParam);

    switch (pdis->CtlID)
    {
        case IDC_WLANWIZ_SCAN_NETWORKS:
        case IDC_WLANWIZ_INSTALL_WLAN:
        case IDC_WLANWIZ_PREFERRED_APS:   
        case IDC_WLANWIZ_ADVANCED_SETTINGS:
            OnSidebarBtnDrawItem(pdis->CtlID, pdis);
            break;
        case IDC_WLANWIZ_LISTBOX:
            OnListboxDrawItem(pdis->CtlID, pdis);
            break;
    }

    bHandled = TRUE;
    return 0;
}

LRESULT
CWlanWizard::OnThemeChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CloseThemeData(m_hThemeEB);
    CloseThemeData(m_hThemeButton);
    m_hThemeEB = OpenThemeData(m_hWnd, L"ExplorerBar");
    m_hThemeButton = OpenThemeData(m_hWnd, L"Button");

    return 0;
}
