/*  CUEVREG.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs registered video replacements
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



/*  _VioGetBuf - replacement for VioGetBuf
 *
 *  This handler has the same parameters as the original VioGetBuf
 *  function, always returning ROUTER_PASS (-1), unless I have hooked
 *  the function.
 *
 *  ENTRY   pdata   far ptr to a VIOMODEINFO packet
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 *
 *  NOTES   This replacement allows me to fake the # rows when we
 *	    are operating on a terminal.
 */

USHORT FAR _VioGetBuf(pbuf, plen, hndl)
PULONG pbuf;
PUSHORT plen;
HVIO hndl;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(5))
	return(rc);
    if (LockThread()) {
	if (psg=mapsg()) {
	    rc = VioGetBuf(pbuf, plen, hndl);
	    if (!rc) {
		if (psg->commport && *plen > 80*2*24)
		    *plen = 80*2*24;
	    }
	}
	UnLockThread();
    }
    RestoreStacks();
    return(rc);
}


/*  _VioShowBuf - replacement for VioShowBuf
 *
 *  This handler has the same parameters as the original VioShowBuf
 *  function, always returning ROUTER_PASS (-1), unless I have hooked
 *  the function.
 *
 *  ENTRY   off     offset within LVB to begin update
 *	    len     length of update
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 *
 *  NOTES   This replacement allows me to fake the # rows when we
 *	    are operating on a terminal.
 */

USHORT FAR _VioShowBuf(off, len, hndl)
USHORT off;
USHORT len;
HVIO hndl;
{
    register NPSG psg;
    register SHORT i;
    PUCHAR pbuf;
    USHORT lenbuf, row, col;

    if (SwitchStacks(3))
	return(ROUTER_PASS);
    if (psg=mapsg()) {
	if (psg->commport && psg->commhandle && psg->commrate) {
	    i = VioGetBuf((PULONG)&pbuf, &lenbuf, hndl);
	    if (!i) {
		if (off > lenbuf-1)
		    off = lenbuf-1;
		if (off+len > lenbuf-2)
		    if (len >= 2)
		      len = lenbuf-off-2;
		pbuf += off;
		len /= 2;		// compensate for char/attr

		row = off/160;
		col = off%160/2;
		ttyhidecursor(psg->commhandle);

		LockSem(&Buffsem);
		while (len && row <= 24-1) {
		    i = 80-1 - col;	// find outermost index for
		    if (i >= len)	// current line
		      i = len-1;
		    else
		      for (; i>=0; i--) // work backwards, find 1st non-blank
			if (pbuf[i*2] != ' ')
			    break;
		    off = col + ++i;	// off is 1st blank column
		    ttysetpos(row, col, psg->commhandle);
		    for (i=0; col<off; i++,col++,len--) {
			Tmpbuff[i] = *pbuf++;
			if (Tmpbuff[i] < 0x20 || Tmpbuff[i] > 0x7e) {
			   #ifdef SOROC
			    Tmpbuff[i] = 23;
			    Tmpbuff[++i] = '?';
			    Tmpbuff[++i] = 4;
			   #else
			    Tmpbuff[i] = '?';
			   #endif
			}
			pbuf++; 	// skip attr for now...
		    }
		    off += len;
		    if (off > 80)
			off = 80;
		    off -= col;
		    if (off) {		// if we didn't process the whole line
			Tmpbuff[i++] = ESCAPE;
		       #ifdef SOROC
			Tmpbuff[i++] = 'T';
		       #else
			Tmpbuff[i++] = 'K';
		       #endif
			pbuf += off*2;	// adjust pointer to next line
			len -= off;
		    }
		    DosWrite(psg->commhandle, Tmpbuff, i, &lenbuf);

		    col = 0;		// prepare for next line
		    row++;
		}
		UnLockSem(&Buffsem);

		VioGetCurPos(&row, &col, hndl);
		ttysetpos(row, col, psg->commhandle);
		ttyshowcursor(psg->commhandle);
	    }
	}
    }
    RestoreStacks();
    return(ROUTER_PASS);
}


