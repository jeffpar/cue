/*  CUEPOP.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs pop-up functions
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"

#define MODE_NONE   0
#define MODE_MOVE   1
#define MODE_EXPAND 2
#define MODE_COLLAPSE 3

#define REDRAW_NONE	0
#define REDRAW_INSIDE	1
#define REDRAW_ALL	2
#define REDRAW_MODEONLY 3


/*  CuePopup - activate popup if not already activated
 *
 *  ENTRY   ppoptbl pop-up table info pointer
 *	    psg     SG data pointer (NULL for current)
 *	    php     history partition pointer (NULL for none)
 *	    hndl    keyboard handle
 *
 *  EXIT    0	    pop-up escaped (or unable to even activate)
 *	    1	    pop-up item selected via ENTER (ie, ENTER keys)
 *	    2	    pop-up item selected via SELECT (ie, RIGHT/LEFT keys)
 */

USHORT CuePopup(ppoptbl, psg, php, hndl)
PPOPTABLE ppoptbl;
NPSG psg;
NPHP php;
HKBD hndl;
{
    register USHORT i;
    PCHAR pbuff;
    POPDATA pop, pop2;
    PPOPFN popfn;
    POPFNDATA ref;
    SHORT row, direction, border;
    USHORT priority;
    USHORT width, height;
    USHORT bar, barindex, rc;
    PCHAR p, bardataptr;
    UCHAR nodata, adjust, redraw, mode;
    KBDKEYINFO kdata;
    UCHAR blankcell[2];


    if (SwitchStacks(5))
	return(ERROR_CUE_NOSTACKS);

    HoldSignals();

    if (!psg)			    /* if caller doesn't know psg */
	psg=mapsg();		    /* then figure it out */

    if (!psg->viomaxrow) {	    /* unsupported video mode */
	beep(); 		    /* beep to indicate cannot popup */
	goto fast_exit;
    }

    p = (PCHAR)Poptable;
    if (FP_SEG(ppoptbl) == FP_SEG(p)) {
	row = ppoptbl - Poptable;   /* if this is one of my popups */
	if (++psg->popdata[row].lock != 0) {
	    psg->popdata[row].lock--;
	    goto fast_exit;
	}
	else			    /* then snap-shot the popdata for this SG */
	    pop = psg->popdata[row];
    }
    else
	pop = ppoptbl->popdata;     /* otherwise, just use the given popdata */

    DosSemWait(&psg->scrlocksem, 2000L);
    psg->poplocks++;		    /* we'll wait up to 2 secs for the unlock */

    DosGetPrty(PRTYS_THREAD, &priority, 0);
    DosSetPrty(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0);

    if (pop.lrr > psg->viomaxrow) {
	row = pop.lrr-psg->viomaxrow;
	if (pop.ulr < row)
	    pop.ulr = 0;
	else
	    pop.ulr -= row;
	pop.lrr = psg->viomaxrow;
    }
    if (pop.lrc > psg->viomaxcol) {
	row = pop.lrc-psg->viomaxcol;
	if (pop.ulc < row)
	    pop.ulc = 0;
	else
	    pop.ulc -= row;
	pop.lrc = psg->viomaxcol;
    }
    bar = 0;
    mode = MODE_NONE;
    nodata = FALSE;
    border = 1;
    direction = 1;
    blankcell[0] = ' ';
    if (pop.style & SCROLL_BAR)
	bar = 1;
    if (pop.style & SCROLL_FULLSCR) {
	border = 0;
	pop.ulr = pop.ulc = 0;
	pop.lrr = psg->viomaxrow;
	pop.lrc = psg->viomaxcol;
	pop.row = 0;
	psg->vioflags |= VIOFLG_SUSPEND;
    }
    pop.lock = (CHAR)border;
    if (pop.style & SCROLL_REVERSE) {
	direction = -1;
	if (bar)			/* bar starts at bottom */
	    bar = pop.lrr-pop.ulr+1-border*2;
    }

    if (savebox(psg, pop.ulr, pop.ulc, pop.lrr, pop.lrc, &pbuff)) {

	if (!php) {
	    for (i=0; i<MAXHISTBUFFS; i++)
	      if (psg->hp[i].hpsize)
		php = psg->hp + i;
	}
	if (!border && bar)
	    bar = ((PPOPDATA)(pbuff))->row + 1;
	popfn = ppoptbl->popfn;
	ref.psg = psg;
	ref.php = php;
	ref.ppop = &pop;
	ref.miscptr = pbuff;
	if (!(*popfn)(POPCMD_INIT, pop.row, &ref)) {
	    bar = 0;
	    nodata++;
	}
	pop.row = ref.index;
	redraw = REDRAW_ALL;
	kdata.chChar = 0;

	while (kdata.chChar != ESCAPE) {

    /* Major drawing logic */

	  if (redraw == REDRAW_ALL) {	    /* redraw box */
	    width = pop.lrc-pop.ulc+1-border*2;
	    height = pop.lrr-pop.ulr+1-border*2;
	    if (border) {
	      drawbox(pop.ulr, pop.ulc, pop.lrr, pop.lrc,
		      pop.style & (THICK_HORIZ|THICK_VERT), (UCHAR)pop.col);
	      printstr(pop.ulr, pop.ulc+1, -(width), TRUE, ppoptbl->hdg, NULL);
	    }
	  }

	  if (redraw >= REDRAW_ALL) {
	    rc = (pop.col+0x40) & 0x7F;
	    if (mode == MODE_MOVE)
	      VioWrtCharStrAtt(" MOVE ", 6, pop.lrr, pop.lrc-9, (PBYTE)&rc, VIO_HANDLE);
	    else if (mode == MODE_EXPAND)
	      VioWrtCharStrAtt(" EXPAND ", 8, pop.lrr, pop.lrc-10, (PBYTE)&rc, VIO_HANDLE);
	    else if (mode == MODE_COLLAPSE)
	      VioWrtCharStrAtt(" COLLAPSE ", 10, pop.lrr, pop.lrc-11, (PBYTE)&rc, VIO_HANDLE);
	    else
	      drawbox(pop.lrr, pop.ulc, pop.lrr, pop.lrc,
		      pop.style & (THICK_HORIZ|THICK_VERT), (UCHAR)pop.col);
	    if (redraw > REDRAW_ALL)
	      redraw = REDRAW_NONE;
	  }

	  if (redraw && redraw <= REDRAW_ALL) {
	    adjust = 0; 		    /* redraw contents */
	    row = pop.ulr+border;
	    showbar(&pop, bar, FALSE);
	    while (row <= pop.lrr-border) {
	      if (direction > 0)
		rc = pop.row+row-pop.ulr-border;
	      else
		rc = pop.row+pop.lrr-row-border;
	      rc = (*popfn)(POPCMD_FIND, rc, &ref);
	      p = ref.ptr;
	      if (!rc) {
		p = "";
		if (bar)
		  if (direction > 0 && !adjust) {
		    bar = row-pop.ulr-border;
		    barindex = ref.index;
		    bardataptr = ref.dataptr;
		    adjust = 2; 	/* got the last good row */
		  }
		  else if (row == pop.ulr+bar+border-1)
		    adjust = 1; 	/* get the next good row */
	      }
	      else {
		if (adjust == 1)	/* set bar now */
		  bar = row-pop.ulr-border+1;
		if (row == pop.ulr+bar+border-1) {
		  barindex = ref.index;
		  bardataptr = ref.dataptr;
		  adjust = 2;		/* got the current good row */
		}
	      }
	      if (!nodata || row != pop.ulr+height/2)
		if (!(pop.style & SCROLL_ATTR))
		  printstr(row, pop.ulc+border, width, FALSE, p, NULL);
		else
		  if (rc)
		    VioWrtCellStr(p, width*2, row, pop.ulc+border, VIO_HANDLE);
		  else {
		    blankcell[1] = WHITE;
		    VioWrtNCell(blankcell, width, row, pop.ulc+border, VIO_HANDLE);
		  }
	      else
		printstr(row, pop.ulc+border, width, TRUE, "<No Information>", NULL);
	      row++;
	    }
	    showbar(&pop, bar, TRUE);	/* highlight row bar within pop */
	  }
	  redraw = REDRAW_NONE;

    /* Input */

	  getkey(psg, &kdata, hndl);	/* wait for a key */

    /* Enter can mean SELECT,
       or EXIT if nothing to select */

	  if (kdata.chChar == psg->kbdturnaround) {
	    rc = 0;
	    if (!bar)
	      break;
	    else {
	      ref.index = barindex;
	      ref.dataptr = bardataptr;
	      if ((*popfn)(POPCMD_ENTER, 0, &ref)) {
		rc = 1;
		break;
	      }
	    }
	  }

    /* ASCII search section */

	  if (kdata.chChar > 32 && kdata.chChar < 127) {
	    if (pop.style & SCROLL_NONE)
	      continue;
	    kdata.chChar = (UCHAR)CueToLower(kdata.chChar);
	    if (bar)
	      row = barindex;
	    else
	      row = pop.row;
	    while (TRUE) {
	      if (!(*popfn)(POPCMD_FIND, ++row, &ref))
		break;
	      adjust = 0;
	      for (p=ref.ptr,rc=0; *p && rc < psg->viomaxcol*2+2; p+=1+1-border,rc+=1+1-border)
		if (*p != ' ' && *p != '\t') {
		  adjust++;
		  break;
		}
	      if (adjust && kdata.chChar == (UCHAR)CueToLower(*p)) {
		showbar(&pop, bar, FALSE);
		if (!bar || ref.index-pop.row >= height) {
		  pop.row = ref.index;
		  if (bar)
		    if (direction > 0)
		      bar = 1;
		    else
		      bar = height;
		  redraw = REDRAW_INSIDE;
		}
		else {
		  bar = ref.index-pop.row+1;
		  if (direction < 0)
		    bar = height-bar+1;
		  showbar(&pop, bar, TRUE);
		  barindex = ref.index;
		  bardataptr = ref.dataptr;
		}
		break;
	      }
	    }
	  }

    /* Help */

	  if (kdata.chScan == F1) {
	    if (!(*popfn)(POPCMD_HELP, 0, &ref))
	      CuePopup(Poptable+POPUP_HELP, psg, NULL, hndl);
	    continue;
	  }

    /* Pop-up color selection */

	  if (kdata.chScan == ALT_F) {
	    showbar(&pop, bar, FALSE);
	    i = pop.col + 0x0001;
	    pop.col = (pop.col & 0xFFF0) | (i & 0x000F);
	    redraw = REDRAW_ALL;
	    continue;
	  }
	  if (kdata.chScan == ALT_B) {
	    showbar(&pop, bar, FALSE);
	    i = pop.col + 0x0010;
	    if (psg->cueflags & CUESTATE_SETSTATE)
		i &= 0x00F0;
	    else
		i &= 0x0070;
	    pop.col = (pop.col & 0xFF0F) | i;
	    redraw = REDRAW_ALL;
	    continue;
	  }

    /* Pop-up mode selection */

	  if (border && !(pop.style & SCROLL_FIXED))
	    if (kdata.chScan == CENTER || kdata.chScan == CTRL_CENTER ||
		kdata.chChar == ESCAPE && mode) {
	      if (kdata.chChar != ESCAPE) {
		mode++;
		if (mode > MODE_COLLAPSE)
		  mode = MODE_NONE;
	      }
	      else {
		mode = MODE_NONE;
		kdata.chChar = 0;
	      }
	      redraw = REDRAW_MODEONLY;
	      continue;
	    }

    /* End of the ASCII stuff */

	  if (kdata.chChar) {
	    rc = 0;
	    continue;
	  }

    /* Pop-up move/expand/collapse section */

	  if (border && mode) {
	    i = 0;
	    pop2 = pop;
	    rc = strlen(ppoptbl->hdg) + 1;
	    if (kdata.chScan == LEFT) {
	      i++;
	      if (mode == MODE_EXPAND) {
		if (pop.ulc >= 4)
		  pop.ulc -= 4;
		else
		  pop.ulc = 0;
	      }
	      else if (mode == MODE_COLLAPSE) {
		if (pop.lrc >= pop.ulc+rc+4)
		  pop.lrc -= 4;
		else
		  pop.lrc = pop.ulc+rc;
	      }
	      else if (pop.ulc >= 4) {
		pop.ulc -= 4;
		pop.lrc -= 4;
	      }
	      else {
		pop.ulc = 0;
		pop.lrc = width+border*2-1;
	      }
	    }
	    else if (kdata.chScan == RIGHT) {
	      i++;
	      if (mode == MODE_EXPAND) {
		if (pop.lrc <= psg->viomaxcol-4)
		  pop.lrc += 4;
		else
		  pop.lrc = psg->viomaxcol;
	      }
	      else if (mode == MODE_COLLAPSE) {
		if (pop.ulc+rc+4 <= pop.lrc)
		  pop.ulc += 4;
		else
		  pop.ulc = pop.lrc-rc;
	      }
	      else if (pop.lrc <= psg->viomaxcol-4) {
		pop.lrc += 4;
		pop.ulc += 4;
	      }
	      else {
		pop.lrc = psg->viomaxcol;
		pop.ulc = pop.lrc - (width+border*2) + 1;
	      }
	    }
	    else if (kdata.chScan == UP) {
	      i++;
	      if (mode == MODE_EXPAND) {
		if (pop.ulr >= 2)
		  pop.ulr -= 2;
		else
		  pop.ulr = 0;
	      }
	      else if (mode == MODE_COLLAPSE) {
		if (pop.lrr >= pop.ulr+4)
		  pop.lrr -= 2;
		else
		  pop.lrr = pop.ulr+2;
	      }
	      else if (pop.ulr >= 2) {
		pop.ulr -= 2;
		pop.lrr -= 2;
	      }
	      else {
		pop.ulr = 0;
		pop.lrr = height+border*2 - 1;
	      }
	    }
	    else if (kdata.chScan == DOWN) {
	      i++;
	      if (mode == MODE_EXPAND) {
		if (pop.lrr <= psg->viomaxrow-2)
		  pop.lrr += 2;
		else
		  pop.lrr = psg->viomaxrow;
	      }
	      else if (mode == MODE_COLLAPSE) {
		if (pop.ulr+4 <= pop.lrr)
		  pop.ulr += 2;
		else
		  pop.ulr = pop.lrr-2;
	      }
	      else if (pop.lrr <= psg->viomaxrow-2) {
		pop.lrr += 2;
		pop.ulr += 2;
	      }
	      else {
		pop.lrr = psg->viomaxrow;
		pop.ulr = pop.lrr - (height+border*2) + 1;
	      }
	    }
	    if (memcmp(&pop, &pop2, sizeof(pop))) {
	      showbar(&pop2, bar, FALSE);
	      pop.lock = pop2.lock;
	      if (direction < 0)
		if (pop.lrr-pop.ulr != pop2.lrr-pop2.ulr) {
		  row = (pop.lrr-pop.ulr) - (pop2.lrr-pop2.ulr);
		  if (bar)
		    if (row < 0)
		      if (-row >= bar)
			bar = 1;
		      else
			bar += row;
		    else
		      bar += row;
		}
	      if (bar && pop.ulr+bar >= pop.lrr)
		bar = pop.lrr-pop.ulr-1;
	      restorebox(psg, pbuff);
	      if (!savebox(psg, pop.ulr, pop.ulc, pop.lrr, pop.lrc, &pbuff)) {
		rc = 0;
		goto error_exit;
	      }
	      redraw = REDRAW_ALL;
	      continue;
	    }
	    if (i)			/* if key was arrow */
	      continue; 		/* but wasn't used, skip it now */
	  }

    /* Nothing else can be done
       for pop-ups without scrolling/selecting */

	  if (nodata || pop.style & SCROLL_NONE)
	    continue;

    /* Major scrolling stuff:
       Right, Left, Down, Up, Pgdn, Pgup, Home, and End */

	  if (kdata.chScan == RIGHT || kdata.chScan == LEFT) {
	    if (bar) {
	      ref.index = barindex;
	      ref.dataptr = bardataptr;
	      if (kdata.chScan == LEFT)
		rc = (*popfn)(POPCMD_LEFT, 0, &ref);
	      else
		rc = (*popfn)(POPCMD_RIGHT, 0, &ref);
	      if (rc) {
		rc = 2;
		break;
	      }
	    }
	  }
	  else if (kdata.chScan == DOWN || kdata.chScan == CTRL_DN) {
	    adjust = TRUE;
	    if (bar && kdata.chScan == CTRL_DN)
	      if (!(*popfn)(POPCMD_FIND, pop.row+direction, &ref))
		adjust = FALSE;
	    if (bar && adjust) {
	      ref.index = barindex;
	      ref.dataptr = bardataptr;
	      if (!(*popfn)(POPCMD_FIND, ref.index+direction, &ref))
		adjust--;
	      else {
		barindex = ref.index;
		bardataptr = ref.dataptr;
		if (bar < height && kdata.chScan == DOWN) {
		  adjust--;		/* no contents adjustment needed */
		  showbar(&pop, bar, FALSE);
		  bar++;		/* advance bar only */
		}
	      }
	    }
	    if (adjust) 		/* if contents must be adjusted */
	      if ((*popfn)(POPCMD_FIND, pop.row+direction, &ref)) {
		showbar(&pop, bar, FALSE);
		blankcell[1] = (UCHAR)pop.col;
		VioScrollUp(pop.ulr+border, pop.ulc+border,
			    pop.lrr-border, pop.lrc-border, 1, blankcell, VIO_HANDLE);
		if (direction > 0)
		  if (!(*popfn)(POPCMD_FIND, pop.row+height, &ref))
		    adjust--;
		if (adjust)
		  if (!(pop.style & SCROLL_ATTR))
		    printstr(pop.lrr-border, pop.ulc+border, -width, FALSE, ref.ptr, NULL);
		  else
		    VioWrtCellStr(ref.ptr, width*2, pop.lrr-border, pop.ulc+border, VIO_HANDLE);
		pop.row += direction;
	      }
	    showbar(&pop, bar, TRUE);
	  }
	  else if (kdata.chScan == UP || kdata.chScan == CTRL_UP) {
	    adjust = TRUE;
	    if (bar && kdata.chScan == CTRL_UP)
	      if (!(*popfn)(POPCMD_FIND, pop.row-direction, &ref))
		adjust = FALSE;
	    if (bar && adjust) {
	      ref.index = barindex;
	      ref.dataptr = bardataptr;
	      if (!(*popfn)(POPCMD_FIND, ref.index-direction, &ref))
		adjust--;
	      else {
		barindex = ref.index;
		bardataptr = ref.dataptr;
		if (bar > 1 && kdata.chScan == UP) {
		  adjust--;		/* no contents adjustment needed */
		  showbar(&pop, bar, FALSE);
		  bar--;		/* advance bar only */
		}
	      }
	    }
	    if (adjust) 		/* if contents must be adjusted */
	      if ((*popfn)(POPCMD_FIND, pop.row-direction, &ref)) {
		showbar(&pop, bar, FALSE);
		blankcell[1] = (UCHAR)pop.col;
		VioScrollDn(pop.ulr+border, pop.ulc+border,
			    pop.lrr-border, pop.lrc-border, 1, blankcell, VIO_HANDLE);
		if (direction < 0)
		  if (!(*popfn)(POPCMD_FIND, pop.row+height, &ref))
		    adjust--;
		if (adjust)
		  if (!(pop.style & SCROLL_ATTR))
		    printstr(pop.ulr+border, pop.ulc+border, -width, FALSE, ref.ptr, NULL);
		  else
		    VioWrtCellStr(ref.ptr, width*2, pop.ulr+border, pop.ulc+border, VIO_HANDLE);
		pop.row -= direction;
	      }
	    showbar(&pop, bar, TRUE);
	  }
	  else if (kdata.chScan == PGDN) {
	    if (!(pop.style & SCROLL_FIXED)) {
	      (*popfn)(POPCMD_NEAR, pop.row+height*direction, &ref);
	      if (pop.row != ref.index) {
		pop.row = ref.index;
		redraw = REDRAW_INSIDE;
	      }
	    }
	  }
	  else if (kdata.chScan == PGUP) {
	    if (!(pop.style & SCROLL_FIXED)) {
	      (*popfn)(POPCMD_NEAR, pop.row-height*direction, &ref);
	      if (pop.row != ref.index) {
		pop.row = ref.index;
		redraw = REDRAW_INSIDE;
	      }
	    }
	  }
	  else if (kdata.chScan == HOME) {
	    if (bar) {
	      if (bar != 1) {
		rc = TRUE;
		if (direction < 0)
		  rc = (*popfn)(POPCMD_FIND, barindex+1, &ref);
		if (rc) {
		  showbar(&pop, bar, FALSE);
		  bar = 1;
		  redraw = REDRAW_INSIDE;
		}
	      }
	    }
	    else
	      kdata.chScan = CTRL_HOME;
	  }
	  else if (kdata.chScan == END) {
	    if (bar) {
	      if (bar != height) {
		rc = TRUE;
		if (direction > 0)
		  rc = (*popfn)(POPCMD_FIND, barindex+1, &ref);
		if (rc) {
		  showbar(&pop, bar, FALSE);
		  bar = height;
		  redraw = REDRAW_INSIDE;
		}
	      }
	    }
	    else
	      kdata.chScan = CTRL_END;
	  }

    /* Rest of the scrolling stuff:
       Ctrl+Home and Ctrl+End, in case Home and End were converted */

	  if (kdata.chScan == CTRL_HOME && direction > 0 ||
		   kdata.chScan == CTRL_END  && direction < 0) {
	    if (pop.row != 0 || barindex != 0) {
	      (*popfn)(POPCMD_NEAR, 0, &ref);
	      pop.row = 0;
	      if (bar) {
		showbar(&pop, bar, FALSE);
		if (direction > 0)
		  bar = 1;
		else
		  bar = height;
	      }
	      redraw = REDRAW_INSIDE;
	    }
	  }
	  else if (kdata.chScan == CTRL_END  && direction > 0 ||
		   kdata.chScan == CTRL_HOME && direction < 0) {
	    (*popfn)(POPCMD_NEAR, SHRT_MAX, &ref);
	    rc = ref.index;
	    (*popfn)(POPCMD_NEAR, ref.index-height+1, &ref);
	    if (ref.index > pop.row)
	      pop.row = ref.index;
	    else if (barindex == rc)
	      continue;
	    if (bar) {
	      showbar(&pop, bar, FALSE);
	      if (direction < 0)
		bar = 1;
	      else
		bar = height;
	    }
	    redraw = REDRAW_INSIDE;
	  }

	}   /* end of while */

error_exit:
	(*popfn)(POPCMD_EXIT, 0, &ref);
	if (pbuff)
	    restorebox(psg, pbuff);

    }  /* end of successful savebox */

    if (!border)
	psg->vioflags &= ~VIOFLG_SUSPEND;

    DosSetPrty(PRTYS_THREAD, priority>>8, 0, 0);

    psg->poplocks--;		    /* reduce total active popups this SG */

    p = (PCHAR)Poptable;
    if (FP_SEG(ppoptbl) == FP_SEG(p)) {
	row = ppoptbl - Poptable;   /* if this is one of my popups */
	psg->popdata[row].ulr = pop.ulr;
	psg->popdata[row].ulc = pop.ulc;
	psg->popdata[row].lrr = pop.lrr;
	psg->popdata[row].lrc = pop.lrc;
	if (bar)
	    if (direction > 0)
		bar--;
	    else
		bar = height-bar;
	if (!nodata)
	    psg->popdata[row].row = pop.row + bar;
	psg->popdata[row].col = pop.col;
	psg->popdata[row].lock--;
    }

fast_exit:
    ReleaseSignals();

    RestoreStacks();

    return(rc);
}


