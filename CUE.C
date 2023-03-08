/*  CUE.C
 *  By Jeff Parsons  Jan '88  Updated Sep '88
 *
 *  Interface to CUE dynlink library
 *
 *  Original options:
 *	-H	Gives usage and command-line options
 *	-Q	Reduces noise
 *	-R	Sets keyboard delay/repeat rate
 *	-#	Sets number of screen lines
 *	-B	Uses CueRegister to specify buffers/partitions
 *	-V	Uses CueRegister to specify scroll buffers
 *	-COMm	Uses CueRegister to set up COMm and default baud rate
 *	-P	Uses CueSetState to select base CMD shell exit-inhibit
 *	-E	Uses CueSetState to map keypad PLUS to ENTER
 *	-COMm:n Uses CueSetState to change COMm initial baud rate to n
 *	-A	Uses CueSetAliases to set up add/delete aliases
 *	-L	Uses CueGetAliases to list current aliases
 *	-S	Uses CueInsertInput to insert input in the keyboard stream
 *	-X	Uses CueUnDetach to attempt to kill of CUE server process
 *
 *  New options:
 *	-D	Ignores TOOLS.INI settings
 *		(except for QUIET, settings are ignored once loaded)
 *	-F	Uses CueRegister to force all functions hooked
 *	-G	Uses CueSetState to force VioSetState on each popup
 *	-I	Uses CueSetState to select INSERT mode as the default
 *	-K	Uses CueSetState to disable VioPrtSc* functions
 *	-O	Uses CueSetState to disable use of Ctrl-function keys
 *	-T	Uses CueSetState to select a normally TALL cursor
 *	-U	Set debug flag (only in debugging version)
 *	-Y	Set automatic line-editing flag
 *	-Nx	Reverses any of the following options:	Q,P,E,G,I,K,O,T,Y
 *
 *  TOOLS.INI options (when they're added):
 *   [cue]
 *	quiet: on|off
 *	repeat: #,#
 *	screen: 25|43|50
 *	buffers: #,#
 *	scroll: #
 *	comm: 1|2,#
 *	perm: on|off
 *	plus: on|off
 *	aliases: <filename>
 *	register: all|minimum
 *	cursor: large|small
 *	insert: on|off
 *	functions: on|off
 *	prtsc: on|off
 *	debug: on|off
 *	help: row,col,height,width,color
 *	history: row,col,height,width,color
 *	process: row,col,height,width,color
 *	status: row,col,height,width,color
 *	screen: row,col,height,width,color
 *	colors: row,col,height,width,color
 *	characters: row,col,height,width,color
 *
 *   [cue:aliases]
 *	foo bar
 *	...
 *
 *  Note that command-line options always override TOOLS.INI settings, in
 *  spite of the fact that command-line options are parsed first (in
 *  order to detect -D).  This is achieved by remembering which options
 *  have been "set", and preventing them from being re-"set".
 *
 *  Note that any aliases listed in TOOLS.INI are loaded into the alias buffer
 *  first, and subsequent alias file contents are simply appended to this
 *  buffer.  The only way to get rid of aliases now is to delete all your
 *  aliases and reload.  An "alias" pop-up would be a good thing, as would be
 *  the ability to delete lines, or ranges of lines, from such pop-ups.
 *
 *  CueSetPopup is a new interface that allows the characteristics of a
 *  specified pop-up to be set.
 */


#include <ctype.h>		/* C runtime stuff */
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdio.h>
#undef	tolower
#undef	toupper
#include <stdlib.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>

#define INCL_BASE		/* include base APIs, subsystem APIs, errors */
#include <os2.h>		/* include it! */

#include "cue.h"


USHORT Firstload;
CHAR Aliasname[MAXARGSIZE];
CHAR Aliases[MAXALIASBUF+1];
NPBYTE Psiminput;
BOOL Quiet, List, Debug;
CUEREGPKT Regpkt;
USHORT Defaultdelayrate[2];
USHORT Historyset, Videobuffset, Delayrateset;

USHORT uCommport = 0;		/* non-zero implies COMM port */
USHORT fsState = 0;		/* flags for CueSetState */
USHORT fsStateSet = 0;		/* flags for CueSetState to set */
USHORT uCommrate = 0;		/* non-zero implies new rate for COMM port */

