/*  CUE.H
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Header for all CUE and CUESUBS modules
 */


/* Common equates */

#define MAXARGSIZE  128 	/* internal limits */
#define MAXALIASBUF 16384

#define TABSTOP     8
#define MAXSTRINGINSIZE 255	/* the current KBDSTRINGIN limitation */


#define BS	    8		/* delete character to left of cursor */
#define CR	    13		/* also ENTER */
#define LF	    10		/* also CTRL_ENTER or CTRL_J */
#define ESCAPE	    27		/* restore template, reset history position */

#define CTRL_A	    1
#define CTRL_C	    3
#define CTRL_H	    8
#define CTRL_I	    9
#define CTRL_J	    10
#define CTRL_K	    11
#define CTRL_L	    12
#define CTRL_Q	    17
#define CTRL_S	    19
#define CTRL_Z	    26		/* what F6 translates to */
#define CTRL_OPEN   ESCAPE
#define CTRL_SLASH  28
#define CTRL_CLOSE  29
#define CTRL_CARET  30
#define CTRL_UNDER  31
#define RUB	    127

#define CTRL_BS     127 	/* actually DEL, but same as BS */
#define CTRL_BREAK  0

#define ALT_LMIN    16
#define ALT_F	    33		/* for foreground color control */
#define ALT_B	    48		/* for background color control */
#define ALT_LMAX    50

#define F1	    59		/* help, or RIGHT */
#define F2	    60		/* skip to character */
#define F3	    61		/* spit out rest of template/goto end of line */
#define F4	    62		/* delete to character */
#define F5	    63		/* copy line to template */
#define F6	    64		/* translate to Ctrl+Z */
#define F7	    65		/* reserved */
#define F8	    66		/* reserved */
#define F9	    67		/* enables/disables automatic line-editing */
#define F10	    68		/* reserved */

#define SHIFT_F1    84		/* reserved */
#define SHIFT_F10   93		/* reserved */

#define CTRL_F1     94		/* help from any point */
#define CTRL_F2     95		/* process list */
#define CTRL_F3     96		/* color table */
#define CTRL_F4     97		/* character table */
#define CTRL_F9    102		/* history */
#define CTRL_F10   103		/* status information */

#define ALT_F1	   104		/* help from command-line */
#define ALT_F2	   105		/* process list */
#define ALT_F3	   106		/* color table */
#define ALT_F4	   107		/* character table */
#define ALT_F9	   112		/* history */
#define ALT_F10    113		/* status information */

#define ALT_1	   120		/* reserved */
#define ALT_EQUAL  131		/* reserved */

#define ESCSCAN      1
#define ATSCAN	     3
#define BKSP	    14		/* goto beginning of line */
#define TABSCAN     15		/* tab scan code */
#define HOME	    71		/* goto beginning of line */
#define END	    79		/* goto end of line */
#define INS	    82		/* turn on character insertion/change cursor */
#define DEL	    83		/* delete character under cursor */
#define UP	    72		/* retrieve less recent line */
#define LEFT	    75		/* left one character */
#define RIGHT	    77		/* right one character */
#define DOWN	    80		/* retrieve more recent line */
#define PGUP	    73		/* pull up history list */
#define CENTER	    76
#define PGDN	    81
#define CTRL_HOME   119 	/* reset entire history, as well as escape */
#define CTRL_LT     115 	/* skip one word to left */
#define CTRL_RT     116 	/* skip one word to right */
#define CTRL_END    117 	/* delete rest of line from cursor on */
#define CTRL_PGDN   118
#define CTRL_PGUP   132
#define CTRL_UP     141
#define CTRL_CENTER 143
#define CTRL_DN     145
#define CTRL_INS    146
#define CTRL_DEL    147
#define KEYPAD_PLUS 78		/* used as Enter if ALTENTER flag set */