/*  showbar - display reverse-video bar at specified line
 *
 *  ENTRY   ppop    pointer to popdata info
 *	    bar     line to display bar (relative to 1)
 *	    on	    TRUE to turn bar ON, FALSE to turn it OFF
 *
 *  EXIT    none
 *	    ppop->lock is modified to reflect new bar state
 */

VOID showbar(ppop, bar, on)
register POPDATA *ppop;
USHORT bar;
BOOL on;
{
    USHORT len;
    PCHAR p;
    register USHORT u, i;

    if (bar && !(ppop->lock & 0x80) == on) {
	if (!on)
	    ppop->lock &= ~0x80;
	if ((ppop->style & SCROLL_ATTR) && (ppop->style & SCROLL_FIXED)) {
	    if (on)
		p = " \x10 ";
	    else
		p = "   ";
	    VioWrtCharStr(p, 3, ppop->ulr+bar-1+ppop->lock,
				ppop->ulc+ppop->lock, VIO_HANDLE);
	}
	else {
	    len = (ppop->lrc - ppop->ulc + 1 - ppop->lock*2)*2;
	    LockSem(&Buffsem);
	    VioReadCellStr(Tmpbuff, &len, ppop->ulr+bar-1+ppop->lock,
					ppop->ulc+ppop->lock, VIO_HANDLE);
	    for (u=1; u<len; u+=2) {
		i = Tmpbuff[u];
		if (on)
		    i += 0x40;		/* magic value to change attributes by */
		else
		    i -= 0x40;
		Tmpbuff[u] = (UCHAR)(i & 0x7F);
	    }

	    VioWrtCellStr(Tmpbuff, len, ppop->ulr+bar-1+ppop->lock,
					ppop->ulc+ppop->lock, VIO_HANDLE);
	    UnLockSem(&Buffsem);
	}
	if (on)
	    ppop->lock |= 0x80;
    }
}
