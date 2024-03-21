/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (Connect/Disconnect button)
 * COPYRIGHT:   Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "main.h"

LRESULT
CWlanWizard::OnMainButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int lbSelection = m_ListboxWLAN.SendMessageW(LB_GETCURSEL);
    DWORD wlanIdx = m_ListboxWLAN.SendMessageW(LB_GETITEMDATA, lbSelection);

    const PWLAN_AVAILABLE_NETWORK item = &m_lstWlanNetworks->Network[wlanIdx];
    GUID interfaceGUID;
    IIDFromString(m_sGUID, &interfaceGUID);

    if (item->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
    {

        WlanDisconnect(m_hWlanClient, &interfaceGUID, NULL);
        m_SidebarButtonSN.SendMessageW(BM_CLICK);
        bHandled = TRUE;
        return 0;
    }

    WLAN_CONNECTION_PARAMETERS params = {};

    WlanConnect(m_hWlanClient, &interfaceGUID, &params, NULL);

    bHandled = TRUE;
    return 0;
}
