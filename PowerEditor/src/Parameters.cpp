// This file is part of Notepad++ project
// Copyright (C) 2022 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <time.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "keys.h"
#include "localization.h"
#include "localizationString.h"
// Mattes #include "UserDefineDialog.h"
// Mattes #include "WindowsDlgRc.h"
#include "resource.h" // Mattes

#ifdef _MSC_VER
#pragma warning(disable : 4996) // for GetVersionEx()
#endif

using namespace std;

namespace // anonymous namespace
{


struct WinMenuKeyDefinition //more or less matches accelerator table definition, easy copy/paste
{
	//const TCHAR * name;	//name retrieved from menu?
	int vKey;
	int functionId;
	bool isCtrl;
	bool isAlt;
	bool isShift;
	const TCHAR * specialName;		//Used when no real menu name exists (in case of toggle for example)
};


struct ScintillaKeyDefinition
{
	const TCHAR * name;
	int functionId;
	bool isCtrl;
	bool isAlt;
	bool isShift;
	int vKey;
	int redirFunctionId;	//this gets set  when a function is being redirected through Notepad++ if Scintilla doesnt do it properly :)
};


/*!
** \brief array of accelerator keys for all std menu items
**
** values can be 0 for vKey, which means its unused
*/
static const WinMenuKeyDefinition winKeyDefs[] =
{
	// V_KEY,    COMMAND_ID,                                    Ctrl,  Alt,   Shift, cmdName
	// -------------------------------------------------------------------------------------
	//
	{ VK_N,       IDM_FILE_NEW,                                 true,  false, false, nullptr },
	{ VK_O,       IDM_FILE_OPEN,                                true,  false, false, nullptr },
	{ VK_NULL,    IDM_FILE_OPEN_FOLDER,                         false, false, false, TEXT("Open containing folder in Explorer") },
	{ VK_NULL,    IDM_FILE_OPEN_CMD,                            false, false, false, TEXT("Open containing folder in Command Prompt") },
	{ VK_NULL,    IDM_FILE_OPEN_DEFAULT_VIEWER,                 false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_OPENFOLDERASWORSPACE,                false, false, false, nullptr },
	{ VK_R,       IDM_FILE_RELOAD,                              true,  false, false, nullptr },
	{ VK_S,       IDM_FILE_SAVE,                                true,  false, false, nullptr },
	{ VK_S,       IDM_FILE_SAVEAS,                              true,  true,  false, nullptr },
	{ VK_NULL,    IDM_FILE_SAVECOPYAS,                          false, false, false, nullptr },
	{ VK_S,       IDM_FILE_SAVEALL,                             true,  false, true,  nullptr },
	{ VK_NULL,    IDM_FILE_RENAME,                              false, false, false, nullptr },
	{ VK_W,       IDM_FILE_CLOSE,                               true,  false, false, nullptr },
	{ VK_W,       IDM_FILE_CLOSEALL,                            true,  false, true,  nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_BUT_CURRENT,                false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_TOLEFT,                     false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_TORIGHT,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_CLOSEALL_UNCHANGED,                  false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_DELETE,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_LOADSESSION,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_FILE_SAVESESSION,                         false, false, false, nullptr },
	{ VK_P,       IDM_FILE_PRINT,                               true,  false, false, nullptr },
	{ VK_NULL,    IDM_FILE_PRINTNOW,                            false, false, false, nullptr },
	{ VK_F4,      IDM_FILE_EXIT,                                false, true,  false, nullptr },
	{ VK_T,       IDM_FILE_RESTORELASTCLOSEDFILE,               true,  false, true,  TEXT("Restore Recent Closed File")},

//	{ VK_NULL,    IDM_EDIT_UNDO,                                false, false, false, nullptr },
//	{ VK_NULL,    IDM_EDIT_REDO,                                false, false, false, nullptr },
//	{ VK_NULL,    IDM_EDIT_CUT,                                 false, false, false, nullptr },
//	{ VK_NULL,    IDM_EDIT_COPY,                                false, false, false, nullptr },
//	{ VK_NULL,    IDM_EDIT_PASTE,                               false, false, false, nullptr },
//	{ VK_NULL,    IDM_EDIT_DELETE,                              false, false, false, nullptr },
//	{ VK_NULL,    IDM_EDIT_SELECTALL,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_BEGINENDSELECT,                      false, false, false, nullptr },

	{ VK_NULL,    IDM_EDIT_FULLPATHTOCLIP,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_FILENAMETOCLIP,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_CURRENTDIRTOCLIP,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_COPY_ALL_NAMES,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_COPY_ALL_PATHS,                      false, false, false, nullptr },

	{ VK_NULL,    IDM_EDIT_INS_TAB,                             false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_RMV_TAB,                             false, false, false, nullptr },
	{ VK_U,       IDM_EDIT_UPPERCASE,                           true,  false, true,  nullptr },
	{ VK_U,       IDM_EDIT_LOWERCASE,                           true,  false, false, nullptr },
	{ VK_U,       IDM_EDIT_PROPERCASE_FORCE,                    false, true,  false, nullptr },
	{ VK_U,       IDM_EDIT_PROPERCASE_BLEND,                    false, true,  true,  nullptr },
	{ VK_U,       IDM_EDIT_SENTENCECASE_FORCE,                  true,  true,  false, nullptr },
	{ VK_U,       IDM_EDIT_SENTENCECASE_BLEND,                  true,  true,  true,  nullptr },
	{ VK_NULL,    IDM_EDIT_INVERTCASE,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_RANDOMCASE,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_REMOVE_CONSECUTIVE_DUP_LINES,        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_REMOVE_ANY_DUP_LINES,                false, false, false, nullptr },
	{ VK_I,       IDM_EDIT_SPLIT_LINES,                         true,  false, false, nullptr },
	{ VK_J,       IDM_EDIT_JOIN_LINES,                          true,  false, false, nullptr },
	{ VK_UP,      IDM_EDIT_LINE_UP,                             true,  false, true,  nullptr },
	{ VK_DOWN,    IDM_EDIT_LINE_DOWN,                           true,  false, true,  nullptr },
	{ VK_NULL,    IDM_EDIT_REMOVEEMPTYLINES,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_REMOVEEMPTYLINESWITHBLANK,           false, false, false, nullptr },
	{ VK_RETURN,  IDM_EDIT_BLANKLINEABOVECURRENT,               true,  true,  false, nullptr },
	{ VK_RETURN,  IDM_EDIT_BLANKLINEBELOWCURRENT,               true,  true,  true,  nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASCENDING,   false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICOGRAPHIC_DESCENDING,  false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_ASCENDING,   false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_LEXICO_CASE_INSENS_DESCENDING,  false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_INTEGER_ASCENDING,         false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_INTEGER_DESCENDING,        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALCOMMA_ASCENDING,    false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALCOMMA_DESCENDING,   false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALDOT_ASCENDING,      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_DECIMALDOT_DESCENDING,     false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_REVERSE_ORDER,             false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SORTLINES_RANDOMLY,                  false, false, false, nullptr },
	{ VK_Q,       IDM_EDIT_BLOCK_COMMENT,                       true,  false, false, nullptr },
	{ VK_K,       IDM_EDIT_BLOCK_COMMENT_SET,                   true,  false, false, nullptr },
	{ VK_K,       IDM_EDIT_BLOCK_UNCOMMENT,                     true,  false, true,  nullptr },
	{ VK_Q,       IDM_EDIT_STREAM_COMMENT,                      true,  false, true,  nullptr },
	{ VK_NULL,    IDM_EDIT_STREAM_UNCOMMENT,                    false, false, false, nullptr },
	{ VK_SPACE,   IDM_EDIT_AUTOCOMPLETE,                        true,  false, false, nullptr },
	{ VK_SPACE,   IDM_EDIT_AUTOCOMPLETE_PATH,                   true,  true,  false, nullptr },
	{ VK_RETURN,  IDM_EDIT_AUTOCOMPLETE_CURRENTFILE,            true,  false, false, nullptr },
	{ VK_SPACE,   IDM_EDIT_FUNCCALLTIP,                         true,  false, true,  nullptr },
	{ VK_UP,      IDM_EDIT_FUNCCALLTIP_PREVIOUS,                false, true,  false, nullptr },
	{ VK_DOWN,    IDM_EDIT_FUNCCALLTIP_NEXT,                    false, true,  false, nullptr },
	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_SHORT,               false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_LONG,                false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_INSERT_DATETIME_CUSTOMIZED,          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_TODOS,                             false, false, false, TEXT("EOL Conversion to Windows (CR LF)") },
	{ VK_NULL,    IDM_FORMAT_TOUNIX,                            false, false, false, TEXT("EOL Conversion to Unix (LF)") },
	{ VK_NULL,    IDM_FORMAT_TOMAC,                             false, false, false, TEXT("EOL Conversion to Macintosh (CR)") },
	{ VK_NULL,    IDM_EDIT_TRIMTRAILING,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_TRIMLINEHEAD,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_TRIM_BOTH,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_EOL2WS,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_TRIMALL,                             false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_TAB2SW,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SW2TAB_ALL,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SW2TAB_LEADING,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_PASTE_AS_HTML,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_PASTE_AS_RTF,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_COPY_BINARY,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_CUT_BINARY,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_PASTE_BINARY,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_OPENASFILE,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_OPENINFOLDER,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_SEARCHONINTERNET,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_CHANGESEARCHENGINE,                  false, false, false, nullptr },
//  { VK_NULL,    IDM_EDIT_COLUMNMODETIP,                       false, false, false, nullptr },
	{ VK_C,       IDM_EDIT_COLUMNMODE,                          false, true,  false, nullptr },
	{ VK_NULL,    IDM_EDIT_CHAR_PANEL,                          false, false, false, TEXT("Toggle Character Panel") },
	{ VK_NULL,    IDM_EDIT_CLIPBOARDHISTORY_PANEL,              false, false, false, TEXT("Toggle Clipboard History") },
	{ VK_NULL,    IDM_EDIT_SETREADONLY,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_EDIT_CLEARREADONLY,                       false, false, false, nullptr },
	{ VK_F,       IDM_SEARCH_FIND,                              true,  false, false, nullptr },
	{ VK_F,       IDM_SEARCH_FINDINFILES,                       true,  false, true,  nullptr },
	{ VK_F3,      IDM_SEARCH_FINDNEXT,                          false, false, false, nullptr },
	{ VK_F3,      IDM_SEARCH_FINDPREV,                          false, false, true,  nullptr },
	{ VK_F3,      IDM_SEARCH_SETANDFINDNEXT,                    true,  false, false, nullptr },
	{ VK_F3,      IDM_SEARCH_SETANDFINDPREV,                    true,  false, true,  nullptr },
	{ VK_F3,      IDM_SEARCH_VOLATILE_FINDNEXT,                 true,  true,  false, nullptr },
	{ VK_F3,      IDM_SEARCH_VOLATILE_FINDPREV,                 true,  true,  true,  nullptr },
	{ VK_H,       IDM_SEARCH_REPLACE,                           true,  false, false, nullptr },
	{ VK_I,       IDM_SEARCH_FINDINCREMENT,                     true,  true,  false, nullptr },
	{ VK_F7,      IDM_FOCUS_ON_FOUND_RESULTS,                   false, false, false, nullptr },
	{ VK_F4,      IDM_SEARCH_GOTOPREVFOUND,                     false, false, true,  nullptr },
	{ VK_F4,      IDM_SEARCH_GOTONEXTFOUND,                     false, false, false, nullptr },
	{ VK_G,       IDM_SEARCH_GOTOLINE,                          true,  false, false, nullptr },
	{ VK_B,       IDM_SEARCH_GOTOMATCHINGBRACE,                 true,  false, false, nullptr },
	{ VK_B,       IDM_SEARCH_SELECTMATCHINGBRACES,              true,  true,  false, nullptr },
	{ VK_M,       IDM_SEARCH_MARK,                              true,  false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT1,                       false, false, false, TEXT("Style all using 1st style") },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT2,                       false, false, false, TEXT("Style all using 2nd style") },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT3,                       false, false, false, TEXT("Style all using 3rd style") },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT4,                       false, false, false, TEXT("Style all using 4th style") },
	{ VK_NULL,    IDM_SEARCH_MARKALLEXT5,                       false, false, false, TEXT("Style all using 5th style") },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT1,                       false, false, false, TEXT("Style one using 1st style") },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT2,                       false, false, false, TEXT("Style one using 2nd style") },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT3,                       false, false, false, TEXT("Style one using 3rd style") },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT4,                       false, false, false, TEXT("Style one using 4th style") },
	{ VK_NULL,    IDM_SEARCH_MARKONEEXT5,                       false, false, false, TEXT("Style one using 5th style") },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT1,                     false, false, false, TEXT("Clear 1st style") },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT2,                     false, false, false, TEXT("Clear 2nd style") },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT3,                     false, false, false, TEXT("Clear 3rd style") },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT4,                     false, false, false, TEXT("Clear 4th style") },
	{ VK_NULL,    IDM_SEARCH_UNMARKALLEXT5,                     false, false, false, TEXT("Clear 5th style") },
	{ VK_NULL,    IDM_SEARCH_CLEARALLMARKS,                     false, false, false, TEXT("Clear all styles") },
	{ VK_1,       IDM_SEARCH_GOPREVMARKER1,                     true,  false, true,  TEXT("Previous style of 1st style") },
	{ VK_2,       IDM_SEARCH_GOPREVMARKER2,                     true,  false, true,  TEXT("Previous style of 2nd style") },
	{ VK_3,       IDM_SEARCH_GOPREVMARKER3,                     true,  false, true,  TEXT("Previous style of 3rd style") },
	{ VK_4,       IDM_SEARCH_GOPREVMARKER4,                     true,  false, true,  TEXT("Previous style of 4th style") },
	{ VK_5,       IDM_SEARCH_GOPREVMARKER5,                     true,  false, true,  TEXT("Previous style of 5th style") },
	{ VK_0,       IDM_SEARCH_GOPREVMARKER_DEF,                  true,  false, true,  TEXT("Previous style of Find Mark style") },
	{ VK_1,       IDM_SEARCH_GONEXTMARKER1,                     true,  false, false, TEXT("Next style of 1st style") },
	{ VK_2,       IDM_SEARCH_GONEXTMARKER2,                     true,  false, false, TEXT("Next style of 2nd style") },
	{ VK_3,       IDM_SEARCH_GONEXTMARKER3,                     true,  false, false, TEXT("Next style of 3rd style") },
	{ VK_4,       IDM_SEARCH_GONEXTMARKER4,                     true,  false, false, TEXT("Next style of 4th style") },
	{ VK_5,       IDM_SEARCH_GONEXTMARKER5,                     true,  false, false, TEXT("Next style of 5th style") },
	{ VK_0,       IDM_SEARCH_GONEXTMARKER_DEF,                  true,  false, false, TEXT("Next style of Find Mark style") },
	{ VK_NULL,    IDM_SEARCH_STYLE1TOCLIP,                      false, false, false, TEXT("Copy Styled Text of 1st Style") },
	{ VK_NULL,    IDM_SEARCH_STYLE2TOCLIP,                      false, false, false, TEXT("Copy Styled Text of 2nd Style") },
	{ VK_NULL,    IDM_SEARCH_STYLE3TOCLIP,                      false, false, false, TEXT("Copy Styled Text of 3rd Style") },
	{ VK_NULL,    IDM_SEARCH_STYLE4TOCLIP,                      false, false, false, TEXT("Copy Styled Text of 4th Style") },
	{ VK_NULL,    IDM_SEARCH_STYLE5TOCLIP,                      false, false, false, TEXT("Copy Styled Text of 5th Style") },
	{ VK_NULL,    IDM_SEARCH_ALLSTYLESTOCLIP,                   false, false, false, TEXT("Copy Styled Text of All Styles") },
	{ VK_NULL,    IDM_SEARCH_MARKEDTOCLIP,                      false, false, false, TEXT("Copy Styled Text of Find Mark style") },
	{ VK_F2,      IDM_SEARCH_TOGGLE_BOOKMARK,                   true,  false, false, nullptr },
	{ VK_F2,      IDM_SEARCH_NEXT_BOOKMARK,                     false, false, false, nullptr },
	{ VK_F2,      IDM_SEARCH_PREV_BOOKMARK,                     false, false, true, nullptr  },
	{ VK_NULL,    IDM_SEARCH_CLEAR_BOOKMARKS,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_CUTMARKEDLINES,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_COPYMARKEDLINES,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_PASTEMARKEDLINES,                  false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_DELETEMARKEDLINES,                 false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_DELETEUNMARKEDLINES,               false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_INVERSEMARKS,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_SEARCH_FINDCHARINRANGE,                   false, false, false, nullptr },
				 
	{ VK_NULL,    IDM_VIEW_ALWAYSONTOP,                         false, false, false, nullptr },
	{ VK_F11,     IDM_VIEW_FULLSCREENTOGGLE,                    false, false, false, nullptr },
	{ VK_F12,     IDM_VIEW_POSTIT,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_DISTRACTIONFREE,                     false, false, false, nullptr },

	{ VK_NULL,    IDM_VIEW_IN_FIREFOX,                          false, false, false, TEXT("View current file in Firefox") },
	{ VK_NULL,    IDM_VIEW_IN_CHROME,                           false, false, false, TEXT("View current file in Chrome") },
	{ VK_NULL,    IDM_VIEW_IN_IE,                               false, false, false, TEXT("View current file in IE") },
	{ VK_NULL,    IDM_VIEW_IN_EDGE,                             false, false, false, TEXT("View current file in Edge")  },

	{ VK_NULL,    IDM_VIEW_TAB_SPACE,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_EOL,                                 false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_ALL_CHARACTERS,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_INDENT_GUIDE,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_WRAP_SYMBOL,                         false, false, false, nullptr },
//  { VK_NULL,    IDM_VIEW_ZOOMIN,                              false, false, false, nullptr },
//  { VK_NULL,    IDM_VIEW_ZOOMOUT,                             false, false, false, nullptr },
//  { VK_NULL,    IDM_VIEW_ZOOMRESTORE,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_GOTO_ANOTHER_VIEW,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_CLONE_TO_ANOTHER_VIEW,               false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_GOTO_NEW_INSTANCE,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_LOAD_IN_NEW_INSTANCE,                false, false, false, nullptr },

	{ VK_NUMPAD1, IDM_VIEW_TAB1,                                true,  false, false, nullptr },
	{ VK_NUMPAD2, IDM_VIEW_TAB2,                                true,  false, false, nullptr },
	{ VK_NUMPAD3, IDM_VIEW_TAB3,                                true,  false, false, nullptr },
	{ VK_NUMPAD4, IDM_VIEW_TAB4,                                true,  false, false, nullptr },
	{ VK_NUMPAD5, IDM_VIEW_TAB5,                                true,  false, false, nullptr },
	{ VK_NUMPAD6, IDM_VIEW_TAB6,                                true,  false, false, nullptr },
	{ VK_NUMPAD7, IDM_VIEW_TAB7,                                true,  false, false, nullptr },
	{ VK_NUMPAD8, IDM_VIEW_TAB8,                                true,  false, false, nullptr },
	{ VK_NUMPAD9, IDM_VIEW_TAB9,                                true,  false, false, nullptr },
	{ VK_NEXT,    IDM_VIEW_TAB_NEXT,                            true,  false, false, nullptr },
	{ VK_PRIOR,   IDM_VIEW_TAB_PREV,                            true,  false, false, nullptr },
	{ VK_NEXT,    IDM_VIEW_TAB_MOVEFORWARD,                     true,  false, true,  nullptr },
	{ VK_PRIOR,   IDM_VIEW_TAB_MOVEBACKWARD,                    true,  false, true,  nullptr },
	{ VK_TAB,     IDC_PREV_DOC,                                 true,  false, true,  TEXT("Switch to previous document") },
	{ VK_TAB,     IDC_NEXT_DOC,                                 true,  false, false, TEXT("Switch to next document") },
	{ VK_NULL,    IDM_VIEW_WRAP,                                false, false, false, nullptr },
	{ VK_H,       IDM_VIEW_HIDELINES,                           false, true,  false, nullptr },
	{ VK_F8,      IDM_VIEW_SWITCHTO_OTHER_VIEW,                 false, false, false, nullptr },

	{ VK_0,       IDM_VIEW_FOLDALL,                             false, true,  false, nullptr },
	{ VK_0,       IDM_VIEW_UNFOLDALL,                           false, true,  true,  nullptr },
	{ VK_F,       IDM_VIEW_FOLD_CURRENT,                        true,  true,  false, nullptr },
	{ VK_F,       IDM_VIEW_UNFOLD_CURRENT,                      true,  true,  true,  nullptr },
	{ VK_1,       IDM_VIEW_FOLD_1,                              false, true,  false, TEXT("Fold Level 1") },
	{ VK_2,       IDM_VIEW_FOLD_2,                              false, true,  false, TEXT("Fold Level 2") },
	{ VK_3,       IDM_VIEW_FOLD_3,                              false, true,  false, TEXT("Fold Level 3") },
	{ VK_4,       IDM_VIEW_FOLD_4,                              false, true,  false, TEXT("Fold Level 4") },
	{ VK_5,       IDM_VIEW_FOLD_5,                              false, true,  false, TEXT("Fold Level 5") },
	{ VK_6,       IDM_VIEW_FOLD_6,                              false, true,  false, TEXT("Fold Level 6") },
	{ VK_7,       IDM_VIEW_FOLD_7,                              false, true,  false, TEXT("Fold Level 7") },
	{ VK_8,       IDM_VIEW_FOLD_8,                              false, true,  false, TEXT("Fold Level 8") },

	{ VK_1,       IDM_VIEW_UNFOLD_1,                            false, true,  true,  TEXT("Unfold Level 1") },
	{ VK_2,       IDM_VIEW_UNFOLD_2,                            false, true,  true,  TEXT("Unfold Level 2") },
	{ VK_3,       IDM_VIEW_UNFOLD_3,                            false, true,  true,  TEXT("Unfold Level 3") },
	{ VK_4,       IDM_VIEW_UNFOLD_4,                            false, true,  true,  TEXT("Unfold Level 4") },
	{ VK_5,       IDM_VIEW_UNFOLD_5,                            false, true,  true,  TEXT("Unfold Level 5") },
	{ VK_6,       IDM_VIEW_UNFOLD_6,                            false, true,  true,  TEXT("Unfold Level 6") },
	{ VK_7,       IDM_VIEW_UNFOLD_7,                            false, true,  true,  TEXT("Unfold Level 7") },
	{ VK_8,       IDM_VIEW_UNFOLD_8,                            false, true,  true,  TEXT("Unfold Level 8") },
	{ VK_NULL,    IDM_VIEW_SUMMARY,                             false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_1,                     false, false, false, TEXT("Toggle Project Panel 1") },
	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_2,                     false, false, false, TEXT("Toggle Project Panel 2") },
	{ VK_NULL,    IDM_VIEW_PROJECT_PANEL_3,                     false, false, false, TEXT("Toggle Project Panel 3") },
	{ VK_NULL,    IDM_VIEW_FILEBROWSER,                         false, false, false, TEXT("Toggle Folder as Workspace") },
	{ VK_NULL,    IDM_VIEW_DOC_MAP,                             false, false, false, TEXT("Toggle Document Map") },
	{ VK_NULL,    IDM_VIEW_DOCLIST,                             false, false, false, TEXT("Toggle Document List") },
	{ VK_NULL,    IDM_VIEW_FUNC_LIST,                           false, false, false, TEXT("Toggle Function List") },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_1,            false, false, false, TEXT("Switch to Project Panel 1") },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_2,            false, false, false, TEXT("Switch to Project Panel 2") },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_PROJECT_PANEL_3,            false, false, false, TEXT("Switch to Project Panel 3") },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_FILEBROWSER,                false, false, false, TEXT("Switch to Folder as Workspace") },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_FUNC_LIST,                  false, false, false, TEXT("Switch to Function List") },
	{ VK_NULL,    IDM_VIEW_SWITCHTO_DOCLIST,                    false, false, false, TEXT("Switch to Document List") },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_NONE,                     false, false, false, TEXT("Remove Tab Colour") },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_1,                        false, false, false, TEXT("Apply Tab Colour 1") },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_2,                        false, false, false, TEXT("Apply Tab Colour 2") },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_3,                        false, false, false, TEXT("Apply Tab Colour 3") },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_4,                        false, false, false, TEXT("Apply Tab Colour 4") },
	{ VK_NULL,    IDM_VIEW_TAB_COLOUR_5,                        false, false, false, TEXT("Apply Tab Colour 5") },
	{ VK_NULL,    IDM_VIEW_SYNSCROLLV,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_VIEW_SYNSCROLLH,                          false, false, false, nullptr },
	{ VK_R,       IDM_EDIT_RTL,                                 true,  true,  false, nullptr },
	{ VK_L,       IDM_EDIT_LTR,                                 true,  true,  false, nullptr },
	{ VK_NULL,    IDM_VIEW_MONITORING,                          false, false, false, nullptr },

	{ VK_NULL,    IDM_FORMAT_ANSI,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_AS_UTF_8,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_UTF_8,                             false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_UTF_16BE,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_UTF_16LE,                          false, false, false, nullptr },

	{ VK_NULL,    IDM_FORMAT_ISO_8859_6,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1256,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_13,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1257,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_14,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_5,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_MAC_CYRILLIC,                      false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_KOI8R_CYRILLIC,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_KOI8U_CYRILLIC,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1251,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1250,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_437,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_720,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_737,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_775,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_850,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_852,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_855,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_857,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_858,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_860,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_861,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_862,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_863,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_865,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_866,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_DOS_869,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_BIG5,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_GB2312,                            false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_2,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_7,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1253,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_8,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1255,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_SHIFT_JIS,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_EUC_KR,                            false, false, false, nullptr },
	//{ VK_NULL,    IDM_FORMAT_ISO_8859_10,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_15,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_4,                        false, false, false, nullptr },
	//{ VK_NULL,    IDM_FORMAT_ISO_8859_16,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_3,                        false, false, false, nullptr },
	//{ VK_NULL,    IDM_FORMAT_ISO_8859_11,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_TIS_620,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_9,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1254,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1252,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_ISO_8859_1,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_WIN_1258,                          false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_ANSI,                        false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_AS_UTF_8,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_8,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_16BE,                    false, false, false, nullptr },
	{ VK_NULL,    IDM_FORMAT_CONV2_UTF_16LE,                    false, false, false, nullptr },

	{ VK_NULL,    IDM_LANG_USER_DLG,                            false, false, false, nullptr },
	{ VK_NULL,    IDM_LANG_USER,                                false, false, false, nullptr },
	{ VK_NULL,    IDM_LANG_OPENUDLDIR,                          false, false, false, nullptr },

	{ VK_NULL,    IDM_SETTING_PREFERENCE,                       false, false, false, nullptr },
	{ VK_NULL,    IDM_LANGSTYLE_CONFIG_DLG,                     false, false, false, nullptr },
	{ VK_NULL,    IDM_SETTING_SHORTCUT_MAPPER,                  false, false, false, nullptr },
	{ VK_NULL,    IDM_SETTING_IMPORTPLUGIN,                     false, false, false, nullptr },
	{ VK_NULL,    IDM_SETTING_IMPORTSTYLETHEMS,                 false, false, false, nullptr },
	{ VK_NULL,    IDM_SETTING_EDITCONTEXTMENU,                  false, false, false, nullptr },

	{ VK_R,       IDC_EDIT_TOGGLEMACRORECORDING,                true,  false, true,  TEXT("Toggle macro recording")},
	{ VK_NULL,    IDM_MACRO_STARTRECORDINGMACRO,                false, false, false, nullptr },
	{ VK_NULL,    IDM_MACRO_STOPRECORDINGMACRO,                 false, false, false, nullptr },
	{ VK_P,       IDM_MACRO_PLAYBACKRECORDEDMACRO,              true,  false, true,  nullptr },
	{ VK_NULL,    IDM_MACRO_SAVECURRENTMACRO,                   false, false, false, nullptr },
	{ VK_NULL,    IDM_MACRO_RUNMULTIMACRODLG,                   false, false, false, nullptr },

	{ VK_F5,      IDM_EXECUTE,                                  false, false, false, nullptr },

	{ VK_NULL,    IDM_WINDOW_SORT_FN_ASC,                       false, false, false, TEXT("Sort By Name A to Z") },
	{ VK_NULL,    IDM_WINDOW_SORT_FN_DSC,                       false, false, false, TEXT("Sort By Name Z to A") },
	{ VK_NULL,    IDM_WINDOW_SORT_FP_ASC,                       false, false, false, TEXT("Sort By Path A to Z") },
	{ VK_NULL,    IDM_WINDOW_SORT_FP_DSC,                       false, false, false, TEXT("Sort By Path Z to A") },
	{ VK_NULL,    IDM_WINDOW_SORT_FT_ASC,                       false, false, false, TEXT("Sort By Type A to Z") },
	{ VK_NULL,    IDM_WINDOW_SORT_FT_DSC,                       false, false, false, TEXT("Sort By Type Z to A") },
	{ VK_NULL,    IDM_WINDOW_SORT_FS_ASC,                       false, false, false, TEXT("Sort By Size Smaller to Larger") },
	{ VK_NULL,    IDM_WINDOW_SORT_FS_DSC,                       false, false, false, TEXT("Sort By Size Larger to Smaller") },

	{ VK_NULL,    IDM_CMDLINEARGUMENTS,                         false, false, false, nullptr },
	{ VK_NULL,    IDM_HOMESWEETHOME,                            false, false, false, nullptr },
	{ VK_NULL,    IDM_PROJECTPAGE,                              false, false, false, nullptr },
	{ VK_NULL,    IDM_ONLINEDOCUMENT,                           false, false, false, nullptr },
	{ VK_NULL,    IDM_FORUM,                                    false, false, false, nullptr },
//	{ VK_NULL,    IDM_ONLINESUPPORT,                            false, false, false, nullptr },
//	{ VK_NULL,    IDM_PLUGINSHOME,                              false, false, false, nullptr },

	// The following two commands are not in menu if (nppGUI._doesExistUpdater == 0).
	// They cannot be derived from menu then, only for this reason the text is specified here.
	// In localized environments, the text comes preferably from xml Menu/Main/Commands.
	{ VK_NULL,    IDM_UPDATE_NPP,                               false, false, false, TEXT("Update Notepad++") },
	{ VK_NULL,    IDM_CONFUPDATERPROXY,                         false, false, false, TEXT("Set Updater Proxy...") },
	{ VK_NULL,    IDM_DEBUGINFO,                                false, false, false, nullptr },
	{ VK_F1,      IDM_ABOUT,                                    false, false, false, nullptr }
};




/*!
** \brief array of accelerator keys for all possible scintilla functions
**
** values can be 0 for vKey, which means its unused
*/
static const ScintillaKeyDefinition scintKeyDefs[] =
{
	{TEXT("SCI_CUT"),                     SCI_CUT,                     true,  false, false, VK_X,        IDM_EDIT_CUT},
	{TEXT(""),                            SCI_CUT,                     false, false, true,  VK_DELETE,   0},
	{TEXT("SCI_COPY"),                    SCI_COPY,                    true,  false, false, VK_C,        IDM_EDIT_COPY},
	{TEXT(""),                            SCI_COPY,                    true,  false, false, VK_INSERT,   0},
	{TEXT("SCI_PASTE"),                   SCI_PASTE,                   true,  false, false, VK_V,        IDM_EDIT_PASTE},
	{TEXT(""),                            SCI_PASTE,                   false, false, true,  VK_INSERT,   0},
	{TEXT("SCI_SELECTALL"),               SCI_SELECTALL,               true,  false, false, VK_A,        IDM_EDIT_SELECTALL},
	{TEXT("SCI_CLEAR"),                   SCI_CLEAR,                   false, false, false, VK_DELETE,   IDM_EDIT_DELETE},
	{TEXT("SCI_CLEARALL"),                SCI_CLEARALL,                false, false, false, 0,           0},
	{TEXT("SCI_UNDO"),                    SCI_UNDO,                    true,  false, false, VK_Z,        IDM_EDIT_UNDO},
	{TEXT(""),                            SCI_UNDO,                    false, true,  false, VK_BACK,     0},
	{TEXT("SCI_REDO"),                    SCI_REDO,                    true,  false, false, VK_Y,        IDM_EDIT_REDO},
	{TEXT(""),                            SCI_REDO,                    true,  false, true,  VK_Z,        0},
	{TEXT("SCI_NEWLINE"),                 SCI_NEWLINE,                 false, false, false, VK_RETURN,   0},
	{TEXT(""),                            SCI_NEWLINE,                 false, false, true,  VK_RETURN,   0},
	{TEXT("SCI_TAB"),                     SCI_TAB,                     false, false, false, VK_TAB,      0},
	{TEXT("SCI_BACKTAB"),                 SCI_BACKTAB,                 false, false, true,  VK_TAB,      0},
	{TEXT("SCI_FORMFEED"),                SCI_FORMFEED,                false, false, false, 0,           0},
	{TEXT("SCI_ZOOMIN"),                  SCI_ZOOMIN,                  true,  false, false, VK_ADD,      IDM_VIEW_ZOOMIN},
	{TEXT("SCI_ZOOMOUT"),                 SCI_ZOOMOUT,                 true,  false, false, VK_SUBTRACT, IDM_VIEW_ZOOMOUT},
	{TEXT("SCI_SETZOOM"),                 SCI_SETZOOM,                 true,  false, false, VK_DIVIDE,   IDM_VIEW_ZOOMRESTORE},
	{TEXT("SCI_SELECTIONDUPLICATE"),      SCI_SELECTIONDUPLICATE,      true,  false, false, VK_D,        IDM_EDIT_DUP_LINE},
	{TEXT("SCI_LINESJOIN"),               SCI_LINESJOIN,               false, false, false, 0,           0},
	{TEXT("SCI_SCROLLCARET"),             SCI_SCROLLCARET,             false, false, false, 0,           0},
	{TEXT("SCI_EDITTOGGLEOVERTYPE"),      SCI_EDITTOGGLEOVERTYPE,      false, false, false, VK_INSERT,   0},
	{TEXT("SCI_MOVECARETINSIDEVIEW"),     SCI_MOVECARETINSIDEVIEW,     false, false, false, 0,           0},
	{TEXT("SCI_LINEDOWN"),                SCI_LINEDOWN,                false, false, false, VK_DOWN,     0},
	{TEXT("SCI_LINEDOWNEXTEND"),          SCI_LINEDOWNEXTEND,          false, false, true,  VK_DOWN,     0},
	{TEXT("SCI_LINEDOWNRECTEXTEND"),      SCI_LINEDOWNRECTEXTEND,      false, true,  true,  VK_DOWN,     0},
	{TEXT("SCI_LINESCROLLDOWN"),          SCI_LINESCROLLDOWN,          true,  false, false, VK_DOWN,     0},
	{TEXT("SCI_LINEUP"),                  SCI_LINEUP,                  false, false, false, VK_UP,       0},
	{TEXT("SCI_LINEUPEXTEND"),            SCI_LINEUPEXTEND,            false, false, true,  VK_UP,       0},
	{TEXT("SCI_LINEUPRECTEXTEND"),        SCI_LINEUPRECTEXTEND,        false, true,  true,  VK_UP,       0},
	{TEXT("SCI_LINESCROLLUP"),            SCI_LINESCROLLUP,            true,  false, false, VK_UP,       0},
	{TEXT("SCI_PARADOWN"),                SCI_PARADOWN,                true,  false, false, VK_OEM_6,    0},
	{TEXT("SCI_PARADOWNEXTEND"),          SCI_PARADOWNEXTEND,          true,  false, true,  VK_OEM_6,    0},
	{TEXT("SCI_PARAUP"),                  SCI_PARAUP,                  true,  false, false, VK_OEM_4,    0},
	{TEXT("SCI_PARAUPEXTEND"),            SCI_PARAUPEXTEND,            true,  false, true,  VK_OEM_4,    0},
	{TEXT("SCI_CHARLEFT"),                SCI_CHARLEFT,                false, false, false, VK_LEFT,     0},
	{TEXT("SCI_CHARLEFTEXTEND"),          SCI_CHARLEFTEXTEND,          false, false, true,  VK_LEFT,     0},
	{TEXT("SCI_CHARLEFTRECTEXTEND"),      SCI_CHARLEFTRECTEXTEND,      false, true,  true,  VK_LEFT,     0},
	{TEXT("SCI_CHARRIGHT"),               SCI_CHARRIGHT,               false, false, false, VK_RIGHT,    0},
	{TEXT("SCI_CHARRIGHTEXTEND"),         SCI_CHARRIGHTEXTEND,         false, false, true,  VK_RIGHT,    0},
	{TEXT("SCI_CHARRIGHTRECTEXTEND"),     SCI_CHARRIGHTRECTEXTEND,     false, true,  true,  VK_RIGHT,    0},
	{TEXT("SCI_WORDLEFT"),                SCI_WORDLEFT,                true,  false, false, VK_LEFT,     0},
	{TEXT("SCI_WORDLEFTEXTEND"),          SCI_WORDLEFTEXTEND,          true,  false, true,  VK_LEFT,     0},
	{TEXT("SCI_WORDRIGHT"),               SCI_WORDRIGHT,               true,  false, false, VK_RIGHT,    0},
	{TEXT("SCI_WORDRIGHTEXTEND"),         SCI_WORDRIGHTEXTEND,         false, false, false, 0,           0},
	{TEXT("SCI_WORDLEFTEND"),             SCI_WORDLEFTEND,             false, false, false, 0,           0},
	{TEXT("SCI_WORDLEFTENDEXTEND"),       SCI_WORDLEFTENDEXTEND,       false, false, false, 0,           0},
	{TEXT("SCI_WORDRIGHTEND"),            SCI_WORDRIGHTEND,            false, false, false, 0,           0},
	{TEXT("SCI_WORDRIGHTENDEXTEND"),      SCI_WORDRIGHTENDEXTEND,      true,  false, true,  VK_RIGHT,    0},
	{TEXT("SCI_WORDPARTLEFT"),            SCI_WORDPARTLEFT,            true,  false, false, VK_OEM_2,    0},
	{TEXT("SCI_WORDPARTLEFTEXTEND"),      SCI_WORDPARTLEFTEXTEND,      true,  false, true,  VK_OEM_2,    0},
	{TEXT("SCI_WORDPARTRIGHT"),           SCI_WORDPARTRIGHT,           true,  false, false, VK_OEM_5,    0},
	{TEXT("SCI_WORDPARTRIGHTEXTEND"),     SCI_WORDPARTRIGHTEXTEND,     true,  false, true,  VK_OEM_5,    0},
	{TEXT("SCI_HOME"),                    SCI_HOME,                    false, false, false, 0,           0},
	{TEXT("SCI_HOMEEXTEND"),              SCI_HOMEEXTEND,              false, false, false, 0,           0},
	{TEXT("SCI_HOMERECTEXTEND"),          SCI_HOMERECTEXTEND,          false, false, false, 0,           0},
	{TEXT("SCI_HOMEDISPLAY"),             SCI_HOMEDISPLAY,             false, true,  false, VK_HOME,     0},
	{TEXT("SCI_HOMEDISPLAYEXTEND"),       SCI_HOMEDISPLAYEXTEND,       false, false, false, 0,           0},
	{TEXT("SCI_HOMEWRAP"),                SCI_HOMEWRAP,                false, false, false, 0,           0},
	{TEXT("SCI_HOMEWRAPEXTEND"),          SCI_HOMEWRAPEXTEND,          false, false, false, 0,           0},
	{TEXT("SCI_VCHOME"),                  SCI_VCHOME,                  false, false, false, 0,           0},
	{TEXT("SCI_VCHOMEEXTEND"),            SCI_VCHOMEEXTEND,            false, false, false, 0,           0},
	{TEXT("SCI_VCHOMERECTEXTEND"),        SCI_VCHOMERECTEXTEND,        false, true,  true,  VK_HOME,     0},
	{TEXT("SCI_VCHOMEDISPLAY"),           SCI_VCHOMEDISPLAY,           false, false, false, 0,           0},
	{TEXT("SCI_VCHOMEDISPLAYEXTEND"),     SCI_VCHOMEDISPLAYEXTEND,     false, false, false, 0,           0},
	{TEXT("SCI_VCHOMEWRAP"),              SCI_VCHOMEWRAP,              false, false, false, VK_HOME,     0},
	{TEXT("SCI_VCHOMEWRAPEXTEND"),        SCI_VCHOMEWRAPEXTEND,        false, false, true,  VK_HOME,     0},
	{TEXT("SCI_LINEEND"),                 SCI_LINEEND,                 false, false, false, 0,           0},
	{TEXT("SCI_LINEENDWRAPEXTEND"),       SCI_LINEENDWRAPEXTEND,       false, false, true,  VK_END,      0},
	{TEXT("SCI_LINEENDRECTEXTEND"),       SCI_LINEENDRECTEXTEND,       false, true,  true,  VK_END,      0},
	{TEXT("SCI_LINEENDDISPLAY"),          SCI_LINEENDDISPLAY,          false, true,  false, VK_END,      0},
	{TEXT("SCI_LINEENDDISPLAYEXTEND"),    SCI_LINEENDDISPLAYEXTEND,    false, false, false, 0,           0},
	{TEXT("SCI_LINEENDWRAP"),             SCI_LINEENDWRAP,             false, false, false, VK_END,      0},
	{TEXT("SCI_LINEENDEXTEND"),           SCI_LINEENDEXTEND,           false, false, false, 0,           0},
	{TEXT("SCI_DOCUMENTSTART"),           SCI_DOCUMENTSTART,           true,  false, false, VK_HOME,     0},
	{TEXT("SCI_DOCUMENTSTARTEXTEND"),     SCI_DOCUMENTSTARTEXTEND,     true,  false, true,  VK_HOME,     0},
	{TEXT("SCI_DOCUMENTEND"),             SCI_DOCUMENTEND,             true,  false, false, VK_END,      0},
	{TEXT("SCI_DOCUMENTENDEXTEND"),       SCI_DOCUMENTENDEXTEND,       true,  false, true,  VK_END,      0},
	{TEXT("SCI_PAGEUP"),                  SCI_PAGEUP,                  false, false, false, VK_PRIOR,    0},
	{TEXT("SCI_PAGEUPEXTEND"),            SCI_PAGEUPEXTEND,            false, false, true,  VK_PRIOR,    0},
	{TEXT("SCI_PAGEUPRECTEXTEND"),        SCI_PAGEUPRECTEXTEND,        false, true,  true,  VK_PRIOR,    0},
	{TEXT("SCI_PAGEDOWN"),                SCI_PAGEDOWN,                false, false, false, VK_NEXT,     0},
	{TEXT("SCI_PAGEDOWNEXTEND"),          SCI_PAGEDOWNEXTEND,          false, false, true,  VK_NEXT,     0},
	{TEXT("SCI_PAGEDOWNRECTEXTEND"),      SCI_PAGEDOWNRECTEXTEND,      false, true,  true,  VK_NEXT,     0},
	{TEXT("SCI_STUTTEREDPAGEUP"),         SCI_STUTTEREDPAGEUP,         false, false, false, 0,           0},
	{TEXT("SCI_STUTTEREDPAGEUPEXTEND"),   SCI_STUTTEREDPAGEUPEXTEND,   false, false, false, 0,           0},
	{TEXT("SCI_STUTTEREDPAGEDOWN"),       SCI_STUTTEREDPAGEDOWN,       false, false, false, 0,           0},
	{TEXT("SCI_STUTTEREDPAGEDOWNEXTEND"), SCI_STUTTEREDPAGEDOWNEXTEND, false, false, false, 0,           0},
	{TEXT("SCI_DELETEBACK"),              SCI_DELETEBACK,              false, false, false, VK_BACK,     0},
	{TEXT(""),                            SCI_DELETEBACK,              false, false, true,  VK_BACK,     0},
	{TEXT("SCI_DELETEBACKNOTLINE"),       SCI_DELETEBACKNOTLINE,       false, false, false, 0,           0},
	{TEXT("SCI_DELWORDLEFT"),             SCI_DELWORDLEFT,             true,  false, false, VK_BACK,     0},
	{TEXT("SCI_DELWORDRIGHT"),            SCI_DELWORDRIGHT,            true,  false, false, VK_DELETE,   0},
	{TEXT("SCI_DELLINELEFT"),             SCI_DELLINELEFT,             true,  false, true,  VK_BACK,     0},
	{TEXT("SCI_DELLINERIGHT"),            SCI_DELLINERIGHT,            true,  false, true,  VK_DELETE,   0},
	{TEXT("SCI_LINEDELETE"),              SCI_LINEDELETE,              true,  false, true,  VK_L,        0},
	{TEXT("SCI_LINECUT"),                 SCI_LINECUT,                 true,  false, false, VK_L,        0},
	{TEXT("SCI_LINECOPY"),                SCI_LINECOPY,                true,  false, true,  VK_X,        0},
	{TEXT("SCI_LINETRANSPOSE"),           SCI_LINETRANSPOSE,           true,  false, false, VK_T,        0},
	{TEXT("SCI_LINEDUPLICATE"),           SCI_LINEDUPLICATE,           false, false, false, 0,           0},
	{TEXT("SCI_CANCEL"),                  SCI_CANCEL,                  false, false, false, VK_ESCAPE,   0},
	{TEXT("SCI_SWAPMAINANCHORCARET"),     SCI_SWAPMAINANCHORCARET,     false, false, false, 0,           0},
	{TEXT("SCI_ROTATESELECTION"),         SCI_ROTATESELECTION,         false, false, false, 0,           0}
};


typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

int strVal(const TCHAR *str, int base)
{
	if (!str) return -1;
	if (!str[0]) return 0;

	TCHAR *finStr;
	int result = generic_strtol(str, &finStr, base);
	if (*finStr != '\0')
		return -1;
	return result;
}


int decStrVal(const TCHAR *str)
{
	return strVal(str, 10);
}

int hexStrVal(const TCHAR *str)
{
	return strVal(str, 16);
}

int getKwClassFromName(const TCHAR *str)
{
	if (!lstrcmp(TEXT("instre1"), str)) return LANG_INDEX_INSTR;
	if (!lstrcmp(TEXT("instre2"), str)) return LANG_INDEX_INSTR2;
	if (!lstrcmp(TEXT("type1"), str)) return LANG_INDEX_TYPE;
	if (!lstrcmp(TEXT("type2"), str)) return LANG_INDEX_TYPE2;
	if (!lstrcmp(TEXT("type3"), str)) return LANG_INDEX_TYPE3;
	if (!lstrcmp(TEXT("type4"), str)) return LANG_INDEX_TYPE4;
	if (!lstrcmp(TEXT("type5"), str)) return LANG_INDEX_TYPE5;
	if (!lstrcmp(TEXT("type6"), str)) return LANG_INDEX_TYPE6;
	if (!lstrcmp(TEXT("type7"), str)) return LANG_INDEX_TYPE7;

	if ((str[1] == '\0') && (str[0] >= '0') && (str[0] <= '8')) // up to KEYWORDSET_MAX
		return str[0] - '0';

	return -1;
}


} // anonymous namespace


void cutString(const TCHAR* str2cut, vector<generic_string>& patternVect)
{
	if (str2cut == nullptr) return;

	const TCHAR *pBegin = str2cut;
	const TCHAR *pEnd = pBegin;

	while (*pEnd != '\0')
	{
		if (_istspace(*pEnd))
		{
			if (pBegin != pEnd)
				patternVect.emplace_back(pBegin, pEnd);
			pBegin = pEnd + 1;
		
		}
		++pEnd;
	}

	if (pBegin != pEnd)
		patternVect.emplace_back(pBegin, pEnd);
}

void cutStringBy(const TCHAR* str2cut, vector<generic_string>& patternVect, char byChar, bool allowEmptyStr)
{
	if (str2cut == nullptr) return;

	const TCHAR* pBegin = str2cut;
	const TCHAR* pEnd = pBegin;

	while (*pEnd != '\0')
	{
		if (*pEnd == byChar)
		{
			if (allowEmptyStr)
				patternVect.emplace_back(pBegin, pEnd);
			else if (pBegin != pEnd)
				patternVect.emplace_back(pBegin, pEnd);
			pBegin = pEnd + 1;
		}
		++pEnd;
	}
	if (allowEmptyStr)
		patternVect.emplace_back(pBegin, pEnd);
	else if (pBegin != pEnd)
		patternVect.emplace_back(pBegin, pEnd);
}


std::wstring LocalizationSwitcher::getLangFromXmlFileName(const wchar_t *fn) const
{
	size_t nbItem = sizeof(localizationDefs)/sizeof(LocalizationSwitcher::LocalizationDefinition);
	for (size_t i = 0 ; i < nbItem ; ++i)
	{
		if (0 == wcsicmp(fn, localizationDefs[i]._xmlFileName))
			return localizationDefs[i]._langName;
	}
	return std::wstring();
}


std::wstring LocalizationSwitcher::getXmlFilePathFromLangName(const wchar_t *langName) const
{
	for (size_t i = 0, len = _localizationList.size(); i < len ; ++i)
	{
		if (0 == wcsicmp(langName, _localizationList[i].first.c_str()))
			return _localizationList[i].second;
	}
	return std::wstring();
}


bool LocalizationSwitcher::addLanguageFromXml(const std::wstring& xmlFullPath)
{
	wchar_t * fn = ::PathFindFileNameW(xmlFullPath.c_str());
	wstring foundLang = getLangFromXmlFileName(fn);
	if (!foundLang.empty())
	{
		_localizationList.push_back(pair<wstring, wstring>(foundLang, xmlFullPath));
		return true;
	}
	return false;
}


bool LocalizationSwitcher::switchToLang(const wchar_t *lang2switch) const
{
	wstring langPath = getXmlFilePathFromLangName(lang2switch);
	if (langPath.empty())
		return false;

	return ::CopyFileW(langPath.c_str(), _nativeLangPath.c_str(), FALSE) != FALSE;
}


generic_string ThemeSwitcher::getThemeFromXmlFileName(const TCHAR *xmlFullPath) const
{
	if (!xmlFullPath || !xmlFullPath[0])
		return generic_string();
	generic_string fn(::PathFindFileName(xmlFullPath));
	PathRemoveExtension(const_cast<TCHAR *>(fn.c_str()));
	return fn;
}

int DynamicMenu::getTopLevelItemNumber() const
{
	int nb = 0;
	generic_string previousFolderName;
	for (const MenuItemUnit& i : _menuItems)
	{
		if (i._parentFolderName.empty())
		{
			++nb;
		}
		else
		{
			if (previousFolderName.empty())
			{
				++nb;
				previousFolderName = i._parentFolderName;
			}
			else // previousFolderName is not empty
			{
				if (i._parentFolderName.empty())
				{
					++nb;
					previousFolderName = i._parentFolderName;
				}
				else if (previousFolderName == i._parentFolderName)
				{
					// maintain the number and do nothinh
				}
				else
				{
					++nb;
					previousFolderName = i._parentFolderName;
				}
			}
		}
	}

	return nb;
}

bool DynamicMenu::attach(HMENU hMenu, unsigned int posBase, int lastCmd, const generic_string& lastCmdLabel)
{
	if (!hMenu) return false;

	_hMenu = hMenu;
	_posBase = posBase;
	_lastCmd = lastCmd;
	_lastCmdLabel = lastCmdLabel;

	return createMenu();
}

bool DynamicMenu::clearMenu() const
{
	if (!_hMenu) return false;

	int nbTopItem = getTopLevelItemNumber();
	for (int i = nbTopItem + 1; i >= 0 ; --i)
	{
		::DeleteMenu(_hMenu, static_cast<int32_t>(_posBase) + i, MF_BYPOSITION);
	}

	return true;
}

bool DynamicMenu::createMenu() const
{
	if (!_hMenu) return false;

	bool lastIsSep = false;
	HMENU hParentFolder = NULL;
	generic_string currentParentFolderStr;
	int j = 0;

	size_t nb = _menuItems.size();
	size_t i = 0;
	for (; i < nb; ++i)
	{
		const MenuItemUnit& item = _menuItems[i];
		if (item._parentFolderName.empty())
		{
			currentParentFolderStr.clear();
			hParentFolder = NULL;
			j = 0;
		}
		else
		{
			if (item._parentFolderName != currentParentFolderStr)
			{
				currentParentFolderStr = item._parentFolderName;
				hParentFolder = ::CreateMenu();
				j = 0;

				::InsertMenu(_hMenu, static_cast<UINT>(_posBase + i), MF_BYPOSITION | MF_POPUP, (UINT_PTR)hParentFolder, currentParentFolderStr.c_str());
			}
		}

		unsigned int flag = MF_BYPOSITION | ((item._cmdID == 0) ? MF_SEPARATOR : 0);
		if (hParentFolder)
		{
			::InsertMenu(hParentFolder, j++, flag, item._cmdID, item._itemName.c_str());
			lastIsSep = false;
		}
		else if ((i == 0 || i == _menuItems.size() - 1) && item._cmdID == 0)
		{
			lastIsSep = true;
		}
		else if (item._cmdID != 0)
		{
			::InsertMenu(_hMenu, static_cast<UINT>(_posBase + i), flag, item._cmdID, item._itemName.c_str());
			lastIsSep = false;
		}
		else if (item._cmdID == 0 && !lastIsSep)
		{
			::InsertMenu(_hMenu, static_cast<int32_t>(_posBase + i), flag, item._cmdID, item._itemName.c_str());
			lastIsSep = true;
		}
		else // last item is separator and current item is separator
		{
			lastIsSep = true;
		}
	}

	if (nb > 0)
	{
		::InsertMenu(_hMenu, static_cast<int32_t>(_posBase + i), MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
		::InsertMenu(_hMenu, static_cast<UINT>(_posBase + i + 2), MF_BYCOMMAND, _lastCmd, _lastCmdLabel.c_str());
	}

	return true;
}

winVer NppParameters::getWindowsVersion()
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	BOOL bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *)&osvi);
	if (!bOsVersionInfoEx)
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) )
			return WV_UNKNOWN;
	}

	pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
	if (pGNSI != NULL)
		pGNSI(&si);
	else
		GetSystemInfo(&si);

	switch (si.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_IA64:
		_platForm = PF_IA64;
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		_platForm = PF_X64;
		break;

	case PROCESSOR_ARCHITECTURE_INTEL:
		_platForm = PF_X86;
		break;

	case PROCESSOR_ARCHITECTURE_ARM64:
		_platForm = PF_ARM64;
		break;

	default:
		_platForm = PF_UNKNOWN;
	}

   switch (osvi.dwPlatformId)
   {
		case VER_PLATFORM_WIN32_NT:
		{
			if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber >= 22000)
				return WV_WIN11;

			if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0)
				return WV_WIN10;

			if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
				return WV_WIN81;

			if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
				return WV_WIN8;

			if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
				return WV_WIN7;

			if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
				return WV_VISTA;

			if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
			{
				if (osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
					return WV_XPX64;
				return WV_S2003;
			}

			if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
				return WV_XP;

			if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
				return WV_W2K;

			if (osvi.dwMajorVersion <= 4)
				return WV_NT;
			break;
		}

		// Test for the Windows Me/98/95.
		case VER_PLATFORM_WIN32_WINDOWS:
		{
			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
				return WV_95;

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
				return WV_98;

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
				return WV_ME;
			break;
		}

		case VER_PLATFORM_WIN32s:
			return WV_WIN32S;

		default:
			return WV_UNKNOWN;
   }

   return WV_UNKNOWN;
}


NppParameters::NppParameters()
{
	//Get windows version
	_winVersion = getWindowsVersion();

	// Prepare for default path
	TCHAR nppPath[MAX_PATH];
	::GetModuleFileName(NULL, nppPath, MAX_PATH);

	PathRemoveFileSpec(nppPath);
	_nppPath = nppPath;

	//Initialize current directory to startup directory
	TCHAR curDir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, curDir);
	_currentDirectory = curDir;

	_appdataNppDir.clear();
	generic_string notepadStylePath(_nppPath);
	pathAppend(notepadStylePath, notepadStyleFile);

	_asNotepadStyle = (PathFileExists(notepadStylePath.c_str()) == TRUE);

#if 0 // Mattes
	//Load initial accelerator key definitions
	initMenuKeys();
	initScintillaKeys();
#endif // Mattes
}


NppParameters::~NppParameters()
{
#if 0 // Mattes
	for (int i = 0 ; i < _nbLang ; ++i)
		delete _langList[i];
	for (int i = 0 ; i < _nbRecentFile ; ++i)
		delete _LRFileList[i];
	for (int i = 0 ; i < _nbUserLang ; ++i)
		delete _userLangArray[i];
	if (_hUXTheme)
		FreeLibrary(_hUXTheme);

	for (std::vector<TiXmlDocument *>::iterator it = _pXmlExternalLexerDoc.begin(), end = _pXmlExternalLexerDoc.end(); it != end; ++it )
		delete (*it);

	_pXmlExternalLexerDoc.clear();
#endif // Mattes
}


#if 0 // Mattes
bool NppParameters::reloadStylers(const TCHAR* stylePath)
{
	delete _pXmlUserStylerDoc;

	const TCHAR* stylePathToLoad = stylePath != nullptr ? stylePath : _stylerPath.c_str();
	_pXmlUserStylerDoc = new TiXmlDocument(stylePathToLoad);

	bool loadOkay = _pXmlUserStylerDoc->LoadFile();
	if (!loadOkay)
	{
		if (!_pNativeLangSpeaker)
		{
			::MessageBox(NULL, stylePathToLoad, TEXT("Load stylers.xml failed"), MB_OK);
		}
		else
		{
			_pNativeLangSpeaker->messageBox("LoadStylersFailed",
				NULL,
				TEXT("Load \"$STR_REPLACE$\" failed!"),
				TEXT("Load stylers.xml failed"),
				MB_OK,
				0,
				stylePathToLoad);
		}
		delete _pXmlUserStylerDoc;
		_pXmlUserStylerDoc = NULL;
		return false;
	}
	_lexerStylerVect.clear();
	_widgetStyleArray.clear();

	getUserStylersFromXmlTree();

	//  Reload plugin styles.
	for ( size_t i = 0; i < getExternalLexerDoc()->size(); ++i)
	{
		getExternalLexerFromXmlTree( getExternalLexerDoc()->at(i) );
	}
	return true;
}

bool NppParameters::reloadLang()
{
	// use user path
	generic_string nativeLangPath(_localizationSwitcher._nativeLangPath);

	// if "nativeLang.xml" does not exist, use npp path
	if (!PathFileExists(nativeLangPath.c_str()))
	{
		nativeLangPath = _nppPath;
		pathAppend(nativeLangPath, generic_string(TEXT("nativeLang.xml")));
		if (!PathFileExists(nativeLangPath.c_str()))
			return false;
	}

	delete _pXmlNativeLangDocA;

	_pXmlNativeLangDocA = new TiXmlDocumentA();

	bool loadOkay = _pXmlNativeLangDocA->LoadUnicodeFilePath(nativeLangPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlNativeLangDocA;
		_pXmlNativeLangDocA = nullptr;
		return false;
	}
	return loadOkay;
}
#endif // Mattes

generic_string NppParameters::getSpecialFolderLocation(int folderKind)
{
	TCHAR path[MAX_PATH];
	const HRESULT specialLocationResult = SHGetFolderPath(nullptr, folderKind, nullptr, SHGFP_TYPE_CURRENT, path);

	generic_string result;
	if (SUCCEEDED(specialLocationResult))
	{
		result = path;
	}
	return result;
}


generic_string NppParameters::getSettingsFolder()
{
	if (_isLocal)
		return _nppPath;

	generic_string settingsFolderPath = getSpecialFolderLocation(CSIDL_APPDATA);

	if (settingsFolderPath.empty())
		return _nppPath;

	pathAppend(settingsFolderPath, TEXT("Notepad++"));
	return settingsFolderPath;
}


bool NppParameters::load()
{
	L_END = L_EXTERNAL;
	bool isAllLaoded = true;

	_isx64 = sizeof(void *) == 8;

	// Make localConf.xml path
	generic_string localConfPath(_nppPath);
	pathAppend(localConfPath, localConfFile);

	// Test if localConf.xml exist
	_isLocal = (PathFileExists(localConfPath.c_str()) == TRUE);

	// Under vista and windows 7, the usage of doLocalConf.xml is not allowed
	// if Notepad++ is installed in "program files" directory, because of UAC
	if (_isLocal)
	{
		// We check if OS is Vista or greater version
		if (_winVersion >= WV_VISTA)
		{
			generic_string progPath = getSpecialFolderLocation(CSIDL_PROGRAM_FILES);
			TCHAR nppDirLocation[MAX_PATH];
			wcscpy_s(nppDirLocation, _nppPath.c_str());
			::PathRemoveFileSpec(nppDirLocation);

			if  (progPath == nppDirLocation)
				_isLocal = false;
		}
	}

	_pluginRootDir = _nppPath;
	pathAppend(_pluginRootDir, TEXT("plugins"));

	//
	// the 3rd priority: general default configuration
	//
	generic_string nppPluginRootParent;
	if (_isLocal)
	{
		_userPath = nppPluginRootParent = _nppPath;
		_userPluginConfDir = _pluginRootDir;
		pathAppend(_userPluginConfDir, TEXT("Config"));
	}
	else
	{
		_userPath = getSpecialFolderLocation(CSIDL_APPDATA);

		pathAppend(_userPath, TEXT("Notepad++"));
		if (!PathFileExists(_userPath.c_str()))
			::CreateDirectory(_userPath.c_str(), NULL);

		_appdataNppDir = _userPluginConfDir = _userPath;
		pathAppend(_userPluginConfDir, TEXT("plugins"));
#if 0 // Mattes
		if (!PathFileExists(_userPluginConfDir.c_str()))
			::CreateDirectory(_userPluginConfDir.c_str(), NULL);
		pathAppend(_userPluginConfDir, TEXT("Config"));
		if (!PathFileExists(_userPluginConfDir.c_str()))
			::CreateDirectory(_userPluginConfDir.c_str(), NULL);

		// For PluginAdmin to launch the wingup with UAC
		setElevationRequired(true);
#endif // Mattes
	}

	_pluginConfDir = _pluginRootDir; // for plugin list home
	pathAppend(_pluginConfDir, TEXT("Config"));

#if 0 // Mattes
	if (!PathFileExists(nppPluginRootParent.c_str()))
		::CreateDirectory(nppPluginRootParent.c_str(), NULL);
	if (!PathFileExists(_pluginRootDir.c_str()))
		::CreateDirectory(_pluginRootDir.c_str(), NULL);

	_sessionPath = _userPath; // Session stock the absolute file path, it should never be on cloud

	// Detection cloud settings
	generic_string cloudChoicePath{_userPath};
	cloudChoicePath += TEXT("\\cloud\\choice");

	//
	// the 2nd priority: cloud Choice Path
	//
	_isCloud = (::PathFileExists(cloudChoicePath.c_str()) == TRUE);
	if (_isCloud)
	{
		// Read cloud choice
		std::string cloudChoiceStr = getFileContent(cloudChoicePath.c_str());
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		std::wstring cloudChoiceStrW = wmc.char2wchar(cloudChoiceStr.c_str(), SC_CP_UTF8);

		if (!cloudChoiceStrW.empty() && ::PathFileExists(cloudChoiceStrW.c_str()))
		{
			_userPath = cloudChoiceStrW;
			_nppGUI._cloudPath = cloudChoiceStrW;
			_initialCloudChoice = _nppGUI._cloudPath;
		}
		else
		{
			_isCloud = false;
		}
	}

	//
	// the 1st priority: custom settings dir via command line argument
	//
	if (!_cmdSettingsDir.empty())
	{
		if (!::PathIsDirectory(_cmdSettingsDir.c_str()))
		{
			// The following text is not translatable.
			// _pNativeLangSpeaker is initialized AFTER _userPath being dterminated because nativeLang.xml is from from _userPath.
			generic_string errMsg = TEXT("The given path\r");
			errMsg += _cmdSettingsDir;
			errMsg += TEXT("\nvia command line \"-settingsDir=\" is not a valid directory.\rThis argument will be ignored.");
			::MessageBox(NULL, errMsg.c_str(), TEXT("Invalid directory"), MB_OK);
		}
		else
		{
			_userPath = _cmdSettingsDir;
			_sessionPath = _userPath; // reset session path
		}
	}

	//-------------------------------------//
	// Transparent function for w2k and xp //
	//-------------------------------------//
	HMODULE hUser32 = ::GetModuleHandle(TEXT("User32"));
	if (hUser32)
		_transparentFuncAddr = (WNDPROC)::GetProcAddress(hUser32, "SetLayeredWindowAttributes");

	//---------------------------------------------//
	// Dlg theme texture function for xp and vista //
	//---------------------------------------------//
	_hUXTheme = ::LoadLibraryEx(TEXT("uxtheme.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (_hUXTheme)
		_enableThemeDialogTextureFuncAddr = (WNDPROC)::GetProcAddress(_hUXTheme, "EnableThemeDialogTexture");

	//--------------------------//
	// langs.xml : for per user //
	//--------------------------//
	generic_string langs_xml_path(_userPath);
	pathAppend(langs_xml_path, TEXT("langs.xml"));

	BOOL doRecover = FALSE;
	if (::PathFileExists(langs_xml_path.c_str()))
	{
		WIN32_FILE_ATTRIBUTE_DATA attributes{};

		if (GetFileAttributesEx(langs_xml_path.c_str(), GetFileExInfoStandard, &attributes) != 0)
		{
			if (attributes.nFileSizeLow == 0 && attributes.nFileSizeHigh == 0)
			{
				if (_pNativeLangSpeaker)
				{
					doRecover = _pNativeLangSpeaker->messageBox("LoadLangsFailed",
						NULL,
						TEXT("Load langs.xml failed!\rDo you want to recover your langs.xml?"),
						TEXT("Configurator"),
						MB_YESNO);
				}
				else
				{
					doRecover = ::MessageBox(NULL, TEXT("Load langs.xml failed!\rDo you want to recover your langs.xml?"), TEXT("Configurator"), MB_YESNO);
				}
			}
		}
	}
	else
		doRecover = true;

	if (doRecover)
	{
		generic_string srcLangsPath(_nppPath);
		pathAppend(srcLangsPath, TEXT("langs.model.xml"));
		::CopyFile(srcLangsPath.c_str(), langs_xml_path.c_str(), FALSE);
	}

	_pXmlDoc = new TiXmlDocument(langs_xml_path);


	bool loadOkay = _pXmlDoc->LoadFile();
	if (!loadOkay)
	{
		if (_pNativeLangSpeaker)
		{
			_pNativeLangSpeaker->messageBox("LoadLangsFailedFinal",
				NULL,
				TEXT("Load langs.xml failed!"),
				TEXT("Configurator"),
				MB_OK);
		}
		else
		{
			::MessageBox(NULL, TEXT("Load langs.xml failed!"), TEXT("Configurator"), MB_OK);
		}

		delete _pXmlDoc;
		_pXmlDoc = nullptr;
		isAllLaoded = false;
	}
	else
		getLangKeywordsFromXmlTree();

	//---------------------------//
	// config.xml : for per user //
	//---------------------------//
	generic_string configPath(_userPath);
	pathAppend(configPath, TEXT("config.xml"));

	generic_string srcConfigPath(_nppPath);
	pathAppend(srcConfigPath, TEXT("config.model.xml"));

	if (!::PathFileExists(configPath.c_str()))
		::CopyFile(srcConfigPath.c_str(), configPath.c_str(), FALSE);

	_pXmlUserDoc = new TiXmlDocument(configPath);
	loadOkay = _pXmlUserDoc->LoadFile();
	
	if (!loadOkay)
	{
		TiXmlDeclaration* decl = new TiXmlDeclaration(TEXT("1.0"), TEXT("UTF-8"), TEXT(""));
		_pXmlUserDoc->LinkEndChild(decl);
	}
	else
	{
		getUserParametersFromXmlTree();
	}

	//----------------------------//
	// stylers.xml : for per user //
	//----------------------------//

	_stylerPath = _userPath;
	pathAppend(_stylerPath, TEXT("stylers.xml"));

	if (!PathFileExists(_stylerPath.c_str()))
	{
		generic_string srcStylersPath(_nppPath);
		pathAppend(srcStylersPath, TEXT("stylers.model.xml"));

		::CopyFile(srcStylersPath.c_str(), _stylerPath.c_str(), TRUE);
	}

	if (_nppGUI._themeName.empty() || (!PathFileExists(_nppGUI._themeName.c_str())))
		_nppGUI._themeName.assign(_stylerPath);

	_pXmlUserStylerDoc = new TiXmlDocument(_nppGUI._themeName.c_str());

	loadOkay = _pXmlUserStylerDoc->LoadFile();
	if (!loadOkay)
	{
		if (_pNativeLangSpeaker)
		{
			_pNativeLangSpeaker->messageBox("LoadStylersFailed",
				NULL,
				TEXT("Load \"$STR_REPLACE$\" failed!"),
				TEXT("Load stylers.xml failed"),
				MB_OK,
				0,
				_stylerPath.c_str());
		}
		else
		{
			::MessageBox(NULL, _stylerPath.c_str(), TEXT("Load stylers.xml failed"), MB_OK);
		}
		delete _pXmlUserStylerDoc;
		_pXmlUserStylerDoc = NULL;
		isAllLaoded = false;
	}
	else
		getUserStylersFromXmlTree();

	_themeSwitcher._stylesXmlPath = _stylerPath;
	// Firstly, add the default theme
	_themeSwitcher.addDefaultThemeFromXml(_stylerPath);

	//-----------------------------------//
	// userDefineLang.xml : for per user //
	//-----------------------------------//
	_userDefineLangsFolderPath = _userDefineLangPath = _userPath;
	pathAppend(_userDefineLangPath, TEXT("userDefineLang.xml"));
	pathAppend(_userDefineLangsFolderPath, TEXT("userDefineLangs"));

	std::vector<generic_string> udlFiles;
	getFilesInFolder(udlFiles, TEXT("*.xml"), _userDefineLangsFolderPath);

	_pXmlUserLangDoc = new TiXmlDocument(_userDefineLangPath);
	loadOkay = _pXmlUserLangDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlUserLangDoc;
		_pXmlUserLangDoc = nullptr;
		isAllLaoded = false;
	}
	else
	{
		auto r = addUserDefineLangsFromXmlTree(_pXmlUserLangDoc);
		if (r.second - r.first > 0)
			_pXmlUserLangsDoc.push_back(UdlXmlFileState(_pXmlUserLangDoc, false, r));
	}

	for (const auto& i : udlFiles)
	{
		auto udlDoc = new TiXmlDocument(i);
		loadOkay = udlDoc->LoadFile();
		if (!loadOkay)
		{
			delete udlDoc;
		}
		else
		{
			auto r = addUserDefineLangsFromXmlTree(udlDoc);
			if (r.second - r.first > 0)
				_pXmlUserLangsDoc.push_back(UdlXmlFileState(udlDoc, false, r));
		}
	}

	//----------------------------------------------//
	// nativeLang.xml : for per user				//
	// In case of absence of user's nativeLang.xml, //
	// We'll look in the Notepad++ Dir.			 //
	//----------------------------------------------//

	generic_string nativeLangPath;
	nativeLangPath = _userPath;
	pathAppend(nativeLangPath, TEXT("nativeLang.xml"));

	// LocalizationSwitcher should use always user path
	_localizationSwitcher._nativeLangPath = nativeLangPath;

	if (!_startWithLocFileName.empty()) // localization argument detected, use user wished localization
	{
		// overwrite nativeLangPath variable
		nativeLangPath = _nppPath;
		pathAppend(nativeLangPath, TEXT("localization\\"));
		pathAppend(nativeLangPath, _startWithLocFileName);
	}
	else // use %appdata% location, or (if absence then) npp installed location
	{
		if (!PathFileExists(nativeLangPath.c_str()))
		{
			nativeLangPath = _nppPath;
			pathAppend(nativeLangPath, TEXT("nativeLang.xml"));
		}
	}


	_pXmlNativeLangDocA = new TiXmlDocumentA();

	loadOkay = _pXmlNativeLangDocA->LoadUnicodeFilePath(nativeLangPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlNativeLangDocA;
		_pXmlNativeLangDocA = nullptr;
		isAllLaoded = false;
	}

	//---------------------------------//
	// toolbarIcons.xml : for per user //
	//---------------------------------//
	generic_string toolbarIconsPath(_userPath);
	pathAppend(toolbarIconsPath, TEXT("toolbarIcons.xml"));

	_pXmlToolIconsDoc = new TiXmlDocument(toolbarIconsPath);
	loadOkay = _pXmlToolIconsDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlToolIconsDoc;
		_pXmlToolIconsDoc = nullptr;
		isAllLaoded = false;
	}

	//------------------------------//
	// shortcuts.xml : for per user //
	//------------------------------//
	_shortcutsPath = _userPath;
	pathAppend(_shortcutsPath, TEXT("shortcuts.xml"));

	if (!PathFileExists(_shortcutsPath.c_str()))
	{
		generic_string srcShortcutsPath(_nppPath);
		pathAppend(srcShortcutsPath, TEXT("shortcuts.xml"));

		::CopyFile(srcShortcutsPath.c_str(), _shortcutsPath.c_str(), TRUE);
	}

	_pXmlShortcutDoc = new TiXmlDocument(_shortcutsPath);
	loadOkay = _pXmlShortcutDoc->LoadFile();
	if (!loadOkay)
	{
		delete _pXmlShortcutDoc;
		_pXmlShortcutDoc = nullptr;
		isAllLaoded = false;
	}
	else
	{
		getShortcutsFromXmlTree();
		getMacrosFromXmlTree();
		getUserCmdsFromXmlTree();

		// fill out _scintillaModifiedKeys :
		// those user defined Scintilla key will be used remap Scintilla Key Array
		getScintKeysFromXmlTree();
	}

	//---------------------------------//
	// contextMenu.xml : for per user //
	//---------------------------------//
	_contextMenuPath = _userPath;
	pathAppend(_contextMenuPath, TEXT("contextMenu.xml"));

	if (!PathFileExists(_contextMenuPath.c_str()))
	{
		generic_string srcContextMenuPath(_nppPath);
		pathAppend(srcContextMenuPath, TEXT("contextMenu.xml"));

		::CopyFile(srcContextMenuPath.c_str(), _contextMenuPath.c_str(), TRUE);
	}

	_pXmlContextMenuDocA = new TiXmlDocumentA();
	loadOkay = _pXmlContextMenuDocA->LoadUnicodeFilePath(_contextMenuPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlContextMenuDocA;
		_pXmlContextMenuDocA = nullptr;
		isAllLaoded = false;
	}

	//---------------------------------------------//
	// tabContextMenu.xml : for per user, optional //
	//---------------------------------------------//
	_tabContextMenuPath = _userPath;
	pathAppend(_tabContextMenuPath, TEXT("tabContextMenu.xml"));

	_pXmlTabContextMenuDocA = new TiXmlDocumentA();
	loadOkay = _pXmlTabContextMenuDocA->LoadUnicodeFilePath(_tabContextMenuPath.c_str());
	if (!loadOkay)
	{
		delete _pXmlTabContextMenuDocA;
		_pXmlTabContextMenuDocA = nullptr;
	}

	//----------------------------//
	// session.xml : for per user //
	//----------------------------//

	pathAppend(_sessionPath, TEXT("session.xml"));

	// Don't load session.xml if not required in order to speed up!!
	const NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
	if (nppGUI._rememberLastSession)
	{
		TiXmlDocument* pXmlSessionDoc = new TiXmlDocument(_sessionPath);

		loadOkay = pXmlSessionDoc->LoadFile();
		if (!loadOkay)
			isAllLaoded = false;
		else
			getSessionFromXmlTree(pXmlSessionDoc, _session);

		delete pXmlSessionDoc;

		for (size_t i = 0, len = _pXmlExternalLexerDoc.size() ; i < len ; ++i)
			if (_pXmlExternalLexerDoc[i])
				delete _pXmlExternalLexerDoc[i];
	}

	//-------------------------------------------------------------//
	// enableSelectFgColor.xml : for per user                      //
	// This empty xml file is optional - user adds this empty file //
	// manually in order to set selected text's foreground color.  //
	//-------------------------------------------------------------//
	generic_string enableSelectFgColorPath = _userPath;
	pathAppend(enableSelectFgColorPath, TEXT("enableSelectFgColor.xml"));

	if (PathFileExists(enableSelectFgColorPath.c_str()))
	{
		_isSelectFgColorEnabled = true;
	}


	generic_string filePath, filePath2, issueFileName;

	filePath = _nppPath;
	issueFileName = nppLogNetworkDriveIssue;
	issueFileName += TEXT(".xml");
	pathAppend(filePath, issueFileName);
	_doNppLogNetworkDriveIssue = (PathFileExists(filePath.c_str()) == TRUE);
	if (!_doNppLogNetworkDriveIssue)
	{
		filePath2 = _userPath;
		pathAppend(filePath2, issueFileName);
		_doNppLogNetworkDriveIssue = (PathFileExists(filePath2.c_str()) == TRUE);
	}

	filePath = _nppPath;
	issueFileName = nppLogNulContentCorruptionIssue;
	issueFileName += TEXT(".xml");
	pathAppend(filePath, issueFileName);
	_doNppLogNulContentCorruptionIssue = (PathFileExists(filePath.c_str()) == TRUE);
	if (!_doNppLogNulContentCorruptionIssue)
	{
		filePath2 = _userPath;
		pathAppend(filePath2, issueFileName);
		_doNppLogNulContentCorruptionIssue = (PathFileExists(filePath2.c_str()) == TRUE);
	}
#endif // Mattes




	return isAllLaoded;
}


#if 0 // Mattes
void NppParameters::destroyInstance()
{
	delete _pXmlDoc;
	delete _pXmlUserDoc;
	delete _pXmlUserStylerDoc;
	
	//delete _pXmlUserLangDoc; will be deleted in the vector
	for (auto l : _pXmlUserLangsDoc)
	{
		delete l._udlXmlDoc;
	}

	delete _pXmlNativeLangDocA;
	delete _pXmlToolIconsDoc;
	delete _pXmlShortcutDoc;
	delete _pXmlContextMenuDocA;
	delete _pXmlTabContextMenuDocA;
	delete _pXmlBlacklistDoc;
	delete 	getInstancePointer();
}


void NppParameters::saveConfig_xml()
{
	if (_pXmlUserDoc)
		_pXmlUserDoc->SaveFile();
}


void NppParameters::setWorkSpaceFilePath(int i, const TCHAR* wsFile)
{
	if (i < 0 || i > 2 || !wsFile)
		return;
	_workSpaceFilePathes[i] = wsFile;
}


void NppParameters::removeTransparent(HWND hwnd)
{
	if (hwnd != NULL)
		::SetWindowLongPtr(hwnd, GWL_EXSTYLE,  ::GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~0x00080000);
}


void NppParameters::SetTransparent(HWND hwnd, int percent)
{
	if (nullptr != _transparentFuncAddr)
	{
		::SetWindowLongPtr(hwnd, GWL_EXSTYLE, ::GetWindowLongPtr(hwnd, GWL_EXSTYLE) | 0x00080000);
		if (percent > 255)
			percent = 255;
		if (percent < 0)
			percent = 0;
		_transparentFuncAddr(hwnd, 0, percent, 0x00000002);
	}
}


bool NppParameters::isExistingExternalLangName(const char* newName) const
{
	if ((!newName) || (!newName[0]))
		return true;

	for (int i = 0 ; i < _nbExternalLang ; ++i)
	{
		if (_externalLangArray[i]->_name == newName)
			return true;
	}
	return false;
}


const TCHAR* NppParameters::getUserDefinedLangNameFromExt(TCHAR *ext, TCHAR *fullName) const
{
	if ((!ext) || (!ext[0]))
		return nullptr;

	std::vector<generic_string> extVect;
	int iMatched = -1;
	for (int i = 0 ; i < _nbUserLang ; ++i)
	{
		extVect.clear();
		cutString(_userLangArray[i]->_ext.c_str(), extVect);

		// Force to use dark mode UDL in dark mode or to use  light mode UDL in light mode
		for (size_t j = 0, len = extVect.size(); j < len; ++j)
		{
			if (!generic_stricmp(extVect[j].c_str(), ext) || (_tcschr(fullName, '.') && !generic_stricmp(extVect[j].c_str(), fullName)))
			{
				// preserve ext matched UDL
				iMatched = i;

				if (((NppDarkMode::isEnabled() && _userLangArray[i]->_isDarkModeTheme)) ||
					((!NppDarkMode::isEnabled() && !_userLangArray[i]->_isDarkModeTheme)))
					return _userLangArray[i]->_name.c_str();
			}
		}
	}

	// In case that we are in dark mode but no dark UDL or we are in light mode but no light UDL
	// We use it anyway
	if (iMatched >= 0)
	{
		return _userLangArray[iMatched]->_name.c_str();
	}

	return nullptr;
}


int NppParameters::getExternalLangIndexFromName(const TCHAR* externalLangName) const
{
	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	for (int i = 0 ; i < _nbExternalLang ; ++i)
	{
		if (!lstrcmp(externalLangName, wmc.char2wchar(_externalLangArray[i]->_name.c_str(), CP_ACP)))
			return i;
	}
	return -1;
}


UserLangContainer* NppParameters::getULCFromName(const TCHAR *userLangName)
{
	for (int i = 0 ; i < _nbUserLang ; ++i)
	{
		if (0 == lstrcmp(userLangName, _userLangArray[i]->_name.c_str()))
			return _userLangArray[i];
	}

	//qui doit etre jamais passer
	return nullptr;
}


COLORREF NppParameters::getCurLineHilitingColour()
{
	const Style * pStyle = _widgetStyleArray.findByName(TEXT("Current line background colour"));
	if (!pStyle)
		return COLORREF(-1);
	return pStyle->_bgColor;
}


void NppParameters::setCurLineHilitingColour(COLORREF colour2Set)
{
	Style * pStyle = _widgetStyleArray.findByName(TEXT("Current line background colour"));
	if (!pStyle)
		return;
	pStyle->_bgColor = colour2Set;
}



static int CALLBACK EnumFontFamExProc(const LOGFONT* lpelfe, const TEXTMETRIC*, DWORD, LPARAM lParam)
{
	std::vector<generic_string>& strVect = *(std::vector<generic_string> *)lParam;
	const int32_t vectSize = static_cast<int32_t>(strVect.size());
	const TCHAR* lfFaceName = ((ENUMLOGFONTEX*)lpelfe)->elfLogFont.lfFaceName;

	//Search through all the fonts, EnumFontFamiliesEx never states anything about order
	//Start at the end though, that's the most likely place to find a duplicate
	for (int i = vectSize - 1 ; i >= 0 ; i--)
	{
		if (0 == lstrcmp(strVect[i].c_str(), lfFaceName))
			return 1;	//we already have seen this typeface, ignore it
	}

	//We can add the font
	//Add the face name and not the full name, we do not care about any styles
	strVect.push_back(lfFaceName);
	return 1; // I want to get all fonts
}


void NppParameters::setFontList(HWND hWnd)
{
	//---------------//
	// Sys font list //
	//---------------//
	LOGFONT lf{};
	_fontlist.clear();
	_fontlist.reserve(64); // arbitrary
	_fontlist.push_back(generic_string());

	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfFaceName[0]='\0';
	lf.lfPitchAndFamily = 0;
	HDC hDC = ::GetDC(hWnd);
	::EnumFontFamiliesEx(hDC, &lf, EnumFontFamExProc, reinterpret_cast<LPARAM>(&_fontlist), 0);
}

bool NppParameters::isInFontList(const generic_string& fontName2Search) const
{
	if (fontName2Search.empty())
		return false;

	for (size_t i = 0, len = _fontlist.size(); i < len; i++)
	{
		if (_fontlist[i] == fontName2Search)
			return true;
	}
	return false;
}

HFONT NppParameters::getDefaultUIFont()
{
	static HFONT g_defaultMessageFont = []() {
		NONCLIENTMETRICS ncm = { sizeof(ncm) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

		return CreateFontIndirect(&ncm.lfMessageFont);
	}();
	return g_defaultMessageFont;
}

void NppParameters::getLangKeywordsFromXmlTree()
{
	TiXmlNode *root =
		_pXmlDoc->FirstChild(TEXT("NotepadPlus"));
		if (!root) return;
	feedKeyWordsParameters(root);
}


void NppParameters::getExternalLexerFromXmlTree(TiXmlDocument* externalLexerDoc)
{
	TiXmlNode *root = externalLexerDoc->FirstChild(TEXT("NotepadPlus"));
		if (!root) return;
	feedKeyWordsParameters(root);
	feedStylerArray(root);
}


int NppParameters::addExternalLangToEnd(ExternalLangContainer * externalLang)
{
	_externalLangArray[_nbExternalLang] = externalLang;
	++_nbExternalLang;
	++L_END;
	return _nbExternalLang-1;
}


bool NppParameters::getUserStylersFromXmlTree()
{
	TiXmlNode *root = _pXmlUserStylerDoc->FirstChild(TEXT("NotepadPlus"));
		if (!root) return false;
	return feedStylerArray(root);
}


bool NppParameters::getUserParametersFromXmlTree()
{
	if (!_pXmlUserDoc)
		return false;

	TiXmlNode *root = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return false;

	// Get GUI parameters
	feedGUIParameters(root);

	// Get History parameters
	feedFileListParameters(root);

	// Erase the History root
	TiXmlNode *node = root->FirstChildElement(TEXT("History"));
	root->RemoveChild(node);

	// Add a new empty History root
	TiXmlElement HistoryNode(TEXT("History"));
	root->InsertEndChild(HistoryNode);

	//Get Find history parameters
	feedFindHistoryParameters(root);

	//Get Project Panel parameters
	feedProjectPanelsParameters(root);

	//Get File browser parameters
	feedFileBrowserParameters(root);

	//Get Column editor parameters
	feedColumnEditorParameters(root);

	return true;
}


std::pair<unsigned char, unsigned char> NppParameters::addUserDefineLangsFromXmlTree(TiXmlDocument *tixmldoc)
{
	if (!tixmldoc)
		return std::make_pair(static_cast<unsigned char>(0), static_cast<unsigned char>(0));

	TiXmlNode *root = tixmldoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return std::make_pair(static_cast<unsigned char>(0), static_cast<unsigned char>(0));

	return feedUserLang(root);
}



bool NppParameters::getShortcutsFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;

	TiXmlNode *root = _pXmlShortcutDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return false;

	feedShortcut(root);
	return true;
}


bool NppParameters::getMacrosFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;

	TiXmlNode *root = _pXmlShortcutDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return false;

	feedMacros(root);
	return true;
}


bool NppParameters::getUserCmdsFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;

	TiXmlNode *root = _pXmlShortcutDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return false;

	feedUserCmds(root);
	return true;
}


bool NppParameters::getPluginCmdsFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;

	TiXmlNode *root = _pXmlShortcutDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return false;

	feedPluginCustomizedCmds(root);
	return true;
}


bool NppParameters::getScintKeysFromXmlTree()
{
	if (!_pXmlShortcutDoc)
		return false;

	TiXmlNode *root = _pXmlShortcutDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return false;

	feedScintKeys(root);
	return true;
}

bool NppParameters::getBlackListFromXmlTree()
{
	if (!_pXmlBlacklistDoc)
		return false;

	TiXmlNode *root = _pXmlBlacklistDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return false;

	return feedBlacklist(root);
}

void NppParameters::initMenuKeys()
{
	int nbCommands = sizeof(winKeyDefs)/sizeof(WinMenuKeyDefinition);
	WinMenuKeyDefinition wkd;
	for (int i = 0; i < nbCommands; ++i)
	{
		wkd = winKeyDefs[i];
		Shortcut sc((wkd.specialName ? wkd.specialName : TEXT("")), wkd.isCtrl, wkd.isAlt, wkd.isShift, static_cast<unsigned char>(wkd.vKey));
		_shortcuts.push_back( CommandShortcut(sc, wkd.functionId) );
	}
}

void NppParameters::initScintillaKeys()
{
	int nbCommands = sizeof(scintKeyDefs)/sizeof(ScintillaKeyDefinition);

	//Warning! Matching function have to be consecutive
	ScintillaKeyDefinition skd;
	int prevIndex = -1;
	int prevID = -1;
	for (int i = 0; i < nbCommands; ++i)
	{
		skd = scintKeyDefs[i];
		if (skd.functionId == prevID)
		{
			KeyCombo kc;
			kc._isCtrl = skd.isCtrl;
			kc._isAlt = skd.isAlt;
			kc._isShift = skd.isShift;
			kc._key = static_cast<unsigned char>(skd.vKey);
			_scintillaKeyCommands[prevIndex].addKeyCombo(kc);
		}
		else
		{
			Shortcut s = Shortcut(skd.name, skd.isCtrl, skd.isAlt, skd.isShift, static_cast<unsigned char>(skd.vKey));
			ScintillaKeyMap sm = ScintillaKeyMap(s, skd.functionId, skd.redirFunctionId);
			_scintillaKeyCommands.push_back(sm);
			++prevIndex;
		}
		prevID = skd.functionId;
	}
}
bool NppParameters::reloadContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu)
{
	_contextMenuItems.clear();
	return getContextMenuFromXmlTree(mainMenuHadle, pluginsMenu);
}

int NppParameters::getCmdIdFromMenuEntryItemName(HMENU mainMenuHadle, const generic_string& menuEntryName, const generic_string& menuItemName)
{
	int nbMenuEntry = ::GetMenuItemCount(mainMenuHadle);
	for (int i = 0; i < nbMenuEntry; ++i)
	{
		TCHAR menuEntryString[64];
		::GetMenuString(mainMenuHadle, i, menuEntryString, 64, MF_BYPOSITION);
		if (generic_stricmp(menuEntryName.c_str(), purgeMenuItemString(menuEntryString).c_str()) == 0)
		{
			vector< pair<HMENU, int> > parentMenuPos;
			HMENU topMenu = ::GetSubMenu(mainMenuHadle, i);
			int maxTopMenuPos = ::GetMenuItemCount(topMenu);
			HMENU currMenu = topMenu;
			int currMaxMenuPos = maxTopMenuPos;

			int currMenuPos = 0;
			bool notFound = false;

			do {
				if (::GetSubMenu(currMenu, currMenuPos))
				{
					//  Go into sub menu
					parentMenuPos.push_back(::make_pair(currMenu, currMenuPos));
					currMenu = ::GetSubMenu(currMenu, currMenuPos);
					currMenuPos = 0;
					currMaxMenuPos = ::GetMenuItemCount(currMenu);
				}
				else
				{
					//  Check current menu position.
					TCHAR cmdStr[256];
					::GetMenuString(currMenu, currMenuPos, cmdStr, 256, MF_BYPOSITION);
					if (generic_stricmp(menuItemName.c_str(), purgeMenuItemString(cmdStr).c_str()) == 0)
					{
						return ::GetMenuItemID(currMenu, currMenuPos);
					}

					if ((currMenuPos >= currMaxMenuPos) && (parentMenuPos.size() > 0))
					{
						currMenu = parentMenuPos.back().first;
						currMenuPos = parentMenuPos.back().second;
						parentMenuPos.pop_back();
						currMaxMenuPos = ::GetMenuItemCount(currMenu);
					}

					if ((currMenu == topMenu) && (currMenuPos >= maxTopMenuPos))
					{
						notFound = true;
					}
					else
					{
						++currMenuPos;
					}
				}
			} while (!notFound);
		}
	}
	return -1;
}

int NppParameters::getPluginCmdIdFromMenuEntryItemName(HMENU pluginsMenu, const generic_string& pluginName, const generic_string& pluginCmdName)
{
	int nbPlugins = ::GetMenuItemCount(pluginsMenu);
	for (int i = 0; i < nbPlugins; ++i)
	{
		TCHAR menuItemString[256];
		::GetMenuString(pluginsMenu, i, menuItemString, 256, MF_BYPOSITION);
		if (generic_stricmp(pluginName.c_str(), purgeMenuItemString(menuItemString).c_str()) == 0)
		{
			HMENU pluginMenu = ::GetSubMenu(pluginsMenu, i);
			int nbPluginCmd = ::GetMenuItemCount(pluginMenu);
			for (int j = 0; j < nbPluginCmd; ++j)
			{
				TCHAR pluginCmdStr[256];
				::GetMenuString(pluginMenu, j, pluginCmdStr, 256, MF_BYPOSITION);
				if (generic_stricmp(pluginCmdName.c_str(), purgeMenuItemString(pluginCmdStr).c_str()) == 0)
				{
					return ::GetMenuItemID(pluginMenu, j);
				}
			}
		}
	}
	return -1;
}

bool NppParameters::getContextMenuFromXmlTree(HMENU mainMenuHadle, HMENU pluginsMenu, bool isEditCM)
{
	std::vector<MenuItemUnit>& contextMenuItems = isEditCM ? _contextMenuItems : _tabContextMenuItems;
	TiXmlDocumentA* pXmlContextMenuDocA = isEditCM ? _pXmlContextMenuDocA : _pXmlTabContextMenuDocA;
	std::string cmName = isEditCM ? "ScintillaContextMenu" : "TabContextMenu";

	if (!pXmlContextMenuDocA)
		return false;
	TiXmlNodeA *root = pXmlContextMenuDocA->FirstChild("NotepadPlus");
	if (!root)
		return false;

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	NativeLangSpeaker* pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();

	TiXmlNodeA *contextMenuRoot = root->FirstChildElement(cmName.c_str());
	if (contextMenuRoot)
	{
		for (TiXmlNodeA *childNode = contextMenuRoot->FirstChildElement("Item");
			childNode ;
			childNode = childNode->NextSibling("Item") )
		{
			const char *folderNameDefaultA = (childNode->ToElement())->Attribute("FolderName");
			const char *folderNameTranslateID_A = (childNode->ToElement())->Attribute("TranslateID");
			const char *displayAsA = (childNode->ToElement())->Attribute("ItemNameAs");

			generic_string folderName;
			generic_string displayAs;
			folderName = folderNameDefaultA ? wmc.char2wchar(folderNameDefaultA, SC_CP_UTF8) : TEXT("");
			displayAs = displayAsA ? wmc.char2wchar(displayAsA, SC_CP_UTF8) : TEXT("");

			if (folderNameTranslateID_A)
			{
				folderName = pNativeSpeaker->getLocalizedStrFromID(folderNameTranslateID_A, folderName);
			}

			int id;
			const char *idStr = (childNode->ToElement())->Attribute("id", &id);
			if (idStr)
			{
				contextMenuItems.push_back(MenuItemUnit(id, displayAs.c_str(), folderName.c_str()));
			}
			else
			{
				const char *menuEntryNameA = (childNode->ToElement())->Attribute("MenuEntryName");
				const char *menuItemNameA = (childNode->ToElement())->Attribute("MenuItemName");

				generic_string menuEntryName;
				generic_string menuItemName;
				menuEntryName = menuEntryNameA?wmc.char2wchar(menuEntryNameA, SC_CP_UTF8):TEXT("");
				menuItemName = menuItemNameA?wmc.char2wchar(menuItemNameA, SC_CP_UTF8):TEXT("");

				if (!menuEntryName.empty() && !menuItemName.empty())
				{
					int cmd = getCmdIdFromMenuEntryItemName(mainMenuHadle, menuEntryName, menuItemName);
					if (cmd != -1)
						contextMenuItems.push_back(MenuItemUnit(cmd, displayAs.c_str(), folderName.c_str()));
				}
				else
				{
					const char *pluginNameA = (childNode->ToElement())->Attribute("PluginEntryName");
					const char *pluginCmdNameA = (childNode->ToElement())->Attribute("PluginCommandItemName");

					generic_string pluginName;
					generic_string pluginCmdName;
					pluginName = pluginNameA?wmc.char2wchar(pluginNameA, SC_CP_UTF8):TEXT("");
					pluginCmdName = pluginCmdNameA?wmc.char2wchar(pluginCmdNameA, SC_CP_UTF8):TEXT("");

					// if plugin menu existing plls the value of PluginEntryName and PluginCommandItemName are valid
					if (pluginsMenu && !pluginName.empty() && !pluginCmdName.empty())
					{
						int pluginCmdId = getPluginCmdIdFromMenuEntryItemName(pluginsMenu, pluginName, pluginCmdName);
						if (pluginCmdId != -1)
							contextMenuItems.push_back(MenuItemUnit(pluginCmdId, displayAs.c_str(), folderName.c_str()));
					}
				}
			}
		}
	}
	return true;
}


void NppParameters::setWorkingDir(const TCHAR * newPath)
{
	if (newPath && newPath[0])
	{
		_currentDirectory = newPath;
	}
	else
	{
		if (PathFileExists(_nppGUI._defaultDirExp))
			_currentDirectory = _nppGUI._defaultDirExp;
		else
			_currentDirectory = _nppPath.c_str();
	}
}

bool NppParameters::loadSession(Session & session, const TCHAR *sessionFileName)
{
	TiXmlDocument *pXmlSessionDocument = new TiXmlDocument(sessionFileName);
	bool loadOkay = pXmlSessionDocument->LoadFile();
	if (loadOkay)
		loadOkay = getSessionFromXmlTree(pXmlSessionDocument, session);

	delete pXmlSessionDocument;
	return loadOkay;
}


bool NppParameters::getSessionFromXmlTree(TiXmlDocument *pSessionDoc, Session& session)
{
	if (!pSessionDoc)
		return false;
	
	TiXmlNode *root = pSessionDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
		return false;

	TiXmlNode *sessionRoot = root->FirstChildElement(TEXT("Session"));
	if (!sessionRoot)
		return false;

	TiXmlElement *actView = sessionRoot->ToElement();
	int index = 0;
	const TCHAR *str = actView->Attribute(TEXT("activeView"), &index);
	if (str)
	{
		session._activeView = index;
	}

	const size_t nbView = 2;
	TiXmlNode *viewRoots[nbView];
	viewRoots[0] = sessionRoot->FirstChildElement(TEXT("mainView"));
	viewRoots[1] = sessionRoot->FirstChildElement(TEXT("subView"));
	for (size_t k = 0; k < nbView; ++k)
	{
		if (viewRoots[k])
		{
			int index2 = 0;
			TiXmlElement *actIndex = viewRoots[k]->ToElement();
			str = actIndex->Attribute(TEXT("activeIndex"), &index2);
			if (str)
			{
				if (k == 0)
					session._activeMainIndex = index2;
				else // k == 1
					session._activeSubIndex = index2;
			}
			for (TiXmlNode *childNode = viewRoots[k]->FirstChildElement(TEXT("File"));
				childNode ;
				childNode = childNode->NextSibling(TEXT("File")) )
			{
				const TCHAR *fileName = (childNode->ToElement())->Attribute(TEXT("filename"));
				if (fileName)
				{
					Position position;
					const TCHAR* posStr = (childNode->ToElement())->Attribute(TEXT("firstVisibleLine"));
					if (posStr)
						position._firstVisibleLine = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(TEXT("xOffset"));
					if (posStr)
						position._xOffset = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(TEXT("startPos"));
					if (posStr)
						position._startPos = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(TEXT("endPos"));
					if (posStr)
						position._endPos = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(TEXT("selMode"));
					if (posStr)
						position._selMode = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(TEXT("scrollWidth"));
					if (posStr)
						position._scrollWidth = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(TEXT("offset"));
					if (posStr)
						position._offset = static_cast<intptr_t>(_ttoi64(posStr));
					posStr = (childNode->ToElement())->Attribute(TEXT("wrapCount"));
					if (posStr)
						position._wrapCount = static_cast<intptr_t>(_ttoi64(posStr));

					MapPosition mapPosition;
					const TCHAR* mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapFirstVisibleDisplayLine"));
					if (mapPosStr)
						mapPosition._firstVisibleDisplayLine = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapFirstVisibleDocLine"));
					if (mapPosStr)
						mapPosition._firstVisibleDocLine = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapLastVisibleDocLine"));
					if (mapPosStr)
						mapPosition._lastVisibleDocLine = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapNbLine"));
					if (mapPosStr)
						mapPosition._nbLine = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapHigherPos"));
					if (mapPosStr)
						mapPosition._higherPos = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapWidth"));
					if (mapPosStr)
						mapPosition._width = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapHeight"));
					if (mapPosStr)
						mapPosition._height = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapKByteInDoc"));
					if (mapPosStr)
						mapPosition._KByteInDoc = static_cast<intptr_t>(_ttoi64(mapPosStr));
					mapPosStr = (childNode->ToElement())->Attribute(TEXT("mapWrapIndentMode"));
					if (mapPosStr)
						mapPosition._wrapIndentMode = static_cast<intptr_t>(_ttoi64(mapPosStr));
					const TCHAR *boolStr = (childNode->ToElement())->Attribute(TEXT("mapIsWrap"));
					if (boolStr)
						mapPosition._isWrap = (lstrcmp(TEXT("yes"), boolStr) == 0);

					const TCHAR *langName;
					langName = (childNode->ToElement())->Attribute(TEXT("lang"));
					int encoding = -1;
					const TCHAR *encStr = (childNode->ToElement())->Attribute(TEXT("encoding"), &encoding);
					const TCHAR *backupFilePath = (childNode->ToElement())->Attribute(TEXT("backupFilePath"));

					FILETIME fileModifiedTimestamp{};
					(childNode->ToElement())->Attribute(TEXT("originalFileLastModifTimestamp"), reinterpret_cast<int32_t*>(&fileModifiedTimestamp.dwLowDateTime));
					(childNode->ToElement())->Attribute(TEXT("originalFileLastModifTimestampHigh"), reinterpret_cast<int32_t*>(&fileModifiedTimestamp.dwHighDateTime));

					bool isUserReadOnly = false;
					const TCHAR *boolStrReadOnly = (childNode->ToElement())->Attribute(TEXT("userReadOnly"));
					if (boolStrReadOnly)
						isUserReadOnly = _wcsicmp(TEXT("yes"), boolStrReadOnly) == 0;

					sessionFileInfo sfi(fileName, langName, encStr ? encoding : -1, isUserReadOnly, position, backupFilePath, fileModifiedTimestamp, mapPosition);

					const TCHAR* intStrTabColour = (childNode->ToElement())->Attribute(TEXT("tabColourId"));
					if (intStrTabColour)
					{
						sfi._individualTabColour = generic_atoi(intStrTabColour);
					}

					for (TiXmlNode *markNode = childNode->FirstChildElement(TEXT("Mark"));
						markNode;
						markNode = markNode->NextSibling(TEXT("Mark")))
					{
						const TCHAR* lineNumberStr = (markNode->ToElement())->Attribute(TEXT("line"));
						if (lineNumberStr)
						{
							sfi._marks.push_back(static_cast<size_t>(_ttoi64(lineNumberStr)));
						}
					}

					for (TiXmlNode *foldNode = childNode->FirstChildElement(TEXT("Fold"));
						foldNode;
						foldNode = foldNode->NextSibling(TEXT("Fold")))
					{
						const TCHAR *lineNumberStr = (foldNode->ToElement())->Attribute(TEXT("line"));
						if (lineNumberStr)
						{
							sfi._foldStates.push_back(static_cast<size_t>(_ttoi64(lineNumberStr)));
						}
					}
					if (k == 0)
						session._mainViewFiles.push_back(sfi);
					else // k == 1
						session._subViewFiles.push_back(sfi);
				}
			}
		}
	}

	// Node structure and naming corresponds to config.xml
	TiXmlNode *fileBrowserRoot = sessionRoot->FirstChildElement(TEXT("FileBrowser"));
	if (fileBrowserRoot)
	{
		const TCHAR *selectedItemPath = (fileBrowserRoot->ToElement())->Attribute(TEXT("latestSelectedItem"));
		if (selectedItemPath)
		{
			session._fileBrowserSelectedItem = selectedItemPath;
		}

		for (TiXmlNode *childNode = fileBrowserRoot->FirstChildElement(TEXT("root"));
			childNode;
			childNode = childNode->NextSibling(TEXT("root")))
		{
			const TCHAR *fileName = (childNode->ToElement())->Attribute(TEXT("foldername"));
			if (fileName)
			{
				session._fileBrowserRoots.push_back({ fileName });
			}
		}
	}

	return true;
}

void NppParameters::feedFileListParameters(TiXmlNode *node)
{
	TiXmlNode *historyRoot = node->FirstChildElement(TEXT("History"));
	if (!historyRoot) return;

	// nbMaxFile value
	int nbMaxFile;
	const TCHAR *strVal = (historyRoot->ToElement())->Attribute(TEXT("nbMaxFile"), &nbMaxFile);
	if (strVal && (nbMaxFile >= 0) && (nbMaxFile <= 50))
		_nbMaxRecentFile = nbMaxFile;

	// customLen value
	int customLen;
	strVal = (historyRoot->ToElement())->Attribute(TEXT("customLength"), &customLen);
	if (strVal)
		_recentFileCustomLength = customLen;

	// inSubMenu value
	strVal = (historyRoot->ToElement())->Attribute(TEXT("inSubMenu"));
	if (strVal)
		_putRecentFileInSubMenu = (lstrcmp(strVal, TEXT("yes")) == 0);

	for (TiXmlNode *childNode = historyRoot->FirstChildElement(TEXT("File"));
		childNode && (_nbRecentFile < NB_MAX_LRF_FILE);
		childNode = childNode->NextSibling(TEXT("File")) )
	{
		const TCHAR *filePath = (childNode->ToElement())->Attribute(TEXT("filename"));
		if (filePath)
		{
			_LRFileList[_nbRecentFile] = new generic_string(filePath);
			++_nbRecentFile;
		}
	}
}

void NppParameters::feedFileBrowserParameters(TiXmlNode *node)
{
	TiXmlNode *fileBrowserRoot = node->FirstChildElement(TEXT("FileBrowser"));
	if (!fileBrowserRoot) return;

	const TCHAR *selectedItemPath = (fileBrowserRoot->ToElement())->Attribute(TEXT("latestSelectedItem"));
	if (selectedItemPath)
	{
		_fileBrowserSelectedItemPath = selectedItemPath;
	}

	for (TiXmlNode *childNode = fileBrowserRoot->FirstChildElement(TEXT("root"));
		childNode;
		childNode = childNode->NextSibling(TEXT("root")) )
	{
		const TCHAR *filePath = (childNode->ToElement())->Attribute(TEXT("foldername"));
		if (filePath)
		{
			_fileBrowserRoot.push_back(filePath);
		}
	}
}

void NppParameters::feedProjectPanelsParameters(TiXmlNode *node)
{
	TiXmlNode *projPanelRoot = node->FirstChildElement(TEXT("ProjectPanels"));
	if (!projPanelRoot) return;

	for (TiXmlNode *childNode = projPanelRoot->FirstChildElement(TEXT("ProjectPanel"));
		childNode;
		childNode = childNode->NextSibling(TEXT("ProjectPanel")) )
	{
		int index = 0;
		const TCHAR *idStr = (childNode->ToElement())->Attribute(TEXT("id"), &index);
		if (idStr && (index >= 0 && index <= 2))
		{
			const TCHAR *filePath = (childNode->ToElement())->Attribute(TEXT("workSpaceFile"));
			if (filePath)
			{
				_workSpaceFilePathes[index] = filePath;
			}
		}
	}
}

void NppParameters::feedColumnEditorParameters(TiXmlNode *node)
{
	TiXmlNode * columnEditorRoot = node->FirstChildElement(TEXT("ColumnEditor"));
	if (!columnEditorRoot) return;

	const TCHAR* strVal = (columnEditorRoot->ToElement())->Attribute(TEXT("choice"));
	if (strVal)
	{
		if (lstrcmp(strVal, TEXT("text")) == 0)
			_columnEditParam._mainChoice = false;
		else
			_columnEditParam._mainChoice = true;
	}
	TiXmlNode *childNode = columnEditorRoot->FirstChildElement(TEXT("text"));
	if (!childNode) return;

	const TCHAR* content = (childNode->ToElement())->Attribute(TEXT("content"));
	if (content)
	{
		_columnEditParam._insertedTextContent = content;
	}

	childNode = columnEditorRoot->FirstChildElement(TEXT("number"));
	if (!childNode) return;

	int val;
	strVal = (childNode->ToElement())->Attribute(TEXT("initial"), &val);
	if (strVal)
		_columnEditParam._initialNum = val;

	strVal = (childNode->ToElement())->Attribute(TEXT("increase"), &val);
	if (strVal)
		_columnEditParam._increaseNum = val;

	strVal = (childNode->ToElement())->Attribute(TEXT("repeat"), &val);
	if (strVal)
		_columnEditParam._repeatNum = val;

	strVal = (childNode->ToElement())->Attribute(TEXT("formatChoice"));

	if (strVal)
	{
		if (lstrcmp(strVal, TEXT("hex")) == 0)
			_columnEditParam._formatChoice = 1;
		else if (lstrcmp(strVal, TEXT("oct")) == 0)
			_columnEditParam._formatChoice = 2;
		else if (lstrcmp(strVal, TEXT("bin")) == 0)
			_columnEditParam._formatChoice = 3;
		else // "bin"
			_columnEditParam._formatChoice = 0;
	}

	const TCHAR* boolStr = (childNode->ToElement())->Attribute(TEXT("leadingZeros"));
	if (boolStr)
		_columnEditParam._isLeadingZeros = (lstrcmp(TEXT("yes"), boolStr) == 0);
}

void NppParameters::feedFindHistoryParameters(TiXmlNode *node)
{
	TiXmlNode *findHistoryRoot = node->FirstChildElement(TEXT("FindHistory"));
	if (!findHistoryRoot) return;

	(findHistoryRoot->ToElement())->Attribute(TEXT("nbMaxFindHistoryPath"), &_findHistory._nbMaxFindHistoryPath);
	if (_findHistory._nbMaxFindHistoryPath > NB_MAX_FINDHISTORY_PATH)
	{
		_findHistory._nbMaxFindHistoryPath = NB_MAX_FINDHISTORY_PATH;
	}
	if ((_findHistory._nbMaxFindHistoryPath > 0) && (_findHistory._nbMaxFindHistoryPath <= NB_MAX_FINDHISTORY_PATH))
	{
		for (TiXmlNode *childNode = findHistoryRoot->FirstChildElement(TEXT("Path"));
			childNode && (_findHistory._findHistoryPaths.size() < NB_MAX_FINDHISTORY_PATH);
			childNode = childNode->NextSibling(TEXT("Path")) )
		{
			const TCHAR *filePath = (childNode->ToElement())->Attribute(TEXT("name"));
			if (filePath)
			{
				_findHistory._findHistoryPaths.push_back(generic_string(filePath));
			}
		}
	}

	(findHistoryRoot->ToElement())->Attribute(TEXT("nbMaxFindHistoryFilter"), &_findHistory._nbMaxFindHistoryFilter);
	if (_findHistory._nbMaxFindHistoryFilter > NB_MAX_FINDHISTORY_FILTER)
	{
		_findHistory._nbMaxFindHistoryFilter = NB_MAX_FINDHISTORY_FILTER;
	}
	if ((_findHistory._nbMaxFindHistoryFilter > 0) && (_findHistory._nbMaxFindHistoryFilter <= NB_MAX_FINDHISTORY_FILTER))
	{
		for (TiXmlNode *childNode = findHistoryRoot->FirstChildElement(TEXT("Filter"));
			childNode && (_findHistory._findHistoryFilters.size() < NB_MAX_FINDHISTORY_FILTER);
			childNode = childNode->NextSibling(TEXT("Filter")))
		{
			const TCHAR *fileFilter = (childNode->ToElement())->Attribute(TEXT("name"));
			if (fileFilter)
			{
				_findHistory._findHistoryFilters.push_back(generic_string(fileFilter));
			}
		}
	}

	(findHistoryRoot->ToElement())->Attribute(TEXT("nbMaxFindHistoryFind"), &_findHistory._nbMaxFindHistoryFind);
	if (_findHistory._nbMaxFindHistoryFind > NB_MAX_FINDHISTORY_FIND)
	{
		_findHistory._nbMaxFindHistoryFind = NB_MAX_FINDHISTORY_FIND;
	}
	if ((_findHistory._nbMaxFindHistoryFind > 0) && (_findHistory._nbMaxFindHistoryFind <= NB_MAX_FINDHISTORY_FIND))
	{
		for (TiXmlNode *childNode = findHistoryRoot->FirstChildElement(TEXT("Find"));
			childNode && (_findHistory._findHistoryFinds.size() < NB_MAX_FINDHISTORY_FIND);
			childNode = childNode->NextSibling(TEXT("Find")))
		{
			const TCHAR *fileFind = (childNode->ToElement())->Attribute(TEXT("name"));
			if (fileFind)
			{
				_findHistory._findHistoryFinds.push_back(generic_string(fileFind));
			}
		}
	}

	(findHistoryRoot->ToElement())->Attribute(TEXT("nbMaxFindHistoryReplace"), &_findHistory._nbMaxFindHistoryReplace);
	if (_findHistory._nbMaxFindHistoryReplace > NB_MAX_FINDHISTORY_REPLACE)
	{
		_findHistory._nbMaxFindHistoryReplace = NB_MAX_FINDHISTORY_REPLACE;
	}
	if ((_findHistory._nbMaxFindHistoryReplace > 0) && (_findHistory._nbMaxFindHistoryReplace <= NB_MAX_FINDHISTORY_REPLACE))
	{
		for (TiXmlNode *childNode = findHistoryRoot->FirstChildElement(TEXT("Replace"));
			childNode && (_findHistory._findHistoryReplaces.size() < NB_MAX_FINDHISTORY_REPLACE);
			childNode = childNode->NextSibling(TEXT("Replace")))
		{
			const TCHAR *fileReplace = (childNode->ToElement())->Attribute(TEXT("name"));
			if (fileReplace)
			{
				_findHistory._findHistoryReplaces.push_back(generic_string(fileReplace));
			}
		}
	}

	const TCHAR *boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("matchWord"));
	if (boolStr)
		_findHistory._isMatchWord = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("matchCase"));
	if (boolStr)
		_findHistory._isMatchCase = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("wrap"));
	if (boolStr)
		_findHistory._isWrap = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("directionDown"));
	if (boolStr)
		_findHistory._isDirectionDown = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("fifRecuisive"));
	if (boolStr)
		_findHistory._isFifRecuisive = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("fifInHiddenFolder"));
	if (boolStr)
		_findHistory._isFifInHiddenFolder = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("fifProjectPanel1"));
	if (boolStr)
		_findHistory._isFifProjectPanel_1 = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("fifProjectPanel2"));
	if (boolStr)
		_findHistory._isFifProjectPanel_2 = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("fifProjectPanel3"));
	if (boolStr)
		_findHistory._isFifProjectPanel_3 = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("fifFilterFollowsDoc"));
	if (boolStr)
		_findHistory._isFilterFollowDoc = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("fifFolderFollowsDoc"));
	if (boolStr)
		_findHistory._isFolderFollowDoc = (lstrcmp(TEXT("yes"), boolStr) == 0);

	int mode = 0;
	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("searchMode"), &mode);
	if (boolStr)
		_findHistory._searchMode = (FindHistory::searchMode)mode;

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("transparencyMode"), &mode);
	if (boolStr)
		_findHistory._transparencyMode = (FindHistory::transparencyMode)mode;

	(findHistoryRoot->ToElement())->Attribute(TEXT("transparency"), &_findHistory._transparency);
	if (_findHistory._transparency <= 0 || _findHistory._transparency > 200)
		_findHistory._transparency = 150;

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("dotMatchesNewline"));
	if (boolStr)
		_findHistory._dotMatchesNewline = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("isSearch2ButtonsMode"));
	if (boolStr)
		_findHistory._isSearch2ButtonsMode = (lstrcmp(TEXT("yes"), boolStr) == 0);

	boolStr = (findHistoryRoot->ToElement())->Attribute(TEXT("regexBackward4PowerUser"));
	if (boolStr)
		_findHistory._regexBackward4PowerUser = (lstrcmp(TEXT("yes"), boolStr) == 0);
}

void NppParameters::feedShortcut(TiXmlNode *node)
{
	TiXmlNode *shortcutsRoot = node->FirstChildElement(TEXT("InternalCommands"));
	if (!shortcutsRoot) return;

	for (TiXmlNode *childNode = shortcutsRoot->FirstChildElement(TEXT("Shortcut"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("Shortcut")) )
	{
		int id;
		const TCHAR *idStr = (childNode->ToElement())->Attribute(TEXT("id"), &id);
		if (idStr)
		{
			//find the commandid that matches this Shortcut sc and alter it, push back its index in the modified list, if not present
			size_t len = _shortcuts.size();
			for (size_t i = 0; i < len; ++i)
			{
				if (_shortcuts[i].getID() == (unsigned long)id)
				{	//found our match
					getShortcuts(childNode, _shortcuts[i]);
					addUserModifiedIndex(i);
				}
			}
		}
	}
}

void NppParameters::feedMacros(TiXmlNode *node)
{
	TiXmlNode *macrosRoot = node->FirstChildElement(TEXT("Macros"));
	if (!macrosRoot) return;

	for (TiXmlNode *childNode = macrosRoot->FirstChildElement(TEXT("Macro"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("Macro")) )
	{
		Shortcut sc;
		generic_string fdnm;
		if (getShortcuts(childNode, sc, &fdnm))
		{
			Macro macro;
			getActions(childNode, macro);
			int cmdID = ID_MACRO + static_cast<int32_t>(_macros.size());
			_macros.push_back(MacroShortcut(sc, macro, cmdID));
			_macroMenuItems.push_back(MenuItemUnit(cmdID, sc.getName(), fdnm));
		}
	}
}


void NppParameters::getActions(TiXmlNode *node, Macro & macro)
{
	for (TiXmlNode *childNode = node->FirstChildElement(TEXT("Action"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("Action")) )
	{
		int type;
		const TCHAR *typeStr = (childNode->ToElement())->Attribute(TEXT("type"), &type);
		if ((!typeStr) || (type > 3))
			continue;

		int msg = 0;
		(childNode->ToElement())->Attribute(TEXT("message"), &msg);

		int wParam = 0;
		(childNode->ToElement())->Attribute(TEXT("wParam"), &wParam);

		int lParam = 0;
		(childNode->ToElement())->Attribute(TEXT("lParam"), &lParam);

		const TCHAR *sParam = (childNode->ToElement())->Attribute(TEXT("sParam"));
		if (!sParam)
			sParam = TEXT("");
		recordedMacroStep step(msg, wParam, lParam, sParam, type);
		if (step.isValid())
			macro.push_back(step);

	}
}

void NppParameters::feedUserCmds(TiXmlNode *node)
{
	TiXmlNode *userCmdsRoot = node->FirstChildElement(TEXT("UserDefinedCommands"));
	if (!userCmdsRoot) return;

	for (TiXmlNode *childNode = userCmdsRoot->FirstChildElement(TEXT("Command"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("Command")) )
	{
		Shortcut sc;
		generic_string fdnm;
		if (getShortcuts(childNode, sc, &fdnm))
		{
			TiXmlNode *aNode = childNode->FirstChild();
			if (aNode)
			{
				const TCHAR *cmdStr = aNode->Value();
				if (cmdStr)
				{
					int cmdID = ID_USER_CMD + static_cast<int32_t>(_userCommands.size());
					_userCommands.push_back(UserCommand(sc, cmdStr, cmdID));
					_runMenuItems.push_back(MenuItemUnit(cmdID, sc.getName(), fdnm));
				}
			}
		}
	}
}

void NppParameters::feedPluginCustomizedCmds(TiXmlNode *node)
{
	TiXmlNode *pluginCustomizedCmdsRoot = node->FirstChildElement(TEXT("PluginCommands"));
	if (!pluginCustomizedCmdsRoot) return;

	for (TiXmlNode *childNode = pluginCustomizedCmdsRoot->FirstChildElement(TEXT("PluginCommand"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("PluginCommand")) )
	{
		const TCHAR *moduleName = (childNode->ToElement())->Attribute(TEXT("moduleName"));
		if (!moduleName)
			continue;

		int internalID = -1;
		const TCHAR *internalIDStr = (childNode->ToElement())->Attribute(TEXT("internalID"), &internalID);

		if (!internalIDStr)
			continue;

		//Find the corresponding plugincommand and alter it, put the index in the list
		size_t len = _pluginCommands.size();
		for (size_t i = 0; i < len; ++i)
		{
			PluginCmdShortcut & pscOrig = _pluginCommands[i];
			if (!generic_strnicmp(pscOrig.getModuleName(), moduleName, lstrlen(moduleName)) && pscOrig.getInternalID() == internalID)
			{
				//Found matching command
				getShortcuts(childNode, _pluginCommands[i]);
				addPluginModifiedIndex(i);
				break;
			}
		}
	}
}

void NppParameters::feedScintKeys(TiXmlNode *node)
{
	TiXmlNode *scintKeysRoot = node->FirstChildElement(TEXT("ScintillaKeys"));
	if (!scintKeysRoot) return;

	for (TiXmlNode *childNode = scintKeysRoot->FirstChildElement(TEXT("ScintKey"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("ScintKey")) )
	{
		int scintKey;
		const TCHAR *keyStr = (childNode->ToElement())->Attribute(TEXT("ScintID"), &scintKey);
		if (!keyStr)
			continue;

		int menuID;
		keyStr = (childNode->ToElement())->Attribute(TEXT("menuCmdID"), &menuID);
		if (!keyStr)
			continue;

		//Find the corresponding scintillacommand and alter it, put the index in the list
		size_t len = _scintillaKeyCommands.size();
		for (int32_t i = 0; i < static_cast<int32_t>(len); ++i)
		{
			ScintillaKeyMap & skmOrig = _scintillaKeyCommands[i];
			if (skmOrig.getScintillaKeyID() == (unsigned long)scintKey && skmOrig.getMenuCmdID() == menuID)
			{
				//Found matching command
				_scintillaKeyCommands[i].clearDups();
				getShortcuts(childNode, _scintillaKeyCommands[i]);
				_scintillaKeyCommands[i].setKeyComboByIndex(0, _scintillaKeyCommands[i].getKeyCombo());
				addScintillaModifiedIndex(i);
				KeyCombo kc;
				for (TiXmlNode *nextNode = childNode->FirstChildElement(TEXT("NextKey"));
					nextNode ;
					nextNode = nextNode->NextSibling(TEXT("NextKey")))
				{
					const TCHAR *str = (nextNode->ToElement())->Attribute(TEXT("Ctrl"));
					if (!str)
						continue;
					kc._isCtrl = (lstrcmp(TEXT("yes"), str) == 0);

					str = (nextNode->ToElement())->Attribute(TEXT("Alt"));
					if (!str)
						continue;
					kc._isAlt = (lstrcmp(TEXT("yes"), str) == 0);

					str = (nextNode->ToElement())->Attribute(TEXT("Shift"));
					if (!str)
						continue;
					kc._isShift = (lstrcmp(TEXT("yes"), str) == 0);

					int key;
					str = (nextNode->ToElement())->Attribute(TEXT("Key"), &key);
					if (!str)
						continue;
					kc._key = static_cast<unsigned char>(key);
					_scintillaKeyCommands[i].addKeyCombo(kc);
				}
				break;
			}
		}
	}
}

bool NppParameters::feedBlacklist(TiXmlNode *node)
{
	TiXmlNode *blackListRoot = node->FirstChildElement(TEXT("PluginBlackList"));
	if (!blackListRoot) return false;

	for (TiXmlNode *childNode = blackListRoot->FirstChildElement(TEXT("Plugin"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("Plugin")) )
	{
		const TCHAR *name = (childNode->ToElement())->Attribute(TEXT("name"));
		if (name)
		{
			_blacklist.push_back(name);
		}
	}
	return true;
}

bool NppParameters::getShortcuts(TiXmlNode *node, Shortcut & sc, generic_string* folderName)
{
	if (!node) return false;

	const TCHAR *name = (node->ToElement())->Attribute(TEXT("name"));
	if (!name)
		name = TEXT("");

	bool isCtrl = false;
	const TCHAR *isCtrlStr = (node->ToElement())->Attribute(TEXT("Ctrl"));
	if (isCtrlStr)
		isCtrl = (lstrcmp(TEXT("yes"), isCtrlStr) == 0);

	bool isAlt = false;
	const TCHAR *isAltStr = (node->ToElement())->Attribute(TEXT("Alt"));
	if (isAltStr)
		isAlt = (lstrcmp(TEXT("yes"), isAltStr) == 0);

	bool isShift = false;
	const TCHAR *isShiftStr = (node->ToElement())->Attribute(TEXT("Shift"));
	if (isShiftStr)
		isShift = (lstrcmp(TEXT("yes"), isShiftStr) == 0);

	int key;
	const TCHAR *keyStr = (node->ToElement())->Attribute(TEXT("Key"), &key);
	if (!keyStr)
		return false;

	if (folderName)
	{
		const TCHAR* fn = (node->ToElement())->Attribute(TEXT("FolderName"));
		*folderName = fn ? fn : L"";
	}

	sc = Shortcut(name, isCtrl, isAlt, isShift, static_cast<unsigned char>(key));
	return true;
}


std::pair<unsigned char, unsigned char> NppParameters::feedUserLang(TiXmlNode *node)
{
	int iBegin = _nbUserLang;

	for (TiXmlNode *childNode = node->FirstChildElement(TEXT("UserLang"));
		childNode && (_nbUserLang < NB_MAX_USER_LANG);
		childNode = childNode->NextSibling(TEXT("UserLang")) )
	{
		const TCHAR* name = (childNode->ToElement())->Attribute(TEXT("name"));
		const TCHAR* ext = (childNode->ToElement())->Attribute(TEXT("ext"));
		const TCHAR* darkModeTheme = (childNode->ToElement())->Attribute(TEXT("darkModeTheme"));
		const TCHAR* udlVersion = (childNode->ToElement())->Attribute(TEXT("udlVersion"));

		if (!name || !name[0] || !ext)
		{
			// UserLang name is missing, just ignore this entry
			continue;
		}

		bool isDarkModeTheme = false;

		if (darkModeTheme && darkModeTheme[0])
		{
			isDarkModeTheme = (lstrcmp(TEXT("yes"), darkModeTheme) == 0);
		}

		try {
			_userLangArray[_nbUserLang] = new UserLangContainer(name, ext, isDarkModeTheme, udlVersion ? udlVersion : TEXT(""));

			++_nbUserLang;

			TiXmlNode *settingsRoot = childNode->FirstChildElement(TEXT("Settings"));
			if (!settingsRoot)
				throw std::runtime_error("NppParameters::feedUserLang : Settings node is missing");

			feedUserSettings(settingsRoot);

			TiXmlNode *keywordListsRoot = childNode->FirstChildElement(TEXT("KeywordLists"));
			if (!keywordListsRoot)
				throw std::runtime_error("NppParameters::feedUserLang : KeywordLists node is missing");

			feedUserKeywordList(keywordListsRoot);

			TiXmlNode *stylesRoot = childNode->FirstChildElement(TEXT("Styles"));
			if (!stylesRoot)
				throw std::runtime_error("NppParameters::feedUserLang : Styles node is missing");

			feedUserStyles(stylesRoot);

			// styles that were not read from xml file should get default values
			for (int i = 0 ; i < SCE_USER_STYLE_TOTAL_STYLES ; ++i)
			{
				const Style * pStyle = _userLangArray[_nbUserLang - 1]->_styles.findByID(i);
				if (!pStyle)
					_userLangArray[_nbUserLang - 1]->_styles.addStyler(i, globalMappper().styleNameMapper[i]);
			}

		}
		catch (const std::exception&)
		{
			delete _userLangArray[--_nbUserLang];
		}
	}
	int iEnd = _nbUserLang;
	return pair<unsigned char, unsigned char>(static_cast<unsigned char>(iBegin), static_cast<unsigned char>(iEnd));
}

bool NppParameters::importUDLFromFile(const generic_string& sourceFile)
{
	TiXmlDocument *pXmlUserLangDoc = new TiXmlDocument(sourceFile);

	bool loadOkay = pXmlUserLangDoc->LoadFile();
	if (loadOkay)
	{
		auto r = addUserDefineLangsFromXmlTree(pXmlUserLangDoc);
		loadOkay = (r.second - r.first) != 0;
		if (loadOkay)
		{
			_pXmlUserLangsDoc.push_back(UdlXmlFileState(nullptr, true, r));

			// imported UDL from xml file will be added into default udl, so we should make default udl dirty
			setUdlXmlDirtyFromXmlDoc(_pXmlUserLangDoc);
		}
	}
	delete pXmlUserLangDoc;
	return loadOkay;
}

bool NppParameters::exportUDLToFile(size_t langIndex2export, const generic_string& fileName2save)
{
	if (langIndex2export >= NB_MAX_USER_LANG)
		return false;

	if (static_cast<int32_t>(langIndex2export) >= _nbUserLang)
		return false;

	TiXmlDocument *pNewXmlUserLangDoc = new TiXmlDocument(fileName2save);
	TiXmlNode *newRoot2export = pNewXmlUserLangDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));

	insertUserLang2Tree(newRoot2export, _userLangArray[langIndex2export]);
	bool result = pNewXmlUserLangDoc->SaveFile();

	delete pNewXmlUserLangDoc;
	return result;
}

LangType NppParameters::getLangFromExt(const TCHAR *ext)
{
	int i = getNbLang();
	i--;
	while (i >= 0)
	{
		Lang *l = getLangFromIndex(i--);

		const TCHAR *defList = l->getDefaultExtList();
		const TCHAR *userList = NULL;

		LexerStylerArray &lsa = getLStylerArray();
		const TCHAR *lName = l->getLangName();
		LexerStyler *pLS = lsa.getLexerStylerByName(lName);

		if (pLS)
			userList = pLS->getLexerUserExt();

		generic_string list;
		if (defList)
			list += defList;

		if (userList)
		{
			list += TEXT(" ");
			list += userList;
		}
		if (isInList(ext, list.c_str()))
			return l->getLangID();
	}
	return L_TEXT;
}

void NppParameters::setCloudChoice(const TCHAR *pathChoice)
{
	generic_string cloudChoicePath = getSettingsFolder();
	cloudChoicePath += TEXT("\\cloud\\");

	if (!PathFileExists(cloudChoicePath.c_str()))
	{
		::CreateDirectory(cloudChoicePath.c_str(), NULL);
	}
	cloudChoicePath += TEXT("choice");

	WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
	std::string cloudPathA = wmc.wchar2char(pathChoice, SC_CP_UTF8);

	writeFileContent(cloudChoicePath.c_str(), cloudPathA.c_str());
}

void NppParameters::removeCloudChoice()
{
	generic_string cloudChoicePath = getSettingsFolder();

	cloudChoicePath += TEXT("\\cloud\\choice");
	if (PathFileExists(cloudChoicePath.c_str()))
	{
		::DeleteFile(cloudChoicePath.c_str());
	}
}

bool NppParameters::isCloudPathChanged() const
{
	if (_initialCloudChoice == _nppGUI._cloudPath)
		return false;
	else if (_initialCloudChoice.size() - _nppGUI._cloudPath.size() == 1)
	{
		TCHAR c = _initialCloudChoice.at(_initialCloudChoice.size()-1);
		if (c == '\\' || c == '/')
		{
			if (_initialCloudChoice.find(_nppGUI._cloudPath) == 0)
				return false;
		}
	}
	else if (_nppGUI._cloudPath.size() - _initialCloudChoice.size() == 1)
	{
		TCHAR c = _nppGUI._cloudPath.at(_nppGUI._cloudPath.size() - 1);
		if (c == '\\' || c == '/')
		{
			if (_nppGUI._cloudPath.find(_initialCloudChoice) == 0)
				return false;
		}
	}
	return true;
}

bool NppParameters::writeSettingsFilesOnCloudForThe1stTime(const generic_string & cloudSettingsPath)
{
	bool isOK = false;

	if (cloudSettingsPath.empty())
		return false;

	// config.xml
	generic_string cloudConfigPath = cloudSettingsPath;
	pathAppend(cloudConfigPath, TEXT("config.xml"));
	if (!::PathFileExists(cloudConfigPath.c_str()) && _pXmlUserDoc)
	{
		isOK = _pXmlUserDoc->SaveFile(cloudConfigPath.c_str());
		if (!isOK)
			return false;
	}

	// stylers.xml
	generic_string cloudStylersPath = cloudSettingsPath;
	pathAppend(cloudStylersPath, TEXT("stylers.xml"));
	if (!::PathFileExists(cloudStylersPath.c_str()) && _pXmlUserStylerDoc)
	{
		isOK = _pXmlUserStylerDoc->SaveFile(cloudStylersPath.c_str());
		if (!isOK)
			return false;
	}

	// langs.xml
	generic_string cloudLangsPath = cloudSettingsPath;
	pathAppend(cloudLangsPath, TEXT("langs.xml"));
	if (!::PathFileExists(cloudLangsPath.c_str()) && _pXmlUserDoc)
	{
		isOK = _pXmlDoc->SaveFile(cloudLangsPath.c_str());
		if (!isOK)
			return false;
	}

	// userDefineLang.xml
	generic_string cloudUserLangsPath = cloudSettingsPath;
	pathAppend(cloudUserLangsPath, TEXT("userDefineLang.xml"));
	if (!::PathFileExists(cloudUserLangsPath.c_str()) && _pXmlUserLangDoc)
	{
		isOK = _pXmlUserLangDoc->SaveFile(cloudUserLangsPath.c_str());
		if (!isOK)
			return false;
	}

	// shortcuts.xml
	generic_string cloudShortcutsPath = cloudSettingsPath;
	pathAppend(cloudShortcutsPath, TEXT("shortcuts.xml"));
	if (!::PathFileExists(cloudShortcutsPath.c_str()) && _pXmlShortcutDoc)
	{
		isOK = _pXmlShortcutDoc->SaveFile(cloudShortcutsPath.c_str());
		if (!isOK)
			return false;
	}

	// contextMenu.xml
	generic_string cloudContextMenuPath = cloudSettingsPath;
	pathAppend(cloudContextMenuPath, TEXT("contextMenu.xml"));
	if (!::PathFileExists(cloudContextMenuPath.c_str()) && _pXmlContextMenuDocA)
	{
		isOK = _pXmlContextMenuDocA->SaveUnicodeFilePath(cloudContextMenuPath.c_str());
		if (!isOK)
			return false;
	}

	// nativeLang.xml
	generic_string cloudNativeLangPath = cloudSettingsPath;
	pathAppend(cloudNativeLangPath, TEXT("nativeLang.xml"));
	if (!::PathFileExists(cloudNativeLangPath.c_str()) && _pXmlNativeLangDocA)
	{
		isOK = _pXmlNativeLangDocA->SaveUnicodeFilePath(cloudNativeLangPath.c_str());
		if (!isOK)
			return false;
	}

	return true;
}

/*
Default UDL + Created + Imported

*/
void NppParameters::writeDefaultUDL()
{
	bool firstCleanDone = false;
	std::vector<bool> deleteState;
	for (auto udl : _pXmlUserLangsDoc)
	{
		if (!_pXmlUserLangDoc)
		{
			_pXmlUserLangDoc = new TiXmlDocument(_userDefineLangPath);
			TiXmlDeclaration* decl = new TiXmlDeclaration(TEXT("1.0"), TEXT("UTF-8"), TEXT(""));
			_pXmlUserLangDoc->LinkEndChild(decl);
			_pXmlUserLangDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
		}

		bool toDelete = (udl._indexRange.second - udl._indexRange.first) == 0;
		deleteState.push_back(toDelete);
		if ((!udl._udlXmlDoc || udl._udlXmlDoc == _pXmlUserLangDoc) && udl._isDirty && !toDelete) // new created or/and imported UDL plus _pXmlUserLangDoc (if exist)
		{
			TiXmlNode *root = _pXmlUserLangDoc->FirstChild(TEXT("NotepadPlus"));
			if (root && !firstCleanDone)
			{
				_pXmlUserLangDoc->RemoveChild(root);
				_pXmlUserLangDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
				firstCleanDone = true;
			}

			root = _pXmlUserLangDoc->FirstChild(TEXT("NotepadPlus"));

			for (int i = udl._indexRange.first; i < udl._indexRange.second; ++i)
			{
				insertUserLang2Tree(root, _userLangArray[i]);
			}
		}
	}

	bool deleteAll = true;
	for (bool del : deleteState)
	{
		if (!del)
		{
			deleteAll = false;
			break;
		}
	}

	if (firstCleanDone) // at least one udl is for saving, the udl to be deleted are ignored
	{
		_pXmlUserLangDoc->SaveFile();
	}
	else if (deleteAll)
	{
		if (::PathFileExists(_userDefineLangPath.c_str()))
		{
			::DeleteFile(_userDefineLangPath.c_str());
		}
	}
	// else nothing to change, do nothing
}

void NppParameters::writeNonDefaultUDL()
{
	for (auto udl : _pXmlUserLangsDoc)
	{
		if (udl._isDirty && udl._udlXmlDoc != nullptr && udl._udlXmlDoc != _pXmlUserLangDoc)
		{
			if (udl._indexRange.second == udl._indexRange.first) // no more udl for this xmldoc container
			{
				// no need to save, delete file
				const TCHAR* docFilePath = udl._udlXmlDoc->Value();
				if (docFilePath && ::PathFileExists(docFilePath))
				{
					::DeleteFile(docFilePath);
				}
			}
			else
			{
				TiXmlNode *root = udl._udlXmlDoc->FirstChild(TEXT("NotepadPlus"));
				if (root)
				{
					udl._udlXmlDoc->RemoveChild(root);
				}

				udl._udlXmlDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));

				root = udl._udlXmlDoc->FirstChild(TEXT("NotepadPlus"));

				for (int i = udl._indexRange.first; i < udl._indexRange.second; ++i)
				{
					insertUserLang2Tree(root, _userLangArray[i]);
				}
				udl._udlXmlDoc->SaveFile();
			}
		}
	}
}

void NppParameters::writeNeed2SaveUDL()
{
	writeDefaultUDL();
	writeNonDefaultUDL();
}


void NppParameters::insertCmd(TiXmlNode *shortcutsRoot, const CommandShortcut & cmd)
{
	const KeyCombo & key = cmd.getKeyCombo();
	TiXmlNode *sc = shortcutsRoot->InsertEndChild(TiXmlElement(TEXT("Shortcut")));
	sc->ToElement()->SetAttribute(TEXT("id"), cmd.getID());
	sc->ToElement()->SetAttribute(TEXT("Ctrl"), key._isCtrl?TEXT("yes"):TEXT("no"));
	sc->ToElement()->SetAttribute(TEXT("Alt"), key._isAlt?TEXT("yes"):TEXT("no"));
	sc->ToElement()->SetAttribute(TEXT("Shift"), key._isShift?TEXT("yes"):TEXT("no"));
	sc->ToElement()->SetAttribute(TEXT("Key"), key._key);
}


void NppParameters::insertMacro(TiXmlNode *macrosRoot, const MacroShortcut & macro, const generic_string& folderName)
{
	const KeyCombo & key = macro.getKeyCombo();
	TiXmlNode *macroRoot = macrosRoot->InsertEndChild(TiXmlElement(TEXT("Macro")));
	macroRoot->ToElement()->SetAttribute(TEXT("name"), macro.getMenuName());
	macroRoot->ToElement()->SetAttribute(TEXT("Ctrl"), key._isCtrl?TEXT("yes"):TEXT("no"));
	macroRoot->ToElement()->SetAttribute(TEXT("Alt"), key._isAlt?TEXT("yes"):TEXT("no"));
	macroRoot->ToElement()->SetAttribute(TEXT("Shift"), key._isShift?TEXT("yes"):TEXT("no"));
	macroRoot->ToElement()->SetAttribute(TEXT("Key"), key._key);
	if (!folderName.empty())
		macroRoot->ToElement()->SetAttribute(TEXT("FolderName"), folderName);

	for (size_t i = 0, len = macro._macro.size(); i < len ; ++i)
	{
		TiXmlNode *actionNode = macroRoot->InsertEndChild(TiXmlElement(TEXT("Action")));
		const recordedMacroStep & action = macro._macro[i];
		actionNode->ToElement()->SetAttribute(TEXT("type"), action._macroType);
		actionNode->ToElement()->SetAttribute(TEXT("message"), action._message);
		actionNode->ToElement()->SetAttribute(TEXT("wParam"), static_cast<int>(action._wParameter));
		actionNode->ToElement()->SetAttribute(TEXT("lParam"), static_cast<int>(action._lParameter));
		actionNode->ToElement()->SetAttribute(TEXT("sParam"), action._sParameter.c_str());
	}
}


void NppParameters::insertUserCmd(TiXmlNode *userCmdRoot, const UserCommand & userCmd, const generic_string& folderName)
{
	const KeyCombo & key = userCmd.getKeyCombo();
	TiXmlNode *cmdRoot = userCmdRoot->InsertEndChild(TiXmlElement(TEXT("Command")));
	cmdRoot->ToElement()->SetAttribute(TEXT("name"), userCmd.getMenuName());
	cmdRoot->ToElement()->SetAttribute(TEXT("Ctrl"), key._isCtrl?TEXT("yes"):TEXT("no"));
	cmdRoot->ToElement()->SetAttribute(TEXT("Alt"), key._isAlt?TEXT("yes"):TEXT("no"));
	cmdRoot->ToElement()->SetAttribute(TEXT("Shift"), key._isShift?TEXT("yes"):TEXT("no"));
	cmdRoot->ToElement()->SetAttribute(TEXT("Key"), key._key);
	cmdRoot->InsertEndChild(TiXmlText(userCmd._cmd.c_str()));
	if (!folderName.empty())
		cmdRoot->ToElement()->SetAttribute(TEXT("FolderName"), folderName);
}


void NppParameters::insertPluginCmd(TiXmlNode *pluginCmdRoot, const PluginCmdShortcut & pluginCmd)
{
	const KeyCombo & key = pluginCmd.getKeyCombo();
	TiXmlNode *pluginCmdNode = pluginCmdRoot->InsertEndChild(TiXmlElement(TEXT("PluginCommand")));
	pluginCmdNode->ToElement()->SetAttribute(TEXT("moduleName"), pluginCmd.getModuleName());
	pluginCmdNode->ToElement()->SetAttribute(TEXT("internalID"), pluginCmd.getInternalID());
	pluginCmdNode->ToElement()->SetAttribute(TEXT("Ctrl"), key._isCtrl?TEXT("yes"):TEXT("no"));
	pluginCmdNode->ToElement()->SetAttribute(TEXT("Alt"), key._isAlt?TEXT("yes"):TEXT("no"));
	pluginCmdNode->ToElement()->SetAttribute(TEXT("Shift"), key._isShift?TEXT("yes"):TEXT("no"));
	pluginCmdNode->ToElement()->SetAttribute(TEXT("Key"), key._key);
}


void NppParameters::insertScintKey(TiXmlNode *scintKeyRoot, const ScintillaKeyMap & scintKeyMap)
{
	TiXmlNode *keyRoot = scintKeyRoot->InsertEndChild(TiXmlElement(TEXT("ScintKey")));
	keyRoot->ToElement()->SetAttribute(TEXT("ScintID"), scintKeyMap.getScintillaKeyID());
	keyRoot->ToElement()->SetAttribute(TEXT("menuCmdID"), scintKeyMap.getMenuCmdID());

	//Add main shortcut
	KeyCombo key = scintKeyMap.getKeyComboByIndex(0);
	keyRoot->ToElement()->SetAttribute(TEXT("Ctrl"), key._isCtrl?TEXT("yes"):TEXT("no"));
	keyRoot->ToElement()->SetAttribute(TEXT("Alt"), key._isAlt?TEXT("yes"):TEXT("no"));
	keyRoot->ToElement()->SetAttribute(TEXT("Shift"), key._isShift?TEXT("yes"):TEXT("no"));
	keyRoot->ToElement()->SetAttribute(TEXT("Key"), key._key);

	//Add additional shortcuts
	size_t size = scintKeyMap.getSize();
	if (size > 1)
	{
		for (size_t i = 1; i < size; ++i)
		{
			TiXmlNode *keyNext = keyRoot->InsertEndChild(TiXmlElement(TEXT("NextKey")));
			key = scintKeyMap.getKeyComboByIndex(i);
			keyNext->ToElement()->SetAttribute(TEXT("Ctrl"), key._isCtrl?TEXT("yes"):TEXT("no"));
			keyNext->ToElement()->SetAttribute(TEXT("Alt"), key._isAlt?TEXT("yes"):TEXT("no"));
			keyNext->ToElement()->SetAttribute(TEXT("Shift"), key._isShift?TEXT("yes"):TEXT("no"));
			keyNext->ToElement()->SetAttribute(TEXT("Key"), key._key);
		}
	}
}


void NppParameters::writeSession(const Session & session, const TCHAR *fileName)
{
	const TCHAR *pathName = fileName?fileName:_sessionPath.c_str();

	TiXmlDocument* pXmlSessionDoc = new TiXmlDocument(pathName);

	TiXmlDeclaration* decl = new TiXmlDeclaration(TEXT("1.0"), TEXT("UTF-8"), TEXT(""));
	pXmlSessionDoc->LinkEndChild(decl);

	TiXmlNode *root = pXmlSessionDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));

	if (root)
	{
		TiXmlNode *sessionNode = root->InsertEndChild(TiXmlElement(TEXT("Session")));
		(sessionNode->ToElement())->SetAttribute(TEXT("activeView"), static_cast<int32_t>(session._activeView));

		struct ViewElem {
			TiXmlNode *viewNode;
			vector<sessionFileInfo> *viewFiles;
			size_t activeIndex;
		};
		const int nbElem = 2;
		ViewElem viewElems[nbElem];
		viewElems[0].viewNode = sessionNode->InsertEndChild(TiXmlElement(TEXT("mainView")));
		viewElems[1].viewNode = sessionNode->InsertEndChild(TiXmlElement(TEXT("subView")));
		viewElems[0].viewFiles = (vector<sessionFileInfo> *)(&(session._mainViewFiles));
		viewElems[1].viewFiles = (vector<sessionFileInfo> *)(&(session._subViewFiles));
		viewElems[0].activeIndex = session._activeMainIndex;
		viewElems[1].activeIndex = session._activeSubIndex;

		for (size_t k = 0; k < nbElem ; ++k)
		{
			(viewElems[k].viewNode->ToElement())->SetAttribute(TEXT("activeIndex"), static_cast<int32_t>(viewElems[k].activeIndex));
			vector<sessionFileInfo> & viewSessionFiles = *(viewElems[k].viewFiles);

			for (size_t i = 0, len = viewElems[k].viewFiles->size(); i < len ; ++i)
			{
				TiXmlNode *fileNameNode = viewElems[k].viewNode->InsertEndChild(TiXmlElement(TEXT("File")));

				TCHAR szInt64[64];

				(fileNameNode->ToElement())->SetAttribute(TEXT("firstVisibleLine"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._firstVisibleLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("xOffset"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._xOffset), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("scrollWidth"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._scrollWidth), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("startPos"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._startPos), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("endPos"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._endPos), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("selMode"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._selMode), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("offset"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._offset), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("wrapCount"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._wrapCount), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("lang"), viewSessionFiles[i]._langName.c_str());
				(fileNameNode->ToElement())->SetAttribute(TEXT("encoding"), viewSessionFiles[i]._encoding);
				(fileNameNode->ToElement())->SetAttribute(TEXT("userReadOnly"), (viewSessionFiles[i]._isUserReadOnly && !viewSessionFiles[i]._isMonitoring) ? TEXT("yes") : TEXT("no"));
				(fileNameNode->ToElement())->SetAttribute(TEXT("filename"), viewSessionFiles[i]._fileName.c_str());
				(fileNameNode->ToElement())->SetAttribute(TEXT("backupFilePath"), viewSessionFiles[i]._backupFilePath.c_str());
				(fileNameNode->ToElement())->SetAttribute(TEXT("originalFileLastModifTimestamp"), static_cast<int32_t>(viewSessionFiles[i]._originalFileLastModifTimestamp.dwLowDateTime));
				(fileNameNode->ToElement())->SetAttribute(TEXT("originalFileLastModifTimestampHigh"), static_cast<int32_t>(viewSessionFiles[i]._originalFileLastModifTimestamp.dwHighDateTime));
				(fileNameNode->ToElement())->SetAttribute(TEXT("tabColourId"), static_cast<int32_t>(viewSessionFiles[i]._individualTabColour));

				// docMap 
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapFirstVisibleDisplayLine"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._firstVisibleDisplayLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapFirstVisibleDocLine"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._firstVisibleDocLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapLastVisibleDocLine"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._lastVisibleDocLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapNbLine"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._nbLine), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapHigherPos"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._higherPos), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapWidth"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._width), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapHeight"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._height), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapKByteInDoc"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._KByteInDoc), szInt64, 10));
				(fileNameNode->ToElement())->SetAttribute(TEXT("mapWrapIndentMode"), _i64tot(static_cast<LONGLONG>(viewSessionFiles[i]._mapPos._wrapIndentMode), szInt64, 10));
				fileNameNode->ToElement()->SetAttribute(TEXT("mapIsWrap"), viewSessionFiles[i]._mapPos._isWrap ? TEXT("yes") : TEXT("no"));

				for (size_t j = 0, len = viewSessionFiles[i]._marks.size() ; j < len ; ++j)
				{
					size_t markLine = viewSessionFiles[i]._marks[j];
					TiXmlNode *markNode = fileNameNode->InsertEndChild(TiXmlElement(TEXT("Mark")));
					markNode->ToElement()->SetAttribute(TEXT("line"), _ui64tot(static_cast<ULONGLONG>(markLine), szInt64, 10));
				}

				for (size_t j = 0, len = viewSessionFiles[i]._foldStates.size() ; j < len ; ++j)
				{
					size_t foldLine = viewSessionFiles[i]._foldStates[j];
					TiXmlNode *foldNode = fileNameNode->InsertEndChild(TiXmlElement(TEXT("Fold")));
					foldNode->ToElement()->SetAttribute(TEXT("line"), _ui64tot(static_cast<ULONGLONG>(foldLine), szInt64, 10));
				}
			}
		}

		if (session._includeFileBrowser)
		{
			// Node structure and naming corresponds to config.xml
			TiXmlNode* fileBrowserRootNode = sessionNode->InsertEndChild(TiXmlElement(TEXT("FileBrowser")));
			fileBrowserRootNode->ToElement()->SetAttribute(TEXT("latestSelectedItem"), session._fileBrowserSelectedItem.c_str());
			for (const auto& root : session._fileBrowserRoots)
			{
				TiXmlNode *fileNameNode = fileBrowserRootNode->InsertEndChild(TiXmlElement(TEXT("root")));
				(fileNameNode->ToElement())->SetAttribute(TEXT("foldername"), root.c_str());
			}
		}
	}
	pXmlSessionDoc->SaveFile();

	delete pXmlSessionDoc;
}


void NppParameters::writeShortcuts()
{
	if (!_isAnyShortcutModified) return;

	if (!_pXmlShortcutDoc)
	{
		//do the treatment
		_pXmlShortcutDoc = new TiXmlDocument(_shortcutsPath);
		TiXmlDeclaration* decl = new TiXmlDeclaration(TEXT("1.0"), TEXT("UTF-8"), TEXT(""));
		_pXmlShortcutDoc->LinkEndChild(decl);
	}

	TiXmlNode *root = _pXmlShortcutDoc->FirstChild(TEXT("NotepadPlus"));
	if (!root)
	{
		root = _pXmlShortcutDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *cmdRoot = root->FirstChild(TEXT("InternalCommands"));
	if (cmdRoot)
		root->RemoveChild(cmdRoot);

	cmdRoot = root->InsertEndChild(TiXmlElement(TEXT("InternalCommands")));
	for (size_t i = 0, len = _customizedShortcuts.size(); i < len ; ++i)
	{
		size_t index = _customizedShortcuts[i];
		CommandShortcut csc = _shortcuts[index];
		insertCmd(cmdRoot, csc);
	}

	TiXmlNode *macrosRoot = root->FirstChild(TEXT("Macros"));
	if (macrosRoot)
		root->RemoveChild(macrosRoot);

	macrosRoot = root->InsertEndChild(TiXmlElement(TEXT("Macros")));

	for (size_t i = 0, len = _macros.size(); i < len ; ++i)
	{
		insertMacro(macrosRoot, _macros[i], _macroMenuItems.getItemFromIndex(i)._parentFolderName);
	}

	TiXmlNode *userCmdRoot = root->FirstChild(TEXT("UserDefinedCommands"));
	if (userCmdRoot)
		root->RemoveChild(userCmdRoot);

	userCmdRoot = root->InsertEndChild(TiXmlElement(TEXT("UserDefinedCommands")));

	for (size_t i = 0, len = _userCommands.size(); i < len ; ++i)
	{
		insertUserCmd(userCmdRoot, _userCommands[i], _runMenuItems.getItemFromIndex(i)._parentFolderName);
	}

	TiXmlNode *pluginCmdRoot = root->FirstChild(TEXT("PluginCommands"));
	if (pluginCmdRoot)
		root->RemoveChild(pluginCmdRoot);

	pluginCmdRoot = root->InsertEndChild(TiXmlElement(TEXT("PluginCommands")));
	for (size_t i = 0, len = _pluginCustomizedCmds.size(); i < len ; ++i)
	{
		insertPluginCmd(pluginCmdRoot, _pluginCommands[_pluginCustomizedCmds[i]]);
	}

	TiXmlNode *scitillaKeyRoot = root->FirstChild(TEXT("ScintillaKeys"));
	if (scitillaKeyRoot)
		root->RemoveChild(scitillaKeyRoot);

	scitillaKeyRoot = root->InsertEndChild(TiXmlElement(TEXT("ScintillaKeys")));
	for (size_t i = 0, len = _scintillaModifiedKeyIndices.size(); i < len ; ++i)
	{
		insertScintKey(scitillaKeyRoot, _scintillaKeyCommands[_scintillaModifiedKeyIndices[i]]);
	}
	_pXmlShortcutDoc->SaveFile();
}


int NppParameters::addUserLangToEnd(const UserLangContainer & userLang, const TCHAR *newName)
{
	if (isExistingUserLangName(newName))
		return -1;
	unsigned char iBegin = _nbUserLang;
	_userLangArray[_nbUserLang] = new UserLangContainer();
	*(_userLangArray[_nbUserLang]) = userLang;
	_userLangArray[_nbUserLang]->_name = newName;
	++_nbUserLang;
	unsigned char iEnd = _nbUserLang;

	_pXmlUserLangsDoc.push_back(UdlXmlFileState(nullptr, true, make_pair(iBegin, iEnd)));

	// imported UDL from xml file will be added into default udl, so we should make default udl dirty
	setUdlXmlDirtyFromXmlDoc(_pXmlUserLangDoc);

	return _nbUserLang-1;
}


void NppParameters::removeUserLang(size_t index)
{
	if (static_cast<int32_t>(index) >= _nbUserLang)
		return;
	delete _userLangArray[index];

	for (int32_t i = static_cast<int32_t>(index); i < (_nbUserLang - 1); ++i)
		_userLangArray[i] = _userLangArray[i+1];
	_nbUserLang--;

	removeIndexFromXmlUdls(index);
}


void NppParameters::feedUserSettings(TiXmlNode *settingsRoot)
{
	const TCHAR *boolStr;
	TiXmlNode *globalSettingNode = settingsRoot->FirstChildElement(TEXT("Global"));
	if (globalSettingNode)
	{
		boolStr = (globalSettingNode->ToElement())->Attribute(TEXT("caseIgnored"));
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_isCaseIgnored = (lstrcmp(TEXT("yes"), boolStr) == 0);

		boolStr = (globalSettingNode->ToElement())->Attribute(TEXT("allowFoldOfComments"));
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_allowFoldOfComments = (lstrcmp(TEXT("yes"), boolStr) == 0);

		(globalSettingNode->ToElement())->Attribute(TEXT("forcePureLC"), &_userLangArray[_nbUserLang - 1]->_forcePureLC);
		(globalSettingNode->ToElement())->Attribute(TEXT("decimalSeparator"), &_userLangArray[_nbUserLang - 1]->_decimalSeparator);

		boolStr = (globalSettingNode->ToElement())->Attribute(TEXT("foldCompact"));
		if (boolStr)
			_userLangArray[_nbUserLang - 1]->_foldCompact = (lstrcmp(TEXT("yes"), boolStr) == 0);
	}

	TiXmlNode *prefixNode = settingsRoot->FirstChildElement(TEXT("Prefix"));
	if (prefixNode)
	{
		const TCHAR *udlVersion = _userLangArray[_nbUserLang - 1]->_udlVersion.c_str();
		if (!lstrcmp(udlVersion, TEXT("2.1")) || !lstrcmp(udlVersion, TEXT("2.0")))
		{
			for (int i = 0 ; i < SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
			{
				boolStr = (prefixNode->ToElement())->Attribute(globalMappper().keywordNameMapper[i+SCE_USER_KWLIST_KEYWORDS1]);
				if (boolStr)
					_userLangArray[_nbUserLang - 1]->_isPrefix[i] = (lstrcmp(TEXT("yes"), boolStr) == 0);
			}
		}
		else	// support for old style (pre 2.0)
		{
			TCHAR names[SCE_USER_TOTAL_KEYWORD_GROUPS][7] = {TEXT("words1"), TEXT("words2"), TEXT("words3"), TEXT("words4")};
			for (int i = 0 ; i < 4 ; ++i)
			{
				boolStr = (prefixNode->ToElement())->Attribute(names[i]);
				if (boolStr)
					_userLangArray[_nbUserLang - 1]->_isPrefix[i] = (lstrcmp(TEXT("yes"), boolStr) == 0);
			}
		}
	}
}


void NppParameters::feedUserKeywordList(TiXmlNode *node)
{
	const TCHAR * udlVersion = _userLangArray[_nbUserLang - 1]->_udlVersion.c_str();
	int id = -1;

	for (TiXmlNode *childNode = node->FirstChildElement(TEXT("Keywords"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("Keywords")))
	{
		const TCHAR * keywordsName = (childNode->ToElement())->Attribute(TEXT("name"));
		TiXmlNode *valueNode = childNode->FirstChild();
		if (valueNode)
		{
			const TCHAR *kwl = nullptr;
			if (!lstrcmp(udlVersion, TEXT("")) && !lstrcmp(keywordsName, TEXT("Delimiters")))	// support for old style (pre 2.0)
			{
				basic_string<TCHAR> temp;
				kwl = (valueNode)?valueNode->Value():TEXT("000000");

				temp += TEXT("00");	 if (kwl[0] != '0') temp += kwl[0];	 temp += TEXT(" 01");
				temp += TEXT(" 02");	if (kwl[3] != '0') temp += kwl[3];
				temp += TEXT(" 03");	if (kwl[1] != '0') temp += kwl[1];	 temp += TEXT(" 04");
				temp += TEXT(" 05");	if (kwl[4] != '0') temp += kwl[4];
				temp += TEXT(" 06");	if (kwl[2] != '0') temp += kwl[2];	 temp += TEXT(" 07");
				temp += TEXT(" 08");	if (kwl[5] != '0') temp += kwl[5];

				temp += TEXT(" 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23");
				wcscpy_s(_userLangArray[_nbUserLang - 1]->_keywordLists[SCE_USER_KWLIST_DELIMITERS], temp.c_str());
			}
			else if (!lstrcmp(keywordsName, TEXT("Comment")))
			{
				kwl = (valueNode)?valueNode->Value():TEXT("");
				//int len = _tcslen(kwl);
				basic_string<TCHAR> temp{TEXT(" ")};

				temp += kwl;
				size_t pos = 0;

				pos = temp.find(TEXT(" 0"));
				while (pos != string::npos)
				{
					temp.replace(pos, 2, TEXT(" 00"));
					pos = temp.find(TEXT(" 0"), pos+1);
				}
				pos = temp.find(TEXT(" 1"));
				while (pos != string::npos)
				{
					temp.replace(pos, 2, TEXT(" 03"));
					pos = temp.find(TEXT(" 1"));
				}
				pos = temp.find(TEXT(" 2"));
				while (pos != string::npos)
				{
					temp.replace(pos, 2, TEXT(" 04"));
					pos = temp.find(TEXT(" 2"));
				}

				temp += TEXT(" 01 02");
				if (temp[0] == ' ')
					temp.erase(0, 1);

				wcscpy_s(_userLangArray[_nbUserLang - 1]->_keywordLists[SCE_USER_KWLIST_COMMENTS], temp.c_str());
			}
			else
			{
				kwl = (valueNode)?valueNode->Value():TEXT("");
				if (globalMappper().keywordIdMapper.find(keywordsName) != globalMappper().keywordIdMapper.end())
				{
					id = globalMappper().keywordIdMapper[keywordsName];
					if (_tcslen(kwl) < max_char)
					{
						wcscpy_s(_userLangArray[_nbUserLang - 1]->_keywordLists[id], kwl);
					}
					else
					{
						wcscpy_s(_userLangArray[_nbUserLang - 1]->_keywordLists[id], TEXT("imported string too long, needs to be < max_char(30720)"));
					}
				}
			}
		}
	}
}

void NppParameters::feedUserStyles(TiXmlNode *node)
{
	int id = -1;

	for (TiXmlNode *childNode = node->FirstChildElement(TEXT("WordsStyle"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("WordsStyle")))
	{
		const TCHAR *styleName = (childNode->ToElement())->Attribute(TEXT("name"));
		if (styleName)
		{
			if (globalMappper().styleIdMapper.find(styleName) != globalMappper().styleIdMapper.end())
			{
				id = globalMappper().styleIdMapper[styleName];
				_userLangArray[_nbUserLang - 1]->_styles.addStyler((id | L_USER << 16), childNode);
			}
		}
	}
}

bool NppParameters::feedStylerArray(TiXmlNode *node)
{
	TiXmlNode *styleRoot = node->FirstChildElement(TEXT("LexerStyles"));
	if (!styleRoot) return false;

	// For each lexer
	for (TiXmlNode *childNode = styleRoot->FirstChildElement(TEXT("LexerType"));
		 childNode ;
		 childNode = childNode->NextSibling(TEXT("LexerType")) )
	{
		TiXmlElement *element = childNode->ToElement();
		const TCHAR *lexerName = element->Attribute(TEXT("name"));
		const TCHAR *lexerDesc = element->Attribute(TEXT("desc"));
		const TCHAR *lexerUserExt = element->Attribute(TEXT("ext"));
		const TCHAR *lexerExcluded = element->Attribute(TEXT("excluded"));
		if (lexerName)
		{
			_lexerStylerVect.addLexerStyler(lexerName, lexerDesc, lexerUserExt, childNode);
			if (lexerExcluded != NULL && (lstrcmp(lexerExcluded, TEXT("yes")) == 0))
			{
				int index = getExternalLangIndexFromName(lexerName);
				if (index != -1)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)(index + L_EXTERNAL)));
			}
		}
	}

	_lexerStylerVect.sort();

	// The global styles for all lexers
	TiXmlNode *globalStyleRoot = node->FirstChildElement(TEXT("GlobalStyles"));
	if (!globalStyleRoot) return false;

	for (TiXmlNode *childNode = globalStyleRoot->FirstChildElement(TEXT("WidgetStyle"));
		 childNode ;
		 childNode = childNode->NextSibling(TEXT("WidgetStyle")) )
	{
		TiXmlElement *element = childNode->ToElement();
		const TCHAR *styleIDStr = element->Attribute(TEXT("styleID"));

		int styleID = -1;
		if ((styleID = decStrVal(styleIDStr)) != -1)
		{
			_widgetStyleArray.addStyler(styleID, childNode);
		}
	}

	const Style* pStyle = _widgetStyleArray.findByName(TEXT("EOL custom color"));
	if (!pStyle)
	{
		TiXmlNode* eolColorkNode = globalStyleRoot->InsertEndChild(TiXmlElement(TEXT("WidgetStyle")));
		eolColorkNode->ToElement()->SetAttribute(TEXT("name"), TEXT("EOL custom color"));
		eolColorkNode->ToElement()->SetAttribute(TEXT("styleID"), TEXT("0"));
		eolColorkNode->ToElement()->SetAttribute(TEXT("fgColor"), TEXT("DADADA"));

		_widgetStyleArray.addStyler(0, eolColorkNode);
	}

	return true;
}

void LexerStylerArray::addLexerStyler(const TCHAR *lexerName, const TCHAR *lexerDesc, const TCHAR *lexerUserExt , TiXmlNode *lexerNode)
{
	_lexerStylerVect.emplace_back();
	LexerStyler & ls = _lexerStylerVect.back();
	ls.setLexerName(lexerName);
	if (lexerDesc)
		ls.setLexerDesc(lexerDesc);

	if (lexerUserExt)
		ls.setLexerUserExt(lexerUserExt);

	for (TiXmlNode *childNode = lexerNode->FirstChildElement(TEXT("WordsStyle"));
		 childNode ;
		 childNode = childNode->NextSibling(TEXT("WordsStyle")) )
	{
		TiXmlElement *element = childNode->ToElement();
		const TCHAR *styleIDStr = element->Attribute(TEXT("styleID"));

		if (styleIDStr)
		{
			int styleID = -1;
			if ((styleID = decStrVal(styleIDStr)) != -1)
			{
				ls.addStyler(styleID, childNode);
			}
		}
	}
}

void StyleArray::addStyler(int styleID, TiXmlNode *styleNode)
{
	bool isUser = styleID >> 16 == L_USER;
	if (isUser)
	{
		styleID = (styleID & 0xFFFF);
		if (styleID >= SCE_USER_STYLE_TOTAL_STYLES || findByID(styleID))
			return;
	}

	_styleVect.emplace_back();
	Style & s = _styleVect.back();
	s._styleID = styleID;

	if (styleNode)
	{
		TiXmlElement *element = styleNode->ToElement();

		// TODO: translate to English
		// Pour _fgColor, _bgColor :
		// RGB() | (result & 0xFF000000) c'est pour le cas de -1 (0xFFFFFFFF)
		// retourné par hexStrVal(str)
		const TCHAR *str = element->Attribute(TEXT("name"));
		if (str)
		{
			if (isUser)
				s._styleDesc = globalMappper().styleNameMapper[styleID];
			else
				s._styleDesc = str;
		}

		str = element->Attribute(TEXT("fgColor"));
		if (str)
		{
			unsigned long result = hexStrVal(str);
			s._fgColor = (RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000);

		}

		str = element->Attribute(TEXT("bgColor"));
		if (str)
		{
			unsigned long result = hexStrVal(str);
			s._bgColor = (RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000);
		}

		str = element->Attribute(TEXT("colorStyle"));
		if (str)
		{
			s._colorStyle = decStrVal(str);
		}

		str = element->Attribute(TEXT("fontName"));
		if (str)
		{
			s._fontName = str;
			s._isFontEnabled = true;
		}

		str = element->Attribute(TEXT("fontStyle"));
		if (str)
		{
			s._fontStyle = decStrVal(str);
		}

		str = element->Attribute(TEXT("fontSize"));
		if (str)
		{
			s._fontSize = decStrVal(str);
		}
		str = element->Attribute(TEXT("nesting"));

		if (str)
		{
			s._nesting = decStrVal(str);
		}

		str = element->Attribute(TEXT("keywordClass"));
		if (str)
		{
			s._keywordClass = getKwClassFromName(str);
		}

		TiXmlNode *v = styleNode->FirstChild();
		if (v)
		{
			s._keywords = v->Value();
		}
	}
}

bool NppParameters::writeRecentFileHistorySettings(int nbMaxFile) const
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *historyNode = nppRoot->FirstChildElement(TEXT("History"));
	if (!historyNode)
	{
		historyNode = nppRoot->InsertEndChild(TiXmlElement(TEXT("History")));
	}

	(historyNode->ToElement())->SetAttribute(TEXT("nbMaxFile"), nbMaxFile!=-1?nbMaxFile:_nbMaxRecentFile);
	(historyNode->ToElement())->SetAttribute(TEXT("inSubMenu"), _putRecentFileInSubMenu?TEXT("yes"):TEXT("no"));
	(historyNode->ToElement())->SetAttribute(TEXT("customLength"), _recentFileCustomLength);
	return true;
}

bool NppParameters::writeColumnEditorSettings() const
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *oldColumnEditorNode = nppRoot->FirstChildElement(TEXT("ColumnEditor"));
	if (oldColumnEditorNode)
	{
		// Erase the Project Panel root
		nppRoot->RemoveChild(oldColumnEditorNode);
	}

	// Create the new ColumnEditor root
	TiXmlElement columnEditorRootNode{TEXT("ColumnEditor")};
	(columnEditorRootNode.ToElement())->SetAttribute(TEXT("choice"), _columnEditParam._mainChoice ? L"number" : L"text");

	TiXmlElement textNode{ TEXT("text") };
	(textNode.ToElement())->SetAttribute(TEXT("content"), _columnEditParam._insertedTextContent.c_str());
	(columnEditorRootNode.ToElement())->InsertEndChild(textNode);

	TiXmlElement numberNode{ TEXT("number") };
	(numberNode.ToElement())->SetAttribute(TEXT("initial"), _columnEditParam._initialNum);
	(numberNode.ToElement())->SetAttribute(TEXT("increase"), _columnEditParam._increaseNum);
	(numberNode.ToElement())->SetAttribute(TEXT("repeat"), _columnEditParam._repeatNum);
	(numberNode.ToElement())->SetAttribute(TEXT("leadingZeros"), _columnEditParam._isLeadingZeros ? L"yes" : L"no");

	wstring format = TEXT("dec");
	if (_columnEditParam._formatChoice == 1)
		format = TEXT("hex");
	else if (_columnEditParam._formatChoice == 2)
		format = TEXT("oct");
	else if (_columnEditParam._formatChoice == 3)
		format = TEXT("bin");
	(numberNode.ToElement())->SetAttribute(TEXT("formatChoice"), format);
	(columnEditorRootNode.ToElement())->InsertEndChild(numberNode);



	// (Re)Insert the Project Panel root
	(nppRoot->ToElement())->InsertEndChild(columnEditorRootNode);
	return true;
}

bool NppParameters::writeProjectPanelsSettings() const
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *oldProjPanelRootNode = nppRoot->FirstChildElement(TEXT("ProjectPanels"));
	if (oldProjPanelRootNode)
	{
		// Erase the Project Panel root
		nppRoot->RemoveChild(oldProjPanelRootNode);
	}

	// Create the Project Panel root
	TiXmlElement projPanelRootNode{TEXT("ProjectPanels")};

	// Add 3 Project Panel parameters
	for (int32_t i = 0 ; i < 3 ; ++i)
	{
		TiXmlElement projPanelNode{TEXT("ProjectPanel")};
		(projPanelNode.ToElement())->SetAttribute(TEXT("id"), i);
		(projPanelNode.ToElement())->SetAttribute(TEXT("workSpaceFile"), _workSpaceFilePathes[i]);

		(projPanelRootNode.ToElement())->InsertEndChild(projPanelNode);
	}

	// (Re)Insert the Project Panel root
	(nppRoot->ToElement())->InsertEndChild(projPanelRootNode);
	return true;
}

bool NppParameters::writeFileBrowserSettings(const vector<generic_string> & rootPaths, const generic_string & latestSelectedItemPath) const
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *oldFileBrowserRootNode = nppRoot->FirstChildElement(TEXT("FileBrowser"));
	if (oldFileBrowserRootNode)
	{
		// Erase the file broser root
		nppRoot->RemoveChild(oldFileBrowserRootNode);
	}

	// Create the file browser root
	TiXmlElement fileBrowserRootNode{ TEXT("FileBrowser") };

	if (rootPaths.size() != 0)
	{
		fileBrowserRootNode.SetAttribute(TEXT("latestSelectedItem"), latestSelectedItemPath.c_str());

		// add roots
		size_t len = rootPaths.size();
		for (size_t i = 0; i < len; ++i)
		{
			TiXmlElement fbRootNode{ TEXT("root") };
			(fbRootNode.ToElement())->SetAttribute(TEXT("foldername"), rootPaths[i].c_str());

			(fileBrowserRootNode.ToElement())->InsertEndChild(fbRootNode);
		}
	}

	// (Re)Insert the file browser root
	(nppRoot->ToElement())->InsertEndChild(fileBrowserRootNode);
	return true;
}

bool NppParameters::writeHistory(const TCHAR *fullpath)
{
	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *historyNode = nppRoot->FirstChildElement(TEXT("History"));
	if (!historyNode)
	{
		historyNode = nppRoot->InsertEndChild(TiXmlElement(TEXT("History")));
	}

	TiXmlElement recentFileNode(TEXT("File"));
	(recentFileNode.ToElement())->SetAttribute(TEXT("filename"), fullpath);

	(historyNode->ToElement())->InsertEndChild(recentFileNode);
	return true;
}

TiXmlNode * NppParameters::getChildElementByAttribut(TiXmlNode *pere, const TCHAR *childName,\
			const TCHAR *attributName, const TCHAR *attributVal) const
{
	for (TiXmlNode *childNode = pere->FirstChildElement(childName);
		childNode ;
		childNode = childNode->NextSibling(childName))
	{
		TiXmlElement *element = childNode->ToElement();
		const TCHAR *val = element->Attribute(attributName);
		if (val)
		{
			if (!lstrcmp(val, attributVal))
				return childNode;
		}
	}
	return NULL;
}

// 2 restes : L_H, L_USER
LangType NppParameters::getLangIDFromStr(const TCHAR *langName)
{
	int lang = static_cast<int32_t>(L_TEXT);
	for (; lang < L_EXTERNAL; ++lang)
	{
		const TCHAR * name = ScintillaEditView::_langNameInfoArray[lang]._langName;
		if (!lstrcmp(name, langName)) //found lang?
		{
			return (LangType)lang;
		}
	}

	//Cannot find language, check if its an external one

	LangType l = (LangType)lang;
	if (l == L_EXTERNAL) //try find external lexer
	{
		int id = NppParameters::getInstance().getExternalLangIndexFromName(langName);
		if (id != -1) return (LangType)(id + L_EXTERNAL);
	}

	return L_TEXT;
}

generic_string NppParameters::getLocPathFromStr(const generic_string & localizationCode)
{
	if (localizationCode == TEXT("af"))
		return TEXT("afrikaans.xml");
	if (localizationCode == TEXT("sq"))
		return TEXT("albanian.xml");
	if (localizationCode == TEXT("ar") || localizationCode == TEXT("ar-dz") || localizationCode == TEXT("ar-bh") || localizationCode == TEXT("ar-eg") ||localizationCode == TEXT("ar-iq") || localizationCode == TEXT("ar-jo") || localizationCode == TEXT("ar-kw") || localizationCode == TEXT("ar-lb") || localizationCode == TEXT("ar-ly") || localizationCode == TEXT("ar-ma") || localizationCode == TEXT("ar-om") || localizationCode == TEXT("ar-qa") || localizationCode == TEXT("ar-sa") || localizationCode == TEXT("ar-sy") || localizationCode == TEXT("ar-tn") || localizationCode == TEXT("ar-ae") || localizationCode == TEXT("ar-ye"))
		return TEXT("arabic.xml");
	if (localizationCode == TEXT("an"))
		return TEXT("aragonese.xml");
	if (localizationCode == TEXT("az"))
		return TEXT("azerbaijani.xml");
	if (localizationCode == TEXT("eu"))
		return TEXT("basque.xml");
	if (localizationCode == TEXT("be"))
		return TEXT("belarusian.xml");
	if (localizationCode == TEXT("bn"))
		return TEXT("bengali.xml");
	if (localizationCode == TEXT("bs"))
		return TEXT("bosnian.xml");
	if (localizationCode == TEXT("pt-br"))
		return TEXT("brazilian_portuguese.xml");
	if (localizationCode == TEXT("br-fr"))
		return TEXT("breton.xml");
	if (localizationCode == TEXT("bg"))
		return TEXT("bulgarian.xml");
	if (localizationCode == TEXT("ca"))
		return TEXT("catalan.xml");
	if (localizationCode == TEXT("zh-tw") || localizationCode == TEXT("zh-hk") || localizationCode == TEXT("zh-sg"))
		return TEXT("taiwaneseMandarin.xml");
	if (localizationCode == TEXT("zh") || localizationCode == TEXT("zh-cn"))
		return TEXT("chineseSimplified.xml");
	if (localizationCode == TEXT("co") || localizationCode == TEXT("co-fr"))
		return TEXT("corsican.xml");
	if (localizationCode == TEXT("hr"))
		return TEXT("croatian.xml");
	if (localizationCode == TEXT("cs"))
		return TEXT("czech.xml");
	if (localizationCode == TEXT("da"))
		return TEXT("danish.xml");
	if (localizationCode == TEXT("nl") || localizationCode == TEXT("nl-be"))
		return TEXT("dutch.xml");
	if (localizationCode == TEXT("eo"))
		return TEXT("esperanto.xml");
	if (localizationCode == TEXT("et"))
		return TEXT("estonian.xml");
	if (localizationCode == TEXT("fa"))
		return TEXT("farsi.xml");
	if (localizationCode == TEXT("fi"))
		return TEXT("finnish.xml");
	if (localizationCode == TEXT("fr") || localizationCode == TEXT("fr-be") || localizationCode == TEXT("fr-ca") || localizationCode == TEXT("fr-fr") || localizationCode == TEXT("fr-lu") || localizationCode == TEXT("fr-mc") || localizationCode == TEXT("fr-ch"))
		return TEXT("french.xml");
	if (localizationCode == TEXT("fur"))
		return TEXT("friulian.xml");
	if (localizationCode == TEXT("gl"))
		return TEXT("galician.xml");
	if (localizationCode == TEXT("ka"))
		return TEXT("georgian.xml");
	if (localizationCode == TEXT("de") || localizationCode == TEXT("de-at") || localizationCode == TEXT("de-de") || localizationCode == TEXT("de-li") || localizationCode == TEXT("de-lu") || localizationCode == TEXT("de-ch"))
		return TEXT("german.xml");
	if (localizationCode == TEXT("el"))
		return TEXT("greek.xml");
	if (localizationCode == TEXT("gu"))
		return TEXT("gujarati.xml");
	if (localizationCode == TEXT("he"))
		return TEXT("hebrew.xml");
	if (localizationCode == TEXT("hi"))
		return TEXT("hindi.xml");
	if (localizationCode == TEXT("hu"))
		return TEXT("hungarian.xml");
	if (localizationCode == TEXT("id"))
		return TEXT("indonesian.xml");
	if (localizationCode == TEXT("it") || localizationCode == TEXT("it-ch"))
		return TEXT("italian.xml");
	if (localizationCode == TEXT("ja"))
		return TEXT("japanese.xml");
	if (localizationCode == TEXT("kn"))
		return TEXT("kannada.xml");
	if (localizationCode == TEXT("kk"))
		return TEXT("kazakh.xml");
	if (localizationCode == TEXT("ko") || localizationCode == TEXT("ko-kp") || localizationCode == TEXT("ko-kr"))
		return TEXT("korean.xml");
	if (localizationCode == TEXT("ku"))
		return TEXT("kurdish.xml");
	if (localizationCode == TEXT("ky"))
		return TEXT("kyrgyz.xml");
	if (localizationCode == TEXT("lv"))
		return TEXT("latvian.xml");
	if (localizationCode == TEXT("lt"))
		return TEXT("lithuanian.xml");
	if (localizationCode == TEXT("lb"))
		return TEXT("luxembourgish.xml");
	if (localizationCode == TEXT("mk"))
		return TEXT("macedonian.xml");
	if (localizationCode == TEXT("ms"))
		return TEXT("malay.xml");
	if (localizationCode == TEXT("mr"))
		return TEXT("marathi.xml");
	if (localizationCode == TEXT("mn"))
		return TEXT("mongolian.xml");
	if (localizationCode == TEXT("no") || localizationCode == TEXT("nb"))
		return TEXT("norwegian.xml");
	if (localizationCode == TEXT("nn"))
		return TEXT("nynorsk.xml");
	if (localizationCode == TEXT("oc"))
		return TEXT("occitan.xml");
	if (localizationCode == TEXT("pl"))
		return TEXT("polish.xml");
	if (localizationCode == TEXT("pt") || localizationCode == TEXT("pt-pt"))
		return TEXT("portuguese.xml");
	if (localizationCode == TEXT("pa") || localizationCode == TEXT("pa-in"))
		return TEXT("punjabi.xml");
	if (localizationCode == TEXT("ro") || localizationCode == TEXT("ro-mo"))
		return TEXT("romanian.xml");
	if (localizationCode == TEXT("ru") || localizationCode == TEXT("ru-mo"))
		return TEXT("russian.xml");
	if (localizationCode == TEXT("sc"))
		return TEXT("sardinian.xml");
	if (localizationCode == TEXT("sr"))
		return TEXT("serbian.xml");
	if (localizationCode == TEXT("sr-cyrl-ba") || localizationCode == TEXT("sr-cyrl-sp"))
		return TEXT("serbianCyrillic.xml");
	if (localizationCode == TEXT("si"))
		return TEXT("sinhala.xml");
	if (localizationCode == TEXT("sk"))
		return TEXT("slovak.xml");
	if (localizationCode == TEXT("sl"))
		return TEXT("slovenian.xml");
	if (localizationCode == TEXT("es") || localizationCode == TEXT("es-bo") || localizationCode == TEXT("es-cl") || localizationCode == TEXT("es-co") || localizationCode == TEXT("es-cr") || localizationCode == TEXT("es-do") || localizationCode == TEXT("es-ec") || localizationCode == TEXT("es-sv") || localizationCode == TEXT("es-gt") || localizationCode == TEXT("es-hn") || localizationCode == TEXT("es-mx") || localizationCode == TEXT("es-ni") || localizationCode == TEXT("es-pa") || localizationCode == TEXT("es-py") || localizationCode == TEXT("es-pe") || localizationCode == TEXT("es-pr") || localizationCode == TEXT("es-es") || localizationCode == TEXT("es-uy") || localizationCode == TEXT("es-ve"))
		return TEXT("spanish.xml");
	if (localizationCode == TEXT("es-ar"))
		return TEXT("spanish_ar.xml");
	if (localizationCode == TEXT("sv"))
		return TEXT("swedish.xml");
	if (localizationCode == TEXT("tl"))
		return TEXT("tagalog.xml");
	if (localizationCode == TEXT("tg-cyrl-tj"))
		return TEXT("tajikCyrillic.xml");
	if (localizationCode == TEXT("ta"))
		return TEXT("tamil.xml");
	if (localizationCode == TEXT("tt"))
		return TEXT("tatar.xml");
	if (localizationCode == TEXT("te"))
		return TEXT("telugu.xml");
	if (localizationCode == TEXT("th"))
		return TEXT("thai.xml");
	if (localizationCode == TEXT("tr"))
		return TEXT("turkish.xml");
	if (localizationCode == TEXT("uk"))
		return TEXT("ukrainian.xml");
	if (localizationCode == TEXT("ur") || localizationCode == TEXT("ur-pk"))
		return TEXT("urdu.xml");
	if (localizationCode == TEXT("ug-cn"))
		return TEXT("uyghur.xml");
	if (localizationCode == TEXT("uz"))
		return TEXT("uzbek.xml");
	if (localizationCode == TEXT("uz-cyrl-uz"))
		return TEXT("uzbekCyrillic.xml");
	if (localizationCode == TEXT("vec"))
		return TEXT("venetian.xml");
	if (localizationCode == TEXT("vi") || localizationCode == TEXT("vi-vn"))
		return TEXT("vietnamese.xml");
	if (localizationCode == TEXT("cy-gb"))
		return TEXT("welsh.xml");
	if (localizationCode == TEXT("zu") || localizationCode == TEXT("zu-za"))
		return TEXT("zulu.xml");
	if (localizationCode == TEXT("ne") || localizationCode == TEXT("nep"))
		return TEXT("nepali.xml");
	if (localizationCode == TEXT("oc-aranes"))
		return TEXT("aranese.xml");
	if (localizationCode == TEXT("exy"))
		return TEXT("extremaduran.xml");
	if (localizationCode == TEXT("keb"))
		return TEXT("kabyle.xml");
	if (localizationCode == TEXT("lij"))
		return TEXT("ligurian.xml");
	if (localizationCode == TEXT("ga"))
		return TEXT("irish.xml");
	if (localizationCode == TEXT("sgs"))
		return TEXT("samogitian.xml");
	if (localizationCode == TEXT("yue"))
		return TEXT("hongKongCantonese.xml");
	if (localizationCode == TEXT("ab") || localizationCode == TEXT("abk"))
		return TEXT("abkhazian.xml");

	return generic_string();
}


void NppParameters::feedKeyWordsParameters(TiXmlNode *node)
{
	TiXmlNode *langRoot = node->FirstChildElement(TEXT("Languages"));
	if (!langRoot)
		return;

	for (TiXmlNode *langNode = langRoot->FirstChildElement(TEXT("Language"));
		langNode ;
		langNode = langNode->NextSibling(TEXT("Language")) )
	{
		if (_nbLang < NB_LANG)
		{
			TiXmlElement* element = langNode->ToElement();
			const TCHAR* name = element->Attribute(TEXT("name"));
			if (name)
			{
				_langList[_nbLang] = new Lang(getLangIDFromStr(name), name);
				_langList[_nbLang]->setDefaultExtList(element->Attribute(TEXT("ext")));
				_langList[_nbLang]->setCommentLineSymbol(element->Attribute(TEXT("commentLine")));
				_langList[_nbLang]->setCommentStart(element->Attribute(TEXT("commentStart")));
				_langList[_nbLang]->setCommentEnd(element->Attribute(TEXT("commentEnd")));

				int tabSettings;
				if (element->Attribute(TEXT("tabSettings"), &tabSettings))
					_langList[_nbLang]->setTabInfo(tabSettings);

				for (TiXmlNode *kwNode = langNode->FirstChildElement(TEXT("Keywords"));
					kwNode ;
					kwNode = kwNode->NextSibling(TEXT("Keywords")) )
				{
					const TCHAR *indexName = (kwNode->ToElement())->Attribute(TEXT("name"));
					TiXmlNode *kwVal = kwNode->FirstChild();
					const TCHAR *keyWords = TEXT("");
					if ((indexName) && (kwVal))
						keyWords = kwVal->Value();

					int i = getKwClassFromName(indexName);

					if (i >= 0 && i <= KEYWORDSET_MAX)
						_langList[_nbLang]->setWords(keyWords, i);
				}
				++_nbLang;
			}
		}
	}
}

extern "C" {
typedef DWORD (WINAPI * EESFUNC) (LPCTSTR, LPTSTR, DWORD);
}

void NppParameters::feedGUIParameters(TiXmlNode *node)
{
	TiXmlNode *GUIRoot = node->FirstChildElement(TEXT("GUIConfigs"));
	if (nullptr == GUIRoot)
		return;

	for (TiXmlNode *childNode = GUIRoot->FirstChildElement(TEXT("GUIConfig"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("GUIConfig")) )
	{
		TiXmlElement* element = childNode->ToElement();
		const TCHAR* nm = element->Attribute(TEXT("name"));
		if (nullptr == nm)
			continue;

		if (!lstrcmp(nm, TEXT("ToolBar")))
		{
			const TCHAR* val = element->Attribute(TEXT("visible"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("no")))
					_nppGUI._toolbarShow = false;
				else// if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._toolbarShow = true;
			}
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, TEXT("small")))
						_nppGUI._toolBarStatus = TB_SMALL;
					else if (!lstrcmp(val, TEXT("large")))
						_nppGUI._toolBarStatus = TB_LARGE;
					else if (!lstrcmp(val, TEXT("small2")))
						_nppGUI._toolBarStatus = TB_SMALL2;
					else if (!lstrcmp(val, TEXT("large2")))
						_nppGUI._toolBarStatus = TB_LARGE2;
					else //if (!lstrcmp(val, TEXT("standard")))
						_nppGUI._toolBarStatus = TB_STANDARD;
				}
			}
		}
		else if (!lstrcmp(nm, TEXT("StatusBar")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, TEXT("hide")))
						_nppGUI._statusBarShow = false;
					else if (!lstrcmp(val, TEXT("show")))
						_nppGUI._statusBarShow = true;
				}
			}
		}
		else if (!lstrcmp(nm, TEXT("MenuBar")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, TEXT("hide")))
						_nppGUI._menuBarShow = false;
					else if (!lstrcmp(val, TEXT("show")))
						_nppGUI._menuBarShow = true;
				}
			}
		}
		else if (!lstrcmp(nm, TEXT("TabBar")))
		{
			bool isFailed = false;
			int oldValue = _nppGUI._tabStatus;
			const TCHAR* val = element->Attribute(TEXT("dragAndDrop"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus = TAB_DRAGNDROP;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus = 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("drawTopBar"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_DRAWTOPBAR;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("drawInactiveTab"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_DRAWINACTIVETAB;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("reduce"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_REDUCE;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("closeButton"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_CLOSEBUTTON;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("doubleClick2Close"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_DBCLK2CLOSE;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}
			val = element->Attribute(TEXT("vertical"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_VERTICAL;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("multiLine"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_MULTILINE;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("hide"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_HIDE;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("quitOnEmpty"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("yes")))
					_nppGUI._tabStatus |= TAB_QUITONEMPTY;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			val = element->Attribute(TEXT("iconSetNumber"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("1")))
					_nppGUI._tabStatus |= TAB_ALTICONS;
				else if (!lstrcmp(val, TEXT("0")))
					_nppGUI._tabStatus |= 0;
				else
					isFailed = true;
			}

			if (isFailed)
				_nppGUI._tabStatus = oldValue;
		}
		else if (!lstrcmp(nm, TEXT("Auto-detection")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, TEXT("yesOld")))
						_nppGUI._fileAutoDetection = cdEnabledOld;
					else if (!lstrcmp(val, TEXT("autoOld")))
						_nppGUI._fileAutoDetection = (cdEnabledOld | cdAutoUpdate);
					else if (!lstrcmp(val, TEXT("Update2EndOld")))
						_nppGUI._fileAutoDetection = (cdEnabledOld | cdGo2end);
					else if (!lstrcmp(val, TEXT("autoUpdate2EndOld")))
						_nppGUI._fileAutoDetection = (cdEnabledOld | cdAutoUpdate | cdGo2end);
					else if (!lstrcmp(val, TEXT("yes")))
						_nppGUI._fileAutoDetection = cdEnabledNew;
					else if (!lstrcmp(val, TEXT("auto")))
						_nppGUI._fileAutoDetection = (cdEnabledNew | cdAutoUpdate);
					else if (!lstrcmp(val, TEXT("Update2End")))
						_nppGUI._fileAutoDetection = (cdEnabledNew | cdGo2end);
					else if (!lstrcmp(val, TEXT("autoUpdate2End")))
						_nppGUI._fileAutoDetection = (cdEnabledNew | cdAutoUpdate | cdGo2end);
					else //(!lstrcmp(val, TEXT("no")))
						_nppGUI._fileAutoDetection = cdDisabled;
				}
			}
		}

		else if (!lstrcmp(nm, TEXT("TrayIcon")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					_nppGUI._isMinimizedToTray = (lstrcmp(val, TEXT("yes")) == 0);
				}
			}
		}
		else if (!lstrcmp(nm, TEXT("RememberLastSession")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._rememberLastSession = true;
					else
						_nppGUI._rememberLastSession = false;
				}
			}
		}
		else if (!lstrcmp(nm, TEXT("DetectEncoding")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._detectEncoding = true;
					else
						_nppGUI._detectEncoding = false;
				}
			}
		}
		else if (!lstrcmp(nm, TEXT("SaveAllConfirm")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._saveAllConfirm = true;
					else
						_nppGUI._saveAllConfirm = false;
				}
			}
		}
		else if (lstrcmp(nm, TEXT("MaitainIndent")) == 0)
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._maitainIndent = true;
					else
						_nppGUI._maitainIndent = false;
				}
			}
		}
		// <GUIConfig name="MarkAll" matchCase="yes" wholeWordOnly="yes" </GUIConfig>
		else if (!lstrcmp(nm, TEXT("MarkAll")))
		{
			const TCHAR* val = element->Attribute(TEXT("matchCase"));
			if (val)
			{
				if (lstrcmp(val, TEXT("yes")) == 0)
					_nppGUI._markAllCaseSensitive = true;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._markAllCaseSensitive = false;
			}

			val = element->Attribute(TEXT("wholeWordOnly"));
			if (val)
			{
				if (lstrcmp(val, TEXT("yes")) == 0)
					_nppGUI._markAllWordOnly = true;
				else if (!lstrcmp(val, TEXT("no")))
					_nppGUI._markAllWordOnly = false;
			}
		}
		// <GUIConfig name="SmartHighLight" matchCase="yes" wholeWordOnly="yes" useFindSettings="no">yes</GUIConfig>
		else if (!lstrcmp(nm, TEXT("SmartHighLight")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._enableSmartHilite = true;
					else
						_nppGUI._enableSmartHilite = false;
				}

				val = element->Attribute(TEXT("matchCase"));
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._smartHiliteCaseSensitive = true;
					else if (!lstrcmp(val, TEXT("no")))
						_nppGUI._smartHiliteCaseSensitive = false;
				}

				val = element->Attribute(TEXT("wholeWordOnly"));
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._smartHiliteWordOnly = true;
					else if (!lstrcmp(val, TEXT("no")))
						_nppGUI._smartHiliteWordOnly = false;
				}

				val = element->Attribute(TEXT("useFindSettings"));
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._smartHiliteUseFindSettings = true;
					else if (!lstrcmp(val, TEXT("no")))
						_nppGUI._smartHiliteUseFindSettings = false;
				}

				val = element->Attribute(TEXT("onAnotherView"));
				if (val)
				{
					if (lstrcmp(val, TEXT("yes")) == 0)
						_nppGUI._smartHiliteOnAnotherView = true;
					else if (!lstrcmp(val, TEXT("no")))
						_nppGUI._smartHiliteOnAnotherView = false;
				}
			}
		}

		else if (!lstrcmp(nm, TEXT("TagsMatchHighLight")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					_nppGUI._enableTagsMatchHilite = !lstrcmp(val, TEXT("yes"));
					const TCHAR *tahl = element->Attribute(TEXT("TagAttrHighLight"));
					if (tahl)
						_nppGUI._enableTagAttrsHilite = !lstrcmp(tahl, TEXT("yes"));

					tahl = element->Attribute(TEXT("HighLightNonHtmlZone"));
					if (tahl)
						_nppGUI._enableHiliteNonHTMLZone = !lstrcmp(tahl, TEXT("yes"));
				}
			}
		}

		else if (!lstrcmp(nm, TEXT("TaskList")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					_nppGUI._doTaskList = (!lstrcmp(val, TEXT("yes")))?true:false;
				}
			}
		}

		else if (!lstrcmp(nm, TEXT("MRU")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
					_nppGUI._styleMRU = (!lstrcmp(val, TEXT("yes")));
			}
		}

		else if (!lstrcmp(nm, TEXT("URL")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					int const i = generic_atoi (val);
					if ((i >= urlMin) && (i <= urlMax))
						_nppGUI._styleURL = urlMode(i);
				}
			}
		}

		else if (!lstrcmp(nm, TEXT("uriCustomizedSchemes")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				_nppGUI._uriSchemes = val;
			}
		}

		else if (!lstrcmp(nm, TEXT("CheckHistoryFiles")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, TEXT("no")))
						_nppGUI._checkHistoryFiles = false;
					else if (!lstrcmp(val, TEXT("yes")))
						_nppGUI._checkHistoryFiles = true;
				}
			}
		}

		else if (!lstrcmp(nm, TEXT("ScintillaViewsSplitter")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, TEXT("vertical")))
						_nppGUI._splitterPos = POS_VERTICAL;
					else if (!lstrcmp(val, TEXT("horizontal")))
						_nppGUI._splitterPos = POS_HORIZOTAL;
				}
			}
		}

		else if (!lstrcmp(nm, TEXT("UserDefineDlg")))
		{
			bool isFailed = false;
			int oldValue = _nppGUI._userDefineDlgStatus;

			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
				{
					if (!lstrcmp(val, TEXT("hide")))
						_nppGUI._userDefineDlgStatus = 0;
					else if (!lstrcmp(val, TEXT("show")))
						_nppGUI._userDefineDlgStatus = UDD_SHOW;
					else
						isFailed = true;
				}
			}

			const TCHAR* val = element->Attribute(TEXT("position"));
			if (val)
			{
				if (!lstrcmp(val, TEXT("docked")))
					_nppGUI._userDefineDlgStatus |= UDD_DOCKED;
				else if (!lstrcmp(val, TEXT("undocked")))
					_nppGUI._userDefineDlgStatus |= 0;
				else
					isFailed = true;
			}
			if (isFailed)
				_nppGUI._userDefineDlgStatus = oldValue;
		}

		else if (!lstrcmp(nm, TEXT("TabSetting")))
		{
			int i;
			const TCHAR* val = element->Attribute(TEXT("size"), &i);
			if (val)
				_nppGUI._tabSize = i;

			if ((_nppGUI._tabSize == -1) || (_nppGUI._tabSize == 0))
				_nppGUI._tabSize = 4;

			val = element->Attribute(TEXT("replaceBySpace"));
			if (val)
				_nppGUI._tabReplacedBySpace = (!lstrcmp(val, TEXT("yes")));
		}

		else if (!lstrcmp(nm, TEXT("Caret")))
		{
			int i;
			const TCHAR* val = element->Attribute(TEXT("width"), &i);
			if (val)
				_nppGUI._caretWidth = i;

			val = element->Attribute(TEXT("blinkRate"), &i);
			if (val)
				_nppGUI._caretBlinkRate = i;
		}

		else if (!lstrcmp(nm, TEXT("ScintillaGlobalSettings")))
		{
			const TCHAR* val = element->Attribute(TEXT("enableMultiSelection"));
			if (val)
			{
				if (lstrcmp(val, TEXT("yes")) == 0)
					_nppGUI._enableMultiSelection = true;
				else if (lstrcmp(val, TEXT("no")) == 0)
					_nppGUI._enableMultiSelection = false;
			}
		}

		else if (!lstrcmp(nm, TEXT("AppPosition")))
		{
			RECT oldRect = _nppGUI._appPos;
			bool fuckUp = true;
			int i;

			if (element->Attribute(TEXT("x"), &i))
			{
				_nppGUI._appPos.left = i;

				if (element->Attribute(TEXT("y"), &i))
				{
					_nppGUI._appPos.top = i;

					if (element->Attribute(TEXT("width"), &i))
					{
						_nppGUI._appPos.right = i;

						if (element->Attribute(TEXT("height"), &i))
						{
							_nppGUI._appPos.bottom = i;
							fuckUp = false;
						}
					}
				}
			}
			if (fuckUp)
				_nppGUI._appPos = oldRect;

			const TCHAR* val = element->Attribute(TEXT("isMaximized"));
			if (val)
				_nppGUI._isMaximized = (lstrcmp(val, TEXT("yes")) == 0);
		}

		else if (!lstrcmp(nm, TEXT("FindWindowPosition")))
		{
			RECT oldRect = _nppGUI._findWindowPos;
			bool incomplete = true;
			int i;

			if (element->Attribute(TEXT("left"), &i))
			{
				_nppGUI._findWindowPos.left = i;

				if (element->Attribute(TEXT("top"), &i))
				{
					_nppGUI._findWindowPos.top = i;

					if (element->Attribute(TEXT("right"), &i))
					{
						_nppGUI._findWindowPos.right = i;

						if (element->Attribute(TEXT("bottom"), &i))
						{
							_nppGUI._findWindowPos.bottom = i;
							incomplete = false;
						}
					}
				}
			}
			if (incomplete)
			{
				_nppGUI._findWindowPos = oldRect;
			}

			const TCHAR* val = element->Attribute(TEXT("isLessModeOn"));
			if (val)
				_nppGUI._findWindowLessMode = (lstrcmp(val, TEXT("yes")) == 0);
		}

		else if (!lstrcmp(nm, TEXT("FinderConfig")))
		{
			const TCHAR* val = element->Attribute(TEXT("wrappedLines"));
			if (val)
			{
				_nppGUI._finderLinesAreCurrentlyWrapped = (!lstrcmp(val, TEXT("yes")));
			}

			val = element->Attribute(TEXT("purgeBeforeEverySearch"));
			if (val)
			{
				_nppGUI._finderPurgeBeforeEverySearch = (!lstrcmp(val, TEXT("yes")));
			}

			val = element->Attribute(TEXT("showOnlyOneEntryPerFoundLine"));
			if (val)
			{
				_nppGUI._finderShowOnlyOneEntryPerFoundLine = (!lstrcmp(val, TEXT("yes")));
			}
		}

		else if (!lstrcmp(nm, TEXT("NewDocDefaultSettings")))
		{
			int i;
			if (element->Attribute(TEXT("format"), &i))
			{
				EolType newFormat = EolType::osdefault;
				switch (i)
				{
					case static_cast<LPARAM>(EolType::windows) :
						newFormat = EolType::windows;
						break;
					case static_cast<LPARAM>(EolType::macos) :
						newFormat = EolType::macos;
						break;
					case static_cast<LPARAM>(EolType::unix) :
						newFormat = EolType::unix;
						break;
					default:
						assert(false and "invalid buffer format - fallback to default");
				}
				_nppGUI._newDocDefaultSettings._format = newFormat;
			}

			if (element->Attribute(TEXT("encoding"), &i))
				_nppGUI._newDocDefaultSettings._unicodeMode = (UniMode)i;

			if (element->Attribute(TEXT("lang"), &i))
				_nppGUI._newDocDefaultSettings._lang = (LangType)i;

			if (element->Attribute(TEXT("codepage"), &i))
				_nppGUI._newDocDefaultSettings._codepage = (LangType)i;

			const TCHAR* val = element->Attribute(TEXT("openAnsiAsUTF8"));
			if (val)
				_nppGUI._newDocDefaultSettings._openAnsiAsUtf8 = (lstrcmp(val, TEXT("yes")) == 0);

		}

		else if (!lstrcmp(nm, TEXT("langsExcluded")))
		{
			// TODO
			int g0 = 0; // up to 8
			int g1 = 0; // up to 16
			int g2 = 0; // up to 24
			int g3 = 0; // up to 32
			int g4 = 0; // up to 40
			int g5 = 0; // up to 48
			int g6 = 0; // up to 56
			int g7 = 0; // up to 64
			int g8 = 0; // up to 72
			int g9 = 0; // up to 80
			int g10= 0; // up to 88
			int g11= 0; // up to 96
			int g12= 0; // up to 104

			// TODO some refactoring needed here....
			{
				int i;
				if (element->Attribute(TEXT("gr0"), &i))
				{
					if (i <= 255)
						g0 = i;
				}
				if (element->Attribute(TEXT("gr1"), &i))
				{
					if (i <= 255)
						g1 = i;
				}
				if (element->Attribute(TEXT("gr2"), &i))
				{
					if (i <= 255)
						g2 = i;
				}
				if (element->Attribute(TEXT("gr3"), &i))
				{
					if (i <= 255)
						g3 = i;
				}
				if (element->Attribute(TEXT("gr4"), &i))
				{
					if (i <= 255)
						g4 = i;
				}
				if (element->Attribute(TEXT("gr5"), &i))
				{
					if (i <= 255)
						g5 = i;
				}
				if (element->Attribute(TEXT("gr6"), &i))
				{
					if (i <= 255)
						g6 = i;
				}
				if (element->Attribute(TEXT("gr7"), &i))
				{
					if (i <= 255)
						g7 = i;
				}
				if (element->Attribute(TEXT("gr8"), &i))
				{
					if (i <= 255)
						g8 = i;
				}
				if (element->Attribute(TEXT("gr9"), &i))
				{
					if (i <= 255)
						g9 = i;
				}
				if (element->Attribute(TEXT("gr10"), &i))
				{
					if (i <= 255)
						g10 = i;
				}
				if (element->Attribute(TEXT("gr11"), &i))
				{
					if (i <= 255)
						g11 = i;
				}
				if (element->Attribute(TEXT("gr12"), &i))
				{
					if (i <= 255)
						g12 = i;
				}
			}

			UCHAR mask = 1;
			for (int i = 0 ; i < 8 ; ++i)
			{
				if (mask & g0)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 8 ; i < 16 ; ++i)
			{
				if (mask & g1)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 16 ; i < 24 ; ++i)
			{
				if (mask & g2)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 24 ; i < 32 ; ++i)
			{
				if (mask & g3)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 32 ; i < 40 ; ++i)
			{
				if (mask & g4)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 40 ; i < 48 ; ++i)
			{
				if (mask & g5)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 48 ; i < 56 ; ++i)
			{
				if (mask & g6)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 56 ; i < 64 ; ++i)
			{
				if (mask & g7)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 64; i < 72; ++i)
			{
				if (mask & g8)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 72; i < 80; ++i)
			{
				if (mask & g9)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 80; i < 88; ++i)
			{
				if (mask & g10)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 88; i < 96; ++i)
			{
				if (mask & g11)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			mask = 1;
			for (int i = 96; i < 104; ++i)
			{
				if (mask & g12)
					_nppGUI._excludedLangList.push_back(LangMenuItem((LangType)i));
				mask <<= 1;
			}

			const TCHAR* val = element->Attribute(TEXT("langMenuCompact"));
			if (val)
				_nppGUI._isLangMenuCompact = (!lstrcmp(val, TEXT("yes")));
		}

		else if (!lstrcmp(nm, TEXT("Print")))
		{
			const TCHAR* val = element->Attribute(TEXT("lineNumber"));
			if (val)
				_nppGUI._printSettings._printLineNumber = (!lstrcmp(val, TEXT("yes")));

			int i;
			if (element->Attribute(TEXT("printOption"), &i))
				_nppGUI._printSettings._printOption = i;

			val = element->Attribute(TEXT("headerLeft"));
			if (val)
				_nppGUI._printSettings._headerLeft = val;

			val = element->Attribute(TEXT("headerMiddle"));
			if (val)
				_nppGUI._printSettings._headerMiddle = val;

			val = element->Attribute(TEXT("headerRight"));
			if (val)
				_nppGUI._printSettings._headerRight = val;


			val = element->Attribute(TEXT("footerLeft"));
			if (val)
				_nppGUI._printSettings._footerLeft = val;

			val = element->Attribute(TEXT("footerMiddle"));
			if (val)
				_nppGUI._printSettings._footerMiddle = val;

			val = element->Attribute(TEXT("footerRight"));
			if (val)
				_nppGUI._printSettings._footerRight = val;


			val = element->Attribute(TEXT("headerFontName"));
			if (val)
				_nppGUI._printSettings._headerFontName = val;

			val = element->Attribute(TEXT("footerFontName"));
			if (val)
				_nppGUI._printSettings._footerFontName = val;

			if (element->Attribute(TEXT("headerFontStyle"), &i))
				_nppGUI._printSettings._headerFontStyle = i;

			if (element->Attribute(TEXT("footerFontStyle"), &i))
				_nppGUI._printSettings._footerFontStyle = i;

			if (element->Attribute(TEXT("headerFontSize"), &i))
				_nppGUI._printSettings._headerFontSize = i;

			if (element->Attribute(TEXT("footerFontSize"), &i))
				_nppGUI._printSettings._footerFontSize = i;


			if (element->Attribute(TEXT("margeLeft"), &i))
				_nppGUI._printSettings._marge.left = i;

			if (element->Attribute(TEXT("margeTop"), &i))
				_nppGUI._printSettings._marge.top = i;

			if (element->Attribute(TEXT("margeRight"), &i))
				_nppGUI._printSettings._marge.right = i;

			if (element->Attribute(TEXT("margeBottom"), &i))
				_nppGUI._printSettings._marge.bottom = i;
		}

		else if (!lstrcmp(nm, TEXT("ScintillaPrimaryView")))
		{
			feedScintillaParam(element);
		}

		else if (!lstrcmp(nm, TEXT("Backup")))
		{
			int i;
			if (element->Attribute(TEXT("action"), &i))
				_nppGUI._backup = (BackupFeature)i;

			const TCHAR *bDir = element->Attribute(TEXT("useCustumDir"));
			if (bDir)
			{
				_nppGUI._useDir = (lstrcmp(bDir, TEXT("yes")) == 0);
			}
			const TCHAR *pDir = element->Attribute(TEXT("dir"));
			if (pDir)
				_nppGUI._backupDir = pDir;

			const TCHAR *isSnapshotModeStr = element->Attribute(TEXT("isSnapshotMode"));
			if (isSnapshotModeStr && !lstrcmp(isSnapshotModeStr, TEXT("no")))
				_nppGUI._isSnapshotMode = false;

			int timing;
			if (element->Attribute(TEXT("snapshotBackupTiming"), &timing))
				_nppGUI._snapshotBackupTiming = timing;

		}
		else if (!lstrcmp(nm, TEXT("DockingManager")))
		{
			feedDockingManager(element);
		}

		else if (!lstrcmp(nm, TEXT("globalOverride")))
		{
			const TCHAR *bDir = element->Attribute(TEXT("fg"));
			if (bDir)
				_nppGUI._globalOverride.enableFg = (lstrcmp(bDir, TEXT("yes")) == 0);

			bDir = element->Attribute(TEXT("bg"));
			if (bDir)
				_nppGUI._globalOverride.enableBg = (lstrcmp(bDir, TEXT("yes")) == 0);

			bDir = element->Attribute(TEXT("font"));
			if (bDir)
				_nppGUI._globalOverride.enableFont = (lstrcmp(bDir, TEXT("yes")) == 0);

			bDir = element->Attribute(TEXT("fontSize"));
			if (bDir)
				_nppGUI._globalOverride.enableFontSize = (lstrcmp(bDir, TEXT("yes")) == 0);

			bDir = element->Attribute(TEXT("bold"));
			if (bDir)
				_nppGUI._globalOverride.enableBold = (lstrcmp(bDir, TEXT("yes")) == 0);

			bDir = element->Attribute(TEXT("italic"));
			if (bDir)
				_nppGUI._globalOverride.enableItalic = (lstrcmp(bDir, TEXT("yes")) == 0);

			bDir = element->Attribute(TEXT("underline"));
			if (bDir)
				_nppGUI._globalOverride.enableUnderLine = (lstrcmp(bDir, TEXT("yes")) == 0);
		}
		else if (!lstrcmp(nm, TEXT("auto-completion")))
		{
			int i;
			if (element->Attribute(TEXT("autoCAction"), &i))
				_nppGUI._autocStatus = static_cast<NppGUI::AutocStatus>(i);

			if (element->Attribute(TEXT("triggerFromNbChar"), &i))
				_nppGUI._autocFromLen = i;

			const TCHAR * optName = element->Attribute(TEXT("autoCIgnoreNumbers"));
			if (optName)
				_nppGUI._autocIgnoreNumbers = (lstrcmp(optName, TEXT("yes")) == 0);

			optName = element->Attribute(TEXT("insertSelectedItemUseENTER"));
			if (optName)
				_nppGUI._autocInsertSelectedUseENTER = (lstrcmp(optName, TEXT("yes")) == 0);

			optName = element->Attribute(TEXT("insertSelectedItemUseTAB"));
			if (optName)
				_nppGUI._autocInsertSelectedUseTAB = (lstrcmp(optName, TEXT("yes")) == 0);


			optName = element->Attribute(TEXT("funcParams"));
			if (optName)
				_nppGUI._funcParams = (lstrcmp(optName, TEXT("yes")) == 0);
		}
		else if (!lstrcmp(nm, TEXT("auto-insert")))
		{
			const TCHAR * optName = element->Attribute(TEXT("htmlXmlTag"));
			if (optName)
				_nppGUI._matchedPairConf._doHtmlXmlTag = (lstrcmp(optName, TEXT("yes")) == 0);

			optName = element->Attribute(TEXT("parentheses"));
			if (optName)
				_nppGUI._matchedPairConf._doParentheses = (lstrcmp(optName, TEXT("yes")) == 0);

			optName = element->Attribute(TEXT("brackets"));
			if (optName)
				_nppGUI._matchedPairConf._doBrackets = (lstrcmp(optName, TEXT("yes")) == 0);

			optName = element->Attribute(TEXT("curlyBrackets"));
			if (optName)
				_nppGUI._matchedPairConf._doCurlyBrackets = (lstrcmp(optName, TEXT("yes")) == 0);

			optName = element->Attribute(TEXT("quotes"));
			if (optName)
				_nppGUI._matchedPairConf._doQuotes = (lstrcmp(optName, TEXT("yes")) == 0);

			optName = element->Attribute(TEXT("doubleQuotes"));
			if (optName)
				_nppGUI._matchedPairConf._doDoubleQuotes = (lstrcmp(optName, TEXT("yes")) == 0);

			for (TiXmlNode *subChildNode = childNode->FirstChildElement(TEXT("UserDefinePair"));
				 subChildNode;
				 subChildNode = subChildNode->NextSibling(TEXT("UserDefinePair")) )
			{
				int open = -1;
				int openVal = 0;
				const TCHAR *openValStr = (subChildNode->ToElement())->Attribute(TEXT("open"), &openVal);
				if (openValStr && (openVal >= 0 && openVal < 128))
					open = openVal;

				int close = -1;
				int closeVal = 0;
				const TCHAR *closeValStr = (subChildNode->ToElement())->Attribute(TEXT("close"), &closeVal);
				if (closeValStr && (closeVal >= 0 && closeVal <= 128))
					close = closeVal;

				if (open != -1 && close != -1)
					_nppGUI._matchedPairConf._matchedPairsInit.push_back(pair<char, char>(char(open), char(close)));
			}
		}

		else if (!lstrcmp(nm, TEXT("sessionExt")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
					_nppGUI._definedSessionExt = val;
			}
		}

		else if (!lstrcmp(nm, TEXT("workspaceExt")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
					_nppGUI._definedWorkspaceExt = val;
			}
		}

		else if (!lstrcmp(nm, TEXT("noUpdate")))
		{
			TiXmlNode *n = childNode->FirstChild();
			if (n)
			{
				const TCHAR* val = n->Value();
				if (val)
					_nppGUI._autoUpdateOpt._doAutoUpdate = (!lstrcmp(val, TEXT("yes")))?false:true;

				int i;
				val = element->Attribute(TEXT("intervalDays"), &i);
				if (val)
					_nppGUI._autoUpdateOpt._intervalDays = i;

				val = element->Attribute(TEXT("nextUpdateDate"));
				if (val)
					_nppGUI._autoUpdateOpt._nextUpdateDate = Date(val);
			}
		}

		else if (!lstrcmp(nm, TEXT("openSaveDir")))
		{
			const TCHAR * value = element->Attribute(TEXT("value"));
			if (value && value[0])
			{
				if (lstrcmp(value, TEXT("1")) == 0)
					_nppGUI._openSaveDir = dir_last;
				else if (lstrcmp(value, TEXT("2")) == 0)
					_nppGUI._openSaveDir = dir_userDef;
				else
					_nppGUI._openSaveDir = dir_followCurrent;
			}

			const TCHAR * path = element->Attribute(TEXT("defaultDirPath"));
			if (path && path[0])
			{
				lstrcpyn(_nppGUI._defaultDir, path, MAX_PATH);
				::ExpandEnvironmentStrings(_nppGUI._defaultDir, _nppGUI._defaultDirExp, MAX_PATH);
			}
 		}

		else if (!lstrcmp(nm, TEXT("titleBar")))
		{
			const TCHAR * value = element->Attribute(TEXT("short"));
			_nppGUI._shortTitlebar = false;	//default state
			if (value && value[0])
			{
				if (lstrcmp(value, TEXT("yes")) == 0)
					_nppGUI._shortTitlebar = true;
				else if (lstrcmp(value, TEXT("no")) == 0)
					_nppGUI._shortTitlebar = false;
			}
		}

		else if (!lstrcmp(nm, TEXT("insertDateTime")))
		{
			const TCHAR* customFormat = element->Attribute(TEXT("customizedFormat"));
			if (customFormat != NULL && customFormat[0])
				_nppGUI._dateTimeFormat = customFormat;

			const TCHAR* value = element->Attribute(TEXT("reverseDefaultOrder"));
			if (value && value[0])
			{
				if (lstrcmp(value, TEXT("yes")) == 0)
					_nppGUI._dateTimeReverseDefaultOrder = true;
				else if (lstrcmp(value, TEXT("no")) == 0)
					_nppGUI._dateTimeReverseDefaultOrder = false;
			}
		}

		else if (!lstrcmp(nm, TEXT("wordCharList")))
		{
			const TCHAR * value = element->Attribute(TEXT("useDefault"));
			if (value && value[0])
			{
				if (lstrcmp(value, TEXT("yes")) == 0)
					_nppGUI._isWordCharDefault = true;
				else if (lstrcmp(value, TEXT("no")) == 0)
					_nppGUI._isWordCharDefault = false;
			}

			const TCHAR *charsAddedW = element->Attribute(TEXT("charsAdded"));
			if (charsAddedW)
			{
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
				_nppGUI._customWordChars = wmc.wchar2char(charsAddedW, SC_CP_UTF8);
			}
		}
		else if (!lstrcmp(nm, TEXT("delimiterSelection")))
		{
			int leftmost = 0;
			element->Attribute(TEXT("leftmostDelimiter"), &leftmost);
			if (leftmost > 0 && leftmost < 256)
				_nppGUI._leftmostDelimiter = static_cast<char>(leftmost);

			int rightmost = 0;
			element->Attribute(TEXT("rightmostDelimiter"), &rightmost);
			if (rightmost > 0 && rightmost < 256)
				_nppGUI._rightmostDelimiter = static_cast<char>(rightmost);

			const TCHAR *delimiterSelectionOnEntireDocument = element->Attribute(TEXT("delimiterSelectionOnEntireDocument"));
			if (delimiterSelectionOnEntireDocument != NULL && !lstrcmp(delimiterSelectionOnEntireDocument, TEXT("yes")))
				_nppGUI._delimiterSelectionOnEntireDocument = true;
			else
				_nppGUI._delimiterSelectionOnEntireDocument = false;
		}
		else if (!lstrcmp(nm, TEXT("largeFileRestriction")))
		{
			int fileSizeLimit4StylingMB = 0;
			element->Attribute(TEXT("fileSizeMB"), &fileSizeLimit4StylingMB);
			if (fileSizeLimit4StylingMB > 0 && fileSizeLimit4StylingMB < 4096)
				_nppGUI._largeFileRestriction._largeFileSizeDefInByte = (fileSizeLimit4StylingMB * 1024 * 1024);

			const TCHAR* boolVal = element->Attribute(TEXT("isEnabled"));
			if (boolVal != NULL && !lstrcmp(boolVal, TEXT("no")))
				_nppGUI._largeFileRestriction._isEnabled = false;
			else
				_nppGUI._largeFileRestriction._isEnabled = true;

			boolVal = element->Attribute(TEXT("allowAutoCompletion"));
			if (boolVal != NULL && !lstrcmp(boolVal, TEXT("yes")))
				_nppGUI._largeFileRestriction._allowAutoCompletion = true;
			else
				_nppGUI._largeFileRestriction._allowAutoCompletion = false;

			boolVal = element->Attribute(TEXT("allowBraceMatch"));
			if (boolVal != NULL && !lstrcmp(boolVal, TEXT("yes")))
				_nppGUI._largeFileRestriction._allowBraceMatch = true;
			else
				_nppGUI._largeFileRestriction._allowBraceMatch = false;

			boolVal = element->Attribute(TEXT("allowSmartHilite"));
			if (boolVal != NULL && !lstrcmp(boolVal, TEXT("yes")))
				_nppGUI._largeFileRestriction._allowSmartHilite = true;
			else
				_nppGUI._largeFileRestriction._allowSmartHilite = false;

			boolVal = element->Attribute(TEXT("allowClickableLink"));
			if (boolVal != NULL && !lstrcmp(boolVal, TEXT("yes")))
				_nppGUI._largeFileRestriction._allowClickableLink = true;
			else
				_nppGUI._largeFileRestriction._allowClickableLink = false;

			boolVal = element->Attribute(TEXT("deactivateWordWrap"));
			if (boolVal != NULL && !lstrcmp(boolVal, TEXT("no")))
				_nppGUI._largeFileRestriction._deactivateWordWrap = false;
			else
				_nppGUI._largeFileRestriction._deactivateWordWrap = true;
		}
		else if (!lstrcmp(nm, TEXT("multiInst")))
		{
			int val = 0;
			element->Attribute(TEXT("setting"), &val);
			if (val < 0 || val > 2)
				val = 0;
			_nppGUI._multiInstSetting = (MultiInstSetting)val;
		}
		else if (!lstrcmp(nm, TEXT("searchEngine")))
		{
			int i;
			if (element->Attribute(TEXT("searchEngineChoice"), &i))
				_nppGUI._searchEngineChoice = static_cast<NppGUI::SearchEngineChoice>(i);

			const TCHAR * searchEngineCustom = element->Attribute(TEXT("searchEngineCustom"));
			if (searchEngineCustom && searchEngineCustom[0])
				_nppGUI._searchEngineCustom = searchEngineCustom;
		}
		else if (!lstrcmp(nm, TEXT("Searching")))
		{
			const TCHAR* optNameMonoFont = element->Attribute(TEXT("monospacedFontFindDlg"));
			if (optNameMonoFont)
				_nppGUI._monospacedFontFindDlg = (lstrcmp(optNameMonoFont, TEXT("yes")) == 0);

			//This is an option from previous versions of notepad++.  It is handled for compatibility with older settings.
			const TCHAR* optStopFillingFindField = element->Attribute(TEXT("stopFillingFindField"));
			if (optStopFillingFindField) 
			{
				_nppGUI._fillFindFieldWithSelected = (lstrcmp(optStopFillingFindField, TEXT("no")) == 0);
				_nppGUI._fillFindFieldSelectCaret = _nppGUI._fillFindFieldWithSelected;
			}

			const TCHAR* optFillFindFieldWithSelected = element->Attribute(TEXT("fillFindFieldWithSelected"));
			if (optFillFindFieldWithSelected)
				_nppGUI._fillFindFieldWithSelected = (lstrcmp(optFillFindFieldWithSelected, TEXT("yes")) == 0);

			const TCHAR* optFillFindFieldSelectCaret = element->Attribute(TEXT("fillFindFieldSelectCaret"));
			if (optFillFindFieldSelectCaret)
				_nppGUI._fillFindFieldSelectCaret = (lstrcmp(optFillFindFieldSelectCaret, TEXT("yes")) == 0);

			const TCHAR* optFindDlgAlwaysVisible = element->Attribute(TEXT("findDlgAlwaysVisible"));
			if (optFindDlgAlwaysVisible)
				_nppGUI._findDlgAlwaysVisible = (lstrcmp(optFindDlgAlwaysVisible, TEXT("yes")) == 0);

			const TCHAR* optConfirmReplaceOpenDocs = element->Attribute(TEXT("confirmReplaceInAllOpenDocs"));
			if (optConfirmReplaceOpenDocs)
				_nppGUI._confirmReplaceInAllOpenDocs = (lstrcmp(optConfirmReplaceOpenDocs, TEXT("yes")) == 0);

			const TCHAR* optReplaceStopsWithoutFindingNext = element->Attribute(TEXT("replaceStopsWithoutFindingNext"));
			if (optReplaceStopsWithoutFindingNext)
				_nppGUI._replaceStopsWithoutFindingNext = (lstrcmp(optReplaceStopsWithoutFindingNext, TEXT("yes")) == 0);
		}
		else if (!lstrcmp(nm, TEXT("MISC")))
		{
			const TCHAR * optName = element->Attribute(TEXT("fileSwitcherWithoutExtColumn"));
			if (optName)
				_nppGUI._fileSwitcherWithoutExtColumn = (lstrcmp(optName, TEXT("yes")) == 0);
			
			int i = 0;
			if (element->Attribute(TEXT("fileSwitcherExtWidth"), &i))
				_nppGUI._fileSwitcherExtWidth = i;

			const TCHAR * optNamePath = element->Attribute(TEXT("fileSwitcherWithoutPathColumn"));
			if (optNamePath)
				_nppGUI._fileSwitcherWithoutPathColumn = (lstrcmp(optNamePath, TEXT("yes")) == 0);

			if (element->Attribute(TEXT("fileSwitcherPathWidth"), &i))
				_nppGUI._fileSwitcherPathWidth = i;

			const TCHAR * optNameBackSlashEscape = element->Attribute(TEXT("backSlashIsEscapeCharacterForSql"));
			if (optNameBackSlashEscape && !lstrcmp(optNameBackSlashEscape, TEXT("no")))
				_nppGUI._backSlashIsEscapeCharacterForSql = false;

			const TCHAR * optNameWriteTechnologyEngine = element->Attribute(TEXT("writeTechnologyEngine"));
			if (optNameWriteTechnologyEngine)
				_nppGUI._writeTechnologyEngine = (lstrcmp(optNameWriteTechnologyEngine, TEXT("1")) == 0) ? directWriteTechnology : defaultTechnology;

			const TCHAR * optNameFolderDroppedOpenFiles = element->Attribute(TEXT("isFolderDroppedOpenFiles"));
			if (optNameFolderDroppedOpenFiles)
				_nppGUI._isFolderDroppedOpenFiles = (lstrcmp(optNameFolderDroppedOpenFiles, TEXT("yes")) == 0);

			const TCHAR * optDocPeekOnTab = element->Attribute(TEXT("docPeekOnTab"));
			if (optDocPeekOnTab)
				_nppGUI._isDocPeekOnTab = (lstrcmp(optDocPeekOnTab, TEXT("yes")) == 0);

			const TCHAR * optDocPeekOnMap = element->Attribute(TEXT("docPeekOnMap"));
			if (optDocPeekOnMap)
				_nppGUI._isDocPeekOnMap = (lstrcmp(optDocPeekOnMap, TEXT("yes")) == 0);

			const TCHAR* optSortFunctionList = element->Attribute(TEXT("sortFunctionList"));
			if (optSortFunctionList)
				_nppGUI._shouldSortFunctionList = (lstrcmp(optSortFunctionList, TEXT("yes")) == 0);

			const TCHAR* saveDlgExtFilterToAllTypes = element->Attribute(TEXT("saveDlgExtFilterToAllTypes"));
			if (saveDlgExtFilterToAllTypes)
				_nppGUI._setSaveDlgExtFiltToAllTypes = (lstrcmp(saveDlgExtFilterToAllTypes, TEXT("yes")) == 0);

			const TCHAR * optMuteSounds = element->Attribute(TEXT("muteSounds"));
			if (optMuteSounds)
				_nppGUI._muteSounds = lstrcmp(optMuteSounds, TEXT("yes")) == 0;

			const TCHAR * optEnableFoldCmdToggable = element->Attribute(TEXT("enableFoldCmdToggable"));
			if (optEnableFoldCmdToggable)
				_nppGUI._enableFoldCmdToggable = lstrcmp(optEnableFoldCmdToggable, TEXT("yes")) == 0;

			const TCHAR * hideMenuRightShortcuts = element->Attribute(TEXT("hideMenuRightShortcuts"));
			if (hideMenuRightShortcuts)
				_nppGUI._hideMenuRightShortcuts = lstrcmp(hideMenuRightShortcuts, TEXT("yes")) == 0;
		}
		else if (!lstrcmp(nm, TEXT("commandLineInterpreter")))
		{
			TiXmlNode *node = childNode->FirstChild();
			if (node)
			{
				const TCHAR *cli = node->Value();
				if (cli && cli[0])
					_nppGUI._commandLineInterpreter.assign(cli);
			}
		}
		else if (!lstrcmp(nm, TEXT("DarkMode")))
		{
			auto parseYesNoBoolAttribute = [&element](const TCHAR* name, bool defaultValue = false)->bool {
				const TCHAR* val = element->Attribute(name);
				if (val)
				{
					if (!lstrcmp(val, TEXT("yes")))
						return true;
					else if (!lstrcmp(val, TEXT("no")))
						return false;
				}
				return defaultValue;
			};

			_nppGUI._darkmode._isEnabled = parseYesNoBoolAttribute(TEXT("enable"));

			//_nppGUI._darkmode._isEnabledPlugin = parseYesNoBoolAttribute(TEXT("enablePlugin", true));

			int i;
			const TCHAR* val;
			val = element->Attribute(TEXT("colorTone"), &i);
			if (val)
				_nppGUI._darkmode._colorTone = static_cast<NppDarkMode::ColorTone>(i);


			val = element->Attribute(TEXT("customColorTop"), &i);
			if (val)
				_nppGUI._darkmode._customColors.pureBackground = i;

			val = element->Attribute(TEXT("customColorMenuHotTrack"), &i);
			if (val)
				_nppGUI._darkmode._customColors.hotBackground = i;

			val = element->Attribute(TEXT("customColorActive"), &i);
			if (val)
				_nppGUI._darkmode._customColors.softerBackground = i;

			val = element->Attribute(TEXT("customColorMain"), &i);
			if (val)
				_nppGUI._darkmode._customColors.background = i;

			val = element->Attribute(TEXT("customColorError"), &i);
			if (val)
				_nppGUI._darkmode._customColors.errorBackground = i;

			val = element->Attribute(TEXT("customColorText"), &i);
			if (val)
				_nppGUI._darkmode._customColors.text = i;

			val = element->Attribute(TEXT("customColorDarkText"), &i);
			if (val)
				_nppGUI._darkmode._customColors.darkerText = i;

			val = element->Attribute(TEXT("customColorDisabledText"), &i);
			if (val)
				_nppGUI._darkmode._customColors.disabledText = i;

			val = element->Attribute(TEXT("customColorLinkText"), &i);
			if (val)
				_nppGUI._darkmode._customColors.linkText = i;

			val = element->Attribute(TEXT("customColorEdge"), &i);
			if (val)
				_nppGUI._darkmode._customColors.edge = i;

			val = element->Attribute(TEXT("customColorHotEdge"), &i);
			if (val)
				_nppGUI._darkmode._customColors.hotEdge = i;

			val = element->Attribute(TEXT("customColorDisabledEdge"), &i);
			if (val)
				_nppGUI._darkmode._customColors.disabledEdge = i;

			// advanced options section
			auto parseStringAttribute = [&element](const TCHAR* name, const TCHAR* defaultName = TEXT("")) -> const TCHAR* {
				const TCHAR* val = element->Attribute(name);
				if (val != nullptr && val[0])
				{
					return element->Attribute(name);
				}
				return defaultName;
			};

			auto parseToolBarIconsAttribute = [&element](const TCHAR* name, int defaultValue = -1) -> int {
				int val;
				const TCHAR* valStr = element->Attribute(name, &val);
				if (valStr != nullptr && (val >= 0 && val <= 4))
				{
					return val;
				}
				return defaultValue;
			};

			auto parseTabIconsAttribute = [&element](const TCHAR* name, int defaultValue = -1) -> int {
				int val;
				const TCHAR* valStr = element->Attribute(name, &val);
				if (valStr != nullptr && (val >= 0 && val <= 2))
				{
					return val;
				}
				return defaultValue;
			};

			auto& windowsMode = _nppGUI._darkmode._advOptions._enableWindowsMode;
			windowsMode = parseYesNoBoolAttribute(TEXT("enableWindowsMode"));

			auto& darkDefaults = _nppGUI._darkmode._advOptions._darkDefaults;
			auto& darkThemeName = darkDefaults._xmlFileName;
			darkThemeName = parseStringAttribute(TEXT("darkThemeName"), TEXT("DarkModeDefault.xml"));
			darkDefaults._toolBarIconSet = parseToolBarIconsAttribute(TEXT("darkToolBarIconSet"), 0);
			darkDefaults._tabIconSet = parseTabIconsAttribute(TEXT("darkTabIconSet"), 2);
			darkDefaults._tabUseTheme = parseYesNoBoolAttribute(TEXT("darkTabUseTheme"));

			auto& lightDefaults = _nppGUI._darkmode._advOptions._lightDefaults;
			auto& lightThemeName = lightDefaults._xmlFileName;
			lightThemeName = parseStringAttribute(TEXT("lightThemeName"));
			lightDefaults._toolBarIconSet = parseToolBarIconsAttribute(TEXT("lightToolBarIconSet"), 4);
			lightDefaults._tabIconSet = parseTabIconsAttribute(TEXT("lightTabIconSet"), 0);
			lightDefaults._tabUseTheme = parseYesNoBoolAttribute(TEXT("lightTabUseTheme"), true);

			// Windows mode is handled later in Notepad_plus_Window::init from Notepad_plus_Window.cpp
			if (!windowsMode)
			{
				generic_string themePath;
				generic_string xmlFileName = _nppGUI._darkmode._isEnabled ? darkThemeName : lightThemeName;
				const bool isLocalOnly = _isLocal && !_isCloud;

				if (!xmlFileName.empty() && lstrcmp(xmlFileName.c_str(), TEXT("stylers.xml")) != 0)
				{
					themePath = isLocalOnly ? _nppPath : _userPath;
					pathAppend(themePath, TEXT("themes\\"));
					pathAppend(themePath, xmlFileName);

					if (!isLocalOnly && ::PathFileExists(themePath.c_str()) == FALSE)
					{
						themePath = _nppPath;
						pathAppend(themePath, TEXT("themes\\"));
						pathAppend(themePath, xmlFileName);
					}
				}
				else
				{
					themePath = isLocalOnly ? _nppPath : _userPath;
					pathAppend(themePath, TEXT("stylers.xml"));

					if (!isLocalOnly && ::PathFileExists(themePath.c_str()) == FALSE)
					{
						themePath = _nppPath;
						pathAppend(themePath, TEXT("stylers.xml"));
					}
				}

				if (::PathFileExists(themePath.c_str()) == TRUE)
				{
					_nppGUI._themeName.assign(themePath);
				}
			}
		}
	}
}

void NppParameters::feedScintillaParam(TiXmlNode *node)
{
	TiXmlElement* element = node->ToElement();

	// Line Number Margin
	const TCHAR *nm = element->Attribute(TEXT("lineNumberMargin"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("show")))
			_svp._lineNumberMarginShow = true;
		else if (!lstrcmp(nm, TEXT("hide")))
			_svp._lineNumberMarginShow = false;
	}

	// Line Number Margin dynamic width
	nm = element->Attribute(TEXT("lineNumberDynamicWidth"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._lineNumberMarginDynamicWidth = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._lineNumberMarginDynamicWidth = false;
	}

	// Bookmark Margin
	nm = element->Attribute(TEXT("bookMarkMargin"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("show")))
			_svp._bookMarkMarginShow = true;
		else if (!lstrcmp(nm, TEXT("hide")))
			_svp._bookMarkMarginShow = false;
	}

	// Bookmark Margin
	nm = element->Attribute(TEXT("isChangeHistoryEnabled"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
		{
			_svp._isChangeHistoryEnabled = true;
			_svp._isChangeHistoryEnabled4NextSession = true;
		}
		else if (!lstrcmp(nm, TEXT("no")))
		{
			_svp._isChangeHistoryEnabled = false;
			_svp._isChangeHistoryEnabled4NextSession = false;
		}
	}

	// Indent GuideLine
	nm = element->Attribute(TEXT("indentGuideLine"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("show")))
			_svp._indentGuideLineShow = true;
		else if (!lstrcmp(nm, TEXT("hide")))
			_svp._indentGuideLineShow= false;
	}

	// Folder Mark Style
	nm = element->Attribute(TEXT("folderMarkStyle"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("box")))
			_svp._folderStyle = FOLDER_STYLE_BOX;
		else if (!lstrcmp(nm, TEXT("circle")))
			_svp._folderStyle = FOLDER_STYLE_CIRCLE;
		else if (!lstrcmp(nm, TEXT("arrow")))
			_svp._folderStyle = FOLDER_STYLE_ARROW;
		else if (!lstrcmp(nm, TEXT("simple")))
			_svp._folderStyle = FOLDER_STYLE_SIMPLE;
		else if (!lstrcmp(nm, TEXT("none")))
			_svp._folderStyle = FOLDER_STYLE_NONE;
	}

	// Line Wrap method
	nm = element->Attribute(TEXT("lineWrapMethod"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("default")))
			_svp._lineWrapMethod = LINEWRAP_DEFAULT;
		else if (!lstrcmp(nm, TEXT("aligned")))
			_svp._lineWrapMethod = LINEWRAP_ALIGNED;
		else if (!lstrcmp(nm, TEXT("indent")))
			_svp._lineWrapMethod = LINEWRAP_INDENT;
	}

	// Current Line Highlighting State
	nm = element->Attribute(TEXT("currentLineHilitingShow"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("show")))
			_svp._currentLineHiliteMode = LINEHILITE_HILITE;
		else
			_svp._currentLineHiliteMode = LINEHILITE_NONE;
	}
	else
	{
		const TCHAR* currentLineModeStr = element->Attribute(TEXT("currentLineIndicator"));
		if (currentLineModeStr && currentLineModeStr[0])
		{
			if (lstrcmp(currentLineModeStr, TEXT("1")) == 0)
				_svp._currentLineHiliteMode = LINEHILITE_HILITE;
			else if (lstrcmp(currentLineModeStr, TEXT("2")) == 0)
				_svp._currentLineHiliteMode = LINEHILITE_FRAME;
			else
				_svp._currentLineHiliteMode = LINEHILITE_NONE;
		}
	}

	// Current Line Frame Width
	nm = element->Attribute(TEXT("currentLineFrameWidth"));
	if (nm)
	{
		unsigned char frameWidth{ 1 };
		try
		{
			frameWidth = static_cast<unsigned char>(std::stoi(nm));
		}
		catch (...)
		{
			// do nothing. frameWidth is already set to '1'.
		}
		_svp._currentLineFrameWidth = (frameWidth < 1) ? 1 : (frameWidth > 6) ? 6 : frameWidth;
	}

	// Virtual Space
	nm = element->Attribute(TEXT("virtualSpace"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._virtualSpace = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._virtualSpace = false;
	}

	// Scrolling Beyond Last Line State
	nm = element->Attribute(TEXT("scrollBeyondLastLine"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._scrollBeyondLastLine = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._scrollBeyondLastLine = false;
	}

	// Do not change selection or caret position when right-clicking with mouse
	nm = element->Attribute(TEXT("rightClickKeepsSelection"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._rightClickKeepsSelection = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._rightClickKeepsSelection = false;
	}

	// Disable Advanced Scrolling
	nm = element->Attribute(TEXT("disableAdvancedScrolling"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._disableAdvancedScrolling = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._disableAdvancedScrolling = false;
	}

	// Current wrap symbol visibility State
	nm = element->Attribute(TEXT("wrapSymbolShow"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("show")))
			_svp._wrapSymbolShow = true;
		else if (!lstrcmp(nm, TEXT("hide")))
			_svp._wrapSymbolShow = false;
	}

	// Do Wrap
	nm = element->Attribute(TEXT("Wrap"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._doWrap = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._doWrap = false;
	}

	// Do Edge
	nm = element->Attribute(TEXT("isEdgeBgMode"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._isEdgeBgMode = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._isEdgeBgMode = false;
	}

	// Do Scintilla border edge
	nm = element->Attribute(TEXT("borderEdge"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._showBorderEdge = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._showBorderEdge = false;
	}

	nm = element->Attribute(TEXT("edgeMultiColumnPos"));
	if (nm)
	{
		str2numberVector(nm, _svp._edgeMultiColumnPos);
	}

	int val;
	nm = element->Attribute(TEXT("zoom"), &val);
	if (nm)
	{
		_svp._zoom = val;
	}

	nm = element->Attribute(TEXT("zoom2"), &val);
	if (nm)
	{
		_svp._zoom2 = val;
	}

	// White Space visibility State
	nm = element->Attribute(TEXT("whiteSpaceShow"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("show")))
			_svp._whiteSpaceShow = true;
		else if (!lstrcmp(nm, TEXT("hide")))
			_svp._whiteSpaceShow = false;
	}

	// EOL visibility State
	nm = element->Attribute(TEXT("eolShow"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("show")))
			_svp._eolShow = true;
		else if (!lstrcmp(nm, TEXT("hide")))
			_svp._eolShow = false;
	}

	nm = element->Attribute(TEXT("eolMode"), &val);
	if (nm)
	{
		if (val >= 0 && val <= 3)
			_svp._eolMode = static_cast<ScintillaViewParams::crlfMode>(val);
	}

	nm = element->Attribute(TEXT("borderWidth"), &val);
	if (nm)
	{
		if (val >= 0 && val <= 30)
			_svp._borderWidth = val;
	}

	// Do antialiased font
	nm = element->Attribute(TEXT("smoothFont"));
	if (nm)
	{
		if (!lstrcmp(nm, TEXT("yes")))
			_svp._doSmoothFont = true;
		else if (!lstrcmp(nm, TEXT("no")))
			_svp._doSmoothFont = false;
	}

	nm = element->Attribute(TEXT("paddingLeft"), &val);
	if (nm)
	{
		if (val >= 0 && val <= 30)
			_svp._paddingLeft = static_cast<unsigned char>(val);
	}

	nm = element->Attribute(TEXT("paddingRight"), &val);
	if (nm)
	{
		if (val >= 0 && val <= 30)
			_svp._paddingRight = static_cast<unsigned char>(val);
	}

	nm = element->Attribute(TEXT("distractionFreeDivPart"), &val);
	if (nm)
	{
		if (val >= 3 && val <= 9)
			_svp._distractionFreeDivPart = static_cast<unsigned char>(val);
	}
}


void NppParameters::feedDockingManager(TiXmlNode *node)
{
	TiXmlElement *element = node->ToElement();

	int i;
	if (element->Attribute(TEXT("leftWidth"), &i))
		_nppGUI._dockingData._leftWidth = i;

	if (element->Attribute(TEXT("rightWidth"), &i))
		_nppGUI._dockingData._rightWidth = i;

	if (element->Attribute(TEXT("topHeight"), &i))
		_nppGUI._dockingData._topHeight = i;

	if (element->Attribute(TEXT("bottomHeight"), &i))
		_nppGUI._dockingData._bottomHight = i;

	for (TiXmlNode *childNode = node->FirstChildElement(TEXT("FloatingWindow"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("FloatingWindow")) )
	{
		TiXmlElement *floatElement = childNode->ToElement();
		int cont;
		if (floatElement->Attribute(TEXT("cont"), &cont))
		{
			int x = 0;
			int y = 0;
			int w = 100;
			int h = 100;

			floatElement->Attribute(TEXT("x"), &x);
			floatElement->Attribute(TEXT("y"), &y);
			floatElement->Attribute(TEXT("width"), &w);
			floatElement->Attribute(TEXT("height"), &h);
			_nppGUI._dockingData._flaotingWindowInfo.push_back(FloatingWindowInfo(cont, x, y, w, h));
		}
	}

	for (TiXmlNode *childNode = node->FirstChildElement(TEXT("PluginDlg"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("PluginDlg")) )
	{
		TiXmlElement *dlgElement = childNode->ToElement();
		const TCHAR *name = dlgElement->Attribute(TEXT("pluginName"));

		int id;
		const TCHAR *idStr = dlgElement->Attribute(TEXT("id"), &id);
		if (name && idStr)
		{
			int curr = 0; // on left
			int prev = 0; // on left

			dlgElement->Attribute(TEXT("curr"), &curr);
			dlgElement->Attribute(TEXT("prev"), &prev);

			bool isVisible = false;
			const TCHAR *val = dlgElement->Attribute(TEXT("isVisible"));
			if (val)
			{
				isVisible = (lstrcmp(val, TEXT("yes")) == 0);
			}

			_nppGUI._dockingData._pluginDockInfo.push_back(PluginDlgDockingInfo(name, id, curr, prev, isVisible));
		}
	}

	for (TiXmlNode *childNode = node->FirstChildElement(TEXT("ActiveTabs"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("ActiveTabs")) )
	{
		TiXmlElement *dlgElement = childNode->ToElement();

		int cont;
		if (dlgElement->Attribute(TEXT("cont"), &cont))
		{
			int activeTab = 0;
			dlgElement->Attribute(TEXT("activeTab"), &activeTab);
			_nppGUI._dockingData._containerTabInfo.push_back(ContainerTabInfo(cont, activeTab));
		}
	}
}

void NppParameters::duplicateDockingManager(TiXmlNode* dockMngNode, TiXmlElement* dockMngElmt2Clone)
{
	if (!dockMngNode || !dockMngElmt2Clone) return;

	TiXmlElement *dockMngElmt = dockMngNode->ToElement();
	
	int i;
	if (dockMngElmt->Attribute(TEXT("leftWidth"), &i))
		dockMngElmt2Clone->SetAttribute(TEXT("leftWidth"), i);

	if (dockMngElmt->Attribute(TEXT("rightWidth"), &i))
		dockMngElmt2Clone->SetAttribute(TEXT("rightWidth"), i);

	if (dockMngElmt->Attribute(TEXT("topHeight"), &i))
		dockMngElmt2Clone->SetAttribute(TEXT("topHeight"), i);

	if (dockMngElmt->Attribute(TEXT("bottomHeight"), &i))
		dockMngElmt2Clone->SetAttribute(TEXT("bottomHeight"), i);


	for (TiXmlNode *childNode = dockMngNode->FirstChildElement(TEXT("FloatingWindow"));
		childNode;
		childNode = childNode->NextSibling(TEXT("FloatingWindow")))
	{
		TiXmlElement *floatElement = childNode->ToElement();
		int cont;
		if (floatElement->Attribute(TEXT("cont"), &cont))
		{
			TiXmlElement FWNode(TEXT("FloatingWindow"));
			FWNode.SetAttribute(TEXT("cont"), cont);

			int x = 0;
			int y = 0;
			int w = 100;
			int h = 100;

			floatElement->Attribute(TEXT("x"), &x);
			FWNode.SetAttribute(TEXT("x"), x);

			floatElement->Attribute(TEXT("y"), &y);
			FWNode.SetAttribute(TEXT("y"), y);
			
			floatElement->Attribute(TEXT("width"), &w);
			FWNode.SetAttribute(TEXT("width"), w);
			
			floatElement->Attribute(TEXT("height"), &h);
			FWNode.SetAttribute(TEXT("height"), h);

			dockMngElmt2Clone->InsertEndChild(FWNode);
		}
	}

	for (TiXmlNode *childNode = dockMngNode->FirstChildElement(TEXT("PluginDlg"));
		childNode;
		childNode = childNode->NextSibling(TEXT("PluginDlg")))
	{
		TiXmlElement *dlgElement = childNode->ToElement();
		const TCHAR *name = dlgElement->Attribute(TEXT("pluginName"));
		TiXmlElement PDNode(TEXT("PluginDlg"));

		int id;
		const TCHAR *idStr = dlgElement->Attribute(TEXT("id"), &id);
		if (name && idStr)
		{
			int curr = 0; // on left
			int prev = 0; // on left

			dlgElement->Attribute(TEXT("curr"), &curr);
			dlgElement->Attribute(TEXT("prev"), &prev);

			bool isVisible = false;
			const TCHAR *val = dlgElement->Attribute(TEXT("isVisible"));
			if (val)
			{
				isVisible = (lstrcmp(val, TEXT("yes")) == 0);
			}

			PDNode.SetAttribute(TEXT("pluginName"), name);
			PDNode.SetAttribute(TEXT("id"), idStr);
			PDNode.SetAttribute(TEXT("curr"), curr);
			PDNode.SetAttribute(TEXT("prev"), prev);
			PDNode.SetAttribute(TEXT("isVisible"), isVisible ? TEXT("yes") : TEXT("no"));

			dockMngElmt2Clone->InsertEndChild(PDNode);
		}
	}

	for (TiXmlNode *childNode = dockMngNode->FirstChildElement(TEXT("ActiveTabs"));
		childNode;
		childNode = childNode->NextSibling(TEXT("ActiveTabs")))
	{
		TiXmlElement *dlgElement = childNode->ToElement();
		TiXmlElement CTNode(TEXT("ActiveTabs"));
		int cont;
		if (dlgElement->Attribute(TEXT("cont"), &cont))
		{
			int activeTab = 0;
			dlgElement->Attribute(TEXT("activeTab"), &activeTab);

			CTNode.SetAttribute(TEXT("cont"), cont);
			CTNode.SetAttribute(TEXT("activeTab"), activeTab);

			dockMngElmt2Clone->InsertEndChild(CTNode);
		}
	}
}

bool NppParameters::writeScintillaParams()
{
	if (!_pXmlUserDoc) return false;

	const TCHAR *pViewName = TEXT("ScintillaPrimaryView");
	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *configsRoot = nppRoot->FirstChildElement(TEXT("GUIConfigs"));
	if (!configsRoot)
	{
		configsRoot = nppRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfigs")));
	}

	TiXmlNode *scintNode = getChildElementByAttribut(configsRoot, TEXT("GUIConfig"), TEXT("name"), pViewName);
	if (!scintNode)
	{
		scintNode = configsRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig")));
		(scintNode->ToElement())->SetAttribute(TEXT("name"), pViewName);
	}

	(scintNode->ToElement())->SetAttribute(TEXT("lineNumberMargin"), _svp._lineNumberMarginShow?TEXT("show"):TEXT("hide"));
	(scintNode->ToElement())->SetAttribute(TEXT("lineNumberDynamicWidth"), _svp._lineNumberMarginDynamicWidth ?TEXT("yes"):TEXT("no"));
	(scintNode->ToElement())->SetAttribute(TEXT("bookMarkMargin"), _svp._bookMarkMarginShow?TEXT("show"):TEXT("hide"));
	(scintNode->ToElement())->SetAttribute(TEXT("indentGuideLine"), _svp._indentGuideLineShow?TEXT("show"):TEXT("hide"));
	const TCHAR *pFolderStyleStr = (_svp._folderStyle == FOLDER_STYLE_SIMPLE)?TEXT("simple"):
									(_svp._folderStyle == FOLDER_STYLE_ARROW)?TEXT("arrow"):
										(_svp._folderStyle == FOLDER_STYLE_CIRCLE)?TEXT("circle"):
										(_svp._folderStyle == FOLDER_STYLE_NONE)?TEXT("none"):TEXT("box");
	(scintNode->ToElement())->SetAttribute(TEXT("folderMarkStyle"), pFolderStyleStr);
	(scintNode->ToElement())->SetAttribute(TEXT("isChangeHistoryEnabled"), _svp._isChangeHistoryEnabled4NextSession ? TEXT("yes") : TEXT("no"));
	const TCHAR *pWrapMethodStr = (_svp._lineWrapMethod == LINEWRAP_ALIGNED)?TEXT("aligned"):
								(_svp._lineWrapMethod == LINEWRAP_INDENT)?TEXT("indent"):TEXT("default");
	(scintNode->ToElement())->SetAttribute(TEXT("lineWrapMethod"), pWrapMethodStr);

	(scintNode->ToElement())->SetAttribute(TEXT("currentLineIndicator"), _svp._currentLineHiliteMode);
	(scintNode->ToElement())->SetAttribute(TEXT("currentLineFrameWidth"), _svp._currentLineFrameWidth);

	(scintNode->ToElement())->SetAttribute(TEXT("virtualSpace"), _svp._virtualSpace?TEXT("yes"):TEXT("no"));
	(scintNode->ToElement())->SetAttribute(TEXT("scrollBeyondLastLine"), _svp._scrollBeyondLastLine?TEXT("yes"):TEXT("no"));
	(scintNode->ToElement())->SetAttribute(TEXT("rightClickKeepsSelection"), _svp._rightClickKeepsSelection ? TEXT("yes") : TEXT("no"));
	(scintNode->ToElement())->SetAttribute(TEXT("disableAdvancedScrolling"), _svp._disableAdvancedScrolling?TEXT("yes"):TEXT("no"));
	(scintNode->ToElement())->SetAttribute(TEXT("wrapSymbolShow"), _svp._wrapSymbolShow?TEXT("show"):TEXT("hide"));
	(scintNode->ToElement())->SetAttribute(TEXT("Wrap"), _svp._doWrap?TEXT("yes"):TEXT("no"));
	(scintNode->ToElement())->SetAttribute(TEXT("borderEdge"), _svp._showBorderEdge ? TEXT("yes") : TEXT("no"));

	generic_string edgeColumnPosStr;
	for (auto i : _svp._edgeMultiColumnPos)
	{
		std::string s = std::to_string(i);
		edgeColumnPosStr += generic_string(s.begin(), s.end());
		edgeColumnPosStr += TEXT(" ");
	}
	(scintNode->ToElement())->SetAttribute(TEXT("isEdgeBgMode"), _svp._isEdgeBgMode ? TEXT("yes") : TEXT("no"));
	(scintNode->ToElement())->SetAttribute(TEXT("edgeMultiColumnPos"), edgeColumnPosStr);
	(scintNode->ToElement())->SetAttribute(TEXT("zoom"), static_cast<int>(_svp._zoom));
	(scintNode->ToElement())->SetAttribute(TEXT("zoom2"), static_cast<int>(_svp._zoom2));
	(scintNode->ToElement())->SetAttribute(TEXT("whiteSpaceShow"), _svp._whiteSpaceShow?TEXT("show"):TEXT("hide"));
	(scintNode->ToElement())->SetAttribute(TEXT("eolShow"), _svp._eolShow?TEXT("show"):TEXT("hide"));
	(scintNode->ToElement())->SetAttribute(TEXT("eolMode"), _svp._eolMode);
	(scintNode->ToElement())->SetAttribute(TEXT("borderWidth"), _svp._borderWidth);
	(scintNode->ToElement())->SetAttribute(TEXT("smoothFont"), _svp._doSmoothFont ? TEXT("yes") : TEXT("no"));
	(scintNode->ToElement())->SetAttribute(TEXT("paddingLeft"), _svp._paddingLeft);
	(scintNode->ToElement())->SetAttribute(TEXT("paddingRight"), _svp._paddingRight);
	(scintNode->ToElement())->SetAttribute(TEXT("distractionFreeDivPart"), _svp._distractionFreeDivPart);
	return true;
}

void NppParameters::createXmlTreeFromGUIParams()
{
	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *oldGUIRoot = nppRoot->FirstChildElement(TEXT("GUIConfigs"));
	TiXmlElement* dockMngNodeDup = nullptr;
	TiXmlNode* dockMngNodeOriginal = nullptr;
	if (oldGUIRoot && _nppGUI._isCmdlineNosessionActivated)
	{
		for (TiXmlNode *childNode = oldGUIRoot->FirstChildElement(TEXT("GUIConfig"));
			childNode;
			childNode = childNode->NextSibling(TEXT("GUIConfig")))
		{
			TiXmlElement* element = childNode->ToElement();
			const TCHAR* nm = element->Attribute(TEXT("name"));
			if (nullptr == nm)
				continue;

			if (!lstrcmp(nm, TEXT("DockingManager")))
			{
				dockMngNodeOriginal = childNode;
				break;
			}
		}

		// Copy DockingParamNode
		if (dockMngNodeOriginal)
		{
			dockMngNodeDup = new TiXmlElement(TEXT("GUIConfig"));
			dockMngNodeDup->SetAttribute(TEXT("name"), TEXT("DockingManager"));

			duplicateDockingManager(dockMngNodeOriginal, dockMngNodeDup);
		}
	}

	// Remove the old root nod if it exist
	if (oldGUIRoot)
	{
		nppRoot->RemoveChild(oldGUIRoot);
	}

	TiXmlNode *newGUIRoot = nppRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfigs")));

	// <GUIConfig name="ToolBar" visible="yes">standard</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("ToolBar"));
		const TCHAR *pStr = (_nppGUI._toolbarShow) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("visible"), pStr);

		if (_nppGUI._toolBarStatus == TB_SMALL)
			pStr = TEXT("small");
		else if (_nppGUI._toolBarStatus == TB_LARGE)
			pStr = TEXT("large");
		else if (_nppGUI._toolBarStatus == TB_SMALL2)
			pStr = TEXT("small2");
		else if (_nppGUI._toolBarStatus == TB_LARGE2)
			pStr = TEXT("large2");
		else //if (_nppGUI._toolBarStatus == TB_STANDARD)
			pStr = TEXT("standard");
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name="StatusBar">show</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("StatusBar"));
		const TCHAR *pStr = _nppGUI._statusBarShow ? TEXT("show") : TEXT("hide");
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name="TabBar" dragAndDrop="yes" drawTopBar="yes" drawInactiveTab="yes" reduce="yes" closeButton="yes" doubleClick2Close="no" vertical="no" multiLine="no" hide="no" quitOnEmpty="no" iconSetNumber="0" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("TabBar"));

		const TCHAR *pStr = (_nppGUI._tabStatus & TAB_DRAWTOPBAR) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("dragAndDrop"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_DRAGNDROP) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("drawTopBar"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_DRAWINACTIVETAB) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("drawInactiveTab"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_REDUCE) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("reduce"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_CLOSEBUTTON) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("closeButton"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_DBCLK2CLOSE) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("doubleClick2Close"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_VERTICAL) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("vertical"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_MULTILINE) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("multiLine"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_HIDE) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("hide"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_QUITONEMPTY) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("quitOnEmpty"), pStr);

		pStr = (_nppGUI._tabStatus & TAB_ALTICONS) ? TEXT("1") : TEXT("0");
		GUIConfigElement->SetAttribute(TEXT("iconSetNumber"), pStr);
	}

	// <GUIConfig name="ScintillaViewsSplitter">vertical</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("ScintillaViewsSplitter"));
		const TCHAR *pStr = _nppGUI._splitterPos == POS_VERTICAL ? TEXT("vertical") : TEXT("horizontal");
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name="UserDefineDlg" position="undocked">hide</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("UserDefineDlg"));
		const TCHAR *pStr = (_nppGUI._userDefineDlgStatus & UDD_DOCKED) ? TEXT("docked") : TEXT("undocked");
		GUIConfigElement->SetAttribute(TEXT("position"), pStr);
		pStr = (_nppGUI._userDefineDlgStatus & UDD_SHOW) ? TEXT("show") : TEXT("hide");
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name = "TabSetting" size = "4" replaceBySpace = "no" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("TabSetting"));
		const TCHAR *pStr = _nppGUI._tabReplacedBySpace ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("replaceBySpace"), pStr);
		GUIConfigElement->SetAttribute(TEXT("size"), _nppGUI._tabSize);
	}

	// <GUIConfig name = "AppPosition" x = "3900" y = "446" width = "2160" height = "1380" isMaximized = "no" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("AppPosition"));
		GUIConfigElement->SetAttribute(TEXT("x"), _nppGUI._appPos.left);
		GUIConfigElement->SetAttribute(TEXT("y"), _nppGUI._appPos.top);
		GUIConfigElement->SetAttribute(TEXT("width"), _nppGUI._appPos.right);
		GUIConfigElement->SetAttribute(TEXT("height"), _nppGUI._appPos.bottom);
		GUIConfigElement->SetAttribute(TEXT("isMaximized"), _nppGUI._isMaximized ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="FindWindowPosition" left="134" top="320" right="723" bottom="684" />
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("FindWindowPosition"));
		GUIConfigElement->SetAttribute(TEXT("left"), _nppGUI._findWindowPos.left);
		GUIConfigElement->SetAttribute(TEXT("top"), _nppGUI._findWindowPos.top);
		GUIConfigElement->SetAttribute(TEXT("right"), _nppGUI._findWindowPos.right);
		GUIConfigElement->SetAttribute(TEXT("bottom"), _nppGUI._findWindowPos.bottom);
		GUIConfigElement->SetAttribute(TEXT("isLessModeOn"), _nppGUI._findWindowLessMode ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="FinderConfig" wrappedLines="no" purgeBeforeEverySearch="no" showOnlyOneEntryPerFoundLine="yes"/>
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("FinderConfig"));
		const TCHAR* pStr = _nppGUI._finderLinesAreCurrentlyWrapped ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("wrappedLines"), pStr);
		pStr = _nppGUI._finderPurgeBeforeEverySearch ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("purgeBeforeEverySearch"), pStr);
		pStr = _nppGUI._finderShowOnlyOneEntryPerFoundLine ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("showOnlyOneEntryPerFoundLine"), pStr);

	}

	// <GUIConfig name="noUpdate" intervalDays="15" nextUpdateDate="20161022">no</GUIConfig>
	{
		TiXmlElement *element = insertGUIConfigBoolNode(newGUIRoot, TEXT("noUpdate"), !_nppGUI._autoUpdateOpt._doAutoUpdate);
		element->SetAttribute(TEXT("intervalDays"), _nppGUI._autoUpdateOpt._intervalDays);
		element->SetAttribute(TEXT("nextUpdateDate"), _nppGUI._autoUpdateOpt._nextUpdateDate.toString().c_str());
	}

	// <GUIConfig name="Auto-detection">yes</GUIConfig>	
	{
		const TCHAR *pStr = TEXT("no");

		if (_nppGUI._fileAutoDetection & cdEnabledOld)
		{
			pStr = TEXT("yesOld");

			if ((_nppGUI._fileAutoDetection & cdAutoUpdate) && (_nppGUI._fileAutoDetection & cdGo2end))
			{
				pStr = TEXT("autoUpdate2EndOld");
			}
			else if (_nppGUI._fileAutoDetection & cdAutoUpdate)
			{
				pStr = TEXT("autoOld");
			}
			else if (_nppGUI._fileAutoDetection & cdGo2end)
			{
				pStr = TEXT("Update2EndOld");
			}
		}
		else if (_nppGUI._fileAutoDetection & cdEnabledNew)
		{
			pStr = TEXT("yes");

			if ((_nppGUI._fileAutoDetection & cdAutoUpdate) && (_nppGUI._fileAutoDetection & cdGo2end))
			{
				pStr = TEXT("autoUpdate2End");
			}
			else if (_nppGUI._fileAutoDetection & cdAutoUpdate)
			{
				pStr = TEXT("auto");
			}
			else if (_nppGUI._fileAutoDetection & cdGo2end)
			{
				pStr = TEXT("Update2End");
			}
		}

		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("Auto-detection"));
		GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	}

	// <GUIConfig name="CheckHistoryFiles">no</GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, TEXT("CheckHistoryFiles"), _nppGUI._checkHistoryFiles);
	}

	// <GUIConfig name="TrayIcon">no</GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, TEXT("TrayIcon"), _nppGUI._isMinimizedToTray);
	}

	// <GUIConfig name="MaitainIndent">yes</GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, TEXT("MaitainIndent"), _nppGUI._maitainIndent);
	}

	// <GUIConfig name = "TagsMatchHighLight" TagAttrHighLight = "yes" HighLightNonHtmlZone = "no">yes< / GUIConfig>
	{
		TiXmlElement * ele = insertGUIConfigBoolNode(newGUIRoot, TEXT("TagsMatchHighLight"), _nppGUI._enableTagsMatchHilite);
		ele->SetAttribute(TEXT("TagAttrHighLight"), _nppGUI._enableTagAttrsHilite ? TEXT("yes") : TEXT("no"));
		ele->SetAttribute(TEXT("HighLightNonHtmlZone"), _nppGUI._enableHiliteNonHTMLZone ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name = "RememberLastSession">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, TEXT("RememberLastSession"), _nppGUI._rememberLastSession);
	}

	// <GUIConfig name = "DetectEncoding">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, TEXT("DetectEncoding"), _nppGUI._detectEncoding);
	}
	
	// <GUIConfig name = "SaveAllConfirm">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, TEXT("SaveAllConfirm"), _nppGUI._saveAllConfirm);
	}

	// <GUIConfig name = "NewDocDefaultSettings" format = "0" encoding = "0" lang = "3" codepage = "-1" openAnsiAsUTF8 = "no" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("NewDocDefaultSettings"));
		GUIConfigElement->SetAttribute(TEXT("format"), static_cast<int32_t>(_nppGUI._newDocDefaultSettings._format));
		GUIConfigElement->SetAttribute(TEXT("encoding"), _nppGUI._newDocDefaultSettings._unicodeMode);
		GUIConfigElement->SetAttribute(TEXT("lang"), _nppGUI._newDocDefaultSettings._lang);
		GUIConfigElement->SetAttribute(TEXT("codepage"), _nppGUI._newDocDefaultSettings._codepage);
		GUIConfigElement->SetAttribute(TEXT("openAnsiAsUTF8"), _nppGUI._newDocDefaultSettings._openAnsiAsUtf8 ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name = "langsExcluded" gr0 = "0" gr1 = "0" gr2 = "0" gr3 = "0" gr4 = "0" gr5 = "0" gr6 = "0" gr7 = "0" langMenuCompact = "yes" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("langsExcluded"));
		writeExcludedLangList(GUIConfigElement);
		GUIConfigElement->SetAttribute(TEXT("langMenuCompact"), _nppGUI._isLangMenuCompact ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="Print" lineNumber="no" printOption="0" headerLeft="$(FULL_CURRENT_PATH)" headerMiddle="" headerRight="$(LONG_DATE) $(TIME)" headerFontName="IBMPC" headerFontStyle="1" headerFontSize="8" footerLeft="" footerMiddle="-$(CURRENT_PRINTING_PAGE)-" footerRight="" footerFontName="" footerFontStyle="0" footerFontSize="9" margeLeft="0" margeTop="0" margeRight="0" margeBottom="0" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("Print"));
		writePrintSetting(GUIConfigElement);
	}

	// <GUIConfig name="Backup" action="0" useCustumDir="no" dir="" isSnapshotMode="yes" snapshotBackupTiming="7000" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("Backup"));
		GUIConfigElement->SetAttribute(TEXT("action"), _nppGUI._backup);
		GUIConfigElement->SetAttribute(TEXT("useCustumDir"), _nppGUI._useDir ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("dir"), _nppGUI._backupDir.c_str());

		GUIConfigElement->SetAttribute(TEXT("isSnapshotMode"), _nppGUI._isSnapshotMode ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("snapshotBackupTiming"), static_cast<int32_t>(_nppGUI._snapshotBackupTiming));
	}

	// <GUIConfig name = "TaskList">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, TEXT("TaskList"), _nppGUI._doTaskList);
	}

	// <GUIConfig name = "MRU">yes< / GUIConfig>
	{
		insertGUIConfigBoolNode(newGUIRoot, TEXT("MRU"), _nppGUI._styleMRU);
	}

	// <GUIConfig name="URL">2</GUIConfig>
	{
		TCHAR szStr [12] = TEXT("0");
		generic_itoa(_nppGUI._styleURL, szStr, 10);
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("URL"));
		GUIConfigElement->InsertEndChild(TiXmlText(szStr));
	}

	// <GUIConfig name="uriCustomizedSchemes">svn://</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("uriCustomizedSchemes"));
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._uriSchemes.c_str()));
	}
	// <GUIConfig name = "globalOverride" fg = "no" bg = "no" font = "no" fontSize = "no" bold = "no" italic = "no" underline = "no" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("globalOverride"));
		GUIConfigElement->SetAttribute(TEXT("fg"), _nppGUI._globalOverride.enableFg ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("bg"), _nppGUI._globalOverride.enableBg ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("font"), _nppGUI._globalOverride.enableFont ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("fontSize"), _nppGUI._globalOverride.enableFontSize ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("bold"), _nppGUI._globalOverride.enableBold ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("italic"), _nppGUI._globalOverride.enableItalic ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("underline"), _nppGUI._globalOverride.enableUnderLine ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name = "auto-completion" autoCAction = "3" triggerFromNbChar = "1" funcParams = "yes" autoCIgnoreNumbers = "yes" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("auto-completion"));
		GUIConfigElement->SetAttribute(TEXT("autoCAction"), _nppGUI._autocStatus);
		GUIConfigElement->SetAttribute(TEXT("triggerFromNbChar"), static_cast<int32_t>(_nppGUI._autocFromLen));

		const TCHAR * pStr = _nppGUI._autocIgnoreNumbers ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("autoCIgnoreNumbers"), pStr);

		pStr = _nppGUI._autocInsertSelectedUseENTER ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("insertSelectedItemUseENTER"), pStr);

		pStr = _nppGUI._autocInsertSelectedUseTAB ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("insertSelectedItemUseTAB"), pStr);

		pStr = _nppGUI._funcParams ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("funcParams"), pStr);
	}

	// <GUIConfig name = "auto-insert" parentheses = "yes" brackets = "yes" curlyBrackets = "yes" quotes = "no" doubleQuotes = "yes" htmlXmlTag = "yes" / >
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("auto-insert"));

		GUIConfigElement->SetAttribute(TEXT("parentheses"), _nppGUI._matchedPairConf._doParentheses ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("brackets"), _nppGUI._matchedPairConf._doBrackets ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("curlyBrackets"), _nppGUI._matchedPairConf._doCurlyBrackets ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("quotes"), _nppGUI._matchedPairConf._doQuotes ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("doubleQuotes"), _nppGUI._matchedPairConf._doDoubleQuotes ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("htmlXmlTag"), _nppGUI._matchedPairConf._doHtmlXmlTag ? TEXT("yes") : TEXT("no"));

		TiXmlElement hist_element{ TEXT("") };
		hist_element.SetValue(TEXT("UserDefinePair"));
		for (size_t i = 0, nb = _nppGUI._matchedPairConf._matchedPairs.size(); i < nb; ++i)
		{
			int open = _nppGUI._matchedPairConf._matchedPairs[i].first;
			int close = _nppGUI._matchedPairConf._matchedPairs[i].second;

			(hist_element.ToElement())->SetAttribute(TEXT("open"), open);
			(hist_element.ToElement())->SetAttribute(TEXT("close"), close);
			GUIConfigElement->InsertEndChild(hist_element);
		}
	}

	// <GUIConfig name = "sessionExt">< / GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("sessionExt"));
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._definedSessionExt.c_str()));
	}

	// <GUIConfig name="workspaceExt"></GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("workspaceExt"));
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._definedWorkspaceExt.c_str()));
	}

	// <GUIConfig name="MenuBar">show</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("MenuBar"));
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._menuBarShow ? TEXT("show") : TEXT("hide")));
	}

	// <GUIConfig name="Caret" width="1" blinkRate="250" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("Caret"));
		GUIConfigElement->SetAttribute(TEXT("width"), _nppGUI._caretWidth);
		GUIConfigElement->SetAttribute(TEXT("blinkRate"), _nppGUI._caretBlinkRate);
	}

	// <GUIConfig name="ScintillaGlobalSettings" enableMultiSelection="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("ScintillaGlobalSettings"));
		GUIConfigElement->SetAttribute(TEXT("enableMultiSelection"), _nppGUI._enableMultiSelection ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="openSaveDir" value="0" defaultDirPath="" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("openSaveDir"));
		GUIConfigElement->SetAttribute(TEXT("value"), _nppGUI._openSaveDir);
		GUIConfigElement->SetAttribute(TEXT("defaultDirPath"), _nppGUI._defaultDir);
	}

	// <GUIConfig name="titleBar" short="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("titleBar"));
		const TCHAR *pStr = (_nppGUI._shortTitlebar) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("short"), pStr);
	}

	// <GUIConfig name="insertDateTime" path="C:\sources\notepad-plus-plus\PowerEditor\visual.net\..\bin\stylers.xml" />
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("insertDateTime"));
		GUIConfigElement->SetAttribute(TEXT("customizedFormat"), _nppGUI._dateTimeFormat.c_str());
		const TCHAR* pStr = (_nppGUI._dateTimeReverseDefaultOrder) ? TEXT("yes") : TEXT("no");
		GUIConfigElement->SetAttribute(TEXT("reverseDefaultOrder"), pStr);
	}

	// <GUIConfig name="wordCharList" useDefault="yes" charsAdded=".$%"  />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("wordCharList"));
		GUIConfigElement->SetAttribute(TEXT("useDefault"), _nppGUI._isWordCharDefault ? TEXT("yes") : TEXT("no"));
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const wchar_t* charsAddStr = wmc.char2wchar(_nppGUI._customWordChars.c_str(), SC_CP_UTF8);
		GUIConfigElement->SetAttribute(TEXT("charsAdded"), charsAddStr);
	}

	// <GUIConfig name="delimiterSelection" leftmostDelimiter="40" rightmostDelimiter="41" delimiterSelectionOnEntireDocument="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("delimiterSelection"));
		GUIConfigElement->SetAttribute(TEXT("leftmostDelimiter"), _nppGUI._leftmostDelimiter);
		GUIConfigElement->SetAttribute(TEXT("rightmostDelimiter"), _nppGUI._rightmostDelimiter);
		GUIConfigElement->SetAttribute(TEXT("delimiterSelectionOnEntireDocument"), _nppGUI._delimiterSelectionOnEntireDocument ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="largeFileRestriction" fileSizeMB="200" isEnabled="yes" allowAutoCompletion="no" allowBraceMatch="no" deactivateWordWrap="yes" allowClickableLink="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("largeFileRestriction"));
		GUIConfigElement->SetAttribute(TEXT("fileSizeMB"), static_cast<int>((_nppGUI._largeFileRestriction._largeFileSizeDefInByte / 1024) / 1024));
		GUIConfigElement->SetAttribute(TEXT("isEnabled"), _nppGUI._largeFileRestriction._isEnabled ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("allowAutoCompletion"), _nppGUI._largeFileRestriction._allowAutoCompletion ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("allowBraceMatch"), _nppGUI._largeFileRestriction._allowBraceMatch ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("allowSmartHilite"), _nppGUI._largeFileRestriction._allowSmartHilite ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("allowClickableLink"), _nppGUI._largeFileRestriction._allowClickableLink ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("deactivateWordWrap"), _nppGUI._largeFileRestriction._deactivateWordWrap ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="multiInst" setting="0" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("multiInst"));
		GUIConfigElement->SetAttribute(TEXT("setting"), _nppGUI._multiInstSetting);
	}

	// <GUIConfig name="MISC" fileSwitcherWithoutExtColumn="no" backSlashIsEscapeCharacterForSql="yes" isFolderDroppedOpenFiles="no" saveDlgExtFilterToAllTypes="no" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("MISC"));

		GUIConfigElement->SetAttribute(TEXT("fileSwitcherWithoutExtColumn"), _nppGUI._fileSwitcherWithoutExtColumn ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("fileSwitcherExtWidth"), _nppGUI._fileSwitcherExtWidth);
		GUIConfigElement->SetAttribute(TEXT("fileSwitcherWithoutPathColumn"), _nppGUI._fileSwitcherWithoutPathColumn ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("fileSwitcherPathWidth"), _nppGUI._fileSwitcherPathWidth);
		GUIConfigElement->SetAttribute(TEXT("backSlashIsEscapeCharacterForSql"), _nppGUI._backSlashIsEscapeCharacterForSql ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("writeTechnologyEngine"), _nppGUI._writeTechnologyEngine);
		GUIConfigElement->SetAttribute(TEXT("isFolderDroppedOpenFiles"), _nppGUI._isFolderDroppedOpenFiles ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("docPeekOnTab"), _nppGUI._isDocPeekOnTab ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("docPeekOnMap"), _nppGUI._isDocPeekOnMap ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("sortFunctionList"), _nppGUI._shouldSortFunctionList ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("saveDlgExtFilterToAllTypes"), _nppGUI._setSaveDlgExtFiltToAllTypes ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("muteSounds"), _nppGUI._muteSounds ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("enableFoldCmdToggable"), _nppGUI._enableFoldCmdToggable ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("hideMenuRightShortcuts"), _nppGUI._hideMenuRightShortcuts ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="Searching" "monospacedFontFindDlg"="no" stopFillingFindField="no" findDlgAlwaysVisible="no" confirmReplaceOpenDocs="yes" confirmMacroReplaceOpenDocs="yes" confirmReplaceInFiles="yes" confirmMacroReplaceInFiles="yes" replaceStopsWithoutFindingNext="no"/>
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("Searching"));

		GUIConfigElement->SetAttribute(TEXT("monospacedFontFindDlg"), _nppGUI._monospacedFontFindDlg ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("fillFindFieldWithSelected"), _nppGUI._fillFindFieldWithSelected ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("fillFindFieldSelectCaret"), _nppGUI._fillFindFieldSelectCaret ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("findDlgAlwaysVisible"), _nppGUI._findDlgAlwaysVisible ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("confirmReplaceInAllOpenDocs"), _nppGUI._confirmReplaceInAllOpenDocs ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("replaceStopsWithoutFindingNext"), _nppGUI._replaceStopsWithoutFindingNext ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="searchEngine" searchEngineChoice="2" searchEngineCustom="" />
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("searchEngine"));
		GUIConfigElement->SetAttribute(TEXT("searchEngineChoice"), _nppGUI._searchEngineChoice);
		GUIConfigElement->SetAttribute(TEXT("searchEngineCustom"), _nppGUI._searchEngineCustom);
	}

	// <GUIConfig name="MarkAll" matchCase="no" wholeWordOnly="yes" </GUIConfig>
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("MarkAll"));
		GUIConfigElement->SetAttribute(TEXT("matchCase"), _nppGUI._markAllCaseSensitive ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("wholeWordOnly"), _nppGUI._markAllWordOnly ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="SmartHighLight" matchCase="no" wholeWordOnly="yes" useFindSettings="no" onAnotherView="no">yes</GUIConfig>
	{
		TiXmlElement *GUIConfigElement = insertGUIConfigBoolNode(newGUIRoot, TEXT("SmartHighLight"), _nppGUI._enableSmartHilite);
		GUIConfigElement->SetAttribute(TEXT("matchCase"), _nppGUI._smartHiliteCaseSensitive ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("wholeWordOnly"), _nppGUI._smartHiliteWordOnly ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("useFindSettings"), _nppGUI._smartHiliteUseFindSettings ? TEXT("yes") : TEXT("no"));
		GUIConfigElement->SetAttribute(TEXT("onAnotherView"), _nppGUI._smartHiliteOnAnotherView ? TEXT("yes") : TEXT("no"));
	}

	// <GUIConfig name="commandLineInterpreter">powershell</GUIConfig>
	if (_nppGUI._commandLineInterpreter.compare(CMD_INTERPRETER))
	{
		TiXmlElement *GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("commandLineInterpreter"));
		GUIConfigElement->InsertEndChild(TiXmlText(_nppGUI._commandLineInterpreter.c_str()));
	}

	// <GUIConfig name="DarkMode" enable="no" colorTone="0" />
	{
		TiXmlElement* GUIConfigElement = (newGUIRoot->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
		GUIConfigElement->SetAttribute(TEXT("name"), TEXT("DarkMode"));

		NppDarkMode::setAdvancedOptions();

		auto setYesNoBoolAttribute = [&GUIConfigElement](const TCHAR* name, bool value) {
			const TCHAR* pStr = value ? TEXT("yes") : TEXT("no");
			GUIConfigElement->SetAttribute(name, pStr);
		};

		setYesNoBoolAttribute(TEXT("enable"), _nppGUI._darkmode._isEnabled);
		GUIConfigElement->SetAttribute(TEXT("colorTone"), _nppGUI._darkmode._colorTone);

		GUIConfigElement->SetAttribute(TEXT("customColorTop"), _nppGUI._darkmode._customColors.pureBackground);
		GUIConfigElement->SetAttribute(TEXT("customColorMenuHotTrack"), _nppGUI._darkmode._customColors.hotBackground);
		GUIConfigElement->SetAttribute(TEXT("customColorActive"), _nppGUI._darkmode._customColors.softerBackground);
		GUIConfigElement->SetAttribute(TEXT("customColorMain"), _nppGUI._darkmode._customColors.background);
		GUIConfigElement->SetAttribute(TEXT("customColorError"), _nppGUI._darkmode._customColors.errorBackground);
		GUIConfigElement->SetAttribute(TEXT("customColorText"), _nppGUI._darkmode._customColors.text);
		GUIConfigElement->SetAttribute(TEXT("customColorDarkText"), _nppGUI._darkmode._customColors.darkerText);
		GUIConfigElement->SetAttribute(TEXT("customColorDisabledText"), _nppGUI._darkmode._customColors.disabledText);
		GUIConfigElement->SetAttribute(TEXT("customColorLinkText"), _nppGUI._darkmode._customColors.linkText);
		GUIConfigElement->SetAttribute(TEXT("customColorEdge"), _nppGUI._darkmode._customColors.edge);
		GUIConfigElement->SetAttribute(TEXT("customColorHotEdge"), _nppGUI._darkmode._customColors.hotEdge);
		GUIConfigElement->SetAttribute(TEXT("customColorDisabledEdge"), _nppGUI._darkmode._customColors.disabledEdge);

		// advanced options section
		setYesNoBoolAttribute(TEXT("enableWindowsMode"), _nppGUI._darkmode._advOptions._enableWindowsMode);

		GUIConfigElement->SetAttribute(TEXT("darkThemeName"), _nppGUI._darkmode._advOptions._darkDefaults._xmlFileName.c_str());
		GUIConfigElement->SetAttribute(TEXT("darkToolBarIconSet"), _nppGUI._darkmode._advOptions._darkDefaults._toolBarIconSet);
		GUIConfigElement->SetAttribute(TEXT("darkTabIconSet"), _nppGUI._darkmode._advOptions._darkDefaults._tabIconSet);
		setYesNoBoolAttribute(TEXT("darkTabUseTheme"), _nppGUI._darkmode._advOptions._darkDefaults._tabUseTheme);

		GUIConfigElement->SetAttribute(TEXT("lightThemeName"), _nppGUI._darkmode._advOptions._lightDefaults._xmlFileName.c_str());
		GUIConfigElement->SetAttribute(TEXT("lightToolBarIconSet"), _nppGUI._darkmode._advOptions._lightDefaults._toolBarIconSet);
		GUIConfigElement->SetAttribute(TEXT("lightTabIconSet"), _nppGUI._darkmode._advOptions._lightDefaults._tabIconSet);
		setYesNoBoolAttribute(TEXT("lightTabUseTheme"), _nppGUI._darkmode._advOptions._lightDefaults._tabUseTheme);
	}

	// <GUIConfig name="ScintillaPrimaryView" lineNumberMargin="show" bookMarkMargin="show" indentGuideLine="show" folderMarkStyle="box" lineWrapMethod="aligned" currentLineHilitingShow="show" scrollBeyondLastLine="no" rightClickKeepsSelection="no" disableAdvancedScrolling="no" wrapSymbolShow="hide" Wrap="no" borderEdge="yes" edge="no" edgeNbColumn="80" zoom="0" zoom2="0" whiteSpaceShow="hide" eolShow="hide" borderWidth="2" smoothFont="no" />
	writeScintillaParams();

	// <GUIConfig name="DockingManager" leftWidth="328" rightWidth="359" topHeight="200" bottomHeight="436">
	// ...
	if (_nppGUI._isCmdlineNosessionActivated && dockMngNodeDup)
	{
		newGUIRoot->InsertEndChild(*dockMngNodeDup);
		delete dockMngNodeDup;
	}
	else
	{
		insertDockingParamNode(newGUIRoot);
	}
}

bool NppParameters::writeFindHistory()
{
	if (!_pXmlUserDoc) return false;

	TiXmlNode *nppRoot = _pXmlUserDoc->FirstChild(TEXT("NotepadPlus"));
	if (!nppRoot)
	{
		nppRoot = _pXmlUserDoc->InsertEndChild(TiXmlElement(TEXT("NotepadPlus")));
	}

	TiXmlNode *findHistoryRoot = nppRoot->FirstChildElement(TEXT("FindHistory"));
	if (!findHistoryRoot)
	{
		TiXmlElement element(TEXT("FindHistory"));
		findHistoryRoot = nppRoot->InsertEndChild(element);
	}
	findHistoryRoot->Clear();

	(findHistoryRoot->ToElement())->SetAttribute(TEXT("nbMaxFindHistoryPath"),	_findHistory._nbMaxFindHistoryPath);
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("nbMaxFindHistoryFilter"),  _findHistory._nbMaxFindHistoryFilter);
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("nbMaxFindHistoryFind"),	_findHistory._nbMaxFindHistoryFind);
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("nbMaxFindHistoryReplace"), _findHistory._nbMaxFindHistoryReplace);

	(findHistoryRoot->ToElement())->SetAttribute(TEXT("matchWord"),				_findHistory._isMatchWord?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("matchCase"),				_findHistory._isMatchCase?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("wrap"),					_findHistory._isWrap?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("directionDown"),			_findHistory._isDirectionDown?TEXT("yes"):TEXT("no"));

	(findHistoryRoot->ToElement())->SetAttribute(TEXT("fifRecuisive"),			_findHistory._isFifRecuisive?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("fifInHiddenFolder"),		_findHistory._isFifInHiddenFolder?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("fifProjectPanel1"),	    	_findHistory._isFifProjectPanel_1?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("fifProjectPanel2"),	      	_findHistory._isFifProjectPanel_2?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("fifProjectPanel3"),	       	_findHistory._isFifProjectPanel_3?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("fifFilterFollowsDoc"),	_findHistory._isFilterFollowDoc?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("fifFolderFollowsDoc"),	_findHistory._isFolderFollowDoc?TEXT("yes"):TEXT("no"));

	(findHistoryRoot->ToElement())->SetAttribute(TEXT("searchMode"), _findHistory._searchMode);
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("transparencyMode"), _findHistory._transparencyMode);
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("transparency"), _findHistory._transparency);
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("dotMatchesNewline"),		_findHistory._dotMatchesNewline?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("isSearch2ButtonsMode"),		_findHistory._isSearch2ButtonsMode?TEXT("yes"):TEXT("no"));
	(findHistoryRoot->ToElement())->SetAttribute(TEXT("regexBackward4PowerUser"),		_findHistory._regexBackward4PowerUser ? TEXT("yes") : TEXT("no"));

	TiXmlElement hist_element{TEXT("")};

	hist_element.SetValue(TEXT("Path"));
	for (size_t i = 0, len = _findHistory._findHistoryPaths.size(); i < len; ++i)
	{
		(hist_element.ToElement())->SetAttribute(TEXT("name"), _findHistory._findHistoryPaths[i].c_str());
		findHistoryRoot->InsertEndChild(hist_element);
	}

	hist_element.SetValue(TEXT("Filter"));
	for (size_t i = 0, len = _findHistory._findHistoryFilters.size(); i < len; ++i)
	{
		(hist_element.ToElement())->SetAttribute(TEXT("name"), _findHistory._findHistoryFilters[i].c_str());
		findHistoryRoot->InsertEndChild(hist_element);
	}

	hist_element.SetValue(TEXT("Find"));
	for (size_t i = 0, len = _findHistory._findHistoryFinds.size(); i < len; ++i)
	{
		(hist_element.ToElement())->SetAttribute(TEXT("name"), _findHistory._findHistoryFinds[i].c_str());
		findHistoryRoot->InsertEndChild(hist_element);
	}

	hist_element.SetValue(TEXT("Replace"));
	for (size_t i = 0, len = _findHistory._findHistoryReplaces.size(); i < len; ++i)
	{
		(hist_element.ToElement())->SetAttribute(TEXT("name"), _findHistory._findHistoryReplaces[i].c_str());
		findHistoryRoot->InsertEndChild(hist_element);
	}

	return true;
}

void NppParameters::insertDockingParamNode(TiXmlNode *GUIRoot)
{
	TiXmlElement DMNode(TEXT("GUIConfig"));
	DMNode.SetAttribute(TEXT("name"), TEXT("DockingManager"));
	DMNode.SetAttribute(TEXT("leftWidth"), _nppGUI._dockingData._leftWidth);
	DMNode.SetAttribute(TEXT("rightWidth"), _nppGUI._dockingData._rightWidth);
	DMNode.SetAttribute(TEXT("topHeight"), _nppGUI._dockingData._topHeight);
	DMNode.SetAttribute(TEXT("bottomHeight"), _nppGUI._dockingData._bottomHight);

	for (size_t i = 0, len = _nppGUI._dockingData._flaotingWindowInfo.size(); i < len ; ++i)
	{
		FloatingWindowInfo & fwi = _nppGUI._dockingData._flaotingWindowInfo[i];
		TiXmlElement FWNode(TEXT("FloatingWindow"));
		FWNode.SetAttribute(TEXT("cont"), fwi._cont);
		FWNode.SetAttribute(TEXT("x"), fwi._pos.left);
		FWNode.SetAttribute(TEXT("y"), fwi._pos.top);
		FWNode.SetAttribute(TEXT("width"), fwi._pos.right);
		FWNode.SetAttribute(TEXT("height"), fwi._pos.bottom);

		DMNode.InsertEndChild(FWNode);
	}

	for (size_t i = 0, len = _nppGUI._dockingData._pluginDockInfo.size() ; i < len ; ++i)
	{
		PluginDlgDockingInfo & pdi = _nppGUI._dockingData._pluginDockInfo[i];
		TiXmlElement PDNode(TEXT("PluginDlg"));
		PDNode.SetAttribute(TEXT("pluginName"), pdi._name);
		PDNode.SetAttribute(TEXT("id"), pdi._internalID);
		PDNode.SetAttribute(TEXT("curr"), pdi._currContainer);
		PDNode.SetAttribute(TEXT("prev"), pdi._prevContainer);
		PDNode.SetAttribute(TEXT("isVisible"), pdi._isVisible?TEXT("yes"):TEXT("no"));

		DMNode.InsertEndChild(PDNode);
	}

	for (size_t i = 0, len = _nppGUI._dockingData._containerTabInfo.size(); i < len ; ++i)
	{
		ContainerTabInfo & cti = _nppGUI._dockingData._containerTabInfo[i];
		TiXmlElement CTNode(TEXT("ActiveTabs"));
		CTNode.SetAttribute(TEXT("cont"), cti._cont);
		CTNode.SetAttribute(TEXT("activeTab"), cti._activeTab);
		DMNode.InsertEndChild(CTNode);
	}

	GUIRoot->InsertEndChild(DMNode);
}

void NppParameters::writePrintSetting(TiXmlElement *element)
{
	const TCHAR *pStr = _nppGUI._printSettings._printLineNumber?TEXT("yes"):TEXT("no");
	element->SetAttribute(TEXT("lineNumber"), pStr);

	element->SetAttribute(TEXT("printOption"), _nppGUI._printSettings._printOption);

	element->SetAttribute(TEXT("headerLeft"), _nppGUI._printSettings._headerLeft.c_str());
	element->SetAttribute(TEXT("headerMiddle"), _nppGUI._printSettings._headerMiddle.c_str());
	element->SetAttribute(TEXT("headerRight"), _nppGUI._printSettings._headerRight.c_str());
	element->SetAttribute(TEXT("footerLeft"), _nppGUI._printSettings._footerLeft.c_str());
	element->SetAttribute(TEXT("footerMiddle"), _nppGUI._printSettings._footerMiddle.c_str());
	element->SetAttribute(TEXT("footerRight"), _nppGUI._printSettings._footerRight.c_str());

	element->SetAttribute(TEXT("headerFontName"), _nppGUI._printSettings._headerFontName.c_str());
	element->SetAttribute(TEXT("headerFontStyle"), _nppGUI._printSettings._headerFontStyle);
	element->SetAttribute(TEXT("headerFontSize"), _nppGUI._printSettings._headerFontSize);
	element->SetAttribute(TEXT("footerFontName"), _nppGUI._printSettings._footerFontName.c_str());
	element->SetAttribute(TEXT("footerFontStyle"), _nppGUI._printSettings._footerFontStyle);
	element->SetAttribute(TEXT("footerFontSize"), _nppGUI._printSettings._footerFontSize);

	element->SetAttribute(TEXT("margeLeft"), _nppGUI._printSettings._marge.left);
	element->SetAttribute(TEXT("margeRight"), _nppGUI._printSettings._marge.right);
	element->SetAttribute(TEXT("margeTop"), _nppGUI._printSettings._marge.top);
	element->SetAttribute(TEXT("margeBottom"), _nppGUI._printSettings._marge.bottom);
}

void NppParameters::writeExcludedLangList(TiXmlElement *element)
{
	int g0 = 0; // up to 8
	int g1 = 0; // up to 16
	int g2 = 0; // up to 24
	int g3 = 0; // up to 32
	int g4 = 0; // up to 40
	int g5 = 0; // up to 48
	int g6 = 0; // up to 56
	int g7 = 0; // up to 64
	int g8 = 0; // up to 72
	int g9 = 0; // up to 80
	int g10= 0; // up to 88
	int g11= 0; // up to 96
	int g12= 0; // up to 104

	const int groupNbMember = 8;

	for (size_t i = 0, len = _nppGUI._excludedLangList.size(); i < len ; ++i)
	{
		LangType langType = _nppGUI._excludedLangList[i]._langType;
		if (langType >= L_EXTERNAL && langType < L_END)
			continue;

		int nGrp = langType / groupNbMember;
		int nMask = 1 << langType % groupNbMember;


		switch (nGrp)
		{
			case 0 :
				g0 |= nMask;
				break;
			case 1 :
				g1 |= nMask;
				break;
			case 2 :
				g2 |= nMask;
				break;
			case 3 :
				g3 |= nMask;
				break;
			case 4 :
				g4 |= nMask;
				break;
			case 5 :
				g5 |= nMask;
				break;
			case 6 :
				g6 |= nMask;
				break;
			case 7 :
				g7 |= nMask;
				break;
			case 8:
				g8 |= nMask;
				break;
			case 9:
				g9 |= nMask;
				break;
			case 10:
				g10 |= nMask;
				break;
			case 11:
				g11 |= nMask;
				break;
			case 12:
				g12 |= nMask;
				break;
		}
	}

	element->SetAttribute(TEXT("gr0"), g0);
	element->SetAttribute(TEXT("gr1"), g1);
	element->SetAttribute(TEXT("gr2"), g2);
	element->SetAttribute(TEXT("gr3"), g3);
	element->SetAttribute(TEXT("gr4"), g4);
	element->SetAttribute(TEXT("gr5"), g5);
	element->SetAttribute(TEXT("gr6"), g6);
	element->SetAttribute(TEXT("gr7"), g7);
	element->SetAttribute(TEXT("gr8"), g8);
	element->SetAttribute(TEXT("gr9"), g9);
	element->SetAttribute(TEXT("gr10"), g10);
	element->SetAttribute(TEXT("gr11"), g11);
	element->SetAttribute(TEXT("gr12"), g12);
}

TiXmlElement * NppParameters::insertGUIConfigBoolNode(TiXmlNode *r2w, const TCHAR *name, bool bVal)
{
	const TCHAR *pStr = bVal?TEXT("yes"):TEXT("no");
	TiXmlElement *GUIConfigElement = (r2w->InsertEndChild(TiXmlElement(TEXT("GUIConfig"))))->ToElement();
	GUIConfigElement->SetAttribute(TEXT("name"), name);
	GUIConfigElement->InsertEndChild(TiXmlText(pStr));
	return GUIConfigElement;
}

int RGB2int(COLORREF color)
{
	return (((((DWORD)color) & 0x0000FF) << 16) | ((((DWORD)color) & 0x00FF00)) | ((((DWORD)color) & 0xFF0000) >> 16));
}

int NppParameters::langTypeToCommandID(LangType lt) const
{
	int id;
	switch (lt)
	{
		case L_C :
			id = IDM_LANG_C; break;
		case L_CPP :
			id = IDM_LANG_CPP; break;
		case L_JAVA :
			id = IDM_LANG_JAVA;	break;
		case L_CS :
			id = IDM_LANG_CS; break;
		case L_OBJC :
			id = IDM_LANG_OBJC;	break;
		case L_HTML :
			id = IDM_LANG_HTML;	break;
		case L_XML :
			id = IDM_LANG_XML; break;
		case L_JS :
		case L_JAVASCRIPT:
			id = IDM_LANG_JS; break;
		case L_JSON:
			id = IDM_LANG_JSON; break;
		case L_PHP :
			id = IDM_LANG_PHP; break;
		case L_ASP :
			id = IDM_LANG_ASP; break;
		case L_JSP :
			id = IDM_LANG_JSP; break;
		case L_CSS :
			id = IDM_LANG_CSS; break;
		case L_LUA :
			id = IDM_LANG_LUA; break;
		case L_PERL :
			id = IDM_LANG_PERL; break;
		case L_PYTHON :
			id = IDM_LANG_PYTHON; break;
		case L_BATCH :
			id = IDM_LANG_BATCH; break;
		case L_PASCAL :
			id = IDM_LANG_PASCAL; break;
		case L_MAKEFILE :
			id = IDM_LANG_MAKEFILE;	break;
		case L_INI :
			id = IDM_LANG_INI; break;
		case L_ASCII :
			id = IDM_LANG_ASCII; break;
		case L_RC :
			id = IDM_LANG_RC; break;
		case L_TEX :
			id = IDM_LANG_TEX; break;
		case L_FORTRAN :
			id = IDM_LANG_FORTRAN; break;
		case L_FORTRAN_77 :
			id = IDM_LANG_FORTRAN_77; break;
		case L_BASH :
			id = IDM_LANG_BASH; break;
		case L_FLASH :
			id = IDM_LANG_FLASH; break;
		case L_NSIS :
			id = IDM_LANG_NSIS; break;
		case L_USER :
			id = IDM_LANG_USER; break;
		case L_SQL :
			id = IDM_LANG_SQL; break;
		case L_VB :
			id = IDM_LANG_VB; break;
		case L_TCL :
			id = IDM_LANG_TCL; break;

		case L_LISP :
			id = IDM_LANG_LISP; break;
		case L_SCHEME :
			id = IDM_LANG_SCHEME; break;
		case L_ASM :
			id = IDM_LANG_ASM; break;
		case L_DIFF :
			id = IDM_LANG_DIFF; break;
		case L_PROPS :
			id = IDM_LANG_PROPS; break;
		case L_PS :
			id = IDM_LANG_PS; break;
		case L_RUBY :
			id = IDM_LANG_RUBY; break;
		case L_SMALLTALK :
			id = IDM_LANG_SMALLTALK; break;
		case L_VHDL :
			id = IDM_LANG_VHDL; break;

		case L_ADA :
			id = IDM_LANG_ADA; break;
		case L_MATLAB :
			id = IDM_LANG_MATLAB; break;

		case L_HASKELL :
			id = IDM_LANG_HASKELL; break;

		case L_KIX :
			id = IDM_LANG_KIX; break;
		case L_AU3 :
			id = IDM_LANG_AU3; break;
		case L_VERILOG :
			id = IDM_LANG_VERILOG; break;
		case L_CAML :
			id = IDM_LANG_CAML; break;

		case L_INNO :
			id = IDM_LANG_INNO; break;

		case L_CMAKE :
			id = IDM_LANG_CMAKE; break;

		case L_YAML :
			id = IDM_LANG_YAML; break;

		case L_COBOL :
			id = IDM_LANG_COBOL; break;

		case L_D :
			id = IDM_LANG_D; break;

		case L_GUI4CLI :
			id = IDM_LANG_GUI4CLI; break;

		case L_POWERSHELL :
			id = IDM_LANG_POWERSHELL; break;

		case L_R :
			id = IDM_LANG_R; break;

		case L_COFFEESCRIPT :
			id = IDM_LANG_COFFEESCRIPT; break;

		case L_BAANC:
			id = IDM_LANG_BAANC; break;

		case L_SREC :
			id = IDM_LANG_SREC; break;

		case L_IHEX :
			id = IDM_LANG_IHEX; break;

		case L_TEHEX :
			id = IDM_LANG_TEHEX; break;

		case L_SWIFT:
			id = IDM_LANG_SWIFT; break;

		case L_ASN1 :
			id = IDM_LANG_ASN1; break;

        case L_AVS :
			id = IDM_LANG_AVS; break;

		case L_BLITZBASIC :
			id = IDM_LANG_BLITZBASIC; break;

		case L_PUREBASIC :
			id = IDM_LANG_PUREBASIC; break;

		case L_FREEBASIC :
			id = IDM_LANG_FREEBASIC; break;

		case L_CSOUND :
			id = IDM_LANG_CSOUND; break;

		case L_ERLANG :
			id = IDM_LANG_ERLANG; break;

		case L_ESCRIPT :
			id = IDM_LANG_ESCRIPT; break;

		case L_FORTH :
			id = IDM_LANG_FORTH; break;

		case L_LATEX :
			id = IDM_LANG_LATEX; break;

		case L_MMIXAL :
			id = IDM_LANG_MMIXAL; break;

		case L_NIM :
			id = IDM_LANG_NIM; break;

		case L_NNCRONTAB :
			id = IDM_LANG_NNCRONTAB; break;

		case L_OSCRIPT :
			id = IDM_LANG_OSCRIPT; break;

		case L_REBOL :
			id = IDM_LANG_REBOL; break;

		case L_REGISTRY :
			id = IDM_LANG_REGISTRY; break;

		case L_RUST :
			id = IDM_LANG_RUST; break;

		case L_SPICE :
			id = IDM_LANG_SPICE; break;

		case L_TXT2TAGS :
			id = IDM_LANG_TXT2TAGS; break;

		case L_VISUALPROLOG:
			id = IDM_LANG_VISUALPROLOG; break;

		case L_TYPESCRIPT:
			id = IDM_LANG_TYPESCRIPT; break;

		case L_SEARCHRESULT :
			id = -1;	break;

		case L_TEXT :
			id = IDM_LANG_TEXT;	break;


		default :
			if (lt >= L_EXTERNAL && lt < L_END)
				id = lt - L_EXTERNAL + IDM_LANG_EXTERNAL;
			else
				id = IDM_LANG_TEXT;
	}
	return id;
}

generic_string NppParameters:: getWinVersionStr() const
{
	switch (_winVersion)
	{
		case WV_WIN32S: return TEXT("Windows 3.1");
		case WV_95: return TEXT("Windows 95");
		case WV_98: return TEXT("Windows 98");
		case WV_ME: return TEXT("Windows Millennium Edition");
		case WV_NT: return TEXT("Windows NT");
		case WV_W2K: return TEXT("Windows 2000");
		case WV_XP: return TEXT("Windows XP");
		case WV_S2003: return TEXT("Windows Server 2003");
		case WV_XPX64: return TEXT("Windows XP 64 bits");
		case WV_VISTA: return TEXT("Windows Vista");
		case WV_WIN7: return TEXT("Windows 7");
		case WV_WIN8: return TEXT("Windows 8");
		case WV_WIN81: return TEXT("Windows 8.1");
		case WV_WIN10: return TEXT("Windows 10");
		default: /*case WV_UNKNOWN:*/ return TEXT("Windows unknown version");
	}
}

generic_string NppParameters::getWinVerBitStr() const
{
	switch (_platForm)
	{
	case PF_X86:
		return TEXT("32-bit");

	case PF_X64:
	case PF_IA64:
	case PF_ARM64:
		return TEXT("64-bit");

	default:
		return TEXT("Unknown-bit");
	}
}

generic_string NppParameters::writeStyles(LexerStylerArray & lexersStylers, StyleArray & globalStylers)
{
	TiXmlNode *lexersRoot = (_pXmlUserStylerDoc->FirstChild(TEXT("NotepadPlus")))->FirstChildElement(TEXT("LexerStyles"));
	for (TiXmlNode *childNode = lexersRoot->FirstChildElement(TEXT("LexerType"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("LexerType")))
	{
		TiXmlElement *element = childNode->ToElement();
		const TCHAR *nm = element->Attribute(TEXT("name"));

		LexerStyler *pLs = _lexerStylerVect.getLexerStylerByName(nm);
		LexerStyler *pLs2 = lexersStylers.getLexerStylerByName(nm);

		if (pLs)
		{
			const TCHAR *extStr = pLs->getLexerUserExt();
			element->SetAttribute(TEXT("ext"), extStr);
			for (TiXmlNode *grChildNode = childNode->FirstChildElement(TEXT("WordsStyle"));
					grChildNode ;
					grChildNode = grChildNode->NextSibling(TEXT("WordsStyle")))
			{
				TiXmlElement *grElement = grChildNode->ToElement();
				const TCHAR *styleName = grElement->Attribute(TEXT("name"));
				const Style * pStyle = pLs->findByName(styleName);
				Style * pStyle2Sync = pLs2 ? pLs2->findByName(styleName) : nullptr;
				if (pStyle && pStyle2Sync)
				{
					writeStyle2Element(*pStyle, *pStyle2Sync, grElement);
				}
			}
		}
	}

	for (size_t x = 0; x < _pXmlExternalLexerDoc.size(); ++x)
	{
		TiXmlNode* lexersRoot2 = ( _pXmlExternalLexerDoc[x]->FirstChild(TEXT("NotepadPlus")))->FirstChildElement(TEXT("LexerStyles"));
		for (TiXmlNode* childNode = lexersRoot2->FirstChildElement(TEXT("LexerType"));
			childNode ;
			childNode = childNode->NextSibling(TEXT("LexerType")))
		{
			TiXmlElement *element = childNode->ToElement();
			const TCHAR *nm = element->Attribute(TEXT("name"));

			LexerStyler *pLs = _lexerStylerVect.getLexerStylerByName(nm);
			LexerStyler *pLs2 = lexersStylers.getLexerStylerByName(nm);

			if (pLs)
			{
				const TCHAR *extStr = pLs->getLexerUserExt();
				element->SetAttribute(TEXT("ext"), extStr);

				for (TiXmlNode *grChildNode = childNode->FirstChildElement(TEXT("WordsStyle"));
						grChildNode ;
						grChildNode = grChildNode->NextSibling(TEXT("WordsStyle")))
				{
					TiXmlElement *grElement = grChildNode->ToElement();
					const TCHAR *styleName = grElement->Attribute(TEXT("name"));
					const Style * pStyle = pLs->findByName(styleName);
					Style * pStyle2Sync = pLs2 ? pLs2->findByName(styleName) : nullptr;
					if (pStyle && pStyle2Sync)
					{
						writeStyle2Element(*pStyle, *pStyle2Sync, grElement);
					}
				}
			}
		}
		_pXmlExternalLexerDoc[x]->SaveFile();
	}

	TiXmlNode *globalStylesRoot = (_pXmlUserStylerDoc->FirstChild(TEXT("NotepadPlus")))->FirstChildElement(TEXT("GlobalStyles"));

	for (TiXmlNode *childNode = globalStylesRoot->FirstChildElement(TEXT("WidgetStyle"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("WidgetStyle")))
	{
		TiXmlElement *pElement = childNode->ToElement();
		const TCHAR *styleName = pElement->Attribute(TEXT("name"));
		const Style * pStyle = _widgetStyleArray.findByName(styleName);
		Style * pStyle2Sync = globalStylers.findByName(styleName);
		if (pStyle && pStyle2Sync)
		{
			writeStyle2Element(*pStyle, *pStyle2Sync, pElement);
		}
	}

	bool isSaved = _pXmlUserStylerDoc->SaveFile();
	if (!isSaved)
	{
		auto savePath = _themeSwitcher.getSavePathFrom(_pXmlUserStylerDoc->Value());
		if (!savePath.empty())
		{
			_pXmlUserStylerDoc->SaveFile(savePath.c_str());
			return savePath;
		}
	}
	return TEXT("");
}


bool NppParameters::insertTabInfo(const TCHAR *langName, int tabInfo)
{
	if (!_pXmlDoc) return false;
	TiXmlNode *langRoot = (_pXmlDoc->FirstChild(TEXT("NotepadPlus")))->FirstChildElement(TEXT("Languages"));
	for (TiXmlNode *childNode = langRoot->FirstChildElement(TEXT("Language"));
		childNode ;
		childNode = childNode->NextSibling(TEXT("Language")))
	{
		TiXmlElement *element = childNode->ToElement();
		const TCHAR *nm = element->Attribute(TEXT("name"));
		if (nm && lstrcmp(langName, nm) == 0)
		{
			childNode->ToElement()->SetAttribute(TEXT("tabSettings"), tabInfo);
			_pXmlDoc->SaveFile();
			return true;
		}
	}
	return false;
}

void NppParameters::writeStyle2Element(const Style & style2Write, Style & style2Sync, TiXmlElement *element)
{
	if (HIBYTE(HIWORD(style2Write._fgColor)) != 0xFF)
	{
		int rgbVal = RGB2int(style2Write._fgColor);
		TCHAR fgStr[7];
		wsprintf(fgStr, TEXT("%.6X"), rgbVal);
		element->SetAttribute(TEXT("fgColor"), fgStr);
	}

	if (HIBYTE(HIWORD(style2Write._bgColor)) != 0xFF)
	{
		int rgbVal = RGB2int(style2Write._bgColor);
		TCHAR bgStr[7];
		wsprintf(bgStr, TEXT("%.6X"), rgbVal);
		element->SetAttribute(TEXT("bgColor"), bgStr);
	}

	if (style2Write._colorStyle != COLORSTYLE_ALL)
	{
		element->SetAttribute(TEXT("colorStyle"), style2Write._colorStyle);
	}

	if (!style2Write._fontName.empty())
	{
		const TCHAR * oldFontName = element->Attribute(TEXT("fontName"));
		if (oldFontName && oldFontName != style2Write._fontName)
		{
			element->SetAttribute(TEXT("fontName"), style2Write._fontName);
			style2Sync._fontName = style2Write._fontName;
		}
	}

	if (style2Write._fontSize != STYLE_NOT_USED)
	{
		if (!style2Write._fontSize)
			element->SetAttribute(TEXT("fontSize"), TEXT(""));
		else
			element->SetAttribute(TEXT("fontSize"), style2Write._fontSize);
	}

	if (style2Write._fontStyle != STYLE_NOT_USED)
	{
		element->SetAttribute(TEXT("fontStyle"), style2Write._fontStyle);
	}


	if (!style2Write._keywords.empty())
	{
		TiXmlNode *teteDeNoeud = element->LastChild();

		if (teteDeNoeud)
			teteDeNoeud->SetValue(style2Write._keywords.c_str());
		else
			element->InsertEndChild(TiXmlText(style2Write._keywords.c_str()));
	}
}

void NppParameters::insertUserLang2Tree(TiXmlNode *node, UserLangContainer *userLang)
{
	TiXmlElement *rootElement = (node->InsertEndChild(TiXmlElement(TEXT("UserLang"))))->ToElement();

	TCHAR temp[32];
	generic_string udlVersion;
	udlVersion += generic_itoa(SCE_UDL_VERSION_MAJOR, temp, 10);
	udlVersion += TEXT(".");
	udlVersion += generic_itoa(SCE_UDL_VERSION_MINOR, temp, 10);

	rootElement->SetAttribute(TEXT("name"), userLang->_name);
	rootElement->SetAttribute(TEXT("ext"), userLang->_ext);
	if (userLang->_isDarkModeTheme)
		rootElement->SetAttribute(TEXT("darkModeTheme"), TEXT("yes"));
	rootElement->SetAttribute(TEXT("udlVersion"), udlVersion.c_str());

	TiXmlElement *settingsElement = (rootElement->InsertEndChild(TiXmlElement(TEXT("Settings"))))->ToElement();
	{
		TiXmlElement *globalElement = (settingsElement->InsertEndChild(TiXmlElement(TEXT("Global"))))->ToElement();
		globalElement->SetAttribute(TEXT("caseIgnored"),			userLang->_isCaseIgnored ? TEXT("yes"):TEXT("no"));
		globalElement->SetAttribute(TEXT("allowFoldOfComments"),	userLang->_allowFoldOfComments ? TEXT("yes"):TEXT("no"));
		globalElement->SetAttribute(TEXT("foldCompact"),			userLang->_foldCompact ? TEXT("yes"):TEXT("no"));
		globalElement->SetAttribute(TEXT("forcePureLC"),			userLang->_forcePureLC);
		globalElement->SetAttribute(TEXT("decimalSeparator"),	   userLang->_decimalSeparator);

		TiXmlElement *prefixElement = (settingsElement->InsertEndChild(TiXmlElement(TEXT("Prefix"))))->ToElement();
		for (int i = 0 ; i < SCE_USER_TOTAL_KEYWORD_GROUPS ; ++i)
			prefixElement->SetAttribute(globalMappper().keywordNameMapper[i+SCE_USER_KWLIST_KEYWORDS1], userLang->_isPrefix[i]?TEXT("yes"):TEXT("no"));
	}

	TiXmlElement *kwlElement = (rootElement->InsertEndChild(TiXmlElement(TEXT("KeywordLists"))))->ToElement();

	for (int i = 0 ; i < SCE_USER_KWLIST_TOTAL ; ++i)
	{
		TiXmlElement *kwElement = (kwlElement->InsertEndChild(TiXmlElement(TEXT("Keywords"))))->ToElement();
		kwElement->SetAttribute(TEXT("name"), globalMappper().keywordNameMapper[i]);
		kwElement->InsertEndChild(TiXmlText(userLang->_keywordLists[i]));
	}

	TiXmlElement *styleRootElement = (rootElement->InsertEndChild(TiXmlElement(TEXT("Styles"))))->ToElement();

	for (const Style & style2Write : userLang->_styles)
	{
		TiXmlElement *styleElement = (styleRootElement->InsertEndChild(TiXmlElement(TEXT("WordsStyle"))))->ToElement();

		if (style2Write._styleID == -1)
			continue;

		styleElement->SetAttribute(TEXT("name"), style2Write._styleDesc);

		//if (HIBYTE(HIWORD(style2Write._fgColor)) != 0xFF)
		{
			int rgbVal = RGB2int(style2Write._fgColor);
			TCHAR fgStr[7];
			wsprintf(fgStr, TEXT("%.6X"), rgbVal);
			styleElement->SetAttribute(TEXT("fgColor"), fgStr);
		}

		//if (HIBYTE(HIWORD(style2Write._bgColor)) != 0xFF)
		{
			int rgbVal = RGB2int(style2Write._bgColor);
			TCHAR bgStr[7];
			wsprintf(bgStr, TEXT("%.6X"), rgbVal);
			styleElement->SetAttribute(TEXT("bgColor"), bgStr);
		}

		if (style2Write._colorStyle != COLORSTYLE_ALL)
		{
			styleElement->SetAttribute(TEXT("colorStyle"), style2Write._colorStyle);
		}

		if (!style2Write._fontName.empty())
		{
			styleElement->SetAttribute(TEXT("fontName"), style2Write._fontName);
		}

		if (style2Write._fontStyle == STYLE_NOT_USED)
		{
			styleElement->SetAttribute(TEXT("fontStyle"), TEXT("0"));
		}
		else
		{
			styleElement->SetAttribute(TEXT("fontStyle"), style2Write._fontStyle);
		}

		if (style2Write._fontSize != STYLE_NOT_USED)
		{
			if (!style2Write._fontSize)
				styleElement->SetAttribute(TEXT("fontSize"), TEXT(""));
			else
				styleElement->SetAttribute(TEXT("fontSize"), style2Write._fontSize);
		}

		styleElement->SetAttribute(TEXT("nesting"), style2Write._nesting);
	}
}

void NppParameters::addUserModifiedIndex(size_t index)
{
	size_t len = _customizedShortcuts.size();
	bool found = false;
	for (size_t i = 0; i < len; ++i)
	{
		if (_customizedShortcuts[i] == index)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		_customizedShortcuts.push_back(index);
	}
}

void NppParameters::addPluginModifiedIndex(size_t index)
{
	size_t len = _pluginCustomizedCmds.size();
	bool found = false;
	for (size_t i = 0; i < len; ++i)
	{
		if (_pluginCustomizedCmds[i] == index)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		_pluginCustomizedCmds.push_back(index);
	}
}

void NppParameters::addScintillaModifiedIndex(int index)
{
	size_t len = _scintillaModifiedKeyIndices.size();
	bool found = false;
	for (size_t i = 0; i < len; ++i)
	{
		if (_scintillaModifiedKeyIndices[i] == index)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		_scintillaModifiedKeyIndices.push_back(index);
	}
}

void NppParameters::safeWow64EnableWow64FsRedirection(BOOL Wow64FsEnableRedirection)
{
	HMODULE kernel = GetModuleHandle(TEXT("kernel32"));
	if (kernel)
	{
		BOOL isWow64 = FALSE;
		typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
		LPFN_ISWOW64PROCESS IsWow64ProcessFunc = (LPFN_ISWOW64PROCESS) GetProcAddress(kernel,"IsWow64Process");

		if (IsWow64ProcessFunc)
		{
			IsWow64ProcessFunc(GetCurrentProcess(),&isWow64);

			if (isWow64)
			{
				typedef BOOL (WINAPI *LPFN_WOW64ENABLEWOW64FSREDIRECTION)(BOOL);
				LPFN_WOW64ENABLEWOW64FSREDIRECTION Wow64EnableWow64FsRedirectionFunc = (LPFN_WOW64ENABLEWOW64FSREDIRECTION)GetProcAddress(kernel, "Wow64EnableWow64FsRedirection");
				if (Wow64EnableWow64FsRedirectionFunc)
				{
					Wow64EnableWow64FsRedirectionFunc(Wow64FsEnableRedirection);
				}
			}
		}
	}
}

void NppParameters::setUdlXmlDirtyFromIndex(size_t i)
{
	for (auto& uxfs : _pXmlUserLangsDoc)
	{
		if (i >= uxfs._indexRange.first && i < uxfs._indexRange.second)
		{
			uxfs._isDirty = true;
			return;
		}
	}
}

/*
Considering we have done:
load default UDL:  3 languges
load a UDL file:   1 languge
load a UDL file:   2 languges
create a UDL:      1 languge
imported a UDL:    1 languge

the evolution to remove UDL one by one:

0[D1]                        0[D1]                        0[D1]                         [D1]                         [D1]
1[D2]                        1[D2]                        1[D2]                        0[D2]                         [D2]
2[D3]  [DUDL, <0,3>]         2[D3]  [DUDL, <0,3>]         2[D3]  [DUDL, <0,3>]         1[D3]  [DUDL, <0,2>]          [D3]  [DUDL, <0,0>]
3[U1]  [NUDL, <3,4>]         3[U1]  [NUDL, <3,4>]         3[U1]  [NUDL, <3,4>]         2[U1]  [NUDL, <2,3>]          [U1]  [NUDL, <0,0>]
4[U2]                        4[U2]                         [U2]                         [U2]                         [U2]
5[U2]  [NUDL, <4,6>]         5[U2]  [NUDL, <4,6>]         4[U2]  [NUDL, <4,5>]         3[U2]  [NUDL, <3,4>]         0[U2]  [NUDL, <0,1>]
6[C1]  [NULL, <6,7>]          [C1]  [NULL, <6,6>]          [C1]  [NULL, <5,5>]          [C1]  [NULL, <4,4>]          [C1]  [NULL, <1,1>]
7[I1]  [NULL, <7,8>]         6[I1]  [NULL, <6,7>]         5[I1]  [NULL, <5,6>]         4[I1]  [NULL, <4,5>]         1[I1]  [NULL, <1,2>]
*/
void NppParameters::removeIndexFromXmlUdls(size_t i)
{
	bool isUpdateBegin = false;
	for (auto& uxfs : _pXmlUserLangsDoc)
	{
		// Find index
		if (!isUpdateBegin && (i >= uxfs._indexRange.first && i < uxfs._indexRange.second)) // found it
		{
			if (uxfs._indexRange.second > 0)
				uxfs._indexRange.second -= 1;
			uxfs._isDirty = true;

			isUpdateBegin = true;
		}

		// Update
		else if (isUpdateBegin)
		{
			if (uxfs._indexRange.first > 0)
				uxfs._indexRange.first -= 1;
			if (uxfs._indexRange.second > 0)
				uxfs._indexRange.second -= 1;
		}
	}
}

void NppParameters::setUdlXmlDirtyFromXmlDoc(const TiXmlDocument* xmlDoc)
{
	for (auto& uxfs : _pXmlUserLangsDoc)
	{
		if (xmlDoc == uxfs._udlXmlDoc)
		{
			uxfs._isDirty = true;
			return;
		}
	}
}

Date::Date(const TCHAR *dateStr)
{
	// timeStr should be Notepad++ date format : YYYYMMDD
	assert(dateStr);
	int D = lstrlen(dateStr);

	if ( 8==D )
	{
		generic_string ds(dateStr);
		generic_string yyyy(ds, 0, 4);
		generic_string mm(ds, 4, 2);
		generic_string dd(ds, 6, 2);

		int y = generic_atoi(yyyy.c_str());
		int m = generic_atoi(mm.c_str());
		int d = generic_atoi(dd.c_str());

		if ((y > 0 && y <= 9999) && (m > 0 && m <= 12) && (d > 0 && d <= 31))
		{
			_year = y;
			_month = m;
			_day = d;
			return;
		}
	}
	now();
}

// The constructor which makes the date of number of days from now
// nbDaysFromNow could be negative if user want to make a date in the past
// if the value of nbDaysFromNow is 0 then the date will be now
Date::Date(int nbDaysFromNow)
{
	const time_t oneDay = (60 * 60 * 24);

	time_t rawtime;
	tm* timeinfo;

	time(&rawtime);
	rawtime += (nbDaysFromNow * oneDay);

	timeinfo = localtime(&rawtime);
	if (timeinfo)
	{
		_year = timeinfo->tm_year + 1900;
		_month = timeinfo->tm_mon + 1;
		_day = timeinfo->tm_mday;
	}
}

void Date::now()
{
	time_t rawtime;
	tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	if (timeinfo)
	{
		_year = timeinfo->tm_year + 1900;
		_month = timeinfo->tm_mon + 1;
		_day = timeinfo->tm_mday;
	}
}


EolType convertIntToFormatType(int value, EolType defvalue)
{
	switch (value)
	{
		case static_cast<LPARAM>(EolType::windows) :
			return EolType::windows;
		case static_cast<LPARAM>(EolType::macos) :
				return EolType::macos;
		case static_cast<LPARAM>(EolType::unix) :
			return EolType::unix;
		default:
			return defvalue;
	}
}
#endif // Mattes