/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Scanning Routines)
 * COPYRIGHT:   Copyright 2024-2025 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "main.h"

#include <algorithm>
#include <numeric>
#include <set>
#include <string>
#include <vector>

LRESULT CWlanWizard::OnScanNetworks(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MSG msg;
    GUID gCurAdapter;
    PWLAN_RADIO_STATE pWRSCurAdapter = NULL;
    WLAN_OPCODE_VALUE_TYPE wovtCurrAdapter = wlan_opcode_value_type_invalid;
    DWORD dwWRSsize = sizeof(WLAN_RADIO_STATE);

    this->uScanStatus = STATUS_SCANNING;
    this->bScanTimeout = FALSE;
    this->dwSelectedItemID = -1;

    m_ConnectButton.EnableWindow(FALSE);
    m_ListboxWLAN.EnableWindow(FALSE);

    /* Clear listbox from previously discovered networks */
    m_ListboxWLAN.SendMessageW(LB_RESETCONTENT, NULL, NULL);
    m_ListboxWLAN.Invalidate();

    /* Check if the adapter is enabled, both hardware and software */
    IIDFromString(this->m_sGUID, &gCurAdapter);
    WlanQueryInterface(this->hWlanClient,
                       &gCurAdapter,
                       wlan_intf_opcode_radio_state,
                       NULL,
                       &dwWRSsize,
                       reinterpret_cast<PVOID*>(&pWRSCurAdapter),
                       &wovtCurrAdapter);

    DPRINT("WLAN adapter radio status: hardware %s, software %s\n",
        pWRSCurAdapter->PhyRadioState[0].dot11HardwareRadioState == dot11_radio_state_on ? "on" : "off",
        pWRSCurAdapter->PhyRadioState[0].dot11SoftwareRadioState == dot11_radio_state_on ? "on" : "off");

    if (!(   pWRSCurAdapter->PhyRadioState[0].dot11SoftwareRadioState == dot11_radio_state_on
          && pWRSCurAdapter->PhyRadioState[0].dot11HardwareRadioState == dot11_radio_state_on))
    {
        this->uScanStatus = STATUS_SCAN_COMPLETE;
        this->bScanTimeout = TRUE;

        if (this->lstWlanNetworks != NULL)
        {
            WlanFreeMemory(this->lstWlanNetworks);
            RtlSecureZeroMemory(&this->lstWlanNetworks, sizeof(this->lstWlanNetworks));
        }
        return FALSE;
    }

    m_SidebarButtonAS.EnableWindow(FALSE);
    m_SidebarButtonSN.EnableWindow(FALSE);

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

    DPRINT("Discovered %lu access points\n", this->lstWlanNetworks->dwNumberOfItems);
     
    if (this->lstWlanNetworks->dwNumberOfItems > 0)
    {
        auto vecIndexesBySignalQuality = std::vector<DWORD>(this->lstWlanNetworks->dwNumberOfItems);
        DWORD dwConnectedTo = MAXDWORD;
        std::set<DWORD> setDiscoveredAdHocIndexes;
        std::iota(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), 0);

        /* Sort networks by signal level */
        std::sort(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), [&](DWORD left, DWORD right)
        {
            WLAN_AVAILABLE_NETWORK wlanLeft = this->lstWlanNetworks->Network[left];
            WLAN_AVAILABLE_NETWORK wlanRight = this->lstWlanNetworks->Network[right];
            
            if (wlanLeft.dot11BssType == dot11_BSS_type_independent)
                setDiscoveredAdHocIndexes.insert(left);

            if (wlanLeft.dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
                dwConnectedTo = left;

            return wlanLeft.wlanSignalQuality > wlanRight.wlanSignalQuality;
        });

        /* Shift all ad hoc networks to end */
        if (setDiscoveredAdHocIndexes.size() > 0)
        {
            for (const auto& dwAdHocIdx : setDiscoveredAdHocIndexes)
            {
                auto iter = std::find(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), dwAdHocIdx);

                if (iter != vecIndexesBySignalQuality.end())
                {
                    auto idx = iter - vecIndexesBySignalQuality.begin();
                    std::rotate(vecIndexesBySignalQuality.begin() + idx, vecIndexesBySignalQuality.begin() + idx + 1, vecIndexesBySignalQuality.end());
                }
            }
        }

        /* Finally, move currently connected network to beginning */
        if (dwConnectedTo != MAXDWORD)
        {
            auto connectedIdx = std::find(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), dwConnectedTo) - vecIndexesBySignalQuality.begin();
            std::rotate(vecIndexesBySignalQuality.begin() + connectedIdx, vecIndexesBySignalQuality.begin() + connectedIdx + 1, vecIndexesBySignalQuality.end());
        }

        /* TODO: remove networks that do not have a saved profile matching the SSID */
        for (const auto& dwNetwork : vecIndexesBySignalQuality)
        {
            WLAN_AVAILABLE_NETWORK wlanNetwork = this->lstWlanNetworks->Network[dwNetwork];
            std::wstring_view wsvSSID;

            // Convert SSID from UTF-8 to UTF-16
            int iSSIDLengthWide = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(wlanNetwork.dot11Ssid.ucSSID), wlanNetwork.dot11Ssid.uSSIDLength, NULL, 0);

            ATL::CStringW cswSSID = ATL::CStringW(L"", iSSIDLengthWide);
            MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(wlanNetwork.dot11Ssid.ucSSID), wlanNetwork.dot11Ssid.uSSIDLength, cswSSID.GetBuffer(), iSSIDLengthWide);
            wsvSSID = std::wstring_view(cswSSID.GetBuffer());

            if (cswSSID.IsEmpty())
                cswSSID.LoadStringW(IDS_WLANWIZ_HIDDEN_NETWORK);

            LRESULT iItemIdx = m_ListboxWLAN.SendMessageW(LB_INSERTSTRING,
                -1,
                reinterpret_cast<LPARAM>(cswSSID.GetBuffer()));

            m_ListboxWLAN.SendMessageW(LB_SETITEMDATA,
                iItemIdx,
                static_cast<LPARAM>(dwNetwork));
        }
    }

    /* Show discovered networks */
    RECT rc;
    m_ListboxWLAN.EnableWindow();
    m_ListboxWLAN.GetClientRect(&rc);
    m_ListboxWLAN.Invalidate();
    m_ListboxWLAN.SetFocus();

    m_ConnectButton.EnableWindow();
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