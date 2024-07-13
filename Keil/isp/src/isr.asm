
LDR_SIZE    EQU     1000H

MAPISR      MACRO   ADDR
            CSEG    AT  ADDR
            LJMP    LDR_SIZE + $
            ENDM

            MAPISR  0003H
            MAPISR  000BH
            MAPISR  0013H
            MAPISR  001BH
            MAPISR  0023H
            MAPISR  002BH
            MAPISR  0033H
            MAPISR  003BH
            MAPISR  0043H
            MAPISR  004BH
            MAPISR  0053H
            MAPISR  005BH
            MAPISR  0063H
            MAPISR  006BH
            MAPISR  0073H
            MAPISR  007BH
            MAPISR  0083H
            MAPISR  008BH
            MAPISR  0093H
            MAPISR  009BH
            MAPISR  00A3H
            MAPISR  00ABH
            MAPISR  00B3H
            MAPISR  00BBH
            MAPISR  00C3H
            MAPISR  00CBH
            MAPISR  00D3H
            MAPISR  00DBH
            MAPISR  00E3H
            MAPISR  00EBH
            MAPISR  00F3H
            MAPISR  00FBH
            MAPISR  0103H
            MAPISR  010BH
            MAPISR  0113H
            MAPISR  011BH
            MAPISR  0123H
            MAPISR  012BH
            MAPISR  0133H
            MAPISR  013BH
            MAPISR  0143H
            MAPISR  014BH
            MAPISR  0153H
            MAPISR  015BH
            MAPISR  0163H
            MAPISR  016BH
            MAPISR  0173H
            MAPISR  017BH
            MAPISR  0183H
            MAPISR  018BH
            MAPISR  0193H
            MAPISR  019BH
            MAPISR  01A3H
            MAPISR  01ABH
            MAPISR  01B3H
            MAPISR  01BBH
            MAPISR  01C3H
            MAPISR  01CBH
            MAPISR  01D3H
            MAPISR  01DBH
            MAPISR  01E3H
            MAPISR  01EBH
            MAPISR  01F3H
            MAPISR  01FBH

            END
