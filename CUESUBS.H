/*  CUESUBS.H
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Header for all Cuesubs modules
 */


#include <limits.h>

#include "cue.h"


#define MAXSG		16	/* maximum screen groups supported */

#define MAXSGNUM	MAXSG*2

/* MAXSG defines the total number of screen groups/sessions Cue will
   support simultaneously.  When Cue is registered for a physical
   screen group/session number, that number is mapped (by mapsg) to
   a pointer to one of MAXSG logical screen group data structures.
   Physical screen group/session numbers are assigned by the operating
   system as follows:

    0	Hard errors
    1	Shell
    2	3.x Box
    3	VIO pop-ups
    4	1st full-screen VIO
	...
   16	1st windowed VIO (in 1.0, detached processes go here)
	...
   32	1st presentation mgr ID
	...
 */


#define MINHISTSIZE	128	/* other CUESUBS mins/maxs */
#define MAXHISTSIZE	16384
#define DEFHISTSIZE	4096

#define MINHISTBUFFS	1
#define MAXHISTBUFFS	4
#define DEFHISTBUFFS	2

#define EDITBUFFSIZE	80

#define MINSCROLLROWS	10
#define MAXSCROLLROWS	400	/* MAXSCROLLROWS*MAXSCROLLCOLS*2 */
#define MAXSCROLLCOLS	80	/* must not exceed 64k segment size! */

#define MAXACTIVEPOPUPS 4	/* based on typical stack size and usage */

#define MAXINSERTSIZE	128	/* keystroke insert buffer */

#define MINPROCESSES	20	/* processes to grow per-process data seg by */
#define MAXNAMESIZE	14	/* max "file" name size */
#define MAXPNAMESIZE	128	/* max "path" name size */
#define PROC_DETACHED	0x0001	/* per-process flag bit definitions */


#define NULLFRAME	-1	/* reserved SwitchStacks() arguments */
#define VIOROUTERFRAME	-2
#define KBDROUTERFRAME	-3
#define MOUROUTERFRAME	-4

#define ROUTER_DONE	0	/* return values for router functions */
#define ROUTER_PASS	0xffff

#define VIOFLG_SUSPEND	0x01	/* bits for vioflags */
#define VIOFLG_WRTTTY	0x02

#define MONBUFFSIZE	128	/* monitor-related data structures */


#define THIN_VERT	0x00	/* pop-up box flavors */
#define THICK_VERT	0x01
#define THIN_HORIZ	0x00
#define THICK_HORIZ	0x02
#define SCROLL_BAR	0x04	/* scroll bar active */
#define SCROLL_REVERSE	0x08	/* bottom-to-top motion */
#define SCROLL_NONE	0x10	/* no scrolling of any kind */
#define SCROLL_FULLSCR	0x20	/* no border */
#define SCROLL_FIXED	0x40	/* no paging, expanding, or shrinking */
#define SCROLL_ATTR	0x80	/* strings contain attributes */

#define POPCMD_INIT	1	/* pop-function commands */
#define POPCMD_HELP	2
#define POPCMD_FIND	3
#define POPCMD_NEAR	4
#define POPCMD_LEFT	5
#define POPCMD_RIGHT	6
#define POPCMD_ENTER	7
#define POPCMD_RESIZE	8
#define POPCMD_EXIT	9


#define MOUSE_VISIBLE	0x0001


typedef struct {
	USHORT	   monflgs;
	KBDKEYINFO key;
	USHORT	   kbddflgs;
} MONPKT;

typedef MONPKT FAR *PMONPKT;


typedef struct {
	MONPKT pkt;
	USHORT result;
	USHORT bufflen1;	/* must be filled in prior to reg. */
	USHORT ddbufflen1;	/* filled in by DOSMONREG upon error */
	CHAR   buffer1[MONBUFFSIZE-4];
	USHORT bufflen2;	/* must be filled in prior to reg. */
	USHORT ddbufflen2;	/* filled in by DOSMONREG upon error */
	CHAR   buffer2[MONBUFFSIZE-4];
} MONDATA;

typedef MONDATA FAR *PMONDATA;


typedef struct {		/* fast/safe (Quick) semaphore */
    ULONG sem;			/* for Dos semaphore api */
    USHORT pid; 		/* pid of owner (zero if none or unknown) */
} QSEM;

typedef QSEM NEAR *NPQSEM;


typedef struct {		/* history partition */
    USHORT pid; 		/* associated pid */
    USHORT kbdaddr;		/* associated KBDSTRINGIN input addr */
    ULONG  ltime;		/* last access time for this partition */
    USHORT hpsize;		/* size of this partition */
    USHORT hpoffset;		/* offset within seg. of this partition */
    USHORT histpos;		/* cur. recall pos. within partition */
    USHORT histfreepos; 	/* cur. free pos. within partition */
} HPDATA;

