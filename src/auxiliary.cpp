#include "auxiliary.hpp"

void cdecl debugPrintf(LPCWSTR lpFormat,...){
    XCHAR text[256];
    va_list arglist;
    va_start(arglist,lpFormat);
    wvsprintfW(text,lpFormat,arglist);
    va_end(arglist);
    OutputDebugStringW(text);
}

LPWSTR xlStrTocStr(LPCWSTR src) {
	if (src == NULL) return NULL;

	size_t len = src[0]; // 字符串长度
	LPWSTR dest = (LPWSTR)calloc(len + 1, sizeof(WCHAR)); // null-terminal
	if (dest == NULL) {
		return NULL;
	}
	errno_t res = wmemcpy_s(dest, len + 1, src + 1, len);

	if (res) return NULL; // 拷贝失败
	return dest;
}

LPWSTR cStrToxlStr(LPCWSTR src) {
	if (src == NULL) return NULL;
	size_t len = lstrlenW(src);
	if (len > 65535) return NULL;

	LPWSTR dest = (LPWSTR)calloc(len + 1, sizeof(WCHAR)); // byte-count string
	if (dest == NULL) {
		return NULL;
	}
	dest[0] = (WCHAR)len;
	errno_t res = wmemcpy_s(dest + 1, len, src, len);
	if (res) return NULL; // 拷贝失败
	return dest;
}


// wchar_t 16 bit
LPWSTR getWMemory(size_t bytes){
    return (LPWSTR)GetTempMemory(bytes*sizeof(WCHAR));
}

// char 8 bit
LPSTR getMemory(size_t bytes){
    return GetTempMemory(bytes);
}

void freeMemory(){
    FreeAllTempMemory();
}

int cdecl CqExcel(int xlfn,LPXLOPER12 pxRt,int count,...){
    int xlrt;
    va_list ppxArgs;
    va_start(ppxArgs,count);
    xlrt = Excel12v(xlfn,pxRt,count,(LPXLOPER12*)ppxArgs);
    va_end(ppxArgs);

    if (xlrt!=xlretSuccess){
        debugPrintf(L"error happen!!");

        // failed callback
        if (xlfn & xlCommand)
            debugPrintf(L"(%u) xlcommand call back is invalid\r",xlfn & 0x0FFF);
        if (xlfn & xlSpecial)
            debugPrintf(L"(%u) xlSpecial call back failed\r",xlfn & 0x0FFF);
        if (xlfn & xlIntl)
            debugPrintf(L"(%u) xlIntl call back failed\r",xlfn & 0x0FFF);
        if (xlfn & xlPrompt)
            debugPrintf(L"(%u) xlPrompt call back failed\r",xlfn & 0x0FFF);
        // some fatal error
        if (xlfn & xlretUncalced) debugPrintf(L"Uncalced cell\r");
        if (xlfn & xlretStackOvfl) debugPrintf(L"Stack Overflow\r");
        if (xlfn & xlretInvXloper) debugPrintf(L"Invalid XLOPER12\r");
        if (xlfn & xlretInvCount) debugPrintf(L"Invalid Number of Arguments\r");
        if (xlfn & xlretAbort) debugPrintf(L"Macro Halted\r");
    }
    freeMemory(); // reset offset value in mmamager
    return xlrt;
}

// memory of CqXXX function will be free by call freeMemory()
LPXLOPER12 CqNumber(double d){
    LPXLOPER12 lp;
    lp=(LPXLOPER12)getMemory(sizeof(XLOPER12));
    if (!lp) return 0;
    lp->xltype=xltypeNum;
    lp->val.num = d;
    return lp; 
}

// memory of CqXXX function will be free by call freeMemory()
LPXLOPER12 CqWString(LPCWSTR xlstr){
    LPXLOPER12 lp;
    XCHAR* lpd;
    int len = lstrlenW(xlstr);
    lp = (LPXLOPER12)getMemory(sizeof(XLOPER12)+((size_t)(len)+1)*2);
    if (!lp) return nullptr;

    lpd = (XCHAR*)((CHAR*)(lp)+sizeof(XLOPER12));

    lpd[0] = (WCHAR)len;

    wmemcpy_s(lpd+1,(size_t)(len)+1,xlstr,len);

    lp->xltype = xltypeStr;
    lp->val.str = lpd;
    return lp;
}

// memory of CqXXX function will be free by call freeMemory()
LPXLOPER12 CqBool(BOOL b){
    LPXLOPER12 lp;
    lp=(LPXLOPER12)getMemory(sizeof(XLOPER12));
    if (!lp) return 0;

    lp->xltype=xltypeBool;
    lp->val.xbool=b?1:0;
    return lp;
}

