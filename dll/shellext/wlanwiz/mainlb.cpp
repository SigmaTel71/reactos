/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (Listbox procedures)
 * COPYRIGHT:   Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

LRESULT
CWlanWizard::OnDoubleClickLB(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    LRESULT itemID = m_ListboxWLAN.SendMessageW(LB_GETCURSEL);

    if (itemID == LB_ERR)
    {
        bHandled = TRUE;
        return 0;
    }

    PLB_ITEMDATA plbItemData = reinterpret_cast<PLB_ITEMDATA>(m_ListboxWLAN.SendMessageW(LB_GETITEMDATA, itemID));
    const PWLAN_AVAILABLE_NETWORK pwlanNetwork = &m_lstWlanNetworks->Network[plbItemData->dwRealItemID];

    GUID interfaceGUID;
    IIDFromString(m_sGUID, &interfaceGUID);

    /* If the selected network is connected, we disconnect from it */
    if (pwlanNetwork->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
    {
        WlanDisconnect(m_hWlanClient, &interfaceGUID, NULL);
        m_SidebarButtonSN.SendMessageW(BM_CLICK);
        bHandled = TRUE;
        return 0;
    }

    /* Extra extension of the listbox item to fit password input field
     * if the expanded area text is too long */
    RECT rcItem;
    m_ListboxWLAN.SendMessageW(LB_GETITEMRECT, itemID, (LPARAM)&rcItem);

    LONG topPoint = m_rcCheckbox.bottom + 4;
    if (topPoint + 20 > rcItem.bottom)
    {
        m_ListboxWLAN.SendMessageW(LB_SETITEMHEIGHT, itemID, rcItem.bottom - rcItem.top + 20);
        m_ListboxWLAN.Invalidate(FALSE);
    }

    /* We do not show password field for networks with a known profile or no security enabled.
     * TODO: But if the network password is incorrect, we should show it anyway. */
    if (pwlanNetwork->bSecurityEnabled && wcslen(pwlanNetwork->strProfileName) == 0)
    {
        m_ConnectButton.EnableWindow(FALSE);
        m_PasswordWnd.SetWindowPos(NULL, m_rcCheckbox.left, topPoint, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
        m_PasswordWnd.SetFocus();
        m_ListboxWLAN.EnableScrollBar(SB_VERT, ESB_DISABLE_BOTH);
    }

    WLAN_CONNECTION_PARAMETERS params = {};
    WlanConnect(m_hWlanClient, &interfaceGUID, &params, NULL);

    return 0;
}

LRESULT
CWlanWizard::OnSelectionChangeLB(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    LRESULT itemID = m_ListboxWLAN.SendMessageW(LB_GETCURSEL);

    if (itemID == LB_ERR)
    {
        bHandled = TRUE;
        return 0;
    }

    PLB_ITEMDATA plbItemData;

    if (m_selectedItemID != LB_ERR && (m_selectedItemID != itemID))
    {
        m_ListboxWLAN.SendMessageW(LB_SETITEMHEIGHT, m_selectedItemID, 56);
        plbItemData = reinterpret_cast<PLB_ITEMDATA>(m_ListboxWLAN.SendMessageW(LB_GETITEMDATA, m_selectedItemID));
        plbItemData->bShouldAutoconnect = plbItemData->bShouldAutoconnectDefault;
        m_PasswordWnd.ShowWindow(SW_HIDE);
        m_PasswordWnd.SetWindowTextW(L"");
    }

    if (!m_PasswordWnd.IsWindowVisible())
    {
        m_ConnectButton.EnableWindow();
        m_ListboxWLAN.SendMessageW(LB_SETITEMHEIGHT, itemID, 136);
        m_ListboxWLAN.EnableScrollBar(SB_VERT, ESB_ENABLE_BOTH);
    }

    m_selectedItemID = itemID;
    plbItemData = reinterpret_cast<PLB_ITEMDATA>(m_ListboxWLAN.SendMessageW(LB_GETITEMDATA, itemID));

    m_ListboxWLAN.Invalidate(FALSE);

    bool bIsConnected = m_lstWlanNetworks->Network[plbItemData->dwRealItemID].dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED;

    ATL::CStringW cswConnectButtonText(bIsConnected ? (LPCWSTR)IDS_WLANWIZ_DISCONNECT : (LPCWSTR)IDS_WLANWIZ_CONNECT);
    m_ConnectButton.SetWindowTextW(cswConnectButtonText);

    bHandled = TRUE;
    return 0;
}
