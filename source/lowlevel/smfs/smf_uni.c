// Removed unicode code
/****
* This MACRO must return 2byte-length of converted unicode  as an 'int' type
* Parameters
*       - p_uni : unicode string pointer (unsigned short*)
*       - p_mb : multibyte string pointer (unsignd char*)
*       - mb_len : multibyte string length in bytes.
*                               If 0, p_mb is considered to be a NULL terminated string (int)
* Return Value
*       - 2byte-length of converted unicode. It includes NULL character at the end.
*               (eg. If it returns 3, it successfully converted 3 unicode character(6 bytes).
*                       The last 2 bytes are all 0)
* Notes
*       - 'p_uni' must be allocated for the (MAX_FILE_NAME_LEN + 2) bytes.
*       - This function must be able to stop the conversion at the maximum length,
*               and add NULL unicode character at the end.
*****/

#include "smf_fat.h"

int sm_ConvertStrToUnicode(unsigned short *des, unsigned char *src, int len) {
        int x;
        for (x=0;x<len;x++) {
           *des = *src ;
            des++;src++;
         }

        return (len*2);
}

