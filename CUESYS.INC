SUBTTL	Device Table Record, Aliases, Driver Type Symbols

	.286p

;**	Device Table Record
;
;	Devices are described by a chain of these records

SysDev	STRUC
SDevNext	DD	?	;Pointer to next device header
SDevAtt 	DW	?	;Attributes of the device
SDevStrat	DW	?	;Strategy entry point
SDevInt 	DW	?	;Interrupt entry point
SDevName	DB  '        '	;Name of device (only first byte used for block)
SDevProtCS	DW	?	;Protect-mode CS selecter of strategy entry pt
SDevProtDS	DW	?	;Protect-mode DS selecter
SDevRealCS	DW	?	;Real-mode CS segment of strategy entry pt
SDevRealDS	DW	?	;Real-mode DS segment
SysDev	ENDS

;*	Field aliases for use in loading block drivers

SDevCDPBs	equ	byte ptr SDevName+1
SDevDPBseg	equ	word ptr SDevProtCS
SDevBPBvec	equ	word ptr SDevProtDS


;**	Device Driver Type defininitions
;

DEV_CIN		EQU 0000000000000001B ;	 0  2 5 Device is console in
DEV_COUT	EQU 0000000000000010B ;	 1  2 5 Device is console out
DEV_NULL	EQU 0000000000000100B ;	 2  2 5 Device is the Null device
DEV_CLOCK	EQU 0000000000001000B ;	 3  2 5 Device is the clock device
DEV_SPEC	EQU 0000000000010000B ;	 4  2	Devices can support INT 29h
DEV_ADD_ON	EQU 0000000000100000B ;	 5	Device is add-on driver (BWS)
DEV_GIOCTL	EQU 0000000001000000B ;	 6   3	Device supports generic ioctl
DEV_FCNLEV	EQU 0000001110000000B ;9-7    5 Device function level
;		    0000010000000000B ; 10
DEV_30		EQU 0000100000000000B ; 11  2 5 Accepts Open/Close/Removable Media
DEV_SHARE	EQU 0001000000000000B ; 12	Device wants FS sharing checking
DEV_NON_IBM	EQU 0010000000000000B ; 13  2 5 Device is a non IBM device.
DEV_IOCTL	EQU 0100000000000000B ; 14  2	Device accepts IOCTL request
DEV_CHAR_DEV	EQU 1000000000000000B ; 15  2 5 Device is a character device


;	***** BUGBUG *****
;
;   DOS 3.2 Screws us as it uses bit 6 to indicate generic ioctl support
;   for the time being those device drivers are SOL.

;	Level definitions for devices

;		    0000001110000000B	; Device function level
DEVLEV_0    EQU	    0000000000000000B	; DOS 3.0 and before (NEEDS TO BE FIXED)
DEVLEV_1    EQU	    0000000010000000B	; DOS 5.0
DEVLEV_2    EQU	    0000000000000000B
DEVLEV_3    EQU	    0000000000000000B
DEVLEV_4    EQU	    0000000000000000B
DEVLEV_5    EQU	    0000000000000000B


;**	Static Request Header
;
;	This structure defines the fixed first part of a device request
;	record.  Depending upon the request there may be other fields
;	which follow this header.

SrHead	STRUC
ReqLen	DB	?		; Length in bytes of request block
ReqUnit DB	?		; Device unit number
ReqFunc DB	?		; Type of request
ReqStat DW	?		; Status Word
	DD	?		; RESERVED
DevLink DD	?		; link field for use by driver to chain
				;   requests.  (A service in DevHelp)
SrHead	ENDS


;*	Other fields beyond the standard 13 byte header

; Status word masks
STERR	EQU	8000H		; Bit 15 - Error
STINTER EQU	0400H		; Bit 10 - Interim character
STBUI	EQU	0200H		; Bit  9 - Busy
STDON	EQU	0100H		; Bit  8 - Done
STECODE EQU	00FFH		; Error code
WRECODE EQU	0


;**	Device Driver Request Packet Defintions
;


PktMax		equ	18
PktHead 	equ	13