// type convert
LPXLOPER12 CqInteger(short i){
    LPXLOPER12 lp;
    lp=(LPXLOPER12)getMemory(sizeof(XLOPER12));
    if (!lp) return 0;

    lp->xltype = xltypeInt;
    lp->val.w = i;
    return lp;
}

LPXLOPER12 CqError(int i){
    LPXLOPER12 lp;
    lp=(LPXLOPER12)getMemory(sizeof(XLOPER12));
    if (!lp) return 0;

    lp->xltype = xltypeErr;
    lp->val.err = i;
    return lp;
}
//.............colFirst
//...............|
//......rwFirst->Xxxxxxxxxxxxxxx
//...............xxxxxxxxxxxxxxx
//...............xxxxxxxxxxxxxxx
//...............xxxxxxxxxxxxxxX<-rwLast
//.............................|
//..........................colLast
LPXLOPER12 CqActiveRange(RW rwFisrt,RW rwLast,COL colFirst,COL colLast,
LPCWSTR lpSheetName){
    LPXLOPER12 lp;
    LPXLOPER12 lpSheet;
    LPXLMREF12 lpmref;

    int wRet;
    lp = (LPXLOPER12)getMemory(sizeof(XLOPER12));
    lpmref = (LPXLMREF12)getMemory(sizeof(XLMREF12));
    if (!lpmref) return nullptr;

    if (lpSheetName){
        lpSheet = CqWString(lpSheetName);
        // there is no need to call xlFree to free the memory
        // get the idSheet
        wRet = Excel12(xlSheetId,lp,1,lpSheet);
    }
    else{
        wRet = Excel12(xlSheetId,lp,0);
    }
    if (wRet!=xlretSuccess) return nullptr;

    lp->xltype=xltypeRef;
    lp->val.mref.lpmref = lpmref;
    lpmref->count =1;
    lpmref->reftbl[0].rwFirst=rwFisrt;
    lpmref->reftbl[0].rwLast=rwLast;
    lpmref->reftbl[0].colFirst=colFirst;
    lpmref->reftbl[0].colLast=colLast;

    return lp;

}

LPXLOPER12 CqActiveCell(RW rw,COL col,LPCWSTR lpsheetName){
    return CqActiveRange(rw,rw,col,col,lpsheetName);
}

LPXLOPER12 CqActiveRow(RW rw,LPCWSTR lpsheetName){
    return CqActiveRange(rw,rw,0,0x00003FFF,lpsheetName);
}


LPXLOPER12 CqActiveColumn(COL col,LPCWSTR lpsheetName){
    return CqActiveRange(0,0x000FFFFF,col,col,lpsheetName);
}

LPXLOPER12 CqMissing(void){
    LPXLOPER12 lp;
    lp = (LPXLOPER12)getMemory(sizeof(XLOPER12));
    if (!lp) return lp;
    lp->xltype=xltypeMissing;
    return lp;
}

void InitMMamager(void){
    FreeAllTempMemory();
}

void QuitMMamager(void){
    FreeAllTempMemory();
}

void FreeXLOper(LPXLOPER12 pxloper){
    DWORD xltype;
    int cxloper;
    LPXLOPER12 pxloperFree;

    xltype = pxloper->xltype;
    switch (xltype)
    {
    case xltypeStr:
        if(pxloper->val.str!=NULL){
            free(pxloper->val.str);
            pxloper->val.str=NULL;
        }
        break;
    case xltypeRef:
        if (pxloper->val.mref.lpmref!=NULL){
            free(pxloper->val.mref.lpmref);
            pxloper->val.mref.lpmref=NULL;
        }
        break;
    case xltypeMulti:
        cxloper=pxloper->val.array.rows*pxloper->val.array.columns;
        if (pxloper->val.array.lparray){
            pxloperFree = pxloper->val.array.lparray;
            while (cxloper>0)
            {
                FreeXLOper(pxloperFree);
                pxloperFree++;
                cxloper--;
            }
            free(pxloper->val.array.lparray);
            pxloper->val.array.lparray = NULL;
        }
        break;
    case xltypeBigData:
        if (pxloper->val.bigdata.h.lpbData!=NULL){
            free(pxloper->val.bigdata.h.lpbData);
        }
        break;
    default:
        break;
    }
}


BOOL APIENTRY DLLMain(HANDLE hDLL,DWORD dwReason,LPVOID lpReserved){
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    // some code
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }
    return TRUE;
}

