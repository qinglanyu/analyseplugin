/* -------------------------------------
This file is part of AnalysePlugin for NotePad++ 
Copyright (c) 2022 Matthias H. mattesh(at)gmx.net

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
------------------------------------- */
/**
tclTableview implements the WINAPI functionality for the table of patterns 
in the find config dock window
*/
//#include "stdafx.h"
#include "resource.h"
#include "tclTableview.h"
#include "tclPatternList.h"
#include "tclResultList.h"
#include "chardefines.h"
#include <commctrl.h>// For ListView control APIs
#include "myDebug.h"

#define MAX_CHAR_CELL 1000 // max chars in a cell including \0

// use teColumnNums to address the columns
const tstPatternConfTab tclTableview::gPatternConfTab[tclTableview::TBLVIEW_COL_MAX] = {
#ifdef COL_NUMBERING
    {TEXT("#"), 20 },
#endif
    {TEXT("Hits"),   0 }
   ,{TEXT("Active"), 20 }
   ,{TEXT("Order"),  30}
   ,{TEXT("Search"), 100}
   ,{TEXT("Group"),  50 }
#ifdef RESULT_COLORING
   ,{TEXT("Color"),  40 }
   ,{TEXT("BgCol"),  40 }
#endif
   ,{TEXT("Type"),   50 }
   ,{TEXT("Case"),   20 }
   ,{TEXT("Word"),   20 }
   ,{TEXT("Select"), 35 }
   ,{TEXT("Hide"),   20 }
#ifdef RESULT_STYLING
   ,{TEXT("Bold"),   20 }
   ,{TEXT("Italic"), 20 }
   ,{TEXT("Underl."),20 }
#endif
   ,{TEXT("Comment"),200 }
};

generic_string tclTableview::getCell(int item, int column) const {
   static TCHAR cs[MAX_CHAR_CELL];
   ListView_GetItemText(mhList, item, column, cs, MAX_CHAR_CELL);
   TCHAR* cc = (TCHAR*)cs;
   int j = (int)generic_strlen(cs)+1; 
   j = (j>MAX_CHAR_CELL)?MAX_CHAR_CELL:j;
   for(int i=0; i<j; ++i) {
      cc[i] = (TCHAR)cs[i];
   }
   return generic_string(cc);
}

void tclTableview::refillTable(tclPatternList& pl) {
   if(mhList==0) {
      return; // not initialized
   }
   ListView_DeleteAllItems(mhList);
   tclPatternList::const_iterator iPattern = pl.begin();
   int item=0;
   for (; iPattern != pl.end(); ++iPattern) {
      const tclPattern& rp = iPattern.getPattern();
      LVITEM lvi;
      ZeroMemory(&lvi, sizeof(lvi));
      lvi.mask = LVIF_TEXT;
      // lvi.pszText = rp.getSearchText().c_str();
      lvi.iItem = pl.getPatternIndex(iPattern.getPatId());
      item = ListView_InsertItem(mhList, &lvi);
      updateRow(item, rp);
   }
   setOrderNumRowDefaultWidth(pl);
}
int tclTableview::insertRow(){
   if(mhList==0) {
      return -1; // not initialized
   }
   LVITEM lvi;
   ZeroMemory(&lvi, sizeof(lvi));
   lvi.iItem = getSelectedRow();
   if(lvi.iItem == -1) {
      lvi.iItem =0;
   }
   lvi.mask = LVIF_TEXT;
   return ListView_InsertItem(mhList, &lvi);
}

int tclTableview::insertAfterRow(){
   if(mhList==0) {
      return -1; // not initialized
   }
   LVITEM lvi;
   ZeroMemory(&lvi, sizeof(lvi));
   lvi.iItem = getSelectedRow();
   if(lvi.iItem == -1) {
      lvi.iItem = getRowCount();
   } else {
      ++lvi.iItem;
   }
   lvi.mask = LVIF_TEXT;
   return ListView_InsertItem(mhList, &lvi);
}