Packet		struc
PktLen		db	?		; length in bytes of packet
PktUnit 	db	?		; subunit number of block device
PktCmd		db	?		; command code
PktStatus	dw	?		; status word
PktDOSLink	dd	?		; reserved
PktDevLink	dd	?		; device multiple-request link
PktData 	db	PktMax dup (?)	; data pertaing to specific packet
Packet		ends

CMDInit 	equ	0			; INIT command
CMDInitBase	equ	27			; INIT command for base DDs
InitcUnit	equ	byte ptr PktData	; number of units returned
InitpEnd	equ	dword ptr PktData+1	; pointer to free mem after dev
InitDevHlp	equ	dword ptr PktData+1	; @@ address of Device Helper router
InitEcode	equ	word ptr PktData+1	; @@ size of code segment
InitEdata	equ	word ptr PktData+3	; @@ size of data segment
InitParms	equ	dword ptr PktData+5	; @@ pointer parameters
InitpBPB	equ	dword ptr PktData+5	; pointer to BPBs
Initdrv 	equ	byte ptr PktData+9	; drive no. assigned to unit 0
						; for resident drivers only:
InitSysiData	equ	byte ptr PktData+11	; SysInit's DGROUP selector

LENInit 	equ	10			; non-resident drivers
LENInitRes	equ	(LENInit + 2)		; resident drivers
		.errnz	PktMax LT LENInit
		.errnz	PktMax LT LENInitRes

CMDMedChk	equ	1			; Media Check
MedChkmedia	equ	byte ptr PktData	; last media byte seen
MedChkflag	equ	byte ptr PktData+1	; -1=change 0=dont know 1=no change
MedChkpVID	equ	dword ptr PktData+2	; pointer to VID
LENMedChk	equ	6
		.errnz	PktMax LT LENMedChk

CMDBldBPB	equ	2			; build BPB
BldBPBmedia	equ	byte ptr PktData	; media byte
BldBPBbuffer	equ	dword ptr PktData+1	; scratch buffer
BldBPBpBPB	equ	dword ptr PktData+5	; returned pointer to BPB
LENBldBPB	equ	9
		.errnz	PktMax LT LENBldBPB

CMDIOCtlR	equ	3			; reserved for 3.x compatability
CMDInput	equ	4			; read data from device
CMDOutput	equ	8			; write data to device
CMDOutputV	equ	9			; write data and verify
CMDInputBypass	     EQU    24			; @@  cache bypass read data	?? Needed any more ??
CMDOutputBypass      EQU    25			; @@  cache bypass write data	?? Needed any more ??
CMDOutputBypassV     EQU    26			; @@  cache bypass write data and verify  ?? Needed any more ??
CMDIOCtlW	equ	12			; reserved for 3.x compatability
IOmedia 	equ	byte ptr PktData	; media byte
IOpData 	equ	dword ptr PktData+1	; transfer address
IOcount 	equ	word ptr PktData+5	; count of bytes/sectors
IOstart 	equ	word ptr PktData+7	; starting sector (block)
						; starting sector is a DD
IOPhysRBA	equ	dword ptr PktData+7	; physical starting sector
IOSFN		equ	word ptr PktData+11
LENIO		equ	13
		.errnz	PktMax LT LENIO

ReadBypassAdj	equ	CMDInputBypass-CMDInput ; used internally by kernel
WriteBypassAdj	equ	CMDOutputBypass-CMDOutput ; used internally by kernel

CMDNDR		equ	5			; non-destructive read
NDRbyte 	equ	byte ptr PktData	; byte read from device
LENNDR		equ	1
		.errnz	PktMax LT LENNDR

CMDInputS	equ	6			; input status
CMDOutputS	equ	10			; output status
LENStatus	equ	0
		.errnz	PktMax LT LENStatus

CMDInputF	equ	7			; input flush
CMDOutputF	equ	11			; output flush
LENFlush	equ	0
		.errnz	PktMax LT LENFlush

CMDOpen 	equ	13			; device open
CMDClose	equ	14			; device close
OCSFN		equ	word ptr PktData	; sfn of open instance for virtualization
OCLastRef	equ	word ptr PktData+2	; flag indicating last close
LENOpnCls	equ	4
		.errnz	PktMax LT LENOpnCls