/*  _VioGetMode - replacement for VioGetMode
 *
 *  This handler has the same parameters as the original VioGetMode
 *  function, always returning ROUTER_PASS (-1), unless I have hooked
 *  the function.
 *
 *  ENTRY   pdata   far ptr to a VIOMODEINFO packet
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 *
 *  NOTES   This replacement allows me to fake the # rows when we
 *	    are operating on a terminal.
 */

USHORT FAR _VioGetMode(pdata, hndl)
PVIOMODEINFO pdata;
HVIO hndl;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(3))
	return(rc);
    if (LockThread()) {
	if (psg=mapsg()) {
	    rc = VioGetMode(pdata, hndl);
	    if (!rc) {
		if (psg->commport && pdata->cb >= 8) {
		    pdata->fbType = 0;
		    pdata->color = 0;
		    pdata->row = 24;
		}
	    }
	}
	UnLockThread();
    }
    RestoreStacks();
    return(rc);
}


/*  _VioSetCurPos - replacement for VioSetCurPos
 *
 *  This handler has the same parameters as the original VioSetCurPos
 *  function, always returning ROUTER_PASS (-1).  I simply shadow the calls
 *  to maintain the cursor position on the terminal.
 *  the function.
 *
 *  ENTRY   row     row to set
 *	    col     column to set
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _VioSetCurPos(row, col, hndl)
USHORT row;
USHORT col;
HVIO hndl;
{
    register NPSG psg;

    if (SwitchStacks(3))
	return(ROUTER_PASS);
    if (psg=mapsg())
	if (psg->commport)
	    if (psg->commhandle && psg->commrate)
		ttysetpos(row, col, psg->commhandle);
    RestoreStacks();
    return(ROUTER_PASS);
}


/*  _VioSetMode - replacement for VioSetMode
 *
 *  This handler has the same parameters as the original VioSetMode
 *  function, always returning ROUTER_PASS (-1), unless I have hooked
 *  the function.
 *
 *  ENTRY   pdata   far ptr to a VIOMODEINFO packet
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 *
 *  NOTES   This replacement does nothing except allow me to monitor
 *	    successful mode changes, and adjust the scroll buffer at the
 *	    same time.	I used to do the same thing for KbdSetStatus
 *	    (to monitor the keyboard status), but the introduction of
 *	    multiple logical keyboards requires that I instead obtain the
 *	    kbd status as needed, to avoid maintaining a separate state
 *	    table for each possible logical keyboard.  I'd rather let the
 *	    base subsystem keep track of all that.
 *
 *	    Now, I also try to track graphics modes by setting viomaxcol
 *	    to zero.  This is tested in the popup logic, and should be
 *	    probably be tested in other places too.
 */

USHORT FAR _VioSetMode(pdata, hndl)
PVIOMODEINFO pdata;
HVIO hndl;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(3))
	return(rc);
    if (LockThread()) {
	if (psg=mapsg()) {
	    if (psg->commport) {
		if (pdata->fbType & VGMT_GRAPHICS)
		    rc = ERROR_VIO_MODE;
		else if (pdata->cb >= 8) {
		    pdata->cb = 8;
		    pdata->col = 80;
		    pdata->row = 24;
		}
	    }
	    if (rc == ROUTER_PASS)
		rc = VioSetMode(pdata, hndl);
	    if (!rc && pdata->cb >= 8) {
		psg->viomaxrow = pdata->row-1;
		if (pdata->fbType & VGMT_GRAPHICS || pdata->col > MAXSCROLLCOLS)
		    psg->viomaxrow = 0;
		else if (psg->viomaxcol != pdata->col-1) {
		    psg->vioscrollfreepos = 0;
		    psg->viomaxcol = pdata->col-1;
		}
	    }
	}
	UnLockThread();
    }
    RestoreStacks();
    return(rc);
}


