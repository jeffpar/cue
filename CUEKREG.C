/*  CUEKREG.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs registered keyboard replacements
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



/*  _KbdCharIn - replacement for KbdCharIn
 *
 *  This handler has the same parameters as the original KbdCharIn
 *  function, always returning ROUTER_PASS (-1), unless I have data to
 *  simulate as input.
 *
 *  ENTRY   pdata   far ptr to a KBDKEYINFO packet
 *	    nowait  either FALSE or TRUE
 *	    hndl    keyboard handle, usually KBD_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't simulate key input, ROUTER_DONE
 *	    (0) otherwise
 *
 *  NOTES   Ideally, for nowait calls, if I had a character to simulate,
 *	    I would simulate it, and if a timed delay was given, I would
 *	    allow as many nowait calls to fall through as it took for
 *	    that amount of real time to elapse.  If I didn't do this, I
 *	    could provide the illusion of an impossibly fast typist, and
 *	    choke the caller.  I circumvent this whole issue for
 *	    now, by allowing the base subsystem to service nowait requests.
 *	    While it is possible to imagine an application that performs
 *	    all input via nowait requests (hence impossible to simulate
 *	    input into), that would appear to be an uncommon application.
 */

USHORT FAR _KbdCharIn(pdata, nowait, hndl)
PKBDKEYINFO pdata;
USHORT nowait;
HKBD hndl;
{
    PUCHAR p;
    STRINGINBUF len;
    KBDINFO kdata;
    USHORT row, col;
    register NPSG psg;
    register USHORT i, rc = ROUTER_PASS;

    if (SwitchStacks(4))
	return(rc);
    if (!nowait)	    /* if we might end up waiting, */
	SaveSignals();	    /* we must not suppress signals any longer */
    if (psg=mapsg()) {

	pdata->chChar = 0;
	pdata->chScan = 0;
	pdata->fbStatus = 0x40;
	pdata->bNlsShift = 0;
	pdata->fsState = 0;
	pdata->time = PGinfoseg->time;

	i = psg->kbdinsptr;
	p = psg->kbdhistbuff + MAXSTRINGINSIZE;
	while (p[i] && rc) {
	    if (!nowait) {
		if (p[i] == 0xff) {
		    if (p[++i] == 0xff)
			pdata->chChar = 0xff;
		    else if (p[i] == 0xfe) {
			DosSleep((ULONG)(p[++i]*250L));
			goto checksim;
		    }
		    else
			pdata->chScan = p[i];
		}
		else
		    pdata->chChar = p[i];
	    }
	    rc = 0;
	    if (nowait) {
		pdata->fbStatus = 0;
		break;
	    }

    checksim:
	    i++;
	    if (i >= MAXINSERTSIZE) {
		i = MAXINSERTSIZE-1;
		p[i] = 0;
	    }
	    psg->kbdinsptr = i;
	}

	if (rc) {
	    if (psg->chTypeAhead) {
		pdata->chChar = (UCHAR)(psg->chTypeAhead & 0xFF);
		pdata->chScan = (UCHAR)(psg->chTypeAhead / 256);
		psg->chTypeAhead = 0;
		rc = 0;
	    }
	    else if (LockThread()) {
		do {
    reprocess:
		    if (psg->iEditBuff < psg->nEditBuffTotal) {
			pdata->chChar = psg->achEditBuff[psg->iEditBuff++];
			pdata->chScan = 0;
			rc = 0;
			break;
		    }
		    rc = KbdCharIn(pdata, nowait, hndl);
		    if (rc)
			break;
		    if (psg->cueflags & CUESTATE_LINEEDIT) {
			i = pdata->chChar;
			if (i >= CTRL_A && i < ' ')
			    continue;
			if (!i) {
			    i = pdata->chScan;
			    if (i < HOME || i >= SHIFT_F1 && i <= ALT_F10)
				continue;
			}
			VioGetCurPos(&row, &col, VIO_HANDLE);
			if (col != 1)
			    continue;

			kdata.cb = sizeof(kdata);
			KbdGetStatus(&kdata, hndl);
			row = kdata.fsMask;
			col = kdata.chTurnAround;
			kdata.fsMask = KBDMODE_ECHO |
				       KBDMODE_COOKED |
				       KBDMODE_TURNAROUND;
			kdata.chTurnAround = CR;
			KbdSetStatus(&kdata, hndl);
			psg->chTypeAhead = pdata->chChar + pdata->chScan*256;
			len.cb = EDITBUFFSIZE-1;    // BUGBUG: Need -1???
			len.cchIn = 0;
			KbdStringIn(psg->achEditBuff, &len, FALSE, hndl);
			kdata.fsMask = row;
			kdata.chTurnAround = col;
			KbdSetStatus(&kdata, hndl);
			psg->iEditBuff = 0;
			if (psg->cueflags & CUESTATE_LINEEDIT) {
			    psg->nEditBuffTotal = ++len.cchIn;
			    goto reprocess;
			}
			else {
			    psg->nEditBuffTotal = 0;
			    pdata->chChar = 0;
			    pdata->chScan = F8;
			}
		    }
		} while (processkey(psg, pdata, TRUE, hndl));
		UnLockThread();
	    }
	}
    }
    if (!nowait)
	RestoreSignals();
    RestoreStacks();
    return(rc);
}


