8050 // PC keyboard interface constants
8051 
8052 #define KBSTATP         0x64    // kbd controller status port(I)
8053 #define KBS_DIB         0x01    // kbd data in buffer
8054 #define KBDATAP         0x60    // kbd data port(I)
8055 
8056 #define NO              0
8057 
8058 #define SHIFT           (1<<0)
8059 #define CTL             (1<<1)
8060 #define ALT             (1<<2)
8061 
8062 #define CAPSLOCK        (1<<3)
8063 #define NUMLOCK         (1<<4)
8064 #define SCROLLLOCK      (1<<5)
8065 
8066 #define E0ESC           (1<<6)
8067 
8068 // Special keycodes
8069 #define KEY_HOME        0xE0
8070 #define KEY_END         0xE1
8071 #define KEY_UP          0xE2
8072 #define KEY_DN          0xE3
8073 #define KEY_LF          0xE4
8074 #define KEY_RT          0xE5
8075 #define KEY_PGUP        0xE6
8076 #define KEY_PGDN        0xE7
8077 #define KEY_INS         0xE8
8078 #define KEY_DEL         0xE9
8079 
8080 // C('A') == Control-A
8081 #define C(x) (x - '@')
8082 
8083 static uchar shiftcode[256] =
8084 {
8085   [0x1D] CTL,
8086   [0x2A] SHIFT,
8087   [0x36] SHIFT,
8088   [0x38] ALT,
8089   [0x9D] CTL,
8090   [0xB8] ALT
8091 };
8092 
8093 static uchar togglecode[256] =
8094 {
8095   [0x3A] CAPSLOCK,
8096   [0x45] NUMLOCK,
8097   [0x46] SCROLLLOCK
8098 };
8099 
8100 static uchar normalmap[256] =
8101 {
8102   NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
8103   '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
8104   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
8105   'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
8106   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
8107   '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
8108   'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
8109   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
8110   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
8111   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
8112   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
8113   [0x9C] '\n',      // KP_Enter
8114   [0xB5] '/',       // KP_Div
8115   [0xC8] KEY_UP,    [0xD0] KEY_DN,
8116   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
8117   [0xCB] KEY_LF,    [0xCD] KEY_RT,
8118   [0x97] KEY_HOME,  [0xCF] KEY_END,
8119   [0xD2] KEY_INS,   [0xD3] KEY_DEL
8120 };
8121 
8122 static uchar shiftmap[256] =
8123 {
8124   NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
8125   '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
8126   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
8127   'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
8128   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
8129   '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
8130   'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
8131   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
8132   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
8133   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
8134   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
8135   [0x9C] '\n',      // KP_Enter
8136   [0xB5] '/',       // KP_Div
8137   [0xC8] KEY_UP,    [0xD0] KEY_DN,
8138   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
8139   [0xCB] KEY_LF,    [0xCD] KEY_RT,
8140   [0x97] KEY_HOME,  [0xCF] KEY_END,
8141   [0xD2] KEY_INS,   [0xD3] KEY_DEL
8142 };
8143 
8144 
8145 
8146 
8147 
8148 
8149 
8150 static uchar ctlmap[256] =
8151 {
8152   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
8153   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
8154   C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
8155   C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
8156   C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
8157   NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
8158   C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
8159   [0x9C] '\r',      // KP_Enter
8160   [0xB5] C('/'),    // KP_Div
8161   [0xC8] KEY_UP,    [0xD0] KEY_DN,
8162   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
8163   [0xCB] KEY_LF,    [0xCD] KEY_RT,
8164   [0x97] KEY_HOME,  [0xCF] KEY_END,
8165   [0xD2] KEY_INS,   [0xD3] KEY_DEL
8166 };
8167 
8168 
8169 
8170 
8171 
8172 
8173 
8174 
8175 
8176 
8177 
8178 
8179 
8180 
8181 
8182 
8183 
8184 
8185 
8186 
8187 
8188 
8189 
8190 
8191 
8192 
8193 
8194 
8195 
8196 
8197 
8198 
8199 