/*  _VioWrtNChar - replacement for VioWrtNChar
 *
 *  This handler has the same parameters as the original VioWrtNChar
 *  function, always returning ROUTER_PASS (-1).  It's sole purpose is
 *  to shadow output on a dumb terminal.  Note that this is not even REPLACED
 *  unless certain Cue options have been selected, such as COM support.
 *
 *  ENTRY   pdata   far ptr to a character
 *	    length  number of times to repeat
 *	    row     row to start printing
 *	    col     column to start printing
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _VioWrtNChar(pdata, length, row, col, hndl)
PCHAR pdata;
USHORT length;
USHORT row, col;
HVIO hndl;
{
    USHORT rc1, rc2;
    register NPSG psg;
    UCHAR seq[5];

    if (SwitchStacks(6))
	return(ROUTER_PASS);
    if (psg=mapsg())
	if (psg->commport && length && row <= 24-1) {
	    seq[0] = ESCAPE;
	    seq[1] = '-';
	    seq[2] = '3';
	    seq[3] = (UCHAR)(' '+length-1);
	    seq[4] = *pdata;
	    ttysetpos(row, col, psg->commhandle);
	    DosWrite(psg->commhandle, seq, 5, &rc1);
	    VioGetCurPos(&rc1, &rc2, hndl);
	    ttysetpos(rc1, rc2, psg->commhandle);
	}
    RestoreStacks();
    return(ROUTER_PASS);
}


/*  _VioWrtCharStrAtt - replacement for VioWrtCharStrAtt
 *
 *  This handler has the same parameters as the original VioWrtCharStrAtt
 *  function, always returning ROUTER_PASS (-1).  It's sole purpose is
 *  to shadow output on a dumb terminal.  Note that this is not even REPLACED
 *  unless certain Cue options have been selected, such as COM support.
 *
 *  ENTRY   pdata   far ptr to a string of characters
 *	    length  number of characters in string
 *	    row     row to start printing
 *	    col     column to start printing
 *	    pattr   far ptr to attribute
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _VioWrtCharStrAtt(pdata, length, row, col, pattr, hndl)
PCHAR pdata;
USHORT length;
USHORT row, col;
PBYTE pattr;
HVIO hndl;
{
    USHORT rc1, rc2;
    register NPSG psg;

    if (SwitchStacks(8))
	return(ROUTER_PASS);
    if (psg=mapsg())
	if (psg->commport && row <= 24-1) {
	    ttysetpos(row, col, psg->commhandle);
	    DosWrite(psg->commhandle, pdata, length, &rc1);
	    VioGetCurPos(&rc1, &rc2, hndl);
	    ttysetpos(rc1, rc2, psg->commhandle);
	}
    RestoreStacks();
    return(ROUTER_PASS);
}


/*  _VioWrtTTY - replacement for VioWrtTTY
 *
 *  This handler has the same parameters as the original VioWrtTTY
 *  function, always returning ROUTER_PASS (-1), unless I have hooked
 *  the function.  Note that this is not even REPLACED unless certain
 *  Cue options have been selected, such as COM support.
 *
 *  ENTRY   pdata   far ptr to a string of characters
 *	    length  number of characters in string
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _VioWrtTTY(pdata, length, hndl)
PCHAR pdata;
USHORT length;
HVIO hndl;
{
    UCHAR cell[2];
    register NPSG psg;
    USHORT row, col;
    register USHORT i, rc = ROUTER_PASS;

    if (SwitchStacks(4))
	return(rc);
    if (psg=mapsg()) {
	psg->vioflags |= VIOFLG_WRTTTY;
	if (LockThread()) {
	  if (psg->commport) {
	    if (psg->commhandle && psg->commrate) {
	      i = 0;
	      while (length--) {
		if ((row = (pdata[i] == BELL)) || pdata[i] == LF) {
		  DosWrite(psg->commhandle, pdata, ++i, &col);
		  VioWrtTTY(pdata, i-row, hndl);
		  pdata += i;
		  i = 0;
		}
		else
		  i++;
	      }
	      if (i) {
		DosWrite(psg->commhandle, pdata, i, &col);
		VioWrtTTY(pdata, i, hndl);
	      }
	    }
	  }
	  rc = ROUTER_DONE;
	  UnLockThread();
	}
	else if (psg->commport) {
	  VioGetCurPos(&row, &col, hndl);
	  if (row > 24-1) {
	    cell[0] = ' ';
	    cell[1] = WHITE;
	    VioScrollUp(0, 0, 24-1, 80-1, 1, cell, hndl);
	    row = 24-1;
	    VioSetCurPos(row, col, hndl);
	  }
	}
	psg->vioflags &= ~VIOFLG_WRTTTY;
    }
    RestoreStacks();
    return(rc);
}


/*  _VioScrollUp - replacement for VioScrollUp
 *
 *  This handler has the same parameters as the original VioScrollUp
 *  function, always returning ROUTER_PASS (-1), unless I have hooked
 *  the function.  This function allows me to implement Cue's
 *  "scroll-back" buffer.
 *
 *  ENTRY   trow    top row
 *	    lcol    left column
 *	    brow    bottom row
 *	    rcol    right column
 *	    lines   number of lines to scroll
 *	    pcell   filler
 *	    hndl
 *	    length  number of characters in string
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _VioScrollUp(trow, lcol, brow, rcol, lines, pcell, hndl)
SHORT trow, lcol, brow, rcol, lines;
PCHAR pcell;
HVIO hndl;
{
    KBDKEYINFO tmpkey;
    PUCHAR buff;
    USHORT freepos, size, rowsize;
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(8))
	return(rc);
    if (psg=mapsg()) {
      #ifdef  KBDFIXED
	if (psg->kbdreg)	/* kludge to get popups more often */
	    KbdPeek(&tmpkey, KBD_HANDLE);
      #endif
	if (psg->commport) {
	  if (trow == 0 && lcol == 0 &&
	      (rcol == -1 || rcol >= 80-1) && (brow == -1 || brow >= 24-1))
	    if (lines == -1)
	      ttyclear(psg->commhandle);
	    else if (lines == 1 && !(psg->vioflags & VIOFLG_WRTTTY)) {
	      ttyhidecursor(psg->commhandle);
	      ttyscrollup(psg->commhandle);
	      VioGetCurPos(&rowsize, &size, hndl);
	      ttysetpos(rowsize, size, psg->commhandle);
	      ttyshowcursor(psg->commhandle);
	    }
	}
	if (psg->vioscrollsize && !(psg->vioflags & VIOFLG_SUSPEND))
	    if (trow == 0 && lcol == 0 &&
		brow >= psg->viomaxrow-1 &&
		rcol >= psg->viomaxcol && lines == 1) {
		    buff = psg->vioscrollbuff;
		    freepos = psg->vioscrollfreepos;
		    size = psg->vioscrollsize;
		    rowsize = (psg->viomaxcol+1)*2;
		    if (freepos >= size) {
			memcpy(buff, buff+rowsize, size-rowsize);
			freepos = size - rowsize;
		    }
		    psg->vioscrollfreepos = freepos + rowsize;
		    VioReadCellStr(buff+freepos, &rowsize, 0, 0, hndl);
	    }
    }
    RestoreStacks();
    return(rc);
}


