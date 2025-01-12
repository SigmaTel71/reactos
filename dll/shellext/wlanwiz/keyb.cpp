/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (Keyboard Routines)
 * COPYRIGHT:   Copyright 2024-2025 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

/* On sidebar buttons and 'Connect/Disconnect' button we would like to catch 
 * VK_ESCAPE and all buttons that are used to click the button without mouse.
 * VK_TAB does not require special handling, it works as intended in groupbox.
 */
LRESULT CWlanWizard::OnGetDlgCode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
        return DLGC_WANTMESSAGE;

    /* Returning anything else prevents arrows from functioning */
    return FALSE;
}

LRESULT CWlanWizard::OnGetDlgCodeLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return DLGC_WANTMESSAGE;
}

LRESULT CWlanWizard::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch (wParam)
    {
        case VK_ESCAPE:
            SendMessageW(WM_CLOSE);
            break;

        case VK_F5:
            m_SidebarButtonSN.SendMessageW(BM_CLICK);
            break;

        case VK_SPACE:
        case VK_RETURN:
        {
            ATL::CWindow hFocused = GetFocus();
            hFocused.SendMessageW(BM_CLICK);
        }
    }

    return FALSE;
}

LRESULT CWlanWizard::OnVKeyToItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch (LOWORD(wParam))
    {
        case VK_ESCAPE:
            SendMessageW(WM_CLOSE);
            return -2;

        case VK_F5:
            m_SidebarButtonSN.SendMessageW(BM_CLICK);
            return -2;

        case VK_TAB:
        {
            ATL::CWindow hDlgItem = GetNextDlgTabItem(m_ListboxWLAN, GetAsyncKeyState(VK_SHIFT) & 0x01);
            hDlgItem.SetFocus();
            return -2;
        }
    }

    return -1;
}