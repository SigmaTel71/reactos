/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Scanning Routines)
 * COPYRIGHT:   Copyright 2024-2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

#include <algorithm>
#include <numeric>
#include <vector>

WINE_DEFAULT_DEBUG_CHANNEL(wlanwiz);

LRESULT
CWlanWizard::OnScanNetworks(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MSG msg;
    GUID gCurAdapter;
    PWLAN_RADIO_STATE pWRSCurAdapter = NULL;
    WLAN_OPCODE_VALUE_TYPE wovtCurrAdapter = wlan_opcode_value_type_invalid;
    DWORD dwWRSsize = sizeof(WLAN_RADIO_STATE);

    m_uScanStatus = STATUS_SCANNING;
    m_bScanTimeout = FALSE;
    m_dwSelectedItemID = -1;

    m_ConnectButton.EnableWindow(FALSE);
    m_ListboxWLAN.EnableWindow(FALSE);

    /* Clear listbox from previously discovered networks */
    m_ListboxWLAN.SendMessageW(LB_RESETCONTENT, NULL, NULL);
    m_ListboxWLAN.Invalidate();

    /* Check if the adapter is enabled, both hardware and software */
    IIDFromString(m_sGUID, &gCurAdapter);
    DWORD dwResult = WlanQueryInterface(m_hWlanClient,
                                        &gCurAdapter,
                                        wlan_intf_opcode_radio_state,
                                        NULL,
                                        &dwWRSsize,
                                        reinterpret_cast<PVOID*>(&pWRSCurAdapter),
                                        &wovtCurrAdapter);

    if (pWRSCurAdapter == NULL)
    {
        WARN("Cannot determine radio state due to 0x%lx\n", dwResult);
    }
    else
    {
        DOT11_RADIO_STATE rsHw = pWRSCurAdapter->PhyRadioState[0].dot11HardwareRadioState;
        DOT11_RADIO_STATE rsSw = pWRSCurAdapter->PhyRadioState[0].dot11SoftwareRadioState;

        TRACE("WLAN adapter radio status: hardware %s, software %s\n",
            rsHw == dot11_radio_state_on ? "on" : "off (or unknown)",
            rsSw == dot11_radio_state_on ? "on" : "off (or unknown)");
        
        if (!(rsSw == dot11_radio_state_on && rsHw == dot11_radio_state_on))
        {
            m_uScanStatus = STATUS_SCAN_COMPLETE;
            m_bScanTimeout = TRUE;

            if (m_lstWlanNetworks != NULL)
            {
                WlanFreeMemory(m_lstWlanNetworks);
                RtlSecureZeroMemory(&m_lstWlanNetworks, sizeof(m_lstWlanNetworks));
            }
            return FALSE;
        }
    }

    m_SidebarButtonAS.EnableWindow(FALSE);
    m_SidebarButtonSN.EnableWindow(FALSE);

    SetTimer(IDT_SCANNING_NETWORKS, 5000);

    HCURSOR hOldCursor = SetCursor(LoadCursorW(NULL, IDC_APPSTARTING));

    m_hScanThread = CreateThread(NULL, NULL, &ScanNetworksThread, reinterpret_cast<LPVOID>(this), NULL, NULL);

    while (m_bScanTimeout == FALSE)
    {
        switch (MsgWaitForMultipleObjects(0, &m_hScanThread, FALSE, INFINITE, QS_ALLINPUT))
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

    m_uScanStatus = STATUS_SCAN_COMPLETE;

    SetCursor(hOldCursor);

    m_SidebarButtonAS.EnableWindow();
    m_SidebarButtonSN.EnableWindow();
     
    if (m_lstWlanNetworks->dwNumberOfItems > 0)
    {
        auto vecIndexesBySignalQuality = std::vector<DWORD>(m_lstWlanNetworks->dwNumberOfItems);
        DWORD dwConnectedTo = MAXDWORD;
        std::set<DWORD> setDiscoveredAdHocIndexes;
        std::set<DWORD> setAPsWithProfiles;
        std::iota(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), 0);

        /* Sort networks by signal level */
        std::sort(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), [&](auto left, auto right)
        {
            TryInsertToAdHoc(setDiscoveredAdHocIndexes, left);
            TryInsertToAdHoc(setDiscoveredAdHocIndexes, right);

            /* Try to determine if we are connected currently to anything.
             * Once found, these two steps are skipped. */
            if (dwConnectedTo == MAXDWORD)
                dwConnectedTo = TryFindConnected(left);
            
            if (dwConnectedTo == MAXDWORD)
                dwConnectedTo = TryFindConnected(right);

            /* Count network as known if it fully matches SSID with profile name */
            TryInsertToKnown(setAPsWithProfiles, left);
            TryInsertToKnown(setAPsWithProfiles, right);

            return m_lstWlanNetworks->Network[left].wlanSignalQuality > m_lstWlanNetworks->Network[right].wlanSignalQuality;
        });

        /* Remove networks that do not have profile name exactly matching SSID */
        for (const auto& dwKnownAPIdx : setAPsWithProfiles)
        {
            PWLAN_AVAILABLE_NETWORK wlanNetWithProfile = &m_lstWlanNetworks->Network[dwKnownAPIdx];

            vecIndexesBySignalQuality.erase(std::remove_if(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), [&](const DWORD& dwAP)
            {
                if (dwKnownAPIdx == dwAP)
                    return false;

                bool bProfileNameIsSSID = ATL::CStringW(wlanNetWithProfile->strProfileName) == APNameToUnicode(&m_lstWlanNetworks->Network[dwAP].dot11Ssid);
                bool bHasProfile = (m_lstWlanNetworks->Network[dwAP].dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE) != 0;

                return bProfileNameIsSSID && !bHasProfile;
            }), vecIndexesBySignalQuality.end());
        }

        TRACE("Discovered %lu access points (%u are known)\n", m_lstWlanNetworks->dwNumberOfItems, setAPsWithProfiles.size());
        
        /* Shift all ad hoc networks to end */
        for (const auto& dwAdHocIdx : setDiscoveredAdHocIndexes)
        {
            auto iter = std::find(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), dwAdHocIdx);

            if (iter != vecIndexesBySignalQuality.end())
            {
                auto idx = iter - vecIndexesBySignalQuality.begin();
                std::rotate(vecIndexesBySignalQuality.begin() + idx, vecIndexesBySignalQuality.begin() + idx + 1, vecIndexesBySignalQuality.end());
            }
        }

        /* Finally, move currently connected network to beginning */
        if (dwConnectedTo != MAXDWORD)
        {
            auto connectedIdx = std::find(vecIndexesBySignalQuality.begin(), vecIndexesBySignalQuality.end(), dwConnectedTo) - vecIndexesBySignalQuality.begin();

            if (connectedIdx > 0)
            {
                auto iter = vecIndexesBySignalQuality.begin();
                std::rotate(iter, iter + connectedIdx, iter + connectedIdx + 1);
            }
        }

        for (const auto& dwNetwork : vecIndexesBySignalQuality)
        {
            PWLAN_AVAILABLE_NETWORK pWlanNetwork = &m_lstWlanNetworks->Network[dwNetwork];
            ATL::CStringW cswSSID = APNameToUnicode(&pWlanNetwork->dot11Ssid);

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
    m_ListboxWLAN.EnableWindow();
    m_ListboxWLAN.Invalidate();
    m_ListboxWLAN.SetFocus();

    m_ConnectButton.EnableWindow();
    return FALSE;
}

DWORD
WINAPI CWlanWizard::ScanNetworksThread(_In_ LPVOID lpParameter)
{
    GUID gWlanDeviceID = { 0 };
    CWlanWizard* cwwThis = reinterpret_cast<CWlanWizard*>(lpParameter);
    
    if (IIDFromString(cwwThis->m_sGUID, &gWlanDeviceID) == S_OK)
    {
        WlanScan(cwwThis->m_hWlanClient, &gWlanDeviceID, NULL, NULL, NULL);
        WlanGetAvailableNetworkList(cwwThis->m_hWlanClient, &gWlanDeviceID, 0, NULL, &cwwThis->m_lstWlanNetworks);
    }

    return FALSE;
}