void tclTableview::updateRow(int item, const tclPattern& rp) {
   //updateRowColor(item, rp);
#ifdef COL_NUMBERING
   TCHAR num[10];
   updateCell(item, TBLVIEW_COL_NUM, generic_itoa(item, num, 10));
#endif
   updateCell(item, TBLVIEW_COL_HITS, generic_string(TEXT(""))); // TODO test if pattern is dirty before resetting.
   updateCell(item, TBLVIEW_COL_ORDER_NUM, rp.getOrderNumStr());
   updateCell(item, TBLVIEW_COL_DO_SEARCH, rp.getDoSearch()?TEXT("X"):TEXT(""));
   updateCell(item, TBLVIEW_COL_SEARCH_TEXT, rp.getSearchText());
   updateCell(item, TBLVIEW_COL_SEARCH_TYPE, rp.getSearchTypeStr());
   updateCell(item, TBLVIEW_COL_MATCHCASE, rp.getIsMatchCase()?TEXT("X"):TEXT(""));
   updateCell(item, TBLVIEW_COL_WHOLEWORD, rp.getIsWholeWord()?TEXT("X"):TEXT(""));
   updateCell(item, TBLVIEW_COL_SELECT, rp.getSelectionTypeStr());
   updateCell(item, TBLVIEW_COL_HIDE, rp.getIsHideText()?TEXT("X"):TEXT(""));
   updateCell(item, TBLVIEW_COL_COMMENT, rp.getComment());
   updateCell(item, TBLVIEW_COL_GROUP, rp.getGroup());
#ifdef RESULT_STYLING
   updateCell(item, TBLVIEW_COL_BOLD, rp.getIsBold()?TEXT("X"):TEXT(""));
   updateCell(item, TBLVIEW_COL_ITALIC, rp.getIsItalic()?TEXT("X"):TEXT(""));
   updateCell(item, TBLVIEW_COL_UNDERLINED, rp.getIsUnderlined()?TEXT("X"):TEXT(""));
#endif
#ifdef RESULT_COLORING
   updateCell(item, TBLVIEW_COL_COLOR, rp.getColorStr());
   updateCell(item, TBLVIEW_COL_BGCOLOR, rp.getBgColorStr());
#endif
}

void tclTableview::updateCell(int item, int column, const generic_string& s){
   //static TCHAR cs[MAX_CHAR_CELL];
   //int j = (int)s.length()+1; 
   //j = (j>MAX_CHAR_CELL)?MAX_CHAR_CELL:j;
   //for(int i=0; i<j; ++i) {cs[i] = (TCHAR)(s[i]&0xff);}
   ListView_SetItemText(mhList, item, column, (TCHAR*)s.c_str());
}

void tclTableview::setHitsRowVisible(bool bVisible, const tclResultList& results) {
   mbHitsVisible = bVisible;
   int max = 0; // TODO change to size_t and check max line numbers in Scintilla
   TCHAR num[20];
   int row = 0;
   for (tclResultList::const_iterator it = results.begin(); it != results.end(); ++it , ++row) {
      if (bVisible && (results.getPattern(it.getPatId()).getDoSearch())) {
         const tclResult& r = it.getResult();
         int n = (int)r.getPositions().size();
		 // TODO insert check for value overrun 
         generic_itoa(n, num, 10);
         updateCell(row, TBLVIEW_COL_HITS, num);
         max = (n > max) ? n : max;
      }
      else {
         updateCell(row, TBLVIEW_COL_HITS, TEXT(""));
      }
   }
   miHitsCountColSize =   (max<10) ? 20 :
                           (max<1000) ? 30 :
                           (max<10000) ? 35 :
                           (max<100000) ? 40 : 50;
   ListView_SetColumnWidth(mhList, TBLVIEW_COL_HITS, bVisible ? miHitsCountColSize : 0);
}

