/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume notification icon handler
 * COPYRIGHT:   Copyright 2014-2015 David Quintana <gigaherz@gmail.com>
 *              Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "precomp.h"

#include <mmddk.h>

HICON g_hIconVolume;
HICON g_hIconMute;

HMIXER g_hMixer;
DWORD  g_mixerLineID;
DWORD  g_muteControlID;

UINT g_mmDeviceChange;

static BOOL g_IsMute = FALSE;

static HRESULT __stdcall Volume_FindMixerControl(CSysTray * pSysTray)
{
    if (g_hMixer)
        mixerClose(g_hMixer);

    WCHAR pszWinMMErrText[MAXERRORLENGTH] = {0};

    MMRESULT result = mixerOpen(&g_hMixer, 0, (DWORD_PTR)pSysTray->GetHWnd(), 0, MIXER_OBJECTF_HMIXER | CALLBACK_WINDOW);
    if (result != MMSYSERR_NOERROR)
    {
        waveOutGetErrorTextW(result, pszWinMMErrText, _countof(pszWinMMErrText));
        ERR("Volume_FindDefaultMixer: mixerOpen failed: %lu (%S)\n", result, pszWinMMErrText);
        return E_FAIL;
    }

    MIXERLINEW mixerLine;
    mixerLine.cbStruct = sizeof(mixerLine);
    mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    result = mixerGetLineInfoW((HMIXEROBJ)g_hMixer, &mixerLine, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (result != MMSYSERR_NOERROR)
    {
        waveOutGetErrorTextW(result, pszWinMMErrText, _countof(pszWinMMErrText));
        ERR("Volume_FindDefaultMixer: mixerGetLineInfoW failed: %lu (%S)\n", result, pszWinMMErrText);
        return E_FAIL;
    }

    g_mixerLineID = mixerLine.dwLineID;

    MIXERLINECONTROLSW mixerLineControls;
    MIXERCONTROLW mixerControl;
    mixerLineControls.cbStruct = sizeof(mixerLineControls);
    mixerLineControls.dwLineID = mixerLine.dwLineID;
    mixerLineControls.cControls = 1;
    mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    mixerLineControls.pamxctrl = &mixerControl;
    mixerLineControls.cbmxctrl = sizeof(mixerControl);

    if (mixerGetLineControlsW((HMIXEROBJ)g_hMixer, &mixerLineControls, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE))
        return E_FAIL;

    TRACE("Volume_FindDefaultMixer: Found control id %d for mute\n", mixerControl.dwControlID);

    g_muteControlID = mixerControl.dwControlID;

    return S_OK;
}

HRESULT Volume_IsMute()
{
    MIXERCONTROLDETAILS mixerControlDetails;

    if (g_hMixer != NULL && g_muteControlID != (DWORD)-1)
    {
        MIXERCONTROLDETAILS_BOOLEAN detailsResult;
        mixerControlDetails.cbStruct = sizeof(mixerControlDetails);
        mixerControlDetails.hwndOwner = 0;
        mixerControlDetails.dwControlID = g_muteControlID;
        mixerControlDetails.cChannels = 1;
        mixerControlDetails.paDetails = &detailsResult;
        mixerControlDetails.cbDetails = sizeof(detailsResult);
        if (mixerGetControlDetailsW((HMIXEROBJ)g_hMixer, &mixerControlDetails, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE))
            return E_FAIL;

        TRACE("Obtained mute status %d\n", detailsResult);

        g_IsMute = detailsResult.fValue == 1;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE Volume_Init(_In_ CSysTray * pSysTray)
{
    HRESULT hr;
    WCHAR strTooltip[128];

    TRACE("Volume_Init\n");
    g_mmDeviceChange = RegisterWindowMessageW(L"winmm_devicechange");

    if (!g_hMixer)
    {
        hr = Volume_FindMixerControl(pSysTray);
        if (FAILED(hr))
            return hr;
    }

    g_hIconVolume = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_VOLUME));
    g_hIconMute = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_VOLMUTE));

    Volume_IsMute();

    HICON icon;
    if (g_IsMute)
        icon = g_hIconMute;
    else
        icon = g_hIconVolume;

    LoadStringW(g_hInstance, IDS_VOL_VOLUME, strTooltip, _countof(strTooltip));
    return pSysTray->NotifyIcon(NIM_ADD, ID_ICON_VOLUME, icon, strTooltip);
}

HRESULT STDMETHODCALLTYPE Volume_Update(_In_ CSysTray * pSysTray)
{
    BOOL PrevState;

    TRACE("Volume_Update\n");

    PrevState = g_IsMute;
    Volume_IsMute();

    if (PrevState != g_IsMute)
    {
        WCHAR strTooltip[128];
        HICON icon;
        if (g_IsMute)
        {
            icon = g_hIconMute;
            LoadStringW(g_hInstance, IDS_VOL_MUTED, strTooltip, _countof(strTooltip));
        }
        else
        {
            icon = g_hIconVolume;
            LoadStringW(g_hInstance, IDS_VOL_VOLUME, strTooltip, _countof(strTooltip));
        }

        return pSysTray->NotifyIcon(NIM_MODIFY, ID_ICON_VOLUME, icon, strTooltip);
    }
    else
    {
        return S_OK;
    }
}

