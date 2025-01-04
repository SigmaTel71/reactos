/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections
 * COPYRIGHT:   Copyright 2024-2025 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#pragma once
#include <set>

#include <atlbase.h>
#include <atlcoll.h>
#include <atlconv.h>
#include <atlstr.h>
#include <atlwin.h>
#include <debug.h>
#include <strsafe.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wlanapi.h>
#include <vssym32.h>
#ifdef __REACTOS__
#include <atlsimpcoll.h>
#include <cguid.h>
#include <shellapi.h>
#include <shlguid_undoc.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#endif
#include <uxtheme.h>

#include "resource.h"

#define IDT_SCANNING_NETWORKS 700

enum WLAN_SCAN_STATES
{
	STATUS_SCAN_COMPLETE,
	STATUS_SCANNING,
};

static struct ExplorerInstance : public IUnknown
{
	HWND m_hWnd;
	volatile LONG m_rc;

	ExplorerInstance() : m_hWnd(NULL), m_rc(1) {}
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
	{
		const QITAB rgqit[] = { { 0 } };
		return QISearch(this, rgqit, riid, ppv);
	}
	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return InterlockedIncrement(&m_rc);
	}
	virtual ULONG STDMETHODCALLTYPE Release()
	{
		ULONG r = InterlockedDecrement(&m_rc);
		if (!r)
			PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
		return r;
	}
	void Wait()
	{
		SHSetInstanceExplorer(NULL);
		m_hWnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", NULL, WS_POPUP,
			0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
		BOOL loop = InterlockedDecrement(&m_rc) != 0;
		MSG msg;
		while (loop && (int)GetMessageW(&msg, NULL, 0, 0) > 0)
		{
			if (msg.hwnd == m_hWnd && msg.message == WM_CLOSE)
				PostMessageW(m_hWnd, WM_QUIT, 0, 0);
			DispatchMessageW(&msg);
		}
	}
} g_EI;

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
		COMMAND_ID_HANDLER(IDC_WLANWIZ_LISTBOX, OnListBox);

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

	HINSTANCE wlanwiz_hInstance = NULL;
	HANDLE hMutex = INVALID_HANDLE_VALUE;
	BOOL FindWlanDevice(ATL::CString wsGUID = L"");
	void PreCloseCleanup();

private:
	BOOL bScanTimeout = TRUE;
	BOOL bMouseOverButtons = FALSE;
	DWORD dwNegotiatedVersion = 0;
	HANDLE hWlanClient = INVALID_HANDLE_VALUE;
	HANDLE hScanThread = INVALID_HANDLE_VALUE;
	/* We can't have one HTHEME to rule them all.
	 * ExplorerBar is added for now, later expansion will require more handles to use.
	 */
	HTHEME hThemeEB = NULL;
	LPOLESTR m_sGUID = NULL;
	PWLAN_INTERFACE_INFO_LIST lstWlanInterfaces = NULL;
	PWLAN_AVAILABLE_NETWORK_LIST lstWlanNetworks = NULL;
	UINT uScanStatus = STATUS_SCAN_COMPLETE;
	
	/* Sidebar button specific variables */
	LOGFONT lfCaption = { 0 };
	ATL::CAtlList<int> LSidebarBtns;
	ATL::CWindow cPrevWnd;
	WPARAM wPrevCtlID = 0;

	/* Listbox specific variables */
	DWORD dwSelectedItemID = 0;

	HWND CreateToolTip(int toolID);
	static DWORD WINAPI ScanNetworksThread(_In_ LPVOID lpParameter);
	void TryInsertToAdHoc(std::set<DWORD>& setAdHoc, DWORD dwIndex);
	void TryInsertToKnown(std::set<DWORD>& setProfiles, DWORD dwIndex);
	DWORD TryFindConnected(DWORD dwIndex);

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
	LRESULT OnListBox(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	
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