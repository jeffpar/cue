/*  CUEHIST.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Cuesubs history management functions
 */


#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cuesubs.h"



NPHP histbuffer(psg, addr)
NPSG psg;
USHORT addr;
{
    register NPHP php, savephp;
    ULONG lowtime = 0xffffffffL;
    USHORT i, j, pid = PLinfoseg->pidCurrent;

    for (i=0,j=0,php=psg->hp; i<MAXHISTBUFFS; i++,php++) {
      if (php->hpsize) {
	j++;			    /* should always be at least ONE */
	if (pid == php->pid)
	    break;
	else if (addr == php->kbdaddr)
	    break;
      }
    }
    if (i >= MAXHISTBUFFS) {	    /* if we didn't find a suitable buffer */
      for (i=0,php=psg->hp; i<j; i++,php++)
	if (php->ltime < lowtime) {
	    savephp = php;	    /* save the most likely so far */
	    lowtime = php->ltime;
	}
      php = savephp;
      php->pid = pid;
      php->kbdaddr = addr;
      php->histpos = 0; 	    /* throw away any history already there */
      php->histfreepos = 0;
    }
    do {
      php->ltime = PGinfoseg->time;
      if (php->ltime == PGinfoseg->time)
	break;			    /* kludge to insure we read time correctly */
    } while (TRUE);
    return(php);
}


VOID addline(psg, php, count)
NPSG psg;
NPHP php;
USHORT count;
{
    register USHORT p, q;
    SHORT size; 		/* unsigned produced constant ovfl */
    USHORT pos, freepos, skip = FALSE;
    UCHAR savec;
    PUCHAR src, dest, buff;

    if (count > 0) {		/* mikem didn't like this at > 2 -JTP */
	src = psg->kbdhistbuff;
	buff = src + php->hpoffset;
	pos = php->histpos;
	dest = buff + pos;
	size = php->hpsize;
	freepos = php->histfreepos;

	if (count >= size)
	    count = size - 1;
	savec = src[count];
	src[count++] = 0;
	if (freepos-pos >= count)
	    if (!strcmp(src, dest))
		skip = TRUE;
	if (!skip) {
	  while (count > size-freepos)
	    freepos = CueStrPrev(buff, freepos);

	  /* memcpy(buff+count, buff, freepos); */
	  size = freepos;
	  for (p = freepos-1,q = p+count; size > 0; size--)
	    buff[q--] = buff[p--];

	  freepos += count;
	  strcpy(buff, src);

	  php->histpos = 0;
	  php->histfreepos = freepos;
	}
	src[--count] = savec;
    }
}


USHORT fetchline(psg, php, pcount, currentsize, history, fwd)
NPSG psg;
register NPHP php;
USHORT pcount, *currentsize, history, fwd;
{
    register USHORT t;
    USHORT pos, freepos;
    PUCHAR src, dest, buff;

    dest = psg->kbdhistbuff;
    buff = dest + php->hpoffset;
    pos = php->histpos;
    src = buff + pos;
    if (!(freepos = php->histfreepos))
	return(FALSE);

    if (fwd) {
	if (!history && pcount == *currentsize) {
	    dest[pcount] = 0;
	    if (!strcmp(dest, src))
		history = TRUE;
	}
	while (TRUE) {
	    t = strlen(src);
	    if (!history)
		break;
	    else {
		pos += ++t;
		if (pos >= freepos)
		    pos = 0;
		php->histpos = pos;
		src = buff + pos;
		history = FALSE;
	    }
	}
    }
    else {
	if (pos >= 4)		/* there must be one more line */
	    pos -= 2;		/* toward the top of the buffer */
	else
	    pos = freepos-2;	/* otherwise, start at the bottom */
	while (pos > 0 && buff[pos])
	    pos--;
	if (pos)		/* if we didn't exit because pos == 0 */
	    pos++;		/* then we backed up onto a null */
	php->histpos = pos;
	src = buff + pos;
	t = strlen(src);	/* preparing to exit */
    }

    strcpy(dest, src);
    *currentsize = t;
    return(TRUE);
}


VOID resetlines(php, zap)
register NPHP php;
USHORT zap;
{
    if (zap)
	php->histfreepos = php->histpos;
    else
	php->histpos = 0;
}


