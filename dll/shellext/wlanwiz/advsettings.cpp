/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Advanced Settings)
 * COPYRIGHT:   Copyright 2024-2025 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

LRESULT CWlanWizard::OnAdvancedSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	ATL::CStringW cswNetworkAdapterPath = L"";
	LPOLESTR lpwszNetConCLSID;
	CComPtr<IShellFolder> sfConn;
	LPITEMIDLIST pidl = NULL;
	LPCITEMIDLIST pidlChild = NULL;
	SHDESCRIPTIONID did = { 0 };

	if (FAILED(SHGetSpecialFolderLocation(NULL, CSIDL_CONNECTIONS, &pidl)))
		goto Exit;

	if (FAILED(SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<LPVOID*>(&sfConn), &pidlChild)))
		goto Exit;

	if (FAILED(SHGetDataFromIDListW(sfConn, pidlChild, SHGDFIL_DESCRIPTIONID, &did, sizeof(did))))
		goto Exit;

	if (FAILED(StringFromIID(did.clsid, &lpwszNetConCLSID)))
		goto Exit;

	ILFree(pidl);
	sfConn.Release();
	cswNetworkAdapterPath.Format(L"::%s\\::%s", lpwszNetConCLSID, this->m_sGUID);

	/* NT 5.1 & 5.2 would still try to open the property sheet if you unplug the adapter. */
	ShellExecuteW(NULL, L"properties", cswNetworkAdapterPath, NULL, NULL, SW_SHOWNORMAL);

Exit:
	this->ShowWindow(SW_HIDE);
	this->SendMessageW(WM_CLOSE);

	return FALSE;
}