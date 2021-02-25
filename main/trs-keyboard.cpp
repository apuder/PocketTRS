
#include "trs.h"
#include "trs-keyboard.h"

#define ADD_SHIFT_KEY 0x100
#define REMOVE_SHIFT_KEY 0x200

typedef struct {
  uint8_t offset;
  uint16_t mask;
} TRSKey;


/*
Address   1     2     4     8     16     32     64     128    Hex Address
------- ----- ---,- ----- ----- -----  -----  -----   -----   -----------
14337     @     A     B     C      D      E      F      G        3801       1
14338     H     I     J     K      L      M      N      O        3802       2
14340     P     Q     R     S      T      U      V      W        3804       4
14344     X     Y     Z     ,      -      -      -      -        3808       8
14352     0     1!    2"    3#     4$     5%     6&     7'       3810      16
14368     8(    9)   *:    +;     <,     =-     >.     ?/        3820      32
14400  enter  clear break  up    down   left  right  space       3840      64
14464  shift    -     -     -  control    -      -      -        3880     128
*/

static const TRSKey trsKeys[] = {
  {0, 0}, // VK_NONE
  {64, 128}, // VK_SPACE
  {16, 1}, //  VK_0
  {16, 2}, //  VK_1
  {16, 4}, //  VK_2
  {16, 8}, //  VK_3
  {16, 16}, //  VK_4
  {16, 32}, //  VK_5
  {16, 64}, //  VK_6
  {16, 128}, //  VK_7
  {32, 1}, //  VK_8
  {32, 2}, //  VK_9
  {16, 1}, //  VK_KP_0
  {16, 2}, //  VK_KP_1
  {16, 4}, //  VK_KP_2
  {16, 8}, //  VK_KP_3
  {16, 16}, //  VK_KP_4
  {16, 32}, //  VK_KP_5
  {16, 64}, //  VK_KP_6
  {16, 128}, //  VK_KP_7
  {32, 1}, //  VK_KP_8
  {32, 2}, //  VK_KP_9
  {1, 2}, //  VK_a
  {1, 4}, //  VK_b
  {1, 8}, //  VK_c
  {1, 16}, //  VK_d
  {1, 32}, //  VK_e
  {1, 64}, //  VK_f
  {1, 128}, //  VK_g
  {2, 1}, //  VK_h
  {2, 2}, //  VK_i
  {2, 4}, //  VK_j
  {2, 8}, //  VK_k
  {2, 16}, //  VK_l
  {2, 32}, //  VK_m
  {2, 64}, //  VK_n
  {2, 128}, //  VK_o
  {4, 1}, //  VK_p
  {4, 2}, //  VK_q
  {4, 4}, //  VK_r
  {4, 8}, //  VK_s
  {4, 16}, //  VK_t
  {4, 32}, //  VK_u
  {4, 64}, //  VK_v
  {4, 128}, //  VK_w
  {8, 1}, //  VK_x
  {8, 2}, //  VK_y
  {8, 4}, //  VK_z
  {1, 2}, //  VK_A
  {1, 4}, //  VK_B
  {1, 8}, //  VK_C
  {1, 16}, //  VK_D
  {1, 32}, //  VK_E
  {1, 64}, //  VK_F
  {1, 128}, //  VK_G
  {2, 1}, //  VK_H
  {2, 2}, //  VK_I
  {2, 4}, //  VK_J
  {2, 8}, //  VK_K
  {2, 16}, //  VK_L
  {2, 32}, //  VK_M
  {2, 64}, //  VK_N
  {2, 128}, //  VK_O
  {4, 1}, //  VK_P
  {4, 2}, //  VK_Q
  {4, 4}, //  VK_R
  {4, 8}, //  VK_S
  {4, 16}, //  VK_T
  {4, 32}, //  VK_U
  {4, 64}, //  VK_V
  {4, 128}, //  VK_W
  {8, 1}, //  VK_X
  {8, 2}, //  VK_Y
  {8, 4}, //  VK_Z
  {0, 0}, //  VK_GRAVEACCENT
  {0, 0}, //  VK_ACUTEACCENT
  {16, ADD_SHIFT_KEY | 128}, //  VK_QUOTE
  {16, 4}, //  VK_QUOTEDBL
  {32, ADD_SHIFT_KEY | 32}, //  VK_EQUALS
  {32, 32}, //  VK_MINUS
  {32, 32}, //  VK_KP_MINUS
  {32, 8}, //  VK_PLUS
  {32, 8}, //  VK_KP_PLUS
  {32, 4}, //  VK_KP_MULTIPLY
  {32, 4}, //  VK_ASTERISK
  {0, 0}, //  VK_BACKSLASH
  {32, 128}, //  VK_KP_DIVIDE
  {32, 128}, //  VK_SLASH
  {32, 64}, //  VK_KP_PERIOD
  {32, 64}, //  VK_PERIOD
  {32, REMOVE_SHIFT_KEY | 4}, //  VK_COLON
  {32, 16}, //  VK_COMMA
  {32, 8}, //  VK_SEMICOLON
  {16, 64}, //  VK_AMPERSAND
  {0, 0}, //  VK_VERTICALBAR
  {16, 8}, //  VK_HASH
  {1, REMOVE_SHIFT_KEY | 1}, //  VK_AT
  {0, 0}, //  VK_CARET
  {16, 16}, //  VK_DOLLAR
  {16, 8}, //  VK_POUND
  {0, 0}, //  VK_EURO
  {16, 32}, //  VK_PERCENT
  {16, 2}, //  VK_EXCLAIM
  {32, 128}, //  VK_QUESTION
  {0, 0}, //  VK_LEFTBRACE
  {0, 0}, //  VK_RIGHTBRACE
  {0, 0}, //  VK_LEFTBRACKET
  {0, 0}, //  VK_RIGHTBRACKET
  {32, 1}, //  VK_LEFTPAREN
  {32, 2}, //  VK_RIGHTPAREN
  {32, 16}, //  VK_LESS
  {32, 64}, //  VK_GREATER
  {0, 0}, //  VK_UNDERSCORE
  {0, 0}, //  VK_DEGREE
  {0, 0}, //  VK_SECTION
  {0, 0}, //  VK_TILDE
  {0, 0}, //  VK_NEGATION
  {128, 1}, //  VK_LSHIFT
  {128, 1}, //  VK_RSHIFT
  {0, 0}, //  VK_LALT
  {0, 0}, //  VK_RALT
  {128, 16}, //  VK_LCTRL
  {128, 16}, //  VK_RCTRL
  {0, 0}, //  VK_LGUI
  {0, 0}, //  VK_RGUI
  {64, 4}, //  VK_ESCAPE
  {0, 0}, //  VK_PRINTSCREEN
  {0, 0}, //  VK_SYSREQ
  {0, 0}, //  VK_INSERT
  {0, 0}, //  VK_KP_INSERT
  {0, 0}, //  VK_DELETE
  {0, 0}, //  VK_KP_DELETE
  {64, 32}, //  VK_BACKSPACE
  {0, 0}, //  VK_HOME
  {0, 0}, //  VK_KP_HOME
  {0, 0}, //  VK_END
  {0, 0}, //  VK_KP_END
  {0, 0}, //  VK_PAUSE
  {64, 4}, //  VK_BREAK
  {0, 0}, //  VK_SCROLLLOCK
  {0, 0}, //  VK_NUMLOCK
  {0, 0}, //  VK_CAPSLOCK
  {0, 0}, //  VK_TAB
  {64, 1}, //  VK_RETURN
  {64, 1}, //  VK_KP_ENTER
  {0, 0}, //  VK_APPLICATION
  {0, 0}, //  VK_PAGEUP
  {0, 0}, //  VK_KP_PAGEUP
  {0, 0}, //  VK_PAGEDOWN
  {0, 0}, //  VK_KP_PAGEDOWN
  {64, 8}, //  VK_UP
  {64, 8}, //  VK_KP_UP
  {64, 16}, //  VK_DOWN
  {64, 16}, //  VK_KP_DOWN
  {64, 32}, //  VK_LEFT
  {64, 32}, //  VK_KP_LEFT
  {64, 64}, //  VK_RIGHT
  {64, 64}, //  VK_KP_RIGHT
  {0, 0}, //  VK_KP_CENTER
  {16, ADD_SHIFT_KEY | 1}, //  VK_F1
  {64, 2}, //  VK_F2
  {0, 0}, //  VK_F3
  {0, 0}, //  VK_F4
  {0, 0}, //  VK_F5
  {0, 0}, //  VK_F6
  {0, 0}, //  VK_F7
  {0, 0}, //  VK_F8
  {0, 0}, //  VK_F9
  {0, 0}, //  VK_F10
  {0, 0}, //  VK_F11
  {0, 0}, //  VK_F12
  {0, 0}, //  VK_GRAVE_a
  {0, 0}, //  VK_GRAVE_e
  {0, 0}, //  VK_ACUTE_e
  {0, 0}, //  VK_GRAVE_i
  {0, 0}, //  VK_GRAVE_o
  {0, 0}, //  VK_GRAVE_u
  {0, 0}, //  VK_CEDILLA_c
  {0, 0}, //  VK_ESZETT
  {0, 0}, //  VK_UMLAUT_u
  {0, 0}, //  VK_UMLAUT_o
  {0, 0}, //  VK_UMLAUT_a
  {0, 0}, //  VK_CEDILLA_C
  {0, 0}, //  VK_TILDE_n
  {0, 0}, //  VK_TILDE_N
  {0, 0}, //  VK_UPPER_a
  {0, 0}, //  VK_ACUTE_a
  {0, 0}, //  VK_ACUTE_i
  {0, 0}, //  VK_ACUTE_o
  {0, 0}, //  VK_ACUTE_u
  {0, 0}, //  VK_UMLAUT_i
  {0, 0}, //  VK_EXCLAIM_INV
  {0, 0}, //  VK_QUESTION_INV
  {0, 0}, //  VK_ACUTE_A
  {0, 0}, //  VK_ACUTE_E
  {0, 0}, //  VK_ACUTE_I
  {0, 0}, //  VK_ACUTE_O
  {0, 0}, //  VK_ACUTE_U
  {0, 0}, //  VK_GRAVE_A
  {0, 0}, //  VK_GRAVE_E
  {0, 0}, //  VK_GRAVE_I
  {0, 0}, //  VK_GRAVE_O
  {0, 0}, //  VK_GRAVE_U
  {0, 0}, //  VK_INTERPUNCT
  {0, 0}, //  VK_DIAERESIS
  {0, 0}, //  VK_UMLAUT_e
  {0, 0}, //  VK_UMLAUT_A
  {0, 0}, //  VK_UMLAUT_E
  {0, 0}, //  VK_UMLAUT_I
  {0, 0}, //  VK_UMLAUT_O
  {0, 0}, //  VK_UMLAUT_U
  {0, 0}, //  VK_CARET_a
  {0, 0}, //  VK_CARET_e
  {0, 0}, //  VK_CARET_i
  {0, 0}, //  VK_CARET_o
  {0, 0}, //  VK_CARET_u
  {0, 0}, //  VK_CARET_A
  {0, 0}, //  VK_CARET_E
  {0, 0}, //  VK_CARET_I
  {0, 0}, //  VK_CARET_O
  {0, 0}, //  VK_CARET_U
  {0, 0}, //  VK_ASCII
  {0, 0}  //  VK_LAST
};

void process_key(int vk, bool down)
{
  int offset = trsKeys[vk].offset;
  if (offset != 0) {
    bool addShiftKey = trsKeys[vk].mask & ADD_SHIFT_KEY;
    bool removeShiftKey = trsKeys[vk].mask & REMOVE_SHIFT_KEY;
    uint8_t mask = trsKeys[vk].mask & 0xff;
    uint16_t address = 0x3800 + offset;
    if (down) {
      poke_mem(address, peek_mem(address) | mask);
      if (addShiftKey) {
        poke_mem(0x3880, 1);
      }
      if (removeShiftKey) {
        poke_mem(0x3880, 0);
      }
    } else {
      poke_mem(address, peek_mem(address) & ~mask);
      if (addShiftKey) {
        poke_mem(0x3880, 0);
      }
    }
  }
}
