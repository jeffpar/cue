/*  CUEUTIL.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  CUESUBS miscellaneous utility functions
 */


#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



/*  beep - make a friendly double-beep
 *
 *  ENTRY   none
 *
 *  EXIT    none
 */

VOID beep()
{
    DosBeep(500, 50);
    DosSleep(50L);
    DosBeep(500, 50);
}


/*  CueToLower - returns lower-case form of alpha character
 *
 *  ENTRY   c	    character code
 *
 *  EXIT    lower-case form if alphabetic, unchanged otherwise
 */

SHORT FAR CueToLower(i)
register SHORT i;
{
    if (i >= 'A' && i <= 'Z')
	i += 'a'-'A';
    return(i);
}


/*  CueIsAlphaNum - returns TRUE only if char is alphanumeric
 *
 *  ENTRY   c	    character code
 *
 *  EXIT    TRUE if alphanumeric, FALSE otherwise
 */

SHORT FAR CueIsAlphaNum(c)
register SHORT c;
{
    c = CueToLower(c);
    if (c >= 'a' && c <= 'z' || c >= '1' && c <= '9')
	return(TRUE);
    else
	return(FALSE);
}


/*  CueStrniCmp - case-insensitive string comparison
 *
 *  ENTRY   dest    far ptr to string1
 *	    src     far ptr to string2
 *	    n	    maximum compare length
 *
 *  EXIT    -1 if string1 < string2
 *	     0 if string1 = string2
 *	     1 if string1 > string2
 */

SHORT FAR CueStrniCmp(dest, src, n)
PCHAR dest, src;
SHORT n;
{
    register SHORT c = 0;
    register USHORT i = 0;

    while (n && !c) {
	if (!dest[i] || !src[i])
	    break;
	c = CueToLower(dest[i])-CueToLower(src[i]);
	i++;
	n--;
    }
    if (n > 0)
	c = 1;
    else if (src[i])
	c = -1;
    return(c);
}


/*  CueStrPrev - returns position of previous null-terminated string
 *
 *  ENTRY   buff    far ptr to buffer of strings
 *	    pos     index into buffer
 *
 *  EXIT    new index into buffer
 */

USHORT FAR CueStrPrev(buff, pos)
PUCHAR buff;
USHORT pos;
{
    register USHORT p;

    for (p = pos-2; p > 0 && buff[p]; p--)
	;
    return(p + (p != 0));
}


/*  CueStrToken - returns position and length of next token in string
 *
 *  ENTRY   pstr    far ptr to string
 *	    pos     far ptr to string index
 *
 *  EXIT    length of next token
 */

USHORT FAR CueStrToken(pstr, ppos)
PUCHAR pstr;
PUSHORT ppos;
{
    register USHORT i = *ppos, n = 0;

    while (pstr[i] && (pstr[i] == ' ' || pstr[i] == '\t'))
	i++;			    /* skip leading whitespace */
    *ppos = i;
    while (pstr[i] && (pstr[i] != ' ' && pstr[i] != '\t')) {
	i++;
	n++;			    /* adjust size to return */
    }
    return(n);
}