VOID transformline(psg, count, maxlen)
NPSG psg;
USHORT *count;
USHORT maxlen;
{
    register USHORT p, q, size;
    PUCHAR src, dest;

    p = q = 0;
    src = psg->kbdhistbuff;
    while (p++<*count && (*src == ' ' || *src == '\t')) {
	q++;
	src++;			    /* eat leading spaces */
    }
    for (p=0; p<*count-q; p++)
	if (src[p] == ' ' || src[p] == '\t')
	    break;		    /* p has no. chars in potential alias */
    if (psg->cueflags & CUESTATE_PERM) {
	if (PLinfoseg->pidCurrent == psg->cuebasepid)
	    if (!CueStrniCmp(src, "exit", p))
		p = *count = 0;
    }
    if (psg->kbdaliassize && p) {
      getsegments(psg); 	    /* insure addressability */
      dest = psg->kbdaliasbuff;
      while (*dest) {		    /* dest points to current actual alias */
	if (!CueStrniCmp(src, dest, p))
	    break;		    /* dest points to our man */
	dest += strlen(dest)+1;     /* otherwise, skip current actual alias */
	dest += strlen(dest)+1;     /* and its value as well */
      }
      if (*dest) {
	LockSem(&Buffsem);
	dest += strlen(dest)+1;     /* skip actual alias to get its value now */
	size = strlen(dest);	    /* size of the value */
	for (q=0; q<size; q++)
	    Tmpbuff[q] = dest[q];   /* copy value to temp buffer */
	for (; p<*count && q<MAXSTRINGINSIZE*2; p++,q++)
	    Tmpbuff[q] = src[p];    /* copy rest of original line now */
	if (q > maxlen)
	    q = maxlen; 	    /* don't overflow the user's bounds */
	for (p=0; p<q; p++)
	    src[p] = Tmpbuff[p];    /* finally, move new line to currentline */
	*count = q;		    /* return new count */
	UnLockSem(&Buffsem);
      }
    }
}


/*  findhist - history string retriever
 *
 *  ENTRY   cmd       retrieval command
 *	    input     additional input
 *	    pref      address of reference data
 *
 *  EXIT    TRUE if successful, FALSE if error
 */

USHORT FAR findhist(cmd, input, pref)
USHORT cmd;
SHORT input;
register POPFNDATA *pref;
{
    USHORT rc = FALSE;
    PUCHAR firstptr, lastptr;
    register SHORT count, len;

    firstptr = pref->psg->kbdhistbuff + pref->php->hpoffset;
    lastptr = firstptr + pref->php->histfreepos;

    switch(cmd) {

    case POPCMD_INIT:
	pref->index = input = 0;	    /* never use *given* index */
	pref->dataptr = firstptr;
	while (pref->dataptr != firstptr + pref->php->histpos)
	    if (findhist(POPCMD_FIND, input+1, pref))
		input++;
	    else
		break;

    case POPCMD_NEAR:
	if (input < 0)
	    input = 0;

    case POPCMD_FIND:
	if (input < 0)
	    break;
	rc = TRUE;
	count = input - pref->index;
	if (count) {
	  if (input == 0) {		    /* fast reset (can't use INIT) */
	    pref->index = 0;
	    pref->dataptr = firstptr;
	  }
	  else {
	    if (count < 0) {
	      count *= -1;
	      while (count--)
		if (pref->index) {
		  pref->index--;
		  pref->dataptr = firstptr + CueStrPrev(firstptr, (PCHAR)(pref->dataptr)-firstptr);
		}
		else
		  break;
	    }
	    else {
	      while (count--)
		if (pref->dataptr < lastptr) {
		  len = strlen(pref->dataptr)+1;
		  if ((PCHAR)(pref->dataptr)+len < lastptr) {
		    pref->index++;
		    (PCHAR)(pref->dataptr) += len;
		  }
		  else
		    break;
		}
		else
		  break;
	    }
	    if (input != pref->index && cmd == POPCMD_FIND)
	      rc = FALSE;
	  }
	}
	if (pref->dataptr == lastptr)	    /* empty data set */
	  rc = FALSE;
	break;

    case POPCMD_LEFT:
    case POPCMD_RIGHT:
    case POPCMD_ENTER:
	pref->php->histpos = (PCHAR)(pref->dataptr)-firstptr;
	len = strlen(pref->dataptr)+1;
	memcpy(pref->psg->kbdhistbuff, pref->dataptr, len);
	rc = TRUE;
	break;
    }
    pref->ptr = pref->dataptr;		    /* output string same as dataptr */
    return(rc);
}