NPCH pname = "CUE  Version 2.07\n";
NPCH pdesc = "Command-line Utility/Editor for OS/2\n\n";

NPCH author= "Copyright (c) 1988, 1989 by Jeff Parsons, Microsoft Corp.\n\n";

#ifdef	TESTING
NPCH warn  = "THIS COPY PROVIDED FOR TESTING PURPOSES ONLY\n";
PCHAR Valid;
#endif


VOID help()
{
    printf(pname);
    printf(pdesc);
  #ifdef  TESTING
    printf(warn);
    printf(author);
  #endif
    printf("Usage: CUE [options] [program [program options]]\n\n");
    printf("Options: -H  to give usage and options\n");
    printf("         -A<filename>|NUL to load|remove aliases\n");
    printf("         -B<history buffer size>,<partitions>\n");
    printf("         -E  to use keypad [+] as [Enter]\n");
    printf("         -F  to hook all VIO/KBD functions\n");
    printf("         -G  to support 16-color popup bgnds\n");
    printf("         -I  to select INSERT mode as default\n");
    printf("         -K  to disable PrtSc functions\n");
    printf("         -L  to list currently loaded aliases\n");
  #ifdef MOUSE
    printf("         -M  to enable mouse support\n");
  #endif
    printf("         -N<o> to reverse sense of option <o>\n");
    printf("         -O  to disable use of Ctrl-function keys\n");
    printf("         -P  to inhibit exit from base CMD shell\n");
    printf("         -Q  to execute quietly\n");
    printf("         -R<typematic delay>,<chars/sec>\n");
    printf("         -S<string> to simulate keyboard input\n");
    printf("         -T  to select alternate cursor as default\n");
  #ifdef DEBUG
    printf("         -U  to enable DEBUG option\n");
  #endif
    printf("         -V<lines> to reserve for scroll buffer\n");
    printf("         -X  to terminate CUE server process\n");
    printf("         -Y  to enable automatic line-editing\n");
    printf("         -#  to set # lines on EGA/VGA (25, 43 or 50)\n");
    printf("         -COM#[:<baud>] to enable terminal output\n");
    printf("\nIf a program name is specified, CUE will execute the program,\n");
    printf("along with any arguments, and deactivate when the program terminates.\n");
    exit(0);
}


VOID adjustdelayrate()
{
    USHORT kbdhandle, action;

    if (Delayrateset) {
	if (Defaultdelayrate[0] < 250)
	    Defaultdelayrate[0] = 250;
	if (Defaultdelayrate[0] > 1000)
	    Defaultdelayrate[0] = 1000;
	if (Defaultdelayrate[1] == 0 || Defaultdelayrate[1] > 30)
	    Defaultdelayrate[1] = 30;
	if (Defaultdelayrate[1] < 2)
	    Defaultdelayrate[1] = 2;
	if (!DosOpen("KBD$", &kbdhandle, &action, 0L,
		     OPEN_ATTNONE,
		     OPEN_EXISTOPEN, OPEN_DENYRW + OPEN_ACCESSRW, 0L)) {
	    DosDevIOCtl(0, Defaultdelayrate, 0x54, 0x04, kbdhandle);
	    DosClose(kbdhandle);
	}
    }
}


USHORT setlines(n)
USHORT n;
{
    static CHAR crlf[] = {'\r', '\n'};
    USHORT row, newrow, col;
    VIOCONFIGINFO vconfig;
    VIOCURSORINFO vcursor;
    static VIOMODEINFO newmode = {8, VGMT_OTHER, 4, 80, 25};

    if (n != 25 && n != 43 && n != 50)
	return(FALSE);
    vconfig.cb = sizeof(vconfig);
    VioGetConfig(0, &vconfig, VIO_HANDLE);
    if (vconfig.adapter < VIO_EGA)
	return(FALSE);
    if (vconfig.display == VIO_CGA)
	return(FALSE);
    if (vconfig.adapter != VIO_VGA && n == 50)
	return(FALSE);
    newmode.row = n;
    VioGetCurPos(&row, &col, VIO_HANDLE);
    VioSetMode(&newmode, VIO_HANDLE);
    newrow = row;
    if (row >= n)
	newrow = n-1;
    VioSetCurPos(newrow, col, VIO_HANDLE);
    if (row >= n)
	VioWrtTTY(crlf, sizeof(crlf), VIO_HANDLE);
  #ifdef DEBUG
    VioGetCurType(&vcursor, VIO_HANDLE);
    if (Debug)
	printf("DEBUG: new cursortype=%u,%u\n", vcursor.yStart, vcursor.cEnd);
  #endif
    return(TRUE);
}


