; Seven Kingdoms: Ancient Adversaries
;
; Copyright 1997,1998 Enlight Software Ltd.
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;

;Filename    : I_BLTT.ASM
;Description : Blt a bitmap to the display surface buffer with color key transparency handling


INCLUDE IMGFUN.inc
INCLUDE COLCODE.inc

.CODE


;-------- BEGIN OF FUNCTION IMGbltTrans ----------
;
; Put an non-compressed bitmap on image buffer.
; It handles color key transparency. The color key code is 255.
;
; Syntax : IMGbltTrans( imageBuf, pitch, x, y, bitmapBuf )
;
; char *imageBuf - the pointer to the display surface buffer
; int  pitch        - pitch of the display surface buffer
; int  x,y       - where to put the image on the surface buffer
; char *bitmapPtr  - the pointer to the bitmap buffer
;
;-------------------------------------------------
;
; Format of the bitmap data :
;
; <short>  width
; <short>  height
; <char..> bitmap image
;
;-------------------------------------------------

		PUBLIC    IMGbltTrans
IMGbltTrans   	PROC   	  imageBuf, pitch, x, y, bitmapPtr
		STARTPROC

		MOV	EAX, imageBuf		; store the address of the image buffer to a variable
		MOV	image_buf, EAX

		;------ get the bitmap width and height -----;

		MOV     AX , DS
		MOV	ES , AX
		MOV     ESI, bitmapPtr

		XOR	EAX, EAX
		LODSW                            ; get bitmap width
		MOV     EBX, EAX

		LODSW                            ; get bitmap height
		MOV     ECX, EAX

		MOV	EDX, pitch	 ; EDX = lineDiff
		SUB	EDX, EBX		 ; lineDiff = pitch - bitmap_width

		CLD                              ; clear direction flag for MOVSB

		;------- pixels copying loop --------;

		CALC_ADDR EDI, x, y, pitch              ; Get the offset to the image buffer address
@@moreLines:
		PUSH	ECX
		MOV     ECX, EBX		 ; ECX is the line pixel counter

		SHR 	ECX, 2
		JZ	SHORT @@nextScan

;-----------------------------------------------------------------------//
; The idea here is to not branch very often so we unroll the loop by four
; and try to not branch when a whole run of pixels is either transparent
; or not transparent.
;
; There are two loops. One loop is for a run of pixels equal to the
; transparent color, the other is for runs of pixels we need to store.
;
; When we detect a "bad" pixel we jump to the same position in the
; other loop.
;
; Here is the loop we will stay in as long as we encounter a "transparent"
; pixel in the source.
;-----------------------------------------------------------------------//

		align 	4
@@same:
		mov 	al, ds:[esi]
		cmp 	al, TRANSPARENT_CODE
		jne 	short @@diff0

@@same0:
		mov 	al, ds:[esi+1]
		cmp 	al, TRANSPARENT_CODE
		jne 	short @@diff1

@@same1:
		mov 	al, ds:[esi+2]
		cmp 	al, TRANSPARENT_CODE
		jne 	short @@diff2

@@same2:
		mov 	al, ds:[esi+3]
		cmp 	al, TRANSPARENT_CODE
		jne 	short @@diff3

@@same3:
		add 	edi,4
		add 	esi,4
		dec 	ecx
		jnz 	short @@same
		jz  	short @@nextScan

;-----------------------------------------------------------------------//
; Here is the loop we will stay in as long as
; we encounter a "non transparent" pixel in the source.
;-----------------------------------------------------------------------//

		align 	4
@@diff:
		mov 	al, ds:[esi]
		cmp 	al, TRANSPARENT_CODE
		je 	short @@same0

@@diff0:
		mov 	es:[edi],al
		mov 	al, ds:[esi+1]
		cmp 	al, TRANSPARENT_CODE
		je 	short @@same1

@@diff1:
		mov 	es:[edi+1],al
		mov 	al, ds:[esi+2]
		cmp 	al, TRANSPARENT_CODE
		je 	short @@same2

@@diff2:
		mov 	es:[edi+2],al
		mov 	al, ds:[esi+3]
		cmp 	al, TRANSPARENT_CODE
		je 	short @@same3

@@diff3:
		mov 	es:[edi+3],al

		add 	edi,4
		add 	esi,4
		dec 	ecx
		jnz 	short @@diff
		jz  	short @@nextScan

;-----------------------------------------------------------------------//
; We are at the end of a scan, check for odd leftover pixels to do
; and go to the next scan.
;-----------------------------------------------------------------------//

		align 	4
@@nextScan:
		mov 	ecx,ebx           	; ebx = bitmap width
		and 	ecx,11b			; if its pixel count is an odd number
		jnz 	short @@oddStuff

;-----------------------------------------------------------------------//
; move on to the start of the next line
;-----------------------------------------------------------------------//

@@nextScan1:
		add 	edi, edx 		; edx = lineDiff

		pop	ecx
		loop	@@moreLines
		jmp 	short @@end

;-----------------------------------------------------------------------//
; If the width is not a multiple of 4 we will come here to clean up
; the last few pixels
;-----------------------------------------------------------------------//

@@oddStuff:
		inc 	ecx
@@oddLoop:
		dec 	ecx
		jz  	short @@nextScan1
		mov 	al, ds:[esi]
		inc 	esi
		inc 	edi
		cmp 	al, TRANSPARENT_CODE
		je  	short @@oddLoop
		mov 	es:[edi-1],al
		jmp 	short @@oddLoop

@@end:          ENDPROC
IMGbltTrans   	ENDP

;----------- END OF FUNCTION IMGbltTrans ----------


END
