/*  CUEKSUP.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs support for registered keyboard replacements
 */


#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



VOID setinsert(psg, insert, hndl)
NPSG psg;
USHORT insert;
HKBD hndl;
{
    KBDINFO kdata;
    VIOCURSORINFO cd;

    VioGetCurType(&cd, VIO_HANDLE);

    kdata.cb = sizeof(kdata);
    KbdGetStatus(&kdata, hndl);

    cd.yStart = 1;
    if (psg->cueflags & CUESTATE_BIGCUR)
	cd.yStart = cd.cEnd-1;

    kdata.fsMask |= KBDMODE_SSTATE;
    if (insert)
	kdata.fsState |= SSTATE_INSERT;
    else {
	cd.yStart = cd.cEnd-cd.yStart;
	kdata.fsState &= ~SSTATE_INSERT;
    }
    VioSetCurType(&cd, VIO_HANDLE);

    KbdSetStatus(&kdata, hndl);
}


VOID homeline(maxcol, vpos)
register USHORT maxcol;
USHORT *vpos;
{
    USHORT row, col;
    register USHORT pos = *vpos;

    VioGetCurPos(&row, &col, VIO_HANDLE);
    while (pos > col)
	if (row) {
	    pos -= (col+1);
	    row--;
	    col = maxcol;
	}
	else
	    col = pos;
    col -= pos;
    VioSetCurPos(row, col, VIO_HANDLE);
    *vpos = 0;
}


VOID endline(maxcol, vcount, vpos)
register USHORT maxcol;
USHORT vcount, *vpos;
{
    USHORT row, col;
    register USHORT pos = vcount-*vpos;

    VioGetCurPos(&row, &col, VIO_HANDLE);
    col = ++maxcol-col;
    while (pos > col) {
	pos -= col;
	row++;
	col = maxcol;
    }
    col = maxcol-col + pos;
    VioSetCurPos(row, col, VIO_HANDLE);
    *vpos = vcount;
}


VOID eraseline(vpos, vcount)
USHORT vpos, *vcount;
{
    CHAR space = ' ';
    USHORT row, col;

    VioGetCurPos(&row, &col, VIO_HANDLE);
    VioWrtNChar(&space, *vcount-vpos, row, col, VIO_HANDLE);
    *vcount = vpos;
}


VOID addchar(c, pcount, vpos, vcount)
UCHAR c;
USHORT *pcount, *vpos, *vcount;
{
    CHAR caret = '^';
    USHORT trow, tcol;
    register USHORT add = (*vpos == *vcount);

    if (c == '\t') {
	VioGetCurPos(&trow, &tcol, VIO_HANDLE);
	tcol = (tcol & ~(TABSTOP-1)) + TABSTOP - tcol;
	*vpos += tcol;
	if (add)
	    *vcount += tcol;
    }
    else {
	(*vpos)++;
	if (add)
	    (*vcount)++;
	if (c < ' ') {
	    VioWrtTTY(&caret, 1, VIO_HANDLE);
	    (*vpos)++;
	    if (add)
		(*vcount)++;
	    c += 'A'-1;
	}
    }
    VioWrtTTY(&c, 1, VIO_HANDLE);
    if (add)
	(*pcount)++;
}


VOID backup(del, maxcol)
register USHORT del, maxcol;
{
    USHORT row, col;
    static CHAR bs[] = {'\b', ' ', '\b'};

    VioGetCurPos(&row, &col, VIO_HANDLE);
    if (!col) {
	VioSetCurPos(--row, maxcol, VIO_HANDLE);
	if (del == 3) {
	    VioWrtTTY(bs+1, 1, VIO_HANDLE);
	    VioSetCurPos(row, maxcol, VIO_HANDLE);
	}
    }
    else
	VioWrtTTY(bs, del, VIO_HANDLE);
}


VOID subchar(currentline, ppos, del, scol, maxcol, vpos, vcount)
PUCHAR currentline;
USHORT ppos, del, scol, maxcol, *vpos, *vcount;
{
    register USHORT tpos, tcol;

    if (del)
	del = 3;
    else
	del = 1;
    if (currentline[ppos] == '\t') {
	tcol = scol;
	for (tpos=0; tpos < ppos; tpos++)
	    if (currentline[tpos] == '\t')
		tcol = (tcol & ~(TABSTOP-1)) + TABSTOP;
	    else if (currentline[tpos] < ' ')
		tcol += 2;
	    else
		tcol++;
	tcol = *vpos - (tcol - scol);
	*vpos -= tcol;
	if (del == 3)
	    *vcount -= tcol;
	while (tcol--)
	    backup(1, maxcol);
    }
    else {
	if (currentline[ppos] < ' ') {
	    backup(del, maxcol);
	    (*vpos)--;
	    if (del == 3)
		(*vcount)--;
	}
	backup(del, maxcol);
	(*vpos)--;
	if (del == 3)
	    (*vcount)--;
    }
}


VOID multchar(currentline, count, pcount, vpos, vcount)
PUCHAR currentline;
USHORT count;
USHORT *pcount, *vpos, *vcount;
{
    USHORT t, tcol;
    register UCHAR c;
    register USHORT i = 0, j = 0;

    if (!count)
	return;
    LockSem(&Buffsem);
    VioGetCurPos(&t, &tcol, VIO_HANDLE);
    while (count--) {
	c = currentline[i++];
	if (c == '\t') {
	    t = (tcol & ~(TABSTOP-1)) + TABSTOP - tcol;
	    *vcount += t;
	    *vpos += t;
	    tcol += t;
	}
	else {
	    (*vcount)++;
	    (*vpos)++;
	    tcol++;
	    if (c < ' ') {
		Tmpbuff[j++] = '^';
		(*vcount)++;
		(*vpos)++;
		tcol++;
		c += 'A'-1;
	    }
	}
	Tmpbuff[j++] = c;
    }
    *pcount += i;
    VioWrtTTY(Tmpbuff, j, VIO_HANDLE);
    UnLockSem(&Buffsem);
}


VOID replacechar(currentline, ppos, c, psg, pcount, vpos, vcount)
PUCHAR currentline;
UCHAR c;
register NPSG psg;
USHORT ppos, pcount, vpos, *vcount;
{
    USHORT trow, tcol;
    register USHORT tpos = ppos;
    USHORT oldcount = *vcount, prevcount;

    if (c)
	currentline[ppos] = c;
    else
	while (tpos <= pcount) {
	    currentline[tpos] = currentline[tpos+1];
	    tpos++;
	}
    VioGetCurPos(&trow, &tcol, VIO_HANDLE);
    prevcount = *vcount = vpos;
    multchar(currentline+ppos, pcount-ppos, &pcount, &vpos, vcount);
    if (oldcount > vpos)
	eraseline(vpos, &oldcount);
    tpos = trow;
    while (tcol + (*vcount-prevcount) > psg->viomaxcol) {
	tpos++;
	prevcount += psg->viomaxcol;
    }
    if (tpos > psg->viomaxrow)
	trow -= (tpos - psg->viomaxrow);
    VioSetCurPos(trow, tcol, VIO_HANDLE);
}


VOID replaceline(currentline, count, maxcol, pcount, vpos, vcount)
PUCHAR currentline;
USHORT count, maxcol;
USHORT *pcount, *vpos, *vcount;
{
    USHORT tcount = *vcount;

    *pcount = *vcount = 0;
    homeline(maxcol, vpos);
    multchar(currentline, count, pcount, vpos, vcount);
    if (tcount > *vcount)
	eraseline(*vpos, &tcount);
}
