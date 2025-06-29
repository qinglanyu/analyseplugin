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
tclPattern stores the infos to execute one search
*/

#ifndef TCLPATTERN_H
#define TCLPATTERN_H
#include <windows.h>
#include <string>
#include "Common.h"
#include "tclColor.h"

#define MAX_ORDER_NUM_CHARS 20

/**
* A Pattern is a text and additionaly stores the configuration information for it.
* The text has two visualisation forms 1. the text as shown in find config. 
* 2. the text as a result of unescaping special chars 
*/
class tclPattern {
public:
   enum teSearchType {
      normal,
      escaped,
      regex,
	  rgx_multiline,
      max_searchType
   };

   enum teSelectionType {
      text,
      line,
      max_selectionType
   };


protected:
   // following tables are used to translate enum into text and back
   static const TCHAR*  transSearchType[max_searchType];
   static const TCHAR*  transSelectionType[max_selectionType];
   static const TCHAR*  transBool[2];
   //static unsigned long transColorNum16[16];

public:
   
   tclPattern();

   tclPattern(const tclPattern& right) {
      mOrderNum = right.mOrderNum;
      mDoSearch = right.mDoSearch;
      mSearchText = right.mSearchText;
      mReplaceText = right.mReplaceText;
      mSearchType = right.mSearchType;
      mWholeWord = right.mWholeWord;
      mMatchCase = right.mMatchCase;
      mBold = right.mBold;
      mItalic = right.mItalic;
      mUnderlined = right.mUnderlined;
      mColor = right.mColor;
      mBgColor = right.mBgColor;
      mHideText = right.mHideText;
      mDoReplace = right.mDoReplace;
      mSelectionType = right.mSelectionType;
      mComment = right.mComment;
      mGroup = right.mGroup;
   }

   virtual ~tclPattern();
   
   tclPattern& operator = (const tclPattern& right){
      if(&right == this) {
         return *this;
      }
      mOrderNum = right.mOrderNum;
      mDoSearch = right.mDoSearch;
      mSearchText = right.mSearchText;
      mReplaceText = right.mReplaceText;
      mSearchType = right.mSearchType;
      mWholeWord = right.mWholeWord;
      mMatchCase = right.mMatchCase;
      mBold = right.mBold;
      mItalic = right.mItalic;
      mUnderlined = right.mUnderlined;
      mColor = right.mColor;
      mBgColor = right.mBgColor;
      mHideText = right.mHideText;
      mDoReplace = right.mDoReplace;
      mSelectionType = right.mSelectionType;
      mComment = right.mComment;
      mGroup = right.mGroup;
      return *this;
   }

   bool operator == (const tclPattern& right) const {
      if(&right == this) {
         return true;
      }
      bool bRet =((mOrderNum == right.mOrderNum) &&
                  (mDoSearch == right.mDoSearch) &&
                  (mSearchText == right.mSearchText) &&
                  (mReplaceText == right.mReplaceText) &&
                  (mSearchType == right.mSearchType) &&
                  (mWholeWord == right.mWholeWord) &&
                  (mMatchCase == right.mMatchCase) &&
                  (mBold == right.mBold) &&
                  (mItalic == right.mItalic) &&
                  (mUnderlined == right.mUnderlined) &&
                  (mColor == right.mColor) &&
                  (mBgColor == right.mBgColor) &&
                  (mHideText == right.mHideText) &&
                  (mDoReplace == right.mDoReplace) &&
                  (mSelectionType == right.mSelectionType) &&
                  (mComment == right.mComment) &&
                  (mGroup == right.mGroup));
      return bRet;
   }

   bool isSearchEqual(const tclPattern& right) const {
      if(&right == this) {
         return true;
      }
      bool bRet =((mOrderNum == right.mOrderNum) &&
                  (mDoSearch == right.mDoSearch) &&
                  (mSearchText == right.mSearchText) &&
                  (mSearchType == right.mSearchType) &&
                  (mWholeWord == right.mWholeWord) &&
                  (mMatchCase == right.mMatchCase) &&
                  (mSearchText == right.mSearchText) &&
                  (mDoReplace == right.mDoReplace));
     return bRet;
   }

   bool getDoSearch() const {
      return mDoSearch;
   }   
   
   generic_string getDoSearchStr() const {
      return transBool[mDoSearch];
   }

   const generic_string& getOrderNumStr() const {
      return mOrderNum;
   }

   void setDoSearch(bool bActive) {
      mDoSearch = bActive;
   }

   void setDoSearchStr(const generic_string& val) {
      mDoSearch = convBool(val);
   }

   void setOrderNumStr(const generic_string& val) {
      mOrderNum = val;
   }

   const generic_string& getSearchText() const;
   const generic_string& getComment() const;

   generic_string getReplaceText() const;

   /** used for the search algorithm */
   generic_string getSearchTextConverted() const {
      if(mSearchType==escaped) {
         return convertExtendedToString();
      } else {
         return mSearchText;
      }
   }

   void setSearchText(const generic_string& thisSearchText);
   void setComment(const generic_string& thisComment);

   void setReplaceText(const generic_string& thisReplaceText);

   generic_string getSearchTypeStr() const ;

