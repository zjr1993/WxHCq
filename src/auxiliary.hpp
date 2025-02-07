#pragma once
#include <xlcall.h>
#include <malloc.h>
#include <mmanager.hpp>
#include <stdarg.h>
#include <wchar.h>
#define _c(type) const_cast<type>
#define CqXll_infr(date,time) _infr(date,time)
#define _infr(date,time) L"(●'△'●)感谢老板试用CqXll插件\n构建日期 "#date"时间 "#time"\n编译器 Clang14.0.5(MSVC CLI)-amd64"
#define CqXll_remove L"成功卸载CqXll插件($_$)"

#define RegisterItems 10
static LPCWSTR worksheetFuncsInfo[][RegisterItems]={
    /* example==>{
            L"FunctionName",
            L"returnVaule|arguments",
            L"FunctionName",
            L"ArgHits",
            L"macro_type",
            L"category",
            L"",
            L"",
            L"function help",
            L"argument help"
        } */
#include "register.txt"
};

void cdecl debugPrintf(LPCWSTR lpFormat,...);
LPWSTR getWMemory(size_t bytes);
LPSTR getMemory(size_t bytes);
void freeMemory();
int cdecl CqExcel(int xlfn,LPXLOPER12 pxRt,int count,...);
LPXLOPER12 CqNumber(double d);
LPXLOPER12 CqWString(LPCWSTR xlstr);
LPXLOPER12 CqBool(BOOL b);
LPXLOPER12 CqInteger(short i);
LPXLOPER12 CqError(int i);
LPXLOPER12 CqActiveRange(RW rwFisrt,RW rwLast,COL colFirst,COL colLast,LPCWSTR lpSheetName=nullptr);
LPXLOPER12 CqActiveCell(RW rw,COL col,LPCWSTR lpsheetName=nullptr);
LPXLOPER12 CqActiveRow(RW rw,LPCWSTR lpsheetName=nullptr);
LPXLOPER12 CqActiveColumn(COL col,LPCWSTR lpsheetName=nullptr);
LPXLOPER12 CqMissing(void);
void InitMMamager(void);
void QuitMMamager(void);
void FreeXLOper(LPXLOPER12 pxloper);
LPWSTR xlStrTocStr(LPCWSTR src);
LPWSTR cStrToxlStr(LPCWSTR src);
static size_t worksheetFuncsNum = sizeof(worksheetFuncsInfo)/(RegisterItems*sizeof(LPWSTR));