void tclTableview::setGroupColumnVisible(bool bVisible) {
   if (bVisible) {
      ListView_SetColumnWidth(mhList, TBLVIEW_COL_GROUP, mGroupHideColWidth);
      mGroupHideColWidth = 0;
   }
   else {
      mGroupHideColWidth = ListView_GetColumnWidth(mhList, TBLVIEW_COL_GROUP);
      ListView_SetColumnWidth(mhList, TBLVIEW_COL_GROUP, 0);
   }
}

void tclTableview::setOrderNumRowVisible(bool bVisible) {
   if (bVisible) {
      ListView_SetColumnWidth(mhList, TBLVIEW_COL_ORDER_NUM, mOrderNumHideColWidth);
      mOrderNumHideColWidth = 0;
   }
   else {
      mOrderNumHideColWidth = ListView_GetColumnWidth(mhList, TBLVIEW_COL_ORDER_NUM);
      ListView_SetColumnWidth(mhList, TBLVIEW_COL_ORDER_NUM, 0);
   }
}
void tclTableview::checkGroupColVisibility(const tclPatternList& patterns) {
   bool bHasGroup = false;
   for (tclPatternList::const_iterator it = patterns.begin(); it != patterns.end(); ++it) {
      if (it.getPattern().getGroup().length() > 0) {
         bHasGroup = true;
         break;
      }
   }
   if (bHasGroup) {
      if (!isGroupRowVisible()) {
         setGroupColumnVisible();
      }
      else if (0 == ListView_GetColumnWidth(mhList, TBLVIEW_COL_GROUP)) {
         mGroupHideColWidth = gPatternConfTab[TBLVIEW_COL_GROUP].iColumnSize;
         setGroupColumnVisible();
      } // else do nothing in case that column is already visible
   }
   else {
      if (isGroupRowVisible()) {
         setGroupColumnVisible(false);
      }
   }
}

void tclTableview::checkOrderNumRowVisibility(const tclPatternList& patterns) {
   bool hasOrderNum = false;
   for (tclPatternList::const_iterator it = patterns.begin(); it != patterns.end(); ++it) {
      if (it.getPattern().getOrderNumStr().length() > 0) {
         hasOrderNum = true;
         break;
      }
   }
   if (hasOrderNum) {
      if (!isOrderNumRowVisible()) {
         setOrderNumRowVisible();
      }
      else if(0 == ListView_GetColumnWidth(mhList, TBLVIEW_COL_ORDER_NUM)){
         setOrderNumRowDefaultWidth(patterns);
         setOrderNumRowVisible();
      } // else do nothing in case that column is already visible
   }
   else {
      if (isOrderNumRowVisible()) {
         setOrderNumRowVisible(false);
      }
   }
}

void tclTableview::setOrderNumRowDefaultWidth(const tclPatternList& patterns) {
   size_t max = 0;
   for (tclPatternList::const_iterator it = patterns.begin(); it != patterns.end(); ++it) {
      if (it.getPattern().getOrderNumStr().length() > max) {
         max = it.getPattern().getOrderNumStr().length();
      }
   }
   miOrderNumColSize = (max < 2) ? 20 :
      (max < 3) ? 30 :
      (max < 5) ? 35 :
      (max < 7) ? 40 : 50;
}

void tclTableview::create(){
   if(mhList == 0) {
      return; // not initialized
   }
   ListView_SetExtendedListViewStyle(mhList,
      LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);
   //::SendMessage(mhList,GWL_STYLE,lngOldStyle | LVS_SHOWSELALWAYS );
   LVCOLUMN lvc = {0};
   lvc.mask = LVCF_TEXT  | LVCF_WIDTH;
   for (int col = 0; col < TBLVIEW_COL_MAX; ++col) {
      lvc.pszText = gPatternConfTab[col].szColumnName;
      lvc.cx = mColumnWidth[col];
      ListView_InsertColumn(mhList, col, &lvc);
   }
   ListView_SetColumnOrderArray(mhList, TBLVIEW_COL_MAX, mColumnOrder);
}

