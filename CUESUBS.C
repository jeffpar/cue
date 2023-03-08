/*  CUESUBS.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs dynlink library (main module)
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#define CUESUBS_DATA		/* define global Cuesubs data */
#include "cuesubs.h"



/*  main - library initialization entry point
 *
 *  ENTRY   none
 *
 *  EXIT    1, indicating a successful initialization
 */

USHORT FAR main()
{
    register NPSG psg;

    if (SwitchStacks(NULLFRAME))
	return(FALSE);
    psg=mapsg();
    addprocess(psg);
    if (psg) {
	getsegments(psg);
	checkthreads(psg);
    }
    RestoreStacks();
    return(TRUE);
}


/*  CueSetState - sets various Cue options for the current SG
 *
 *  ENTRY   fsState	bit flags
 *	    fsStateSet	bit flags indicating what to set
 *	    uCommrate	new baud rate, if non-zero
 *
 *  EXIT    0		(no errors)
 */

USHORT FAR CueSetState(fsState, fsStateSet, uCommrate)
USHORT fsState;
USHORT fsStateSet;
USHORT uCommrate;
{
    DCBLOCK dcb;
    BYTE b;
    USHORT port;
    register NPSG psg;

    if (SwitchStacks(3))
	return(ERROR_CUE_NOSTACKS);

    if (psg=mapsg()) {
	fsState &= fsStateSet;		/* just to be safe... */
	psg->cueflags &= ~fsStateSet;	/* now clear all bits to be set */
	psg->cueflags |= fsState;	/* OR in the bits we wanted to set */
	if (psg->commport && psg->commhandle && uCommrate) {
	    psg->commrate = uCommrate;
	    DosDevIOCtl(0, &psg->commrate,
			    COM_SETRATE, COM_CATEGORY, psg->commhandle);
	    if (psg->commrate == 19200) {
		port = 0x3F8;
		if (psg->commport == 2)
		    port = 0x2F8;
		b = InB(port+3);
		OutB(port+3, b | 0x80);
		OutB(port, 0x06);
		OutB(port+1, 0);
		OutB(port+3, b);
	    }
	    DosDevIOCtl(&dcb, 0,
			    COM_GETDCB, COM_CATEGORY, psg->commhandle);
	    dcb.flags1 = 0;
	    dcb.flags2 = 0;
	    DosDevIOCtl(0, &dcb,
			    COM_SETDCB, COM_CATEGORY, psg->commhandle);
	    ttyclear(psg->commhandle);
	}
	checkthreads(psg);
    }
    RestoreStacks();
    return(0);
}


/*  CueSetAliases - store aliases for the current SG
 *
 *  ENTRY   pbuff   far pointer to series of asciz strings
 *	    size    total size of all strings, including final extra NULL
 *
 *  EXIT    rc	    0 or error code
 */

USHORT FAR CueSetAliases(pbuff, size)
PCHAR pbuff;
USHORT size;
{
    register NPSG psg;
    register USHORT rc = 0;

    if (SwitchStacks(3))
	return(ERROR_CUE_NOSTACKS);

    if (psg=mapsg())
      if (size == 0)
	psg->kbdaliassize = 0;
      else {
	if (psg->kbdaliassegsize) {
	    DosGetSeg(FP_SEG(psg->kbdaliasbuff));
	    if (size > psg->kbdaliassegsize) {
		rc = DosReallocSeg(size, FP_SEG(psg->kbdaliasbuff));
		if (!rc)
		    psg->kbdaliassegsize = size;
	    }
	}
	else {
	    rc = DosAllocSeg(size, (PSEL)&psg->kbdaliasbuff+1, SEG_GETTABLE);
	    if (!rc) {
		psg->kbdaliassegsize = size;
		psg->cuebasepid = PLinfoseg->pidCurrent;
		CueDetach(NULL);
	    }	    /* change base pid so that during process termination */
	}	    /* we will ask the detached process to hold this segment */
	if (!rc) {
	    psg->kbdaliassize = size;
	    for (rc=0; rc < size; rc++)
		psg->kbdaliasbuff[rc] = pbuff[rc];
	    rc = 0;
	}
      }
    RestoreStacks();
    return(rc);
}


/*  CueGetAliases - retrieve aliases for the current SG
 *
 *  ENTRY   pbuff   far pointer to buffer to hold asciz strings
 *
 *  EXIT    rc	    0 or error code
 */