int WINAPI xlAutoOpen(void){
    XLOPER12 xDLL;
    int cq;
    CqExcel(xlGetName,&xDLL,0);// get the name of xll
    for (cq=0; cq < worksheetFuncsNum; cq++){
        CqExcel(xlfRegister,0,1+RegisterItems,
        &xDLL,
        CqWString(worksheetFuncsInfo[cq][0]),
        CqWString(worksheetFuncsInfo[cq][1]),
        CqWString(worksheetFuncsInfo[cq][2]),
        CqWString(worksheetFuncsInfo[cq][3]),
        CqWString(worksheetFuncsInfo[cq][4]),
        CqWString(worksheetFuncsInfo[cq][5]),
        CqWString(worksheetFuncsInfo[cq][6]),
        CqWString(worksheetFuncsInfo[cq][7]),
        CqWString(worksheetFuncsInfo[cq][8]),
        CqWString(worksheetFuncsInfo[cq][9]));
        FreeAllTempMemory();
    }
    CqExcel(xlFree,0,1,&xDLL);
    return 1; // all xlfunction must return 1
}


int WINAPI xlAutoClose(void){
    int cq;
    for ( cq = 0; cq < worksheetFuncsNum; cq++)
    {
        CqExcel(xlfSetName,0,1,CqWString(worksheetFuncsInfo[cq][2]));
        FreeAllTempMemory();
    }
    return 1;
}

// return 0 if s is equal with t
int lpwstricmp(LPCWSTR s,LPCWSTR t){
    int cq;
    size_t len = wcslen(s);
    if (len!=*t) return 1;

    for ( cq = 1; cq <= len; cq++)
    {
        if (towlower(s[cq-1])!=towlower(t[cq]))
        {
            return 1;
        } 
    }
    return 0;
}

LPXLOPER12 WINAPI xlAutoRegister12(LPXLOPER12 pxName){
    static XLOPER12 xDLL{},xRegId{};
    int cq;
    xRegId.xltype = xltypeErr;
    xRegId.val.err = xlerrValue;
    int rt;

    for(cq=0;cq<worksheetFuncsNum;cq++){
        if (!lpwstricmp(worksheetFuncsInfo[cq][0],
        pxName->val.str)){
            rt = CqExcel(xlfRegister,0,1+RegisterItems,
                &xDLL,
                CqWString(worksheetFuncsInfo[cq][0]),
                CqWString(worksheetFuncsInfo[cq][1]),
                CqWString(worksheetFuncsInfo[cq][2]),
                CqWString(worksheetFuncsInfo[cq][3]),
                CqWString(worksheetFuncsInfo[cq][4]),
                CqWString(worksheetFuncsInfo[cq][5]),
                CqWString(worksheetFuncsInfo[cq][6]),
                CqWString(worksheetFuncsInfo[cq][7]),
                CqWString(worksheetFuncsInfo[cq][8]),
                CqWString(worksheetFuncsInfo[cq][9])
            );
            CqExcel(xlFree,0,1,&xDLL);
            xRegId.xltype = xltypeNum;
            xRegId.val.num = rt;
            return &xRegId;
        }
    }
    return &xRegId;
}

int WINAPI xlAutoAdd(void){
    // Alert is macro sheet function
    CqExcel(xlcAlert,0,2,CqWString(CqXll_infr(__DATE__,__TIME__)),CqNumber(2));
    return 1;
}

int WINAPI xlAutoRemove(void){
    CqExcel(xlcAlert,0,2,CqWString(CqXll_remove),CqNumber(2));
    return 1;
}

LPXLOPER12 WINAPI xlAddInManagerInfo12(LPXLOPER12 xAction){
    LPXLOPER12 pxInfo = (LPXLOPER12)malloc(sizeof(XLOPER12));
    XLOPER12 xIntAction;

    CqExcel(xlCoerce,&xIntAction,2,xAction,CqInteger(xltypeInt));

    if (xIntAction.val.w == 1){
        pxInfo->xltype = xltypeStr;
        pxInfo->val.str = cStrToxlStr(L"CqXll Addins");
    }
    else {
        pxInfo->xltype = xltypeErr;
        pxInfo->val.err = xlerrValue;
    }
    CqExcel(xlFree,0,1,&xIntAction);
    pxInfo->xltype |= xlbitDLLFree;
    return pxInfo;
}

int WINAPI xlAutoFree12(LPXLOPER12 pxFree){
    FreeXLOper(pxFree);
    free(pxFree);
    return 1;
}