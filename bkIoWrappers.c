/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

/******************************************************************************
* Author:
* Andrew Smith, http://littlesvr.ca/misc/contactandrew.php
*
* Contributors:
* 
******************************************************************************/

#include <stdio.h>

#include "bkInternal.h"

bk_off_t bkReadSeekSet(VolInfo* volInfo, bk_off_t offset, int origin)
{
#ifdef MINGW_TEST
    
#else
    return lseek(volInfo->imageForReading, offset, origin);
#endif
}

bk_off_t bkReadSeekTell(VolInfo* volInfo)
{
#ifdef MINGW_TEST
    
#else
    return lseek(volInfo->imageForReading, 0, SEEK_CUR);
#endif
}