typedef HPDATA NEAR *NPHP;


typedef struct {		/* screen group data element */
    USHORT   sgindex;		/* corres. index for this element */
    QSEM     sgsem;		/* for inter-thread synchronization */
    UCHAR    vioreg;		/* ownership flag for element */
    UCHAR    kbdreg;		/* kbd ownership flag (ie, call issued) */
    USHORT   cueflags;		/* ie, PERM, ALTENT, etc */
    USHORT   cuebasepid;	/* process from which Cue was run */
    USHORT   cuechildpid;	/* track a direct descendant */
    USHORT   cuelastpid;	/* last process we did getsegments() for */
    USHORT   viomaxrow; 	/* VIO fields maintained on VioSetMode */
    USHORT   viomaxcol;
    USHORT   commport;		/* if non-zero, comm port */
    USHORT   commrate;		/* baud rate for comm port */
    USHORT   commhandle;	/* current comm handle */
    PMONDATA mondata;
    USHORT   mousehandle;	/* current mouse handle */
    USHORT   mouserow;
    USHORT   mousecol;
    USHORT   mousestate;
    UCHAR    vioflags;		/* ie, SUSPEND */
    UCHAR    kbdturnaround;	/* getkbdstatus() fills this in */
    USHORT   kbdmode;		/* this, too */
    PUCHAR   kbdhistbuff;	/* what getsegments() keeps around */
    PUCHAR   kbdaliasbuff;
    PUCHAR   vioscrollbuff;
    USHORT   kbdinsptr; 	/* buffer sizes, positions, etc */
    USHORT   kbdaliassize;
    USHORT   kbdaliassegsize;
    USHORT   vioscrollsize;
    USHORT   vioscrollfreepos;
    USHORT   poplocks;		/* total pop-ups active this SG */
    USHORT   scrlocks;		/* screen-lock count */
    ULONG    scrlocksem;	/* screen-lock semaphore */
    POPDATA  popdata[MAXPOPUPS];/* all dynamic pop-up data for this SG */
    STR8     modname;
    USHORT   modordinal;
    HPDATA   hp[MAXHISTBUFFS];	/* history partitions */
    USHORT   chTypeAhead;
    USHORT   iEditBuff;
    USHORT   nEditBuffTotal;
    UCHAR    achEditBuff[EDITBUFFSIZE];
} SGDATA;

typedef SGDATA NEAR *NPSG;


typedef struct {		/* per-process data element */
    USHORT sgnum;		/* physical screen group # */
    USHORT pid;
    USHORT ppid;
    USHORT pflags;
    CHAR   name[MAXNAMESIZE];
} PROCDATA;

typedef PROCDATA FAR *PPROCDATA;


typedef struct {		/* pop-up function data */
    NPSG    psg;
    NPHP    php;
    NPPOPDATA ppop;
    USHORT  index;
    PVOID   dataptr;
    PVOID   miscptr;
    PCHAR   ptr;
} POPFNDATA;

typedef USHORT (FAR *PPOPFN)(USHORT, SHORT, POPFNDATA *);


typedef struct {		/* static pop-up data */
    PCHAR    hdg;
    PPOPFN   popfn;
    POPDATA  popdata;
} POPTABLE;

typedef POPTABLE FAR *PPOPTABLE;


typedef struct {		/* internal Cue data */
    USHORT stacksbusy;
    USHORT stacksheld;
    USHORT stacksalloc;
    USHORT stacksmax;
    USHORT stackswitches;
    USHORT stacksize;
    USHORT stacksizeused;
} CUEINFOBLOCK;

typedef CUEINFOBLOCK NEAR *NPCUEINFOBLOCK;


/* external functions */

VOID   SwitchDS(VOID);
VOID   RestoreDS(VOID);
USHORT SwitchStacks(SHORT);
VOID   RestoreStacks(VOID);
VOID   FreeStacks(VOID);
NPCUEINFOBLOCK GetInfo(VOID);
VOID   SaveSignals(VOID);
VOID   RestoreSignals(VOID);
VOID   HoldSignals(VOID);
VOID   ReleaseSignals(VOID);
USHORT LockThread(VOID);
VOID   UnLockThread(VOID);
VOID   LockSem(NPQSEM);
VOID   UnLockSem(NPQSEM);
VOID   CheckSem(NPQSEM);
USHORT FAR TestAndSet(USHORT *, USHORT, USHORT);
BYTE   FAR InB(USHORT);
VOID   FAR OutB(USHORT, BYTE);


