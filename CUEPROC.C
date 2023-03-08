/*  CUEPROC.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs process management functions
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



/*  CusSysVer - return OS/2 version number
 *
 *  ENTRY   none
 *
 *  EXIT    1.xx OS/2 version --> 1xx unsigned integer
 */

USHORT FAR CueSysVer()
{
    register USHORT rc;

    SwitchDS();
    rc = PGinfoseg->uchMajorVersion * 10 + PGinfoseg->uchMinorVersion;
    RestoreDS();
    return(rc);
}


/*  CueDetached - return TRUE if caller is a detached process
 *
 *  ENTRY   none
 *
 *  EXIT    TRUE if detached, FALSE if not
 */

USHORT FAR CueDetached()
{
    register USHORT rc, ver;

    SwitchDS();
    ver = CueSysVer();
    if (ver > 100 && PLinfoseg->typeProcess == PT_DETACHED ||
	ver == 100 && PLinfoseg->sgCurrent >= PGinfoseg->sgMax)
	rc = TRUE;
    else
	rc = FALSE;
    RestoreDS();
    return(rc);
}


/*  CueProcName - copy process name to supplied buffer
 *
 *  Peeks at the current process' environment segment to get the
 *  name, and copies it to the specified location.
 *
 *  ENTRY   dest    pointer to supplied buffer
 *	    size    size of buffer
 *	    badenv  TRUE if environment may be damaged (for now,
 *		    program name will be set to a default in that case)
 *
 *  EXIT    rc	    0 or error code
 *
 *  NOTES   If the size of buffer is only MAXNAMESIZE big, then I
 *	    assume all we want is the base portion of the name (8.3).
 *
 */

USHORT FAR CueProcName(dest, size, badenv)
PCHAR dest;
USHORT size, badenv;
{
    PCHAR p, q;
    register USHORT i, rc = 0;

    if (!badenv) {
	p = 0;
	badenv = DosGetEnv((PUSHORT)&p+1, &badenv);
    }
    if (badenv) {		/* if error, use a default process name */
	p = "Parent";
	i = 8;
	rc = ERROR_CUE_NOPROCESS;
    }
    else {			/* otherwise, extract process name */
	while (*p)		/* from the environment segment */
	    p += strlen(p) + 1;
	q = ++p;
	i = strlen(p) + 1;
	if (size == MAXNAMESIZE) {
	    p += (i - 2);
	    i = 2;
	    while (p > q && i < size)
		if (*p && *p != '\\' && *p != ':') {
		    p--;
		    i++;
		}
		else {
		    p++;
		    i--;
		    break;
		}
	}
    }
    while (i--) 		/* copy the string we've found to the dest. */
	dest[i] = p[i];
    return(rc);
}


/*  addprocess - library initialization process tracking
 *
 *  Here, I add a record for this process to the Procdata segment
 *  and setup an exit-list handler.
 *
 *  ENTRY   psg     SG data pointer
 *
 *  EXIT    none
 *
 *  NOTES   Assumes that this will be called from main(), where stacks
 *	    have already been switched.
 */

void addprocess(psg)
register NPSG psg;
{
    register USHORT i, rc = FALSE;

    LockSem(&Procsem);

    if (!Maxprocesses) {	/* allocate segment for proc info if none yet */
	rc = DosAllocSeg(MINPROCESSES * sizeof(PROCDATA),
			    (PSEL)&PProc+1, SEG_GETTABLE);
	if (!rc) {
	    for (i=0; i<MINPROCESSES; i++)
		PProc[i].pid = 0;
	    Maxprocesses = MINPROCESSES;
	}
    }
    else			/* otherwise, just get addressability */
	DosGetSeg(FP_SEG(PProc));

    for (i=0; i<Maxprocesses; i++)
	if (!PProc[i].pid)	/* find an empty slot */
	    break;
    if (i == Maxprocesses)	/* there weren't any, so expand the segment */
	if (Maxprocesses+MINPROCESSES < 65535/sizeof(PROCDATA)) {
	    rc = DosReallocSeg((Maxprocesses+MINPROCESSES)*sizeof(PROCDATA),
				FP_SEG(PProc));
	    if (!rc) {
		for (i=0; i<MINPROCESSES; i++)
		    PProc[Maxprocesses+i].pid = 0;
		i = Maxprocesses;
		Maxprocesses += MINPROCESSES;
	    }
	}
    if (i != Maxprocesses) {	/* if the segment was expanded, add process */
	PProc[i].pid = PLinfoseg->pidCurrent;
	PProc[i].ppid = PLinfoseg->pidParent;
	PProc[i].sgnum = PLinfoseg->sgCurrent;
	PProc[i].pflags = CueDetached() * PROC_DETACHED;

	if (psg)
	  if (!psg->cuebasepid) {
	    rc = TRUE;	    /* pretend error (env may be trash) */
	    psg->cuebasepid = PLinfoseg->pidCurrent;
	  }
	  else if (!psg->cuechildpid && psg->cuebasepid == PLinfoseg->pidParent)
	    psg->cuechildpid = PLinfoseg->pidCurrent;

				/* assuming env ok, get process name from it */
	CueProcName(PProc[i].name, MAXNAMESIZE, rc);
    }
    DosExitList(EXLST_ADD, (PFNEXITLIST)removeprocess);

    UnLockSem(&Procsem);
}