#define BLACK	    0		/* standard color codes */
#define BLUE	    1
#define GREEN	    2
#define CYAN	    3
#define RED	    4
#define MAGENTA     5
#define BROWN	    6
#define WHITE	    7
#define BRIGHT	    8
#define BCKGRD	    16

#define STDIN	    0		/* standard handles */
#define STDOUT	    1
#define STDERR	    2
#define NO_HANDLE   0xffff
#define VIO_HANDLE  0
#define KBD_HANDLE  0
#define VIO_MYHANDLE	0xeeff	/* my reserved handle */
#define VIO_MSEHANDLE	0xeefe	/* for mouse VIO calls, no need to hide/show */
#define VIO_FASTHANDLE	0xeefd	/* for special internal unblockable VIO calls */

#define VIO_MONO    0		/* standard adapter types */
#define VIO_CGA     1
#define VIO_EGA     2
#define VIO_VGA     3

#define CODEPAGE_US 437

#define INDX_VIOGETBUF	    1	/* VIO and KBD registered func. codes */
#define INDX_VIOSHOWBUF     2
#define INDX_VIOGETMODE     5
#define INDX_VIOSETCURPOS   6
#define INDX_VIOSETMODE     8
#define INDX_VIOWRTCHARSTRATT 15
#define INDX_VIOWRTTTY	    17
#define INDX_VIOSCROLLUP    18
#define INDX_VIOSETANSI     22
#define INDX_VIOGETANSI     23
#define INDX_VIOPRTSC	    24
#define INDX_VIOSCRLOCK     25
#define INDX_VIOSCRUNLOCK   26
#define INDX_VIOPRTSCTOGGLE 31
#define INDX_VIOGETCONFIG   35

#define INDX_KBDCHARIN	    0
#define INDX_KBDPEEK	    1
#define INDX_KBDFLUSHBUFFER 2
#define INDX_KBDSETSTATUS   4
#define INDX_KBDSTRINGIN    5


#define SEM_TEST	    0L	/* standard OS/2 API constants */
#define SEM_WAITFOREVER     -1L
#define SEM_WAITAMINUTE     (60*1000L)
#define OPEN_ATTNONE	    0
#define OPEN_ATTRO	    0x0001
#define OPEN_ATTHIDDEN	    0x0002
#define OPEN_ATTSYSTEM	    0x0004
#define OPEN_ATTSUBDIR	    0x0010
#define OPEN_ATTARCHIVE     0x0020
#define OPEN_EXISTFAIL	    0x0000
#define OPEN_EXISTOPEN	    0x0001
#define OPEN_EXISTRECREATE  0x0002
#define OPEN_NOEXISTFAIL    0x0000
#define OPEN_NOEXISTCREATE  0x0010
#define OPEN_ACCESSREAD     0x0000
#define OPEN_ACCESSWRITE    0x0001
#define OPEN_ACCESSRW	    0x0002
#define OPEN_DENYRW	    0x0010
#define OPEN_DENYWRITE	    0x0020
#define OPEN_DENYREAD	    0x0030
#define OPEN_DENYNONE	    0x0040
#define OPEN_NOINHERIT	    0x0080
#define OPEN_NOHARDERROR    0x2000
#define OPEN_WRITETHROUGH   0x4000
#define OPEN_DIRECTACCESS   0x8000

#define KSTATUS_INTERIM     0x80	/* KeyData "status" bit masks */
#define KSTATUS_FINAL	    0x40
#define KSTATUS_CONVERT     0x20
#define KSTATUS_SHIFTRPT    0x01

#define KBDMODE_ECHO	    0x0001	/* KbdStatus "bit_mask" bit masks */
#define KBDMODE_NOECHO	    0x0002
#define KBDMODE_RAW	    0x0004
#define KBDMODE_COOKED	    0x0008
#define KBDMODE_SSTATE	    0x0010
#define KBDMODE_TURNAROUND  0x0040
#define KBDMODE_EXTTURN     0x0080
#define KBDMODE_INHIBIT     0x8000