/* function prototypes */

    /* cuesubs.c */

USHORT FAR main(VOID);

    /* cuereg.c */

USHORT	 mapsgnum(USHORT);
NPSG	 mapssg(USHORT);
NPSG	 mapsg(VOID);
VOID	 freesg(USHORT);
NPSG	 allocsg(VOID);
USHORT	 getsegments(NPSG);
USHORT	 freesegments(NPSG);
VOID	 readythread(NPSG);
USHORT	 startthread(NPSG, PFNTHREAD);
VOID	 checkthreads(NPSG);
USHORT	 initsg(NPCUEREGPKT);
VOID	 patch(PAPI);
VOID	 unlock(PAPI);

    /* cuepop.c */

USHORT	 CuePopup(PPOPTABLE, NPSG, NPHP, HKBD);
VOID	 showbar(POPDATA *, USHORT, BOOL);

    /* cuefind.c */

USHORT FAR findhelp(USHORT, SHORT, POPFNDATA *);
USHORT FAR findstat(USHORT, SHORT, POPFNDATA *);
USHORT FAR findline(USHORT, SHORT, POPFNDATA *);
USHORT FAR findcolor(USHORT, SHORT, POPFNDATA *);
USHORT FAR findchar(USHORT, SHORT, POPFNDATA *);

    /* cuekbd.c */

USHORT	 getkbdstatus(NPSG, USHORT, HKBD);
USHORT	 setkbdstatus(NPSG, USHORT, HKBD);
VOID	 xlatekey(NPSG, KBDKEYINFO *, HKBD);
USHORT	 getkey(NPSG, KBDKEYINFO *, HKBD);
BOOL	 processkey(NPSG, PKBDKEYINFO, USHORT, HKBD);

    /* cuevio.c */

USHORT	 savebox(NPSG, USHORT, USHORT, USHORT, USHORT, PCHAR *);
VOID	 restorebox(NPSG, PCHAR);
VOID	 drawbox(USHORT, USHORT, USHORT, USHORT, USHORT, UCHAR);
USHORT	 printstr(SHORT, SHORT, SHORT, BOOL, PCHAR, PCHAR);
VOID	 printnum(SHORT, SHORT, SHORT, BOOL, USHORT, PCHAR);

    /* cuekreg.c */

USHORT FAR _KbdCharin(PKBDKEYINFO, USHORT, HKBD);
USHORT FAR _KbdPeek(PKBDKEYINFO, HKBD);
USHORT FAR _KbdFlushBuffer(HKBD);
USHORT FAR _KdbStringIn(PCHAR, PSTRINGINBUF, USHORT, HKBD);
USHORT FAR cdecl KbdRouter(USHORT, USHORT, USHORT, ULONG, USHORT);

    /* cuetty.c */

VOID	 ttyclear(USHORT);
VOID	 ttyscrollup(USHORT);
VOID	 ttysetpos(USHORT, USHORT, USHORT);
VOID	 ttyhidecursor(USHORT);
VOID	 ttyshowcursor(USHORT);

    /* cuevreg.c */

USHORT FAR _VioSetMode(PVIOMODEINFO, HVIO);
USHORT FAR _VioWrtTTY(PCHAR, USHORT, HVIO);
USHORT FAR _VioScrollUp(SHORT, SHORT, SHORT, SHORT, SHORT, PCHAR, HVIO);
USHORT FAR _VioPrtSc(HVIO);
USHORT FAR _VioPrtScToggle(HVIO);
USHORT FAR cdecl VioRouter(USHORT, USHORT, USHORT, ULONG, USHORT);

    /* cuemse.c */

VOID FAR mousethread(VOID);
VOID	 hidemouse(NPSG, HVIO);
VOID	 showmouse(NPSG, HVIO);

    /* cueksup.c */

VOID	 setinsert(NPSG, USHORT, HKBD);
VOID	 homeline(USHORT, USHORT *);
VOID	 endline(USHORT, USHORT, USHORT *);
VOID	 eraseline(USHORT, USHORT *);
VOID	 addchar(UCHAR, USHORT *, USHORT *, USHORT *);
VOID	 backup(USHORT, USHORT);
VOID	 subchar(PUCHAR, USHORT, USHORT, USHORT, USHORT, USHORT *, USHORT *);
VOID	 multchar(PUCHAR, USHORT, USHORT *, USHORT *, USHORT *);
VOID	 replacechar(PUCHAR, USHORT, UCHAR, NPSG, USHORT, USHORT, USHORT *);
VOID	 replaceline(PUCHAR, USHORT, USHORT, USHORT *, USHORT *, USHORT *);

    /* cuehist.c */