/*  _VioSetAnsi - replacement for VioSetAnsi
 *
 *  This handler has the same parameters as the original VioSetAnsi
 *  function, always returning ROUTER_PASS (-1), unless I have disabled
 *  the function, in which case I return ROUTER_DONE (0).
 *
 *  ENTRY   f	    TRUE to activate, FALSE to not
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't disable the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _VioSetAnsi(f, hndl)
BOOL f;
HVIO hndl;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(2))
	return(rc);
    if (psg=mapsg())
	if (psg->commport)		// if terminal active
	    if (f)			// they can turn ANSI off...
		rc = ROUTER_DONE;	// but not on
    RestoreStacks();
    return(rc);
}


/*  _VioPrtSc - replacement for VioPrtSc
 *
 *  This handler has the same parameters as the original VioPrtSc
 *  function, always returning ROUTER_PASS (-1), unless I have disabled
 *  the function, in which case I return ROUTER_DONE (0).
 *
 *  ENTRY   hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't disable the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _VioPrtSc(hndl)
HVIO hndl;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(1))
	return(rc);
    if (psg=mapsg())
	if (psg->cueflags & CUESTATE_NOPRT) {
	    beep();
	    rc = ROUTER_DONE;
	}
    RestoreStacks();
    return(rc);
}


/*  _VioPrtScToggle - replacement for VioPrtScToggle
 *
 *  This handler has the same parameters as the original VioPrtScToggle
 *  function, always returning ROUTER_PASS (-1), unless I have disabled
 *  the function, in which case I return ROUTER_DONE (0).
 *
 *  ENTRY   hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't disable the function, ROUTER_DONE
 *	    (0) otherwise
 */

