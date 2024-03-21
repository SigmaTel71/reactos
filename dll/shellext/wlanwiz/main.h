/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#pragma once
#include <vector>

#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlconv.h>
#include <atlstr.h>
#include <atlwin.h>
#include <prsht.h>
#include <netcfgx.h>
#include <netcfgn.h>
#include <netcon.h>
#include <strsafe.h>
#ifdef __REACTOS__
#include <cguid.h>
#include <CommCtrl.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#else
DECLARE_INTERFACE_(INetConnectionPropertyUi, IUnknown)
{
	STDMETHOD_(HRESULT, QueryInterface)(THIS_ REFIID riid, void** ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS)  PURE;
	STDMETHOD_(ULONG, Release) (THIS) PURE;
	STDMETHOD_(HRESULT, SetConnection) (THIS_ INetConnection* pCon) PURE;
	STDMETHOD_(HRESULT, AddPages) (THIS_ HWND hwndParent, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define INetConnectionPropertyUi_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define INetConnectionPropertyUi_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define INetConnectionPropertyUi_Release(p)                 (p)->lpVtbl->Release(p)
#define INetConnectionPropertyUi_SetConnection(p,a)         (p)->lpVtbl->SetConnection(p,a)
#define INetConnectionPropertyUi_AddPages(p,a,b,c)          (p)->lpVtbl->AddPages(p,a,b,c)
#endif

EXTERN_C const IID IID_INetConnectionPropertyUi;
#endif
#include <wlanapi.h>
#include <Uxtheme.h>
#include <vssym32.h>

#include "resource.h"

#define IDT_SCANNING_NETWORKS 700
#define MAX_PROPERTY_SHEET_PAGE 10

enum WLAN_SCAN_STATES {
	STATUS_SCAN_COMPLETE,
	STATUS_SCANNING,
};

BOOL CALLBACK PropSheetExCallback(HPROPSHEETPAGE hPage, LPARAM lParam);

class CWlanWizard : public CDialogImpl<CWlanWizard>
{
public:
	enum { IDD = IDD_WLANWIZ_DIALOG };

	CContainedWindow m_SidebarButtonSN;
	CContainedWindow m_SidebarButtonIW;
	CContainedWindow m_SidebarButtonPA;
	CContainedWindow m_SidebarButtonAS;
	CContainedWindow m_ListboxWLAN;
	CContainedWindow m_ConnectButton;

	CContainedWindow m_SidebarGroupMain;
	CContainedWindow m_SidebarGroupRelated;
	
	CWlanWizard() :
		m_SidebarButtonSN(L"BUTTON", this, 1),
		m_SidebarButtonIW(L"BUTTON", this, 1),
		m_SidebarButtonPA(L"BUTTON", this, 1),
		m_SidebarButtonAS(L"BUTTON", this, 1),
		m_ListboxWLAN(L"LISTBOX", this, 2),
		m_ConnectButton(L"BUTTON", this, 3),
		m_SidebarGroupMain(L"BUTTON", this, 4),
		m_SidebarGroupRelated(L"BUTTON", this, 4) {};

	BEGIN_MSG_MAP(CWlanWizard)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem);
		MESSAGE_HANDLER(WM_CLOSE, OnClose);
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMoveMain);
		MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem);
		MESSAGE_HANDLER(WM_PAINT, OnPaint);
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);
		MESSAGE_HANDLER(WM_TIMER, OnTimer);
		MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged);
		MESSAGE_HANDLER(WM_VKEYTOITEM, OnVKeyToItem);
		COMMAND_ID_HANDLER(IDC_WLANWIZ_SCAN_NETWORKS, OnScanNetworks);
		COMMAND_ID_HANDLER(IDC_WLANWIZ_ADVANCED_SETTINGS, OnAdvancedSettings);

	ALT_MSG_MAP(1)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode);
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown);
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove);
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgndGroupBoxBtns);

	ALT_MSG_MAP(2)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCodeLB);
		MESSAGE_HANDLER(WM_PAINT, OnPaintLB);

	ALT_MSG_MAP(3)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode);
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown);

	ALT_MSG_MAP(4)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgndGroupBoxBtns);
		MESSAGE_HANDLER(WM_PAINT, OnPaintGroupBox);

	END_MSG_MAP()

	HINSTANCE wlanwiz_hInstance;
	HANDLE hMutex = INVALID_HANDLE_VALUE;
	BOOL FindWlanDevice(ATL::CString wsGUID = L"");
	void PreCloseCleanup();

private:
	BOOL bScanTimeout = TRUE;
	BOOL bMouseOverButtons = FALSE;
	DWORD dwNegotiatedVersion = 0;
	HANDLE hWlanClient = INVALID_HANDLE_VALUE;
	HANDLE hScanThread = INVALID_HANDLE_VALUE;
	HTHEME hTheme = NULL;
	LPOLESTR sGUID;
	PWLAN_INTERFACE_INFO_LIST lstWlanInterfaces = NULL;
	PWLAN_AVAILABLE_NETWORK_LIST lstWlanNetworks = NULL;
	UINT uScanStatus = STATUS_SCAN_COMPLETE;
	
	/* Sidebar button specific variables */
	LOGFONT lfCaption = { 0 };
	ATL::CAtlList<int> LSidebarBtns;
	ATL::CWindow cPrevWnd;
	WPARAM wPrevCtlID;

	HWND CreateToolTip(int toolID);
	static DWORD WINAPI ScanNetworksThread(_In_ LPVOID lpParameter);

	LRESULT OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDrawItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseMoveMain(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMeasureItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnThemeChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	
	/* Control callbacks */
	LRESULT OnScanNetworks(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAdvancedSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	
	/* ALT_MSG_MAP 1 */
	LRESULT OnEraseBkgndGroupBoxBtns(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnGetDlgCode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	
	/* ALT_MSG_MAP 2 */
	LRESULT OnGetDlgCodeLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnVKeyToItem(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaintLB(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	/* ALT_MSG_MAP 4 */
	LRESULT OnPaintGroupBox(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};