VOID checkcomm(p)
register NPCH p;
{

    if (!strnicmp(p, "COM", 3)) {
	p += 3;
	if (isdigit(*p)) {
	    Regpkt.commport = uCommport = *p++ - '0';
	    uCommrate = CUEDEF_BAUDRATE;
	    if (*p == ':')
		uCommrate = atoi(++p);	    /* override the default */
	}
    }
}


VOID checknumeric(p, c)
NPCH p;
register UCHAR c;
{
    if (!isdigit(*p) && *p != ',') {
	fprintf(stderr, "Bad value [%s] for option: %c\n\n", p, c);
	help();
    }
}


VOID getswitches(pargc, pargv)
CHAR ***pargv;
register USHORT *pargc;
{
    register NPCH p;
    SHORT multiplier;
    USHORT c, c2, i, breakout;

  #ifdef  TESTING
    Valid = (PCHAR)warn;
    if (*Valid != 'T')
	Valid = NULL;
  #endif

    --*pargc;  ++*pargv;    /* skip this program's name */
    for (; *pargc; --(*pargc),++(*pargv)) {
	if (***pargv == '-' || ***pargv == '/') {
	  p = **pargv+1;    /* past switch char */
	  breakout = FALSE;
	  c = 0;
	  while (*p && !breakout) {
	    c2 = ' ';
	    if (c != 'n')
	      multiplier = 1;
	    switch(c=tolower(*p++)) {
	  #ifdef DEBUG
	    case 'u':
		Debug = TRUE;
		printf("DEBUG: enabled\n");
		break;
	  #endif
	    case 'f':	    /* past first letter now, too */
		Regpkt.flags |= CUEINIT_ALL*multiplier;
		break;
	    case 'g':
		fsState |= CUESTATE_SETSTATE*multiplier;
		fsStateSet |= CUESTATE_SETSTATE;
		break;
	    case 'h':
		help();
	    case 'i':
		fsState |= CUESTATE_INSERT*multiplier;
		fsStateSet |= CUESTATE_INSERT;
		break;
	    case 'k':
		Regpkt.flags |= CUEINIT_NOPRT*multiplier;
		fsState |= CUESTATE_NOPRT*multiplier;
		fsStateSet |= CUESTATE_NOPRT;
		break;
	    case 'l':
		List = TRUE*multiplier;
		break;
	  #ifdef MOUSE
	    case 'm':
		Regpkt.flags |= CUEINIT_ALL*multiplier;
		fsState |= CUESTATE_MOUSE*multiplier;
		fsStateSet |= CUESTATE_MOUSE;
		break;
	  #endif
	    case 'n':
		multiplier = !multiplier;
		break;
	    case 'o':
		fsState |= CUESTATE_NOCTRL*multiplier;
		fsStateSet |= CUESTATE_NOCTRL;
		break;
	    case 'p':
		fsState |= CUESTATE_PERM*multiplier;
		fsStateSet |= CUESTATE_PERM;
		break;
	    case 'q':
		Quiet = TRUE*multiplier;
		break;
	    case 't':
		fsState |= CUESTATE_BIGCUR*multiplier;
		fsStateSet |= CUESTATE_BIGCUR;
		break;
	    case 'e':
	    case '+':
		fsState |= CUESTATE_ALTENT*multiplier;
		fsStateSet |= CUESTATE_ALTENT;
		break;
	    case 'x':
		i = CueUnDetach();
		if (i == ERROR_CUE_NOPROCESS)
		    printf("CUE server process not active\n");
		else if (i == ERROR_CUE_DETACHED)
		    printf("CUE server process in use\n");
		else if (i == 0)
		    printf("CUE server process terminated\n");
		else
		    fprintf(stderr, "Error [%u] from CueUnDetach\n", i);
		exit(0);
	    case 'y':
		fsState |= CUESTATE_LINEEDIT*multiplier;
		fsStateSet |= CUESTATE_LINEEDIT;
		break;
	    case 'a':
	    case 'b':
	    case 'c':
	    case 'r':
	    case 's':
	    case 'v':
		if (!*p && *pargc > 1) {
		    --(*pargc);
		    p = *++(*pargv);
		}
		switch(c) {
		case 'a':
		    strcpy(Aliasname, p);
		    break;
		case 'b':
		    checknumeric(p, c);
		    Regpkt.histsize = atoi(p);
		    p = strchr(p, ',');
		    if (p)
			Regpkt.histbuffs = atoi(++p);
		    Historyset = TRUE;
		    break;
		case 'c':
		    checkcomm(p-1);
		    break;
		case 'r':
		    checknumeric(p, c);
		    if (c = atoi(p))
			Defaultdelayrate[0] = c;
		    p = strchr(p, ',');
		    if (p)
			if (c = atoi(++p))
			    Defaultdelayrate[1] = c;
		    Delayrateset = TRUE;
		    adjustdelayrate();
		    break;
		case 's':
		    Psiminput = p;
		    break;
		case 'v':
		    checknumeric(p, c);
		    Regpkt.scrollsize = atoi(p);
		    Videobuffset = TRUE;
		    break;
		}
		breakout = TRUE;
		break;
	    case '2':
	    case '4':
	    case '5':
		c2 = *p;
		i = atoi(p++ - 1);
		if (setlines(i))
		    break;
	    default:
		fprintf(stderr, "Invalid option: %c%c\n\n", c, c2);
		help();
	    }
	  }	/* while (*p) */
	}   /* if (- or /) */
	else
	    break;	    /* presumably the optional program stuff begins */
    }
}


