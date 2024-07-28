/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WLAN Advanced Settings)
 * COPYRIGHT:   Copyright 2024 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "main.h"

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

	CComPtr<IContextMenu> pcm;
	if (SUCCEEDED(pShfConn->GetUIObjectOf(NULL, 1, const_cast<LPCITEMIDLIST*>(&lpConnItemPIDL), IID_IContextMenu, NULL, reinterpret_cast<PVOID*>(&pcm.p))))
	{
		CMINVOKECOMMANDINFO ici = { sizeof(ici) };
		ici.hwnd = NULL;
		ici.cbSize = sizeof(ici);
		ici.nShow = SW_NORMAL;
		ici.lpVerb = "properties";
		pcm->InvokeCommand(&ici);
		pcm.Release();
	};

	ILFree(lpConnItemPIDL);
	pShfConn.Release();

	/* Below you can find a rather horrible HACK. */
	this->ShowWindow(SW_HIDE);
	this->SendMessageW(WM_CLOSE, NULL, NULL);

	return FALSE;
}