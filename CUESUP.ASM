	Title	C assembly-support code
	Name	CUESUP

;	By Jeff Parsons  Jan '88  Updated Sep '88

	.286p

INCL_BASE equ	1		    ;include base APIs, etc.
;	include os2.inc
	lis_pidCurrent equ 0	    ;this is all we need
	lis_tidCurrent equ 6


DEFINE	equ	1		    ;publicize everything


_HELP	segment byte public 'FAR_DATA'

	public	Helphelp

Helphelp label far
;	db	"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	db	" ",0
	db	"  Cue  Version 2.07",0
	db	"  by Jeff Parsons  February '89",0
	db	" ",0
	db	"  Functions available from Cue pop-ups:",0
	db	" ",0
	db	"   F1            Help",0
	db	"   Left/Right    Select line",0
	db	"   Enter         Select/enter line",0
	db	"   Esc           Exit",0
	db	"   Up            Previous line",0
	db	"   Down          Next line",0
	db	"   Home          Top line",0
	db	"   End           Bottom line",0
	db	"   Ctrl+Home     First line",0
	db	"   Ctrl+End      Last line",0
	db	"   Ctrl+Up       Scroll up",0
	db	"   Ctrl+Down     Scroll down",0
	db	"   <char>        Next line with <char>",0
	db	"   Center        Select move/expand/collapse",0
	db	"   Alt+F/Alt+B   Alter fgnd/bgnd colors",0
	db	" ",0
	db	"  Functions available from command-line prompts:",0
	db	" ",0
	db	"   Left          Left character",0
	db	"   F1/Right      Right character",0
	db	"   F3            Display input template",0
	db	"   F6            EOF (Ctrl+Z)",0
	db	"   Del           Delete character",0
	db	"   Backspace     Left character and delete",0
	db	"   Ins           Insert mode on/off",0
	db	"   Home          Beginning of line",0
	db	"   End           End of line",0
	db	"   Ctrl+Left     Left word",0
	db	"   Ctrl+Right    Right word",0
	db	"   Ctrl+End      Delete to end of line",0
	db	"   Up            Display previous history",0
	db	"   Down          Display subsequent history",0
	db	"   Esc           Clear line",0
	db	"   Ctrl+Del      Erase to end of history",0
	db	"   PgUp or",0
	db	"    Ctrl+PgUp    History pop-up",0
	db	"   PgDn or",0
	db	"    Ctrl+Up/Down Scroll buffer pop-up",0
	db	"   Alt+F1        Help",0
	db	"   Alt+F2        Process List",0
	db	"   Alt+F3        Color Table",0
	db	"   Alt+F4        Character Table",0
	db	"   Alt+F10       Cue Status",0
	db	" ",0
	db	"  Functions available from the History and Scroll",0
	db	"  buffer pop-ups:",0
	db	" ",0
	db	"   Left/Right    Select for editing",0
	db	"   Enter         Select for immediate execution",0
	db	" ",0
	db	"  Functions available from the Colors pop-up:",0
	db	" ",0
	db	"   Left/Right    Decrease/increase color setting",0
	db	"   Enter         Restore to default",0
	db	" ",0
	db	"  Pop-ups available at any time of input:",0
	db	" ",0
	db	"   Ctrl+F1       Help",0
	db	"   Ctrl+F2       Process List",0
	db	"   Ctrl+F3       Color Table",0
	db	"   Ctrl+F4       Character Table",0
	db	"   Ctrl+F10      Cue Status",0
	db	" ",0
	db	" ",0
	db	"  Note:  Pop-ups limited to 4 at any one time.",0
	db	0
;	db	"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

_HELP	ends


STACKSIZE	equ	1660	    ;in bytes
DEFSTACKS	equ	8	    ;total stack blocks to allocate per grow

FILL		equ	0EEEEh

MAXSIGS 	equ	7	    ;OS/2 1.0/1.1 define a range of 7 signals

struc_farptr	struc		    ;provides access to selector/offset
 off		dw	?
 sel		dw	?
struc_farptr	ends

struc_semaphore struc		    ;my own "fast, safe" ram semaphores
 sem		dd	0	    ;OS/2 ram semaphore
 sem_owner	dw	0	    ;PID of owner, zero if none
struc_semaphore ends

struc_infoblock struc		    ;block returned on GetInfo() call
 ib_stacksbusy	dw	0	    ;# stacks in use right now
 ib_stacksheld	dw	0	    ;# stacks held right now
 ib_stacksalloc dw	0	    ;total stacks allocated so far
 ib_stacksmax	dw	0	    ;maximum total stacks that can be allocated
 ib_stacksws	dw	0	    ;# times stacks switched
 ib_stacksize	dw	0	    ;size of stacks ("space" in stack blocks)
 ib_stackszused dw	0	    ;most amount of space used in a stack
struc_infoblock ends

struc_stackblock struc
 sb_link	dw	0	    ;can be either free-list or sig-list link
 sb_pid 	dw	0	    ;PID of owner, zero if none
 sb_tid 	dw	0	    ;thread # (has no bearing on ownership)
 sb_refs	dw	0	    ;reference count (when in use)
 sb_heldsp	dw	0	    ;caller's SP when stack is on hold list
 sb_lockout	dw	0	    ;lock word (used by C lock routines)
 sb_sigsaves	dw	0	    ;count of SaveSignals()
 sb_sigholds	dw	0	    ;count of HoldSignals()
 sb_sigssaved	dw	0	    ;flag (non-zero if signals actually saved)
 sb_sigacts	dw	MAXSIGS dup(?)
 sb_sighdlrs	dd	MAXSIGS dup(?)
 sb_space	dw	(STACKSIZE-offset sb_space)/2 DUP (FILL)
struc_stackblock ends
		.errnz	size struc_stackblock - STACKSIZE


_DATA	segment word public 'DATA'

	extrn	PGinfoseg:dword
	extrn	PLinfoseg:dword

	IFDEF	DEFINE
	public	stacksem
	public	stackhead
	public	stackfree
	public	stackheld
	public	infoblock
	ENDIF

stacksem	struc_semaphore <>  ;stack list semaphore
stackhead	dw	0	    ;ptr to start of stack blocks
stackfree	dw	0	    ;ptr to free blocks
stackheld	dw	0	    ;ptr to blocks held for signal calls
infoblock	struc_infoblock <>  ;static global information block

NULLFRAME	equ	-1	    ;reserved SwitchStacks() inputs
VIOROUTERFRAME	equ	-2
KBDROUTERFRAME	equ	-3
MOUROUTERFRAME	equ	-4