CMDRemMed	equ	15			; is media removable
LENRemMed	equ	0
		.errnz	PktMax LT LENRemMed

CMDStart EQU	98				; start console output
CMDStop EQU	99				; stop console output
CStpSKG EQU	BYTE PTR PktData+0		; Screen/Keyboard number
LenStp	EQU	1				; Size of request additions.
		.errnz	PktMax LT LenStp

CMDGenIOCtl	EQU	16			; Generic IOCTL
GIOCategory	EQU	BYTE PTR PktData	; Category Code
GIOFunction	EQU	BYTE PTR PktData+1	; Function code
GIOParaPack	EQU	DWORD PTR PktData+2	; pointer to parameter packet
GIODataPack	EQU	DWORD PTR PktData+6	; pointer to data packet
GIOSFN		EQU	WORD PTR PktData+10
LENGenIOCtl	EQU	12
		.errnz	PktMax LT LENGenIOCtl


CMDDeInstall	EQU	20			; De-Install driver
; DINEndLocn	EQU	DWORD PTR PktData
; DINLength	EQU	WORD PTR PktData+5
LENDeIn 	EQU	0
		.errnz	PktMax LT LENDeIn

CMDResMed	EQU	17			; reset media uncertain
LENResMed	equ	0
		.errnz	PktMax LT LENResMed

CMDAddOnPrep	EQU	97			; Prepare for add on
Add_On_NewDD	EQU	DWORD PTR PktData
Add_On_SrvRout	EQU	DWORD PTR PktData
LenAddOn	EQU	4

CMDGetLogMap	EQU	18
CMDSetLogMap	EQU	19
Logical_Drive	EQU	BYTE PTR PktData; logical drive number
LenGetSetLog	EQU	1

CMDPartfixeddisks	EQU  22 	    ;Partitionable Fixed Disks
CMDGetfd_logunitsmap	EQU  23 	    ;Get Fixed Disk/Logical Unit Map

STDDERROR	     EQU     4000H	    ;for DD-defined Error
ERROR_DEVICEDRIVER_DEFINED_BASE EQU 0FE00H  ;for DD-defined Error High Byte


;	Device driver error codes

ERROR_I24_WRITE_PROTECT			EQU	0
ERROR_I24_BAD_UNIT			EQU	1
ERROR_I24_NOT_READY			EQU	2
ERROR_I24_BAD_COMMAND			EQU	3
ERROR_I24_CRC				EQU	4
ERROR_I24_BAD_LENGTH			EQU	5
ERROR_I24_SEEK				EQU	6
ERROR_I24_NOT_DOS_DISK			EQU	7
ERROR_I24_SECTOR_NOT_FOUND		EQU	8
ERROR_I24_OUT_OF_PAPER			EQU	9
ERROR_I24_WRITE_FAULT			EQU	0Ah
ERROR_I24_READ_FAULT			EQU	0Bh
ERROR_I24_GEN_FAILURE			EQU	0Ch
ERROR_I24_DISK_CHANGE			EQU	0Dh
ERROR_I24_WRONG_DISK			EQU	0Fh
ERROR_I24_UNCERTAIN_MEDIA		EQU	10h
ERROR_I24_CHAR_CALL_INTERRUPTED		EQU	11h
ERROR_I24_NO_MONITOR_SUPPORT		EQU	12h
ERROR_I24_INVALID_PARAMETER		EQU	13h


;**	Major and Minor Codes
;	Category(Major) and Function(Minor) Codes
;	-----------------------------------------
;
;	The Major and Minor values are each contained in a byte.
;
;
;	Category Code:
;	(Major Code)
;
;		0... ....		- Microsoft Defined
;		1... ....		- Oem/User Defined
;		.xxx xxxx		- Code
;
;	WARNING:  When adding new category codes update the MAX_CAT_CODE equ.
;
;	Function Code
;	(Minor Code)
;
;		0... ....		- Return error if unsupported
;		1... ....		- Ignore if unsupported
;		.0.. ....		- Intercepted by DOS
;		.1.. ....		- Passed to driver
;		..0. ....		- Function sends data/commands to device
;		..1. ....		- Function querys data/info from device
;		...x xxxx		- Subfunction
;
;
;	Note that the "sends/querys" data bit is intended only to
;	regularize the function set.  It plays no critical role; some
;	functions may contain elements of both command and query.  The
;	convention is that such commands are defined as "sends data".

