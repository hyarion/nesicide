/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once

#include "FamiTrackerDoc.h"

// List control states
#define LCTRL_CHECKBOX_STATE		0x3000
#define LCTRL_CHECKBOX_CHECKED		0x2000
#define LCTRL_CHECKBOX_UNCHECKED	0x1000

class CSequence;

class CInstrumentEditPanel : public CDialog
{
	DECLARE_DYNAMIC(CInstrumentEditPanel)

public:
	CInstrumentEditPanel(UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditPanel();
	virtual int GetIDD() const = 0;
	virtual TCHAR *GetTitle() const = 0;

	// These must be implemented
	virtual void SelectInstrument(int Instrument) = 0;

public:
	bool m_bShow;

protected:
	CFamiTrackerDoc *GetDocument();

	virtual void PreviewNote(unsigned char Key);
	virtual void PreviewRelease(unsigned char Key);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Virtual but not pure
	virtual void OnKeyReturn();		// Called when return is pressed

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
};

// Adds some functions for sequences
class CSequenceInstrumentEditPanel : public CInstrumentEditPanel 
{
   Q_OBJECT
   // Qt interfaces
protected:
   void contextMenuEvent(QContextMenuEvent *event);

public:
   DECLARE_DYNAMIC(CSequenceInstrumentEditPanel)

public:
	CSequenceInstrumentEditPanel(UINT nIDTemplate, CWnd* pParent);
	virtual ~CSequenceInstrumentEditPanel();

	virtual void SetSequenceString(CString Sequence, bool Changed) = 0;

	virtual void TranslateMML(CString String, CSequence *pSequence, int Max, int Min);

protected:
	virtual void PreviewNote(unsigned char Key);
	virtual void PreviewRelease(unsigned char Key);

protected:
	CSequence *m_pSequence;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRClickInstSettings(NMHDR* pNMHDR, LRESULT* pResult);

};