/*  removeprocess - exit-list handler for library
 *
 *  Here, I remove the record for this process, assuming it was
 *  recorded by addprocess().
 *
 *  ENTRY   none
 *
 *  EXIT    none
 *
 *  NOTES   In case the dying process is the one I've recorded as the
 *	    "base" process, and Cue is to remain loaded in this screen
 *	    group (ie, RESIDENT), then I ask the detached Cue process
 *	    to hang onto the selectors for this screen group.
 *
 *	    Ideally, if I could tell that we were returning to the
 *	    shell (ie, that this process was the root process for this
 *	    screen group), I would ask the detached Cue process to
 *	    release the segments I've allocated for this screen group.
 *	    However, the PID of shell process can vary, depending on the
 *	    system configuration, so I must defer the release until the
 *	    screen group is re-used and I discover that I can re-register
 *	    in that screen group.  If this never happens, a certain
 *	    amount of shared memory resources will never be reclaimed, but
 *	    the total is limited by the limit I've placed on total
 *	    supported screen groups.
 */

VOID FAR removeprocess(reason)
USHORT reason;		    /* 1=EXIT, 2=HARD, 3=TRAP, 4=KILL */
{
    register NPSG psg;
    register USHORT i, mypid;

    FreeStacks();	    /* depending on where the process died */
			    /* this will prevent switch_stacks from hanging */
    if (SwitchStacks(1))
	return;

    CheckSem(&Buffsem);     /* make sure these semaphores are also OK */
    CheckSem(&Procsem);
    CheckSem(&Detachsem);

    mypid = PLinfoseg->pidCurrent;
    for (i=0; i < Maxprocesses; i++) {
	if (PProc[i].pid == mypid) {
	    if (psg=mapsg()) {
		CheckSem(&psg->sgsem);
		if (psg->cuebasepid == mypid) {
		    psg->cuebasepid = 0;
		    if (psg->cueflags & CUESTATE_RESIDENT)
			CueSendMessage(CUEMSG_GETSEGS, psg->sgindex);
		    psg->commhandle = 0;    /* order is important! */
		    psg->mousehandle = 0;
		}
		if (psg->cuechildpid == mypid)
		    psg->cuechildpid = 0;
	    }
	    PProc[i].pid = 0;
	    break;
	}
    }
    RestoreStacks();
    DosExitList(EXLST_EXIT, removeprocess);
}


/*  findproc - process string retriever
 *
 *  ENTRY   cmd       retrieval command
 *	    input     additional input
 *	    pref      address of reference data
 *
 *  EXIT    TRUE if successful, FALSE if error
 *	    pref->ptr -> "  SS PPPPP  NNNNNNNNNNNN"
 *
 */

USHORT FAR findproc(cmd, input, pref)
USHORT cmd;
SHORT input;
register POPFNDATA *pref;
{
    USHORT rc = FALSE;
    PPROCDATA p;
    register SHORT count, i;

    LockSem(&Procsem);
    switch(cmd) {

    case POPCMD_INIT:
	pref->index = 0;
	for (i=0; i < Maxprocesses; i++)
	  if (PProc[i].pid && PProc[i].sgnum == PLinfoseg->sgCurrent)
	    break;
	pref->dataptr = PProc+i;
	pref->miscptr = NULL;
	if (DosAllocSeg(32, (PUSHORT)(&pref->miscptr)+1, 0))
	    break;

    case POPCMD_NEAR:
	if (input < 0)
	    input = 0;

    case POPCMD_FIND:
	if (input < 0)
	    break;
	rc = TRUE;
	p = pref->dataptr;
	i = p - PProc;
	count = input - pref->index;
	if (count) {
	  if (count < 0) {
	    count = -count;
	    while (i && count--) {
	      while (i) {
		i--;
		p--;
		if (p->pid && p->sgnum == PLinfoseg->sgCurrent) {
		  pref->index--;
		  pref->dataptr = p;
		  break;
		}
	      }
	    }
	  }
	  else {
	    while (i < Maxprocesses-1 && count--) {
	      while (i < Maxprocesses-1) {
		i++;
		p++;
		if (p->pid && p->sgnum == PLinfoseg->sgCurrent) {
		  pref->index++;
		  pref->dataptr = p;
		  break;
		}
	      }
	    }
	  }
	  if (input != pref->index && cmd == POPCMD_FIND)
	    rc = FALSE;
	}
	if (rc && pref->miscptr) {
	    printnum(0, 0, 4, TRUE, p->sgnum, (PCHAR)(pref->miscptr));
	    *((PCHAR)(pref->miscptr)+4) = ' ';
	    printnum(0, 0, 5, TRUE, p->pid,   (PCHAR)(pref->miscptr)+5);
	    *((PCHAR)(pref->miscptr)+10) = ' ';
	    *((PCHAR)(pref->miscptr)+11) = ' ';
	    printstr(0, 0, 0, FALSE, p->name, (PCHAR)(pref->miscptr)+12);
	    pref->ptr = pref->miscptr;
	}
	break;

    case POPCMD_EXIT:
	DosFreeSeg(FP_SEG(pref->miscptr));
	break;
    }
    UnLockSem(&Procsem);
    return(rc);
}
