	Title	C/Assembly routines for debugging
	Name	DEBUG

;	By Jeff Parsons  Jan '88

	.286p

_TEXT	segment public word 'CODE'

	assume	cs:_TEXT

	public	INT3		    ;int3() for -Gc (pascal) modules
	public	_int3		    ;int3() for non-Gc (pascal) modules
INT3	proc	near
_int3	label	near
	int	3
	ret
INT3	endp

_TEXT	ends

	end
