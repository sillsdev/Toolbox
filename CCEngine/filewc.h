/**************************************************************************/
/* filewc.h  Doug Case 3/96  This has the Windows-specify variables that are
 *                           in filewc.c.  Filewc.c is used in the old DOS
 *                           command line CC using just variables in cc.h.
 *                           The windows version of CC also uses filewc.c
 *                           for its wildcard processing, and this .h file
 *                           has the windows-specific variable definitions.
 *                           These variables are (and need to be) globals.
 *
 *
 * PROPRIETARY RIGHTS NOTICE:  All rights reserved.  This material contains
 *		the valuable properties of Jungle Aviation and Radio Service, Inc.
 *		of Waxhaw, North Carolina, United States of America (JAARS)
 *		embodying substantial creative efforts and confidential information,
 *		ideas and expressions, no part of which may be reproduced or transmitted
 *		in any form or by any means, electronic, mechanical, or otherwise,
 *		including photocopying and recording or in connection with any
 *		information storage or retrieval system without the permission in
 *		writing from JAARS.
 *
 * COPYRIGHT NOTICE:  Copyright <C> 1980 - 1996
 *		an unpublished work by the Summer Institute of Linguistics, Inc.
 *
 *
 *
 *
 * Change History:
 *
 *  15-Mar-96      DRC Original version
 *
 *
 *
/**************************************************************************/

#ifndef GLOBALF
#define GLOBALF
#endif


/* keep these synced up with cc.h unless there is good reason to diverge  */
#define NUMBER_FILES 600
#define PATH_LEN 250             /* allows for a path and file name       */
#if defined(WIN32)
#define PRIMARY_LEN 250
#define EXT_LEN 3
#else
#define PRIMARY_LEN 8
#define EXT_LEN 3
#endif
#define ARRAY_SIZE (PRIMARY_LEN+EXT_LEN+2)
#define ARRAY_SIZE_2 (ARRAY_SIZE-1)


/* filewc.c formerly static variables                                     */
GLOBALF char (*fn_arr_pnt)[ARRAY_SIZE];      /* array of input file names */
GLOBALF char (*pad_arr_pnt)[ARRAY_SIZE_2];   /* used to sort the above    */
GLOBALF char inpath[PATH_LEN];   /* path specification from input file    */
GLOBALF char *inpath_end;        /* pointer to end of input file path     */
GLOBALF char outpath[PATH_LEN];  /* path specification from output file   */
GLOBALF char *outpath_end;       /* pointer to end of output file path    */
GLOBALF int files_read;          /* number of input files to be processed */
GLOBALF int *index;              /* used as a general purpose index       */
GLOBALF int ind_incr;            /* index to next input file              */


/* END */

