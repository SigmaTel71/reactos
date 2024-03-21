/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections
 * COPYRIGHT:   Copyright 2024-2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"
#include "resource.h"
WINE_DEFAULT_DEBUG_CHANNEL(wlanwiz);

HMODULE g_hModule = NULL;

CWlanWizard gModule;

extern "C"
{

BOOL APIENTRY
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        gModule.wlanwiz_hInstance = hinstDLL;
        DisableThreadLibraryCalls(gModule.wlanwiz_hInstance);
        break;
    }
    default:
        break;
    }
    
    return TRUE;
}

void CALLBACK
WlanWizOpen(HWND, HINSTANCE, LPCSTR lpszCmdLine, int) {
    /* Preventing other instances of wlanwiz dialog to open */
    gModule.hMutex = CreateMutexW(NULL, TRUE, L"wlanwizdlg");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        ATL::CStringW cswCaption = "";
        cswCaption.LoadStringW(IDS_DLGCLASS);

        HWND hDlg = FindWindowW(L"#32770", cswCaption);
        ShowWindow(hDlg, SW_SHOWNORMAL);
        SetForegroundWindow(hDlg);
        SetActiveWindow(hDlg);

        CloseHandle(gModule.hMutex);
        ExitProcess(ERROR_ALREADY_EXISTS);
    }

    if (gModule.FindWlanDevice(lpszCmdLine))
    {
        CoInitialize(nullptr);
        SHSetInstanceExplorer(&g_EI);
        InitCommonControls();

        gModule.DoModal();
    }
    else
        gModule.PreCloseCleanup();
}


}

void
CWlanWizard::PreCloseCleanup()
{
    CoTaskMemFree(gModule.m_sGUID);
    CloseHandle(gModule.hMutex);
    WlanFreeMemory(m_lstWlanInterfaces);
    WlanCloseHandle(m_hWlanClient, NULL);
    g_EI.Wait();
    UnloadDrawableItems();
    CoUninitialize();
}

BOOL
CWlanWizard::FindWlanDevice(ATL::CString sGUID)
{
    /* Create WLAN client handle */
    DWORD dwResult = WlanOpenHandle(WLAN_API_VERSION_1_0, NULL, &m_dwNegotiatedVersion, &m_hWlanClient);
    if (FAILED(dwResult))
    {
        ERR("WlanOpenHandle failed: 0x%lx\n", dwResult);
        return FALSE;
    }
    
    /* Get list of all WLAN devices */
    dwResult = WlanEnumInterfaces(m_hWlanClient, NULL, &m_lstWlanInterfaces);
    if (FAILED(dwResult))
    {
        ERR("WlanEnumInterfaces failed: 0x%lx\n", dwResult);
        return FALSE;
    }
    
    /* If GUID was supplied on start, we will try to find a device that has it. */
    if (!sGUID.IsEmpty())
    {
        /* Convert string GUID to GUID */
        GUID gWlanDeviceID = { 0 };
        dwResult = IIDFromString(sGUID, &gWlanDeviceID);

        /* We expect to see netshell opening WLAN dialog window for networks discovered by device with GUID it got. */
        for (DWORD i = 0; i <= m_lstWlanInterfaces->dwNumberOfItems; i++)
        {
            if (IsEqualGUID(gWlanDeviceID, m_lstWlanInterfaces->InterfaceInfo[i].InterfaceGuid))
            {
                TRACE("Using manually selected adapter %S\n", m_lstWlanInterfaces->InterfaceInfo[i].strInterfaceDescription);
                StringFromIID(gWlanDeviceID, &m_sGUID);
                return TRUE;
            }
        }

        /* Device not found. */
        return FALSE;
    }
    else
    {
        BOOL bWlanDevicePresent = m_lstWlanInterfaces->dwNumberOfItems > 0;
        
        if (bWlanDevicePresent)
        {
            TRACE("Using automatically selected adapter %S\n", m_lstWlanInterfaces->InterfaceInfo[0].strInterfaceDescription);
            dwResult = StringFromIID(m_lstWlanInterfaces->InterfaceInfo[0].InterfaceGuid, &m_sGUID);
        }

        return bWlanDevicePresent;
    }
}

