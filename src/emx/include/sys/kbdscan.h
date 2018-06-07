/* sys/kbdscan.h (emx+gcc) */

#ifndef _SYS_KBDSCAN_H
#define _SYS_KBDSCAN_H

/* [DOS]: DOS only */
/* [OS2]: OS/2 only */

#define K_ALT_ESC               0x01   /* <Alt>+<Esc>    [DOS]*/
#define K_CTRL_SPACE            0x02   /* <Ctrl>+<Space> [OS2]*/
#define K_CTRL_AT               0x03   /* <Ctrl>+<@> */
#define K_SHIFT_INS             0x04   /* <Shift>+<Ins>  [OS2]*/
#define K_SHIFT_DEL             0x05   /* <Shift>+<Del>  [OS2]*/
#define K_ALT_BACKSPACE         0x0e   /* <Alt>+<Backspace> */
#define K_BACKTAB               0x0f   /* <Shift>+<Tab> */
#define K_ALT_Q                 0x10   /* <Alt>+<Q> */
#define K_ALT_W                 0x11   /* <Alt>+<W> */
#define K_ALT_E                 0x12   /* <Alt>+<E> */
#define K_ALT_R                 0x13   /* <Alt>+<R> */
#define K_ALT_T                 0x14   /* <Alt>+<T> */
#define K_ALT_Y                 0x15   /* <Alt>+<Y> */
#define K_ALT_U                 0x16   /* <Alt>+<U> */
#define K_ALT_I                 0x17   /* <Alt>+<I> */
#define K_ALT_O                 0x18   /* <Alt>+<O> */
#define K_ALT_P                 0x19   /* <Alt>+<P> */
#define K_ALT_LEFT_BRACKET      0x1a   /* <Alt>+<[> */
#define K_ALT_RIGHT_BRACKET     0x1b   /* <Alt>+<]> */
#define K_ALT_RETURN            0x1c   /* <Alt>+<Return> */
#define K_ALT_A                 0x1e   /* <Alt>+<A> */
#define K_ALT_S                 0x1f   /* <Alt>+<S> */
#define K_ALT_D                 0x20   /* <Alt>+<D> */
#define K_ALT_F                 0x21   /* <Alt>+<F> */
#define K_ALT_G                 0x22   /* <Alt>+<G> */
#define K_ALT_H                 0x23   /* <Alt>+<H> */
#define K_ALT_J                 0x24   /* <Alt>+<J> */
#define K_ALT_K                 0x25   /* <Alt>+<K> */
#define K_ALT_L                 0x26   /* <Alt>+<L> */
#define K_ALT_SEMICOLON         0x27   /* <Alt>+<;> */
#define K_ALT_RIGHT_QUOTE       0x28   /* <Alt>+<'> */
#define K_ALT_LEFT_QUOTE        0x29   /* <Alt>+<`> */
#define K_ALT_BACKSLASH         0x2b   /* <Alt>+<\> */
#define K_ALT_Z                 0x2c   /* <Alt>+<Z> */
#define K_ALT_X                 0x2d   /* <Alt>+<X> */
#define K_ALT_C                 0x2e   /* <Alt>+<C> */
#define K_ALT_V                 0x2f   /* <Alt>+<V> */
#define K_ALT_B                 0x30   /* <Alt>+<B> */
#define K_ALT_N                 0x31   /* <Alt>+<N> */
#define K_ALT_M                 0x32   /* <Alt>+<M> */
#define K_ALT_COMMA             0x33   /* <Alt>+<,> */
#define K_ALT_PERIOD            0x34   /* <Alt>+<.> */
#define K_ALT_SLASH             0x35   /* <Alt>+</> */
#define K_ALT_PAD_ASTERISK      0x37   /* <Alt>+<*> (numeric keypad) */
#define K_ALT_SPACE             0x39   /* <Alt>+<Space>  [OS2] */
#define K_F1                    0x3b   /* <F1> */
#define K_F2                    0x3c   /* <F2> */
#define K_F3                    0x3d   /* <F3> */
#define K_F4                    0x3e   /* <F4> */
#define K_F5                    0x3f   /* <F5> */
#define K_F6                    0x40   /* <F6> */
#define K_F7                    0x41   /* <F7> */
#define K_F8                    0x42   /* <F8> */
#define K_F9                    0x43   /* <F9> */
#define K_F10                   0x44   /* <F10> */
#define K_HOME                  0x47   /* <Home> */
#define K_UP                    0x48   /* <Up arrow> */
#define K_PAGEUP                0x49   /* <Page up> */
#define K_ALT_PAD_MINUS         0x4a   /* <Alt>+<-> (numeric keypad) */
#define K_LEFT                  0x4b   /* <Left arrow> */
#define K_CENTER                0x4c   /* Center cursor */
#define K_RIGHT                 0x4d   /* <Right arrow> */
#define K_ALT_PAD_PLUS          0x4e   /* <Alt>+<+> (numeric keypad) */
#define K_END                   0x4f   /* <End> */
#define K_DOWN                  0x50   /* <Down arrow> */
#define K_PAGEDOWN              0x51   /* <Page down> */
#define K_INS                   0x52   /* <Ins> */
#define K_DEL                   0x53   /* <Del> */
#define K_SHIFT_F1              0x54   /* <Shift>+<F1> */
#define K_SHIFT_F2              0x55   /* <Shift>+<F2> */
#define K_SHIFT_F3              0x56   /* <Shift>+<F3> */
#define K_SHIFT_F4              0x57   /* <Shift>+<F4> */
#define K_SHIFT_F5              0x58   /* <Shift>+<F5> */
#define K_SHIFT_F6              0x59   /* <Shift>+<F6> */
#define K_SHIFT_F7              0x5a   /* <Shift>+<F7> */
#define K_SHIFT_F8              0x5b   /* <Shift>+<F8> */
#define K_SHIFT_F9              0x5c   /* <Shift>+<F9> */
#define K_SHIFT_F10             0x5d   /* <Shift>+<F10> */
#define K_CTRL_F1               0x5e   /* <Ctrl>+<F1> */
#define K_CTRL_F2               0x5f   /* <Ctrl>+<F2> */
#define K_CTRL_F3               0x60   /* <Ctrl>+<F3> */
#define K_CTRL_F4               0x61   /* <Ctrl>+<F4> */
#define K_CTRL_F5               0x62   /* <Ctrl>+<F5> */
#define K_CTRL_F6               0x63   /* <Ctrl>+<F6> */
#define K_CTRL_F7               0x64   /* <Ctrl>+<F7> */
#define K_CTRL_F8               0x65   /* <Ctrl>+<F8> */
#define K_CTRL_F9               0x66   /* <Ctrl>+<F9> */
#define K_CTRL_F10              0x67   /* <Ctrl>+<F10> */
#define K_ALT_F1                0x68   /* <Alt>+<F1> */
#define K_ALT_F2                0x69   /* <Alt>+<F2> */
#define K_ALT_F3                0x6a   /* <Alt>+<F3> */
#define K_ALT_F4                0x6b   /* <Alt>+<F4> */
#define K_ALT_F5                0x6c   /* <Alt>+<F5> */
#define K_ALT_F6                0x6d   /* <Alt>+<F6> */
#define K_ALT_F7                0x6e   /* <Alt>+<F7> */
#define K_ALT_F8                0x6f   /* <Alt>+<F8> */
#define K_ALT_F9                0x70   /* <Alt>+<F9> */
#define K_ALT_F10               0x71   /* <Alt>+<F10> */
#define K_CTRL_PRTSC            0x72   /* <Ctrl>+<PrtSc> */
#define K_CTRL_LEFT             0x73   /* <Ctrl>+<Left arrow> */
#define K_CTRL_RIGHT            0x74   /* <Ctrl>+<Right arrow> */
#define K_CTRL_END              0x75   /* <Ctrl>+<End> */
#define K_CTRL_PAGEDOWN         0x76   /* <Ctrl>+<Page down> */
#define K_CTRL_HOME             0x77   /* <Ctrl>+<Home> */
#define K_ALT_1                 0x78   /* <Alt>+<1> */
#define K_ALT_2                 0x79   /* <Alt>+<2> */
#define K_ALT_3                 0x7a   /* <Alt>+<3> */
#define K_ALT_4                 0x7b   /* <Alt>+<4> */
#define K_ALT_5                 0x7c   /* <Alt>+<5> */
#define K_ALT_6                 0x7d   /* <Alt>+<6> */
#define K_ALT_7                 0x7e   /* <Alt>+<7> */
#define K_ALT_8                 0x7f   /* <Alt>+<8> */
#define K_ALT_9                 0x80   /* <Alt>+<9> */
#define K_ALT_0                 0x81   /* <Alt>+<0> */
#define K_ALT_MINUS             0x82   /* <Alt>+<-> */
#define K_ALT_EQUAL             0x83   /* <Alt>+<=> */
#define K_CTRL_PAGEUP           0x84   /* <Ctrl>+<Page up> */
#define K_F11                   0x85   /* <F11> */
#define K_F12                   0x86   /* <F12> */
#define K_SHIFT_F11             0x87   /* <Shift>+<F11> */
#define K_SHIFT_F12             0x88   /* <Shift>+<F12> */
#define K_CTRL_F11              0x89   /* <Ctrl>+<F11> */
#define K_CTRL_F12              0x8a   /* <Ctrl>+<F12> */
#define K_ALT_F11               0x8b   /* <Alt>+<F11> */
#define K_ALT_F12               0x8c   /* <Alt>+<F12> */
#define K_CTRL_UP               0x8d   /* <Ctrl>+<Up arrow> */
#define K_CTRL_PAD_MINUS        0x8e   /* <Ctrl>+<-> (numeric keypad) */
#define K_CTRL_CENTER           0x8f   /* <Ctrl>+<Center> */
#define K_CTRL_PAD_PLUS         0x90   /* <Ctrl>+<+> (numeric keypad) */
#define K_CTRL_DOWN             0x91   /* <Ctrl>+<Down arrow> */
#define K_CTRL_INS              0x92   /* <Ctrl>+<Ins> */
#define K_CTRL_DEL              0x93   /* <Ctrl>+<Del> */
#define K_CTRL_TAB              0x94   /* <Ctrl>+<Tab> */
#define K_CTRL_PAD_SLASH        0x95   /* <Ctrl>+</> (numeric keypad) */
#define K_CTRL_PAD_ASTERISK     0x96   /* <Ctrl>+<*> (numeric keypad) */
#define K_ALT_HOME              0x97   /* <Alt>+<Home> */
#define K_ALT_UP                0x98   /* <Alt>+<Up arrow> */
#define K_ALT_PAGEUP            0x99   /* <Alt>+<Page up> */
#define K_ALT_LEFT              0x9b   /* <Alt>+<Left arrow> */
#define K_ALT_RIGHT             0x9d   /* <Alt>+<Right arrow> */
#define K_ALT_END               0x9f   /* <Alt>+<End> */
#define K_ALT_DOWN              0xa0   /* <Alt>+<Down arrow> */
#define K_ALT_PAGEDOWN          0xa1   /* <Alt>+<Page down> */
#define K_ALT_INS               0xa2   /* <Alt>+<Ins> */
#define K_ALT_DEL               0xa3   /* <Alt>+<Del> */
#define K_ALT_PAD_SLASH         0xa4   /* <Alt>+</> (numeric keypad) */
#define K_ALT_TAB               0xa5   /* <Alt>+<Tab>  [DOS] */
#define K_ALT_PAD_ENTER         0xa6   /* <Alt>+<Enter> (numeric keypad) */

#endif /* not SYS_KBDSCAN_H */