USHORT FAR _VioPrtScToggle(hndl)
HVIO hndl;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(1))
	return(rc);
    if (psg=mapsg())
	if (psg->cueflags & CUESTATE_NOPRT) {
	    beep();
	    rc = ROUTER_DONE;
	}
    RestoreStacks();
    return(rc);
}


/*  _VioScrLock - replacement for VioScrLock
 *
 *  This handler has the same parameters as the original VioScrLock
 *  function, always returning ROUTER_PASS (-1), and simply recording
 *  the lock-state of the screen group.
 *
 *  ENTRY   fWait   TRUE if supposed to wait for a lock
 *	    pStat   returned 0 if successful, 1 if failed
 *	    hvio    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1)
 */

USHORT FAR _VioScrLock(fWait, pStat, hvio)
BOOL fWait;
PBYTE pStat;
HVIO hvio;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(4))
	return(rc);
    if (LockThread()) {
	if (psg=mapsg()) {
	    rc = VioScrLock(fWait, pStat, hvio);
	    if (!rc)
		if (!psg->scrlocks++)
		    DosSemSet(&psg->scrlocksem);
	}
	UnLockThread();
    }
    RestoreStacks();
    return(rc);
}


/*  _VioScrUnLock - replacement for VioScrUnLock
 *
 *  This handler has the same parameters as the original VioScrUnLock
 *  function, always returning ROUTER_PASS (-1), and simply recording
 *  the lock-state of the screen group.
 *
 *  ENTRY   hvio    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1)
 */

USHORT FAR _VioScrUnLock(hvio)
HVIO hvio;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(1))
	return(rc);
    if (LockThread()) {
	if (psg=mapsg()) {
	    rc = VioScrUnLock(hvio);
	    if (!rc)
		if (!--psg->scrlocks)
		    DosSemClear(&psg->scrlocksem);
	}
	UnLockThread();
    }
    RestoreStacks();
    return(rc);
}


/*  _VioGetConfig - replacement for VioGetConfig
 *
 *  This handler has the same parameters as the original VioGetConfig
 *  function, always returning ROUTER_PASS (-1), unless I have hooked
 *  the function.
 *
 *  ENTRY   pdata   far ptr to a VIOCONFIGINFO packet
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise
 *
 *  NOTES   This replacement allows me to fake the display type (as MONO)
 *	    when we are operating on a terminal.
 */

USHORT FAR _VioGetConfig(usRes, pdata, hndl)
USHORT usRes;
PVIOCONFIGINFO pdata;
HVIO hndl;
{
    register NPSG psg;
    register USHORT rc = ROUTER_PASS;

    if (SwitchStacks(4))
	return(rc);
    if (LockThread()) {
	if (psg=mapsg()) {
	    rc = VioGetConfig(usRes, pdata, hndl);
	    if (!rc) {
		if (psg->commport && pdata->cb >= 6) {
		    pdata->adapter = 0; // force to MONO configuration
		    pdata->display = 0;
		}
	    }
	}
	UnLockThread();
    }
    RestoreStacks();
    return(rc);
}


/*  _VioRouter - routes control to appropriate replacement function
 *
 *  ENTRY   ds	    Caller's DS (unused)
 *	    addr    VIOCALLS internal address (unused)
 *	    index   replacement function #
 *	    faraddr far return address of Caller (unused)
 *	    hndl    video handle, usually VIO_HANDLE (0)
 *
 *  EXIT    ROUTER_PASS (-1) if I didn't hook the function, ROUTER_DONE
 *	    (0) otherwise.  The exit code is determined by the replacement
 *	    function.
 */

