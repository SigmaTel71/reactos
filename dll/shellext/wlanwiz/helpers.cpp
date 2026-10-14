/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (Helper Functions)
 * COPYRIGHT:   Copyright 2024-2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"
#include <xmllite.h>

WINE_DEFAULT_DEBUG_CHANNEL(wlanwiz);

ATL::CStringW CWlanWizard::APNameToUnicode(_In_ PDOT11_SSID pDot11Ssid)
{
    int iSSIDLengthWide = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(pDot11Ssid->ucSSID), pDot11Ssid->uSSIDLength, NULL, 0);

    ATL::CStringW cswSSID = ATL::CStringW(L"", iSSIDLengthWide);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCSTR>(pDot11Ssid->ucSSID), pDot11Ssid->uSSIDLength, cswSSID.GetBuffer(), iSSIDLengthWide);

    return cswSSID;
}

void CWlanWizard::TryInsertToKnown(_Inout_ std::set<DWORD>& setProfiles, _In_ DWORD dwIndex)
{
    PWLAN_AVAILABLE_NETWORK pWlanNetwork = &m_lstWlanNetworks->Network[dwIndex];

    if ((pWlanNetwork->dwFlags & WLAN_AVAILABLE_NETWORK_HAS_PROFILE) == WLAN_AVAILABLE_NETWORK_HAS_PROFILE)
    {
        if (APNameToUnicode(&pWlanNetwork->dot11Ssid) == pWlanNetwork->strProfileName)
            setProfiles.insert(dwIndex);
    }
}

void
CWlanWizard::PreloadDrawableItems()
{
    /* Load sidebar icons. The +20 offset originates from resource.h, the IDs are laid out
     * in a way they can be safely batch processed in loops, as the range is fixed. */
    for (int i = 0; i <= m_MSidebarBtns.GetSize() - 1; ++i)
    {
        DWORD dwCtrlID = m_MSidebarBtns.GetKeyAt(i);

        CreateToolTip(dwCtrlID);

        if (dwCtrlID == IDC_WLANWIZ_PREFERRED_APS)
            m_MSidebarBtns.SetAt(dwCtrlID, LoadIconW(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCEW(IDI_SHELL32_FAVORITES)));
        else
            m_MSidebarBtns.SetAt(dwCtrlID, LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCEW(dwCtrlID + 20)));
    }

    /* Load listbox icons */
    for (DWORD dwWlanIconID = IDI_WLANICON; dwWlanIconID <= IDI_WLANICON_100; ++dwWlanIconID)
    {
        m_MListboxIcons.SetAt(dwWlanIconID,
                              LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCEW(dwWlanIconID)));
    }

    m_MListboxIcons.SetAt(IDI_SHELL32_FAVORITES, LoadIconW(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCEW(IDI_SHELL32_FAVORITES)));
    m_MListboxIcons.SetAt(IDI_SHELL32_LOCK, LoadIconW(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCEW(IDI_SHELL32_LOCK)));
    m_MListboxIcons.SetAt(IDI_AP_DHCP_FAILED, LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCEW(IDI_AP_DHCP_FAILED)));
    m_MListboxIcons.SetAt(IDI_BSS_INFRA, static_cast<HICON>(LoadImageW(wlanwiz_hInstance, MAKEINTRESOURCEW(IDI_BSS_INFRA), IMAGE_ICON, 48, 48, LR_LOADTRANSPARENT)));
    m_MListboxIcons.SetAt(IDI_BSS_ADHOC, static_cast<HICON>(LoadImageW(wlanwiz_hInstance, MAKEINTRESOURCEW(IDI_BSS_ADHOC), IMAGE_ICON, 48, 48, LR_LOADTRANSPARENT)));
}

void
CWlanWizard::UnloadDrawableItems()
{
    /* Unload sidebar buttons' icons */
    for (int i = 0; i < m_MSidebarBtns.GetSize() - 1; ++i)
    {
        DWORD dwCtlID = m_MSidebarBtns.GetKeyAt(i);
        DestroyIcon(m_MSidebarBtns.Lookup(dwCtlID));
    }

    for (int i = 0; i < m_MListboxIcons.GetSize() - 1; ++i)
    {
        DWORD dwResID = m_MListboxIcons.GetKeyAt(i);
        DestroyIcon(m_MListboxIcons.Lookup(dwResID));
    }
}