USHORT hex(p, pi)
register NPBYTE p;
USHORT *pi;
{
    register USHORT i, x, j = 0, d = 0;

    i = *pi + 1;
    while (isxdigit(p[i]) && j < 2) {
	x = tolower(p[i]) - '0';
	if (x > 9)
	    x = x + '0' + 10 - 'a';
	d = d * 16 + x;
	i++;
	j++;
    }
    *pi = i - 1;
    return(d);
}


USHORT decimal(p, pi)
register NPBYTE p;
USHORT *pi;
{
    register USHORT i, j = 0, d = 0;

    i = *pi + 1;
    while (isdigit(p[i]) && j < 3) {
	d = d * 10 + (p[i] - '0');
	i++;
	j++;
    }
    *pi = i - 1;
    return(d);
}


VOID loadinput()
{
    USHORT i;
    register USHORT j;

    if (Psiminput) {
	i = j = 0;
	while (Psiminput[i]) {
	    if (Psiminput[i] == '\\')
		switch (Psiminput[++i]) {
		case 'r':
		case 'n':
		    Psiminput[j++] = '\r';
		    break;
		case 'b':
		    Psiminput[j++] = '\b';
		    break;
		case '\'':
		case '\"':
		case '\\':
		    Psiminput[j++] = Psiminput[i];
		    break;
		case 'X':
		    Psiminput[j++] = 0xff;
		case 'x':
		    Psiminput[j++] = (unsigned char)hex(Psiminput, &i);
		    break;
		case 'D':
		    Psiminput[j++] = 0xff;
		case 'd':
		    Psiminput[j++] = (unsigned char)decimal(Psiminput, &i);
		    break;
		case 'p':
		    Psiminput[j++] = 0xff;
		    Psiminput[j++] = 0xfe;
		    Psiminput[j++] = (unsigned char)decimal(Psiminput, &i);
		    break;
		}
	    else
		Psiminput[j++] = Psiminput[i];
	    i++;
	}
	Psiminput[j] = 0;
	CueInsertInput(Psiminput);
    }
}