USHORT FAR CueGetAliases(pbuff)
PCHAR pbuff;
{
    register NPSG psg;
    register USHORT rc = ERROR_CUE_NOALIASES;

    if (SwitchStacks(2))
	return(ERROR_CUE_NOSTACKS);

    if (psg=mapsg())
      if (psg->kbdaliassize) {
	rc = DosGetSeg(FP_SEG(psg->kbdaliasbuff));
	if (!rc) {
	    for (rc=0; rc < psg->kbdaliassize; rc++)
		pbuff[rc] = psg->kbdaliasbuff[rc];
	    rc = 0;
	}
      }
    RestoreStacks();
    return(rc);
}


/*  CueInsertInput - store data to be used as kbd input
 *
 *  ENTRY   pbuff   far pointer to buffer of asciz kbd data to simulate
 *
 *  EXIT    rc	    0 or error code
 */

USHORT FAR CueInsertInput(pbuff)
PCHAR pbuff;
{
    PCHAR dest;
    register NPSG psg;
    register USHORT rc = 0;

    if (SwitchStacks(2))
	return(ERROR_CUE_NOSTACKS);

    if (psg=mapsg()) {
	rc = DosGetSeg(FP_SEG(psg->kbdhistbuff));
	if (!rc) {
	    psg->kbdinsptr = 0;
	    dest = psg->kbdhistbuff + MAXSTRINGINSIZE;
	    for (rc=0; rc < MAXINSERTSIZE; rc++)
		if (!(dest[rc] = pbuff[rc]))
		    break;
	    rc = 0;
	}
    }
    RestoreStacks();
    return(rc);
}


/*  CueDetach - called when detached Cue server is required
 *
 *  When Cue wishes to load permanently within a screen group, it
 *  must call this routine to insure that a detached process (also
 *  called the Cue server) is running (or can be started).  If the
 *  detached process must be started, this routine will wait
 *  until it is running and ready to receive messages.
 *
 *  ENTRY   prog    far ptr to name of Cue server
 *		    or NULL to use name of calling process
 *  EXIT    none
 */

USHORT FAR CueDetach(prog)
PCHAR prog;
{
    RESULTCODES rcs;
    register NPSG psg;
    static CHAR pname[MAXPNAMESIZE];
    register USHORT rc = ERROR_CUE_NOSG;

    if (SwitchStacks(2))
	return(ERROR_CUE_NOSTACKS);

    if (psg=mapsg()) {
	LockSem(&Detachsem);
	psg->cueflags |= CUESTATE_RESIDENT;

	if (!Srvpid) {
	    DosSemSet(&Detachstopsem);
	    if (prog)
		strcpy(pname, prog);
	    else
		CueProcName(pname, MAXPNAMESIZE, FALSE);

	    rc = DosExecPgm(0, 0, EXEC_BACKGROUND, 0, 0, &rcs, pname);
	    if (!rc)
		DosSemWait(&Detachstopsem, SEM_WAITFOREVER);
	}
	else
	    rc = ERROR_CUE_DETACHED;
	UnLockSem(&Detachsem);
    }
    RestoreStacks();
    return(rc);
}


/*  CueUnDetach - called when detached Cue server to be terminated
 *
 *  When Cue wishes to unload from a screen group, it should
 *  call this routine to verify that no processes are active in
 *  screen groups with the RESIDENT option set, and terminate the
 *  detached server process if there are none.
 *
 *  ENTRY   none
 *
 *  EXIT    Returns ERROR_CUE_NOPROCESS if server process not running,
 *	    or ERROR_CUE_DETACHED if cannot be terminated at this time.
 */

USHORT FAR CueUnDetach()
{
    register USHORT i;
    register NPSG psg;
    USHORT mypid, rc = ERROR_CUE_NOPROCESS;

    if (SwitchStacks(0))
	return(ERROR_CUE_NOSTACKS);

    LockSem(&Detachsem);
    if (Srvpid) {
	rc = 0;
	mypid = PLinfoseg->pidCurrent;
	LockSem(&Buffsem);
	for (i=0; i<Maxprocesses; i++)
	    if (PProc[i].pid && PProc[i].pid != Srvpid && PProc[i].pid != mypid)
		if (psg=mapssg(PProc[i].sgnum))
		    if (psg->cueflags & CUESTATE_RESIDENT)
			rc = ERROR_CUE_DETACHED;
	UnLockSem(&Buffsem);
    }
    UnLockSem(&Detachsem);
    if (!rc)
	CueSendMessage(CUEMSG_EXIT, NULL);

    RestoreStacks();
    return(rc);
}