LRESULT
CWlanWizard::OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_hThemeEB = OpenThemeData(m_hWnd, L"ExplorerBar");
    m_hThemeButton = OpenThemeData(m_hWnd, L"Button");

    /* Create a list of sidebar buttons for easier 'batch' calling */
    m_MSidebarBtns.Add(IDC_WLANWIZ_SCAN_NETWORKS, NULL);
    m_MSidebarBtns.Add(IDC_WLANWIZ_INSTALL_WLAN, NULL);
    m_MSidebarBtns.Add(IDC_WLANWIZ_PREFERRED_APS, NULL);
    m_MSidebarBtns.Add(IDC_WLANWIZ_ADVANCED_SETTINGS, NULL);

    /* Choose font appropriate to the hover state */
    m_lfCaption.lfCharSet = DEFAULT_CHARSET;
    m_lfCaption.lfQuality = DEFAULT_QUALITY;
    m_lfCaption.lfHeight = -MulDiv(8, GetDeviceCaps(this->GetDC(), LOGPIXELSY), 72);
    m_lfCaption.lfUnderline = FALSE;

    StringCchCopyW(m_lfCaption.lfFaceName, _countof(m_lfCaption.lfFaceName), L"MS Shell Dlg 2");

    /* Apply 'heading' style to IDD_WLANWIZ_CHOOSE_NET_TITLE */
    LOGFONTW lfTitle = { 0 };    
    lfTitle.lfWeight = FW_BOLD;
    lfTitle.lfCharSet = DEFAULT_CHARSET;
    lfTitle.lfQuality = DEFAULT_QUALITY;

    StringCchCopyW(lfTitle.lfFaceName, _countof(lfTitle.lfFaceName), L"MS Shell Dlg 2");

    HFONT hTitleFont = CreateFontIndirectW(&lfTitle);
    ATL::CWindow cwTitle = GetDlgItem(IDD_WLANWIZ_CHOOSE_NET_TITLE);
    cwTitle.SetFont(hTitleFont);

    /* Assign window caption and taskbar icons */
    SetIcon(LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCE(IDI_MAINICON)), FALSE);
    SetIcon(LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCE(IDI_MAINICON)));

    /* Subclass buttons for mouseover detection. */
    m_SidebarButtonSN.SubclassWindow(GetDlgItem(IDC_WLANWIZ_SCAN_NETWORKS));
    m_SidebarButtonIW.SubclassWindow(GetDlgItem(IDC_WLANWIZ_INSTALL_WLAN));
    m_SidebarButtonPA.SubclassWindow(GetDlgItem(IDC_WLANWIZ_PREFERRED_APS));
    m_SidebarButtonAS.SubclassWindow(GetDlgItem(IDC_WLANWIZ_ADVANCED_SETTINGS));

    /* Subclass discovered networks listbox for drawing text over it when the list is empty */
    m_ListboxWLAN.SubclassWindow(GetDlgItem(IDC_WLANWIZ_LISTBOX));

    /* Subclass 'Connect/Disconnect' button to detect VK_ESCAPE and VK_F5 keystrikes */
    m_ConnectButton.SubclassWindow(GetDlgItem(IDC_WLANWIZ_MAINBUTTON));

    /* Subclass groupboxes to allow themed drawing */
    m_SidebarGroupMain.SubclassWindow(GetDlgItem(IDC_WLANWIZ_MAINGROUP));
    m_SidebarGroupRelated.SubclassWindow(GetDlgItem(IDC_WLANWIZ_RELATEDGROUP));

    /* Perform 'batched' operations to sidebar buttons */
    for (UINT i = 0; i <= m_MSidebarBtns.GetSize() - 1; i++)
    {
        DWORD dwCtrlID = m_MSidebarBtns.GetKeyAt(i);
        
        /* Assign tooltips to each sidebar button */
        CreateToolTip(dwCtrlID);

        /* Load their icons. The +20 offset originates from resource.h, the IDs are laid out in a way
         * they can be safely batch processed in loops, as the range is fixed. */
        if (dwCtrlID == IDC_WLANWIZ_PREFERRED_APS)
            m_MSidebarBtns.SetAt(dwCtrlID,
                                 LoadIconW(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCEW(IDI_SHELL32_FAVORITES)));
        else
            m_MSidebarBtns.SetAt(dwCtrlID, LoadIconW(wlanwiz_hInstance, MAKEINTRESOURCEW(dwCtrlID + 20)));
    }

    PreloadDrawableItems();

    /* Listbox and connect buttons is disabled until we get any content there */
    m_ListboxWLAN.EnableWindow(FALSE);
    m_ConnectButton.EnableWindow(FALSE);

    return FALSE;
}