/*  _KbdPeek - replacement for KbdPeek
 *
 *  This handler has the same parameters as the original KbdPeek
 *  function, always returning ROUTER_PASS (-1), unless I have data to
 *  simulate as input.
 *
 *  ENTRY   pdata   far ptr to a KBDKEYINFO packet
 *	    hndl    keyboard handle, usually KBD_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't simulate key input, ROUTER_DONE
 *	    (0) otherwise
 *
 *  NOTES   Ideally, if I had a character to simulate, I would simulate
 *	    it here, as well as in _KbdCharIn.	However, I'm only adding
 *	    this function right now to detect any keys I may have reserved
 *	    before an application can, so that I can grab them before the
 *	    application does something it'll regret.
 */

USHORT FAR _KbdPeek(pdata, hndl)
PKBDKEYINFO pdata;
HKBD hndl;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(3))
	return(rc);
    if (LockThread()) {
	if (psg=mapsg())
	    do {
		rc = KbdPeek(pdata, hndl);
		if (rc)
		    break;
	    } while (processkey(psg, pdata, FALSE, hndl));
	UnLockThread();
    }
    RestoreStacks();
    return(rc);
}


/*  _KbdFlushBuffer - replacement for KbdFlushBuffer
 *
 *  This handler has the same parameters as the original KbdFlushBuffer
 *  function, always returning ROUTER_PASS (-1), unless I have data to
 *  simulate as input.
 *
 *  ENTRY   hndl    keyboard handle, usually KBD_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't simulate key input, ROUTER_DONE
 *	    (0) otherwise
 *
 *  NOTES   This is to prevent the base subsystem from flushing any
 *	    real user input while simulated input exists, allowing
 *	    type-ahead while simulated input is being generated.
 *	    Simulated input is already unflushable because the KBD
 *	    flush IOTCL has no effect here.  This function is not
 *	    strictly necessary, as I think I only added it for VTP.
 */

USHORT FAR _KbdFlushBuffer(hndl)
HKBD hndl;
{
    PUCHAR p;
    register NPSG psg;
    register USHORT i, rc = ROUTER_PASS;

    if (SwitchStacks(1))
	return(rc);
    if (psg=mapsg()) {
	i = psg->kbdinsptr;
	p = psg->kbdhistbuff + MAXSTRINGINSIZE;
	if (p[i])
	    rc = ROUTER_DONE;
    }
    RestoreStacks();
    return(rc);
}