/*  CueGetMessage - entry point for detached Cue server
 *
 *  The given msg structure is filled in within the appropriate
 *  message data and returned.	If no message has been issued, then
 *  the routine blocks.
 *
 *  ENTRY   pmsg    far ptr to a message structure
 *
 *  EXIT    none
 */

USHORT FAR CueGetMessage(pmsg)
PCUEMSG pmsg;
{
    static SHORT lock = -1;

    if (SwitchStacks(2))
	return(ERROR_CUE_NOSTACKS);

    if (++lock != 0) {
	--lock; 		    /* don't need another detached */
	return(ERROR_CUE_INSTALLED);/* Cue server, thank you anyway */
    }
    Srvpid = PLinfoseg->pidCurrent;
    Msg = *pmsg;
    DosSemSet(&Detachstartsem);
    DosSemClear(&Detachstopsem);
    DosSemWait(&Detachstartsem, SEM_WAITFOREVER);
    *pmsg = Msg;

    lock--;			    /* we can unlock now */

    RestoreStacks();
    return(0);
}


/*  CueSendMessage - send a message to the detached Cue server
 *
 *  The given msg code and SG index are passed as a "message"
 *  to the detached Cue process (if there is one), and waits for
 *  the message to be processed.
 *
 *  ENTRY   code    message code (see CUEMSG_ constants in cue.h)
 *	    sgindex SG structure index
 *
 *  EXIT    0	    if message processed successfully
 *	    other   if message processing resulted in an error
 */

USHORT FAR CueSendMessage(code, sgindex)
USHORT code, sgindex;
{
    register USHORT rc = ERROR_CUE_NOPROCESS;

    if (SwitchStacks(2))
	return(ERROR_CUE_NOSTACKS);

    LockSem(&Detachsem);
    if (Srvpid) {
	Msg.code = code;
	Msg.sg = sgindex;
	DosSemSet(&Detachstopsem);
	DosSemClear(&Detachstartsem);
	if (code != CUEMSG_EXIT) {
	    DosSemWait(&Detachstopsem, SEM_WAITFOREVER);
	    rc = Msg.returncode;
	}
	else {
	    Srvpid = 0;
	    rc = 0;
	}
    }
    UnLockSem(&Detachsem);

    RestoreStacks();
    return(rc);
}


/*  CueGetSegments - entry point for detached Cue server
 *
 *  This entry point is where the detached Cue process comes to
 *  retrieve the segments I've allocated for a given screen group.
 *
 *  ENTRY   sgindex SG structure index
 *
 *  EXIT    none
 *
 *  NOTES   On the assumption this is only called when the "base"
 *	    process in a screen group dies, and only BY the detached
 *	    server process, I close the commhandle (if any) for the
 *	    detached process, to avoid a sharing violation when the new
 *	    "base" process for the screen group tries to open it.
 */

USHORT FAR CueGetSegments(sgindex)
USHORT sgindex;
{
    USHORT rc;
    register NPSG psg;

    if (SwitchStacks(1))
	return(ERROR_CUE_NOSTACKS);

    psg = Sg+sgindex;
    if (psg->commport && psg->commhandle) {
	DosClose(psg->commhandle);
    }
    rc = getsegments(psg);
    if (rc)		    /* if I couldn't obtain addressability */
	freesg(TRUE);	    /* then I should remove this SG from the map */

    RestoreStacks();
    return(rc);
}


/*  CueFreeSegments - entry point for detached Cue server
 *
 *  This entry point is where the detached Cue process comes to
 *  release the segments I've allocated for a given screen group.
 *
 *  ENTRY   sgindex SG structure index
 *
 *  EXIT    none
 */

USHORT FAR CueFreeSegments(sgindex)
USHORT sgindex;
{
    USHORT rc;

    if (SwitchStacks(1))
	return(ERROR_CUE_NOSTACKS);

    rc = freesegments(Sg+sgindex);

    RestoreStacks();
    return(rc);
}


/*  CueSetPopup - updates pop-up table for specified pop-up
 *
 *  ENTRY   i	    pop-up index
 *	    ppop    far ptr to pop-up description data
 *
 *  EXIT    none
 */

USHORT FAR CueSetPopup(i, ppop)
USHORT i;
PPOPDATA ppop;
{
    return(0);
}
