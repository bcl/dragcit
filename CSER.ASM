; -----------------------------------------------------------------------
; CSER.ASM - Buffered interrupt-driven serial I/O routines for C.
;            Orignaly written by Paul Ketrick. You may use these
;            routines freely, but please give credit for them to
;            the those listed here in your source code.
;
;            DTR controls added 
;               by Vegetable DataComm                                1989
;
;            Cleaned up for TASM extensions
;               by Peter "The Dragon" Torkelson                      1989
;
;           Should work with MSC 5.10, Turbo C 2.0, and all compilers
;           that follow these calling conventions
; -----------------------------------------------------------------------
; Routines in this file (all C-callable):
; int_init()  - Installs serial interrupt handling routines
; com_init()  - Initializes serial port, sets baud rate, etc.
; com_read()  - Reads a character from the serial port
; com_write() - Sends a characer to the serial port
; com_stat()  - Returns serial port status
; com_exit()  - Disables serial interrupts
; drop_dtr()  - Drops DTR
; raise_dtr() - Raises DTR
; -----------------------------------------------------------------------
; Function usage:
;
; void int_init(com_base, irq_num)
;   Installs serial port interrupt handling for the specified serial port.
;   com_base may equal 0x3f8 or 0x2f8 for IBM's COM1 or COM2, respectively;
;   irq_num in most cases should equal 4 for COM1 and 3 for COM2. This
;   function must be called before com_init() in order to save the
;   serial port base address in memory. At present, interrupt-driven
;   serial communications are supported on only one serial port at
;   a time.
;
; void cominit(baud, parity, d_bits, s_bits)
;   Initializes a serial port for buffered communications.
;   baud is the baud rate divisor to be used;
;   parity should be set as follows: 0=None, 1=Odd, 3=Even,
;   5=Mark, 7=Space; d_bits represents the number of data
;   bits per character and may be 5, 6, 7 or 8; s_bits represents
;   the number of stop bits per character and may be 1 or 2.
;
; void drop_dtr()
;   Does just that!     Vegetable DataComm  1989
;
; void raise_dtr()
;   Does just that!     Vegetable DataComm  1989
;
; int com_read()
;   Returns -1 if there is no received character available; otherwise
;   returns the character received from the serial port.
;
; void com_write(ch)
;   Places the given character in the serial output buffer; if the
;   output buffer is full, the routine waits until it there is
;   room in the buffer for one character.
;
; unsigned int com_stat()
;   Returns the serial port's status as follows:
;   Bit   Value   Description
;   ---   -----   -----------
;    0    0x0001  Delta CTS	;
;    1    0x0002  Delta DSR	;
;    2    0x0004  Delta RI	;
;    3    0x0008  Delta CD	;--- Bits 0-7 are modem status bits
;    4    0x0010  CTS		;
;    5    0x0020  DSR		;
;    6    0x0040  RI		;
;    7    0x0080  CD		;
;    8    0x0100  Received data ready
;    9    0x0200  Overrun error
;   10    0x0400  Parity error
;   11    0x0800  Framing error
;   12    0x1000  Break detect
;   13    0x2000  Transmit ready
;   14    0x4000  Transmitter shift register empty
;   15    0x8000  Always zero
;   Note that bits 8 and 13 reflect the status of the serial input and
;   output buffers respectively, not necessarily the instantaneous
;   UART status.
;
; void com_exit()
;   Disables serial interrupts and resets the serial interrupt vector
;   in preparation for exiting the program.
; -----------------------------------------------------------------------

.MODEL  LARGE,C

; -----------------------------------------------------------------------
;   Data segment
; -----------------------------------------------------------------------
.DATA

;	public	inbuf, outbuf, ibin, ibout, obin, obout, combase, irqnum
;	The above may be made public for debugging

inbsize equ 32              ; serial input buffer size
inbuf	db	inbsize dup(?)	; input buffer
inbend	label	byte		; represents end of input buffer

otbsize equ 64              ; serial output buffer size
outbuf	db	otbsize dup(?)	; output buffer
otbend	label	byte		; represents end of output buffer

ibin    dw  ?               ; points to where next serial input
                            ; character will be stored (input
                            ; buffer "in" address)

ibout   dw  ?               ; points to the first character
                            ; received that has not yet been
                            ; read from the buffer (input
                            ; buffer "out" address)

obin    dw  ?               ; points to where next character to
                            ; be sent out should be stored
                            ; (output buffer "in" address)

obout   dw  ?               ; points to the first character
                            ; placed in the output buffer that
                            ; hasn't been sent out yet (output
                            ; buffer "out" address)

save1   dw  0               ; storage for old interrupt vector
save2	dw	0

combase dw  0               ; holds UART base I/O address       
irqnum  db  0               ; holds IRQ number used by serial port

; -----------------------------------------------------------------------
;   Code segment
; -----------------------------------------------------------------------
.CODE
    public  int_init, com_init, com_read, com_write
    public  com_stat, com_exit, drop_dtr, raise_dtr
