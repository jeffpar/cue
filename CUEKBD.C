/*  CUEKBD.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cue keyboard routines
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"


USHORT getkbdstatus(psg, firsttime, hndl)
register NPSG psg;
USHORT firsttime;
HKBD hndl;
{
    KBDINFO kdata;
    register USHORT rc = 0;

    kdata.cb = sizeof(kdata);
    rc = KbdGetStatus(&kdata, hndl);
    psg->kbdmode = kdata.fsMask;
    if (psg->kbdmode & (KBDMODE_NOECHO | KBDMODE_RAW | KBDMODE_EXTTURN))
	psg->kbdmode |= KBDMODE_INHIBIT;
    psg->kbdturnaround = (UCHAR)kdata.chTurnAround;
    if (firsttime && !(psg->cueflags & CUESTATE_ALTENT)) {
	kdata.fsMask |= KBDMODE_SSTATE;
	kdata.fsState &= ~SSTATE_NUM;
	KbdSetStatus(&kdata, hndl);
    }
    return(rc);
}


USHORT setkbdstatus(psg, cooked, hndl)
register NPSG psg;
USHORT cooked;
HKBD hndl;
{
    KBDINFO kdata;
    register unsigned rc = 0;

    kdata.cb = sizeof(kdata);
    kdata.fsMask = psg->kbdmode & (KBDMODE_ECHO | KBDMODE_NOECHO);
    if (!cooked)
	kdata.fsMask |= KBDMODE_RAW;
    else
	kdata.fsMask |= KBDMODE_COOKED;
    rc = KbdSetStatus(&kdata, hndl);
    return(rc);
}


VOID xlatekey(psg, pkeydata, hndl)
register NPSG psg;
KBDKEYINFO *pkeydata;
HKBD hndl;
{
    register UCHAR c1, c2;

    c1 = pkeydata->chChar;
    c2 = pkeydata->chScan;
    if (c2 == KEYPAD_PLUS && (psg->cueflags & CUESTATE_ALTENT))
	c1 = psg->kbdturnaround;
    if (c1 == 0 || c1 == 0xE0)
	c1 = 0;
    else
	c2 = 0;
    pkeydata->chChar = c1;
    pkeydata->chScan = c2;
}


USHORT getkey(psg, pkeydata, hndl)
register NPSG psg;
KBDKEYINFO *pkeydata;
HKBD hndl;
{
    register USHORT rc;

    do {
	rc = KbdCharIn(pkeydata, IO_WAIT, hndl);
	if (rc)
	    break;
    } while (processkey(psg, pkeydata, TRUE, hndl));

    if (!rc)
	xlatekey(psg, pkeydata, hndl);
    return(rc);
}


BOOL processkey(psg, pkeydata, read, hndl)
register NPSG psg;
PKBDKEYINFO pkeydata;
USHORT read;
HKBD hndl;
{
    KBDKEYINFO tmpkey;
    register BOOL rc = FALSE;

    tmpkey = *pkeydata;
    if (tmpkey.fbStatus & KSTATUS_FINAL) {

	xlatekey(psg, &tmpkey, hndl);
	if (!tmpkey.chChar && !(psg->cueflags & CUESTATE_NOCTRL)) {

	    if (tmpkey.chScan == CTRL_F1)
		rc = POPUP_HELP+1;
	    else if (tmpkey.chScan == CTRL_F2)
		rc = POPUP_PROC+1;
	    else if (tmpkey.chScan == CTRL_F3)
		rc = POPUP_COLOR+1;
	    else if (tmpkey.chScan == CTRL_F4)
		rc = POPUP_CHAR+1;
	    else if (tmpkey.chScan == CTRL_F9)
		rc = POPUP_HIST+1;
	    else if (tmpkey.chScan == CTRL_F10)
		rc = POPUP_STAT+1;

	    if (rc) {
		if (!read)			/* eat key if not yet read */
		    KbdCharIn(&tmpkey, IO_WAIT, hndl);
		if (psg->poplocks < MAXACTIVEPOPUPS)
		    CuePopup(Poptable+(rc-1), psg, NULL, hndl);
	    }
	}
    }
    return(rc);
}
