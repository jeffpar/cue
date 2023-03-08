/*  CUEREG.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs registration-related functions
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



/*  mapsgnum - validate SG number
 *
 *  ENTRY   sgnum   screen group #
 *		    if zero (usual), it is obtained from the Ldt
 *
 *  EXIT    sgnum   the screen group # (either passed or obtained from the
 *		    Ldt) is returned unchanged as long as it falls within the
 *		    allowed range;  otherwise, it is mapped to an arbitrary
 *		    but consistent value in the allowed range
 */

USHORT mapsgnum(sgnum)
register USHORT sgnum;
{
    register USHORT i;

    if (!sgnum)
	sgnum = PLinfoseg->sgCurrent;
    if (!Maxsgnum) {		/* could make this critical section except */
	Maxsgnum = MAXSGNUM;	/* it doesn't matter if done more than once */
	i = PGinfoseg->sgMax;
	if (PGinfoseg->uchMajorVersion*10+PGinfoseg->uchMinorVersion > 100)
	    i += PGinfoseg->csgWindowableVioMax;
	if (i < Maxsgnum)
	    Maxsgnum = i;
    }
    return(sgnum % Maxsgnum);
}


/*  mapssg - map specific SG number to SG data pointer
 *
 *  ENTRY   sgnum   screen group #
 *		    if zero (usual), it is obtained from the Ldt
 *
 *  EXIT    psg     SG data pointer,
 *		    or NULL if none (ie, screen group not registered/mapped)
 */

NPSG mapssg(sgnum)
USHORT sgnum;
{
    register SHORT i;
    register NPSG psg;

    i = Sgmap[mapsgnum(sgnum)]-1;
    if (i < 0)			/* no mapping exists for the sgnum */
	psg = NULL;
    else
	psg = Sg + i;
    return(psg);
}


/*  mapsg - map current SG number to SG data pointer
 *
 *  ENTRY   none
 *
 *  EXIT    psg     SG data pointer,
 *		    or NULL if none (ie, screen group not registered/mapped)
 */

NPSG mapsg()
{
    return(mapssg(0));
}


/*  freesg - release current SG data structure
 *
 *  ENTRY   unmap   TRUE if SG data structure should be released
 *		    FALSE if only associated segments should be released
 *
 *  EXIT    none
 */

VOID freesg(unmap)
USHORT unmap;
{
    register NPSG psg;

    if (psg=mapsg()) {		    /* get SG data pointer */

	freesegments(psg);	    /* insure old segments are released */
				    /* by BOTH current process and Cue server */
	CueSendMessage(CUEMSG_FREESEGS, psg->sgindex);

	psg->kbdreg = FALSE;	    /* reset kbd ownership flag */

	if (unmap) {
	    psg->vioreg = FALSE;    /* free the SG data structure, too */
	    Sgmap[mapsgnum(0)] = 0; /* and zap the map */
	}
    }
}


/*  allocsg - allocate an SG data structure
 *
 *  ENTRY   none
 *
 *  EXIT    psg     SG data pointer,
 *		    or NULL if none available
 */

NPSG allocsg()
{
    register USHORT i;
    register NPSG psg;

    if (psg=mapsg())		    /* when re-using an SG */
	freesg(FALSE);		    /* make sure we free its old segments */
    else {
	LockSem(&Buffsem);
	for (i=0,psg=Sg; i<MAXSG; i++,psg++)
	    if (!psg->vioreg && !psg->kbdreg) {
		psg->sgindex = i;
		psg->vioreg = TRUE;
		break;
	    }
	if (i == MAXSG)
	    psg = NULL;
	else			    /* slap it in the map */
	    Sgmap[mapsgnum(0)] = (CHAR)(i+1);
	UnLockSem(&Buffsem);
    }
    return(psg);
}


/*  getsegments - get addressability to SG data segments
 *
 *  Retrieves the segments allocated for the given screen group.
 *
 *  ENTRY   psg     SG data pointer
 *
 *  EXIT    none
 */

