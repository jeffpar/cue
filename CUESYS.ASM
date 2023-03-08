	PAGE	58,132
	TITLE	CUE Device Driver (CUE.SYS)

;	By Jeff Parsons  Sep '88

	include cuesys.inc

	extrn	DosGetInfoSeg:far


Patchblk struc
 foff	dw	?			;offset of function
 fsel	dw	?			;selector of function
 slen	dw	?			;search pattern length
 spat	db	6 dup(?)		;search pattern
 rlen	dw	?			;replace pattern length
 rpat	db	6 dup(?)		;replace pattern
 tlen	dw	?			;terminate pattern length
 tpat	db	6 dup(?)		;terminate pattern
Patchblk ends


SysData SEGMENT word public 'DATA'

SYSATTR equ	DEV_CHAR_DEV+DEVLEV_1+DEV_COUT

	public	SysHdr
SysHdr	dd	-1			;pointer to next device header
	dw	SYSATTR 		;attributes of the device
	dw	offset Request		;strategy entry point
	dw	offset far_ret		;interrupt entry point
	db	'CUE$    '		;device name
	dw	0			;protected-mode CS selector of driver
	dw	0			;protected-mode DS selector
	dw	0			;real-mode CS segment of driver
	dw	0			;real-mode DS segment

	public	DevHlp
DevHlp	dd	0			;DevHlp function router address

	public	selGinfo,selLinfo
selGinfo dw	?			;global infoseg selector
selLinfo dw	?			;local infoseg selector

	public	lockseg,lockhandle
lockseg dw	?			;last segment locked down
lockhandle dw	?,?			;handle to last segment locked down

endSysData  label   byte

SysData ENDS


SysCode SEGMENT byte public 'CODE'
	assume	cs:SysCode,ds:SysData,es:nothing,ss:nothing


;***	Request - Device driver strategy routine
;
;	ENTRY	ES:BX = Address of request packet
;
;	EXIT	Status field of request packet filled in
;
;	USES	All
;
;	STEPS	Process generic IOCTLs for Category 83h

	public	Request
Request PROC	far
	push	bx			;save ptr to req pkt
	push	es
	mov	al,es:[bx].ReqFunc
	cmp	al,CMDInit		;Init request?
	jne	request_notinit 	;no
	call	Init			;process the init request
	jmp	short request_done

request_notinit:
	cmp	al,CMDGenIOCtl		;generic IOCTL?
	jne	request_notioctl      ;no
	cmp	es:[bx].GIOCategory,83h
	jne	request_badcmd		;not screen-related category
	call	Ioctl			;otherwise, process the IOCTL
	jmp	short request_done

request_notioctl:			;allow the following requests
	cmp	al,CMDInput
	je	request_ok
	cmp	al,CMDOutput
	je	request_ok
	cmp	al,CMDOutputV
	je	request_ok

request_badcmd:
	mov	ax,STERR+ERROR_I24_BAD_COMMAND
	jmp	short request_done

request_ok:
	sub	ax,ax
	mov	es:[bx].IOcount,ax

request_done:
	or	ax,STDON
	pop	es
	pop	bx			;restore ptr to req pkt
	mov	es:[bx].ReqStat,ax	;set return status
far_ret:
	ret
Request ENDP


;***	Ioctl - Device driver IOCTL routine
;
;	ENTRY	ES:BX = Address of request packet
;
;	EXIT	AX = Status
;
;	USES	All
;
;	STEPS	Process generic IOCTLs for Category 83h
;		    Functions 40h (patch) and 41h (unlock)

	public	Ioctl
Ioctl	PROC	near
	cmp	es:[bx].GIOFunction,40h ;"patch" IOCTL function?
	je	ioctl_patch		;yes
	cmp	es:[bx].GIOFunction,41h ;"unlock" IOCTL function?
	stc
	jne	ioctl_errorexit 	;no

	mov	cx,2
	call	Verify			;verify parameter packet is OK
	jc	ioctl_errorexit 	;is BAD

	cmp	ax,[lockseg]		;is segment same as last one locked?
	stc
	jne	ioctl_errorexit 	;hmmm...
	mov	[lockseg],0
	mov	bx,[lockhandle]
	mov	ax,[lockhandle+2]	;retrieve last lock handle
	mov	dl,DevHlp_Unlock
	call	[DevHlp]		;unlock the sucker
	jc	ioctl_errorexit
	jmp	short ioctl_success

ioctl_patch:
	mov	cx,size Patchblk
	call	Verify			;verify parameter packet is OK

ioctl_errorexit:
	jc	ioctl_error		;is BAD
	push	es			;save address of parmeter packet
	push	bx
	push	ax			;save source selector
	mov	bx,100h 		;BX = long term lock + wait
	mov	dl,DevHlp_Lock
	call	[DevHlp]		;lock that code segment down!
	mov	[lockhandle],bx
	mov	[lockhandle+2],ax
	pop	bx			;restore source selector
	jc	ioctl_badlock		;can work anyway, in spite of bad lock
	mov	[lockseg],bx

