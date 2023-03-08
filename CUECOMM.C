/*  CUECOMM.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Communication-related functions
 */

#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



/*  kbdthread - process kbd input and feed it back to the keyboard
 *
 *  ENTRY   none
 *
 *  EXIT    none
 */

VOID FAR kbdthread()
{
    register USHORT rc;
    register NPSG psg = NULL;

    if (!SwitchStacks(0))
	psg = mapsg();

    if (psg) {
	readythread(psg);
	DosSetPrty(PRTYS_THREAD, PRTYC_TIMECRITICAL, PRTYD_MAXIMUM, 0);
	rc = FALSE;
	while (!rc) {
	    psg->mondata->result = sizeof(MONPKT);
	    rc = DosMonRead((PCHAR)(&psg->mondata->bufflen1), 0,
		    (PCHAR)(&psg->mondata->pkt), &psg->mondata->result);
	    if (!rc)
		DosMonWrite((PCHAR)(&psg->mondata->bufflen2),
		    (PCHAR)(&psg->mondata->pkt), psg->mondata->result);
	}
    }
}


/*  commthread - process comm input and feed it to the keyboard as well
 *
 *  ENTRY   none
 *
 *  EXIT    none
 */

VOID FAR commthread()
{
    PMONPKT pkt;
    BOOL pause = FALSE;
    BOOL esc = FALSE;
    USHORT monhandle, action;
    register USHORT rc;
    register NPSG psg = NULL;

    if (!SwitchStacks(0))
	psg = mapsg();

    if (psg) {
	readythread(psg);
	DosSetPrty(PRTYS_THREAD, PRTYC_TIMECRITICAL, PRTYD_MAXIMUM, 0);
	if (!DosMonOpen("KBD$", &monhandle)) {

	    psg->mondata = NULL;
	    if (DosAllocSeg(sizeof(MONDATA), (PUSHORT)(&psg->mondata)+1, 0))
		goto comm_abort;

	    psg->mondata->bufflen1 = psg->mondata->bufflen2 = MONBUFFSIZE;

	    if (DosMonReg(monhandle, (PCHAR)(&psg->mondata->bufflen1),
				     (PCHAR)(&psg->mondata->bufflen2),
				      1, PLinfoseg->sgCurrent))
		goto comm_abort;

	    startthread(psg, kbdthread);/* having gotten this far, */
					/* might as well start up this too */
	    rc = FALSE;
	    pkt = &psg->mondata->pkt;
	    while (!rc) {
		rc = DosRead(psg->commhandle, &pkt->key.chChar, 1, &action);
		if (action) {
		    action = 0; 	/* think of this as signal action now */
		    pkt->monflgs = 0;
		    pkt->kbddflgs = 0;

		    rc = pkt->key.chChar;
		    if (rc == ESCAPE) {
			if (!esc) {
			    esc++;
			    goto nextkey;
			}
		    }

		    /*** Speed key detection ***/
		    if (esc) {
			if (rc == '<') {
			    DosSetPrty(PRTYS_PROCESSTREE,
				       PRTYC_REGULAR,
				       0, 0);
			    DosSetPrty(PRTYS_THREAD,
				       PRTYC_TIMECRITICAL,
				       0, 0);
			    esc--;
			    goto nextkey;
			}
			else if (rc == '>') {
			    DosSetPrty(PRTYS_PROCESSTREE,
				       PRTYC_TIMECRITICAL,
				       0, 0);
			    esc--;
			    goto nextkey;
			}
		    }

		    /*** Pause key simulation ***/
		    if (rc == CTRL_S) {
			if (psg->kbdmode & KBDMODE_COOKED) {
			    pause = TRUE;
			    pkt->kbddflgs |= KBDD_PSEUDOPAUSE;
			}
		    }
		    else if (pause) {
			pause = FALSE;
			pkt->kbddflgs |= KBDD_WAKEUP;
		    }

		    /*** Signal key simulation ***/
		    if (psg->kbdmode & KBDMODE_COOKED) {
			if (rc == 0)
			   #ifndef DDSIG
			    action = SIG_CTRLBREAK;
			   #else
			    pkt->kbddflgs |= KBDD_BRK;
			   #endif
			else if (rc == CTRL_C)
			   #ifndef DDSIG
			    action = SIG_CTRLC;
			   #else
			    pkt->kbddflgs |= KBDD_PSEUDOBRK;
			   #endif
		    }

		    if (!action || pkt->kbddflgs) {
			pkt->key.chScan = 0;
			pkt->key.fbStatus = KSTATUS_FINAL;
			pkt->key.bNlsShift = 0;
			pkt->key.fsState = 0;
			pkt->key.time = PGinfoseg->time;
			mapkey(&pkt->key, esc);
			rc = DosMonWrite((PCHAR)(&psg->mondata->bufflen2),
						(PCHAR)pkt, sizeof(MONPKT));
		    }
		    if (action)
			sendsignal(psg, action);
		    esc = FALSE;
		nextkey:
		    rc = 0;
		}
	    }
comm_abort:
	DosMonClose(monhandle);
	}
    }
}