/*  _KbdSetStatus - replacement for KbdSetStatus
 *
 *  This handler has the same parameters as the original KbdSetStatus
 *  function, always returning ROUTER_PASS (-1).  I simply want to (try
 *  to) shadow the input state.
 *
 *  ENTRY   pinfo   far ptr to a KBDINFO packet
 *	    hndl    keyboard handle, usually KBD_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't simulate key input, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _KbdSetStatus(pinfo, hndl)
PKBDINFO pinfo;
HKBD hndl;
{
    register NPSG psg;

    if (!SwitchStacks(3)) {
      if (psg=mapsg())
	psg->kbdmode = pinfo->fsMask;
      RestoreStacks();
    }
    return(ROUTER_PASS);
}


/*  _KbdStringIn - replacement for KbdStringIn
 *
 *  This handler has the same parameters as the original KbdStringIn
 *  function, always returning ROUTER_PASS (-1), unless I determine the
 *  function should be hooked.
 *
 *  ENTRY   hndl    keyboard handle, usually KBD_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _KbdStringIn(pbuff, plen, nowait, hndl)
PCHAR pbuff;
PSTRINGINBUF plen;
USHORT nowait;
HKBD hndl;
{
    KBDKEYINFO kdata;
    NPHP php;
    register NPSG psg;
    register UCHAR c;
    USHORT rc = ROUTER_PASS;
    PUCHAR currentline;
    USHORT srow, scol;
    static CHAR ctrlc[] = {'^', 'C', '\r', '\n'};
    USHORT insert, history = FALSE, flag;
    SHORT ppos, pcount, tcount;
    USHORT currentsize, vpos, vcount, maxlen;

    if (SwitchStacks(6))
	return(rc);
    SaveSignals();
    if (psg=mapsg()) {
      getkbdstatus(psg, FALSE, hndl);
      if (!(psg->kbdmode & KBDMODE_INHIBIT || nowait)) {
	rc = ROUTER_DONE;
	currentline = psg->kbdhistbuff;
	if ((pcount = plen->cchIn) > MAXSTRINGINSIZE-1)
	    pcount = MAXSTRINGINSIZE-1;
	for (ppos=0; pcount; pcount--,ppos++)
	    currentline[ppos] = pbuff[ppos];
	currentsize = ppos;
	ppos = 0;		/* pcount already zero */
	vpos = vcount = 0;
	if ((maxlen = plen->cb) > MAXSTRINGINSIZE-1)
	    maxlen = MAXSTRINGINSIZE-1;

	if (maxlen--) { 	/* if maxlen ok, convert to maxpos now */
	  php = histbuffer(psg, FP_OFF(pbuff));
	  VioGetCurPos(&srow, &scol, VIO_HANDLE);
	  insert = ((psg->cueflags & CUESTATE_INSERT) != 0);
	  setinsert(psg, insert, hndl);
	  c = 0;
	  while (c != psg->kbdturnaround) {
	    if (rc = getkey(psg, &kdata, hndl))
		break;
	    c = kdata.chChar;
	    if (c == 0) {		/* if non-ascii keystroke */
		c = kdata.chScan;
		if (c == F1 || c == RIGHT) {
		    if (ppos < currentsize)
			addchar(currentline[ppos++], &pcount, &vpos, &vcount);
		}
		else if (c == CTRL_END) {
		    eraseline(vpos, &vcount);
		    pcount = ppos;
		}
		else if (c == CTRL_DEL) {
		    resetlines(php, TRUE);
		    kdata.chChar = ESCAPE;
		}
		else if (c == F3 || c == END) {
		    endline(psg->viomaxcol, vcount, &vpos);
		    ppos = pcount;
		    if (c == F3) {
			multchar(currentline+ppos,
				 currentsize-ppos, &pcount, &vpos, &vcount);
			ppos += currentsize-ppos;
		    }
		}
		else if (c == UP || c == DOWN) {
		    if (fetchline(psg, php, pcount, &currentsize, history, (c==UP))) {
			history = TRUE;
			replaceline(currentline, currentsize,
				    psg->viomaxcol, &pcount, &vpos, &vcount);
			ppos = pcount;
		    }
		}
		else if (c == LEFT) {
		    if (ppos)
			subchar(currentline, --ppos, FALSE, scol,
				    psg->viomaxcol, &vpos, &vcount);
		}
		else if (c == HOME) {
		    homeline(psg->viomaxcol, &vpos);
		    ppos = 0;
		}
		else if (c == F6) {
		    kdata.chChar = CTRL_Z;
		}
		else if (c == F10) {
		    if (psg->cueflags & CUESTATE_LINEEDIT) {
			psg->cueflags &= ~CUESTATE_LINEEDIT;
			kdata.chChar = psg->kbdturnaround;
		    }
		}
		else if (c == CTRL_RT) {
		    flag = FALSE;
		    while (ppos < pcount) {
			c = currentline[ppos];
			if (flag) {
			    if (CueIsAlphaNum(c))
			       break;
			}
			else
			    flag = !CueIsAlphaNum(c);
			addchar(c, &pcount, &vpos, &vcount);
			ppos++;
		    }
		}
		else if (c == CTRL_LT) {
		    flag = FALSE;
		    while (ppos > 0) {
			c = currentline[ppos-1];
			if (flag) {
			    if (!CueIsAlphaNum(c))
				break;
			}
			else
			    flag = CueIsAlphaNum(c);
			subchar(currentline, --ppos, FALSE, scol,
				    psg->viomaxcol, &vpos, &tcount);
		    }
		}
		else if (c == DEL) {
		    if (ppos < pcount) {
			replacechar(currentline, ppos,
				    0, psg, --pcount, vpos, &vcount);
			if (currentsize > 0)
			    currentsize--;
		    }
		}
		else if (c == INS) {
		    insert = !insert;
		    setinsert(psg, insert, hndl);
		}
		else if (c == CTRL_PGUP || c == PGUP ||
			 c == CTRL_UP	|| c == CTRL_DN || c == PGDN) {
		    if	(c == CTRL_PGUP || c == PGUP)
			tcount = POPUP_HIST;
		    else
			tcount = POPUP_SCREEN;
		    if (tcount = CuePopup(Poptable+tcount, psg, php, hndl)) {
			currentsize = strlen(currentline);
			replaceline(currentline, currentsize,
				    psg->viomaxcol, &pcount, &vpos, &vcount);
			ppos = pcount;
			if (tcount == 1)
			    kdata.chChar = psg->kbdturnaround;
		    }
		}
		else {
		  tcount = 0;
		  if (c == ALT_F1)
		    tcount = POPUP_HELP+1;
		  else if (c == ALT_F2)
		    tcount = POPUP_PROC+1;
		  else if (c == ALT_F3)
		    tcount = POPUP_COLOR+1;
		  else if (c == ALT_F4)
		    tcount = POPUP_CHAR+1;
		  else if (c == ALT_F9)
		    tcount = POPUP_HIST+1;
		  else if (c == ALT_F10)
		    tcount = POPUP_STAT+1;
		  if (tcount)
		    CuePopup(Poptable+(tcount-1), psg, php, hndl);
		}
	    }
	    c = kdata.chChar;
	    if (c == ESCAPE) {
		history = FALSE;
		resetlines(php, FALSE);
		homeline(psg->viomaxcol, &vpos);
		eraseline(vpos, &vcount);
		if ((pcount = plen->cchIn) > MAXSTRINGINSIZE-1)
		    pcount = MAXSTRINGINSIZE-1;
		for (ppos=0; pcount; pcount--,ppos++)
		    currentline[ppos] = pbuff[ppos];
		currentsize = ppos;
		ppos = 0;		/* pcount already zero */
	    }
	    else if (c == '\b' || c == CTRL_BS) {
	      if (ppos)
		if (ppos == pcount) {
		    subchar(currentline, --ppos, TRUE, scol,
				psg->viomaxcol, &vpos, &vcount);
		    pcount--;
		}
		else {
		    subchar(currentline, --ppos, FALSE, scol,
				psg->viomaxcol, &vpos, &vcount);
		    replacechar(currentline, ppos,
				0, psg, --pcount, vpos, &vcount);
		    if (currentsize > 0)
			currentsize--;
		}
	    }
	    else if (c && (ppos < maxlen ||
		     ppos == maxlen && c == psg->kbdturnaround)) {
		if (c == psg->kbdturnaround) {
		    endline(psg->viomaxcol, vcount, &vpos);
		    VioWrtTTY(&kdata.chChar, 1, VIO_HANDLE);
		    addline(psg, php, pcount);
		    transformline(psg, &pcount, maxlen);
		    ppos = pcount++;
		}
		else {
		    if (insert) {
			if (pcount >= maxlen)
			    continue;
			tcount = pcount++;
			while (tcount > ppos) {
			    currentline[tcount] = currentline[tcount-1];
			    tcount--;
			}
		    }
		    if (ppos < pcount)
			replacechar(currentline, ppos,
				    c, psg, pcount, vpos, &vcount);
		    addchar(c, &pcount, &vpos, &vcount);
		}
		currentline[ppos++] = c;
		if (pcount > currentsize)
		    currentsize++;
	    }
	  }
	  setinsert(psg, FALSE, hndl);
	}

	if (!rc) {
	    plen->cchIn = pcount - 1;
	    for (ppos=0; pcount; pcount--,ppos++)
		pbuff[ppos] = currentline[ppos];
	}
      }
    }
    RestoreSignals();
    RestoreStacks();
    return(rc);
}