viorouterargs	db	3,5,3,5,3,3,3,3,3,7,7,6,6,6
		db	6,8,6,4,8,8,8,8,2,3,1,4,1,4,3,3,1,1,4,3,3,4,3,4,3,3,3
MAXVIOINDEX	equ	($-viorouterargs) - 1
		.errnz	MAXVIOINDEX-40	    ;I only know about 41 VIO functions

kbdrouterargs	db	4,3,1,3,3,6,2,1,2,1,5,3,3,3
MAXKBDINDEX	equ	($-kbdrouterargs) - 1
		.errnz	MAXKBDINDEX-13	    ;I only know about 14 KBD functions

mourouterargs	db	3,3,3,3,5,3,3,3,3,3,3,4,1,5,5,1,3,3,3,2,1,3
MAXMOUINDEX	equ	($-mourouterargs) - 1
		.errnz	MAXMOUINDEX-21	    ;I only know about 22 MOU functions

_DATA	ends


	__acrtused equ 0
	public	__acrtused	    ;satisfy C runtime references


	public	SwitchDS	    ;routines for general consumption
	public	RestoreDS
	public	SwitchStacks
	public	RestoreStacks
	public	FreeStacks
	public	GetInfo
	public	SaveSignals
	public	RestoreSignals
	public	HoldSignals
	public	ReleaseSignals
	public	LockThread
	public	UnLockThread
	public	LockSem
	public	UnLockSem
	public	CheckSem

	public	TestAndSet
	public	InB
	public	OutB

	extrn	main:far	    ;to define main() as lib init routine

	extrn	DosExit:far
	extrn	DosSemClear:far
	extrn	DosSemWait:far
	extrn	DosSemRequest:far
	extrn	DosReallocSeg:far
	extrn	DosHoldSignal:far
	extrn	DosSetSigHandler:far
	extrn	DosGetInfoSeg:far

	extrn	VioWrtTTY:far


_TEXT	segment word public 'CODE'


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SwitchDS
;
; Purpose:
;   To save caller's DS, and set DS to DGROUP.  SS is unchanged, and
;   hence may be invalid (if SS != DD), therefore this is provided at the
;   caller's own risk.  Must be undone by RestoreDS() before return().
;
; Entry:
;   None
;
; Exit:
;   DS = DGROUP, old DS saved on caller's stack.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:nothing,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	SwitchDS
	ENDIF
SwitchDS proc near
	pop	ax		    ;remove return address
	push	ds		    ;save caller's DS
	push	seg _DATA
	pop	ds		    ;set DS to our own DGROUP
	jmp	ax		    ;return
SwitchDS  endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; RestoreDS
;
; Purpose:
;   To restore caller's DS.  Undoes a SwitchDS() call.  See SwitchDS for
;   details.
;
; Entry:
;   None
;
; Exit:
;   DS = caller's original value.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:nothing,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	RestoreDS
	ENDIF
RestoreDS proc near
	pop	ax		    ;remove return address
	pop	ds		    ;restore DS
	jmp	ax		    ;return
RestoreDS endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SwitchStacks(words)
;
; Purpose:
;   To save the caller's registers and stack context.
;   Used immediately upon entry to ALL entry points into the DLL,
;   whether it's the initialization procedure, an exported function,
;   or the exit-list handler.
;
; Entry:
;   # of function argument words (pushed on stack before called).
;
; Exit:
;   Stacks switched, registers saved on new stack (allocated from DGROUP).
;   Returns AX = 0 if success, !0 if failure (ie, no stacks available).
;   All callers MUST test the return value, and immediately exit on failure.
;   Failure to do so will violate the model of this library (SS = DS), and
;   almost certainly result in death for the calling process.  Note that
;   if I detect the caller is already running on one of our stacks, I just
;   bump its ref count and exit w/o error.
;
;   A side-effect of this routine is to insure that the global PGinfoseg and
;   PLinfoseg pointers have been initialized.  I use PLinfoseg here to get
;   the caller's PID, so I can flag the assigned stack as belonging to him.
;   Also, assuming the routine succeeds, signals will be held until either
;   (1) I execute a corresponding RestoreStacks(), or (2) I perform a
;   SaveSignals() call.  See SaveSignals() for more on this exciting topic.
;
; Notes:
;   On entry, I have to push the caller's registers on the CALLER'S
;   stack, so I have something to work with.  On exit however, it is
;   desirable to have those registers saved on the NEW stack, and record
;   the caller's SS:SP as their stack existed before they called into the
;   library.
;
;   Hence, the NEW stack will contain the following upon exit:
;	1a. Caller's SS
;	1b. Caller's SP (value before library function called)
;	1c. Caller's BP
;	 2. Caller's [BP]
;	 3. Caller's BX,CX,DX,DS,ES,SI,DI (only AX is NOT saved)
;	4a. Function arguments
;	4b. Far return address from original library entry
;	 5. # of function argument words (instead of a frame ptr)
;	 6. Local function storage
;
;   The last three items represent the stack frame I copy from the old stack
;   to the new.  If the # words passed on entry is NULLFRAME (-1), then I
;   assume that there is no genuine frame (ie, BP is meaningless), and push
;   only the NULLFRAME argument onto the new stack.
;
;   Also, there's the question of what do when no stacks are available.  This
;   should never happen in practice;  since DGROUP can grow to 64k and since
;   a fairly small portion (10%?) is all that's needed for other purposes, we
;   should run well with the remaining no. of 1k stacks.  At this time, I
;   require all SwitchStacks callers to immediately exit upon error, but I
;   could optionally wait for a stack to become available.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:nothing,es:nothing,ss:nothing

SwitchStacks proc near
	mov	ax,ss		    ;check for simple case
	cmp	ax,seg _DATA	    ;are we already on one of our stacks?
	jne	ss0		    ;no

	pop	ax		    ;get return address
	add	sp,2		    ;throw away parameter
	push	bx		    ;and just save the registers C doesn't
	push	cx
	push	dx
	push	ds
	push	es
	push	seg _DATA
	pop	ds
	assume	ds:_DATA

	call	GetStackbase	    ;get stack base in BX
	inc	[bx].sb_refs	    ;increment reference count
	mov	bx,ax
	sub	ax,ax		    ;not an error
	jmp	bx		    ;QUICK EXIT

	assume	ds:nothing
ss0:	push	bx		    ;save all the registers used
	push	cx
	push	dx
	push	ds
	push	es
	push	si		    ;these two have to be copied back to
	push	di		    ;the caller's actual regs before I leave

	mov	bx,seg _DATA
	mov	ds,bx
	assume	ds:_DATA
	mov	cx,[PGinfoseg].sel
	or	cx,cx
	jnz	save_noinit	    ;infoseg addresses already obtained
	push	ds
	push	offset [PGinfoseg].sel
	push	ds
	push	offset [PLinfoseg].sel
	call	DosGetInfoSeg	    ;otherwise, obtain the selectors now