/*  sendsignal - simulate a signal for the current screen group
 *
 *  ENTRY   psg     SG data pointer
 *	    signal  signal number to send
 *
 *  EXIT    none
 */

VOID sendsignal(psg, signal)
register NPSG psg;
USHORT signal;
{
    register USHORT rc;
    static CHAR ctrlc[] = {'^', 'C', CR, LF};

    if (psg->cuechildpid) {
	rc = DosSendSignal(psg->cuechildpid, signal);
	if (rc == ERROR_NO_SIGNAL_SENT) {
	    VioWrtTTY(ctrlc, sizeof(ctrlc), VIO_HANDLE);
	    rc = DosKillProcess(DKP_PROCESSTREE, psg->cuechildpid);
	}
    }
}


/*  mapkey - map an ascii code to an IBM char/scan key combination
 *
 *  ENTRY   pkey    far ptr to key structure
 *
 *  EXIT    none
 */

VOID mapkey(pkey, esc)
register PKBDKEYINFO pkey;
BOOL esc;
{
    register UCHAR c;
    static UCHAR arrowscan[] = {LEFT, NULL, DOWN, UP, RIGHT};
    static UCHAR arrowscanEsc[] = {HOME, NULL, PGDN, PGUP, END};

    c = pkey->chChar;
    if (c == ESCAPE)
	pkey->chScan = ESCSCAN;
    else if (c == '\\')
	c = '@';
    else if (c == '@')
	c = '\\';
    else if (c == '|')
	c = '`';
    else if (c == '`')
	c = '|';
    else if (c == '^')
	c = RUB;
    else if (c == RUB)
	c = '^';
    else if (c == '~')
	c = '_';
    else if (c == '_')
	c = '~';
    else if (c == CTRL_SLASH) {
	pkey->chScan = ATSCAN;
	c = 0;
    }
    else if (c == 0)
	c = CTRL_SLASH;
    else if (c == CTRL_UNDER)
	c = CTRL_CARET;
    else if (c == CTRL_CARET)
	c = CTRL_UNDER;

    if (c == RUB) {
	if (esc) {
	    pkey->chScan = DEL;
	    c = 0;
	}
	else {
	    pkey->chScan = BKSP;
	    c = CTRL_H;
	}
    }
    else if (c == CTRL_I)
	pkey->chScan = TABSCAN;
    else if (c >= CTRL_H && c <= CTRL_L) {
	if (!esc)
	    pkey->chScan = arrowscan[c-CTRL_H];
	else
	    pkey->chScan = arrowscanEsc[c-CTRL_H];
	c = 0;
    }

    if (esc) {
	if (c >= '0' && c <= '9') {
	    if (c == '0')
		c = '9' + 1;
	    pkey->chScan = (UCHAR)(F1 + (c - '1'));
	    c = 0;
	}
    }
    pkey->chChar = c;
}