void CWlanWizard::TryInsertToAdHoc(_Inout_ std::set<DWORD>& setAdHoc, _In_ DWORD dwIndex)
{
    PWLAN_AVAILABLE_NETWORK pWlanNetwork = &m_lstWlanNetworks->Network[dwIndex];

    if (pWlanNetwork->dot11BssType == dot11_BSS_type_independent)
        setAdHoc.insert(dwIndex);
}

void CWlanWizard::InitPasswordField()
{
    RECT rcPassword = {.left = 52, .top = 0, .right = 192 + rcPassword.left, .bottom = 20 };
    ATL::CStringW cswCueBanner((LPCWSTR)IDS_WLANWIZ_PASSWORD_CUEBANNER);

    m_PasswordWnd.Create(WC_EDITW, m_ListboxWLAN, &rcPassword, nullptr,
                         WS_CHILD | ES_PASSWORD | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);

    m_PasswordWnd.SendMessageW(EM_SETLIMITTEXT, 63); /* WPA2 and WPA3 do not allow more */
    m_PasswordWnd.SendMessageW(EM_SETCUEBANNER, TRUE, (LPARAM)(LPCWSTR)cswCueBanner);
    m_PasswordWnd.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));
}

void
CALLBACK
CWlanWizard::WlanNotificationCallback(PWLAN_NOTIFICATION_DATA pData, PVOID ctx)
{
    if (!pData)
        return;

    switch (pData->NotificationSource)
    {
        case WLAN_NOTIFICATION_SOURCE_ACM:
        {
            WARN("hello?\n");
            return;
        }
        default:
            return;
    }
}

const bool CWlanWizard::PreferAutoConnection(_In_ PWLAN_AVAILABLE_NETWORK pWlanNetwork)
{
    GUID interfaceGuid;
    LPWSTR pstrProfileXml = NULL;
    DWORD dwFlags = 0, dwGrantedAccess = WLAN_READ_ACCESS;
    IIDFromString(m_sGUID, &interfaceGuid);
    bool preferAuto = false;

    if (SUCCEEDED(WlanGetProfile(m_hWlanClient, &interfaceGuid, pWlanNetwork->strProfileName, NULL, &pstrProfileXml, &dwFlags, &dwGrantedAccess)) && pstrProfileXml != NULL)
    {
        ATL::CComPtr<IStream> xmlStream = CreateDataStream(pstrProfileXml, wcslen(pstrProfileXml));
        ATL::CComPtr<IXmlReader> xmlReader;
        WlanFreeMemory(pstrProfileXml);

        CreateXmlReader(IID_IXmlReader, reinterpret_cast<LPVOID*>(&xmlReader), 0);
        xmlReader->SetInput(xmlStream);

        bool bNodeFound = false;
        XmlNodeType xnType = XmlNodeType_None;
        while (!bNodeFound)
        {
            LPCWSTR pwszLocalName;
            xmlReader->Read(&xnType);

            switch (xnType)
            {
            case XmlNodeType_Element:
            {
                xmlReader->GetLocalName(&pwszLocalName, NULL);

                if (pwszLocalName && CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, pwszLocalName, -1, L"connectionMode", -1) == CSTR_EQUAL)
                {
                    xmlReader->Read(&xnType);
                    LPCWSTR pwszConnectionMode;
                    xmlReader->GetValue(&pwszConnectionMode, NULL);

                    preferAuto = CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, pwszConnectionMode, -1, L"auto", -1) == CSTR_EQUAL;
                    bNodeFound = true;
                }
                break;
            }
            default:
                break;
            }

        }
        xmlReader.Release();
        xmlStream.Release();
    }

    return preferAuto;
}

DWORD CWlanWizard::TryFindConnected(_In_ DWORD dwIndex)
{
    PWLAN_AVAILABLE_NETWORK pWlanNetwork = &m_lstWlanNetworks->Network[dwIndex];

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

    HWND hDlgItem = GetDlgItem(nID);
    ATL::CWindow hWndTip;
    hWndTip.Create(TOOLTIPS_CLASSW, this->m_hWnd, ATL::CWindow::rcDefault, 0, WS_POPUP);

    if (!hDlgItem || !hWndTip)
        return NULL;

    TTTOOLINFOW toolInfo =
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
        ERR("GlobalAlloc failed: 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()));
        return nullptr;
    }

    PVOID ptr = GlobalLock(hGlobal);
    memcpy(ptr, pvData, size);

    hr = CreateStreamOnHGlobal(hGlobal, TRUE, &stream);

    if (FAILED(hr))
    {
        ERR("CreateStreamOnHGlobal failed: 0x%lx\n", hr);
        return nullptr;
    }

    GlobalUnlock(hGlobal);

    return stream;
}
