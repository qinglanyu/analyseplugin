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

#ifndef TCLTABLEVIEW_H
#define TCLTABLEVIEW_H

#include <windows.h>
#include <string>
#include "Common.h"

class tclPatternList;
class tclPattern;
class tclResultList;

struct tstPatternConfTab {
   TCHAR* szColumnName;
   int iColumnSize;
};

/**
 * encapsulates all necessary functionality to maintain the list view and its content. 
 */
class tclTableview {
public:
   enum teColumnNums {
#ifdef COL_NUMBERING
      TBLVIEW_COL_NUM,
#endif
      TBLVIEW_COL_HITS,
      TBLVIEW_COL_DO_SEARCH,
      TBLVIEW_COL_ORDER_NUM,
      TBLVIEW_COL_SEARCH_TEXT,
      TBLVIEW_COL_GROUP,
   #ifdef RESULT_COLORING
      TBLVIEW_COL_COLOR,
      TBLVIEW_COL_BGCOLOR,
   #endif
      TBLVIEW_COL_SEARCH_TYPE,
      TBLVIEW_COL_MATCHCASE,
      TBLVIEW_COL_WHOLEWORD,
      TBLVIEW_COL_SELECT,
      TBLVIEW_COL_HIDE,
   #ifdef RESULT_STYLING
      TBLVIEW_COL_BOLD,
      TBLVIEW_COL_ITALIC,
      TBLVIEW_COL_UNDERLINED,
   #endif
      TBLVIEW_COL_COMMENT,
      TBLVIEW_COL_MAX
   };

   tclTableview():mhList(0){
      resetTableColumns();
   }

   ~tclTableview(){
      mhList=0;
   }

   void resetTableColumns(bool bUpdateWindow=false);
   void setListViewHandle(HWND hwnd){
      mhList = hwnd;
   }

   HWND getListViewHandle() const {
      return mhList;
   }

   void refillTable(tclPatternList& pl);

   void create();

   int getSelectedRow() const ;

   int setSelectedRow(int index) const ;

   int getRowCount() const ;

   void setHitsRowVisible(bool bVisible, const tclResultList& results);
   void setGroupColumnVisible(bool bVisible = true);
   void setOrderNumRowVisible(bool bVisible = true);
   void checkGroupColVisibility(const tclPatternList& patterns);
   void checkOrderNumRowVisibility(const tclPatternList& patterns);
   void setOrderNumRowDefaultWidth(const tclPatternList& patterns);
   
   int getGroupHideColWidth() const {
      return mGroupHideColWidth;
   }
   void setGroupHideColWidth(int width) {
      mGroupHideColWidth = width;
   }
      int getOrderNumHideColWidth() const {
      return mOrderNumHideColWidth;
   }
   void setOrderNumHideColWidth(int width) {
      mOrderNumHideColWidth = width;
   }

   bool isGroupRowVisible() const {
      return (mGroupHideColWidth == 0);
   }

   bool isOrderNumRowVisible() const {
      return (mOrderNumHideColWidth == 0);
   }
   bool isHitsRowVisible() const {
      return mbHitsVisible;
   }

   generic_string getSearchTextStr() const ;
   generic_string getSearchTypeStr() const ;
   generic_string getMatchCaseStr() const ;
   generic_string getWholeWordStr() const ;
   generic_string getSelectStr() const ;
   generic_string getHideStr() const ;
   generic_string getCommentStr() const;
   generic_string getGroupStr() const ;
   generic_string getDoSearchStr() const ;
   generic_string getHitsStr() const;
   generic_string getOrderNumStr() const;
#ifdef COL_NUMBERING
   generic_string getItemNumStr() const ;
#endif

#ifdef RESULT_STYLING
   generic_string getBoldStr() const ;
   generic_string getItalicStr() const ;
   generic_string getUnderlinedStr() const ;
#endif
#ifdef RESULT_COLORING
   generic_string getColorStr() const ;
   generic_string getBgColorStr() const ;
#endif

   generic_string getItem(int column) const { 
      if(mhList==0) {
         return generic_string(); // not initialized
      }
      return getCell(getSelectedRow(), column);
   }

   void setTableColumns(const generic_string& str);
   generic_string getTableColumns() const;

   void setTableColumnOrder(const generic_string& str);
   generic_string getTableColumnOrder() const;

   void setRowItems(const tclPattern& pattern);
   int insertRow();
   int insertAfterRow();
   void removeRow(int row);
   void removeAll();

protected:
   static const tstPatternConfTab gPatternConfTab[TBLVIEW_COL_MAX];

   generic_string getCell(int item, int column) const;
   void updateRow(int item, const tclPattern& rp);
   void updateCell(int item, int column, const generic_string& s);
   //void updateRowColor(int item, const tclPattern& rp);

   HWND mhList;
   bool mbHitsVisible = false;
   int miHitsCountColSize = 0;
   int mColumnWidth[TBLVIEW_COL_MAX];
   int mColumnOrder[TBLVIEW_COL_MAX];
   int miOrderNumColSize = 0;
   int mOrderNumHideColWidth = 0; // 0 defines order# column visible >0 takes the width used init before
   int mGroupHideColWidth = 0; // 0 defines order# column visible >0 takes the width used init before
};
#endif //TCLTABLEVIEW_H