USHORT getsegments(psg)
register NPSG psg;
{
    register USHORT rc = 0;

    if (psg->cuelastpid != PLinfoseg->pidCurrent) {
	psg->cuelastpid = PLinfoseg->pidCurrent;
	rc = DosGetSeg(FP_SEG(psg->kbdhistbuff));
	if (!rc && psg->kbdaliassegsize) {
	    rc = DosGetSeg(FP_SEG(psg->kbdaliasbuff));
	    if (rc) {
		psg->kbdaliassize = 0;
		psg->kbdaliassegsize = 0;
	    }
	}
	if (!rc && psg->vioscrollsize) {
	    rc = DosGetSeg(FP_SEG(psg->vioscrollbuff));
	    if (rc)
		psg->vioscrollsize = 0;
	}
    }
    return(rc);
}


/*  freesegments - release addressability to SG data segments
 *
 *  Releases the segments allocated for the given screen group.
 *
 *  ENTRY   psg     SG data pointer
 *
 *  EXIT    none
 */

USHORT freesegments(psg)
register NPSG psg;
{
    register USHORT rc = ERROR_CUE_NOSG;

    if (!psg)
	psg = mapsg();
    if (psg) {
	rc = DosFreeSeg(FP_SEG(psg->kbdhistbuff));
	if (!rc && psg->kbdaliassegsize)
	    DosFreeSeg(FP_SEG(psg->kbdaliasbuff));
	if (!rc && psg->vioscrollsize)
	    DosFreeSeg(FP_SEG(psg->vioscrollbuff));
    }
    return(rc);
}


/*  readythread - indicate thread is running, stack can be released
 *
 *  ENTRY   psg     SG data pointer
 *
 *  EXIT    none
 */

VOID readythread(psg)
NPSG psg;
{
    DosSemClear(&psg->sgsem.sem);
}


/*  startthread - start up a new thread
 *
 *  ENTRY   fn	    far pointer to thread function
 *
 *  EXIT    rc	    0 if OK, otherwise an error code
 */

USHORT startthread(psg, fn)
register NPSG psg;
PFNTHREAD fn;
{
    PCHAR tmpstack;
    USHORT action;
    register USHORT rc;

    tmpstack = NULL;
    rc = DosAllocSeg(512, (PUSHORT)&tmpstack+1, 0);
    if (!rc) {
	DosSemSet(&psg->sgsem.sem);
	if (DosCreateThread(fn, &action, tmpstack+512))
	    readythread(psg);
	DosSemWait(&psg->sgsem.sem, SEM_WAITFOREVER);
	DosFreeSeg(FP_SEG(tmpstack));
    }
    return(rc);
}


/*  checkthreads - make sure any necessary threads are running
 *
 *  ENTRY   psg     SG data pointer
 *
 *  EXIT    none
 *
 *  NOTES   Currently, there might only be two such threads, and they
 *	    are comm and kbd input threads.  A third thread is created
 *	    automatically by the monitors subsystem.
 */

VOID checkthreads(psg)
register NPSG psg;
{
    USHORT action;
    CHAR commname[5];
    register USHORT rc;

    if (psg->commport) {
	if (!psg->commhandle) { 	/* need a handle */
	    strcpy(commname, "COM1");
	    commname[3] = (char)(psg->commport+'0');
	    rc = DosOpen(commname, &psg->commhandle, &action, 0L,
			 OPEN_ATTNONE,
			 OPEN_EXISTOPEN, OPEN_DENYRW + OPEN_ACCESSRW, 0L);
	    if (rc)
		psg->commport = 0;
	    else {
		DosSetPrty(PRTYS_PROCESSTREE, PRTYC_REGULAR, PRTYD_MAXIMUM, 0);
		startthread(psg, commthread);
	    }
	}
    }
    if (psg->cueflags & CUESTATE_MOUSE) {
	if (!psg->mousehandle)
	    if (MouOpen(NULL, &psg->mousehandle))
		psg->cueflags &= ~CUESTATE_MOUSE;
	    else
		startthread(psg, mousethread);
    }
    else if (psg->mousehandle) {
	MouClose(psg->mousehandle);
	psg->mousehandle = 0;
    }
}


