/*  CUEVIO.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cue video routines
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



USHORT savebox(psg, ulr, ulc, lrr, lrc, ppbuff)
NPSG psg;
PCHAR *ppbuff;
register USHORT ulr, ulc, lrr, lrc;
{
    PUSHORT p;
    USHORT length;
    VIOINTENSITY intense;
    VIOCURSORINFO cdata;

    *(USHORT *)ppbuff = 0;
    if (DosAllocSeg(SAVEHDR+(lrr-ulr+1)*(lrc-ulc+1)*2, (PUSHORT)ppbuff+1, 0))
	return(FALSE);
    p = (PUSHORT)*ppbuff;
    *p++ = ulr;
    *p++ = ulc;
    *p++ = lrr;
    *p++ = lrc;
    VioGetCurPos(p, p+1, VIO_HANDLE);
    p += 3;			    /* skip 2 for pos, 1 for unused pop info */
    VioGetCurType((PVIOCURSORINFO)p, VIO_HANDLE);
    p += sizeof(VIOCURSORINFO)/2;
    ((PPALSTATE)(p))->cb = sizeof(PALSTATE);
    ((PPALSTATE)(p))->type = 0;     /* get palette state */
    ((PPALSTATE)(p))->iFirst = 0;
    if (psg->cueflags & CUESTATE_SETSTATE)
	VioGetState(p, VIO_HANDLE);
    p += sizeof(PALSTATE)/2;
    ((PVIOINTENSITY)(p))->cb = sizeof(VIOINTENSITY);
    ((PVIOINTENSITY)(p))->type = 2; /* get intensity/blink state */
    if (psg->cueflags & CUESTATE_SETSTATE) {
	VioGetState(p, VIO_HANDLE);
	intense.cb = sizeof(VIOINTENSITY);
	intense.type = 2;
	intense.fs = 1; 	    /* force to intensity state */
	VioSetState(&intense, VIO_HANDLE);
    }
    p += sizeof(VIOINTENSITY)/2;
    cdata.yStart = 0;
    cdata.cEnd = 0;
    cdata.cx = 1;
    cdata.attr = 0xffff;
    VioSetCurType(&cdata, VIO_HANDLE);
    length = (lrc-ulc+1) * 2;
    while (ulr <= lrr) {
	VioReadCellStr((PCHAR)p, &length, ulr++, ulc, VIO_HANDLE);
	(PCHAR)p += length;
    }
    return(TRUE);
}


VOID restorebox(psg, pbuff)
NPSG psg;
PCHAR pbuff;
{
    PUSHORT p;
    register USHORT ulr, ulc, lrr, lrc, row, col, length;

    p = (PUSHORT)pbuff;
    ulr = *p++;
    ulc = *p++;
    lrr = *p++;
    lrc = *p++;
    row = *p++;
    col = *p++;
    p += 1 + sizeof(VIOCURSORINFO)/2 + sizeof(PALSTATE)/2 + sizeof(VIOINTENSITY)/2;
    length = (lrc-ulc+1) * 2;
    while (ulr <= lrr) {
	VioWrtCellStr((PCHAR)p, length, ulr++, ulc, VIO_HANDLE);
	(PCHAR)p += length;
    }
    VioSetCurPos(row, col, VIO_HANDLE);
    pbuff += sizeof(POPDATA);
    VioSetCurType((PVIOCURSORINFO)pbuff, VIO_HANDLE);
    if (psg->cueflags & CUESTATE_SETSTATE) {
	pbuff += sizeof(VIOCURSORINFO);
	VioSetState(pbuff, VIO_HANDLE); /* restore palette state */
	pbuff += sizeof(PALSTATE);
	VioSetState(pbuff, VIO_HANDLE); /* restore intensity state */
    }
    DosFreeSeg(FP_SEG(pbuff));
}


