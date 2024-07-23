
entry:
	ldv V0, a
	ldv V1, 0
	ldv V2, 0
	ldv v3, v2
loop:
	get_sprite V2
	draw V0, V1, 5
	add V2, 1
	jmp loop
