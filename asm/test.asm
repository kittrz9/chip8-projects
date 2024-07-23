
entry:
	ldv V0, 0
	ldv V1, 0
	ldv V2, a
loop:
	get_sprite V2
	draw V0, V1, 5
	add V2, 1
	jmp loop
