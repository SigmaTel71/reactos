/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Scanning Routines)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

/* TODO: Insertion sort, maybe? Or come back to STL sort, gladly C++20 is here to help */
void CWlanWizard::SortScannedNetworks(ATL::CAtlList<ATL::CSimpleArray<UINT>>& calcsa)
{
    for (UINT i = 0; i < calcsa.GetCount(); ++i)
    {
        for (UINT i = 0; i < calcsa.GetCount() - 1; ++i)
        {
            auto left = calcsa.GetAt(calcsa.FindIndex(i));
            auto right = calcsa.GetAt(calcsa.FindIndex(i + 1));

            if (left[0] < right[0])
                calcsa.SwapElements(calcsa.FindIndex(i), calcsa.FindIndex(i + 1));
        }
    }
}

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
        ATL::CAtlList<ATL::CSimpleArray<UINT>> calIndexToSignalQuality, calIndexToSignalQualityAdHoc;

        for (;
            this->lstWlanNetworks->dwIndex <= this->lstWlanNetworks->dwNumberOfItems - 1;
            this->lstWlanNetworks->dwIndex++)
        {
            ATL::CSimpleArray<UINT> csaIndexAndQuality;
            WLAN_AVAILABLE_NETWORK wlanNetwork = this->lstWlanNetworks->Network[this->lstWlanNetworks->dwIndex];
            csaIndexAndQuality.Add(wlanNetwork.wlanSignalQuality);
            csaIndexAndQuality.Add(this->lstWlanNetworks->dwIndex);

            if (wlanNetwork.dot11BssType == dot11_BSS_type_infrastructure)
                calIndexToSignalQuality.AddTail(csaIndexAndQuality);
            else if (wlanNetwork.dot11BssType == dot11_BSS_type_independent)
                calIndexToSignalQualityAdHoc.AddTail(csaIndexAndQuality);
        }

        SortScannedNetworks(calIndexToSignalQuality);
        SortScannedNetworks(calIndexToSignalQualityAdHoc);

        /* Ad hoc networks are shown last */
        calIndexToSignalQuality.AddTailList(&calIndexToSignalQualityAdHoc);

        POSITION posIAQ = calIndexToSignalQuality.GetHeadPosition();

        while (posIAQ != NULL)
        {
            auto csaIAQ = calIndexToSignalQuality.GetNext(posIAQ);

            ATL::CStringW cswWlanNetworkName = "";
            cswWlanNetworkName = CA2W(reinterpret_cast<LPCSTR>(this->lstWlanNetworks->Network[csaIAQ[1]].dot11Ssid.ucSSID));
            
            if (cswWlanNetworkName.IsEmpty())
                cswWlanNetworkName.LoadStringW(IDS_WLANWIZ_HIDDEN_NETWORK);
            else
                cswWlanNetworkName = cswWlanNetworkName.Left(DOT11_SSID_MAX_LENGTH);

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
        WlanGetAvailableNetworkList(cwwThis->hWlanClient, &gWlanDeviceID, 0, NULL, &cwwThis->lstWlanNetworks);
    }

    return FALSE;
}