/*  initsg - allocate and initialize an SG data structure
 *
 *  This should only be called from CueRegister after both VIO and
 *  KBD registration have been successfully achieved.
 *
 *  ENTRY   pkt 	near pointer to a Cue registration pakcet
 *
 *  EXIT    0 if OK, otherwise an error code.
 */

USHORT initsg(pkt)
NPCUEREGPKT pkt;
{
    USHORT i, rc;
    register NPSG psg;
    register NPHP php;
    VIOMODEINFO mdata;

    psg = allocsg();
    if (!psg)
	return(ERROR_CUE_NOSG);
    psg->sgsem.sem = 0L;
    psg->sgsem.pid = 0; 		/* set cueflags to a subset */
    psg->cueflags = pkt->flags & CUEINIT_NOPRT;
    psg->cuebasepid = PLinfoseg->pidCurrent;
    psg->cuechildpid = 0;
    psg->cuelastpid = psg->cuebasepid;
    mdata.cb = sizeof(mdata);
    rc = VioGetMode(&mdata, VIO_HANDLE);
    psg->vioflags = 0;
    psg->viomaxrow = mdata.row-1;
    psg->viomaxcol = mdata.col-1;
    psg->commport = pkt->commport;	/* set up comm port info */
    psg->commrate = 0;			/* baud rate unknown at this point */
    psg->commhandle = 0;		/* so is handle */
    psg->mousehandle = 0;
    psg->mouserow = psg->viomaxrow/2;
    psg->mousecol = psg->viomaxcol/2;
    psg->mousestate = FALSE;
    psg->chTypeAhead = 0;
    psg->iEditBuff = 0;
    psg->nEditBuffTotal = 0;
    checkthreads(psg);			/* get us some threads set up */
    pkt->commport = psg->commport;
    psg->poplocks = 0;
    psg->scrlocks = 0;
    psg->scrlocksem = 0L;
    for (i=0; i<MAXPOPUPS; i++)
	psg->popdata[i] = Poptable[i].popdata;
    psg->modordinal = 0;
    psg->kbdaliassize = 0;
    psg->kbdaliassegsize = 0;
    psg->kbdinsptr = 0;
    for (i=0,php=psg->hp; i<MAXHISTBUFFS; i++,php++) {
	php->pid = 0;
	php->kbdaddr = 0xffff;
	php->ltime = 0;
	if (i >= pkt->histbuffs)
	    php->hpsize = 0;
	else
	    php->hpsize = pkt->histsize/pkt->histbuffs;
	php->hpoffset = php->hpsize * i + MAXSTRINGINSIZE + MAXINSERTSIZE;
    }
    psg->vioscrollsize = 0;
    rc = DosAllocSeg(MAXSTRINGINSIZE+MAXINSERTSIZE + pkt->histsize,
			(PSEL)&psg->kbdhistbuff+1, SEG_GETTABLE);
    if (!rc) {
	psg->kbdhistbuff[MAXSTRINGINSIZE] = 0;
	if (pkt->scrollsize) {
	    psg->vioscrollsize = pkt->scrollsize * (mdata.col*2);
	    rc = DosAllocSeg(psg->vioscrollsize,
				(PSEL)&psg->vioscrollbuff+1, SEG_GETTABLE);
	    if (rc)
		psg->vioscrollsize = 0;
	    else
		psg->vioscrollfreepos = 0;
	}
    }
    if (rc)				/* if error(s), release */
	freesg(TRUE);			/* anything we might have alloc'ed */
    return(rc);
}


/*  patch - lock and patch segment containing VIO/KBD function
 *
 *  Also obtains global semaphore to insure that no patching/unlocking
 *  can be performed by other processes.
 *
 *  ENTRY   patchfn	function to patch
 *
 *  EXIT    none
 */