HRESULT STDMETHODCALLTYPE Volume_Shutdown(_In_ CSysTray * pSysTray)
{
    TRACE("Volume_Shutdown\n");

    return pSysTray->NotifyIcon(NIM_DELETE, ID_ICON_VOLUME, NULL, NULL);
}

HRESULT Volume_OnDeviceChange(_In_ CSysTray * pSysTray, WPARAM wParam, LPARAM lParam)
{
    return Volume_FindMixerControl(pSysTray);
}

static void _RunVolume(BOOL bTray)
{
    ShellExecuteW(NULL,
                  NULL,
                  L"sndvol32.exe",
                  bTray ? L"/t" : NULL,
                  NULL,
                  SW_SHOWNORMAL);
}

static void _RunMMCpl()
{
    CSysTray::RunDll("mmsys.cpl", "");
}

static void _ShowContextMenu(CSysTray * pSysTray)
{
    WCHAR strAdjust[128];
    WCHAR strOpen[128];
    LoadStringW(g_hInstance, IDS_VOL_OPEN, strOpen, _countof(strOpen));
    LoadStringW(g_hInstance, IDS_VOL_ADJUST, strAdjust, _countof(strAdjust));

    HMENU hPopup = CreatePopupMenu();
    AppendMenuW(hPopup, MF_STRING, IDS_VOL_OPEN, strOpen);
    AppendMenuW(hPopup, MF_STRING, IDS_VOL_ADJUST, strAdjust);
    SetMenuDefaultItem(hPopup, IDS_VOL_OPEN, FALSE);

    DWORD flags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN;
    POINT pt;
    SetForegroundWindow(pSysTray->GetHWnd());
    GetCursorPos(&pt);

    DWORD id = TrackPopupMenuEx(hPopup, flags,
        pt.x, pt.y,
        pSysTray->GetHWnd(), NULL);

    DestroyMenu(hPopup);

    switch (id)
    {
    case IDS_VOL_OPEN:
        _RunVolume(FALSE);
        break;
    case IDS_VOL_ADJUST:
        _RunMMCpl();
        break;
    }
}

HRESULT STDMETHODCALLTYPE Volume_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult)
{
    if (uMsg == g_mmDeviceChange)
        return Volume_OnDeviceChange(pSysTray, wParam, lParam);

    switch (uMsg)
    {
        case WM_USER + 220:
            TRACE("Volume_Message: WM_USER+220\n");
            if (wParam == VOLUME_SERVICE_FLAG)
            {
                if (lParam)
                {
                    pSysTray->EnableService(VOLUME_SERVICE_FLAG, TRUE);
                    return Volume_Init(pSysTray);
                }
                else
                {
                    pSysTray->EnableService(VOLUME_SERVICE_FLAG, FALSE);
                    return Volume_Shutdown(pSysTray);
                }
            }
            return S_FALSE;

        case WM_USER + 221:
            TRACE("Volume_Message: WM_USER+221\n");
            if (wParam == VOLUME_SERVICE_FLAG)
            {
                lResult = (LRESULT)pSysTray->IsServiceEnabled(VOLUME_SERVICE_FLAG);
                return S_OK;
            }
            return S_FALSE;

        case WM_TIMER:
            if (wParam == VOLUME_TIMER_ID)
            {
                KillTimer(pSysTray->GetHWnd(), VOLUME_TIMER_ID);
                _RunVolume(TRUE);
            }
            break;

        case ID_ICON_VOLUME:
            TRACE("Volume_Message uMsg=%d, w=%x, l=%x\n", uMsg, wParam, lParam);

            Volume_Update(pSysTray);

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    SetTimer(pSysTray->GetHWnd(), VOLUME_TIMER_ID, GetDoubleClickTime(), NULL);
                    break;

                case WM_LBUTTONUP:
                    break;

                case WM_LBUTTONDBLCLK:
                    KillTimer(pSysTray->GetHWnd(), VOLUME_TIMER_ID);
                    _RunVolume(FALSE);
                    break;

                case WM_RBUTTONDOWN:
                    break;

                case WM_RBUTTONUP:
                    _ShowContextMenu(pSysTray);
                    break;

                case WM_RBUTTONDBLCLK:
                    break;

                case WM_MOUSEMOVE:
                    break;

                case MM_MIXM_LINE_CHANGE:
                {
                    DPRINTF("MM_MIXM_LINE_CHANGE received: hMixer %lx; dwLineID %lu", wParam, lParam);
                    break;
                }
            }
            return S_OK;

        default:
            TRACE("Volume_Message received for unknown ID %d, ignoring.\n");
            return S_FALSE;
    }

    return S_FALSE;
}
