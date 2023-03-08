/*  CUETTY.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs terminal-related services
 */


#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"


/*  ttyclear - clear terminal screen
 *
 *  ENTRY   hndl    communication handle
 */

VOID ttyclear(hndl)
USHORT hndl;
{
    USHORT rc;
   #ifdef SOROC
    static UCHAR seq[2] = {ESCAPE, '*'};
   #else
    static UCHAR seq[2] = {ESCAPE, '*'};
   #endif

    DosWrite(hndl, seq, 2, &rc);
}


/*  ttyscrollup - scroll terminal screen
 *
 *  ENTRY   hndl    communication handle
 */

VOID ttyscrollup(hndl)
USHORT hndl;
{
    USHORT rc;
   #ifdef SOROC
    static UCHAR seq[1] = {LF};
   #else
    static UCHAR seq[1] = {LF};
   #endif

    ttysetpos(24-1, 1-1, hndl);
    DosWrite(hndl, seq, 1, &rc);
}


/*  ttysetpos - set cursor position on terminal
 *
 *  ENTRY   row     row to set
 *	    col     column to set
 *	    hndl    communication handle
 */

VOID ttysetpos(row, col, hndl)
USHORT row;
USHORT col;
USHORT hndl;
{
   #ifdef SOROC
    static UCHAR seq[4] = {ESCAPE, '='};
   #else
    static UCHAR seq[4] = {ESCAPE, 'Y'};
   #endif

    if (row > 24-1)
	row = 24-1;
    seq[2] = (UCHAR)(row+' ');
    seq[3] = (UCHAR)(col+' ');
    DosWrite(hndl, seq, 4, &col);
}


/*  ttyhidecursor - hide cursor on terminal
 *
 *  ENTRY   hndl    communication handle
 */

VOID ttyhidecursor(hndl)
USHORT hndl;
{
    USHORT u;
    static UCHAR seq[7] = {ESCAPE, '.', '2', '6', '0', '0', 14};

   #ifdef SOROC
    DosWrite(hndl, seq, 7, &u);
   #endif
}


/*  ttyshowcursor - show cursor on terminal
 *
 *  ENTRY   hndl    communication handle
 */

VOID ttyshowcursor(hndl)
USHORT hndl;
{
    USHORT u;
    static UCHAR seq[7] = {ESCAPE, '.', '2', '6', '0', '1', 14};

   #ifdef SOROC
    DosWrite(hndl, seq, 7, &u);
   #endif
}