save_noinit:
	mov	es,[PLinfoseg].sel  ;get current process PID in DX
	mov	dx,es:[lis_pidCurrent]
	mov	cx,es:[lis_tidCurrent]
	mov	es,bx		    ;get thread # also
	assume	es:_DATA

	call	BeginCritical	    ;BEGIN CRITICAL LIST MANIPULATION

	mov	bx,[stackfree]	    ;get first link from head
	or	bx,bx
	jnz	ss1
	call	AddStacks	    ;allocate some more stacks
	jz	ss2		    ;error

ss1:	inc	[infoblock].ib_stacksws
	jnz	ss1a
	dec	[infoblock].ib_stacksws
ss1a:	mov	ax,[bx].sb_link     ;take the link to the next block
	mov	[stackfree],ax	    ;and make that the new head
	mov	[bx].sb_pid,dx	    ;save PID retrieved from PLinfoseg earlier
	mov	[bx].sb_tid,cx	    ;save thread #
	mov	ax,-1
	mov	[bx].sb_sigsaves,ax
	mov	[bx].sb_sigholds,ax ;initialize counts
	lea	di,[bx].sb_sigacts
	mov	si,cx		    ;save CX
	mov	cx,MAXSIGS
	cld
	rep	stosw		    ;init all sigacts to -1
	mov	cx,si		    ;restore CX
	inc	ax
	mov	[bx].sb_sigssaved,ax;clear save flag
	mov	[bx].sb_lockout,ax  ;clear lockout flag
	mov	[bx].sb_heldsp,ax   ;as long as this is zero, it's not held
	inc	ax
	mov	[bx].sb_refs,ax     ;initialize reference count
	inc	cx		    ;bump thread to prevent unhold for thread 1

ss2:	pushf
	call	EndCritical	    ;END CRITICAL LIST MANIPULATION
	popf
	jnz	ss2a
	jmp	save_error

ss2a:	dec	cx		    ;restore CX to actual thread #
	push	ss
	pop	es
	assume	es:nothing
	mov	di,sp		    ;ES:DI is current SS:SP
	mov	si,di
	add	di,9*2		    ;set up ES:DI to address stack before call
	add	si,6*2		    ;set up ES:SI to address saved regs
	push	ds
	pop	ss		    ;SWITCHING STACKS NOW
	lea	sp,[bx+STACKSIZE]

	mov	ax,es:[di-1*2]	    ;get caller's arg word count

	cmp	ax,VIOROUTERFRAME   ;assume VIO router frame?
	jne	ss3a		    ;no
	mov	bx,es:[bp+10]	    ;this is where the router index is
	cmp	bx,MAXVIOINDEX
	ja	ss3d
	mov	al,viorouterargs[bx]
	jmp	short ss3c	    ;got # arg words

ss3a:	cmp	ax,KBDROUTERFRAME
	jne	ss3b
	mov	bx,es:[bp+10]	    ;this is where the router index is
	cmp	bx,MAXKBDINDEX
	ja	ss3d
	mov	al,kbdrouterargs[bx]
	jmp	short ss3c	    ;got # arg words

ss3b:	cmp	ax,MOUROUTERFRAME
	jne	ss3d
	mov	bx,es:[bp+10]	    ;this is where the router index is
	cmp	bx,MAXMOUINDEX
	ja	ss3d
	mov	al,mourouterargs[bx]
ss3c:	cbw
	add	ax,5		    ;add 5 more (standard) router words

ss3d:	mov	bx,ax		    ;BX gets "real" arg word count
	sub	ax,ax		    ;amount of local storage
	cmp	bx,NULLFRAME
	je	ss3e		    ;no frame exists
	mov	ax,bp
	sub	ax,di		    ;count local storage (may even be zero)
	add	di,bx
	add	di,bx		    ;and offset ES:DI by number of arg words
	add	di,1*2		    ;offset for frame ptr, too

ss3e:	add	di,ax		    ;now adjust for any local storage
	add	di,4		    ;and adjust for far return, too
	shr	ax,1		    ;convert local storage count to words

	push	ax
	cmp	cx,1		    ;is this thread #1?
	jne	save_notthread1     ;no
	mov	cx,di
	call	TestHeld	    ;see if any held stacks I should free

save_notthread1:
	pop	dx		    ;move local storage word count to DX
	std			    ;PART 1:
	mov	cx,7
	push	es		    ;save old SS
	push	di		    ;save old SP (adjusted to top of frame)
	push	bp		    ;save old BP
	cmp	bx,NULLFRAME
	je	ss4
	mov	ax,es:[bp]	    ;PART 2:
ss4:	push	ax		    ;save old [BP]

ss5:	lods	word ptr es:[si]    ;PART 3:
	push	ax		    ;save old BX,CX,DX,DS,ES,SI,DI
	loop	ss5
				    ;PART 4:
	lea	si,[di-1*2]	    ;now point to very top of old frame
	mov	cx,2		    ;always count far return
	cmp	bx,NULLFRAME
	je	ss6		    ;no frame, just copy far return
	add	cx,bx
ss6:	lods	word ptr es:[si]
	push	ax
	loop	ss6		    ;function args copied to new stack

	cmp	bx,NULLFRAME	    ;PART 5:
	je	ss7		    ;there isn't any frame ptr to skip
	sub	si,2		    ;otherwise, skip it
