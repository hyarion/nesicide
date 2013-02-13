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

// This is the pattern editor class

#pragma once

#include "SoundGen.h"

enum {TRANSPOSE_DEC_NOTES, TRANSPOSE_INC_NOTES, TRANSPOSE_DEC_OCTAVES, TRANSPOSE_INC_OCTAVES};

#include <QWidget>
#include <QPaintEvent>

#include "FamiTrackerDoc.h"
#include "cqtmfc.h"

// Graphical layout of pattern editor

// Top header (channel names etc)
const int HEADER_HEIGHT = 36 /*+ 16*/;
const int HEADER_CHAN_START = /*16*/ 0;
const int HEADER_CHAN_HEIGHT = 36;
// Row column
const int ROW_COL_WIDTH = 32;
const int ROW_HEIGHT = 12;		// 12
// Patterns
const int CHANNEL_WIDTH = 107;
const int COLUMN_SPACING = 4;
const int CHAR_WIDTH = 10;

// Structure used by clipboard, no pointers allowed here
struct stClipData {
	int	Channels;		// Number of channels copied
	int Rows;			// Number of rows copied
	int StartColumn;	// Start column in first channel
	int EndColumn;		// End column in last channel
	stChanNote Pattern[MAX_CHANNELS][MAX_PATTERN_LENGTH];
};

// Row color cache
struct RowColorInfo_t {
	COLORREF Note;
	COLORREF Instrument;
	COLORREF Volume;
	COLORREF Effect;
	COLORREF Back;
	COLORREF Shaded;
};

// Cursor position
class CCursorPos {
public:
	CCursorPos();
	CCursorPos(int Row, int Channel, int Column);
	const CCursorPos& operator=(const CCursorPos &pos);
	bool Invalid() const;

public:
	int m_iRow;
	int m_iColumn;
	int m_iChannel;
};

// Selection
class CSelection {
public:
	int GetRowStart() const;
	int GetRowEnd() const;
	int GetColStart() const;
	int GetColEnd() const;
	int GetChanStart() const;
	int GetChanEnd() const;
	bool IsWithin(CCursorPos pos) const;
	bool IsSingleChannel() const;

	void SetStart(CCursorPos pos);
	void SetEnd(CCursorPos pos);

public:
	CCursorPos m_cpStart;
	CCursorPos m_cpEnd;
};

// External classes
class CFamiTrackerDoc;
class CFamiTrackerView;

namespace Ui {
class CPatternView;
}

class CPatternView : public CWnd
{
   Q_OBJECT
   
public:
   explicit CPatternView(QWidget *parent = 0);
   ~CPatternView();
	static const unsigned int ROW_PLAY_COLOR = 0x400050;
   
   void SetDocument(CFamiTrackerDoc* pDoc,CFamiTrackerView* pView);
   CFamiTrackerDoc* GetDocument() { return m_pDocument; }
	void AdjustCursor();
	void AdjustCursorChannel();
   
   int GetFrame() const;
	int GetChannel() const;
	int GetRow() const;
	int GetColumn() const;
	int GetPlayFrame() const;
	int GetPlayRow() const;

   void SetBlockStart();
   void SetBlockEnd();

   CSelection GetSelection() const;

   // Selection routines
	void ResetSelection();
	void SetSelectionStart();
	void UpdateSelection();

	void SetSelection(CSelection &selection);
   bool IsSelecting() const;
   void SelectAllChannel();
	void SelectAll();
   
   bool IsPlayCursorVisible() const;

   void ClearSelection();

   void DragPaste(stClipData *pClipData, CSelection *pDragTarget, bool bMix);
   
   void ExpandPattern();
	void ShrinkPattern();

   void ScrollLeft();
	void ScrollRight();
	void ScrollNextChannel();
	void ScrollPreviousChannel();
   
	void MoveToRow(int Row);
	void MoveToFrame(int Frame);
	void MoveToChannel(int Channel);
	void MoveToColumn(int Column);
	void NextFrame();
	void PreviousFrame();

   void Interpolate();
	void Reverse();
	void ReplaceInstrument(int Instrument);

   bool IsColumnSelected(int Column, int Channel) const;
	int  GetSelectColumn(int Column) const;

   void GetVolumeColumn(CString &str) const;

   // Various
	void Transpose(int Type);
	void ScrollValues(int Type);

	// Edit: Copy & paste, selection
	void CopyEntire(stClipData *pClipData);
	void Copy(stClipData *pClipData);
	void Cut();
	void PasteEntire(stClipData *pClipData);
	void Paste(stClipData *pClipData);
	void PasteMix(stClipData *pClipData);
	void DeleteSelectionRows(CSelection &selection);
	void DeleteSelection(CSelection &selection);
	void Delete();
	void RemoveSelectedNotes();
   void ApplyColorScheme();

   // Keys
	void ShiftPressed(bool Pressed);
	void ControlPressed(bool Pressed);

   // Cursor
	void MoveDown(int Step);
	void MoveUp(int Step);
	void MoveLeft();
	void MoveRight();
	void MoveToTop();
	void MoveToBottom();
	void MovePageUp();
	void MovePageDown();
	void NextChannel();
	void PreviousChannel();
	void FirstChannel();
	void LastChannel();
	void MoveChannelLeft();
	void MoveChannelRight();
	void OnHomeKey();
	void OnEndKey();
   bool ScrollTimer();
   void SetDPCMState(stDPCMState State);

   void DrawMeters(CDC *pDC);
   
