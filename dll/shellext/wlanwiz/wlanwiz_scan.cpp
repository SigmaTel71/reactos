/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Scanning Routines)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include <algorithm>
#include "main.h"

LRESULT CWlanWizard::OnScanNetworks(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MSG msg;

    this->uScanStatus = STATUS_SCANNING;
    this->bScanTimeout = FALSE;

    m_SidebarButtonAS.EnableWindow(FALSE);
    m_SidebarButtonSN.EnableWindow(FALSE);
    m_ConnectButton.EnableWindow(FALSE);
    m_ListboxWLAN.EnableWindow(FALSE);

    /* Clear listbox from previously discovered networks */
    m_ListboxWLAN.SendMessageW(LB_RESETCONTENT, NULL, NULL);
    m_ListboxWLAN.RedrawWindow(NULL, NULL, RDW_ERASENOW | RDW_ERASE | RDW_INVALIDATE);

    SetTimer(IDT_SCANNING_NETWORKS, 5000);

    HCURSOR hOldCursor = SetCursor(LoadCursorW(NULL, IDC_APPSTARTING));

    this->hScanThread = CreateThread(NULL, NULL, &this->ScanNetworksThread, reinterpret_cast<LPVOID>(this), NULL, NULL);

    while (this->bScanTimeout == FALSE)
    {
        switch (MsgWaitForMultipleObjects(0, &this->hScanThread, FALSE, INFINITE, QS_ALLINPUT))
        {
        case WAIT_OBJECT_0: {
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            break;
        }
        }
    }

    this->uScanStatus = STATUS_SCAN_COMPLETE;

    SetCursor(hOldCursor);

    m_SidebarButtonAS.EnableWindow();
    m_SidebarButtonSN.EnableWindow();

    if (this->lstWlanNetworks->dwNumberOfItems > 0)
    {
        m_ListboxWLAN.EnableWindow();

        /* Show discovered networks */
        RECT rc;
        m_ListboxWLAN.GetClientRect(&rc);

        /* Add discovered networks to listbox and sort networks by signal level */
        std::vector<std::pair<UINT, UINT>> vIndexToSignalQuality;
        for (;
            this->lstWlanNetworks->dwIndex <= this->lstWlanNetworks->dwNumberOfItems - 1;
            this->lstWlanNetworks->dwIndex++)
        {
            vIndexToSignalQuality.push_back(std::make_pair(this->lstWlanNetworks->Network[this->lstWlanNetworks->dwIndex].wlanSignalQuality,
                this->lstWlanNetworks->dwIndex));
        }

        std::sort(vIndexToSignalQuality.begin(), vIndexToSignalQuality.end(), [](std::pair<UINT, UINT> left, std::pair<UINT, UINT> right)
            {
                return left.first > right.first;
            });

        for (auto& pNetSorted : vIndexToSignalQuality)
        {
            ATL::CStringW cswWlanNetworkName = "";
            cswWlanNetworkName = CA2W(reinterpret_cast<LPCSTR>(this->lstWlanNetworks->Network[pNetSorted.second].dot11Ssid.ucSSID), CP_ACP);

            if (cswWlanNetworkName.IsEmpty())
                cswWlanNetworkName.LoadStringW(IDS_WLANWIZ_HIDDEN_NETWORK);

            LRESULT iItemIdx = m_ListboxWLAN.SendMessageW(LB_ADDSTRING,
                NULL,
                reinterpret_cast<LPARAM>(cswWlanNetworkName.GetBuffer()));

            m_ListboxWLAN.SendMessageW(LB_SETITEMDATA,
                iItemIdx,
                static_cast<LPARAM>(pNetSorted.second));
        }
    }

    m_ListboxWLAN.RedrawWindow(NULL, NULL, RDW_ERASENOW | RDW_ERASE | RDW_INVALIDATE);
    m_ListboxWLAN.SetFocus();

    return FALSE;
}

DWORD WINAPI CWlanWizard::ScanNetworksThread(_In_ LPVOID lpParameter)
{
    GUID gWlanDeviceID = { 0 };
    CWlanWizard* cwwThis = reinterpret_cast<CWlanWizard*>(lpParameter);
    IIDFromString(cwwThis->m_sGUID, &gWlanDeviceID);

    WlanScan(cwwThis->hWlanClient, &gWlanDeviceID, NULL, NULL, NULL);
    WlanGetAvailableNetworkList(cwwThis->hWlanClient, &gWlanDeviceID, 1, NULL, &cwwThis->lstWlanNetworks);

    return FALSE;
}