void tclTableview::resetTableColumns(bool bUpdateWindow) {
   for (int i = 0; i < TBLVIEW_COL_MAX; ++i) {
      mColumnWidth[i] = gPatternConfTab[i].iColumnSize;
      mColumnOrder[i] = i;
   }
   if (bUpdateWindow) {
      ListView_SetColumnOrderArray(mhList, TBLVIEW_COL_MAX, mColumnOrder);
      for (int i = 0; i < TBLVIEW_COL_MAX; ++i) {
         if (i != TBLVIEW_COL_HITS) {
            ListView_SetColumnWidth(mhList, i, mColumnWidth[i]);
         }
      }
      ListView_SetColumnWidth(mhList, TBLVIEW_COL_HITS, isHitsRowVisible() ? miHitsCountColSize : 0);
      ListView_SetColumnWidth(mhList, TBLVIEW_COL_ORDER_NUM, isOrderNumRowVisible() ? miOrderNumColSize : 0);
      ListView_SetColumnWidth(mhList, TBLVIEW_COL_GROUP, isGroupRowVisible() ? gPatternConfTab[TBLVIEW_COL_GROUP].iColumnSize : 0);
      ::InvalidateRgn(mhList, 0, TRUE);
      ::UpdateWindow(mhList);
   }
}

// has to be called before create()
void tclTableview::setTableColumns(const generic_string& str) {
   TCHAR tmp[MAX_CHAR_CELL];
   generic_strncpy(tmp, str.c_str(), MAX_CHAR_CELL);
   TCHAR* colWidth = generic_strtok(tmp, TEXT(","));
   int col = 0;
   while (colWidth && col < TBLVIEW_COL_MAX) {
      mColumnWidth[col] = generic_atoi(colWidth);
      colWidth = generic_strtok(NULL, TEXT(","));
      ++col;
   }
}

generic_string tclTableview::getTableColumns() const {
   generic_string res;
   TCHAR tmp[20];
   for (int col = 0; col < TBLVIEW_COL_MAX; ++col) {
      tmp[0] = 0;
      int width = (int)(mhList?ListView_GetColumnWidth(mhList, col):gPatternConfTab[col].iColumnSize);
      if (width > 90000) {
         width = 90000;
      }
      generic_itoa(width, tmp, 10);
      res += tmp;
      if (col < (TBLVIEW_COL_MAX-1)) {
         //no comma at end
         res += generic_string(TEXT(","));
      }
   }
   return res;
}

// ListView_SetColumnOrderArray(mTableView.getListViewHandle(), tclTableview::TBLVIEW_ITM_MAX, colOrder);
void tclTableview::setTableColumnOrder(const generic_string& str) {
   TCHAR tmp[MAX_CHAR_CELL];
   generic_strncpy(tmp, str.c_str(), MAX_CHAR_CELL);
   TCHAR* colOrder = generic_strtok(tmp, TEXT(","));
   int col = 0;
   while (colOrder && col < TBLVIEW_COL_MAX) {
      mColumnOrder[col] = generic_atoi(colOrder);
      colOrder = generic_strtok(NULL, TEXT(","));
      ++col;
   }
   DBG1("setTableColumnOrder: done %s", str.c_str());
}

generic_string tclTableview::getTableColumnOrder() const {
   generic_string res;
   TCHAR tmp[20];
   int colOrder[TBLVIEW_COL_MAX];
   memset(colOrder, 0, sizeof(int) * TBLVIEW_COL_MAX);
   ListView_GetColumnOrderArray(mhList, TBLVIEW_COL_MAX, colOrder);
   bool valid = true;
   for (int col = 0; col < TBLVIEW_COL_MAX; ++col) {
      if (colOrder[col] < 0) {
         valid = false;
      }
   }
   for (int col = 0; col < TBLVIEW_COL_MAX; ++col) {
      tmp[0] = 0;
      if(valid) {
         generic_itoa(colOrder[col], tmp, 10);
      } else {
         generic_itoa(col, tmp, 10);
      }
      res += tmp;
      if (col < (TBLVIEW_COL_MAX - 1)) {
         //no comma at end
         res += generic_string(TEXT(","));
      }
   }
   return res;
}


