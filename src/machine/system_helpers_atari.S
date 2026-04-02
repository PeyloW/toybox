#if TOYBOX_TARGET_ATARI
|
| Primitive Interupt handlers.
|

    .extern g_vbl_functions
    .extern g_vbl_tick
    .extern g_clock_functions
    .extern g_clock_tick
    .extern g_active_display_config
    .globl g_vbl_interupt
    .globl g_system_vbl_freq
    .globl g_system_vbl_interupt
    .globl g_clock_interupt
    .globl g_system_clock_interupt

    .extern g_mouse_buttons
    .extern g_mouse_position
    .globl g_mouse_interupt
    .globl g_system_mouse_interupt

    .extern g_joysticks
    .globl g_joystick_interupt
    .globl g_system_joystick_interupt

    .globl g_microwire_write

    .struct
timer_func_next:        ds.l    1
timer_func_freq:        ds.b    1
timer_func_cnt:         ds.b    1
timer_func_func:        ds.l    1
timer_func_context:     ds.l    1

    .struct
display_config_bitmap_start:    ds.l    1
display_config_extra_words:     ds.b    1
display_config_pixel_shift:     ds.b    1

    .data
    .even
    
g_system_vbl_freq:
    .dc.w   0x32
g_system_vbl_interupt:
    .dc.l    0x0
g_system_clock_interupt:
    .dc.l    0x0
g_system_mouse_interupt:
    .dc.l    0x0
g_system_joystick_interupt:
    .dc.l   0x0

    .text
    .even

g_vbl_interupt:
    movem.l %d0-%d2/%a0-%a2,-(%sp)
    ori.w    #0x0700,%sr
    lea.l   g_active_display_config,%a0
    move.l  display_config_bitmap_start(%a0),%d0
    beq.s   .no_active_config
|   move.l  %d0,0x044e.w           ||| Do not let TOS overrwite.
    move.l  %d0,%d2
    move.w  %d0,%d1
    lsr.l   #8,%d1
    swap    %d2
    move.b  %d2,0xffff8201.w
    move.b  %d1,0xffff8203.w
    move.b  %d0,0xffff820d.w
    move.b  %d2,0xffff8205.w
    move.b  %d1,0xffff8207.w
    move.b  %d0,0xffff8209.w
    move.b  display_config_extra_words(%a0),0xffff820f.w
    move.b  display_config_pixel_shift(%a0),0xffff8265.w
.no_active_config:
    addq.l  #1,g_vbl_tick
    move.w    #0x2400,%sr
    lea     g_vbl_functions, %a2
.next_func_v:
    move.l  timer_func_next(%a2),%d0
    beq.s   .no_more_funcs_v
    move.l  %d0,%a2
    move.b  timer_func_freq(%a2),%d0
    sub.b   %d0,timer_func_cnt(%a2)
    bcc.s   .no_call_v
    move.w  g_system_vbl_freq,%d0
    add.b   %d0,timer_func_cnt(%a2)
    move.l  timer_func_func(%a2),%a1
    move.l  timer_func_context(%a2),%a0
    jsr     (%a1)
.no_call_v:
    bra.s   .next_func_v
.no_more_funcs_v:
    movem.l (%sp)+,%d0-%d2/%a0-%a2
#ifdef TOYBOX_SYSTEM_VBL
    move.l  g_system_vbl_interupt,-(%sp)
    rts
#else
    rte
#endif

.even
g_clock_interupt:
    movem.l %d0-%d2/%a0-%a2,-(%sp)
#if TOYBOX_DEBUG_CPU
    move.w  0xffff8240.w,-(%sp)
    move.w  #0x011,0xffff8240.w
#endif
    addq.l   #1,g_clock_tick
    lea     g_clock_functions,%a2
.next_func_c:
    move.l  timer_func_next(%a2),%d0
    beq.s   .no_more_funcs_c
    move.l  %d0,%a2
    move.b  timer_func_freq(%a2),%d0
    sub.b   %d0,timer_func_cnt(%a2)
    bcc.s   .no_call_c
    add.b   #200,timer_func_cnt(%a2)
    move.l  timer_func_func(%a2),%a1
    move.l  timer_func_context(%a2),%a0
    jsr     (%a1)
.no_call_c:
    bra.s   .next_func_c
.no_more_funcs_c:
#if TOYBOX_DEBUG_CPU
    move.w  (%sp)+,0xffff8240.w
#endif
    movem.l (%sp)+,%d0-%d2/%a0-%a2
    move.l  g_system_clock_interupt,-(%sp)
    rts

    .even
g_mouse_interupt:
    move.w  %d0, -(%sp)
#if TOYBOX_DEBUG_CPU
    move.w  0xffff8240.w,-(%sp)
    move.w  #0x102,0xffff8240.w
#endif
    move.b  (%a0),%d0
    and.b   #0x3,%d0
    move.b  %d0,g_mouse_buttons
    move.b  1(%a0),%d0
    ext.w   %d0
    add.w   %d0,g_mouse_position
    move.b  2(%a0),%d0
    ext.w   %d0
    add.w   %d0,g_mouse_position+2
#if TOYBOX_DEBUG_CPU
    move.w  (%sp)+,0xffff8240.w
#endif
    move.w  (%sp)+,%d0
    move.l  g_system_mouse_interupt,-(%sp)
    rts

g_joystick_interupt:
#if TOYBOX_DEBUG_CPU
    move.w  0xffff8240.w,-(%sp)
    move.w  #0x201,0xffff8240.w
#endif
    cmp.b   #0xfe,(%a0)
    bne.s   .not_joy0
    move.b  1(%a0),g_joysticks+0
    bra.s   .joy_done
.not_joy0:
    cmp.b   #0xff,(%a0)
    bne.s   .joy_done
    move.b  2(%a0),g_joysticks+1
.joy_done:
#if TOYBOX_DEBUG_CPU
    move.w  (%sp)+,0xffff8240.w
#endif
    move.l  g_system_joystick_interupt,-(%sp)
    rts

g_microwire_write:
    move.w  #0x07ff,0xffff8924.w
    move.w  %d0,0xffff8922.w
.wait:
    cmp.w   #0x07ff,0xffff8924.w
    bne.s   .wait
    move.w  %d0,0xffff8922.w
    rts

#endif