#define SSTATE_RSHIFT	    0x01	/* KbdStatus "shift_state" bits masks */
#define SSTATE_LSHIFT	    0x02
#define SSTATE_CTRL	    0x04
#define SSTATE_ALT	    0x08
#define SSTATE_SCROLL	    0x10
#define SSTATE_NUM	    0x20
#define SSTATE_CAPS	    0x40
#define SSTATE_INSERT	    0x80

#define KBDD_BRK	    0x11
#define KBDD_PSEUDOBRK	    0x12
#define KBDD_PSEUDOPAUSE    0x09
#define KBDD_WAKEUP	    0x0A

#define COM_CATEGORY	    1		/* communications equates */

#define COM_SETRATE	    0x41
#define COM_SETDCB	    0x53
#define COM_GETDCB	    0x73

#define FLAGS1_DTROFF	    0x00
#define FLAGS1_DTRON	    0x01
#define FLAGS1_DTRSHAKE     0x02
#define FLAGS1_CTSSHAKE     0x08
#define FLAGS1_DSRSHAKEOUT  0x10
#define FLAGS1_DCDSHAKE     0x20
#define FLAGS1_DSRSHAKEIN   0x40

#define FLAGS2_XONXMIT	    0x01
#define FLAGS2_XONRECV	    0x02
#define FLAGS2_REPLERROR    0x04
#define FLAGS2_STRIPNULLS   0x08
#define FLAGS2_REPLBREAK    0x10
#define FLAGS2_RTSOFF	    0x00
#define FLAGS2_RTSON	    0x40
#define FLAGS2_RTSSHAKE     0x80
#define FLAGS2_RTSTOGGLE    0xC0


/* Common types */

typedef UCHAR BBOOL;

typedef CHAR FAR FARCHAR[];

typedef USHORT (APIENTRY *PAPI)();

typedef struct {			/* comm driver data block */
    USHORT writetimeout;
    USHORT readtimeout;
    BYTE flags1;
    BYTE flags2;
    BYTE flags3;
    UCHAR errchar;
    UCHAR brkchar;
    UCHAR xonchar;
    UCHAR xoffchar;
} DCBLOCK;

typedef struct {			/* VioGetState palette block */
    USHORT  cb;
    USHORT  type;
    USHORT  iFirst;
    USHORT  acolor[16];
} PALSTATE;

typedef PALSTATE FAR *PPALSTATE;


/* CUE Regpkt flags */

#define CUEINIT_ALL	  0x0001	/* full registration */
#define CUEINIT_NOPRT	  0x0020	/* disable VioPrt* functions */


/* CUE SetState flags */

#define CUESTATE_PERM	  0x0001	/* ignore "exit" from command-line */
#define CUESTATE_ALTENT   0x0002	/* interpret PLUS on keypad as ENTER */
#define CUESTATE_BIGCUR   0x0004	/* large cursor as default */
#define CUESTATE_INSERT   0x0008	/* insert-mode as default */
#define CUESTATE_NOCTRL   0x0010	/* disable Ctrl-Function keys */
#define CUESTATE_NOPRT	  0x0020	/* disable VioPrtSc* functions */
#define CUESTATE_MOUSE	  0x0040	/* activate mouse for screen group */
#define CUESTATE_LINEEDIT 0x0080	/* allow line-editing */
#define CUESTATE_SETSTATE 0x0100	/* force VioSetState operations */
#define CUESTATE_RESIDENT 0x8000	/* run in resident (non-shell) mode */


/* CUE server messages */

#define CUEMSG_EXIT	0
#define CUEMSG_GETSEGS	1
#define CUEMSG_FREESEGS 2


/* CUE error codes */