VOID listaliases(new)
USHORT new;
{
    register USHORT i = 0, j = 0, len;

    printf("\n");
    if (!new)
	printf("Current aliases:\n");
    else
	printf("Aliases now loaded:\n");
    while (Aliases[i]) {
	if (j == 24) {
	    printf("[...]\n");
	    break;
	}
	len = strlen(Aliases+i);
	printf("%-8.8s ", Aliases+i);
	i += len + 1;
	if (Aliases[i]) {
	    len = strlen(Aliases+i);
	    printf("%-20.20s", Aliases+i);
	    if (len > 20)
		printf("...   ");
	    else
		printf("      ");
	    i += len;
	}
	i++;
	if (!(++j & 1))
	    printf("\n");
    }
    if (j & 1)
	printf("\n");
}


VOID loadaliases()
{
    SHORT handle;
    FILE *infile = 0;
    USHORT preloaded = FALSE;
    register USHORT i, lasti, j, len, state, rc;

    rc = CueGetAliases(Aliases);
    if (!rc)
	preloaded = TRUE;
    if (List) {
	if (!rc)
	    listaliases(FALSE);
	else
	    printf("\nNo aliases loaded\n");
    }
    if (!Aliasname[0])
	return;
    handle = sopen(Aliasname, O_RDONLY|O_TEXT, SH_DENYWR);
    if (handle >= 0)
	infile = fdopen(handle, "r");
    if (!infile) {
	fprintf(stderr, "Could not open alias file %s\n", Aliasname);
	return;
    }
    lasti = i = 0;			/* overflow control for i */
    Aliases[MAXALIASBUF] = -1;		/* overflow marker */
    while (!feof(infile)) {
	lasti = i;
	if (!fgets(Aliasname, MAXARGSIZE, infile))
	    break;
	if (i+3 >= MAXALIASBUF)
	    break;
	len = strlen(Aliasname);
	for (j=0,state=0; j<len; j++) {
	    if (state == 0) {		/* state: alias not found yet */
		if (!isspace(Aliasname[j]))
		    state++;
	    }
	    if (state == 1) {		/* state: alias found */
		if (isspace(Aliasname[j])) {
		    Aliases[i++] = 0;	/* null-terminate the alias */
		    state++;
		}
		else if (i < MAXALIASBUF-3)
		    Aliases[i++] = Aliasname[j];
		else
		    break;
	    }
	    if (state == 2) {		/* state: alias value not found yet */
		if (!isspace(Aliasname[j]))
		    state++;
	    }
	    if (state == 3) {		/* state: alias value found */
		if (Aliasname[j] != '\n')
		    if (i < MAXALIASBUF-2)
			Aliases[i++] = Aliasname[j];
		    else
			break;
	    }
	}
	if (state < 1)
	    continue;			/* if no alias, don't terminate anything */
	if (state == 1)
	    Aliases[i++] = 0;		/* null-terminate the alias */
	if (state > 1)
	    Aliases[i++] = 0;		/* null-terminate the value now */
    }
    i = lasti;
    if (i)
	Aliases[i++] = 0;		/* a final, non-alias, non-value null */

    fclose(infile);

    if (Aliases[MAXALIASBUF] != -1)
	fprintf(stderr, "Warning: internal buffer overflow\n");

    rc = CueSetAliases(Aliases, i);
    if (rc)
	fprintf(stderr, "Error [%u] attempting to set aliases\n", rc);
    else if (!i) {
	/* if (!Quiet) */
	    printf("\nAliases removed\n");
    }
    else {
	listaliases(TRUE);
	Firstload = !preloaded;
    }
}


VOID stats(firsttime, argc)
USHORT firsttime, *argc;
{
  #ifdef  TESTING
    *Valid = 'T';
  #endif
    CueSetState(fsState, fsStateSet, uCommrate);

    if (!Quiet) {
	if (firsttime)
	    printf("CUE is now active in this screen group\n");
	else {
	    printf("CUE is still active in this screen group\n");
	    if (Historyset || Videobuffset)
		printf("Cannot resize existing buffer(s)\n");
	    if (*argc)
		printf("Ignoring the specified program name\n");
	}
	printf("Press Alt-F1 for Help\n");
    }
    if (!firsttime)
	*argc = 0;

    loadinput();
    loadaliases();

    if (!Quiet) {
	printf("\n");
	if (Regpkt.commport)
	    printf("Output shadowed on COM%c (%u baud)\n",
		    Regpkt.commport+'0', uCommrate);
	else if (uCommport)
	    printf("Cannot access COM%c\n", uCommport+'0');
	if (Delayrateset)
	    printf("Typematic delay %ums, repeat %u chars/sec\n",
		Defaultdelayrate[0], Defaultdelayrate[1]);
	if (Regpkt.scrollsize)
	    printf("Video scroll buffer holds %u lines\n", Regpkt.scrollsize);
	printf("History buffer is %u bytes, divided into %u partition(s)\n",
		Regpkt.histsize, Regpkt.histbuffs);
    }
}