USHORT FAR cdecl VioRouter(ds, addr, index, faraddr, hndl)
USHORT index, addr, ds, hndl;
ULONG faraddr;
{
    register NPSG psg;
    register USHORT *p;
    PVOID pfn;
    USHORT ohndl;
    USHORT rc = ROUTER_PASS;

    if (!SwitchStacks(VIOROUTERFRAME)) {

      if (psg=mapsg()) {
	if (psg->poplocks) {
	  pfn = VioRouter;
	  if (FP_SEG(faraddr) != FP_SEG(pfn)) {
	    if (index == INDX_VIOSCRLOCK)
		**(PBYTE *)(p+1) = 1;	/* make lock appear unsuccessful */
	    rc = 1;			/* interim measure: */
	    goto fast_exit;		/* error on other threads' I/O */
	  }
	}
	ohndl = hndl;
	if (psg->mousehandle)
	  if (hndl == VIO_FASTHANDLE || hndl == VIO_MSEHANDLE)
	    hndl = VIO_HANDLE;		/* special VIO calls go through */
	  else				/* non-mouse VIO calls hide mouse */
	    hidemouse(psg, VIO_FASTHANDLE);
      }
      p = &hndl;			/* if this list gets much bigger, */
      switch (index) {			/* I'll need an enable-array instead */
      case INDX_VIOGETBUF:
	  rc = _VioGetBuf(*(PULONG *)(p+3), *(PUSHORT *)(p+1), *p);
	  break;
      case INDX_VIOSHOWBUF:
	  rc = _VioShowBuf(*(p+2), *(p+1), *p);
	  break;
      case INDX_VIOGETMODE:
	  rc = _VioGetMode(*(PVIOMODEINFO *)(p+1), *p);
	  break;
      case INDX_VIOSETCURPOS:
	  rc = _VioSetCurPos(*(p+2), *(p+1), *p);
	  break;
      case INDX_VIOSETMODE:
	  rc = _VioSetMode(*(PVIOMODEINFO *)(p+1), *p);
	  break;
      case INDX_VIOWRTNCHAR:
	  rc = _VioWrtNChar(*(PCH *)(p+4), *(p+3), *(p+2), *(p+1), *p);
	  break;
      case INDX_VIOWRTCHARSTRATT:
	  rc = _VioWrtCharStrAtt(*(PCH *)(p+6), *(p+5), *(p+4), *(p+3), *(PBYTE *)(p+1), *p);
	  break;
      case INDX_VIOWRTTTY:
	  rc = _VioWrtTTY(*(PCH *)(p+2), *(p+1), *p);
	  break;
      case INDX_VIOSCROLLUP:
	  rc = _VioScrollUp(*(p+7), *(p+6), *(p+5),
			      *(p+4), *(p+3), *(PCH *)(p+1), *p);
	  break;
      case INDX_VIOSETANSI:
	  rc = _VioSetAnsi(*(p+1), *p);
	  break;
      case INDX_VIOPRTSC:
	  rc = _VioPrtSc(*p);
	  break;
      case INDX_VIOSCRLOCK:
	  rc = _VioScrLock(*(p+3), *(PBYTE *)(p+1), *p);
	  break;
      case INDX_VIOSCRUNLOCK:
	  rc = _VioScrUnLock(*p);
	  break;
      case INDX_VIOPRTSCTOGGLE:
	  rc = _VioPrtScToggle(*p);
	  break;
      case INDX_VIOGETCONFIG:
	  rc = _VioGetConfig(*(p+3), *(PVIOCONFIGINFO *)(p+1), *p);
	  break;
      }
      if (psg)
	if (psg->mousehandle)
	  if (ohndl != VIO_FASTHANDLE && ohndl != VIO_MSEHANDLE)
	    showmouse(psg, VIO_FASTHANDLE);

fast_exit:
      RestoreStacks();
    }
    return(rc);
}
