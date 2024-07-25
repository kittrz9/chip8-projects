
; funny comment
entry:
	ldv V0, #0 ; end of line comment
	ldv V1, #0
	ldv V2, #a
	call @deez
loop:
	get_sprite V2
	draw V0, V1, #5
	add V2, #1
loop2:
	jmp @loop2
	bcd V1


deez:
	ldv V2, #b
	jmp @beez
	ret

beez:
	ret