VOID checkdetached()
{
    CUEMSG msg;
    register USHORT version;

    if (CueDetached()) {
	while (TRUE) {
	    if (CueGetMessage(&msg))
		break;		/* presume a redundant detach occurred */
	    switch (msg.code) {

	    case CUEMSG_EXIT:
		goto exit_checkdetached;

	    case CUEMSG_GETSEGS:
		msg.returncode = CueGetSegments(msg.sg);
		break;

	    case CUEMSG_FREESEGS:
		msg.returncode = CueFreeSegments(msg.sg);
		break;
	    }
	}
exit_checkdetached:
	exit(0);
    }
}


VOID cdecl main(argc, argv)
USHORT argc;
NPCH *argv;
{
    register USHORT i, j, k, rc = 0;
    USHORT pid;
    USHORT firsttime = TRUE, abort = FALSE;
    RESULTCODES rcs;
    CHAR prog[MAXARGSIZE+1], args[MAXARGSIZE+1];


    checkdetached();		    /* see if we are server process */

    strcpy(prog, *argv);	    /* save our own program name */

    getswitches(&argc, &argv);

    if (!Quiet) {
	printf(pname);
	printf(pdesc);
      #ifdef  TESTING
	printf(warn);
	printf(author);
      #endif
    }

    rc = CueRegister(&Regpkt);	    /* get registered! */

    if (rc == ERROR_CUE_INSTALLED) {
	rc = 0;
	firsttime = FALSE;
    }
    else if (rc) {
	if (rc == ERROR_VIO_REGISTER || rc == ERROR_KBD_REGISTER)
	    fprintf(stderr, "Another subsystem is already active in this screen group\n");
	else
	    fprintf(stderr, "Error [%u] from CUESUBS\n", rc);
	exit(rc);
    }

    stats(firsttime, &argc);

    if (argc) { 		    /* if args left (after getswitches) */
	strcpy(prog, *argv);	    /* replace program name with specified */
	i = 0;	j = 0;
	while (i < argc && j < MAXARGSIZE) {
	    if (i == 1)
		args[j++] = 0;	    /* args contains a null-terminated */
	    if (i > 1)		    /* program name, followed by */
		args[j++] = ' ';    /* all the space-delimited arguments */
	    k = 0;
	    while (argv[i][k])
		args[j++] = argv[i][k++];
	    i++;
	}
	args[j] = 0;
    }

    i = strlen(prog);		    /* ensure program name has .exe appended */
    if (strcmpi(prog+i-4, ".exe"))
	if (islower(prog[i-1]))
	    strcat(prog, ".exe");
	else
	    strcat(prog, ".EXE");

    if (!argc) {		    /* if no program to shell to */
	if (firsttime)
	    rc = CueDetach(prog);   /* detach if we think we haven't yet */
	if (rc == ERROR_CUE_DETACHED)
	    rc = 0;		    /* already detached, so clear the error */
	else if (rc)
	    fprintf(stderr, "\nError [%u] attempting to detach %s\n", rc, prog);
    }
    else {			    /* if there IS a program to shell to */
	rc = DosExecPgm(0, 0, EXEC_SYNC, args, 0, &rcs, prog);
	if (rc)
	    fprintf(stderr, "\nError [%u] attempting to execute %s\n", rc, prog);
    }

    if (argc || rc) {
	CueUnDetach();
	rc = CueDeRegister();
	if (rc)
	    fprintf(stderr, "Error [%u] from CUESUBS\n", rc);
	else if (!Quiet)
	    printf("CUE has now been deactivated in this screen group\n");
    }

    exit(rc);
}
