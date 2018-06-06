; endlink386.asm (emx+gcc) -- Copyright (c) 1995 by Eberhard Mattes

                .386

                PUBLIC  _end
                PUBLIC  _edata

DATA32          SEGMENT PUBLIC PARA USE32 'DATA'
DATA32          ENDS

________DATA    SEGMENT PUBLIC PARA USE32 'DATA'
_edata          LABEL BYTE
________DATA    ENDS


c_common        SEGMENT PUBLIC PARA USE32 'BSS'
c_common        ENDS

________BSS     SEGMENT PUBLIC PARA USE32 'BSS'
_end            LABEL BYTE
________BSS     ENDS

DGROUP          GROUP   DATA32, ________DATA, c_common, ________BSS

                END