;	public	serint		; may be made public for debugging

; -----------------------------------------------------------------------
; int_init() - Installs serial interrupt handling
; -----------------------------------------------------------------------
int_init PROC   
        ARG com_base:WORD, irq_num:BYTE
    
        ; ---------------------------------------------------------------
        cli             ; disable all interrupts while we're
                        ; fooling around with them
        mov dx,com_base
        mov combase,dx  ; store UART base address for use by
                        ; other routines
    
        mov ah,35h      ; DOS get-interrupt-vector function
        mov al,irq_num
        mov irqnum,al   ; save irq number for later use
        add al,8
        int 21h
        mov save1,bx
        mov save2,es    ; save old interrupt vector
    
        mov ah,25h      ; DOS set-interrupt-vector function
        mov al,irqnum   ;
        add al,8        ; Interupt to change 
        mov dx,offset serint
        mov bx,@code    ; To this value (ds:dx)
        push ds
        mov ds,bx
        int 21h         ; install our interrupt-service routine
        pop ds
    
        mov ah,1
        mov cl,irqnum
        shl ah,cl
        not ah
        in  al,21h
        and al,ah       ; enable irq on interrupt controller chip
        out 21h,al
    
        mov dx,combase
        in  al,dx       ; clear any pending received data
        sti             ; safe to turn interrupts on now
        ; ---------------------------------------------------------------
    
        ret
int_init endp

; -----------------------------------------------------------------------
; com_init() - Initializes serial port
; -----------------------------------------------------------------------
com_init PROC
        ARG baud:WORD, parity:BYTE, d_bits:BYTE, s_bits:BYTE

baud_l  equ BYTE PTR baud
baud_h  equ BYTE PTR baud[1]

        mov dx,[combase]
        add dx,3
        mov al,128
        out dx,al       ; prepare to load baud rate divisor
        mov dx,[combase]
        mov al,baud_l   ; get baud rate divisor lsb
        out dx,al
        inc dx
        mov al,baud_h   ; get baud rate divisor msb
        out dx,al
        add dx,2        ; dx=combase+3
        mov al,parity   ; get parity selection
        and al,7
        shl al,1
        shl al,1
        shl al,1
        mov bl,s_bits   ; get number of stop bits
        dec bl
        and bl,1
        shl bl,1
        shl bl,1
        or  al,bl
        mov bl,d_bits   ; get number of data bits
        sub bl,5
        and bl,3
        or  al,bl
        out dx,al       ; set UART line control register
    
        inc dx          ; access uart modem-control register
        mov al,11       ; dtr, rts active, enable uart interrupts
        out dx,al
        mov dx,[combase]
        inc dx          ; enable received-data-ready interrupt only
        mov al,1        ; transmit-ready interrupt will be enabled
        out dx,al       ; when data gets put in the output buffer
    
        ret

com_init ENDP

; -----------------------------------------------------------------------
; drop_dtr() - does just that!     Vegetable DataComm  1989
; -----------------------------------------------------------------------
drop_dtr PROC

        mov     dx,combase
        add     dx,4
        mov     al,0
        in      al,dx
        and     al,0feh
        out     dx,al
        ret

drop_dtr ENDP

; -----------------------------------------------------------------------
; raise_dtr() - does just that!   Vegetable DataComm  1989
; -----------------------------------------------------------------------
raise_dtr PROC

        mov     dx,combase
        add     dx,4
        mov     al,0
        in      al,dx
        or      al,1
        out     dx,al
        ret

raise_dtr ENDP


; -----------------------------------------------------------------------
; com_read() - Read a character from serial buffer
;              returns -1 if none available
; -----------------------------------------------------------------------
com_read PROC

    mov bx,ibout        ; if input buffer's "in" address equals its
    cmp bx,ibin         ; "out" address then there's nothing in buffer
	jz	comrd2
    mov al,[bx]         ; if not, [ibout] points to the first character
    inc bx              ; we should take from the buffer
	cmp	bx,offset DGROUP:inbend
    jb  comrd1a         ; if we're past the end of the buffer,
	mov	bx,offset DGROUP:inbuf	; wrap around
comrd1a:
    mov ibout,bx        ; update buffer "out" pointer
    xor ah,ah           ; return character in AX
	ret
comrd2:                 ; None avalivle, return -1
    mov ax,0ffffh
	ret

com_read ENDP

; -----------------------------------------------------------------------
; com_write() - Sends a character to the serial port
; -----------------------------------------------------------------------
com_write PROC
        ARG char:BYTE

comwr1: 
        mov bx,obin
        inc bx                      ; compute [obin]+1
        cmp bx,offset DGROUP:otbend
        jb  comwr2                  ; if past end of buffer,
        mov bx,offset DGROUP:outbuf ; wrap around