NPHP	 histbuffer(NPSG, USHORT);
VOID	 addline(NPSG, NPHP, USHORT);
USHORT	 fetchline(NPSG, NPHP, USHORT, USHORT *, USHORT, USHORT);
VOID	 resetlines(NPHP, USHORT);
VOID	 transformline(NPSG, USHORT *, USHORT);
USHORT FAR findhist(USHORT, SHORT, POPFNDATA *);

    /* cueproc.c */

VOID	 addprocess(NPSG);
VOID FAR removeprocess(USHORT);
USHORT FAR findproc(USHORT, SHORT, POPFNDATA *);

    /* cuecomm.c */

VOID FAR kbdthread(VOID);
VOID FAR commthread(VOID);
VOID	 sendsignal(NPSG, USHORT);
VOID	 mapkey(PKBDKEYINFO, BOOL);

    /* cueutil.c */

VOID	 beep(VOID);



/* external data */

extern FARCHAR Helphelp;


/* data definitions */

#ifdef CUESUBS_DATA

PGINFOSEG PGinfoseg;		/* define global Cuesubs data */
PLINFOSEG PLinfoseg;
PPROCDATA PProc;
BYTE	  Sgmap[MAXSGNUM];
SGDATA	  Sg[MAXSG];
CUEMSG	  Msg;
QSEM	  Buffsem;
QSEM	  Procsem;
QSEM	  Detachsem;
QSEM	  Detachstartsem;
QSEM	  Detachstopsem;
USHORT	  Maxsgnum;
USHORT	  Srvpid;
USHORT	  Processes;
USHORT	  Maxprocesses;
UCHAR	  Tmpbuff[MAXSTRINGINSIZE*2];
POPTABLE  far Poptable[MAXPOPUPS] = {
	    {"\xB5 CUE Help \xC6", findhelp,
		{ 4, 13, 17, 67,  0,  CYAN*BCKGRD+BRIGHT+WHITE,
		 THIN_VERT|THICK_HORIZ, -1}
	    },
	    {"\xB5 CUE History \xC6", findhist,
		{ 3, 18, 14, 62,  0,  BLUE*BCKGRD+BRIGHT+WHITE,
		 THIN_VERT|THICK_HORIZ|SCROLL_BAR|SCROLL_REVERSE, -1}
	    },
	    {"\xB5 CUE Processes \xC6", findproc,
		{ 5, 22, 14, 58,  0,  GREEN*BCKGRD+BRIGHT+WHITE,
		 THICK_VERT|THICK_HORIZ, -1}
	    },
	    {"\xB5 CUE Status \xC6", findstat,
		{ 8, 25, 18, 55,  0,  RED*BCKGRD+BRIGHT+WHITE,
		 THIN_VERT|THICK_HORIZ|SCROLL_NONE, -1}
	    },
	    {NULL,    findline,
		{ 0,  0,  0,  0,  0,  0,
		 SCROLL_BAR|SCROLL_REVERSE|SCROLL_FULLSCR|SCROLL_ATTR, -1}
	    },
	    {"\xB4 CUE Colors \xC3", findcolor,
		{ 6, 31, 23, 48,  0,  BLACK*BCKGRD+BRIGHT+WHITE,
		 THIN_VERT|THIN_HORIZ|SCROLL_BAR|SCROLL_FIXED|SCROLL_ATTR, -1}
	    },
	    {"\xB4 CUE Characters \xC3", findchar,
		{ 7, 20, 16, 61,  0,  BLUE*BCKGRD+BRIGHT+WHITE,
		 THIN_VERT|THIN_HORIZ|SCROLL_BAR, -1}
	    }
	  };

#else

extern PGINFOSEG PGinfoseg;	/* declare global Cuesubs data */
extern PLINFOSEG PLinfoseg;
extern PPROCDATA PProc;
extern BYTE	 Sgmap[MAXSGNUM];
extern SGDATA	 Sg[MAXSG];
extern CUEMSG	 Msg;
extern QSEM	 Buffsem;
extern QSEM	 Procsem;
extern QSEM	 Detachsem;
extern QSEM	 Detachstartsem;
extern QSEM	 Detachstopsem;
extern USHORT	 Maxsgnum;
extern USHORT	 Srvpid;
extern USHORT	 Processes;
extern USHORT	 Maxprocesses;
extern UCHAR	 Tmpbuff[MAXSTRINGINSIZE*2];
extern POPTABLE  far Poptable[MAXPOPUPS];

#endif