ss7:	push	bx		    ;dummy ptr (for remembering # arg words)
	mov	bp,sp
				    ;PART 6:
	mov	cx,dx		    ;set up local storage word count
	jcxz	ss9
ss8:	lods	word ptr es:[si]
	push	ax
	loop	ss8		    ;local storage copied to new stack

ss9:	cld
	mov	bx,es:[si-1*2]	    ;get return address
	mov	di,es:[si-8*2]	    ;restore DI with entry value
	mov	si,es:[si-7*2]	    ;SI, too
	sub	ax,ax
	jmp	bx		    ;EXIT

save_error:
	pop	di
	pop	si
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx		    ;caller's registers restored
	mov	ax,1		    ;this is what I return on error
	ret	2		    ;EXIT
SwitchStacks endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; RestoreStacks()
;
; Purpose:
;   Undoes everything SwitchStacks() did.
;
; Entry:
;   Nothing.  I verify SS = DS, and return if not (but should never
;   happen in practice).
;
; Exit:
;   Stacks switched backed, registers restored to values at time of
;   SwitchStacks(), with exception of AX.  This is also subject to the
;   ref count of the stack:  if it's greater than 1, then this is a
;   nested call, for which I simply return.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

RestoreStacks proc near
	mov	ax,ss
	cmp	ax,seg _DATA	    ;are we running on one of my stacks?
	jne	restore_exit	    ;no
	call	GetStackbase
	dec	[bx].sb_refs	    ;decrement ref count
	jz	rs0		    ;release the stack when it goes to zero

	pop	ax		    ;get return address
	pop	es		    ;restore caller's regs
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	jmp	ax		    ;QUICK EXIT

restore_exit:
	ret			    ;QUICK EXIT (this would be difficult to do)

rs0:	push	si
	push	di
	push	ds
	pop	es
	assume	es:_DATA
	mov	di,sp

	mov	cx,2		    ;count far return
	mov	dx,3*2		    ;minimum local storage to copy
	mov	bx,[bp] 	    ;get caller's arg word count
	cmp	bx,NULLFRAME
	je	rs1		    ;no frame exists
	add	cx,bx		    ;add # arg words to count
	mov	dx,bp
	sub	dx,di		    ;count local storage

rs1:	add	di,dx		    ;adjust for any local storage
	add	di,cx
	add	di,cx		    ;and adjust for # arg words/far ret
	shr	dx,1		    ;convert local storage count to words

	mov	ss,[di+11*2]	    ;RESTORING STACKS NOW
	mov	sp,[di+10*2]
	mov	bp,[di+9*2]	    ;get old BP too

	std			    ;PUSH CALLER'S ARGS BACK
	mov	si,di		    ;now point to very top of current frame
rs2:	lods	word ptr es:[si]
	push	ax
	loop	rs2		    ;function args copied to old stack

	sub	si,2		    ;always skip our dummy ptr
	mov	cx,dx		    ;set up local storage word count
	cmp	bx,NULLFRAME
	je	rs3		    ;they don't have a frame ptr to push
	push	[di+8*2]	    ;PUSH CALLER'S FRAME PTR BACK

rs3:	lods	word ptr es:[si]    ;PUSH CALLER'S LOCAL STORAGE BACK
	push	ax
	loop	rs3		    ;local storage copied to old stack

	mov	cx,5		    ;PUSH CALLER'S ORIGINAL REGISTERS BACK
	lea	si,[di+7*2]
rs4:	lods	word ptr es:[si]
	push	ax		    ;push old BX,CX,DX,DS,ES
	loop	rs4
	cld

	lea	bx,[di+12*2-STACKSIZE]
	sub	cx,cx		    ;dummy thread # to prevent another hold
	call	BeginCritical	    ;BEGIN CRITICAL LIST MANIPULATION

	call	LinkFree	    ;add this stack block to the free list

	mov	cx,[bx].sb_tid
	call	EndCritical	    ;END CRITICAL LIST MANIPULATION

	pop	es		    ;restore original registers
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	di		    ;SI,DI and return address were copied back
	pop	si
	ret			    ;EXIT
RestoreStacks endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; AddStacks
;
; Purpose:
;   To extend the length of the DGROUP segment in order to allocate
;   additional stacks.
;
; Entry:
;   Nothing.
;
; Exit:
;   On error, Zero flag is set.  Reasons are (1) there's no more room in
;   the segment, or (2) DosReallocSeg failed even though the segment
;   had room.
;
;   On success, Zero flag is clear, and BX contains the base address
;   of a new set of linked stacks.
;
; Uses:
;   AX, BX and DI.
;
; Notes:
;   This routine should be modified to squeeze as many stacks as it can
;   out of what space remains, once space has gotten "tight".  Currently,
;   it tries to allocate a default number, and if it can't, it returns
;   error.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:_DATA,ss:nothing

	IFDEF	DEFINE
	public	AddStacks
	ENDIF
AddStacks proc near
	mov	ax,ds
	lsl	ax,ax
	mov	di,ax		    ;save current limit
	add	ax,STACKSIZE * DEFSTACKS + 1
	jz	add_ok		    ;zero just means 64k
	jc	add_error	    ;overflow
add_ok:
	push	ax		    ;new size
	push	ds		    ;selector
	call	DosReallocSeg
	or	ax,ax
	jnz	add_error	    ;yet another error

	cld
	inc	di		    ;DI points to first of new blocks
	mov	bx,di		    ;save a copy in BX
	push	cx
	mov	cx,DEFSTACKS	    ;counter

	cmp	[stackhead],ax	    ;is stackhead zero still?
	jnz	add_init	    ;nope
	mov	[stackhead],bx	   ;initialize it then

add_init:
	lea	ax,[di+STACKSIZE]
	stosw			    ;initialize link
	sub	ax,ax
	stosw			    ;null PID
	stosw			    ;null TID
	stosw			    ;null refs
	stosw			    ;null heldsp
	mov	ax,FILL
	push	cx
	mov	cx,(STACKSIZE-10)/2
	.errnz	(STACKSIZE-10) AND 1
	rep	stosw		    ;initialize the space in the block
	pop	cx
	loop	add_init
	pop	cx
				    ;make sure link in last new block is null
	mov	[di-STACKSIZE].sb_link,0
	or	al,1		    ;ensure zero flag clear
	ret			    ;EXIT

add_error:
	sub	ax,ax		    ;return zero flag set on error
	ret			    ;EXIT
AddStacks endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; FreeStacks()
;
; Purpose:
;   To scan ALL our stacks and return those owned by the current process
;   to the free list.  This is really only for use by the exit-list handler.
;   Its current register usage and assumptions, including the assumption
;   that we're NOT currently running on one my stacks, make it unsuitable for
;   much else.
;
; Entry:
;   Nothing.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX, BX, CX, DX, DS and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:nothing,es:nothing,ss:nothing

FreeStacks proc near
	push	seg _DATA
	pop	ds
	assume	ds:_DATA

	push	offset stacksem
	call	CheckSem	    ;make sure semaphore isn't hosed

	sub	cx,cx		    ;don't be discriminating
	mov	es,[PLinfoseg].sel  ;get (dying) PID of interest in DX
	mov	dx,es:[lis_pidCurrent]
	call	TestHeld	    ;release all held stacks for process

	sub	cx,cx
	call	BeginCritical	    ;BEGIN CRITICAL LIST MANIPULATION

	mov	bx,[stackhead]
	or	bx,bx
	jz	free_exit	    ;zippo stack blocks
	mov	cx,ds
	lsl	cx,cx		    ;get segment limit in CX

free_loop:
	cmp	bx,cx
	ja	free_exit	    ;no more stack blocks
	cmp	[bx].sb_pid,dx	    ;current block contain PID of interest?
	jne	free_skip
	call	LinkFree	    ;free this sucker
free_skip:
	add	bx,STACKSIZE
	jnc	free_loop

free_exit:
	sub	cx,cx
	call	EndCritical	    ;END CRITICAL LIST MANIPULATION

	ret			    ;EXIT
FreeStacks endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GetInfo()
;
; Purpose:
;   To return a pointer to the stack info block, which will be filled
;   in with up-to-date information before returning.
;
; Entry:
;   Nothing.
;
; Exit:
;   AX points to global info block.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

GetInfo proc near
	sub	ax,ax		    ;counter for ib_stacksalloc
	mov	[infoblock].ib_stacksbusy,ax
	mov	[infoblock].ib_stacksheld,ax
	mov	[infoblock].ib_stacksalloc,ax
	mov	[infoblock].ib_stacksize,size sb_space
	mov	[infoblock].ib_stackszused,ax
	mov	dx,ds
	lsl	dx,dx		    ;get segment limit in DX

	mov	ax,seg _DATA
	mov	es,ax
	assume	es:_DATA

	sub	cx,cx
	call	BeginCritical	    ;BEGIN CRITICAL LIST SCANNING

	cld
	push	di
	mov	ax,[stackhead]
	or	ax,ax
	jnz	get_start
	mov	ax,dx
	inc	ax
get_start:			    ;AX is start of stack space
	neg	ax		    ;now, AX is total stack space available
	IF	STACKSIZE EQ 2048
	shr	ax,11		    ;divide by struc_stackblock size
	ELSE
	push	dx
	sub	dx,dx
	mov	bx,STACKSIZE
	div	bx
	pop	dx
	ENDIF
	mov	[infoblock].ib_stacksmax,ax

	mov	bx,[stackhead]
	or	bx,bx
	jz	get_scanexit	    ;zippo stack blocks

get_scanloop:
	cmp	bx,dx
	ja	get_scanexit	    ;no more stack blocks
	inc	[infoblock].ib_stacksalloc
	cmp	[bx].sb_pid,0	    ;is stack busy (ie, owned?)
	je	get_notowned	    ;no
	inc	[infoblock].ib_stacksbusy
get_notowned:
	cmp	[bx].sb_heldsp,0    ;is stack (perhaps also) held?
	je	get_notheld	    ;no
	inc	[infoblock].ib_stacksheld
get_notheld:
	lea	di,[bx].sb_space    ;point to start of stack space
	mov	cx,size sb_space/2
	mov	ax,FILL
	repe	scasw		    ;scan as long as we see FILL
	inc	cx
	shl	cx,1		    ;convert to # bytes
	cmp	cx,[infoblock].ib_stackszused
	jb	get_notmuchused
	mov	[infoblock].ib_stackszused,cx
get_notmuchused:
	add	bx,STACKSIZE
	jnc	get_scanloop

get_scanexit:
	pop	di

	sub	cx,cx
	call	EndCritical	    ;END CRITICAL LIST SCANNING

	mov	ax,offset [infoblock]
	ret			    ;EXIT
GetInfo endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GetStackbase
;
; Purpose:
;   To return the base (ie, lowest) address of the current stack block.
;
; Entry:
;   SP points to a stack block in DGROUP.
;
; Exit:
;   BX points to the base address of that stack block.
;
; Uses:
;   Only BX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

	IFDEF	DEFINE
	public	GetStackbase
	ENDIF
GetStackbase proc near
	mov	bx,sp
	sub	bx,[stackhead]	    ;make pointer relative to 0

	IF	STACKSIZE EQ 2048
	shr	bx,11		    ;divide by STACKSIZE
	shl	bx,11		    ;multiply back by STACKSIZE
	ELSE
	push	dx
	push	ax
	sub	dx,dx
	mov	ax,bx
	mov	bx,STACKSIZE
	div	bx
	mul	bx
	mov	bx,ax
	pop	ax
	pop	dx
	ENDIF

	add	bx,[stackhead]	    ;make pointer relative to stackhead again
	ret			    ;EXIT
GetStackbase endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SaveSignals()
;
; Purpose:
;   To save the caller's complete signal-handling status, for those
;   library calls where I may spend an indeterminate amount of time,
;   and so for which I want to switch to my own handler and undo the
;   DosHoldSignal.  If we're thread #1, this means saving all previous
;   signal handlers and defining our own, which by default will allow all
;   that were allowed before (it just allows them to occur by temporarily
;   switching stacks again).  If we're not thread #1, then all I do is
;   unhold signals, because this call is not intended to inhibit signals.
;   Use HoldSignals for that purpose.
;
; Entry:
;   SP points to *some* point in OUR stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

SaveSignals proc near		    ;SaveSignals()
	call	GetStackbase	    ;get stack base in BX
SaveSignals1:
	inc	[bx].sb_sigsaves
	jnz	save_done	    ;all already done
	cmp	[bx].sb_tid,1	    ;thread #1?
	jne	save_unhold	    ;no, so just unhold signals
	call	GetSignals
save_unhold:
	push	0		    ;I can unhold signals now
	call	DosHoldSignal
save_done:
	ret
SaveSignals endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; RestoreSignals()
;
; Purpose:
;   To undo the action done by a corresponding SaveSignals().
;
; Entry:
;   SP points to *some* point in OUR stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

RestoreSignals proc near	    ;RestoreSignals()
	call	GetStackbase	    ;get stack base in BX
RestoreSignals1:
	dec	[bx].sb_sigsaves
	jns	restore_done	    ;all already done
	push	1		    ;I can rehold signals now
	call	DosHoldSignal
	cmp	[bx].sb_tid,1	    ;thread #1?
	jne	restore_done	    ;no, so just unhold signals
	call	PutSignals
restore_done:
	ret
RestoreSignals endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; HoldSignals()
;
; Purpose:
;   To lock out Ctrl-C and Ctrl-Break signals without resorting to
;   DosHoldSignal.  Currently, this can't be done unless the caller
;   is also thread #1;	if not, I MUST resort to DosHoldSignal.  Reason
;   is because the old signal handler info must be in stored in thread
;   1's stack block, and thread 1 may not even be using one of my stack
;   blocks.  An alternative is maintain "signal blocks", apart from
;   stack blocks, on a per-process basis, but this is more work than I
;   want to get into right now.
;
; Entry:
;   SP points to *some* point in OUR stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

HoldSignals proc near		    ;HoldSignals()
	call	GetStackbase	    ;get stack base in BX
	inc	[bx].sb_sigholds
	jnz	hold_done
	cmp	[bx].sb_tid,1	    ;are we thread #1?
	je	hold_save	    ;yes
	push	1
	call	DosHoldSignal	    ;no, so must hold signals
	ret			    ;EXIT
hold_save:
	call	SaveSignals1	    ;otherwise, I can use my signal handler
hold_done:
	ret
HoldSignals endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ReleaseSignals()
;
; Purpose:
;   To undo the action done by a corresponding HoldSignals().
;
; Entry:
;   SP points to *some* point in OUR stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

ReleaseSignals proc near	    ;ReleaseSignals()
	call	GetStackbase
	dec	[bx].sb_sigholds
	jns	release_done
	cmp	[bx].sb_tid,1	    ;are we thread #1?
	je	release_restore     ;yes
	push	0
	call	DosHoldSignal	    ;no, so we can unhold signals
	ret			    ;EXIT
release_restore:
	call	RestoreSignals1     ;otherwise, I can use my signal handler
release_done:
	ret
ReleaseSignals endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LockThread()
;
; Purpose:
;   To establish a lock for the caller, or report that a lock is already
;   in effect.	Provided for input routines.
;
; Entry:
;   SP points to *some* point in OUR stack.
;
; Exit:
;   AX = 1 (TRUE) if lock succeeded, or 0 (FALSE) if failed.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

LockThread proc near		    ;LockThread()
	call	GetStackbase	    ;get stack base in BX
	mov	ax,1
	xchg	ax,[bx].sb_lockout
	dec	ax		    ;0 implies locked, 1 not
	neg	ax		    ;hence, I return 1 if we locked, 0 if not
	ret
LockThread endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UnLockThread()
;
; Purpose:
;   To remove a lock for the caller, who is trusted not to call if he
;   didn't lock it in the first place.  Provided for input routines.
;
; Entry:
;   SP points to *some* point in OUR stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

UnLockThread proc near		    ;UnLockThread()
	call	GetStackbase	    ;get stack base in BX
	mov	[bx].sb_lockout,0
	ret
UnLockThread endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; SigHandler
;
; Purpose:
;   To pass a signal on to the caller IF I haven't specified that they
;   be suppressed.  The purpose of this routine is having a central place
;   where I try to guarantee stacks are switched before signals are
;   processed.	But of course, I can't *guarantee* that, because other
;   threads for the current process could be running and doing signally-type
;   things all the time.  This is only a "works most of the time hopefully"
;   solution.
;
; Entry:
;   SP points to *some* point in OUR stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   Any (signal handlers don't need to save any regs).
;
; Notes:
;   Even though I've changed signal simulation to simulate only when action
;   is to "call" a handler, there are intervals when our signal handler might
;   get called for a some other action.  Hence, I've left in the code for
;   the "default" and "ignore" cases.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:nothing,es:nothing,ss:_DATA

ctrlc	db	"^C",13,10
ctrlc_len	equ	$-ctrlc

	IFDEF	DEFINE
	public	SigHandler
	ENDIF
SigHandler proc far		    ;SigHandler(sigarg, signum)
	push	ss		    ;get DGROUP addressability
	pop	ds
	assume	ds:_DATA
	mov	bp,sp
	call	GetStackbase	    ;get stack base in BX
	mov	si,[bp+4]
	mov	dx,[bp+6]	    ;save sigarg in DX
	mov	cx,si		    ;save signum in CX
	dec	si
	add	si,si		    ;get action for that signum in AX
	mov	ax,[bx].sb_sigacts[si]

	cmp	[bx].sb_sigholds,-1
	je	sig_nohold
	cmp	cx,1		    ;Ctrl-C?
	je	sig_hold	    ;yes, hold it
	cmp	cx,4		    ;Ctrl-Break?
	je	sig_hold	    ;yes, hold it

sig_nohold:
	or	ax,ax		    ;are we supposed to die?
	jnz	sig_notdie	    ;no
	push	cs
	push	offset _TEXT:ctrlc  ;PUSH &string
	push	ctrlc_len	    ;PUSH length
	push	0		    ;PUSH viohandle
	call	VioWrtTTY
	push	1		    ;PUSH ActionCode
	push	0		    ;PUSH ReturnCode
	call	DosExit 	    ;yes, just terminate
sig_notdie:
	cmp	ax,2		    ;are we supposed to call the handler?
	je	sig_switch	    ;yes
sig_hold:			    ;no, so just re-enable signal and exit
	sub	ax,ax
	push	ax
	push	ax		    ;PUSH NewHandler (none)
	push	ax
	push	ax		    ;PUSH &PrevHandler (none)
	push	ax
	push	ax		    ;PUSH &PrevAction (none)
	push	4		    ;PUSH NewAction (reset signal)
	push	cx		    ;PUSH Signal number
	call	DosSetSigHandler
	jmp	short sig_exit

sig_switch:
	mov	ax,sp
	mov	ss,[bx+STACKSIZE-2]
	mov	sp,[bx+STACKSIZE-4]
	mov	di,sp
	push	ax		    ;save our stack position on theirs
	push	bx
	push	ds
	call	LinkHeld	    ;put stack block on list of held stacks
	call	PutSignals	    ;restore caller's signal handlers
	push	dx		    ;PUSH SigArg
	push	cx		    ;PUSH SigNumber
	add	si,si
	call	[bx].sb_sighdlrs[si]
	pop	ds
	pop	bx
	call	GetSignals	    ;save caller's signal handlers again
	call	UnlinkHeld	    ;remove stack block from list of held ones
	pop	ax		    ;recover our current stack position
	push	ds
	pop	ss
	mov	sp,ax		    ;running back on our stack again

sig_exit:
	ret	4
SigHandler endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; GetSignals
;
; Purpose:
;   To copy signal information into the current stack block.
;
; Entry:
;   BX is current stack base.
;
; Exit:
;   Nothing.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	GetSignals
	ENDIF
GetSignals proc near
	push	cx
	mov	ax,1
	mov	cx,ax		    ;initial signal number
	xchg	ax,[bx].sb_sigssaved
	or	ax,ax
	jnz	get_done
	push	si
	push	di
	lea	si,[bx].sb_sigacts
	lea	di,[bx].sb_sighdlrs
get_loop:
	cmp	cx,2		    ;no real signal for #2
	je	get_skip	    ;skip it
	push	cs
	push	offset SigHandler   ;PUSH NewHandler
	push	ds
	push	di		    ;PUSH &PrevHandler
	push	ds
	push	si		    ;PUSH &PrevAction
	push	2		    ;PUSH NewAction (call NewHandler)
	push	cx		    ;PUSH Signal number
	call	DosSetSigHandler
	cmp	word ptr [si],2     ;was previous action to call a handler?
	je	get_skip	    ;yes
	call	PutSignal	    ;no, so put signal back to whatever
get_skip:
	add	si,2
	add	di,4
	inc	cx
	cmp	cx,MAXSIGS
	jbe	get_loop
	pop	di
	pop	si
get_done:
	pop	cx
	ret
GetSignals endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PutSignals
;
; Purpose:
;   To restore signal information from the current stack block.
;
; Entry:
;   BX is current stack base.
;
; Exit:
;   Nothing.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	PutSignals
	ENDIF
PutSignals proc near
	sub	ax,ax
	xchg	[bx].sb_sigssaved,ax
	or	ax,ax
	jz	put_done	    ;nothing there
	push	cx
	push	si
	push	di
	mov	cx,ax		    ;initial signal number
	lea	si,[bx].sb_sigacts
	lea	di,[bx].sb_sighdlrs
put_loop:
	cmp	cx,2		    ;no real signal for #2
	je	put_skip	    ;skip it
	call	PutSignal
put_skip:
	add	si,2
	add	di,4
	inc	cx
	cmp	cx,MAXSIGS
	jbe	put_loop
	pop	di
	pop	si
	pop	cx
put_done:
	ret
PutSignals endp


PutSignal proc near
	mov	ax,-1
	xchg	[si],ax 	    ;signal no longer hooked
	cmp	ax,-1		    ;was it even hooked before?
	je	ps9		    ;no
	push	[di+2]
	push	[di]		    ;PUSH PrevHandler
	push	0
	push	0		    ;PUSH &OurHandler
	push	0
	push	0		    ;PUSH &OurAction
	push	ax		    ;PUSH PrevAction
	push	cx		    ;PUSH Signal number
	call	DosSetSigHandler
ps9:	ret
PutSignal endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; TestHeld
;
; Purpose:
;   Removes stack blocks from the "held" list if it appears that the
;   caller has reset the SP of his old stack (presumably implying they
;   aren't gonna return from their signal handler), or if I'm called
;   from FreeStacks (time of process death).  This is only called on
;   behalf of thread #1.  In SwitchStacks, I actually check the thread
;   ID first, and FreeStacks should only be called during exit-list
;   processing, implying thread 1 is the caller.
;
; Entry:
;   DX = process ID of interest
;   CX = 0 to free all stacks for process, or stack pointer value to check
;
; Exit:
;   Nothing.
;
; Uses:
;   Only AX and CX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	TestHeld
	ENDIF
TestHeld proc near
	cmp	[stackheld],0
	je	th9		    ;nothing to do

	push	si
	push	bx
	push	cx
	mov	cx,1
	call	BeginCritical	    ;very critical section
	pop	cx

	lea	si,[stackheld]
th1:	mov	bx,[si]
	or	bx,bx
	jz	th8		    ;end of list
	cmp	dx,[bx].sb_pid
	jne	th3		    ;this block not for this process
	jcxz	th2
	cmp	cx,[bx].sb_heldsp
	jb	th3		    ;SP is ok, continue holding stack
th2:	mov	[bx].sb_heldsp,0
	mov	ax,[bx].sb_link
	mov	[si],ax 	    ;overwrite ptr to this block with next

	call	LinkFree	    ;now that it's removed from held list,
				    ;add to free list
th3:	mov	si,bx
	jcxz	th1

th8:	mov	cx,1
	call	EndCritical	    ;end of very critical section
	pop	bx
	pop	si

th9:	ret
TestHeld endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LinkHeld
;
; Purpose:
;   Takes the stack block addressed by BX and links it onto the "held"
;   list.
;
; Entry:
;   BX is current stack base.
;   DI is value of SP we switched back to.
;
; Exit:
;   Nothing.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	LinkHeld
	ENDIF
LinkHeld proc near
	push	cx
	mov	cx,1
	call	BeginCritical	    ;very critical section

	mov	ax,[stackheld]
	mov	[bx].sb_link,ax     ;make freed block point to head
	mov	[stackheld],bx	    ;and make head now point to this block
	mov	[bx].sb_heldsp,di   ;record SP at point of holding

	call	EndCritical	    ;end of very critical section
	pop	cx
	ret
LinkHeld endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UnlinkHeld
;
; Purpose:
;   Takes the stack block addressed by BX off the "held" list.
;
; Entry:
;   BX is current stack base.
;
; Exit:
;   Nothing.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	UnlinkHeld
	ENDIF
UnlinkHeld proc near
	push	cx
	mov	cx,1
	call	BeginCritical	    ;very critical section

	mov	ax,bx		    ;AX to hold stack block
	push	si
	lea	si,[stackheld]	    ;start at top of list
uh1:	mov	bx,[si]
	or	bx,bx
	jz	uh9		    ;link at this location is zero
	cmp	bx,ax		    ;is this block same as given block?
	jne	uh8		    ;no
	mov	[bx].sb_heldsp,0    ;yes, so clear this...
	mov	bx,[bx].sb_link
	mov	[si],bx 	    ;and rip it out of the chain
	jmp	short uh9
uh8:	mov	si,bx		    ;otherwise,
	jmp	uh1
uh9:	pop	si
	mov	bx,ax

	call	EndCritical	    ;end of very critical section
	pop	cx
	ret
UnlinkHeld endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LinkFree
;
; Purpose:
;   Takes the stack block addressed by BX and links it onto the free
;   list.
;
; Entry:
;   BX is current stack base.
;
; Exit:
;   AX is zeroed.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	LinkFree
	ENDIF
LinkFree proc near
	mov	ax,[stackfree]
	mov	[bx].sb_link,ax     ;make freed block point to head
	mov	[stackfree],bx	    ;and make head now point to this block
	sub	ax,ax
	mov	[bx].sb_pid,ax	    ;now that block is free, clear this too
	ret
LinkFree endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; BeginCritical
;
; Purpose:
;   Requests stack semaphore, optionally holding signals first.
;
; Entry:
;   CX == 1 (thread #) if signals should be held first.
;
; Exit:
;   Nothing.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	BeginCritical
	ENDIF
BeginCritical proc near
	cmp	cx,1
	jnz	bc1
	push	1
	call	DosHoldSignal
bc1:	push	bx
	push	cx
	push	dx
	push	es
	push	offset stacksem
	call	LockSem
	pop	es
	pop	dx
	pop	cx
	pop	bx
	ret
BeginCritical endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; EndCritical
;
; Purpose:
;   Frees stack semaphore, optionally releasing signals afterward.
;
; Entry:
;   CX == 1 (thread #) if signals should be released afterward.
;
; Exit:
;   Nothing.
;
; Uses:
;   Only AX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

	IFDEF	DEFINE
	public	EndCritical
	ENDIF
EndCritical proc near
	push	bx
	push	offset stacksem
	call	UnLockSem
	pop	bx
	cmp	cx,1
	jnz	ec9
	push	0
	call	DosHoldSignal
ec9:	ret
EndCritical endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LockSem()
;
; Purpose:
;   Requests a semaphore, setting the owner field afterward.
;
; Entry:
;   Address of semaphore (relative to DGROUP) pushed on stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;void LockSem(ps)
;register struct struc_semaphore *ps;
;{
;    while (TRUE) {
;	 DosSemRequest((semptr)(&ps->sem), SEM_WAITFOREVER);
;	 if (TestAndSet(&ps->pid, PLinfoseg->pid, 0))
;	     break;
;    }
;}

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

LockSem proc near
	push	bp
	mov	bp,sp
lock_retry:
	mov	bx,[bp+4]	    ;get address of semaphore
	push	ds
	push	bx		    ;PUSH &semaphore
	push	-1
	push	-1		    ;PUSH infinite timeout
	call	DosSemRequest	    ;what to do on error???
	lea	ax,[bx].sem_owner
	push	ax		    ;PUSH &ps->pid
	mov	es,[PLinfoseg].sel
	push	es:[lis_pidCurrent] ;PUSH PLinfoseg->pid
	push	0		    ;PUSH 0
	call	far ptr TestAndSet  ;set the owner field accordingly
	or	ax,ax
	jz	lock_retry	    ;set failed
	pop	bp
	ret	2
LockSem endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UnLockSem()
;
; Purpose:
;   Frees a semaphore, clearing the owner field beforehand.
;
; Entry:
;   Address of semaphore (relative to DGROUP) pushed on stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX and BX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;void UnLockSem(ps)
;register struct struc_semaphore *ps;
;{
;    ps->pid = 0;
;    DosSemClear((semptr)(&ps->sem));
;}

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

UnLockSem proc near
	push	bp
	mov	bp,sp
	mov	bx,[bp+4]	    ;get address of semaphore
	mov	[bx].sem_owner,0
	push	ds
	push	bx		    ;PUSH &semaphore
	call	DosSemClear	    ;what to do on error???
	pop	bp
	ret	2
UnLockSem endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CheckSem()
;
; Purpose:
;   If the given semaphore is set, but the owner is unknown, release it.
;
; Entry:
;   Address of semaphore (relative to DGROUP) pushed on stack.
;
; Exit:
;   Nothing.
;
; Uses:
;   AX, BX, CX, DX and ES.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;void CheckSem(ps)
;register struct struc_semaphore *ps;
;{
;    if (DosSemWait((semptr)(&ps->sem), SEM_TEST)) {
;	 TestAndSet(&ps->pid, 0, PLinfoseg->pid);
;	 if (!ps->pid)
;	     DosSemClear((semptr)(&ps->sem));
;    }
;}

	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:nothing

CheckSem proc near
	push	bp
	mov	bp,sp
	mov	bx,[bp+4]	    ;get address of semaphore
	push	ds
	push	bx		    ;PUSH &semaphore
	push	0
	push	0		    ;PUSH null timeout
	call	DosSemWait
	or	ax,ax
	jz	check_exit
	lea	ax,[bx].sem_owner
	push	ax		    ;PUSH &ps->pid
	push	0		    ;PUSH 0
	mov	es,[PLinfoseg].sel
	push	es:[lis_pidCurrent] ;PUSH PLinfoseg->pid
	call	far ptr TestAndSet  ;clear the owner field accordingly
	cmp	[bx].sem_owner,0
	jne	check_exit	    ;was it cleared (or unknown)?
	push	ds
	push	bx		    ;PUSH &semaphore
	call	DosSemClear	    ;what to do on error???
check_exit:
	pop	bp
	ret	2
CheckSem endp


	assume	cs:_TEXT,ds:_DATA,es:nothing,ss:_DATA

	public	debug		    ;debug()
debug	proc	near
	int	3
	ret
debug	endp

_TEXT	ends


_IOPLTEXT segment word public 'CODE'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; TestAndSet(addr, to, on)
;
; Purpose:
;   To provide, as an atomic operation, the ability to test a word for
;   a specific value and set it to another specific value if the test succeeds.
;
; Entry:
;   Parameters as shown.
;
; Exit:
;   Returns TRUE if the test was a match (value is then changed);
;   FALSE otherwise (value is unchanged).
;
; Uses:
;   AX, BX, CX and DX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_IOPLTEXT,ds:_DATA,es:nothing,ss:nothing

TestAndSet proc far		    ;TestAndSet(word, to_value, on_value)
	push	bp
	mov	bp,sp
	sub	ax,ax		    ;assume FALSE
	mov	bx,[bp+10]	    ;get DS-relative ptr to word
	mov	cx,[bp+8]	    ;get to-value
	mov	dx,[bp+6]	    ;get on-value
	cli
	cmp	[bx],dx 	    ;is value equal to on-value?
	jne	si1		    ;no
	mov	[bx],cx 	    ;yes, so set to to-value
	inc	ax		    ;return TRUE
si1:	sti
	pop	bp
	ret	6		    ;intersegment return, removing 3 words
TestAndSet endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; InB(port)
;
; Purpose:
;   To input a byte from the specified port.
;
; Entry:
;   Parameters as shown.
;
; Exit:
;   Value input.
;
; Uses:
;   AX and DX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_IOPLTEXT,ds:nothing,es:nothing,ss:nothing

InB	proc	far		    ;InB(port)
	push	bp
	mov	bp,sp
	mov	dx,[bp+6]	    ;get port
	sub	ax,ax
	in	al,dx
	pop	bp
	ret	2		    ;intersegment return, removing 1 word
InB	endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; OutB(port, byte)
;
; Purpose:
;   To output a byte to the specified port.
;
; Entry:
;   Parameters as shown.
;
; Exit:
;   None
;
; Uses:
;   AX and DX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assume	cs:_IOPLTEXT,ds:nothing,es:nothing,ss:nothing

OutB	proc	far		    ;OutB(port, byte)
	push	bp
	mov	bp,sp
	mov	al,[bp+6]	    ;get byte
	mov	dx,[bp+8]	    ;get port
	out	dx,al
	pop	bp
	ret	4		    ;intersegment return, removing 2 words
OutB	endp

_IOPLTEXT ends


	end	main
