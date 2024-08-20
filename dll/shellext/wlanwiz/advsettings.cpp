/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Advanced Settings)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "main.h"

LRESULT CWlanWizard::OnAdvancedSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	ATL::CStringW cswNetworkAdapterPath = L"";
	LPOLESTR lpwszNetConCLSID;
#if (_WIN32_WINNT < _WIN32_WINNT_VISTA) || !defined(__REACTOS__)
	HRESULT hr = StringFromIID(CLSID_NetworkConnections, &lpwszNetConCLSID);
#else
	HRESULT hr = StringFromIID(CLSID_ConnectionFolder, &lpwszNetConCLSID);
#endif
	if (SUCCEEDED(hr))
	{
		cswNetworkAdapterPath.Format(L"::%s\\::%s", lpwszNetConCLSID, this->m_sGUID);

		/* NT 5.1 & 5.2 would still try to open the property sheet if you unplug the adapter. */
		ShellExecuteW(NULL, L"properties", cswNetworkAdapterPath, NULL, NULL, SW_SHOWNORMAL);
	}

	this->ShowWindow(SW_HIDE);
	this->SendMessageW(WM_CLOSE);

	return FALSE;
}