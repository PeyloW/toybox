| Trampoline program for auto folder
| Launches the main program from the root directory using Pexec()

	.globl	_start

	.text
_start:
	| Get basepage pointer from stack
	move.l	4(%sp),%a5		| a5 = basepage

	| Calculate memory needed: basepage + text + data + bss + 256 bytes stack
	move.l	#256,%d0		| basepage size
	add.l	12(%a5),%d0		| + p_tlen (text length)
	add.l	20(%a5),%d0		| + p_dlen (data length)
	add.l	28(%a5),%d0		| + p_blen (bss length)
	add.l	#256,%d0		| + stack space

	| Mshrink(0, block, newsize)
	move.l	%d0,-(%sp)		| new size
	move.l	%a5,-(%sp)		| block = basepage address
	clr.w	-(%sp)			| dummy 0
	move.w	#0x4A,-(%sp)		| Mshrink function
	trap	#1
	lea	12(%sp),%sp

	| Cconws - print startup message
	pea	msg_starting(%pc)
	move.w	#0x09,-(%sp)		| Cconws function
	trap	#1
	addq.l	#6,%sp

	| Print filename
	pea	filename(%pc)
	move.w	#0x09,-(%sp)
	trap	#1
	addq.l	#6,%sp

	| Print newline
	pea	msg_newline(%pc)
	move.w	#0x09,-(%sp)
	trap	#1
	addq.l	#6,%sp

	| Pexec(0, filename, cmdline, env)
	clr.l	-(%sp)			| env = NULL (inherit)
	pea	cmdline(%pc)		| command line (empty)
	pea	filename(%pc)		| program filename
	clr.w	-(%sp)			| mode = 0 (load and execute)
	move.w	#0x4B,-(%sp)		| Pexec function number
	trap	#1
	lea	16(%sp),%sp

	| Check for error (negative return value)
	tst.l	%d0
	bmi.s	error

	| Pterm(return_code)
	move.w	%d0,-(%sp)
	move.w	#0x4C,-(%sp)
	trap	#1

error:
	| Save error code
	move.l	%d0,-(%sp)

	| Print error message
	pea	msg_error(%pc)
	move.w	#0x09,-(%sp)
	trap	#1
	addq.l	#6,%sp

	| Print error code (simple decimal conversion)
	move.l	(%sp),%d0
	neg.l	%d0			| make positive for display
	bsr.s	print_number

	| Print newline
	pea	msg_newline(%pc)
	move.w	#0x09,-(%sp)
	trap	#1
	addq.l	#6,%sp

	| Cconin - wait for keypress
	move.w	#0x01,-(%sp)
	trap	#1
	addq.l	#2,%sp

	| Pterm(-1)
	move.w	#-1,-(%sp)
	move.w	#0x4C,-(%sp)
	trap	#1

| Print number in d0 as decimal
print_number:
	lea	num_buffer+8(%pc),%a0
	move.b	#0,-(%a0)		| null terminator
.digit_loop:
	divu.w	#10,%d0
	swap	%d0
	add.b	#'0',%d0
	move.b	%d0,-(%a0)
	clr.w	%d0
	swap	%d0
	tst.w	%d0
	bne.s	.digit_loop
	| Print the number
	move.l	%a0,-(%sp)
	move.w	#0x09,-(%sp)
	trap	#1
	addq.l	#6,%sp
	rts

	.data
msg_starting:
	.asciz	"Starting "
msg_error:
	.asciz	"\r\nPexec error: "
msg_newline:
	.asciz	"\r\n"
cmdline:
	.byte	0

	.bss
num_buffer:
	.space	9