LRESULT
CWlanWizard::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_SidebarButtonSN.UnsubclassWindow();
    m_SidebarButtonIW.UnsubclassWindow();
    m_SidebarButtonPA.UnsubclassWindow();
    m_SidebarButtonAS.UnsubclassWindow();
    m_ListboxWLAN.UnsubclassWindow();
    m_ConnectButton.UnsubclassWindow();
    m_SidebarGroupMain.UnsubclassWindow();
    m_SidebarGroupRelated.UnsubclassWindow();

    PreCloseCleanup();
    EndDialog(ERROR_SUCCESS);
    return FALSE;
}

LRESULT
CWlanWizard::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_bMouseOverButtons)
    {
        POINT ps;
        GetCursorPos(&ps);
        m_lfCaption.lfUnderline = TRUE;
        m_bMouseOverButtons = TRUE;

        /* Only one button can have hover underline, or none.
         * We save button window handle now and underline the text under it. */
        m_cPrevWnd = WindowFromPoint(ps);
        m_wPrevCtlID = m_cPrevWnd.GetDlgCtrlID();

        m_cPrevWnd.InvalidateRect(NULL);
    }
    
    SetClassLongPtrW(m_cPrevWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_HAND)));
    
    return FALSE;
}

/* Do not update cursor if the mouse is above sidebar buttons, otherwise it flickers. */
LRESULT
CWlanWizard::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(!m_bMouseOverButtons)
        SetCursor(LoadCursorW(NULL, !m_bScanTimeout ? IDC_APPSTARTING : IDC_ARROW));
    
    return FALSE;
}

LRESULT
CWlanWizard::OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch(wParam)
    {
        case IDT_SCANNING_NETWORKS:
        {
            KillTimer(IDT_SCANNING_NETWORKS);
            m_bScanTimeout = TRUE;
            CloseHandle(m_hScanThread);
        }
    }
    return FALSE;
}

/* The font is reverted from main window procedure because subclassed function detects
 * that cursor is on the button, while the main procedure ensures that it's *not* on the button. */
LRESULT
CWlanWizard::OnMouseMoveMain(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_bMouseOverButtons)
    {
        m_lfCaption.lfUnderline = FALSE;
        m_bMouseOverButtons = FALSE;

        m_cPrevWnd.InvalidateRect(NULL);
    }
    
    SetClassLongPtrW(m_cPrevWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursorW(NULL, IDC_ARROW)));

    return FALSE;
}

LRESULT
CWlanWizard::OnListBox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HDC dcLB = m_ListboxWLAN.GetDC();

    switch (wNotifyCode)
    {
    case LBN_SETFOCUS:
    {
        LRESULT dwItemID = m_ListboxWLAN.SendMessageW(LB_GETCURSEL);

        RECT rcItem;
        m_ListboxWLAN.SendMessageW(LB_GETITEMRECT, dwItemID, (LPARAM)&rcItem);

        rcItem.top += 1;
        rcItem.left += 1;
        rcItem.bottom -= 1;
        rcItem.right -= 1;

        DrawFocusRect(dcLB, &rcItem);
        ReleaseDC(dcLB);
        break;
    }
    case LBN_SELCHANGE:
        LRESULT dwItemID = m_ListboxWLAN.SendMessageW(LB_GETCURSEL);

        if (dwItemID == LB_ERR)
            break;

        m_ListboxWLAN.SendMessageW(LB_SETITEMHEIGHT, m_dwSelectedItemID, 56);
        m_ListboxWLAN.SendMessageW(LB_SETITEMHEIGHT, dwItemID, 136);
        
        m_dwSelectedItemID = static_cast<DWORD>(dwItemID);
        LRESULT itemRealID = m_ListboxWLAN.SendMessageW(LB_GETITEMDATA, dwItemID);
        
        m_ListboxWLAN.Invalidate(FALSE);
        m_ListboxWLAN.UpdateWindow();

        RECT rcItem;
        m_ListboxWLAN.SendMessageW(LB_GETITEMRECT, dwItemID, (LPARAM)&rcItem);
        
        rcItem.top += 1;
        rcItem.left += 1;
        rcItem.bottom -= 1;
        rcItem.right -= 1;
        
        DrawFocusRect(dcLB, &rcItem);

        DWORD dwConnectBtnStringID = IDS_WLANWIZ_CONNECT;
        if (m_lstWlanNetworks->Network[itemRealID].dwFlags & WLAN_AVAILABLE_NETWORK_CONNECTED)
            dwConnectBtnStringID = IDS_WLANWIZ_DISCONNECT;

        ATL::CStringW cswConnectButtonText((LPCWSTR)dwConnectBtnStringID);
        m_ConnectButton.SetWindowTextW(cswConnectButtonText);
        break;
    }

    ReleaseDC(dcLB);
    bHandled = TRUE;
    return 0;
}