VOID patch(patchfn)
PAPI patchfn;
{
    USHORT hdl, act;
    register USHORT i;
    static PATCHBLK patches[] = {

	{VioRegister,
	 6,
	 {0x83, 0x3e, 0xee, 0x00, 0x00, 0x74},
	 6,
	 {0x83, 0x3e, 0xee, 0x00, 0x00, 0xeb},
	 3,
	 {0xca, 0x10, 0x00}
	}
	,
	{VioDeRegister,
	 6,
	 {0x83, 0x3e, 0xee, 0x00, 0x00, 0x74},
	 6,
	 {0x83, 0x3e, 0xee, 0x00, 0x00, 0xeb},
	 1,
	 {0xcb}
	}
	,
	{KbdRegister,
	 6,
	 {0x83, 0x3e, 0xee, 0x00, 0x00, 0x74},
	 6,
	 {0x83, 0x3e, 0xee, 0x00, 0x00, 0xeb},
	 3,
	 {0xca, 0x0c, 0x00}
	}
	,
	{KbdDeRegister,
	 6,
	 {0x83, 0x3e, 0xee, 0x00, 0x00, 0x74},
	 6,
	 {0x83, 0x3e, 0xee, 0x00, 0x00, 0xeb},
	 1,
	 {0xcb}
	}
    };

    for (i=0; i<sizeof(patches)/sizeof(PATCHBLK); i++)
	if (patchfn == patches[i].pfn)
	    break;

    LockSem(&Buffsem);
    if (i < sizeof(patches)/sizeof(PATCHBLK))
      if (!DosOpen("CUE$", &hdl, &act, 0L, 0, 0x01, 0x42, 0L)) {
	DosDevIOCtl(0, &patches[i], 0x40, 0x83, hdl);
	DosClose(hdl);
      }
}


/*  unlock - unlock segment containing VIO/KBD function
 *
 *  Should be done as soon as patch is no longer needed, to allow the
 *  segment to be unlocked and moved/swapped as needed.  Releases global
 *  semaphore to allow patching/unlocking by other processes.
 *
 *  ENTRY   unlockfn	function to unlock
 *
 *  EXIT    none
 */

VOID unlock(unlockfn)
PAPI unlockfn;
{
    USHORT hdl, act;

    if (!DosOpen("CUE$", &hdl, &act, 0L, 0, 0x01, 0x42, 0L)) {
	DosDevIOCtl(0, &unlockfn, 0x41, 0x83, hdl);
	DosClose(hdl);
    }
    UnLockSem(&Buffsem);
}


/*  CueRegister - register Cue in the current screen group
 *
 *  ENTRY   pregpkt	far pointer to a Cue registration packet
 *
 *  EXIT    0 if OK, otherwise a VIO or KBD error code.
 */