/*  _KbdRouter - routes control to appropriate replacement function
 *
 *  ENTRY   ds	    Caller's DS (unused)
 *	    addr    KBDCALLS internal address (unused)
 *	    index   replacement function #
 *	    faraddr far return address of Caller (unused)
 *	    hndl    keyboard handle, usually KBD_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise.  The exit code is determined by the replacement
 *	    function.
 */

USHORT FAR cdecl KbdRouter(ds, addr, index, faraddr, hndl)
USHORT index, addr, ds, hndl;
ULONG faraddr;
{
    register NPSG psg;
    register USHORT *p;
    USHORT rc = ROUTER_PASS;

    if (!SwitchStacks(KBDROUTERFRAME)) {

      if (psg=mapsg())
	  if (!psg->kbdreg) {	  /* do some magic on first call for SG */
	      psg->kbdreg = TRUE;
	      getkbdstatus(psg, TRUE, KBD_HANDLE);
	  }

      p = &hndl;
      switch (index) {
      case INDX_KBDCHARIN:
	  rc = _KbdCharIn(*(PKBDKEYINFO *)(p+2), *(p+1), *p);
	  break;
      case INDX_KBDPEEK:
	  rc = _KbdPeek(*(PKBDKEYINFO *)(p+1), *p);
	  break;
      case INDX_KBDFLUSHBUFFER:
	  rc = _KbdFlushBuffer(*p);
	  break;
      case INDX_KBDSETSTATUS:
	  rc = _KbdSetStatus(*(PKBDINFO *)(p+1), *p);
	  break;
      case INDX_KBDSTRINGIN:
	  rc = _KbdStringIn(*(PCH *)(p+4),
			    *(PSTRINGINBUF *)(p+2), *(p+1), *p);
	  break;
      }
      RestoreStacks();
    }
    return(rc);
}
