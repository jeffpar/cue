/*  CUEFIND.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs pop-up find functions
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"


/*  findhelp - help string retriever
 *
 *  ENTRY   cmd       retrieval command
 *	    input     additional input
 *	    pref      address of reference data
 *
 *  EXIT    TRUE if successful, FALSE if error
 */

USHORT FAR findhelp(cmd, input, pref)
USHORT cmd;
SHORT input;
register POPFNDATA *pref;
{
    USHORT rc = FALSE;
    register SHORT count, len;

    switch(cmd) {

    case POPCMD_INIT:
	pref->index = 0;
	pref->dataptr = Helphelp;

    case POPCMD_NEAR:
	if (input < 0)
	    input = 0;

    case POPCMD_FIND:
	if (input < 0)
	    break;
	rc = TRUE;
	count = input - pref->index;
	if (count) {
	  if (input == 0)
	    rc = findhelp(POPCMD_INIT, 0, pref);
	  else {
	    if (count < 0) {
	      count = -count;
	      while (count--)
		if (pref->index) {
		  pref->index--;
		  pref->dataptr = Helphelp + CueStrPrev(Helphelp, (PCHAR)(pref->dataptr)-Helphelp);
		}
		else
		  break;
	    }
	    else {
	      while (count--)
		if (*(PCHAR)(pref->dataptr)) {
		  len = strlen(pref->dataptr)+1;
		  if (*((PCHAR)(pref->dataptr) + len)) {
		    pref->index++;
		    (PCHAR)(pref->dataptr) += len;
		  }
		  else
		    break;
		}
		else
		  break;
	    }
	    if (input != pref->index && cmd == POPCMD_FIND)
	      rc = FALSE;
	  }
	}
	break;
    }
    pref->ptr = pref->dataptr;
    return(rc);
}


/*  findstat - status string retriever
 *
 *  ENTRY   cmd       retrieval command
 *	    input     additional input
 *	    pref      address of reference data
 *
 *  EXIT    TRUE if successful, FALSE if error
 */

USHORT FAR findstat(cmd, input, pref)
USHORT cmd;
SHORT input;
register POPFNDATA *pref;
{
    USHORT rc = FALSE;
    PCHAR p;
    register SHORT v;
    NPCUEINFOBLOCK pinfo;

    pinfo = GetInfo();

    switch(cmd) {

    case POPCMD_INIT:
	pref->index = 0;
	pref->miscptr = NULL;
	if (DosAllocSeg(32, (PUSHORT)(&pref->miscptr)+1, 0))
	    break;
	*((PCHAR)(pref->miscptr)) = ' ';
	*((PCHAR)(pref->miscptr)+1) = ' ';

    case POPCMD_NEAR:
	if (input < 0)
	    input = 0;

    case POPCMD_FIND:
	if (input < 0)
	    break;

	p = NULL;
	switch(input) {
	case 0:
	    p = "";
	    break;
	case 1:
	    p = "Stacks busy:";
	    v = pinfo->stacksbusy;
	    break;
	case 2:
	    p = "Stacks held:";
	    v = pinfo->stacksheld;
	    break;
	case 3:
	    p = "Stacks alloced:";
	    v = pinfo->stacksalloc;
	    break;
	case 4:
	    p = "";
	    break;
	case 5:
	    p = "Stack limit:";
	    v = pinfo->stacksmax;
	    break;
	case 6:
	    p = "Stack size:";
	    v = pinfo->stacksize;
	    break;
	case 7:
	    p = "Stack size used:";
	    v = pinfo->stacksizeused;
	    break;
	}
	if (p && pref->miscptr) {
	    rc = TRUE;
	    pref->index = input;
	    if (*p) {
		printstr(0, 0, 18, FALSE, p, (PCHAR)(pref->miscptr)+2);
		printnum(0, 0,	0, TRUE,  v, (PCHAR)(pref->miscptr)+20);
		p = pref->miscptr;
	    }
	    pref->dataptr = p;
	}
	break;

    case POPCMD_EXIT:
	DosFreeSeg(FP_SEG(pref->miscptr));
	break;
    }
    pref->ptr = pref->dataptr;
    return(rc);
}


/*  findline - screen line retriever
 *
 *  ENTRY   cmd       retrieval command
 *	    input     additional input
 *	    pref      address of reference data
 *
 *  EXIT    TRUE if successful, FALSE if error
 */

