/*  CUEMSE.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Mouse-related functions
 */


#include <dos.h>		/* C macros only (ie, FP_SEG) */

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



/*  mousethread - process mouse input and display a pointer
 *
 *  ENTRY   none
 *
 *  EXIT    none
 */

VOID FAR mousethread()
{
    register USHORT rc;
    register NPSG psg = NULL;
    USHORT wait = 1;
    MOUEVENTINFO moudata;

    if (!SwitchStacks(0))
	psg = mapsg();

    if (psg) {
	readythread(psg);
	DosSetPrty(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0);
	rc = FALSE;
	while (!rc) {
	    showmouse(psg, VIO_MSEHANDLE);
	    rc = MouReadEventQue(&moudata, &wait, psg->mousehandle);
	    hidemouse(psg, VIO_MSEHANDLE);
	    psg->mouserow = moudata.row;
	    psg->mousecol = moudata.col;
	}
    }
}


/*  hidemouse - hide the mouse pointer, restoring screen contents
 *
 *  ENTRY   psg     SG data pointer
 *	    hndl    VIO handle
 *
 *  EXIT    none
 *
 *  NOTES   Needs to block in the VioRouter if another VIO operation
 *	    is in progress, until that operation is complete.  Also note
 *	    that the VioRouter calls this service to hide the mouse
 *	    during other operations.
 */

VOID hidemouse(psg, hndl)
register NPSG psg;
HVIO hndl;
{
    UCHAR cell[2];
    USHORT size = sizeof(cell);

    if (TestAndSet(&psg->mousestate, FALSE, TRUE)) {
	VioReadCellStr(cell, &size, psg->mouserow, psg->mousecol, hndl);
	cell[1] ^= 0x70;
	VioWrtCellStr(cell, size, psg->mouserow, psg->mousecol, hndl);
    }
}


/*  showmouse - show the mouse pointer, modifying screen contents
 *
 *  ENTRY   psg     SG data pointer
 *	    hndl    VIO handle
 *
 *  EXIT    none
 *
 *  NOTES   Needs to block in the VioRouter if another VIO operation
 *	    is in progress, until that operation is complete.  Also note
 *	    that the VioRouter calls this service to show the mouse
 *	    after other operations.
 */

VOID showmouse(psg, hndl)
register NPSG psg;
HVIO hndl;
{
    UCHAR cell[2];
    USHORT size = sizeof(cell);

    if (TestAndSet(&psg->mousestate, TRUE, FALSE)) {
	VioReadCellStr(cell, &size, psg->mouserow, psg->mousecol, hndl);
	cell[1] ^= 0x70;
	VioWrtCellStr(cell, size, psg->mouserow, psg->mousecol, hndl);
    }
}