   bool StepRow();
	bool StepFrame();
	void JumpToRow(int Row);
	void JumpToFrame(int Frame);

   
	// Other
	int GetCurrentPatternLength(int Frame) const;
   void SetHighlight(int Rows, int SecondRows);
   void SetFollowMove(bool bEnable);
   void SetFocus(bool bFocus);

protected:
   void paintEvent(QPaintEvent *event);
   void wheelEvent(QWheelEvent *event);
   void mouseMoveEvent(QMouseEvent *event);
   void mousePressEvent(QMouseEvent *event);
   void mouseReleaseEvent(QMouseEvent *event);
   void mouseDoubleClickEvent(QMouseEvent *event);   
   void resizeEvent(QResizeEvent *event);
   void keyPressEvent(QKeyEvent *event) { event->ignore(); }
   void keyReleaseEvent(QKeyEvent *event) { event->ignore(); }
   
private:
   Ui::CPatternView *ui;
   CFamiTrackerView* m_pView;
   
	static LPCTSTR DEFAULT_HEADER_FONT;
	static const int DEFAULT_FONT_SIZE;
	static const int DEFAULT_HEADER_FONT_SIZE;

	static const int SELECT_THRESHOLD;

   QWidget* songPatterns;
   CFamiTrackerDoc* m_pDocument;
	// Window
	int m_iWinWidth, m_iWinHeight;		// Window height & width
	int	m_iVisibleRows;					// Number of visible rows on screen

   CSelection m_selection;
   CCursorPos m_cpSelCursor;

   void OnMouseScroll(int Delta);
   bool OnMouseHover(UINT nFlags, CPoint point);
   void OnMouseDown(CPoint point);
   void OnMouseRDown(CPoint point);
   void OnMouseUp(CPoint point);
   void OnMouseMove(UINT nFlags, CPoint point);
   void OnMouseDblClk(CPoint point);
   
   void OnVScroll(UINT nSBCode, UINT nPos);
   void OnHScroll(UINT nSBCode, UINT nPos);
   
   void CreateBackground(CDC *pDC, bool bForce);
   void DrawRowArea(CDC *pDC);
   void ClearRow(CDC *pDC, int Line);
   void DrawRow(CDC *pDC, int Row, int Line, int Frame, bool bPreview);
   void DrawChar(int x, int y, TCHAR c, COLORREF Color, CDC *pDC);
   void DrawCell(int PosX, int Column, int Channel, bool bInvert, stChanNote *pNoteData, CDC *pDC, RowColorInfo_t *pColorInfo);
   void DrawHeader(CDC *pDC);
   void DrawChannelNames(CDC *pDC);
   void DrawUnbufferedArea(CDC *pDC);
   void DrawScreen(CDC *pDC/*, CFamiTrackerView *pView*/);
   void FastScrollDown(CDC *pDC);
   void PaintEditor();

   void SetWindowSize(int width, int height);
   
   CCursorPos GetCursorAtPoint(CPoint point) const;
   int  GetRowAtPoint(int PointY) const;
	int  GetColumnAtPoint(int PointX) const;
   int GetChannelAtPoint(int PointX) const;
   bool IsSingleChannelSelection() const;

   void UpdateVerticalScroll();
	void UpdateHorizontalScroll();
   
   // Drag
	CSelection m_selDrag;
	CCursorPos m_cpDragPoint;

	bool m_bSelectionInvalid;
	bool m_bFullRowSelect;

	CPoint m_ptSelStartPoint;

   // Keys
	bool m_bShiftPressed;
	bool m_bControlPressed;

   // GDI objects
	CDC		*m_pBackDC;
	CBitmap *m_pBackBmp;
	CBitmap	m_bmpCache, *m_pOldCacheBmp;
	CFont	m_fontHeader;
	CFont	m_fontPattern;
	CFont	m_fontCourierNew;

   // Colors
	COLORREF m_colEmptyBg;
	COLORREF m_colSeparator;
	COLORREF m_colHead1, m_colHead2, m_colHead3, m_colHead4;

   // Meters and DPCM
	stDPCMState m_DPCMState;

   // Drawing
	int m_iDrawCursorRow;
	int m_iDrawMiddleRow;
	int m_iDrawFrame;
	int m_iFirstChannel;
	int m_iChannelsVisible;
	int m_iHighlight;
	int m_iHighlightSecond;
	int m_iPatternWidth;
	int m_iVisibleWidth;
	int m_iRowHeight;
   
   int m_iMiddleRow;					// The row in the middle of the editor
   
   int m_iChannels;
	int m_iChannelWidths[MAX_CHANNELS];
	int m_iColumns[MAX_CHANNELS];

   int	m_iMouseHoverChan;
	int m_iMouseHoverEffArrow;
	//int m_iMouseClickChan;

	bool m_bForceFullRedraw;
	bool m_bDrawEntire;

   bool m_bHasFocus;
   bool m_bUpdated;
	bool m_bErasedBg;
   
   int m_iPatternFontSize;

   // Selection
	bool m_bSelecting;
	bool m_bCurrentlySelecting;
	bool m_bDragStart;
	bool m_bDragging;
	bool m_bSelectedAll;

   // Scrolling
	CPoint	m_ptScrollMousePos;
	UINT	m_nScrollFlags;
	int		m_iScrolling;
	int		m_iCurrentHScrollPos;
   
   // Edit cursor
	CCursorPos m_cpCursorPos;			// Cursor position

   int	m_iCurrentFrame;
	int m_iPatternLength;
	int	m_iPrevPatternLength;
	int	m_iNextPatternLength;
	int m_iPlayPatternLength;
   
   // Play cursor
	int m_iPlayRow;
	int m_iPlayFrame;
	bool m_bForcePlayRowUpdate;
   
	bool m_bFollowMode;					// Follow mode enable/disable
   
public slots:
   void updateViews(long hint);
private slots:
   void on_verticalScrollBar_actionTriggered(int action);
   void on_horizontalScrollBar_actionTriggered(int action);
};