USHORT FAR findline(cmd, input, pref)
USHORT cmd;
SHORT input;
register POPFNDATA *pref;
{
    USHORT rc = FALSE;
    PCHAR p;
    register SHORT count, len;

    len = (pref->psg->viomaxcol+1)*2;

    switch(cmd) {

    case POPCMD_INIT:
	pref->index = 0;
	pref->dataptr = (PCHAR)(pref->miscptr) +
			 SAVEHDR + pref->psg->viomaxrow * len;

    case POPCMD_NEAR:
	if (input < 0)
	    input = 0;

    case POPCMD_FIND:
	if (input < 0)
	    break;
	rc = TRUE;
	count = input - pref->index;
	if (count) {
	  if (input == 0)
	    rc = findline(POPCMD_INIT, 0, pref);
	  else {
	    if (count < 0) {
	      count = -count;
	      while (count--)
		if (pref->index) {
		  pref->index--;
		  (PUCHAR)(pref->dataptr) += len;
		  if (pref->dataptr == pref->psg->vioscrollbuff +
				       pref->psg->vioscrollfreepos)
		    pref->dataptr = (PUCHAR)(pref->miscptr) + SAVEHDR;
		}
		else
		  break;
	    }
	    else {
	      while (count--) {
		p = pref->dataptr;
		if (p == (PUCHAR)(pref->miscptr) + SAVEHDR)
		  p = pref->psg->vioscrollbuff + pref->psg->vioscrollfreepos;
		if (p != pref->psg->vioscrollbuff) {
		  pref->index++;
		  (PUCHAR)(pref->dataptr) = p - len;
		}
		else
		  break;
	      }
	    }
	    if (input != pref->index && cmd == POPCMD_FIND)
	      rc = FALSE;
	  }
	}
	break;

    case POPCMD_LEFT:
    case POPCMD_RIGHT:
    case POPCMD_ENTER:
	for (count = 0; count < len; count += 2)
	  if (*((PUCHAR)(pref->dataptr)+count) != ' ')
	    break;
	while (len > count) {
	  if (*((PUCHAR)(pref->dataptr)+len-2) != ' ')
	    break;
	  len -= 2;
	}
	if (count < len) {
	  for (rc = 0; count < len; count += 2) {
	    cmd = *((PUCHAR)(pref->dataptr)+count);
	    if (cmd < 32 || cmd >= 127)
		cmd = 32;
	    pref->psg->kbdhistbuff[rc++] = (UCHAR)cmd;
	  }
	  pref->psg->kbdhistbuff[rc] = 0;
	  rc = TRUE;
	}
	break;
    }
    pref->ptr = pref->dataptr;
    return(rc);
}


/*  findcolor - color string retriever
 *
 *  ENTRY   cmd       retrieval command
 *	    input     additional input
 *	    pref      address of reference data
 *
 *  EXIT    TRUE if successful, FALSE if error
 */

