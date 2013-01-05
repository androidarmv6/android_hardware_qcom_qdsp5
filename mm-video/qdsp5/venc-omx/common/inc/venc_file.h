#ifndef _VENC_FILE_H
#define _VENC_FILE_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/inc/venc_file.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Initial revision

 ========================================================================== */
#ifdef __cplusplus
extern "C" {
#endif
/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/

/**
 * @brief Opens the file in read or write mode
 *
 * @param handle The new file handle
 * @param filename The name of the file
 * @param readonly Set to 1 for read access, 0 for write access.
 */
int venc_file_open(void** handle, 
                   char* filename, 
                   int readonly);

/**
 * @brief Closes the file
 */
int venc_file_close(void* handle);

/**
 * @brief Reads the file. Only valid in read mode.
 *
 * @param handle The file handle
 * @param buffer The buffer to read into
 * @param bytes The number of bytes to read
 * @param bytes_read The number of bytes actually read (output)
 */
int venc_file_read(void* handle, 
                   void* buffer, 
                   int bytes);

/**
 * @brief Writes to the file. Only valid in write mode.
 *
 * @param handle The file handle
 * @param buffer The buffer to write from
 * @param bytes The number of bytes to write
 * @param bytes_write The number of bytes actually written (output)
 */
int venc_file_write(void* handle, 
                    void* buffer,
                    int bytes);

/**
 * @brief Reposition the file pointer.
 *
 * @param handle The file handle
 * @param bytes The number of bytes from the start of file
 */
int venc_file_seek_start(void* handle, 
                         int bytes);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VENC_FILE_H