USHORT FAR CueRegister(pregpkt)
PCUEREGPKT pregpkt;
{
    CUEREGPKT pkt;
    USHORT i, rc, rc2;
    register NPSG psg;
    register NPHP php;
    ULONG viomask1, viomask2, kbdmask;

    if (SwitchStacks(2))
	return(ERROR_CUE_NOSTACKS);

    pkt = *pregpkt;
    viomask1 = VR_VIOSETMODE;
 /* viomask1 = VR_VIOSETMODE | VR_VIOSCRLOCK | VR_VIOSCRUNLOCK; */
    viomask2 = 0;
    kbdmask  = KR_KBDCHARIN | KR_KBDPEEK | KR_KBDFLUSHBUFFER | KR_KBDSTRINGIN;

    if (pkt.histsize == 0)
	pkt.histsize = DEFHISTSIZE;
    else if (pkt.histsize < MINHISTSIZE)
	pkt.histsize = MINHISTSIZE;
    else if (pkt.histsize > MAXHISTSIZE)
	pkt.histsize = MAXHISTSIZE;

    if (pkt.histbuffs == 0)
	pkt.histbuffs = DEFHISTBUFFS;
    else if (pkt.histbuffs < MINHISTBUFFS)
	pkt.histbuffs = MINHISTBUFFS;
    else if (pkt.histbuffs > MAXHISTBUFFS)
	pkt.histbuffs = MAXHISTBUFFS;

    if (pkt.scrollsize) {
	viomask1 |= VR_VIOSCROLLUP;
	if (pkt.scrollsize < MINSCROLLROWS)
	    pkt.scrollsize = MINSCROLLROWS;
	else if (pkt.scrollsize > MAXSCROLLROWS)
	    pkt.scrollsize = MAXSCROLLROWS;
    }

    if (pkt.commport) {
	pkt.flags |= CUEINIT_ALL;   /* formerly, viomask1 |= VR_VIOWRTTTY */
	VioSetAnsi(FALSE, VIO_HANDLE);
    }

    if (pkt.flags & CUEINIT_ALL) {
	viomask1 |= 0xffffffff;
	viomask2 |= 0x000001ff;
    }
    else if (pkt.flags & CUEINIT_NOPRT)
	viomask1 |= VR_VIOPRTSC | VR_VIOPRTSCTOGGLE;

    viomask1 &= ~(VR_VIOSAVREDRAWWAIT | VR_VIOSAVREDRAWUNDO |
		  VR_VIOMODEWAIT      | VR_VIOMODEUNDO);
    patch(VioRegister);
    rc = VioRegister("CUESUBS", "_VIOROUTER", viomask1, viomask2);
    unlock(VioRegister);

    psg = mapsg();
    if (rc == ERROR_VIO_REGISTER)
	if (psg)
	  if (psg->vioreg)	    /* already registered! */
	    rc = ERROR_CUE_INSTALLED;

    if (!rc || rc == ERROR_CUE_INSTALLED) {

	if (pkt.flags & CUEINIT_ALL)
	    kbdmask = 0x00003fff;

	patch(KbdRegister);
	rc2 = KbdRegister("CUESUBS", "_KBDROUTER", kbdmask);
	unlock(KbdRegister);

	if (rc) {
	  if (!rc2)		    /* attempt to re-register, in case the */
	    psg->kbdreg = FALSE;    /* officially registered process has died */
	}
	else
	    rc = rc2;

	if (rc2 == ERROR_KBD_REGISTER)
	    if (psg)
	      if (psg->vioreg)	    /* already registered! */
		rc = ERROR_CUE_INSTALLED;
    }

    if (!rc)			    /* assign an SG structure and init it */
	rc = initsg(&pkt);
    else
    if (rc == ERROR_CUE_INSTALLED) {
	pkt.histsize = 0;	    /* or just return buffer sizes for the SG */
	for (i=0,php=psg->hp; i<MAXHISTBUFFS; i++,php++)
	    if (!php->hpsize)
		break;
	    else
		pkt.histsize += php->hpsize;
	pkt.histbuffs = i;
	pkt.scrollsize = psg->vioscrollsize / ((psg->viomaxcol+1)*2);
    }
    *pregpkt = pkt;		    /* return a possibly updated packet */

    RestoreStacks();
    return(rc);
}


/*  CueDeRegister - de-register Cue in the current screen group
 *
 *  ENTRY   none
 *
 *  EXIT    0 if OK, otherwise a VIO or KBD error code.
 */

USHORT FAR CueDeRegister()
{
    register USHORT rc, rc2;

    if (SwitchStacks(0))
	return(ERROR_CUE_NOSTACKS);

    patch(VioDeRegister);
    rc = VioDeRegister();
    unlock(VioDeRegister);

    patch(KbdDeRegister);
    rc2 = KbdDeRegister();
    unlock(KbdDeRegister);

    if (!rc)
	rc = rc2;

    freesg(TRUE);

    RestoreStacks();
    return(rc);
}