IOFUNC_SUBFUNC	equ	1Fh		; Function code subfunction
IOFUNC_READ	equ	20h		; Read subfunction bit
IOFUNC_DRIVER	equ	40h		; passed to driver
IOFUNC_IGNERR	equ	80h		; ignore if unsupported
IOCAT_USERDEF	equ	80h		; user defined category

IOMIN_SUBFUNC	equ	1fh		; Minor code subfunction
IOMIN_READ	equ	20h		; Read subfunction bit
IOMIN_DRIVER	equ	40h		; Passed to driver
IOMIN_IGNERR	equ	80h		; Ignore if unsupported


;**	Major and Minor Codes


IOC_SE	equ	 1		; serial device control
	IOSW_RE equ	14H		; restore the 40:0 com port area
	IOSW_ZO equ	34H		; zero out the 40:0 com port area
	IOSW_BR equ	41H		; set baud rate
	IOSR_BR equ	61H		; return baud rate
	IOSW_LC equ	42h		; set line control (stop bits, parity)
	IOSR_LC equ	62h		; return line control
	IOSW_FI equ	03h		; flush input (seen by DOS and driver)
	IOSW_FO equ	04h		; flush output (seen by DOS and driver)
	IOSR_LS equ	65h		; return current line status
	IOSW_MC equ	46h		; return modem control
	IOSR_MC equ	66h		; set modem control
	IOSR_MS equ	67h		; return modem status
	IOSR_SI equ	68h		; return # of input chars available
	IOSR_SO equ	69h		; return # of queued output chars
	IOSW_TI equ	4ah		; transmit immediate
	IOSW_TB equ	4bh		; transmit "break"
	IOSW_XF equ	4ch		; behave as if XOFF received
	IOSW_XN equ	4dh		; behave as if XON received
	IOSW_XE equ	4eh		; enable automatic XON/XOFF behaviour
	IOSW_XD equ	4fh		; disable automatic XON/XOFF behaviour

IOC_TC	EQU	 2		; Terminal Control
	IOTW_KI EQU	03H		; set keyboard intercept characters
	IOTR_KI EQU	23H		; get keyboard intercept characters
	IOTW_VS EQU	44H		; Set video state
	IOTR_VS EQU	64H		; Return video state
	IOTW_SS EQU	45H		; screen shift
	IOTR_SS EQU	66H		; return screen shift status

IOC_SC	EQU	 3		; Screen Control
	IOSC_LS EQU	41H		; Locate SIB
	IOSC_SS EQU	42H		; save segment
	IOSC_RS EQU	43H		; restore segment
	IOSC_EI EQU	44H		; re-enable I/O
	IOSC_IS EQU	45H		; initialize screen
	IOSC_RW EQU	47H		; reset pop-up window
	IOSC_PW EQU	48H		; establish pop-up window
	IOSC_DW EQU	49H		; destroy pop-up window
	IOSC_DI EQU	4AH		; disable I/O
	IOSC_VS EQU	4BH		; Set/return video mode
	IOSC_SH EQU	4CH		; Set/return screen shift
	IOSC_MS EQU	4DH		; Set/return maximum screensegsize
	IOSC_CS EQU	4FH		; Close Screen

IOC_KC	EQU	 4		; Keyboard Control
	IOKC_IM EQU	51H		; Set Input Mode
	IOKC_LK EQU	41H		; Locate KIB
	IOKC_SS EQU	42H		; save segment
	IOKC_RS EQU	43H		; restore segment
	IOKC_CK EQU	44H		; change keyboard images
	IOKC_IK EQU	45H		; initialize keyboard
	IOKC_SL EQU	06H		; set console locus
	IOKC_RL EQU	07H		; reset console locus
	IOKC_PK EQU	48H		; establish pop-up keyboard
	IOKC_DK EQU	49H		; drop pop-up keyboard
	IOKC_DI EQU	4AH		; disable I/O
	IOKC_MS EQU	6DH		; Return keyboard size.
	IOKC_KC EQU	6FH		; Close Keyboard.

