/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Scanning Routines)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

#include <string>
#include <unordered_set>

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
    DWORD dwResult = WlanQueryInterface(this->hWlanClient,
                       &gCurAdapter,
                       wlan_intf_opcode_radio_state,
                       NULL,
                       &dwWRSsize,
                       reinterpret_cast<PVOID*>(&pWRSCurAdapter),
                       &wovtCurrAdapter);

    if (pWRSCurAdapter == NULL)
    {
        DPRINT1("Cannot determine radio state due to 0x%lx\n", dwResult);
    }
    else
    {
        DOT11_RADIO_STATE rsHw = pWRSCurAdapter->PhyRadioState[0].dot11HardwareRadioState;
        DOT11_RADIO_STATE rsSw = pWRSCurAdapter->PhyRadioState[0].dot11SoftwareRadioState;

        DPRINT("WLAN adapter radio status: hardware %s, software %s\n",
            rsHw == dot11_radio_state_on ? "on" : "off (or unknown)",
            rsSw == dot11_radio_state_on ? "on" : "off (or unknown)");
        
        if (!(rsSw == dot11_radio_state_on && rsHw == dot11_radio_state_on))
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

    /*
     * WIP:  Single pass sorting of discovered networks.
     * TODO: Restore ad-hoc network positioning in the end of the list.
     */     
    if (this->lstWlanNetworks->dwNumberOfItems > 0)
    {
        /* Add discovered networks to listbox and sort networks by signal level */
        std::unordered_set<std::wstring_view> ucAPsWithProfiles;
        WLAN_SIGNAL_QUALITY ulPrevSignalQuality = 0, ulWorstSignalQuality = 100;
                
        for (;
            this->lstWlanNetworks->dwIndex <= this->lstWlanNetworks->dwNumberOfItems - 1;
            ++this->lstWlanNetworks->dwIndex)
        {
            WLAN_AVAILABLE_NETWORK wlanNetwork = this->lstWlanNetworks->Network[this->lstWlanNetworks->dwIndex];
            std::wstring_view wsvSSID;
            int iInsertAt = -1;

            // Convert SSID from UTF-8 to UTF-16
            int iSSIDLengthWide = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(wlanNetwork.dot11Ssid.ucSSID), wlanNetwork.dot11Ssid.uSSIDLength, NULL, 0);

            ATL::CStringW cswSSID = ATL::CStringW(L"", iSSIDLengthWide);
            MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(wlanNetwork.dot11Ssid.ucSSID), wlanNetwork.dot11Ssid.uSSIDLength, cswSSID.GetBuffer(), iSSIDLengthWide);
            wsvSSID = std::wstring_view(cswSSID.GetBuffer());

            // TODO: filter access points by profile
            if (ucAPsWithProfiles.find(wsvSSID) != ucAPsWithProfiles.end())
                continue;

            /* This allows discarding duplicated entries that with APs we had already connected earlier */
            if (wlanNetwork.dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE)
                ucAPsWithProfiles.emplace(wsvSSID);

            /* Sort by descending signal order */
            if (this->lstWlanNetworks->dwIndex == 0 || wlanNetwork.wlanSignalQuality > ulPrevSignalQuality)
                iInsertAt = 0;
            else if (wlanNetwork.wlanSignalQuality < ulPrevSignalQuality)
                iInsertAt = 1;

            if (cswSSID.IsEmpty())
                cswSSID.LoadStringW(IDS_WLANWIZ_HIDDEN_NETWORK);

            LRESULT iItemIdx = m_ListboxWLAN.SendMessageW(LB_INSERTSTRING,
                iInsertAt,
                reinterpret_cast<LPARAM>(cswSSID.GetBuffer()));

            m_ListboxWLAN.SendMessageW(LB_SETITEMDATA,
                iItemIdx,
                static_cast<LPARAM>(this->lstWlanNetworks->dwIndex));

            ulPrevSignalQuality = wlanNetwork.wlanSignalQuality;

            if (ulPrevSignalQuality < ulWorstSignalQuality)
                ulWorstSignalQuality = ulPrevSignalQuality;
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