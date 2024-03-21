/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Wizard for Wireless Network Connections (WM_DRAWITEM, sidebar buttons)
 * COPYRIGHT:   Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */
#include "main.h"

void
CWlanWizard::OnSidebarBtnDrawItem(_In_ UINT dwCtlID, _In_ PDRAWITEMSTRUCT pdis)
{
    if (!(pdis->itemAction & (ODA_SELECT | ODA_DRAWENTIRE | ODA_FOCUS)))
        return;

    ATL::CStringW cswWindowText;

    /* Bounding box for button text */
    RECT rcBtnText = pdis->rcItem;
    rcBtnText.left += 24;
    rcBtnText.top += 3;

    GetDlgItemTextW(static_cast<int>(dwCtlID), cswWindowText);

    /* Step 1: Prepare drawing surface */
    HBRUSH hbrRect = reinterpret_cast<HBRUSH>(COLOR_WINDOW);

    if (m_hThemeEB)
    {
        SetBkMode(pdis->hDC, TRANSPARENT);

        COLORREF crFill = 0;
        GetThemeColor(m_hThemeEB, EBP_NORMALGROUPBACKGROUND, 0, TMT_FILLCOLOR, &crFill);
        hbrRect = CreateSolidBrush(crFill);
    }

    FillRect(pdis->hDC, &pdis->rcItem, hbrRect);
    DeleteObject(hbrRect);

    /* Step 2: Draw related icon to the sidebar button */
    DrawIconEx(pdis->hDC, pdis->rcItem.left + 2, pdis->rcItem.top + 2,
               m_MSidebarBtns.Lookup(pdis->CtlID),
               16, 16,
               NULL, NULL,
               DI_NORMAL);

    /* Step 3: Draw button text */
    HFONT hfBtnCaption = NULL;
    LOGFONTW lfBtnCaption = {0};

    if (m_hThemeEB)
    {
        COLORREF crBtnColor = 0;
        /* According to msstyles, the items do not have transparent background,
         * so they have been filled by color to blend in. Same as in wzcdlg. */
        GetThemeColor(m_hThemeEB, EBP_NORMALGROUPBACKGROUND, 0, TMT_TEXTCOLOR, &crBtnColor);

        /* We can't use DrawThemeText directly as it's not aware of underline style? */
        GetThemeFont(m_hThemeEB, pdis->hDC, EBP_NORMALGROUPBACKGROUND, 0, TMT_FONT, &lfBtnCaption);
        SetTextColor(pdis->hDC, crBtnColor);
    }
    else
        lfBtnCaption = m_lfCaption; /* Fall back to default caption font */

    lfBtnCaption.lfUnderline = m_bMouseOverButtons && pdis->CtlID == m_wPrevCtlID;
    hfBtnCaption = CreateFontIndirectW(&lfBtnCaption);

    HGDIOBJ hOld = SelectObject(pdis->hDC, hfBtnCaption);

    DrawTextW(pdis->hDC, cswWindowText, cswWindowText.GetLength(), &rcBtnText, DT_WORDBREAK);

    SelectObject(pdis->hDC, hOld);
    DeleteObject(hfBtnCaption);

    if (pdis->itemState & ODS_FOCUS)
        DrawFocusRect(pdis->hDC, &pdis->rcItem);
}
