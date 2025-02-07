#include "auxiliary.hpp"

LPXLOPER12 WINAPI CqRangeType(LPXLOPER12 range){
    static XLOPER12 rt;
    rt.xltype=xltypeStr;
    DWORD xltype = range->xltype;
    switch (xltype)
    {
    case xltypeStr:
        rt.val.str = cStrToxlStr(L"值类型|xltypeStr");
        break;
    case xltypeRef:
        rt.val.str = cStrToxlStr(L"引用类型|xltypeRef");
        break;
    case xltypeNum:
        rt.val.str = cStrToxlStr(L"值类型|xltypeNum11");
        break;
    case xltypeErr:
        rt.val.str = cStrToxlStr(L"值类型|xltypeErr");
        break;
    case xltypeNil:
        rt.val.str = cStrToxlStr(L"值类型|xltypeNil");
        break;
    case xltypeSRef:
        rt.val.str = cStrToxlStr(L"引用类型|xltypeSRef");
        break;
    case xltypeMissing:
        rt.val.str = cStrToxlStr(L"值类型|xltypeMissing");
        break;
    case xltypeMulti:
        rt.val.str = cStrToxlStr(L"值类型|xltypeMulti");
        break;
    default:
        rt.val.str = cStrToxlStr(L"不重要类型");
        break;
    }
    return &rt;
}

short WINAPI CqSetValue(short cq){
    XLOPER12 xValue;
    // xRef.xltype = xltypeSRef;
    // xRef.val.sref.count=1;
    // xRef.val.sref.ref.rwFirst=9;
    // xRef.val.sref.ref.rwLast=11;
    // xRef.val.sref.ref.colFirst=1;
    // xRef.val.sref.ref.colLast=1;
    xValue.xltype = xltypeInt;
    xValue.val.w = cq;
    Excel12(xlSet,0,2,CqActiveRange(1,2,1,2,L"Sheet1"),&xValue);
    return 1;
}

LPXLOPER12 WINAPI fArray(LPXLOPER12 rng){
    size_t num = 0;
    LPXLOPER12 ptrArr=nullptr;
    LPXLOPER12 rest = (LPXLOPER12)malloc(sizeof(XLOPER12));

    rest->xltype = xltypeErr;
    rest->val.err = xlerrValue;


    if (rng->xltype == xltypeMulti){
        num = rng->val.array.columns * rng->val.array.rows;
        ptrArr = rng->val.array.lparray;

        rest->val.array.lparray = (LPXLOPER12)malloc(num * sizeof(XLOPER12));
        rest->xltype = xltypeMulti | xlbitDLLFree;
        rest->val.array.columns = rng->val.array.columns;
        rest->val.array.rows = rng->val.array.rows;

        for (size_t cq = 0; cq < num; cq++)
        {
            rest->val.array.lparray[cq].xltype = xltypeNum;
            if ((ptrArr[cq]).xltype & xltypeNum){
                rest->val.array.lparray[cq].val.num = ptrArr[cq].val.num *  ptrArr[cq].val.num;
            }
            else {
                rest->val.array.lparray[cq].val.num = 0;
            }
        }
    }
    return rest;
}