/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"
#include "resource.h"

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

HWND CWlanWizard::CreateToolTip(int nID)
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

    TOOLINFOW toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = this->m_hWnd;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = reinterpret_cast<UINT_PTR>(hDlgItem);
    toolInfo.lpszText = cswTooltip.GetBuffer();

    hWndTip.SendMessageW(TTM_ADDTOOL, NULL, reinterpret_cast<LPARAM>(&toolInfo));

    return hWndTip;
}

void CWlanWizard::PreCloseCleanup()
{
    CoTaskMemFree(gModule.m_sGUID);
    CloseHandle(gModule.hMutex);
    WlanFreeMemory(this->lstWlanInterfaces);
    WlanCloseHandle(this->hWlanClient, NULL);
    g_EI.Wait();
    CoUninitialize();
}

BOOL CWlanWizard::FindWlanDevice(ATL::CString sGUID)
{
    /* Create WLAN client handle */
    DWORD dwResult = WlanOpenHandle(WLAN_API_VERSION_1_0, NULL, &this->dwNegotiatedVersion, &this->hWlanClient);
    if (FAILED(dwResult))
    {
        DPRINT1("WlanOpenHandle failed: 0x%lx\n", dwResult);
        return FALSE;
    }
    
    /* Get list of all WLAN devices */
    dwResult = WlanEnumInterfaces(this->hWlanClient, NULL, &this->lstWlanInterfaces);
    if (FAILED(dwResult))
    {
        DPRINT1("WlanEnumInterfaces failed: 0x%lx\n", dwResult);
        return FALSE;
    }
    
    /* If GUID was supplied on start, we will try to find a device that has it. */
    if (!sGUID.IsEmpty())
    {
        /* Convert string GUID to GUID */
        GUID gWlanDeviceID = { 0 };
        dwResult = IIDFromString(sGUID, &gWlanDeviceID);

        /* We expect to see netshell opening WLAN dialog window for networks discovered by device with GUID it got. */
        for (DWORD i = 0; i <= this->lstWlanInterfaces->dwNumberOfItems; i++)
        {
            if (IsEqualGUID(gWlanDeviceID, this->lstWlanInterfaces->InterfaceInfo[i].InterfaceGuid))
            {
                StringFromIID(gWlanDeviceID, &this->m_sGUID);
                return TRUE;
            }
        }

        /* Device not found. */
        return FALSE;
    }
    else
    {
        BOOL bWlanDevicePresent = this->lstWlanInterfaces->dwNumberOfItems > 0;
        
        if (bWlanDevicePresent)
            dwResult = StringFromIID(this->lstWlanInterfaces->InterfaceInfo[0].InterfaceGuid, &this->m_sGUID);

        return bWlanDevicePresent;
    }
}

LRESULT CWlanWizard::OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    this->hThemeEB = OpenThemeData(m_hWnd, L"ExplorerBar");

    /* Create a list of sidebar buttons for easier 'batch' calling */
    LSidebarBtns.AddTail(IDC_WLANWIZ_SCAN_NETWORKS);
    LSidebarBtns.AddTail(IDC_WLANWIZ_INSTALL_WLAN);
    LSidebarBtns.AddTail(IDC_WLANWIZ_PREFERRED_APS);
    LSidebarBtns.AddTail(IDC_WLANWIZ_ADVANCED_SETTINGS);

    /* Choose font appropriate to the hover state */
    this->lfCaption.lfCharSet = DEFAULT_CHARSET;
    this->lfCaption.lfQuality = DEFAULT_QUALITY;
    this->lfCaption.lfHeight = -MulDiv(8, GetDeviceCaps(this->GetDC(), LOGPIXELSY), 72);
    this->lfCaption.lfUnderline = FALSE;

    StringCchCopyW(this->lfCaption.lfFaceName, _countof(this->lfCaption.lfFaceName), L"MS Shell Dlg 2");

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
    POSITION mapPos = this->LSidebarBtns.GetHeadPosition();

    for (UINT i = 0; i <= this->LSidebarBtns.GetCount() - 1; i++)
    {
        int iCtrlID = this->LSidebarBtns.GetAt(mapPos);
        
        /* Assign tooltips to each sidebar button */
        CreateToolTip(iCtrlID);
        
        this->LSidebarBtns.GetNext(mapPos);
    }

    /* Listbox and connect buttons is disabled until we get any content there */
    m_ListboxWLAN.EnableWindow(FALSE);
    m_ConnectButton.EnableWindow(FALSE);

    return FALSE;
}

LRESULT CWlanWizard::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

LRESULT CWlanWizard::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!this->bMouseOverButtons)
    {
        POINT ps;
        GetCursorPos(&ps);
        this->lfCaption.lfUnderline = TRUE;
        this->bMouseOverButtons = TRUE;

        /* Only one button can have hover underline, or none.
         * We save button window handle now and underline the text under it. */
        this->cPrevWnd = WindowFromPoint(ps);
        this->wPrevCtlID = this->cPrevWnd.GetDlgCtrlID();

        this->cPrevWnd.InvalidateRect(NULL);
    }
    
    SetClassLongPtrW(this->cPrevWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_HAND)));
    
    return FALSE;
}

/* Do not update cursor if the mouse is above sidebar buttons, otherwise it flickers. */
LRESULT CWlanWizard::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(!this->bMouseOverButtons)
        SetCursor(LoadCursorW(NULL, !this->bScanTimeout ? IDC_APPSTARTING : IDC_ARROW));
    
    return FALSE;
}

LRESULT CWlanWizard::OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch(wParam)
    {
        case IDT_SCANNING_NETWORKS:
        {
            KillTimer(IDT_SCANNING_NETWORKS);
            this->bScanTimeout = TRUE;
            CloseHandle(this->hScanThread);
        }
    }
    return FALSE;
}

/* The font is reverted from main window procedure because subclassed function detects
 * that cursor is on the button, while the main procedure ensures that it's *not* on the button. */
LRESULT CWlanWizard::OnMouseMoveMain(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (this->bMouseOverButtons)
    {
        this->lfCaption.lfUnderline = FALSE;
        this->bMouseOverButtons = FALSE;

        this->cPrevWnd.InvalidateRect(NULL);
    }
    
    SetClassLongPtrW(this->cPrevWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursorW(NULL, IDC_ARROW)));

    return FALSE;
}

LRESULT CWlanWizard::OnListBox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    switch (wNotifyCode)
    {
    case LBN_SELCHANGE:
        ATL::CWindow cwLB = hWndCtl;
        LRESULT dwItemID = cwLB.SendMessageW(LB_GETCURSEL, 0, 0);

        if (dwItemID == LB_ERR)
            break;

        cwLB.SendMessageW(LB_SETITEMHEIGHT, this->dwSelectedItemID, 56);
        cwLB.SendMessageW(LB_SETITEMHEIGHT, dwItemID, 136);
        
        this->dwSelectedItemID = static_cast<DWORD>(dwItemID);
        
        cwLB.Invalidate(FALSE);
        cwLB.UpdateWindow();
        break;
    }

    return FALSE;
}