#ifdef COL_NUMBERING
generic_string tclTableview::getItemNumStr() const { return getItem(TBLVIEW_COL_NUM);}
#endif
generic_string tclTableview::getHitsStr() const { return getItem(TBLVIEW_COL_HITS); }
generic_string tclTableview::getOrderNumStr() const { return getItem(TBLVIEW_COL_ORDER_NUM); }
generic_string tclTableview::getDoSearchStr() const { return getItem(TBLVIEW_COL_DO_SEARCH);}
generic_string tclTableview::getSearchTextStr() const { return getItem(TBLVIEW_COL_SEARCH_TEXT);}
generic_string tclTableview::getSearchTypeStr() const {return getItem(TBLVIEW_COL_SEARCH_TYPE);}
generic_string tclTableview::getMatchCaseStr() const {return getItem(TBLVIEW_COL_MATCHCASE );}
generic_string tclTableview::getWholeWordStr() const {return getItem(TBLVIEW_COL_WHOLEWORD);}
generic_string tclTableview::getSelectStr() const {return getItem(TBLVIEW_COL_SELECT );}
generic_string tclTableview::getHideStr() const {return getItem(TBLVIEW_COL_HIDE);}
generic_string tclTableview::getCommentStr() const {return getItem(TBLVIEW_COL_COMMENT);}
generic_string tclTableview::getGroupStr() const { return getItem(TBLVIEW_COL_GROUP);}
#ifdef RESULT_STYLING
generic_string tclTableview::getBoldStr() const {return getItem(TBLVIEW_COL_BOLD);}
generic_string tclTableview::getItalicStr() const {return getItem(TBLVIEW_COL_ITALIC );}
generic_string tclTableview::getUnderlinedStr() const {return getItem(TBLVIEW_COL_UNDERLINED );}
#endif
#ifdef RESULT_COLORING
generic_string tclTableview::getColorStr() const {return getItem(TBLVIEW_COL_COLOR );}
generic_string tclTableview::getBgColorStr() const {return getItem(TBLVIEW_COL_BGCOLOR );}
#endif

int tclTableview::getSelectedRow() const {
   if(mhList) {
      return ListView_GetSelectionMark(mhList); // 0-n entry ; -1 nothing
   } else {
      return -1; // nothing selected
   }
}

// returns the row selected before
int tclTableview::setSelectedRow(int index) const {
   if(mhList) {
      int prevPos = ListView_SetSelectionMark(mhList, index); // 0-n entry ; -1 nothing      
      ListView_SetItemState(mhList, index, (LVIS_FOCUSED|LVIS_SELECTED),(LVIS_FOCUSED|LVIS_SELECTED));
      ListView_EnsureVisible(mhList, index, FALSE);
      return prevPos;
   } else {
      return -1; // nothing selected
   }
}

int tclTableview::getRowCount() const {
   if(mhList) {
      return ListView_GetItemCount(mhList); // 0-n entries 
   } else {
      return 0; // nothing 
   }
}

void tclTableview::setRowItems(const tclPattern& pattern){
   if(mhList==0) {
      return;
   }
   if(getSelectedRow()!=-1){
      updateRow(getSelectedRow(), pattern);
   }
}

void tclTableview::removeRow(int row) {
   if(mhList==0) {
      return;
   }
   if(getRowCount()>row && row>=0) {
      ListView_DeleteItem(mhList, row);
   }
}

void tclTableview::removeAll() {
   ListView_DeleteAllItems(mhList);
}