VOID drawbox(ulr, ulc, lrr, lrc, style, color)
UCHAR color;
USHORT ulr, ulc, lrr, lrc, style;
{
    UCHAR cell[2];
    UCHAR filler[2];
    USHORT id;
    register USHORT i;
    static UCHAR horizontal[] = {196, 205};
    static UCHAR vertical[]   = {179, 186};
    static UCHAR upperleft[]  = {218, 214, 213, 201};
    static UCHAR upperright[] = {191, 183, 184, 187};
    static UCHAR lowerleft[]  = {192, 211, 212, 200};
    static UCHAR lowerright[] = {217, 189, 190, 188};

    VioGetCp(0, &id, VIO_HANDLE);
    if (id != CODEPAGE_US)  /* if U.S. codepage not in use */
	style = 0;	    /* then force box style to thin-line variety */
    cell[1] = color;
    if (ulr != lrr) {
	cell[0] = upperleft[style];
	VioWrtNCell(cell, 1, ulr, ulc, VIO_HANDLE);
	cell[0] = horizontal[style >> 1];
	VioWrtNCell(cell, lrc-ulc-1, ulr, ulc+1, VIO_HANDLE);
	cell[0] = upperright[style];
	VioWrtNCell(cell, 1, ulr, lrc, VIO_HANDLE);

	filler[0] = ' ';
	filler[1] = color;
	cell[0] = vertical[style & THICK_VERT];
	for (i=ulr+1; i<lrr; i++) {
	    VioWrtNCell(cell, 1, i, ulc, VIO_HANDLE);
	    /* if (color & ((BRIGHT+WHITE)*BCKGRD)) */
		VioWrtNCell(filler, lrc-ulc-1, i, ulc+1, VIO_HANDLE);
	    VioWrtNCell(cell, 1, i, lrc, VIO_HANDLE);
	}
    }
    cell[0] = lowerleft[style];
    VioWrtNCell(cell, 1, lrr, ulc, VIO_HANDLE);
    cell[0] = horizontal[style >> 1];
    VioWrtNCell(cell, lrc-ulc-1, lrr, ulc+1, VIO_HANDLE);
    cell[0] = lowerright[style];
    VioWrtNCell(cell, 1, lrr, lrc, VIO_HANDLE);
}


/*  printstr - print string, optionally centered and/or padded with spaces
 *
 *  ENTRY   row     row to print string
 *	    col     column to print string
 *	    width   width of area in which string must fit (zero if
 *		    unlimited, negative if no padding needed)
 *	    center  TRUE if string is to be centered within field
 *	    src     far ptr to source string
 *	    dest    far ptr to destination string, NULL if none
 *
 *  EXIT    none
 */

USHORT printstr(row, col, width, center, src, dest)
SHORT row, col;
register SHORT width;
BOOL center;
PCHAR src, dest;
{
    USHORT j;
    register USHORT i, k;
    CHAR space = ' ', nopad = FALSE;
    CHAR front, back;

    i = strlen(src);
    if (width == 0)
	width = i;		/* width is as big as string, exactly */
    else if (width < 0) {
	nopad++;
	width *= -1;
    }
    if (i > width)
	i = width;		/* make sure string is within width */
    if (center) {
	j = (width-i)/2;	/* amount of whitespace for left side */
	if (!nopad)
	    if (!dest)
		VioWrtNChar(&space, j, row, col, VIO_HANDLE);
	    else
		for (k=j; k; k--)
		    *dest++ = space;
	col += j;
	width -= j;
    }
    if (!dest) {
	VioGetCp(0, &j, VIO_HANDLE);
	if (j != CODEPAGE_US) {
	  front = *src;
	  back = *(src+i-1);
	  if (front == '\xB5')
	    *src = '\xB4';
	  if (back == '\xC6')
	    *(src+i-1) = '\xC3';
	}
	VioWrtCharStr(src, i, row, col, VIO_HANDLE);
	if (j != CODEPAGE_US) {
	  *src = front;
	  *(src+i-1) = back;
	}
    }
    else
	for (k=i; k; k--)
	    *dest++ = *src++;
    if (width > i && !nopad)	/* pad rest of width (if any) with spaces */
	if (!dest)
	    VioWrtNChar(&space, width-i, row, col+i, VIO_HANDLE);
	else
	    for (k=width-i; k; k--)
		*dest++ = space;
    if (!dest)
	VioSetCurPos(row, col+i, VIO_HANDLE);
    else
       *dest = 0;
    return(i);
}


/*  printnum - print number, optionally padded with spaces
 *
 *  ENTRY   row     row to print string
 *	    col     column to print string
 *	    width   width of area in which number must fit
 *	    typ     0 if DEC, 1 if DEC with spaces, 2 if HEX
 *	    i	    number to print
 *	    dest    far ptr to destination string, NULL if none
 *
 *  EXIT    none
 */

VOID printnum(row, col, width, typ, i, dest)
SHORT row, col, width;
BOOL typ;
USHORT i;
PCHAR dest;
{
    CHAR s[6];
    register USHORT d = 10, j = 6, k;

    if (!width)
	width = 6;
    if (typ == 2)
	d = 16;
    s[--j] = 0;
    while (i && width) {
	k = i/d;
	i -= k*d;
	width--;
	if (i >= 10)
	    s[--j] = (CHAR)('A' + i-10);
	else
	    s[--j] = (CHAR)('0' + i);
	i = k;
    }
    if (j == 5)
	s[--j] = '0';
    while (j && width--)
	if (typ == 1)
	  s[--j] = ' ';
	else
	  s[--j] = '0';
    printstr(row, col, 0, FALSE, s+j, dest);
}