comwr2: 
        cmp bx,obout                ; if [obin]+1 = [obout], buffer is full
        je  comwr1                  ; if the output buffer is full,
                                    ; wait until it is not
        push    bx                  ; save [obin]+1
        mov bx,obin                 ; get address where char will be stored
        mov al,char
        mov [bx],al                 ; store it
        pop bx                      ; get new output buffer "in" address
        mov obin,bx
        mov dx,combase
        inc dx
        mov al,3
        out dx,al                   ;enable transmit-ready interrupts
        ret
    
com_write ENDP

; -----------------------------------------------------------------------
; com_stat() - Returns serial port status
; -----------------------------------------------------------------------
com_stat PROC

    mov dx,combase
	add	dx,5
    in  al,dx                   ; get line status
	mov	ah,al
	inc	dx
    in  al,dx                   ; get modem status
	mov	bx,obin
    inc bx                      ; compute [obin]+1
	cmp	bx,offset DGROUP:otbend
    jb  comst1                  ; wrap around if necessary
	mov	bx,offset DGROUP:outbuf
comst1: cmp bx,obout            ; is output buffer full?
	mov	cl,0
	je	comst2
    mov cl,32                   ; if not, set bit 5 of ah register
comst2: and ah,255-32           ; mask off bit 5
	or	ah,cl

	mov	bx,ibin
    cmp bx,ibout                ; any received data ready?
	mov	cl,0
	je	comst3
    mov cl,1                    ; if so, set bit 1 of ah register
comst3: and ah,255-1            ; mask off bit 0
	or	ah,cl
    ret                         ; return status in ax

com_stat ENDP

; -----------------------------------------------------------------------
; com_exit() - Disables serial interrupts in preparation
;              for program exit
; -----------------------------------------------------------------------
com_exit PROC   
    
        mov dx,combase
        inc dx
        mov al,0
        out dx,al       ; disable TxRdy, other interrupts (comebase + 1)
        add dx,3        
        in  al,dx       ; (combase + 4)
        and al,3
        out dx,al       ; leave dtr, rts active but shut off interrupts
    
        mov ah,1
        mov cl,irqnum
        shl ah,cl
        in  al,21h
        or  al,ah
        out 21h,al      ; disable irq on interrupt controller chip
    
        mov ah,25h
        mov al,irqnum
        add al,8
        mov dx,save1
        push ds
        mov ds,save2
        int 21h         ; restore old interrupt vector
        pop ds
    
        ret
com_exit ENDP

; -----------------------------------------------------------------------
; serint - Serial interupt procedure
;          interrupt handling routines must have far
; -----------------------------------------------------------------------
serint:
        push    ax      ; attribute and must preserve any and all
        push    bx      ; registers they use, except flag register
        push    dx      ; which is automatically saved by the CPU
        push    ds      ; when an interrupt occurs
        mov ax,DGROUP
        mov ds,ax
serin1: 
        mov dx,combase
        add dx,2
        in  al,dx       ; read interrupt-ident reg
        add dx,4
        in  al,dx       ; read modem-stat reg
        dec dx
        in  al,dx       ; read line-status register
        test al,1       ; received data ready?
        jz  serin4      ; jump if not
        push    ax
        test    al,1eh  ; parity/framing/overrun error or break detect?
        jz  serin2      ; jump if no error
        mov dx,combase  ; clear received character buffer
        in  al,dx
        mov al,33       ; display ! for errors
        jmp serin3      ; change this to "jmp intret" to ignore
                        ; communications errors
serin2:
        mov dx,combase
        in  al,dx       ; read received character from uart
serin3:
        mov bx,ibin     ; find out where rec'd data should be stored
        mov [bx],al     ; and store it there
        inc bx
        cmp bx,offset DGROUP:inbend
        jb  serin3a
        mov bx,offset DGROUP:inbuf
serin3a:
        cmp bx,ibout    ; have we overflowed input buffer?
        jz  serin3b     ; if so, ignore the last character rec'd
        mov ibin,bx     ; otherwise update pointer
serin3b:
        pop ax
serin4:
        mov bx,obout    ; if obout does not equal obin then there
        cmp bx,obin     ; is data in the output buffer
        je  intret
        test    al,32   ; is the UART ready to xmit a character?
        jz  intret
        mov al,[bx]
        mov dx,combase  ; send the next character to be sent out to
        out dx,al       ; the uart
        inc bx
        cmp bx,offset DGROUP:otbend
        jb  serin5
        mov bx,offset DGROUP:outbuf
serin5: 
        mov obout,bx
        cmp bx,obin
        jne intret
        mov dx,combase
        inc dx          ; if there's no more data in the output buffer,
        mov al,1        ; disable transmit-ready interrupts
        out dx,al
intret:
        pop ds          ; restore registers
        pop dx
        pop bx
        mov al,20h      ; signal end-of-interrupt to the interrupt
        out 20h,al      ; controller
        pop ax
        iret            ; return from interrupt-handling routine

; -----------------------------------------------------------------------
END