   teSearchType getSearchType() const{
      return mSearchType;
   }
   
   int getDefSearchTypeListSize() const {
      return max_searchType;
   }

   const TCHAR** getDefSearchTypeList() const ;

   void setSearchTypeStr(const generic_string& type) ;

   void setSearchType(int type) ;
   generic_string getBoldStr() const;

   bool getIsBold() const{
      return mBold;
   }

   void setBold(bool isBold) ;

   void setBoldStr(const generic_string& val){
      mBold = convBool(val);
   }

   generic_string getWholeWordStr() const {
      return transBool[mWholeWord];
   }

   bool getIsWholeWord() const{
      return mWholeWord;
   }

   void setWholeWord(bool isWholeWord) {
      mWholeWord =isWholeWord;
   }

   void setWholeWordStr(const generic_string& val){
      mWholeWord = convBool(val);
   }

   generic_string getMatchCaseStr() const {
      return transBool[mMatchCase];
   }

   bool getIsMatchCase() const{
      return mMatchCase;
   }
   bool getIsHideText() const{
      return mHideText;
   }
   bool getIsReplaceText() const{
      return mDoReplace;
   }
   bool getIsUnderlined() const{
      return mUnderlined;
   }
   bool getIsItalic() const{
      return mItalic;
   }

   void setMatchCase(bool isMatchCase) {
      mMatchCase =isMatchCase;
   }

   void setMatchCaseStr(const generic_string& val){
      mMatchCase = convBool(val);
   }

   generic_string getItalicStr() const;
   void setItalic(bool isItalic) ;

   void setItalicStr(const generic_string& val){
      mItalic = convBool(val);
   }

   generic_string getUnderlinedStr() const;

   void setUnderlined(bool isUnderlined) ;

   void setUnderlinedStr(const generic_string& val){
      mUnderlined = convBool(val);
   }

   generic_string getColorStr()const;
   generic_string getBgColorStr()const;
   
   tColor getColor() const {
      return tclColor::getColRgb(mColor);
   }
   tColor getBgColor() const {
      return tclColor::getColRgb(mBgColor);
   }

   tColor getColorNum() const ;
   tColor getBgColorNum() const ;
   static unsigned long getDefColorNum(int e);

   void setColorStr(const generic_string& color) ;
   void setBgColorStr(const generic_string& color) ;

   void setColor(tColor color) ;
   void setBgColor(tColor color) ;

   static int getDefColorListSize() {
      return tclColor::getDefColorListSize();
   }

   static unsigned long convColorStr2Rgb(const generic_string& color);
   static tColor convColorNum2Enum(unsigned long color);
   //static generic_string convColor2Str(teColor col);

   //static const TCHAR** getDefColorList() ;
   //static const unsigned long* getDefColorNumList() ;

   generic_string getHideTextStr()const;
   generic_string getIsReplaceTextStr()const;

   void setHideText(bool isHideText) ;
   void setIsReplaceText(bool isReplaceText);

   void setIsReplaceTextStr(const generic_string& val) {
      mDoReplace = convBool(val);
   }

   void setHideTextStr(const generic_string& val) {
      mHideText = convBool(val);
   }

   generic_string getSelectionTypeStr()const;

   teSelectionType getSelectionType()const{
      return mSelectionType;
   }

   int getDefSelTypeListSize() const {
      return max_selectionType;
   }

   const TCHAR** getDefSelTypeList() const ;

   void setSelectionType(int selectionType) ;

   void setSelectionTypeStr(const generic_string& type);
   const generic_string& getGroup() const {
      return mGroup;
   }
   void setGroup(const generic_string& str) {
      mGroup = str;
   }
protected:
   /** function is original copy from NPP project to be in syncwith their options */
   generic_string convertExtendedToString() const;
   /** function is original copy from NPP project to be in syncwith their options */
   bool readBase(const TCHAR* string, int* value, int base, int size) const;
   /** used to translate 0 = false and 1 = true */
   bool convBool(const generic_string& val) const;

protected:
   generic_string mOrderNum;
   /** set to true in case that the pattern shall be regarded in the search **/
   bool mDoSearch;
   /** the search pattern as entered by the user */
   generic_string mSearchText;
   /** the replacement text as entered by the user */
   generic_string mReplaceText;
   /** kind of treatment for the search pattern same as in find dialog of NPP */
   teSearchType mSearchType;
   /** true if the found text shall be fitting to a whole word */
   bool mWholeWord;
   /** true if the found text shall be searched case sensitive */
   bool mMatchCase;
   /** true if the found text shall be shown in bold */
   bool mBold;
   /** true if the found text shall be shown in italic */
   bool mItalic;
   /** true if the found text shall be shown underlined  */
   bool mUnderlined;
   /** color in which the found text shall be shown */
   tColor mColor;
   tColor mBgColor;
   /** true if the found text shall be madi invisible */
   bool mHideText;
   bool mDoReplace;
   /** defines if the found text or the whole in which the text was found shall be 
   highlighted with aforesaid atributes */
   teSelectionType mSelectionType;
   /** the comment as entered by the user */
   generic_string mComment;
   /** defines the group to which this pattern belongs */
   generic_string mGroup;
};
#endif //TCLPATTERN_H