ioctl_badlock:
	push	ds
	pop	es
	mov	ds,bx
	sub	si,si			;DS:SI -> segment to patch
	mov	dl,DevHlp_VirtToPhys
	call	es:[DevHlp]		;AX:BX == physical address
	pop	si
	pop	ds
	push	es			;save device driver segment
	mov	dx,DevHlp_PhysToVirt+100h
	call	es:[DevHlp]		;ES:DI -> writeable segment to patch
	dec	cx
	mov	ax,cx			;AX == limit of segment to patch
	mov	bx,si			;DS:BX -> patchblk

	mov	cx,[bx].tlen		;CX = length of ending pattern
	lea	si,[bx].tpat		;DS:SI -> pattern
	mov	di,[bx].foff		;ES:DI -> starting point for search
	call	Search			;find it!
	jc	ioctl_patchdone 	;couldn't find it

	mov	ax,di
	dec	ax			;AX = new limit for segment
	mov	cx,[bx].slen		;CX = length of search pattern
	lea	si,[bx].spat		;DS:SI -> pattern
	mov	di,[bx].foff		;ES:DI -> starting point for search
	call	Search			;find it!
	jc	ioctl_patchdone 	;couldn't find it
					;ES:DI -> pattern to replace
	mov	cx,[bx].rlen		;CX = length of replacement pattern
	lea	si,[bx].rpat		;DS:SI -> pattern
ioctl_patchloop:
	lodsb
	cmp	al,0EEh
	je	ioctl_patchloop2
	stosb
	jmp	short ioctl_patchloop3
ioctl_patchloop2:
	inc	di
ioctl_patchloop3:
	loop	ioctl_patchloop
	clc				;indicate success

ioctl_patchdone:
	pop	ds			;restore device driver segment
	pushf
	mov	dl,DevHlp_UnPhysToVirt
	call	[DevHlp]
	popf
	jc	ioctl_error

ioctl_success:
	sub	ax,ax			;return SUCCESS
	jmp	short ioctl_done

ioctl_error:				;return ERROR
	mov	ax,STERR+ERROR_I24_GEN_FAILURE

ioctl_done:
	ret

Ioctl	ENDP


;***	Verify - IOCTL packet verifier
;
;	ENTRY	CX = size of parm pkt
;		ES:BX -> request packet
;
;	EXIT	SUCCESS
;		    CY = CLEAR
;		    ES:BX -> parameter packet
;		    AX = selector (from 1st word in parameter packet)
;		    CX = size of segment
;
;		FAILURE
;		    CY = SET
;
;	USES	All

	public	Verify
Verify	PROC	near

	mov	ax,word ptr es:[bx].GIOParaPack+2
	mov	di,word ptr es:[bx].GIOParaPack
	mov	dx,DevHlp_VerifyAccess	;verify read access
	call	[DevHlp]		;verify access to segment
	jc	verify_error

	les	bx,es:[bx].GIOParaPack	;ES:BX -> a patchblk
	mov	ax,es:[bx].fsel 	;get selector passed by the user
	sub	di,di			;offset 0
	lsl	cx,ax			;get segment limit
	stc
	jnz	verify_error

	inc	cx			;convert limit to size
	call	[DevHlp]		;verify access to segment

verify_error:
	ret

Verify	ENDP


;***	Search - Return location of pattern in a segment
;
;	ENTRY	AX = limit of segment
;		CX = size of pattern
;		DS:SI -> pattern
;		ES:DI -> segment (starting position)
;
;	EXIT	CY = CLEAR if found (DI indicates position), SET if not
;
;	USES	All but AX, BX, DX, DS, ES

	public	Search
Search	PROC	near

	cld
search_loop:
	mov	bp,di
	dec	cx
	add	bp,cx			;point to last byte we'd compare
	jc	search_bad		;would overflow segment
	cmp	bp,ax			;higher than segment limit?
	ja	search_bad		;yes
	inc	cx
	mov	bp,cx			;save CX
search_next:
	repe	cmpsb			;do the compare from this point
	je	search_good		;found pattern!
	cmp	byte ptr [si-1],0EEh	;0EEh matches anything!
	je	search_next
	neg	cx
	add	cx,bp			;get number of bytes compared
	sub	si,cx			;rewind pattern address
	sub	di,cx			;rewind compare position
	mov	cx,bp			;restore CX
	inc	di			;adjust position for next compare
	jmp	search_loop

search_bad:
	stc
	jmp	short search_exit

search_good:
	sub	di,bp			;return good positions

search_exit:
	ret

Search	ENDP


;***	Init - Device driver initialization routine
;
;	ENTRY	ES:BX = Request packet
;		DS    = Device driver data segment
;
;	EXIT	DevHlp address saved
;		Code segment size set
;		Data segment size set
;		Return code set
;
;	NOTE	This must be the beginning of the non-resident code!

	public	Init
Init	PROC	near

	mov	ax,word ptr es:[bx].InitDevHlp
	mov	word ptr [DevHlp],ax
	mov	ax,word ptr es:[bx].InitDevHlp+2
	mov	word ptr [DevHlp+2],ax

	mov	word ptr es:[bx].InitEcode,offset Init-1
	mov	word ptr es:[bx].InitEdata,offset endSysData-1

	push	ds
	push	offset selGinfo
	push	ds
	push	offset selLinfo
	call	DosGetInfoSeg		;get global/local infoseg selectors

	sub	ax,ax			;return code (success)
	ret

Init	ENDP


SysCode ENDS

	end
