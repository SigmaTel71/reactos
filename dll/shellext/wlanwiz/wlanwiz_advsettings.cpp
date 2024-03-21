#include "main.h"

typedef void (CALLBACK NCFREENETCONPROPERTIES)(NETCON_PROPERTIES* pProps);

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
	switch (uMsg)
	{
	case PSCB_INITIALIZED:
		{
			HICON hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(IDI_MAINICON));
			SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			break;
		}
	}
	return FALSE;
}

LRESULT CWlanWizard::OnAdvancedSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	IEnumNetConnection* pEnum = NULL;
	INetConnection* pNetConn = NULL;
	INetConnectionManager* pNetCM = NULL;
	INetConnectionPropertyUi* pNetCCPui = NULL;
	NETCON_PROPERTIES* pNetProps = NULL;
	
	CLSID clsid = CLSID_NULL;
	HRESULT hr = S_OK;
	ULONG ulCount = 0;

    hr = CNetConnectionManager_CreateInstance(IID_PPV_ARG(INetConnectionManager, &pNetCM));
	//hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_ALL, IID_INetConnectionManager, reinterpret_cast<LPVOID*>(&pNetCM));

	if (FAILED(hr))
		return FALSE;

	hr = pNetCM->EnumConnections(NCME_DEFAULT, &pEnum);

	if (FAILED(hr))
		return FALSE;

	/* Convert our WLAN adapter GUID from text to respective object */
	GUID gWlanAdapter = GUID_NULL;
	IIDFromString(this->sGUID, &gWlanAdapter);

	/* Find our WLAN adapter */
	while (pEnum->Next(1, &pNetConn, &ulCount) == S_OK)
	{
		hr = pNetConn->GetProperties(&pNetProps);

		if (IsEqualGUID(pNetProps->guidId, gWlanAdapter))
			break;
	}

	/* Open property sheet for our WLAN adapter */
	hr = pNetConn->GetUiObjectClassId(&clsid);

	if (FAILED(hr))
		return FALSE;

	hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_INetConnectionPropertyUi, reinterpret_cast<LPVOID*>(&pNetCCPui));

	if (FAILED(hr))
		return FALSE;

	hr = pNetCCPui->SetConnection(pNetConn);

	if (FAILED(hr))
		return FALSE;

	PROPSHEETHEADERW pinfo;
	HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];

	ZeroMemory(&pinfo, sizeof(PROPSHEETHEADERW));
	ZeroMemory(hppages, sizeof(hppages));

	pinfo.dwSize = sizeof(PROPSHEETHEADERW);
	pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_USEICONID | PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_USECALLBACK;
	pinfo.hInstance = wlanwiz_hInstance;
	pinfo.pszIcon = MAKEINTRESOURCEW(IDI_MAINICON);
	pinfo.phpage = hppages;
	pinfo.hwndParent = NULL;
	pinfo.pfnCallback = PropSheetProc;
	pinfo.pszCaption = pNetProps->pszwName;

	hr = pNetCCPui->AddPages(NULL, PropSheetExCallback, reinterpret_cast<LPARAM>(&pinfo));
	if (SUCCEEDED(hr))
	{
		if (PropertySheetW(&pinfo) < 0)
			hr = E_FAIL;
	}
	
	HMODULE hmnetshell = LoadLibraryW(L"netshell.dll");
	NCFREENETCONPROPERTIES* NcFreeNetconProperties = (NCFREENETCONPROPERTIES*)GetProcAddress(hmnetshell, "NcFreeNetconProperties");
	NcFreeNetconProperties(pNetProps);

	return FALSE;
}

BOOL
CALLBACK
PropSheetExCallback(HPROPSHEETPAGE hPage, LPARAM lParam)
{
	PROPSHEETHEADERW* pinfo = (PROPSHEETHEADERW*)lParam;

	if (pinfo->nPages < MAX_PROPERTY_SHEET_PAGE)
	{
		pinfo->phpage[pinfo->nPages++] = hPage;
		return TRUE;
	}
	return FALSE;
}