IOC_PC	EQU	 5		; printer Control
	IOLW_FC EQU	42H		; Frame control (CPL, LPI)
	IOLR_FC EQU	62H		;
	IOLW_RD EQU	43H		; Re-direction
	IOLR_RD EQU	63H		;
	IOLW_IR EQU	44H		; Infinite re-try
	IOLR_IR EQU	64H		;
	IOLW_AF EQU	48H		; @@ Activate Font


IOC_LP	EQU	 6		; light pen
;;	<undefined>


IOC_MC	EQU	 7		; mouse Control
;;	<undefined>

;IOC_PA EQU	8		; print arbitrater
;	IOPA_SP EQU	040H		; Set arbitrator pushout buffer
;	IOPA_EN EQU	041H		; Enable arbitration
;	IOPA_RG EQU	042H		; Register process as spooler


IOC_DC	EQU	8		; disk control
	IODC_LK EQU	00H		; Lock drive
	IODC_UL EQU	01H		; Unlock drive
	IODC_RM EQU	02H		; Redetermine media
	IODC_SL EQU	03H		; Set Logical Map
	IODC_BR EQU	20H		; Block removable
	IODC_GL EQU	21H		; Get Logical Map
	IODC_SP EQU	43H		; Set device parameters
	IODC_WT EQU	44H		; Write track
	IODC_FT EQU	45H		; Format track
	IODC_GP EQU	63H		; Get device parameters
	IODC_RT EQU	64H		; Read track
	IODC_VT EQU	65H		; Verify track


IOC_PD	EQU	9		; @@ Physical Disk Control
	IOPD_LK EQU	00H		; @@ Lock physical drive
	IOPD_UL EQU	01H		; @@ Unlock physical drive


IOC_MO	EQU	10		; Monitor control
	IOMO_RE EQU	40H		; Register
	IOMO_IO EQU	41H		; I/O

IOC_BC	EQU	11		; Device buffer control
	IOBC_FI EQU	01H		; flush input
	IOBC_FO EQU	02H		; flush output

IOC_SQ	EQU	12		; Single Queue device driver

MAX_CAT_CODE	EQU	12	; the largest legal category code
				; change this equ as new IOCTL
				; categories get added

;
; Command code definitions for certain functions
;

; Category 8, Function 43 - Set Device Parameters

	IODC_SP_DEV	EQU	01H	; Set BPB for physical device
	IODC_SP_MEDIA	EQU	02H	; Set fake BPB for medium


SUBTTL	DevHlp - Definitions for device driver helper functions


DevHlp_SchedClock	EQU	 0	;  0	Called each timer tick
DevHlp_DevDone		EQU	 1	;  1	Device I/O complete
DevHlp_Yield		EQU	 2	;  2	yield CPU if resched set
DevHlp_TCYield		EQU	 3	;  3	yield to time critical task

DevHlp_ProcBlock	EQU	 4	;  4	Block on event
DevHlp_ProcRun		EQU	 5	;  5	Unblock process

DevHlp_SemRequest	EQU	 6	;  6	claim a semaphore
DevHlp_SemClear		EQU	 7	;  7	release a semaphore
DevHlp_SemHandle	EQU	 8	;  8	obtain a semaphore handle

DevHlp_PushRequest	EQU	 9	;  9	Push the request
DevHlp_PullRequest	EQU	10	;  A	Pull next request from Q
DevHlp_PullParticular	EQU	11	;  B	Pull a specific request
DevHlp_SortRequest	EQU	12	;  C	Push request in sorted order

DevHlp_AllocReqPacket	EQU	13	;  D	allocate request packet
DevHlp_FreeReqPacket	EQU	14	;  E	free request packet

DevHlp_QueueInit	EQU	15	;  F	Init/Clear char queue
DevHlp_QueueFlush	EQU	16	; 10	flush queue
DevHlp_QueueWrite	EQU	17	; 11	Put a char in the queue
DevHlp_QueueRead	EQU	18	; 12	Get a char from the queue

