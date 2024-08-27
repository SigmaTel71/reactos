/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Scanning Routines)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "main.h"

LRESULT CWlanWizard::OnScanNetworks(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MSG msg;

    this->uScanStatus = STATUS_SCANNING;
    this->bScanTimeout = FALSE;
    this->dwSelectedItemID = -1;

    m_SidebarButtonAS.EnableWindow(FALSE);
    m_SidebarButtonSN.EnableWindow(FALSE);
    m_ConnectButton.EnableWindow(FALSE);
    m_ListboxWLAN.EnableWindow(FALSE);

    /* Clear listbox from previously discovered networks */
    m_ListboxWLAN.SendMessageW(LB_RESETCONTENT, NULL, NULL);
    m_ListboxWLAN.Invalidate();

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
        m_ConnectButton.EnableWindow();

        /* Show discovered networks */
        RECT rc;
        m_ListboxWLAN.GetClientRect(&rc);

        /* Add discovered networks to listbox and sort networks by signal level */
        ATL::CAtlList<ATL::CSimpleArray<UINT>> calIndexToSignalQuality;

        for (;
            this->lstWlanNetworks->dwIndex <= this->lstWlanNetworks->dwNumberOfItems - 1;
            this->lstWlanNetworks->dwIndex++)
        {
            ATL::CSimpleArray<UINT> csaIndexAndQuality;
            csaIndexAndQuality.Add(this->lstWlanNetworks->Network[this->lstWlanNetworks->dwIndex].wlanSignalQuality);
            csaIndexAndQuality.Add(this->lstWlanNetworks->dwIndex);

            calIndexToSignalQuality.AddTail(csaIndexAndQuality);
        }

        for (UINT i = 0; i < calIndexToSignalQuality.GetCount(); ++i)
        {
            for (UINT i = 0; i < calIndexToSignalQuality.GetCount() - 1; ++i)
            {
                auto left = calIndexToSignalQuality.GetAt(calIndexToSignalQuality.FindIndex(i));
                auto right = calIndexToSignalQuality.GetAt(calIndexToSignalQuality.FindIndex(i + 1));

                if (left[0] < right[0])
                    calIndexToSignalQuality.SwapElements(calIndexToSignalQuality.FindIndex(i), calIndexToSignalQuality.FindIndex(i + 1));
            }
        }

        POSITION posIAQ = calIndexToSignalQuality.GetHeadPosition();

        while (posIAQ != NULL)
        {
            auto csaIAQ = calIndexToSignalQuality.GetNext(posIAQ);

            ATL::CStringW cswWlanNetworkName = "";
            cswWlanNetworkName = CA2W(reinterpret_cast<LPCSTR>(this->lstWlanNetworks->Network[csaIAQ[1]].dot11Ssid.ucSSID), CP_ACP);

            if (cswWlanNetworkName.IsEmpty())
                cswWlanNetworkName.LoadStringW(IDS_WLANWIZ_HIDDEN_NETWORK);

            LRESULT iItemIdx = m_ListboxWLAN.SendMessageW(LB_ADDSTRING,
                NULL,
                reinterpret_cast<LPARAM>(cswWlanNetworkName.GetBuffer()));

            m_ListboxWLAN.SendMessageW(LB_SETITEMDATA,
                iItemIdx,
                static_cast<LPARAM>(csaIAQ[1]));
        }
    }

    m_ListboxWLAN.Invalidate();
    m_ListboxWLAN.SetFocus();

    return FALSE;
}

DWORD WINAPI CWlanWizard::ScanNetworksThread(_In_ LPVOID lpParameter)
{
    GUID gWlanDeviceID = { 0 };
    CWlanWizard* cwwThis = reinterpret_cast<CWlanWizard*>(lpParameter);
    
    if (IIDFromString(cwwThis->m_sGUID, &gWlanDeviceID) == S_OK)
    {
        WlanScan(cwwThis->hWlanClient, &gWlanDeviceID, NULL, NULL, NULL);
        WlanGetAvailableNetworkList(cwwThis->hWlanClient, &gWlanDeviceID, 1, NULL, &cwwThis->lstWlanNetworks);
    }

    return FALSE;
}