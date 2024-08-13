/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Advanced Settings)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "main.h"
#ifndef __REACTOS__
typedef HRESULT (WINAPI* _SHInvokeCommand)(HWND, IShellFolder*, LPCITEMIDLIST, LPCSTR);
#endif

LRESULT CWlanWizard::OnAdvancedSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HRESULT hr = ERROR_SUCCESS;
	LPITEMIDLIST lpConnFolderPIDL = SHCloneSpecialIDList(NULL, CSIDL_CONNECTIONS, FALSE), lpConnItemPIDL = NULL;
	LPCITEMIDLIST lpChildList = NULL;

	if (FAILED(hr))
		return FALSE;

	/* Convert our WLAN adapter GUID from text to respective object */
	GUID gWlanAdapter = GUID_NULL;
	hr = IIDFromString(this->m_sGUID, &gWlanAdapter);

	if (FAILED(hr))
		return FALSE;

	/* Get into 'Network Connections' folder to find our WLAN device */
	CComPtr<IShellFolder> pShfParent, pShfConn;
	CComPtr<IEnumIDList> pEnum;

	if (SUCCEEDED(SHBindToParent(lpConnFolderPIDL, IID_IShellFolder, reinterpret_cast<PVOID*>(&pShfParent.p), &lpChildList)))
	{
		pShfParent->BindToObject(lpChildList, NULL, IID_IShellFolder, reinterpret_cast<PVOID*>(&pShfConn.p));
		pShfParent.Release();
	}

	ILFree(lpConnFolderPIDL);

	if (pShfConn == nullptr)
		return FALSE;

	pShfConn->EnumObjects(NULL, SHCONTF_NONFOLDERS, &pEnum);

	while (pEnum->Next(1, &lpConnItemPIDL, NULL) == S_OK)
	{
		PNETCONIDSTRUCT nfid = reinterpret_cast<PNETCONIDSTRUCT>(lpConnItemPIDL);
		if (!IsEqualGUID(gWlanAdapter, nfid->guidId))
			ILFree(lpConnItemPIDL);
		else
			break;
	}

#ifndef __REACTOS__
	HMODULE hmShell32 = GetModuleHandleW(L"shlwapi.dll");

	if (hmShell32 == NULL)
		return FALSE;

	_SHInvokeCommand SHInvokeCommand = reinterpret_cast<_SHInvokeCommand>(GetProcAddress(hmShell32, MAKEINTRESOURCEA(363)));
#endif

	SHInvokeCommand(NULL, pShfConn, lpConnItemPIDL, "properties");

	ILFree(lpConnItemPIDL);
	pShfConn.Release();

	/* Below you can find a rather horrible HACK. */
	this->ShowWindow(SW_HIDE);
	this->SendMessageW(WM_CLOSE, NULL, NULL);

	return FALSE;
}