USHORT FAR findcolor(cmd, input, pref)
USHORT cmd;
SHORT input;
register POPFNDATA *pref;
{
    USHORT rc = FALSE;
    PUCHAR p;
    register USHORT i;
    VIOINTENSITY intense;
    static CHAR colors[16][11] = {
		"Black     ",
		"Blue      ",
		"Green     ",
		"Cyan      ",
		"Red       ",
		"Magenta   ",
		"Brown     ",
		"White     ",
		"Gray      ",
		"B. Blue   ",
		"B. Green  ",
		"B. Cyan   ",
		"B. Red    ",
		"B. Magenta",
		"Yellow    ",
		"B. White  "
		};

    static UCHAR palette[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
				0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};

    switch(cmd) {

    case POPCMD_INIT:
	pref->index = input = 0;
	p = NULL;
	if (DosAllocSeg(sizeof(PVOID)+32, (PUSHORT)(&p)+1, 0))
	    break;
	*(PVOID FAR *)p = pref->miscptr;
	pref->miscptr = p;
	if (!(pref->psg->cueflags & CUESTATE_SETSTATE)) {
	    p = *(PCHAR FAR *)(pref->miscptr);
	    p += sizeof(POPDATA) + sizeof(VIOCURSORINFO);
	    VioGetState(p, VIO_HANDLE);
	    p += sizeof(PALSTATE);
	    VioGetState(p, VIO_HANDLE);
	    intense.cb = sizeof(VIOINTENSITY);
	    intense.type = 2;
	    intense.fs = 1;		/* force to intensity state */
	    VioSetState(&intense, VIO_HANDLE);
	}

    case POPCMD_NEAR:
	if (input < 0)
	    input = 0;

    case POPCMD_FIND:
	if (input < 0)
	    break;
	if (input < 16 && pref->miscptr) {
	    rc = TRUE;
	    pref->index = input;
	    p = pref->dataptr = (PUCHAR)(pref->miscptr)+sizeof(PVOID);
	    for (i=0; i<16; i++) {
		if (i < 3 || i > 12)
		    *p++ = ' ';
		else
		    *p++ = colors[input][i-3];
		if (input < 8)
		    *p++ = (UCHAR)(input << 4) + BRIGHT+WHITE;
		else
		    *p++ = (UCHAR)(input << 4) + BLACK;
	    }
	}
	break;

    case POPCMD_LEFT:
	p = *(PCHAR FAR *)(pref->miscptr);
	p += sizeof(POPDATA) + sizeof(VIOCURSORINFO);
	if (((PPALSTATE)(p))->acolor[pref->index] == 0)
	    ((PPALSTATE)(p))->acolor[pref->index] = 0x3F;
	else
	    ((PPALSTATE)(p))->acolor[pref->index]--;
	VioSetState(p, VIO_HANDLE);
	break;

    case POPCMD_RIGHT:
	p = *(PCHAR FAR *)(pref->miscptr);
	p += sizeof(POPDATA) + sizeof(VIOCURSORINFO);
	if (((PPALSTATE)(p))->acolor[pref->index] == 0x3F)
	    ((PPALSTATE)(p))->acolor[pref->index] = 0;
	else
	    ((PPALSTATE)(p))->acolor[pref->index]++;
	VioSetState(p, VIO_HANDLE);
	break;

    case POPCMD_ENTER:
	p = *(PCHAR FAR *)(pref->miscptr);
	p += sizeof(POPDATA) + sizeof(VIOCURSORINFO);
	((PPALSTATE)(p))->acolor[pref->index] = palette[pref->index];
	VioSetState(p, VIO_HANDLE);
	break;

    case POPCMD_EXIT:
	if (!(pref->psg->cueflags & CUESTATE_SETSTATE)) {
	    p = *(PCHAR FAR *)(pref->miscptr);
	    p += sizeof(POPDATA) + sizeof(VIOCURSORINFO) + sizeof(PALSTATE);
	    VioSetState(p, VIO_HANDLE);     /* restore intensity state */
	}
	DosFreeSeg(FP_SEG(pref->miscptr));
	break;
    }
    pref->ptr = pref->dataptr;
    return(rc);
}


/*  findchar - character string retriever
 *
 *  ENTRY   cmd       retrieval command
 *	    input     additional input
 *	    pref      address of reference data
 *
 *  EXIT    TRUE if successful, FALSE if error
 *	    Format of string returned:	" X DDD HH  X DDD HH  ... "
 */

USHORT FAR findchar(cmd, input, pref)
USHORT cmd;
SHORT input;
register POPFNDATA *pref;
{
    USHORT rc = FALSE;
    PUCHAR p;
    register USHORT i, perrow, percol;

    perrow = (pref->ppop->lrc - pref->ppop->ulc - 1)/10;
    percol = (254+perrow)/perrow - 1;

    switch(cmd) {

    case POPCMD_INIT:
	pref->index = 0;
	pref->miscptr = NULL;
	if (DosAllocSeg(80, (PUSHORT)(&pref->miscptr)+1, 0))
	    break;

    case POPCMD_NEAR:
	if (input < 0)
	    input = 0;
	else if (input > percol)
	    input = percol;

    case POPCMD_FIND:
	p = pref->miscptr;
	if (input < 0 || input > percol || !p)
	    break;
	i = input + 1;
	while (i < 256 && perrow--) {
	    if (!rc) {
	      rc = TRUE;
	      pref->index = input;
	      pref->dataptr = p;
	    }
	    *p++ = ' ';
	    *p++ = (UCHAR)i;
	    *p++ = ' ';
	    printnum(0, 0, 3, 0, i, p);
	    p += 3;
	    *p++ = ' ';
	    printnum(0, 0, 2, 2, i, p);
	    p += 2;
	    *p++ = ' ';
	    i += percol+1;
	}
	*p = 0;
	break;

    case POPCMD_EXIT:
	DosFreeSeg(FP_SEG(pref->miscptr));
	break;
    }
    pref->ptr = pref->dataptr;
    return(rc);
}