#define ERROR_CUE_INSTALLED  0xffff
#define ERROR_CUE_NOSTACKS   0xfffe
#define ERROR_CUE_NOALIASES  0xfffd
#define ERROR_CUE_NOSG	     0xfffc
#define ERROR_CUE_NOPROCESS  0xfffb
#define ERROR_CUE_DETACHED   0xfffa


/* CUE pop-up IDs */

#define POPUP_HELP	0
#define POPUP_HIST	1
#define POPUP_PROC	2
#define POPUP_STAT	3
#define POPUP_SCREEN	4
#define POPUP_COLOR	5
#define POPUP_CHAR	6
#define MAXPOPUPS	7


/* CUE defaults */

#define CUEDEF_BAUDRATE     9600


/* Common CUE types */

typedef struct {
	PAPI   pfn;
	USHORT cbSearch;		/* search pattern length */
	BYTE   abSearch[6];		/* search pattern */
	USHORT cbReplace;		/* replace pattern length */
	BYTE   abReplace[6];		/* replace pattern */
	USHORT cbTerm;			/* terminate pattern length */
	BYTE   abTerm[6];		/* terminate pattern */
} PATCHBLK;


typedef struct {			/* CUE registration packet */
	USHORT histsize;		/* total bytes history this SG */
	USHORT histbuffs;		/* total partitions */
	USHORT scrollsize;		/* total lines buffer can hold */
	USHORT commport;		/* associated comm port, if any */
	USHORT flags;
} CUEREGPKT;

typedef CUEREGPKT FAR *PCUEREGPKT;
typedef CUEREGPKT NEAR *NPCUEREGPKT;


typedef struct {
	USHORT code;			/* message code */
	USHORT sg;			/* SG index */
	USHORT returncode;		/* error/return code */
} CUEMSG;

typedef CUEMSG FAR *PCUEMSG;
typedef CUEMSG NEAR *NPCUEMSG;


typedef struct {
    USHORT  ulr;			/* upper-left row */
    USHORT  ulc;			/* upper-left column */
    USHORT  lrr;			/* lower-right row */
    USHORT  lrc;			/* lower-right column */
    USHORT  row;			/* cursor row or index (varies) */
    USHORT  col;			/* cursor column or color (varies) */
    UCHAR   style;			/* pop-up style bits */
    CHAR    lock;			/* pop-up re-entrancy lock (-1=unlocked) */
} POPDATA;

typedef POPDATA FAR *PPOPDATA;
typedef POPDATA NEAR *NPPOPDATA;

#define SAVEHDR (sizeof(POPDATA)+sizeof(VIOCURSORINFO)+sizeof(PALSTATE)+sizeof(VIOINTENSITY))


/* CUE function prototypes */

USHORT FAR CueRegister(PCUEREGPKT);
USHORT FAR CueDeRegister(VOID);
USHORT FAR CueSetState(USHORT, USHORT, USHORT);
USHORT FAR CueSetAliases(PCHAR, USHORT);
USHORT FAR CueGetAliases(PCHAR);
USHORT FAR CueInsertInput(PCHAR);
USHORT FAR CueDetach(PCHAR);
USHORT FAR CueUnDetach(VOID);
USHORT FAR CueGetMessage(PCUEMSG);
USHORT FAR CueSendMessage(USHORT, USHORT);
USHORT FAR CueGetSegments(USHORT);
USHORT FAR CueFreeSegments(USHORT);
USHORT FAR CueSetPopup(USHORT, PPOPDATA);
USHORT FAR CueSysVer(VOID);
USHORT FAR CueDetached(VOID);
USHORT FAR CueProcName(PCHAR, USHORT, USHORT);
SHORT  FAR CueToLower(SHORT);
SHORT  FAR CueIsAlphaNum(SHORT);
SHORT  FAR CueStrniCmp(PCHAR, PCHAR, SHORT);
USHORT FAR CueStrPrev(PUCHAR, USHORT);
USHORT FAR CueStrToken(PUCHAR, PUSHORT);