DevHlp_Lock		EQU	19	; 13	Lock segment
DevHlp_Unlock		EQU	20	; 14	Unlock segment

DevHlp_PhysToVirt	EQU	21	; 15	convert physical address to virtual
DevHlp_VirtToPhys	EQU	22	; 16	convert virtual address to physical
DevHlp_PhysToUVirt	EQU	23	; 17	convert physical to LDT

DevHlp_AllocPhys	EQU	24	; 18	allocate physical memory
DevHlp_FreePhys		EQU	25	; 19	free physical memory

DevHlp_SetROMVector	EQU	26	; 1A	set a ROM service routine vector
DevHlp_SetIRQ		EQU	27	; 1B	set an IRQ interrupt
DevHlp_UnSetIRQ		EQU	28	; 1C	unset an IRQ interrupt

DevHlp_SetTimer		EQU	29	; 1D	set timer request handler
DevHlp_ResetTimer	EQU	30	; 1E	unset timer request handler

DevHlp_MonitorCreate	EQU	31	; 1F	create a monitor
DevHlp_Register		EQU	32	; 20	install a monitor
DevHlp_DeRegister	EQU	33	; 21	remove a monitor
DevHlp_MonWrite		EQU	34	; 22	pass data records to monitor
DevHlp_MonFlush		EQU	35	; 23	remove all data from stream

DevHlp_GetDOSVar	EQU	36	; 24	Return pointer to DOS variable
DevHlp_SendEvent	EQU	37	; 25	an event occurred
DevHlp_ROMCritSection	EQU	38	; 26	ROM Critical Section
DevHlp_VerifyAccess	EQU	39	; 27	Verify access to memory
DevHlp_RAS		EQU	40	; 28	Put info in RAS trace buffer

DevHlp_ABIOSGetParms	EQU	41	; 29	Get ABIOS Calling Parms
DevHlp_AttachDD		EQU	42	; 2A	Attach to a device driver

DevHlp_AllocGDTSelector EQU	45	; 2D	Allocate GDT Selectors
DevHlp_PhysToGDTSelector EQU	46	; 2E	Convert phys addr to GDT sel
DevHlp_RealToProt	EQU	47	; 2F	Change from real to protected mode
DevHlp_ProtToReal	EQU	48	; 30	Change from protected to real mode

DevHlp_EOI		EQU	49	; 31	Send EOI to PIC
DevHlp_UnPhysToVirt	EQU	50	; 32	mark completion of PhysToVirt
DevHlp_TickCount	EQU	51	; 33	modify timer

DevHlp_GetLIDEntry	EQU	52	; 34	Obtain Logical ID
DevHlp_FreeLIDEntry	EQU	53	; 35	Release Logical ID
DevHlp_ABIOSCall	EQU	54	; 36	Call ABIOS
DevHlp_ABIOSCommonEntry EQU	55	; 37	Invoke Common Entry Point
DevHlp_GetDeviceBlock	EQU	56	; 38	Get ABIOS Device Block


event_SM_Mou		equ	0	; event was Session Manager-mouse@@
    event_MUF		equ	0	; (OBSOLETE) event was muf key
event_CtrlBrk		equ	1	; event was Ctrl-Break
event_CtrlC		equ	2	; event was Ctrl-C
event_CtrlScrLk		equ	3	; event was Ctrl-ScrollLock
event_CtrlPrtSc		equ	4	; event was Ctrl-PrtSc
event_ShftPrtSc		equ	5	; event was Shift-PrtSc
event_SM_Kbd		equ	6	; event was Session Manager -keyboard@@
event_count		equ	7	; @@ Number of possible events

;	Character Queue structure
;
;	QueueInit must be called before any other queue manipulation
;	subroutine.  The Qsize field must be initialized before
;	calling QueueInit.

CharQueue STRUC
	Qsize	DW	?		; Size of queue in bytes
	Qchrout DW	?		; Index of next char out
	Qcount	DW	?		; Count of characters in the queue
	Qbase	DB	?		; Queue buffer
CharQueue ENDS
