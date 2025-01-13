/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (Helper Functions)
 * COPYRIGHT:   Copyright 2024-2025 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

ATL::CStringW CWlanWizard::APNameToUnicode(_In_ PDOT11_SSID pDot11Ssid)
{
    int iSSIDLengthWide = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(pDot11Ssid->ucSSID), pDot11Ssid->uSSIDLength, NULL, 0);

    ATL::CStringW cswSSID = ATL::CStringW(L"", iSSIDLengthWide);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(pDot11Ssid->ucSSID), pDot11Ssid->uSSIDLength, cswSSID.GetBuffer(), iSSIDLengthWide);

    return cswSSID;
}

void CWlanWizard::TryInsertToKnown(_Inout_ std::set<DWORD>& setProfiles, _In_ DWORD dwIndex)
{
    PWLAN_AVAILABLE_NETWORK pWlanNetwork = &this->lstWlanNetworks->Network[dwIndex];

    if ((pWlanNetwork->dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE) == WLAN_AVAILABLE_NETWORK_HAS_PROFILE)
    {
        if (APNameToUnicode(&pWlanNetwork->dot11Ssid) == pWlanNetwork->strProfileName)
            setProfiles.insert(dwIndex);
    }
}

void CWlanWizard::TryInsertToAdHoc(_Inout_ std::set<DWORD>& setAdHoc, _In_ DWORD dwIndex)
{
    PWLAN_AVAILABLE_NETWORK pWlanNetwork = &this->lstWlanNetworks->Network[dwIndex];

    if (pWlanNetwork->dot11BssType == dot11_BSS_type_independent)
        setAdHoc.insert(dwIndex);
}

DWORD CWlanWizard::TryFindConnected(_In_ DWORD dwIndex)
{
    PWLAN_AVAILABLE_NETWORK pWlanNetwork = &this->lstWlanNetworks->Network[dwIndex];

    if ((pWlanNetwork->dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED) == WLAN_AVAILABLE_NETWORK_CONNECTED)
        return dwIndex;

    return MAXDWORD;
}

HWND CWlanWizard::CreateToolTip(_In_ int nID)
{
    ATL::CStringW cswTooltip;
    BOOL bLoaded = cswTooltip.LoadStringW(nID + 40);

    if (!nID || !bLoaded)
        return FALSE;

    HWND hDlgItem = this->GetDlgItem(nID);
    ATL::CWindow hWndTip = ::CreateWindowExW(NULL,
                                             TOOLTIPS_CLASSW,
                                             NULL,
                                             WS_POPUP,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             this->m_hWnd,
                                             NULL,
                                             wlanwiz_hInstance,
                                             NULL);

    if (!hDlgItem || !hWndTip)
        return NULL;

    TOOLINFOW toolInfo =
    {
        .cbSize = sizeof(toolInfo),
        .uFlags = TTF_IDISHWND | TTF_SUBCLASS,
        .hwnd = this->m_hWnd,
        .uId = reinterpret_cast<UINT_PTR>(hDlgItem),
        .lpszText = cswTooltip.GetBuffer()
    };

    hWndTip.SendMessageW(TTM_ADDTOOL, NULL, reinterpret_cast<LPARAM>(&toolInfo));

    return hWndTip;
}

ATL::CComPtr<IStream> CWlanWizard::CreateDataStream(const PVOID pvData, size_t size)
{
    ATL::CComPtr<IStream> stream;
    HRESULT hr;

    HGLOBAL hGlobal = GlobalAlloc(GHND, size);

    if (FAILED(HRESULT_FROM_WIN32(GetLastError())))
    {
        DPRINT1("GlobalAlloc failed: 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()));
        return nullptr;
    }

    PVOID ptr = GlobalLock(hGlobal);
    memcpy(ptr, pvData, size);

    hr = CreateStreamOnHGlobal(hGlobal, TRUE, &stream);

    if (FAILED(hr))
    {
        DPRINT1("CreateStreamOnHGlobal failed: 0x%lx\n", hr);
        return nullptr;
    }

    GlobalUnlock(hGlobal);

    return stream;
}