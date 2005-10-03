/**********
 *
 * smf_fat.c: SmartMedia File System FAT part
 *
 * Portable File System designed for SmartMedia
 *
 * Created by Kim Hyung Gi (06/99) - kimhg@mmrnd.sec.samsung.co.kr
 * Samsung Electronics, M/M R&D Center, Visual Communication Group
 *
 **********
 *
 * Modified by Lee Jin Woo (1999/08/17)
 *      - Added smfsUserInit() call at the end of smInit() (1999/08/17)
 *
 **********/
#include "smf_conf.h"
#include "smf_cmn.h"
#include "smf_fat.h"
#include "smf_lpt.h"
#include "smf_io.h"
#include "smf_buf.h"


char* acSMFSVersion = "2.22";           /* must be x.xx type */

/**********
 * Macro Definitions
 **********/
#define FAT_RETURN(x, y) \
                do { sm_PostFAT((x));  return (y); } while (0)

#define SET_FAT_TBL(drv_no, offset, value) \
                do { \
                        if ((udword)(offset) < s_fatTableIndexCount[(drv_no)]) { \
                                s_fat[(drv_no)][(udword)(offset)] = (value); \
                        } \
                } while (0)

#define GET_FAT_TBL(drv_no, offset) \
                (((udword)(offset) < s_fatTableIndexCount[(drv_no)]) \
                        ? s_fat[(drv_no)][(udword)(offset)] : 0)


/**********
 * Static Variables
 **********/

static sSM_INFO s_smInfo[MAX_DRIVE];

static uword* s_fat[MAX_DRIVE] = { NULL, };

static udword s_fat1StartSector[MAX_DRIVE];
static udword s_fat1Sectors[MAX_DRIVE];
static udword s_rootStartSector[MAX_DRIVE];
static udword s_rootSectors[MAX_DRIVE];
static udword s_clusterStartSector[MAX_DRIVE];
static udword s_fatTableIndexCount[MAX_DRIVE];

static sOPENED_LIST* s_openedFile[MAX_DRIVE] = { NULL, };
static udword s_fatFlag[MAX_DRIVE];
static ERR_CODE s_smErr;
//static udword s_cacheSize[MAX_DRIVE];
static ubyte s_drvName[MAX_DRIVE][MAX_DRIVE_NAME_LEN + 1];

static void (*s_pfCBInserted)(udword);
static void (*s_pfCBEjected)(udword);

static udword bNoFATUpdate[MAX_DRIVE] = {FALSE, };
static udword bInnerNoFATUpdate[MAX_DRIVE] = {FALSE, };
/**********/

/* static funtions */
static void     sm_FATInitDefaultValue(void);
static ERR_CODE sm_FATInit(udword drv_no);
static ERR_CODE sm_PreFAT(udword drv_no);
static ERR_CODE sm_PostFAT(udword drv_no);
static ERR_CODE sm_WriteSMInfo(udword drv_no, const sSM_INFO* p_sm_info);
static ERR_CODE sm_UpdateBlock(udword drv_no, udword new_block, udword old_block, udword offset, udword count, const ubyte* p_buf, bool touchAllSectors);
static udword   sm_GetDrvNo(const ubyte* p_name);
static ERR_CODE sm_CheckPath(udword drv_no, const ubyte* p_file_name, udword check_mode, F_HANDLE* p_handle);
static ERR_CODE sm_FATUpdate(udword drv_no);
static ERR_CODE sm_RemoveFile(udword drv_no, F_HANDLE h_dir, const ubyte* p_name);
static ERR_CODE sm_RemoveFirstBottommostDir(udword drv_no, F_HANDLE h_dir);
static ERR_CODE sm_FindEntryInDirectory(udword drv_no, udword find_mode, F_HANDLE h_parent, udword var_par, ubyte* p_info, udword* p_cluster, udword* p_offset);
static ERR_CODE sm_AddToDirEntry(udword drv_no, F_HANDLE h_dir, const ubyte* long_name, const uword* p_unicode_name, sFILE_STAT* p_stat, udword b_insert, udword cluster, udword offset);
static ERR_CODE sm_ExpandClusterSpace(udword drv_no, F_HANDLE handle, udword cluster, udword offset, udword size);
static ERR_CODE sm_DeleteFromDirEntry(udword drv_no, F_HANDLE h_dir, const ubyte* long_name);
static ERR_CODE sm_AddToOpenedFileList(udword drv_no, sOPENED_LIST* p_list);
static ERR_CODE sm_DeleteFromOpenedFileList(udword drv_no, udword cluster);
//static ERR_CODE sm_WriteCacheToDisk(udword drv_no, sOPENED_LIST* p_list);
static ERR_CODE sm_WriteToDisk(udword drv_no, sOPENED_LIST* p_list,     const ubyte* p_buf, udword count);
static ERR_CODE sm_ReadFromDisk(udword drv_no, sOPENED_LIST* p_list, void* p_buf, udword count, udword* p_read_count);
static bool     sm_ConvertToFATName(const ubyte* p_name, ubyte* p_short_name);
static ERR_CODE sm_GetOpenedList(udword drv_no, udword cluster, udword id, sOPENED_LIST** pp_list);
static bool     sm_ExtractLastName(const ubyte* p_name, ubyte* p_last_name);
static bool     sm_FindChar(const ubyte* p_str, ubyte ch, ubyte** p_found);
static bool     sm_GetTime(sTIME* p_time);
static ERR_CODE sm_GetLongName(udword drv_no, udword h_parent, udword short_cluster, udword offset, uword* p_lname);

/* added by jwlee */
static ERR_CODE sm_addClusters(udword drv_no, udword cluster, udword addedClusterCount);
static ERR_CODE sm_eraseSectors(udword drv_no, udword startSector, udword count);
static ERR_CODE sm_erasePartialBlock(udword drv_no, udword block, udword startOffset, udword count);
static ERR_CODE sm_eraseWholeBlock(udword drv_no, udword block);
static udword   sm_searchCluster(udword drv_no, udword count, udword* maxCount);
static void     sm_prepareFileClose(udword drv_no, const sOPENED_LIST* p_list);

/* Cluster layer functions */
SM_EXPORT ERR_CODE smcAllocCluster(udword drv_no, udword* p_cluster);
SM_EXPORT ERR_CODE smcReadCluster(udword drv_no, udword cluster_no, udword offset, void* buf, udword size);
SM_EXPORT ERR_CODE smcWriteCluster(udword drv_no, udword cluster_no, udword offset, const void* buf, udword size);
static ERR_CODE smcSetCluster(udword drv_no, udword cluster_no, ubyte val);
static ERR_CODE smcUpdateBlock(udword drv_no, udword block, udword offset, const void* buf, udword count);

/* Utility functions */
static ubyte    sm_toUpper(ubyte c);
static void             sm_UnicodeStrToUpper(uword* p_dest, uword* p_src, udword len);


/**********
 * File API definitions
 **********/

/**********
 * Function: smInit
 * Remarks:
 *      - This API must be called when system starts up
 **********/
SM_EXPORT ERR_CODE smInit(void)
{
        udword drv_no;
        ubyte name[10];
        sTIME now = { 0,0,0,0,0,0};

        sm_GetTime(&now);
        SM_SRAND(now.sec);

        smbBufPoolInit();

        sm_IOInitDefaultValue();
        sm_LPTInitDefaultValue();
        sm_FATInitDefaultValue();

        /* drive name initialize */
        SM_STRCPY((char*)name, "dev");
        for (drv_no = 0; drv_no < MAX_DRIVE; ++drv_no)
        {
                if (drv_no < 10)
                {
                        name[3] = (ubyte)drv_no + '0';
                        name[4] = 0;
                }
#if (MAX_DRIVE >= 10)
                else if (drv_no < 100)
                {
                        name[3] = (ubyte)(drv_no / 10) + '0';
                        name[4] = (ubyte)(drv_no % 10) + '0';
                        name[5] = 0;
                }
                else
                {
                        name[3] = (ubyte)(drv_no / 100) + '0';
                        name[4] = (ubyte)((drv_no % 100) / 10) + '0';
                        name[5] = (ubyte)(drv_no % 10) + '0';
                        name[6] = 0;
                }
#endif

                SM_STRCPY((char*)s_drvName[drv_no], (char*)name);
        }

#ifdef SM_MULTITASK
        SM_LOCK_INIT(drv_no);
#endif

        /* perform user initialization */
        if (smfsUserInit() != 0)
        {
                return ERR_INTERNAL;
        }

        return SM_OK;
}


/**********
 * Function: smCreateFile
 * Remarks:
 *      - creates file and returns the handle of created file
 * Parameters
 *      . p_file_name: file name that should be created
 *                                      (ex. dev0:\temp\readme.txt)
 *      . fcreate_mode: NOT_IF_EXIST, ALWAYS_CREATE
 *      . p_handle (result): handle of the created file
 **********/
SM_EXPORT ERR_CODE smCreateFile(const smchar* p_filename, udword fcreate_mode, F_HANDLE* p_handle)
{
        udword drv_no;
        F_HANDLE h_dir;
        udword cluster;
        sOPENED_LIST* p_list;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];
        ubyte dummy_info[32];
        udword offset;
        ubyte *p_file_name = (ubyte *)p_filename;
        sFILE_STAT sStat;
#ifdef  LONG_FILE_NAME_ENABLE
        uword* p_unicode;
        ubyte fat_name[13];
#endif

        drv_no = sm_GetDrvNo(p_file_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_file_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(p_file_name, long_name);

        /* check if the same name exists already */
        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, dummy_info, &cluster, &offset);
        if (s_smErr != ERR_NOT_FOUND)
        {
                if (s_smErr == SM_OK)
                {
                        if (fcreate_mode == ALWAYS_CREATE)
                                sm_RemoveFile(drv_no, h_dir, long_name);
                        else
                                FAT_RETURN(drv_no, ERR_FILE_EXIST);
                }
                else
                        FAT_RETURN(drv_no, s_smErr);
        }

        s_smErr = smcAllocCluster(drv_no, &cluster);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* FAT table update */
        /* This must be done before another smcAllocCluster() is called */
        SET_FAT_TBL(drv_no, cluster, LAST_CLUSTER);

        /* pOpenedList update */
        p_list = (sOPENED_LIST*)SM_MALLOC(sizeof(sOPENED_LIST));
        if (p_list == NULL)
        {
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);
        }

        p_list->prev = NULL;
        p_list->next = NULL;
        p_list->cluster = cluster;
        p_list->h_parent = h_dir;
        p_list->f_ptr = 0;
        p_list->cur_cluster = cluster;
        p_list->old_cur_cluster = 0;
        p_list->size = 0;
        p_list->mode = OPEN_R | OPEN_W;
        p_list->id = 0;
#ifndef WRITE_CACHE_DISABLE
        p_list->cache.f_ptr = 0;
        p_list->cache.count = 0;
        p_list->cache.p_buf = (ubyte*)SM_MALLOC(s_cacheSize[drv_no]);
        if (p_list->cache.p_buf == NULL)
        {
                SM_FREE(p_list);
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);
        }
#endif

        s_smErr = sm_AddToOpenedFileList(drv_no, p_list);
        if (s_smErr != SM_OK)
        {
#ifndef WRITE_CACHE_DISABLE
                SM_FREE(p_list->cache.p_buf);
#endif
                SM_FREE(p_list);
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, s_smErr);
        }

        /* directory contents update */
#ifdef  LONG_FILE_NAME_ENABLE
        p_unicode = (uword*)SM_MALLOC(MAX_FILE_NAME_LEN+2);
        if(p_unicode == NULL)
        {
                sm_DeleteFromOpenedFileList(drv_no, cluster);
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);
        }

        sStat.attr = ATTRIB_FILE;
        sStat.cluster = cluster;
        sStat.size = 0;
        sm_GetTime(&sStat.time);

        if(sm_ConvertToFATName(long_name, fat_name))
        {
                SM_MBS_TO_UNICODE(p_unicode, long_name, 0);
                s_smErr = sm_AddToDirEntry(drv_no, h_dir, long_name, p_unicode, &sStat, 0, 0, 0);
        }
        else
                s_smErr = sm_AddToDirEntry(drv_no, h_dir, long_name, NULL, &sStat, 0, 0, 0);

        SM_FREE(p_unicode);
#else
        sStat.attr = ATTRIB_FILE;
        sStat.cluster = cluster;
        sStat.size = 0;
        sm_GetTime(&sStat.time);

        s_smErr = sm_AddToDirEntry(drv_no, h_dir, long_name, NULL, &sStat, 0, 0, 0);
#endif
        if (s_smErr != SM_OK)
        {
                sm_DeleteFromOpenedFileList(drv_no, cluster);
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, s_smErr);
        }

#ifndef FAT_UPDATE_WHEN_FILE_CLOSE
        /* this can be moved to smCloseFile for speed */
        sm_FATUpdate(drv_no);
#endif
        *p_handle = ((drv_no << 24) & 0xff000000) | ((p_list->id << 16) & 0xff0000) | (cluster & 0xffff);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smOpenFile
 * Remarks:
 *      - open file and returns the handle of opened file
 * Parameters
 *      . p_file_name: file name that should be opened
 *                                      (ex. dev0:\temp\readme.txt)
 *      . fopen_mode: OPEN_R, OPEN_W (can be ORed)
 *      . p_handle (result): handle of the opened file
 **********/
SM_EXPORT ERR_CODE smOpenFile(const smchar* p_filename, udword fopen_mode, F_HANDLE* p_handle)
{
        udword drv_no;
        udword cluster;
        F_HANDLE h_dir;
        sOPENED_LIST* p_list;
        ubyte file_info[32];
        ubyte long_name[MAX_FILE_NAME_LEN + 1];
        udword dummy_offset;
        udword id;
        ubyte *p_file_name = (ubyte *)p_filename;

        drv_no = sm_GetDrvNo(p_file_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_file_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(p_file_name, long_name);

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, file_info, &cluster, &dummy_offset);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        cluster = (udword)file_info[26] | ((udword)file_info[27] << 8);

        for (id = 0; id < MAX_OPENED_FILE_NUM; ++id)
        {
                s_smErr = sm_GetOpenedList(drv_no, cluster, id, &p_list);
                if (s_smErr == ERR_FILE_NOT_OPENED)
                        break;
        }

        if (id >= MAX_OPENED_FILE_NUM)
                FAT_RETURN(drv_no, ERR_FILE_OPENED);

        /* pOpenedList update */
        p_list = (sOPENED_LIST*)SM_MALLOC(sizeof(sOPENED_LIST));
        if (p_list == NULL)
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);

        p_list->prev = NULL;
        p_list->next = NULL;
        p_list->cluster = cluster;
        p_list->h_parent = h_dir;
        p_list->f_ptr = 0;
        p_list->cur_cluster = cluster;
        p_list->old_cur_cluster = 0;
        p_list->size = (udword)file_info[28] | ((udword)file_info[29] << 8)
                | ((udword)file_info[30] << 16) | ((udword)file_info[31] << 24);
        p_list->mode = fopen_mode;
        p_list->id = id;

#ifndef WRITE_CACHE_DISABLE
        p_list->cache.f_ptr = 0;
        p_list->cache.count = 0;
        p_list->cache.p_buf = (ubyte*)SM_MALLOC(s_cacheSize[drv_no]);
        if (p_list->cache.p_buf == NULL)
        {
                SM_FREE(p_list);
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);
        }
#endif

        s_smErr = sm_AddToOpenedFileList(drv_no, p_list);
        if (s_smErr != SM_OK)
        {
#ifndef WRITE_CACHE_DISABLE
                SM_FREE(p_list->cache.p_buf);
#endif
                SM_FREE(p_list);
                FAT_RETURN(drv_no, s_smErr);
        }

        *p_handle = ((drv_no << 24) & 0xff000000) | ((p_list->id << 16) & 0xff0000) | (cluster & 0xffff);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smReadFile
 * Remarks:
 *      - read the contents from file to p_buf
 * Parameters
 *      . h_file: handle of opened file
 *      . p_buf: data buffer pointer that should be read from the disk
 *      . count: data size that shoul be read from the disk
 *      . p_read_count (result): data size that was read from the disk
 * Notes:
 *      - p_buf must be prepared for the bytes lager than "count"
 **********/
SM_EXPORT ERR_CODE smReadFile(F_HANDLE h_file, void* p_buf,     udword count, udword* p_read_count)
{
        udword drv_no;
        udword cluster;
        sOPENED_LIST* p_list = NULL;
        udword head_count, tail_count;
        udword remaining_count;
        udword read_result;
        udword read_sum;
        udword id;
#ifndef WRITE_CACHE_DISABLE
        udword offset;
        udword cache_count;
#endif

        drv_no = (h_file >> 24) & 0x7f;
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        cluster = h_file & 0xffff;
        id = (h_file >> 16) & 0xff;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_GetOpenedList(drv_no, cluster, id, &p_list);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        *p_read_count = 0;

        /*
         * head_count: read size before the cache area
         * cache_count: read size in the cache area
         * tail_count: read size after the cache area
         */
        head_count = tail_count = 0;
#ifndef WRITE_CACHE_DISABLE
        cache_count = 0;
#endif
        remaining_count = count;

#ifndef WRITE_CACHE_DISABLE
        if (p_list->cache.count)
        {
                /* p_list->f_ptr is before the cache area */
                if (p_list->f_ptr < p_list->cache.f_ptr)
                {
                        head_count      = (p_list->cache.f_ptr - p_list->f_ptr > remaining_count)
                                                        ? remaining_count       : (p_list->cache.f_ptr - p_list->f_ptr);
                        remaining_count -= head_count;

                        if (remaining_count > 0)
                        {
                                cache_count = (p_list->cache.count > remaining_count) ? remaining_count : p_list->cache.count;
                                remaining_count -= cache_count;

                                tail_count = remaining_count;
                        }
                }
                /* p_list->f_ptr is in the cache area */
                else if (p_list->f_ptr < p_list->cache.f_ptr + p_list->cache.count)
                {
                        cache_count     = (p_list->cache.count - (p_list->f_ptr - p_list->cache.f_ptr) > remaining_count)
                                                        ?  remaining_count : (p_list->cache.count - (p_list->f_ptr - p_list->cache.f_ptr));
                        remaining_count -= cache_count;

                        tail_count = remaining_count;
                }
                /* p_list->f_ptr is after the cache area */
                else
                {
                        tail_count = remaining_count;
                }
        }
        else
        {
                head_count = remaining_count;
        }
#else
        head_count = remaining_count;
#endif

        read_sum = 0;

        if (head_count)
        {
                s_smErr = sm_ReadFromDisk(drv_no, p_list, p_buf, head_count, &read_result);
                if ((s_smErr != SM_OK) && (s_smErr != ERR_EOF))
                        FAT_RETURN(drv_no, s_smErr);

                read_sum += read_result;
        }

#ifndef WRITE_CACHE_DISABLE
        if (cache_count)
        {
                udword cluster_size;
                udword walked_cluster_count;

                offset = p_list->f_ptr - p_list->cache.f_ptr;
                SM_MEMCPY((char*)p_buf + head_count, p_list->cache.p_buf + offset, cache_count);

                /* update current cluster */
                cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
                walked_cluster_count = (p_list->f_ptr % cluster_size + cache_count) / cluster_size;
                if (walked_cluster_count > 0)
                {
                        while (walked_cluster_count-- > 0)
                        {
                                p_list->old_cur_cluster = p_list->cur_cluster;
                                p_list->cur_cluster = GET_FAT_TBL(drv_no, p_list->cur_cluster);
                                if (p_list->cur_cluster == LAST_CLUSTER)
                                        FAT_RETURN(drv_no, ERR_INTERNAL);
                        }
                }

                p_list->f_ptr += cache_count;

                read_sum += cache_count;
        }
#endif

        if (tail_count)
        {
#ifndef WRITE_CACHE_DISABLE
                s_smErr = sm_ReadFromDisk(drv_no, p_list, (ubyte*)p_buf + head_count + cache_count, tail_count, &read_result);
#else
                s_smErr = sm_ReadFromDisk(drv_no, p_list, (ubyte*)p_buf + head_count, tail_count, &read_result);
#endif
                if ((s_smErr != SM_OK) && (s_smErr != ERR_EOF))
                        FAT_RETURN(drv_no, s_smErr);

                read_sum += read_result;
        }

        *p_read_count = read_sum;

        if (count > read_sum)
                FAT_RETURN(drv_no, ERR_EOF);

        FAT_RETURN(drv_no, s_smErr);
}


/**********
 * Function: smWriteFile
 * Remarks:
 *      - write the contents of the p_buf to the file
 * Parameters
 *      . h_file: handle of opened file
 *      . p_buf: writing contents
 *      . count: amount of writing contents
 **********/
SM_EXPORT ERR_CODE smWriteFile(F_HANDLE h_file, const void* p_buf, udword count)
{
        udword drv_no;
        udword cluster;
        const ubyte* p_write;
        udword write_count;
        sOPENED_LIST* p_list = NULL;
        udword id;

        drv_no = (h_file >> 24) & 0x7f;
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        cluster = h_file & 0xffff;
        id = (h_file >> 16) & 0xff;

        p_write = (const ubyte*)p_buf;
        write_count = count;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_GetOpenedList(drv_no, cluster, id, &p_list);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        if (!(p_list->mode & OPEN_W))
                FAT_RETURN(drv_no, ERR_NOT_PERMITTED);

#ifndef WRITE_CACHE_DISABLE
        if ((p_list->cache.count != 0) && (p_list->f_ptr != p_list->cache.f_ptr + p_list->cache.count))
        {
                sm_WriteCacheToDisk(drv_no, p_list);
        }

        if (p_list->cache.count + write_count < s_cacheSize[drv_no])
        {
                udword cluster_size;
                udword walked_cluster_count;

                SM_MEMCPY(p_list->cache.p_buf + p_list->cache.count, p_write, write_count);

                p_list->cache.count += write_count;

                /* update current cluster */
                cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
                walked_cluster_count = (p_list->f_ptr % cluster_size + write_count) / cluster_size;
                if (walked_cluster_count > 0)
                {
                        while (walked_cluster_count-- > 0)
                        {
                                p_list->old_cur_cluster = p_list->cur_cluster;
                                p_list->cur_cluster = GET_FAT_TBL(drv_no, p_list->cur_cluster);
                        }
                }
                p_list->f_ptr += write_count;
                if (p_list->size < p_list->f_ptr)
                        p_list->size = p_list->f_ptr;
        }
        else
        {
                s_smErr = sm_WriteCacheToDisk(drv_no, p_list);
                if (s_smErr != SM_OK)
                        FAT_RETURN(drv_no, s_smErr);

                s_smErr = sm_WriteToDisk(drv_no, p_list, p_write, write_count);
                if (s_smErr != SM_OK)
                        FAT_RETURN(drv_no, s_smErr);

                p_list->cache.f_ptr = p_list->f_ptr;
        }

        FAT_RETURN(drv_no, SM_OK);
#else
        s_smErr = sm_WriteToDisk(drv_no, p_list, p_write, write_count);
        FAT_RETURN(drv_no, s_smErr);
#endif
}


/**********
 * Function: smSeekFile
 * Remarks:
 *      - moves the file pointer
 * Parameters
 *      . h_file: handle of opened file
 *      . seek_mode: FROM_CURRENT, FROM_BEGIN, FROM_END
 *      . offset: amount of movement
 *      . p_old_offset (result): file pointer before it is moved
 **********/
SM_EXPORT ERR_CODE smSeekFile(F_HANDLE h_file, udword seek_mode, dword offset, dword* p_old_offset)
{
        udword drv_no;
        udword cluster;
        sOPENED_LIST* p_list = NULL;
        udword id;
        udword cluster_size;

        drv_no = (h_file >> 24) & 0x7f;
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;

        cluster = h_file & 0xffff;
        id = (h_file >> 16) & 0xff;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_GetOpenedList(drv_no, cluster, id, &p_list);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        *p_old_offset = p_list->f_ptr;

        if (seek_mode == FROM_CURRENT)
        {
                if (offset >= 0)
                {
                        udword walked_cluster_count;

                        /* update current cluster */
                        walked_cluster_count = (p_list->f_ptr % cluster_size + offset) / cluster_size;
                        if (walked_cluster_count > 0)
                        {
                                while (walked_cluster_count-- > 0)
                                {
                                        p_list->old_cur_cluster = p_list->cur_cluster;
                                        p_list->cur_cluster = GET_FAT_TBL(drv_no, p_list->cur_cluster);
                                }
                        }

                        p_list->f_ptr += offset;
                        if (p_list->f_ptr > p_list->size)
                                p_list->f_ptr = p_list->size;
                }
                else
                {
                        udword i;

                        if (p_list->f_ptr > (udword)(-offset))
                                p_list->f_ptr += offset;
                        else
                                p_list->f_ptr= 0;

                        /* update current cluster */
                        p_list->cur_cluster = p_list->cluster;
                        p_list->old_cur_cluster = 0;
                        for (i = cluster_size; i <= p_list->f_ptr; i += cluster_size)
                        {
                                p_list->old_cur_cluster = p_list->cur_cluster;
                                p_list->cur_cluster = GET_FAT_TBL(drv_no, p_list->cur_cluster);
                        }
                }
        }
        else if (seek_mode == FROM_BEGIN)
        {
                udword i;

                if (offset >= 0)
                {
                        p_list->f_ptr = offset;
                        if (p_list->f_ptr > p_list->size)
                                p_list->f_ptr = p_list->size;
                }
                else
                        p_list->f_ptr = 0;

                /* update current cluster */
                p_list->cur_cluster = p_list->cluster;
                p_list->old_cur_cluster = 0;
                for (i = cluster_size; i <= p_list->f_ptr; i += cluster_size)
                {
                        p_list->old_cur_cluster = p_list->cur_cluster;
                        p_list->cur_cluster = GET_FAT_TBL(drv_no, p_list->cur_cluster);
                }
        }
        else if (seek_mode == FROM_END)
        {
                udword i;

                if (offset >= 0)
                        p_list->f_ptr = p_list->size;
                else
                {
                        if (p_list->size < (udword)(-offset))
                                p_list->f_ptr = 0;
                        else
                                p_list->f_ptr = p_list->size + offset;
                }

                /* update current cluster */
                p_list->cur_cluster = p_list->cluster;
                p_list->old_cur_cluster = 0;
                for (i = cluster_size; i <= p_list->f_ptr; i += cluster_size)
                {
                        p_list->old_cur_cluster = p_list->cur_cluster;
                        p_list->cur_cluster = GET_FAT_TBL(drv_no, p_list->cur_cluster);
                }
        }
        else
                FAT_RETURN(drv_no, ERR_INVALID_PARAM);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smCloseFile
 * Remarks:
 *      - close the opened file
 *      - updates file modified time, date, and the size
 * Parameters
 *      . h_file: opened file handle that should be closed
 * Notes:
 *      - if FAT_UPDATE_WHEN_CLOSE is defined, the FAT table must be updated.
 *              if not defined, it is updated everytime the file handle chain
 *              structure is changed
 **********/
SM_EXPORT ERR_CODE smCloseFile(F_HANDLE h_file)
{
        udword drv_no;
        udword cluster;
        sOPENED_LIST* p_list = NULL;
        sTIME time_val = { 0,0,0,0,0,0};
        ubyte file_info[32];
        udword cluster_no;
        udword offset;
        udword id;

        drv_no = (h_file >> 24) & 0x7f;
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        cluster = h_file & 0xffff;
        id = (h_file >> 16) & 0xff;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_GetOpenedList(drv_no, cluster, id, &p_list);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        if (p_list->mode & OPEN_W)
        {
#ifndef WRITE_CACHE_DISABLE
                if (p_list->cache.count != 0)
                        sm_WriteCacheToDisk(drv_no, p_list);
#endif

                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_CLUSTER, p_list->h_parent, cluster, file_info, &cluster_no, &offset);
                if (s_smErr != SM_OK)
                        FAT_RETURN(drv_no, s_smErr);

                sm_GetTime(&time_val);
                if (time_val.year > 1980)
                        time_val.year -= 1980;
                else
                        time_val.year = 0;

                /* file_info[22-23] <= time */
                /*      Hour | Min | Sec => 5bit | 6bit | 5bit(half value) */
                file_info[22] = ((time_val.sec >> 1) & 0x1f) | ((time_val.min << 5) & 0xe0);
                file_info[23] = ((time_val.min >> 3) & 0x07) | ((time_val.hour << 3) & 0xf8);

                /* file_info[24-25] <= date */
                /*      Year | Month | Day => 7bit | 4bit | 5bit */
                file_info[24] = (time_val.day & 0x1f) | ((time_val.month << 5) & 0xe0);
                file_info[25] = ((time_val.month >> 3) & 0x01) | ((time_val.year << 1) & 0xfe);

                /* file_info[28-31] <= file size */
                file_info[28] = (ubyte)(p_list->size & 0xff);
                file_info[29] = (ubyte)((p_list->size >> 8) & 0xff);
                file_info[30] = (ubyte)((p_list->size >> 16) & 0xff);
                file_info[31] = (ubyte)((p_list->size >> 24) & 0xff);

                s_smErr = smcWriteCluster(drv_no, cluster_no, offset, file_info, 32);
                if (s_smErr != SM_OK)
                        FAT_RETURN(drv_no, s_smErr);

                sm_prepareFileClose(drv_no, p_list);

#ifdef FAT_UPDATE_WHEN_FILE_CLOSE
                sm_FATUpdate(drv_no);
#endif
        }

        sm_DeleteFromOpenedFileList(drv_no, cluster);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smRemoveFile
 * Remarks:
 *      - remove file and update FAT table
 * Parameters
 *      . p_file_name: file name that should be removed
 **********/
SM_EXPORT ERR_CODE smRemoveFile(const smchar* p_filename)
{
        udword drv_no;
        F_HANDLE h_dir;
        ubyte *p_file_name = (ubyte *)p_filename;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];

        drv_no = sm_GetDrvNo(p_file_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_file_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(p_file_name, long_name);

        s_smErr = sm_RemoveFile(drv_no, h_dir, long_name);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smGetSizeFile
 * Remarks:
 *      - retrieves file size
 * Parameters
 *      . p_file_name: file name string with full path
 *      . p_size (result): buffer that saves the file size
 **********/
SM_EXPORT ERR_CODE smGetSizeFile(const smchar* p_filename, udword* p_size)
{
        udword drv_no;
        F_HANDLE h_dir;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];
        ubyte file_info[32];
        udword cluster;
        udword offset;
        ubyte *p_file_name = (ubyte *)p_filename;

        drv_no = sm_GetDrvNo(p_file_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        *p_size = 0;

        s_smErr = sm_CheckPath(drv_no, p_file_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(p_file_name, long_name);

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, file_info, &cluster, &offset);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        *p_size = (udword)file_info[28] | ((udword)file_info[29] << 8)
                | ((udword)file_info[30] << 16) | ((udword)file_info[31] << 24);

        FAT_RETURN(drv_no, s_smErr);
}


/**********
 * Function: smFileExtend
 * Remarks:
 *      - expands the file size of the opened file
 * Parameters
 *      . h_file: file handle
 *      . size : amount of expand size
 **********/
SM_EXPORT ERR_CODE smFileExtend(F_HANDLE h_file, udword size)
{
        udword drv_no;
        udword cluster;
        sOPENED_LIST* p_list = NULL;
        udword id;
        udword addedClusterCount;
        udword sectorsPerCluster;
        udword clusterSize;

        drv_no = (h_file >> 24) & 0x7f;
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        cluster = h_file & 0xffff;
        id = (h_file >> 16) & 0xff;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_GetOpenedList(drv_no, cluster, id, &p_list);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        size += p_list->size;
        sectorsPerCluster = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster;
        clusterSize = sectorsPerCluster * SECTOR_SIZE;

        /* check if extended size needs more cluster */
        addedClusterCount = (size + clusterSize - 1) / clusterSize - (p_list->size + clusterSize - 1) / clusterSize;
        if (addedClusterCount)
        {
//                udword curEndSector;
//                udword nextEndSector;

                s_smErr = sm_addClusters(drv_no, cluster, addedClusterCount);
                if (s_smErr != SM_OK)
                        FAT_RETURN(drv_no, s_smErr);
#if 0   /* file may not have its clusters continously (dalma, 2000.3.15) */
                curEndSector = s_clusterStartSector[drv_no]     + (cluster - 2) * sectorsPerCluster
                                                + (p_list->size - p_list->size / clusterSize * clusterSize)     / SECTOR_SIZE;
                nextEndSector = s_clusterStartSector[drv_no] + (cluster + 1 - 2) * sectorsPerCluster - 1;

                if (curEndSector != nextEndSector)
                {
                        s_smErr = sm_eraseSectors(drv_no, curEndSector + 1, nextEndSector - curEndSector);
                        if (s_smErr != SM_OK)
                                FAT_RETURN(drv_no, s_smErr);
                }
#endif
        }
        else
        {
//                udword curEndSector;
//                udword nextEndSector;

#if 0   /* file may not have its clusters continously (dalma, 2000.3.15) */
                curEndSector = s_clusterStartSector[drv_no]     + (cluster - 2) * sectorsPerCluster
                                                + (p_list->size - p_list->size / clusterSize * clusterSize)     / SECTOR_SIZE;
                nextEndSector = s_clusterStartSector[drv_no] + (cluster - 2) * sectorsPerCluster
                                                + (size - size / clusterSize * clusterSize)     / SECTOR_SIZE;

                if (curEndSector != nextEndSector)
                {
                        s_smErr = sm_eraseSectors(drv_no, curEndSector + 1, nextEndSector - curEndSector);
                        if (s_smErr != SM_OK)
                                FAT_RETURN(drv_no, s_smErr);
                }
#endif
        }

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smCreateDir
 * Remarks:
 *      - creates directory and writes its default sub directory information
 * Parameters
 *      . p_dir_name: directory name that should be created
 *                                      (ex. dev0:\temp\readme)
 *      . dcreate_mode: reserved
 **********/
SM_EXPORT ERR_CODE smCreateDir(const smchar* p_dirname, udword dcreate_mode)
{
        udword drv_no;
        udword cluster;
        F_HANDLE h_dir;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];
        ubyte dummy_info[32];
        udword offset;
        sFILE_STAT sStat;
        ubyte *p_dir_name = (ubyte *)p_dirname;
#ifdef  LONG_FILE_NAME_ENABLE
        uword* p_unicode;
        ubyte fat_name[13];
#endif

        drv_no = sm_GetDrvNo(p_dir_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_dir_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(p_dir_name, long_name);

        /* check if the same name exists already */
        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, dummy_info, &cluster, &offset);
        if (s_smErr != ERR_NOT_FOUND)
                FAT_RETURN(drv_no, ERR_FILE_EXIST);

        s_smErr = smcAllocCluster(drv_no, &cluster);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* directory contents initialize */
        s_smErr = smcSetCluster(drv_no, cluster, 0);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* FAT table update */
        /* This must be done before another smcAllocCluster() is called */
        SET_FAT_TBL(drv_no, cluster, LAST_CLUSTER);

        /*
         * new directory contents write
         * (current dir, parent dir information write)
         */
        sStat.attr = ATTRIB_DIR;
        sStat.cluster = cluster;
        sStat.size = 0;
        sm_GetTime(&sStat.time);

        s_smErr = sm_AddToDirEntry(drv_no, cluster, (ubyte*)".", NULL, &sStat, 0, 0, 0);
        if (s_smErr != SM_OK)
        {
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, s_smErr);
        }

        sStat.cluster = h_dir;
        s_smErr = sm_AddToDirEntry(drv_no, cluster, (ubyte*)"..", NULL, &sStat, 0, 0, 0);
        if (s_smErr != SM_OK)
        {
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, s_smErr);
        }

        /* parent directory contents update */
#ifdef  LONG_FILE_NAME_ENABLE
        p_unicode = (uword*)SM_MALLOC(MAX_FILE_NAME_LEN+2);
        if(p_unicode == NULL)
        {
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);
        }

        sStat.cluster = cluster;
        if(sm_ConvertToFATName(long_name, fat_name))
        {
                SM_MBS_TO_UNICODE(p_unicode, long_name, 0);
                s_smErr = sm_AddToDirEntry(drv_no, h_dir, long_name, p_unicode, &sStat, 0, 0, 0);
        }
        else
                s_smErr = sm_AddToDirEntry(drv_no, h_dir, long_name, NULL, &sStat, 0, 0, 0);

        SM_FREE(p_unicode);
#else
        sStat.cluster = cluster;
        s_smErr = sm_AddToDirEntry(drv_no, h_dir, long_name, NULL, &sStat, 0, 0, 0);
#endif
        if (s_smErr != SM_OK)
        {
                SET_FAT_TBL(drv_no, cluster, UNUSED_CLUSTER);
                FAT_RETURN(drv_no, s_smErr);
        }

        sm_FATUpdate(drv_no);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smRemoveDir
 * Remarks:
 *      - remove directory
 * Parameters
 *      . p_dir_name: directory name that should be removed
 *      . ddel_mode: ALWAYS_DELETE, NOT_IF_NOT_EMPTY
 **********/
SM_EXPORT ERR_CODE smRemoveDir(const smchar* p_dirname, udword ddel_mode)
{
        udword drv_no;
        F_HANDLE h_dir;
        F_HANDLE handle;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];
        ubyte file_info[32];
        udword cluster;
        udword offset;
        udword i;
        ubyte *p_dir_name = (ubyte *)p_dirname;

        drv_no = sm_GetDrvNo(p_dir_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* if the given name is root directory, it returns error here */
        s_smErr = sm_CheckPath(drv_no, p_dir_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(p_dir_name, long_name);

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, file_info, &cluster, &offset);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, SM_OK);

        /* check if the parameter is directory */
        if (!(file_info[11] & 0x10))
                FAT_RETURN(drv_no, ERR_INVALID_PARAM);

        handle = file_info[26] | ((udword)file_info[27] << 8);

        if (ddel_mode == ALWAYS_DELETE)
        {
                /*
                 * It deletes the first bottommost directory and its files
                 * again and again, and at last there is no more subdirectory.
                 * Then sm_RemoveFirstBottommostDir() deletes all the files in
                 * the "p_dir_name" directory, and returns ERR_NO_MORE_ENTRY.
                 *
                 * *** It is implemented not to use recursive call to save
                 *              stack size.
                 *              Instead, it takes more time.
                 */
                s_smErr = SM_OK;
                while (s_smErr == SM_OK)
                {
                        s_smErr = sm_RemoveFirstBottommostDir(drv_no, handle);
                }

                if (s_smErr != ERR_NO_MORE_ENTRY)
                        FAT_RETURN(drv_no, s_smErr);
        }
        else
        {
                /* check if the directory has any entry */
                for (i = 0; i < 3; ++i)
                {
                        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_INDEX, handle, i, file_info, &cluster, &offset);
                        if (s_smErr != SM_OK)
                        {
                                if (s_smErr == ERR_NOT_FOUND)
                                {
                                        /*
                                         * this is the only case that the remove should be
                                         * continued
                                         */
                                        break;
                                }
                                else
                                        FAT_RETURN(drv_no, s_smErr);
                        }

                        if (file_info[0] == '.')                        /* skip ".", ".." */
                                continue;
                        else
                                FAT_RETURN(drv_no, ERR_DIR_NOT_EMPTY);
                }
        }

        /* deletes the directory itself from its parent entries */
        s_smErr = sm_RemoveFile(drv_no, h_dir, long_name);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smGetListNumber
 * Remarks:
 *      - retrieves the number of entry in the directory "p_dir_name"
 * Parameters
 *      . p_dir_name: directory name string with full path
 *      . p_num (result): the number of entry in the directory
 **********/
SM_EXPORT ERR_CODE smGetListNumDir(const smchar* p_dirname, udword* p_num)
{
        udword drv_no;
        F_HANDLE h_dir;
        udword cluster;
        udword offset;
        udword cluster_size;
        udword i, j;
        udword entry_count;
//        udword sector_count;
        ubyte* buf;
        ubyte *p_dir_name = (ubyte *)p_dirname;

        drv_no = sm_GetDrvNo(p_dir_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_dir_name, PATH_FULL, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        *p_num = 0;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
        entry_count = 0;

        buf = SMB_ALLOC_BUF();
        if (!buf)
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);

        if (h_dir == ROOT_HANDLE)
        {
                for (i = s_rootStartSector[drv_no]; i < s_rootStartSector[drv_no] + s_rootSectors[drv_no]; ++i)
                {
                        s_smErr = smlReadSector(drv_no, i, 0, buf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(buf);
                                FAT_RETURN(drv_no, s_smErr);
                        }

                        for (j = 0; j < SECTOR_SIZE; j += 32)
                        {
                                /*
                                 * exclude no file, deleted file, entry for long file
                                 * name
                                 */
                                if(buf[j] == 0)
                                        break;

                                if ((buf[j] != 0xe5) && ((buf[j + 11] & 0xf) != 0xf))
                                        ++entry_count;
                        }

                        if(j < SECTOR_SIZE)
                                break;
                }
        }
        else
        {
                cluster = h_dir & 0xffff;

                while (cluster != LAST_CLUSTER)
                {
                        for (offset = 0; offset < cluster_size; offset += SECTOR_SIZE)
                        {
                                s_smErr = smcReadCluster(drv_no, cluster, offset, buf, SECTOR_SIZE);
                                if (s_smErr != SM_OK)
                                {
                                        SMB_FREE_BUF(buf);
                                        FAT_RETURN(drv_no, s_smErr);
                                }

                                for (j = 0; j < SECTOR_SIZE; j += 32)
                                {
                                        /*
                                         * exclude no file, deleted file, entry for long
                                         * file name
                                         */
                                        if(buf[j] == 0)
                                                break;

                                        if ((buf[j] != 0xe5) && ((buf[j + 11] & 0xf) != 0xf))
                                                ++entry_count;
                                }

                                if(j < SECTOR_SIZE)
                                        break;
                        }

                        cluster = GET_FAT_TBL(drv_no, cluster);

                        /*
                         * this is the case that FAT table is not updated correctly
                         */
                        if (cluster == UNUSED_CLUSTER)
                        {
                                SMB_FREE_BUF(buf);
                                FAT_RETURN(drv_no, ERR_INCORRECT_FAT);
                        }
                }
        }

        *p_num = entry_count;

        SMB_FREE_BUF(buf);
        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smReadDir
 * Remarks:
 *      - retrieves entries' simple information in the directory
 *              "p_dir_name"
 * Parameters
 *      . p_dir_name: directory name string with full path
 *      . entry_start: start index of the entry
 *      . entry_count: number of entries that should be retrieved
 *      . p_buf (result): buffer that saves the entries' information
 *      . p_read_count (result): number of entries that was saved
 * Notes:
 *      - "p_buf" must be prepared for space of
 *              "entry_count * sizeof(sDIR_ENTRY)"
 **********/
SM_EXPORT ERR_CODE smReadDir(const smchar* p_dirname, udword entry_start, udword entry_count, sDIR_ENTRY *p_buf, udword* p_read_count)
{
        udword drv_no;
        F_HANDLE h_dir;
        ubyte file_info[32];
        udword cluster;
        udword offset;
        udword i, j, k;
        ubyte *p_dir_name = (ubyte *)p_dirname;

        drv_no = sm_GetDrvNo(p_dir_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_dir_name, PATH_FULL, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        *p_read_count = 0;

        for (i = 0; i < entry_count; ++i)
        {
                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_INDEX, h_dir, entry_start + i, file_info, &cluster, &offset);
                if (s_smErr != SM_OK)
                {
                        if (s_smErr == ERR_NOT_FOUND)
                        {
                                *p_read_count = i;
                                FAT_RETURN(drv_no, ERR_EOF);
                        }
                        else
                                FAT_RETURN(drv_no, s_smErr);
                }

                for (j = 0, k = 0; j < 11; ++j)
                {
                        if (file_info[j] != ' ')
                        {
                                if (j == 8)
                                {
                                        p_buf[i].name[k] = '.';
                                        ++k;
                                }

                                p_buf[i].name[k] = file_info[j];
                                ++k;
                        }
                }

                p_buf[i].name[k] = 0;
        }

        *p_read_count = entry_count;

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smReadDirEx
 * Remarks:
 *      - retrieves entries' full information in the directory "p_dir_name"
 * Parameters
 *      . p_dir_name: directory name string with full path
 *      . entry_start: start index of the entry
 *      . entry_count: number of entries that should be retrieved
 *      . p_buf (result): buffer that saves the entries' information
 *      . p_read_count (result): number of entries that was saved
 * Notes:
 *      - "p_buf" must be prepared for space of
 *              "entry_count * sizeof(sDIR_ENTRY_EX)"
 **********/
SM_EXPORT ERR_CODE smReadDirEx(const smchar* p_dirname, udword entry_start, udword entry_count, sDIR_ENTRY_EX p_buf[],  udword* p_read_count)
{
        udword drv_no;
        F_HANDLE h_dir;
        ubyte file_info[32];
        udword cluster;
        udword offset;
        udword i, j, k;
        ubyte *p_dir_name = (ubyte *)p_dirname;

        drv_no = sm_GetDrvNo(p_dir_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_dir_name, PATH_FULL, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        *p_read_count = 0;

        for (i = 0; i < entry_count; ++i)
        {
                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_INDEX, h_dir, entry_start + i, file_info, &cluster, &offset);
                if (s_smErr != SM_OK)
                {
                        if (s_smErr == ERR_NOT_FOUND)
                        {
                                *p_read_count = i;
                                FAT_RETURN(drv_no, ERR_EOF);
                        }
                        else
                                FAT_RETURN(drv_no, s_smErr);
                }

                for (j = 0, k = 0; j < 11; ++j)
                {
                        if (file_info[j] != ' ')
                        {
                                if (j == 8)
                                {
                                        p_buf[i].name[k] = '.';
                                        ++k;
                                }

                                p_buf[i].name[k] = file_info[j];
                                ++k;
                        }
                }
                p_buf[i].name[k] = 0;

#ifdef LONG_FILE_NAME_ENABLE
                s_smErr = sm_GetLongName(drv_no, h_dir, cluster, offset, p_buf[i].long_name);
                if ((s_smErr != SM_OK) && (s_smErr != ERR_FILE_NAME_LEN_TOO_LONG))
                        p_buf[i].long_name[0] = 0;
#else
                p_buf[i].long_name[0] = 0;
#endif

                p_buf[i].stat.attr = file_info[11];
                p_buf[i].stat.time.year = 1980 + ((file_info[25] >> 1) & 0x7f);
                p_buf[i].stat.time.month = ((file_info[25] << 3) & 0x08) | ((file_info[24] >> 5) & 0x07);
                p_buf[i].stat.time.day = file_info[24] & 0x1f;
                p_buf[i].stat.time.hour = (file_info[23] >> 3) & 0x1f;
                p_buf[i].stat.time.min = ((file_info[23] << 3) & 0x38) | ((file_info[22] >> 5) & 0x07);
                p_buf[i].stat.time.sec = file_info[22] & 0x1f;
                p_buf[i].stat.time.msec = 0;
                p_buf[i].stat.cluster = (uword)file_info[26] | ((uword)file_info[27] << 8);
                p_buf[i].stat.size = (udword)file_info[28] | ((udword)file_info[29] << 8)
                                                        | ((udword)file_info[30] << 16) | ((udword)file_info[31] << 24);
        }

        *p_read_count = entry_count;

        FAT_RETURN(drv_no, SM_OK);
}

/**********
 * Function: smReadDirEx
 * Remarks:
 *      - retrieves entries' full information in the directory "p_dir_name"
 * Parameters
 *      . p_dir_name: directory name string with full path
 *      . entry_start: start index of the entry
 *      . entry_count: number of entries that should be retrieved
 *      . p_buf (result): buffer that saves the entries' information
 *      . p_read_count (result): number of entries that was saved
 * Notes:
 *      - "p_buf" must be prepared for space of
 *              "entry_count * sizeof(sDIR_ENTRY_EX)"
 **********/
SM_EXPORT ERR_CODE smReadDirExHandle(F_HANDLE h_file, udword entry_start, udword entry_count, sDIR_ENTRY_EX p_buf[],    udword* p_read_count)
{
        udword drv_no;
        F_HANDLE h_dir;
        sOPENED_LIST* p_list;
        ubyte file_info[32];
        udword cluster;
        udword offset, id;
        udword i, j, k;

        //ubyte *p_dir_name = (ubyte *)p_dirname;

        drv_no = (h_file >> 24) & 0x7f;
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        cluster = h_file & 0xffff;
        id = (h_file >> 16) & 0xff;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_GetOpenedList(drv_no, cluster, id, &p_list);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        *p_read_count = 0;

        h_dir = (F_HANDLE)cluster;

        for (i = 0; i < entry_count; ++i)
        {
                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_INDEX, h_dir, entry_start + i, file_info, &cluster, &offset);
                if (s_smErr != SM_OK)
                {
                        if (s_smErr == ERR_NOT_FOUND)
                        {
                                *p_read_count = i;
                                FAT_RETURN(drv_no, ERR_EOF);
                        }
                        else
                                FAT_RETURN(drv_no, s_smErr);
                }

                for (j = 0, k = 0; j < 11; ++j)
                {
                        if (file_info[j] != ' ')
                        {
                                if (j == 8)
                                {
                                        p_buf[i].name[k] = '.';
                                        ++k;
                                }

                                p_buf[i].name[k] = file_info[j];
                                ++k;
                        }
                }
                p_buf[i].name[k] = 0;

#ifdef LONG_FILE_NAME_ENABLE
                s_smErr = sm_GetLongName(drv_no, h_dir, cluster, offset, p_buf[i].long_name);
                if ((s_smErr != SM_OK) && (s_smErr != ERR_FILE_NAME_LEN_TOO_LONG))
                        p_buf[i].long_name[0] = 0;
#else
                p_buf[i].long_name[0] = 0;
#endif

                p_buf[i].stat.attr = file_info[11];
                p_buf[i].stat.time.year = 1980 + ((file_info[25] >> 1) & 0x7f);
                p_buf[i].stat.time.month = ((file_info[25] << 3) & 0x08) | ((file_info[24] >> 5) & 0x07);
                p_buf[i].stat.time.day = file_info[24] & 0x1f;
                p_buf[i].stat.time.hour = (file_info[23] >> 3) & 0x1f;
                p_buf[i].stat.time.min = ((file_info[23] << 3) & 0x38) | ((file_info[22] >> 5) & 0x07);
                p_buf[i].stat.time.sec = file_info[22] & 0x1f;
                p_buf[i].stat.time.msec = 0;
                p_buf[i].stat.cluster = (uword)file_info[26] | ((uword)file_info[27] << 8);
                p_buf[i].stat.size = (udword)file_info[28] | ((udword)file_info[29] << 8)
                                                        | ((udword)file_info[30] << 16) | ((udword)file_info[31] << 24);
        }

        *p_read_count = entry_count;

        FAT_RETURN(drv_no, SM_OK);
}

/**********
 * Function: smReadStat
 * Remarks:
 *      - retrieves file(directory) information
 * Parameters
 *      . p_name: file(directory) name string with full path
 *      . p_stat (result): buffer that saves the file(directory) information
 * Notes:
 *      - "p_stat" must be prepared for space of "sizeof(sFILE_STAT)"
 *      - if "p_name" indicates root directory, it returns ERR_ROOT_DIR
 **********/
SM_EXPORT ERR_CODE smReadStat(const smchar* p_entry_name, sFILE_STAT* p_stat)
{
        udword drv_no;
        F_HANDLE h_dir;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];
        ubyte file_info[32];
        udword cluster;
        udword offset;
        ubyte *p_name = (ubyte *)p_entry_name;

        drv_no = sm_GetDrvNo(p_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
        {
                s_smErr =  sm_CheckPath(drv_no, p_name, PATH_FULL, &h_dir);
                if (s_smErr == SM_OK)
                {
                        if (h_dir == ROOT_HANDLE)
                                FAT_RETURN(drv_no, ERR_ROOT_DIR);
                        else
                                FAT_RETURN(drv_no, ERR_INVALID_PARAM);
                }
                else
                        FAT_RETURN(drv_no, s_smErr);
        }

        sm_ExtractLastName(p_name, long_name);

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, file_info, &cluster, &offset);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        p_stat->attr = file_info[11];
        p_stat->time.year = 1980 + ((file_info[25] >> 1) & 0x7f);
        p_stat->time.month = ((file_info[25] << 3) & 0x08) | ((file_info[24] >> 5) & 0x07);
        p_stat->time.day = file_info[24] & 0x1f;
        p_stat->time.hour = (file_info[23] >> 3) & 0x1f;
        p_stat->time.min = ((file_info[23] << 3) & 0x38) | ((file_info[22] >> 5) & 0x07);
        p_stat->time.sec = file_info[22] & 0x1f;
        p_stat->time.msec = 0;
        p_stat->cluster = (udword)file_info[26] | ((udword)file_info[27] << 8);
        p_stat->size = (udword)file_info[28] | ((udword)file_info[29] << 8)
                                        | ((udword)file_info[30] << 16) | ((udword)file_info[31] << 24);

        FAT_RETURN(drv_no, s_smErr);
}


/**********
 * Function: smWriteStat
 * Remarks:
 *      - updates the file(directory) information
 * Parameters
 *      . p_name: file(directory) name string with full path
 *      . p_stat (result): file(directory) information pointer
 * Notes:
 *      - p_stat->cluster, p_stat->size is not adapted. these are not the
 *              values user can change
 *      - if "p_name" indicates root directory, it returns ERR_ROOT_DIR
 **********/
SM_EXPORT ERR_CODE smWriteStat(const smchar* p_entry_name, const sFILE_STAT* p_stat)
{
        udword drv_no;
        F_HANDLE h_dir;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];
        ubyte file_info[32];
        udword cluster;
        udword offset;
        ubyte year;
        ubyte *p_name = (ubyte *)p_entry_name;

        drv_no = sm_GetDrvNo(p_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
        {
                s_smErr =  sm_CheckPath(drv_no, p_name, PATH_FULL, &h_dir);
                if (s_smErr == SM_OK)
                {
                        if (h_dir == ROOT_HANDLE)
                                FAT_RETURN(drv_no, ERR_ROOT_DIR);
                        else
                                FAT_RETURN(drv_no, ERR_INVALID_PARAM);
                }
                else
                        FAT_RETURN(drv_no, s_smErr);
        }

        sm_ExtractLastName(p_name, long_name);

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, file_info, &cluster, &offset);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        file_info[11] = (ubyte)(p_stat->attr & 0xff);

        /* file_info[22-23] <= time */
        /*      Hour | Min | Sec => 5bit | 6bit | 5bit(half value) */
        file_info[22] = ((p_stat->time.sec >> 1) & 0x1f) | ((p_stat->time.min << 5) & 0xe0);
        file_info[23] = ((p_stat->time.min >> 3) & 0x07) | ((p_stat->time.hour << 3) & 0xf8);

        /* file_info[24-25] <= date */
        /*      Year | Month | Day => 7bit | 4bit | 5bit */
        file_info[24] = ((p_stat->time.day) & 0x1f)     | ((p_stat->time.month << 5) & 0xe0);
        if (p_stat->time.year > 1980)
                year = p_stat->time.year - 1980;
        else
                year = 0;

        file_info[25] = ((p_stat->time.month >> 3) & 0x01) | ((year << 1) & 0xfe);

        s_smErr = smcWriteCluster(drv_no, cluster, offset, file_info, 32);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        FAT_RETURN(drv_no, SM_OK);
}


#ifdef LONG_FILE_NAME_ENABLE
/**********
 * Function: smSetLongName
 * Remarks:
 *      - set the long name
 * Parameters
 *      . p_file_name: file name that exists (ex. dev0:\temp\readme.txt)
 *      . p_long_name: long name(without directory path) that is in Unicode
 * Notes:
 *      - SMFS does not support creation of file with its long name becuase
 *              it needs very large Unicode table. Instead, it is possible to
 *              set its long name using this function in the environment that
 *              has methods for Unicode translation
 **********/
SM_EXPORT ERR_CODE smSetLongName(const smchar* p_filename, const uword* p_long_name)
{
        udword drv_no;
        F_HANDLE h_dir;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];
        ubyte file_info[32];
        udword cluster;
        udword offset;
        sFILE_STAT sStat;
        ubyte *p_file_name = (ubyte *)p_filename;

        drv_no = sm_GetDrvNo(p_file_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_file_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(p_file_name, long_name);

        /* search existing file for use of the 32 byte file_info */
        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, file_info, &cluster, &offset);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sStat.attr = file_info[11];
        sStat.cluster = file_info[26] | ((udword)file_info[27] << 8);
        sStat.size = file_info[28] | ((udword)file_info[29] << 8) | ((udword)file_info[30] << 16) | ((udword)file_info[31] << 24);
        sStat.time.year = 1980 + ((file_info[25] >> 1) & 0x7f);
        sStat.time.month = ((file_info[25] << 3) & 0x08) | ((file_info[24] >> 5) & 0x07);
        sStat.time.day = file_info[24] & 0x1f;
        sStat.time.hour = (file_info[23] >> 3) & 0x1f;
        sStat.time.min = ((file_info[23] << 3) & 0x38) | ((file_info[22] >> 5) & 0x07);
        sStat.time.sec = file_info[22] & 0x1f;
        sStat.time.msec = 0;

        /* delete existing entry */
        s_smErr = sm_DeleteFromDirEntry(drv_no, h_dir, long_name);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* set new entry with long file name */
        s_smErr = sm_AddToDirEntry(drv_no, h_dir, (const ubyte*)long_name, p_long_name, &sStat, 0, 0, 0);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smGetLongName
 * Remarks:
 *      - get the long name
 * Parameters
 *      . p_file_name: file name that exists (ex. dev0:\temp\readme.txt)
 *      . p_long_name: long name(without directory path) that is in Unicode
 * Notes:
 *      - p_long_name is returned with Unicode
 **********/
SM_EXPORT ERR_CODE smGetLongName(const smchar* p_filename, uword* p_long_name)
{
        udword drv_no;
        F_HANDLE h_dir;
        ubyte file_info[32];
        udword cluster;
        udword offset;
        ubyte *p_file_name = (ubyte *)p_filename;
        ubyte long_name[MAX_FILE_NAME_LEN + 1];

        drv_no = sm_GetDrvNo(p_file_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        s_smErr = sm_CheckPath(drv_no, p_file_name, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(p_file_name, long_name);

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, file_info, &cluster, &offset);

        s_smErr = sm_GetLongName(drv_no, h_dir, cluster, offset, p_long_name);
        if (s_smErr != SM_OK)
        {
                if(s_smErr != ERR_FILE_NAME_LEN_TOO_LONG)
                        p_long_name[0] = 0;

                FAT_RETURN(drv_no, s_smErr);
        }

        FAT_RETURN(drv_no, SM_OK);
}
#endif          /* LONG_FILE_NAME_ENABLE */


/**********
 * Function: smGetVolInfo
 * Remarks:
 *      - get volumn information
 * Parameters
 *      . p_vol_name: volumn name that is mounted (ex. "dev0:")
 *      . p_info (result): pointer of volumn info structure
 **********/
SM_EXPORT ERR_CODE smGetVolInfo(const smchar* p_volumn_name, sVOL_INFO* p_info)
{
        udword drv_no;
        udword total_clusters;
        udword cluster_size;
        udword fat_val;
        udword i;
        ubyte *p_vol_name = (ubyte *)p_volumn_name;

        drv_no = sm_GetDrvNo(p_vol_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
        total_clusters = (s_smInfo[drv_no].pbr.bpb.total_sectors ? s_smInfo[drv_no].pbr.bpb.total_sectors : s_smInfo[drv_no].pbr.bpb.huge_sectors - 1)
                                        / s_smInfo[drv_no].pbr.bpb.sectors_per_cluster + 1;
        p_info->total_size = total_clusters * s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
        p_info->sector_per_cluster = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster;
        p_info->sector_size = SECTOR_SIZE;

        p_info->used_size = p_info->free_size = p_info->bad_cluster_num = 0;


        for (i = 2; i < total_clusters + 2; ++i)
        {
                fat_val = GET_FAT_TBL(drv_no, i);

                if (fat_val == UNUSED_CLUSTER)
                        p_info->free_size += cluster_size;
                else if (fat_val == DEFECTIVE_CLUSTER)
                        ++p_info->bad_cluster_num;
                else
                        p_info->used_size += cluster_size;
        }

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smFormatVol
 * Remarks:
 *      - lower layer format, get device info from lower layer,
 *              set MBR/PBR/FAT, and initialize FAT layer variables
 * Parameters:
 *      . p_vol_name: volumn name that is mounted (ex. "dev0:")
 *      . p_label: label string
 *      . format_id: FORMAT_NORMAL, FORMAT_RESCUE
 *      . p_bad_count (result): number of error blocks
 * Notes:
 *      - do not implements sm_PreFAT as in other API's, but lock is needed
 *      - format_id is not used now (reserved)
 **********/
SM_EXPORT ERR_CODE smFormatVol(const smchar* p_volumn_name, const smchar* p_label, udword format_id, udword* p_bad_count)
{
        udword drv_no;
        udword i, j;
        sSM_INFO sm_info;
        const sDEV_INFO* dev_info;
        udword pbr_sector;
        udword block;
        udword sector;
        udword offset;
        udword changed_pblock;
        udword needed_blocks;
        udword needed_sectors;
        udword fat_sectors;
        udword root_sectors;
        udword err_count;
        ubyte* acFATBuf;
        ubyte *p_vol_name = (ubyte *)p_volumn_name;

        drv_no = sm_GetDrvNo(p_vol_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        /*
         * lower layer format. all physical blocks are erased now.
         */
        format_id = (format_id == FORMAT_RESCUE) ? LPT_FORMAT_RESCUE : LPT_FORMAT_NORMAL;
        s_smErr = smlFormatVol(drv_no, format_id, &err_count);
        if (s_smErr != SM_OK)
                return s_smErr;

        *p_bad_count = err_count;

        /*
         * get device info
         */
        s_smErr = smlGetDeviceInfo(drv_no, &dev_info);
        if (s_smErr != SM_OK)
                return s_smErr;

        fat_sectors = (dev_info->allS <= 128000)
                                        ? (((dev_info->LBpV + 2) * 3 / 2 - 1) / dev_info->szS + 1)
                                        : (((dev_info->LBpV + 2) * 2 - 1) / dev_info->szS + 1);
        root_sectors = (ROOT_DIR_ENTRY * 32 - 1) / SECTOR_SIZE + 1;
        needed_sectors = 1 + 2 * fat_sectors + root_sectors;
        /* let 0th block reserved only for MBR */
        needed_blocks = (needed_sectors - 1) / dev_info->SpB + 2;
        pbr_sector = needed_blocks * dev_info->SpB - needed_sectors;

        /* set MBR values */
        sm_info.mbr.partition_entry[0].def_boot = 0x80;
        sm_info.mbr.partition_entry[0].start_cyl = (pbr_sector / dev_info->SpH) / dev_info->HpC;
        sm_info.mbr.partition_entry[0].start_head = (pbr_sector / dev_info->SpH) % dev_info->HpC;
        sm_info.mbr.partition_entry[0].start_sector     = (pbr_sector % dev_info->SpH) + 1;
        sm_info.mbr.partition_entry[0].pt_type = 1;             /*??? */
        sm_info.mbr.partition_entry[0].end_cyl = dev_info->CpV - 1;
        sm_info.mbr.partition_entry[0].end_head = dev_info->HpC - 1;
        sm_info.mbr.partition_entry[0].end_sector = dev_info->SpH;
        sm_info.mbr.partition_entry[0].start_lba_sector = pbr_sector;
        sm_info.mbr.partition_entry[0].total_available_sectors = dev_info->CpV * dev_info->HpC * dev_info->SpH - pbr_sector;
        sm_info.mbr.signature = 0x55aa;

        /* set PBR values */
        SM_MEMCPY(&sm_info.pbr.oem_name[0], "SMFS1.0", 8);
        sm_info.pbr.bpb.bytes_per_sector = dev_info->szS;
        sm_info.pbr.bpb.sectors_per_cluster = dev_info->SpB;
        sm_info.pbr.bpb.reserved_sectors = 1;           /*??? */
        sm_info.pbr.bpb.fat_num = 2;
        sm_info.pbr.bpb.root_entry = ROOT_DIR_ENTRY;
/*      sm_info.pbr.bpb.total_sectors
                = (dev_info->allS <= 65535) ? dev_info->allS : 0; */
        sm_info.pbr.bpb.total_sectors = (sm_info.mbr.partition_entry[0].total_available_sectors <= 65535)
                                                                        ? sm_info.mbr.partition_entry[0].total_available_sectors : 0;
        sm_info.pbr.bpb.format_type = 0xf8;
        sm_info.pbr.bpb.sectors_in_fat = fat_sectors;
        sm_info.pbr.bpb.sectors_per_track = dev_info->SpH;      /*??? */
        sm_info.pbr.bpb.head_num = dev_info->HpC;
        sm_info.pbr.bpb.hidden_sectors = pbr_sector;
/*      sm_info.pbr.bpb.huge_sectors
                = (dev_info->allS <= 65535) ? 0 : dev_info->allS; */
        sm_info.pbr.bpb.huge_sectors = (sm_info.mbr.partition_entry[0].total_available_sectors <= 65535)
                                                                        ? 0     : sm_info.mbr.partition_entry[0].total_available_sectors;
        sm_info.pbr.drive_num = 0x80;
        sm_info.pbr.reserved = 0;
        sm_info.pbr.ext_boot_signature = 0x29;
        sm_info.pbr.vol_id = 0;         /*??? */
        SM_MEMSET(&sm_info.pbr.vol_label[0], ' ', 11);
        for (i = 0; i < 11; ++i)
        {
                if (p_label[i] == 0)
                        break;

                sm_info.pbr.vol_label[i] = p_label[i];
        }
        if (dev_info->allS <= 128000)           /* 64MB */
                sm_info.pbr.file_sys_type = FS_FAT12;
        else
                sm_info.pbr.file_sys_type = FS_FAT16;

        sm_info.pbr.signature = 0x55aa;

        /* write MBR & PBR */
        s_smErr = sm_WriteSMInfo(drv_no, &sm_info);
        if (s_smErr != SM_OK)
                return s_smErr;

        /*
         * register new logical blocks until the end of root_dir_entry
         */
        block = (pbr_sector + (2 * sm_info.pbr.bpb.sectors_in_fat) + ((ROOT_DIR_ENTRY * 32 - 1) / SECTOR_SIZE + 1))     / sm_info.pbr.bpb.sectors_per_cluster;
        for (i = pbr_sector / sm_info.pbr.bpb.sectors_per_cluster + 1;  i <= block; ++i)
        {
                smlGetNewSpecifiedBlock(drv_no, i, &changed_pblock);
        }

        /*
         * FAT table write
         */
        acFATBuf = SMB_ALLOC_BUF();
        if (!acFATBuf)
                return ERR_OUT_OF_MEMORY;

        SM_MEMSET(acFATBuf, 0, SECTOR_SIZE);
        block = (pbr_sector + 1) / sm_info.pbr.bpb.sectors_per_cluster;
        offset = ((pbr_sector + 1) % sm_info.pbr.bpb.sectors_per_cluster) * SECTOR_SIZE;

        for (i = 0; i < 2; ++i)
        {
                acFATBuf[0] = 0xf8;
                acFATBuf[1] = 0xff;
                acFATBuf[2] = 0xff;

                s_smErr = smlWriteBlock(drv_no, block, offset, acFATBuf, SECTOR_SIZE);
                if (s_smErr != SM_OK)
                {
                        SMB_FREE_BUF(acFATBuf);
                        return s_smErr;
                }

                offset += SECTOR_SIZE;
                if (offset >= sm_info.pbr.bpb.sectors_per_cluster * SECTOR_SIZE)
                {
                        ++block;
                        offset = 0;
                }

                acFATBuf[0] = 0;
                acFATBuf[1] = 0;
                acFATBuf[2] = 0;

                for (j = 1; j < sm_info.pbr.bpb.sectors_in_fat; ++j)
                {
                        s_smErr = smlWriteBlock(drv_no, block, offset, acFATBuf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(acFATBuf);
                                return s_smErr;
                        }

                        offset += SECTOR_SIZE;
                        if (offset >= sm_info.pbr.bpb.sectors_per_cluster * SECTOR_SIZE)
                        {
                                ++block;
                                offset = 0;
                        }
                }
        }

        /*
         * Root Dir Entry write (empty)
         */
        SM_MEMSET(acFATBuf, 0, SECTOR_SIZE);
        sector = root_sectors;
        for (i = 0; i < sector; ++i)
        {
                s_smErr = smlWriteBlock(drv_no, block, offset, acFATBuf, SECTOR_SIZE);
                if (s_smErr != SM_OK)
                {
                        SMB_FREE_BUF(acFATBuf);
                        return s_smErr;
                }

                offset += SECTOR_SIZE;
                if (offset >= sm_info.pbr.bpb.sectors_per_cluster * SECTOR_SIZE)
                {
                        ++block;
                        offset = 0;
                }
        }
        SMB_FREE_BUF(acFATBuf);

        /*
         * FAT variables initialize
         */
        sm_FATInit(drv_no);

        return SM_OK;
}


/**********
 * Function: smMountVol
 * Remarks:
 *      - maps string volumn name to the drive. Default name is
 *              "dev0", .., "dev99"
 * Parameters
 *      . drv_no: drive number that is initialized in configuration file
 *      . p_vol_name: string volumn name that is mapped to the drive
 **********/
SM_EXPORT ERR_CODE smMountVol(udword drv_no, const smchar* p_volumn_name)
{
        ubyte *p_vol_name = (ubyte *)p_volumn_name;

        if (SM_STRLEN((char*)p_vol_name) > MAX_DRIVE_NAME_LEN)
                return ERR_INVALID_PARAM;

        SM_STRCPY((char*)s_drvName[drv_no], (char*)p_vol_name);

        return SM_OK;
}


/**********
 * Function: smUnmountVol
 * Remarks:
 *      - reverts to the default string volumn name
 *              (ex. "dev0", .., "dev99")
 * Parameters
 *      . drv_no: drive number that is initialized in configuration file
 **********/
SM_EXPORT ERR_CODE smUnmountVol(udword drv_no)
{
        ubyte name[10];

        SM_STRCPY((char*)name, "dev");
        if (drv_no < 10)
        {
                name[4] = (ubyte)drv_no + '0';
                name[5] = 0;
        }
#if (MAX_DRIVE >= 10)
        else if (drv_no < 100)
        {
                name[4] = drv_no / 10 + '0';
                name[5] = drv_no % 10 + '0';
                name[6] = 0;
        }
        else
        {
                name[4] = drv_no / 100 + '0';
                name[5] = (drv_no / 10) % 10 + '0';
                name[6] = drv_no % 10 + '0';
                name[7] = 0;
        }
#endif

        SM_STRCPY((char*)s_drvName[drv_no], (char*)name);

        return SM_OK;
}


/**********
 * Function: smMoveFile
 * Remarks:
 *  - move a file or directory to a new directory
 *  - old path and new path should be on the same volume
 *  - new path must not already exist
 * Parameters
 *  . old_name: path of old file or directory
 *  . new_name: path of new file or directory
 **********/
SM_EXPORT ERR_CODE smMoveFile(const smchar* old_name, const smchar* new_name)
{
        udword drv_no;
        F_HANDLE h_dir;
        F_HANDLE h_new_dir;
        F_HANDLE h_file;
        ubyte fileInfo[32];
        udword cluster;
        udword offset;
        ubyte oldName[MAX_FILE_NAME_LEN + 1];
        ubyte newName[MAX_FILE_NAME_LEN + 1];
        ubyte * oldPath = (ubyte *)old_name;
        ubyte * newPath = (ubyte *)new_name;
        sFILE_STAT sStat;
#ifdef  LONG_FILE_NAME_ENABLE
        uword* p_unicode;
        ubyte fat_name[13];
#endif

        drv_no = sm_GetDrvNo(oldPath);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        /* if different volumes, fail */
        if (drv_no != sm_GetDrvNo(newPath))
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* check if old & new directories are the same. */
        s_smErr = sm_CheckPath(drv_no, oldPath, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, ERR_FILE_NOT_EXIST);

        s_smErr = sm_CheckPath(drv_no, newPath, PATH_EXCEPT_LAST, &h_new_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, ERR_FILE_EXIST);

        /* check if newPath exist. if so, fail */
        s_smErr = sm_CheckPath(drv_no, newPath, PATH_FULL, &h_file);
        if (s_smErr == SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(oldPath, oldName);
        sm_ExtractLastName(newPath, newName);

        /* retrieve old file or directory info */
        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)oldName, fileInfo, &cluster, &offset);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* delete old file */
        s_smErr = sm_DeleteFromDirEntry(drv_no, h_dir, oldName);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* create new directory entry */
        sStat.attr = fileInfo[11];
        sStat.cluster = fileInfo[26] | ((udword)fileInfo[27] << 8);
        sStat.size = fileInfo[28] | ((udword)fileInfo[29] << 8) | ((udword)fileInfo[30] << 16) | ((udword)fileInfo[31] << 24);
        sStat.time.year = 1980 + ((fileInfo[25] >> 1) & 0x7f);
        sStat.time.month = ((fileInfo[25] << 3) & 0x08) | ((fileInfo[24] >> 5) & 0x07);
        sStat.time.day = fileInfo[24] & 0x1f;
        sStat.time.hour = (fileInfo[23] >> 3) & 0x1f;
        sStat.time.min = ((fileInfo[23] << 3) & 0x38) | ((fileInfo[22] >> 5) & 0x07);
        sStat.time.sec = fileInfo[22] & 0x1f;
        sStat.time.msec = 0;

#ifdef  LONG_FILE_NAME_ENABLE
        p_unicode = (uword*)SM_MALLOC(MAX_FILE_NAME_LEN+2);
        if(p_unicode == NULL)
        {
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);
        }

        if(sm_ConvertToFATName(newName, fat_name))
        {
                SM_MBS_TO_UNICODE(p_unicode, newName, 0);
                s_smErr = sm_AddToDirEntry(drv_no, h_new_dir, newName, p_unicode, &sStat, 0, 0, 0);
        }
        else
                s_smErr = sm_AddToDirEntry(drv_no, h_new_dir, newName, NULL, &sStat, 0, 0, 0);

        SM_FREE(p_unicode);
#else
        s_smErr = sm_AddToDirEntry(drv_no, h_new_dir, newName, NULL, &sStat, 0, 0, 0);
#endif

        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        FAT_RETURN(drv_no, SM_OK);
}



/**********
 * Function: smRenameFile
 * Remarks:
 *  - rename a file or directory
 *  - old path and new path should have the same parent path
 *  - new path must not already exist
 * Parameters
 *  . old_name: path of old file or directory
 *  . new_name: path of new file or directory
 * Notes
 *      - smRemoveFile deletes old_name and create new_name, but smRenameFile
 *              just renames old_name to new_name at the same position
 *      - This API is not recommended to use in normal case. It could have a lot of overheads
 *              that shifts the whole block contents in the worst case
 **********/
SM_EXPORT ERR_CODE smRenameFile(const smchar* old_name, const smchar* new_name)
{
        udword drv_no;
        F_HANDLE h_dir;
        F_HANDLE h_new_dir;
        F_HANDLE h_file;
        ubyte fileInfo[32];
        udword cluster;
        udword offset;
        sFILE_STAT sStat;
        ubyte oldName[MAX_FILE_NAME_LEN + 1];
        ubyte newName[MAX_FILE_NAME_LEN + 1];
        ubyte * oldPath = (ubyte *)old_name;
        ubyte * newPath = (ubyte *)new_name;
#ifdef  LONG_FILE_NAME_ENABLE
        uword* p_unicode;
        ubyte fat_name[13];
#endif

        drv_no = sm_GetDrvNo(oldPath);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        /* if different volumes, fail */
        if (drv_no != sm_GetDrvNo(newPath))
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* check if old & new directories are the same. */
        s_smErr = sm_CheckPath(drv_no, oldPath, PATH_EXCEPT_LAST, &h_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, ERR_FILE_NOT_EXIST);

        s_smErr = sm_CheckPath(drv_no, newPath, PATH_EXCEPT_LAST, &h_new_dir);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, ERR_FILE_EXIST);

        if(h_dir != h_new_dir)
                FAT_RETURN(drv_no, ERR_INVALID_PARAM);

        /* check if newPath exist. if so, fail */
        s_smErr = sm_CheckPath(drv_no, newPath, PATH_FULL, &h_file);
        if (s_smErr == SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        sm_ExtractLastName(oldPath, oldName);
        sm_ExtractLastName(newPath, newName);

        /* check if oldPath exist. if not, fail */
        /* retrieve old file or directory info */
        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)oldName, fileInfo, &cluster, &offset);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* delete old file */
        s_smErr = sm_DeleteFromDirEntry(drv_no, h_dir, oldName);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* create new directory entry */
        sStat.attr = fileInfo[11];
        sStat.cluster = fileInfo[26] | ((udword)fileInfo[27] << 8);
        sStat.size = fileInfo[28] | ((udword)fileInfo[29] << 8) | ((udword)fileInfo[30] << 16) | ((udword)fileInfo[31] << 24);
        sStat.time.year = 1980 + ((fileInfo[25] >> 1) & 0x7f);
        sStat.time.month = ((fileInfo[25] << 3) & 0x08) | ((fileInfo[24] >> 5) & 0x07);
        sStat.time.day = fileInfo[24] & 0x1f;
        sStat.time.hour = (fileInfo[23] >> 3) & 0x1f;
        sStat.time.min = ((fileInfo[23] << 3) & 0x38) | ((fileInfo[22] >> 5) & 0x07);
        sStat.time.sec = fileInfo[22] & 0x1f;
        sStat.time.msec = 0;

#ifdef  LONG_FILE_NAME_ENABLE
        p_unicode = (uword*)SM_MALLOC(MAX_FILE_NAME_LEN+2);
        if(p_unicode == NULL)
        {
                FAT_RETURN(drv_no, ERR_OUT_OF_MEMORY);
        }

        if(sm_ConvertToFATName(newName, fat_name))
        {
                SM_MBS_TO_UNICODE(p_unicode, newName, 0);
                s_smErr = sm_AddToDirEntry(drv_no, h_dir, newName, p_unicode, &sStat, 1, cluster, offset);
        }
        else
                s_smErr = sm_AddToDirEntry(drv_no, h_dir, newName, NULL, &sStat, 1, cluster, offset);

        SM_FREE(p_unicode);
#else
        s_smErr = sm_AddToDirEntry(drv_no, h_dir, newName, NULL, &sStat, 1, cluster, offset);
#endif

        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        FAT_RETURN(drv_no, SM_OK);
}



/**********
 * Function: smNoFATUpdate
 * Remarks:
 *      - prohibits that internal routines update FAT table
 *      - this is helpful for speed-up when processing(create, delete) multiple files
 * Parameters
 *      . p_vol_name: drive name
 * Notes :
 *      - This function must always be followed by smFATUpdate()
 **********/
SM_EXPORT ERR_CODE smNoFATUpdate(const smchar* p_volumn_name)
{
        udword drv_no;
        ubyte *p_vol_name = (ubyte *)p_volumn_name;

        drv_no = sm_GetDrvNo(p_vol_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        bNoFATUpdate[drv_no] = TRUE;

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smFATUpdate
 * Remarks:
 *      - cancels the NoFATUpdate mode started by smNoFATUpdate
 *      - updates FAT table by calling sm_FATUpdate()
 * Parameters
 *      . p_vol_name: drive name
 **********/
SM_EXPORT ERR_CODE smFATUpdate(const smchar* p_volumn_name)
{
        udword drv_no;
        ubyte *p_vol_name = (ubyte *)p_volumn_name;

        drv_no = sm_GetDrvNo(p_vol_name);
        if (drv_no >= MAX_DRIVE)
                return ERR_INVALID_PARAM;

        s_smErr = sm_PreFAT(drv_no);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        bNoFATUpdate[drv_no] = FALSE;
        sm_FATUpdate(drv_no);

        FAT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smRegisterCallBack
 * Remarks:
 *      - register callback functions
 * Parameters
 *      . drv_no: drive number that is initialized in configuration file
 *      . func_id: CALLBACK_CARD_INSERT / CALLBACK_CARD_EJECT
 *      . pFunc: pointer of function that returns void, and has parameter
 *              of udword
 **********/
SM_EXPORT ERR_CODE smRegisterCallBack(udword drv_no, udword func_id, void (*pFunc)(udword))
{
        if (func_id == CALLBACK_CARD_INSERT)
                s_pfCBInserted = pFunc;
        else if (func_id == CALLBACK_CARD_EJECT)
                s_pfCBEjected = pFunc;
        else
                return ERR_INVALID_PARAM;

        return SM_OK;
}


/**********
 * Function: smUnregisterCallBack
 * Remarks:
 *      - unregister callback functions
 * Parameters
 *      . drv_no: drive number that is initialized in configuration file
 *      . func_id: CALLBACK_CARD_INSERT / CALLBACK_CARD_EJECT
 **********/
SM_EXPORT ERR_CODE smUnregisterCallBack(udword drv_no, udword func_id)
{
        if (func_id == CALLBACK_CARD_INSERT)
                s_pfCBInserted = NULL;
        else if (func_id == CALLBACK_CARD_EJECT)
                s_pfCBEjected = NULL;
        else
                return ERR_INVALID_PARAM;

        return SM_OK;
}


/**********
 * Function: smGetVersionInfo
 * Remarks:
 *      - get version info and version number
 *      - ex) Version 2.10 =>
 *                      acVersion= "SmartMedia File System v2.10", iMajorVersion=2, iMinorVersion=10
 * Parameters
 *      . p_version: pointer of sVERSION structure
 **********/
SM_EXPORT ERR_CODE smGetVersionInfo(sVERSION *p_version)
{
        udword len;
        udword i;

        SM_STRCPY(p_version->acInfo, "SmartMedia File System v");
        len = (udword)SM_STRLEN("SmartMedia File System v");
        SM_STRCPY((char*)(p_version->acInfo + len), acSMFSVersion);

        len = (udword)SM_STRLEN(acSMFSVersion);
        for(i=0; i<len; i++)
        {
                if(*(acSMFSVersion+i) == '.')
                        break;
        }

        if(i > 1)
                p_version->iMajorVersion = ((int)(acSMFSVersion[i-2] - '0') * 10) + (int)(acSMFSVersion[i-1] - '0');
        else if(i == 1)
                p_version->iMajorVersion =  (int)(acSMFSVersion[i-1] - '0');
        else
                p_version->iMajorVersion = 0;

        if(len + 1 - i == 2)    /* 2 character for minor version */
                p_version->iMinorVersion = ((int)(acSMFSVersion[i+1] - '0') * 10) + (int)(acSMFSVersion[i+2] - '0');
        else if(len + 1 - i == 1)       /* 1 character for minor version */
                p_version->iMinorVersion = (int)(acSMFSVersion[i+1] - '0');
        else
                p_version->iMinorVersion = 0;

        return SM_OK;
}


/**********
 * Function: SMCBCardInserted
 * Remarks:
 *      - callback function that is called when a SmartMedia is inserted
 *      - function provided by higher application could be called here
 * Parameters
 *      . drv_no: drive number that is initialized in configuration file
 **********/
void SMCBCardInserted(udword drv_no)
{
        if (s_pfCBInserted != NULL)
                s_pfCBInserted(drv_no);
}


/**********
 * Function: SMCBCardEjected
 * Remarks:
 *      - callback function that is called when a SmartMedia is ejected
 *      - function provided by higher application could be called here
 * Parameters
 *      . drv_no: drive number that is initialized in configuration file
 **********/
void SMCBCardEjected(udword drv_no)
{
        if (s_pfCBEjected != NULL)
                s_pfCBEjected(drv_no);

        s_fatFlag[drv_no] = NOT_INITIALIZED;
}




/**********
 * Local Function definitions
 **********/


/**********
 * Function: smFATInitDefaultValue
 * Remarks:
 *      - called when the file system is initialized
 **********/
void sm_FATInitDefaultValue(void)
{
        udword i;

        for (i = 0; i < MAX_DRIVE; ++i)
        {
                s_fatFlag[i] = NOT_INITIALIZED;
                if (s_fat[i] != NULL)
                {
                        SM_FREE(s_fat[i]);
                        s_fat[i] = NULL;
                }
                while (s_openedFile[i] != NULL)
                {
                        sm_DeleteFromOpenedFileList(i, s_openedFile[i]->cluster);
                }
        }

        s_pfCBInserted = NULL;
        s_pfCBEjected = NULL;
}


/**********
 * Function: sm_FATInit
 * Remarks:
 *      - called when the udFATFlag is not initialized and FAT API is called
 **********/
ERR_CODE sm_FATInit(udword drv_no)
{
        udword fat_bytes;
        udword offset;
        udword total_clusters;
        udword i, j;
        udword pbr_offset;
        const sDEV_INFO* s_devinfo;
        ubyte* acFATBuf;
        udword sector_plus;

        s_fatFlag[drv_no] = NOT_INITIALIZED;

        /* opened file list clear. */
        if (s_openedFile[drv_no] != NULL)
                s_openedFile[drv_no] = NULL;

        /*
         * MBR table initial
         */
        acFATBuf = SMB_ALLOC_BUF();
        if (!acFATBuf)
                return ERR_OUT_OF_MEMORY;

        s_smErr = smlReadSector(drv_no, 0, 0, acFATBuf, SECTOR_SIZE);
        if (s_smErr != SM_OK)
        {
                SMB_FREE_BUF(acFATBuf);
                if (s_smErr == ERR_INVALID_BLOCK)
                        return ERR_NOT_FORMATTED;
                else
                        return s_smErr;
        }

        pbr_offset = 0x1be;

        s_smInfo[drv_no].mbr.signature = ((uword)acFATBuf[0x1fe] << 8) | acFATBuf[0x1ff];               /* 0x55aa by big_endian */
        s_smInfo[drv_no].mbr.partition_entry[0].def_boot = acFATBuf[pbr_offset + 0];
        s_smInfo[drv_no].mbr.partition_entry[0].start_cyl = ((udword)(acFATBuf[pbr_offset + 2] & 0xc0) << 2) | acFATBuf[pbr_offset + 3];
        s_smInfo[drv_no].mbr.partition_entry[0].start_head = acFATBuf[pbr_offset + 1];
        s_smInfo[drv_no].mbr.partition_entry[0].start_sector = (acFATBuf[pbr_offset + 2] & 0x3f) - 1;
        s_smInfo[drv_no].mbr.partition_entry[0].pt_type = acFATBuf[pbr_offset + 4];
        s_smInfo[drv_no].mbr.partition_entry[0].end_cyl = ((udword)(acFATBuf[pbr_offset + 6] & 0xc0) << 2) | acFATBuf[pbr_offset + 7];
        s_smInfo[drv_no].mbr.partition_entry[0].end_head = acFATBuf[pbr_offset + 5];
        s_smInfo[drv_no].mbr.partition_entry[0].end_sector = (acFATBuf[pbr_offset + 6] & 0x3f) - 1;
        s_smInfo[drv_no].mbr.partition_entry[0].start_lba_sector = ((udword)acFATBuf[pbr_offset + 11] << 24) | ((udword)acFATBuf[pbr_offset + 10] << 16)
                                                                        | ((udword)acFATBuf[pbr_offset + 9] << 8) | acFATBuf[pbr_offset + 8];
        s_smInfo[drv_no].mbr.partition_entry[0].total_available_sectors = ((udword)acFATBuf[pbr_offset + 15] << 24)     | ((udword)acFATBuf[pbr_offset + 14] << 16)
                                                                        | ((udword)acFATBuf[pbr_offset + 13] << 8) | acFATBuf[pbr_offset + 12];

        /* MBR check recommended by SSFDC forum */
        s_smErr = smlGetDeviceInfo(drv_no, &s_devinfo);
        if (s_smErr != SM_OK)
        {
                SMB_FREE_BUF(acFATBuf);
                return s_smErr;
        }

        if ((s_smInfo[drv_no].mbr.signature != 0x55aa) || (s_smInfo[drv_no].mbr.partition_entry[0].start_lba_sector     >= s_devinfo->allS))
        {
                SMB_FREE_BUF(acFATBuf);
                return ERR_INVALID_MBR;
        }


        /*
         * PBR table initial
         */
        s_smErr = smlReadSector(drv_no, s_smInfo[drv_no].mbr.partition_entry[0].start_lba_sector, 0, acFATBuf, SECTOR_SIZE);
        if (s_smErr != SM_OK)
        {
                SMB_FREE_BUF(acFATBuf);
                return s_smErr;
        }

        SM_MEMCPY(s_smInfo[drv_no].pbr.oem_name, &acFATBuf[0x3], 8);
        s_smInfo[drv_no].pbr.oem_name[8] = 0;
        s_smInfo[drv_no].pbr.drive_num = acFATBuf[0x24];
        s_smInfo[drv_no].pbr.reserved = acFATBuf[0x25];
        s_smInfo[drv_no].pbr.ext_boot_signature = acFATBuf[0x26];
        s_smInfo[drv_no].pbr.vol_id = ((udword)acFATBuf[0x2a] << 24) | ((udword)acFATBuf[0x29] << 16) | ((udword)acFATBuf[0x28] << 8) | acFATBuf[0x27];
        SM_MEMCPY(s_smInfo[drv_no].pbr.vol_label, &acFATBuf[0x2b], 11);
        s_smInfo[drv_no].pbr.vol_label[11] = 0;
        if (SM_MEMCMP(&acFATBuf[0x36], "FAT12   ", 8) == 0)
                s_smInfo[drv_no].pbr.file_sys_type = FS_FAT12;
        else if (SM_MEMCMP(&acFATBuf[0x36], "FAT16   ", 8) == 0)
                s_smInfo[drv_no].pbr.file_sys_type = FS_FAT16;
        else
        {
                s_smInfo[drv_no].pbr.file_sys_type = FS_UNKNOWN;
                SMB_FREE_BUF(acFATBuf);
                return ERR_INVALID_PBR;
        }
        s_smInfo[drv_no].pbr.signature = ((uword)acFATBuf[0x1fe] << 8) | acFATBuf[0x1ff];
        s_smInfo[drv_no].pbr.bpb.bytes_per_sector = ((udword)acFATBuf[0xc] << 8) | acFATBuf[0xb];
        s_smInfo[drv_no].pbr.bpb.sectors_per_cluster = acFATBuf[0xd];
        s_smInfo[drv_no].pbr.bpb.reserved_sectors = ((udword)acFATBuf[0xf] << 8) | acFATBuf[0xe];
        s_smInfo[drv_no].pbr.bpb.fat_num = acFATBuf[0x10];
        s_smInfo[drv_no].pbr.bpb.root_entry     = ((udword)acFATBuf[0x12] << 8) | acFATBuf[0x11];
        s_smInfo[drv_no].pbr.bpb.total_sectors = ((udword)acFATBuf[0x14] << 8) | acFATBuf[0x13];
        s_smInfo[drv_no].pbr.bpb.format_type = acFATBuf[0x15];
        s_smInfo[drv_no].pbr.bpb.sectors_in_fat = ((udword)acFATBuf[0x17] << 8) | acFATBuf[0x16];
        s_smInfo[drv_no].pbr.bpb.sectors_per_track = ((udword)acFATBuf[0x19] << 8) | acFATBuf[0x18];
        s_smInfo[drv_no].pbr.bpb.head_num = ((udword)acFATBuf[0x1b] << 8) | acFATBuf[0x1a];
        s_smInfo[drv_no].pbr.bpb.hidden_sectors = ((udword)acFATBuf[0x1f] << 24) | ((udword)acFATBuf[0x1e] << 16)
                                                                                                | ((udword)acFATBuf[0x1d] << 8) | acFATBuf[0x1c];
        s_smInfo[drv_no].pbr.bpb.huge_sectors = ((udword)acFATBuf[0x23] << 24) | ((udword)acFATBuf[0x22] << 16)
                                                                                                | ((udword)acFATBuf[0x21] << 8) | acFATBuf[0x20];

        if (s_smInfo[drv_no].pbr.bpb.total_sectors == 0)
                s_smInfo[drv_no].pbr.bpb.total_sectors = s_smInfo[drv_no].pbr.bpb.huge_sectors;

        /* PBR check recommended by SSFDC forum */
        if ((s_smInfo[drv_no].pbr.signature != 0x55aa)
                        || (s_smInfo[drv_no].pbr.bpb.bytes_per_sector != 512)
/*                      || (s_smInfo[drv_no].pbr.bpb.sectors_per_cluster */
/*                              != s_devinfo->SpB */
/*                      || (s_smInfo[drv_no].pbr.bpb.reserved_sectors == 0) */
                        || ((s_smInfo[drv_no].pbr.bpb.fat_num < 1)
                                        || (s_smInfo[drv_no].pbr.bpb.fat_num > 16))
                        || (s_smInfo[drv_no].pbr.bpb.root_entry == 0)
                        || (s_smInfo[drv_no].pbr.bpb.format_type != 0xf8))
        {
                SMB_FREE_BUF(acFATBuf);
                return ERR_INVALID_PBR;
        }

        /* Here, only up to 2 FATs are supported (1-16, in SSFDC) */
        if (s_smInfo[drv_no].pbr.bpb.fat_num > 2)
        {
                SMB_FREE_BUF(acFATBuf);
                return ERR_INVALID_PBR;
        }

        /*
         * FAT table initial
         */
        if (s_fat[drv_no] != NULL)
                SM_FREE(s_fat[drv_no]);

        total_clusters = s_smInfo[drv_no].pbr.bpb.total_sectors / s_smInfo[drv_no].pbr.bpb.sectors_per_cluster;

        s_fatTableIndexCount[drv_no] = total_clusters + 2;
        s_fat[drv_no] = (uword*)SM_MALLOC(s_fatTableIndexCount[drv_no] * sizeof(uword));
        if (s_fat[drv_no] == NULL)
        {
                SMB_FREE_BUF(acFATBuf);
                return ERR_INTERNAL;
        }

        if (s_smInfo[drv_no].pbr.file_sys_type == FS_FAT12)
        {
                fat_bytes = (total_clusters + 2) * 3 / 2;
                fat_bytes += (s_smInfo[drv_no].pbr.bpb.bytes_per_sector - (fat_bytes % s_smInfo[drv_no].pbr.bpb.bytes_per_sector));
        }
        else
        {
                fat_bytes = (total_clusters + 2) * 2;
                fat_bytes += (s_smInfo[drv_no].pbr.bpb.bytes_per_sector - (fat_bytes % s_smInfo[drv_no].pbr.bpb.bytes_per_sector));
        }

        s_fat1StartSector[drv_no] = s_smInfo[drv_no].mbr.partition_entry[0].start_lba_sector + 1;
        s_fat1Sectors[drv_no] = fat_bytes / s_smInfo[drv_no].pbr.bpb.bytes_per_sector;

        /* check if the first FAT ID is valid */
        sector_plus = 0;
        for (i = 0; i < s_smInfo[drv_no].pbr.bpb.fat_num; ++i)
        {
                if (smlReadSector(drv_no, s_fat1StartSector[drv_no], 0, acFATBuf, SECTOR_SIZE) != SM_OK)
                {
                        SMB_FREE_BUF(acFATBuf);
                        return ERR_INTERNAL;
                }

                if ((acFATBuf[0] != 0xf8) || (acFATBuf[1] != 0xff) || (acFATBuf[2] != 0xff))
                {
                        sector_plus += s_fat1Sectors[drv_no] * SECTOR_SIZE;
                        continue;
                }
                else
                        break;
        }

        if (i >= s_smInfo[drv_no].pbr.bpb.fat_num)
        {
                SMB_FREE_BUF(acFATBuf);
                return ERR_INVALID_FAT;
        }

        if (s_smInfo[drv_no].pbr.file_sys_type == FS_FAT12)
        {
                udword remained;
                udword unreadCount;

                offset = 0;
                remained = 0;
                unreadCount = 0;

                for (i = 0; i < s_fat1Sectors[drv_no]; ++i)
                {
                        if (unreadCount)
                        {
                                if (smlReadSector(drv_no, s_fat1StartSector[drv_no] + sector_plus + i - 1, SECTOR_SIZE - unreadCount, acFATBuf + remained, unreadCount) != SM_OK)
                                {
                                        SMB_FREE_BUF(acFATBuf);
                                        return ERR_INTERNAL;
                                }
                        }

                        if (smlReadSector(drv_no, s_fat1StartSector[drv_no] + sector_plus + i, 0, acFATBuf + remained + unreadCount, SECTOR_SIZE - remained - unreadCount) != SM_OK)
                        {
                                SMB_FREE_BUF(acFATBuf);
                                return ERR_INTERNAL;
                        }

                        unreadCount += remained;
                        if (unreadCount >= SECTOR_SIZE)
                        {
                                --i;
                                unreadCount -= SECTOR_SIZE;
                        }

                        for (j = 0; j <= SECTOR_SIZE - 3; j += 3)
                        {
                                SET_FAT_TBL(drv_no, offset,     ((uword)(acFATBuf[j + 1] & 0xf) << 8) | acFATBuf[j]);
                                ++offset;

                                /*
                                 * maximum offset value is not checked here because
                                 * the value can not be odd
                                 */

                                SET_FAT_TBL(drv_no, offset, (acFATBuf[j + 2] << 4) | ((acFATBuf[j + 1] & 0xf0) >> 4));
                                ++offset;

                                /* make pFAT value to be predefined value */
                                /*      buf don't touch the FAT ID area */
                                if (offset >= 4)
                                {
                                        if (GET_FAT_TBL(drv_no, offset - 2) >= 0xff0)
                                        {
                                                if ((GET_FAT_TBL(drv_no, offset - 2) >= 0xff8) && (GET_FAT_TBL(drv_no, offset - 2) <= 0xfff))
                                                        SET_FAT_TBL(drv_no, offset - 2, LAST_CLUSTER);
                                                else if (GET_FAT_TBL(drv_no, offset - 2) == 0xff7)
                                                        SET_FAT_TBL(drv_no, offset - 2, DEFECTIVE_CLUSTER);
                                        }
                                        if (GET_FAT_TBL(drv_no, offset - 1) >= 0xff0)
                                        {
                                                if ((GET_FAT_TBL(drv_no, offset - 1) >= 0xff8) && (GET_FAT_TBL(drv_no, offset - 1) <= 0xfff))
                                                        SET_FAT_TBL(drv_no, offset - 1, LAST_CLUSTER);
                                                else if (GET_FAT_TBL(drv_no, offset - 1) == 0xff7)
                                                        SET_FAT_TBL(drv_no, offset - 1, DEFECTIVE_CLUSTER);
                                        }
                                }

                                if (offset >= total_clusters + 2)
                                        break;
                        }

                        remained = SECTOR_SIZE - j;

                        SM_MEMCPY(acFATBuf,     acFATBuf + SECTOR_SIZE - remained, remained);
                }
        }
        else    /* FS_FAT16 */
        {
                offset = 0;

                for (i = 0; i < s_fat1Sectors[drv_no]; ++i)
                {
                        s_smErr = smlReadSector(drv_no, s_fat1StartSector[drv_no] + sector_plus + i, 0, acFATBuf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(acFATBuf);
                                return s_smErr;
                        }

                        for (j = 0; j <= SECTOR_SIZE - 2; j += 2)
                        {
                                SET_FAT_TBL(drv_no, offset,     ((uword)acFATBuf[j + 1] << 8)  | acFATBuf[j]);
                                ++offset;

                                /* make pFAT value to be predefined value */
                                /*      buf don't touch the FAT ID area */
                                if (offset >= 3)
                                {
                                        if (GET_FAT_TBL(drv_no, offset - 1) >= 0xfff0)
                                        {
                                                if ((GET_FAT_TBL(drv_no, offset - 1) >= 0xfff8) && (GET_FAT_TBL(drv_no, offset - 1)     <= 0xffff))
                                                        SET_FAT_TBL(drv_no, offset - 1, LAST_CLUSTER);
                                                else if(GET_FAT_TBL(drv_no, offset - 1) == 0xfff7)
                                                        SET_FAT_TBL(drv_no, offset - 1, DEFECTIVE_CLUSTER);
                                        }
                                }

                                if (offset >= total_clusters + 2)
                                        break;
                        }
                }
        }
        SMB_FREE_BUF(acFATBuf);

        /*
         * Root start sector initial
         */
        s_rootStartSector[drv_no] = s_fat1StartSector[drv_no] + s_fat1Sectors[drv_no] * s_smInfo[drv_no].pbr.bpb.fat_num;
        s_rootSectors[drv_no] = (s_smInfo[drv_no].pbr.bpb.root_entry * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
        /* root start + root size */
        s_clusterStartSector[drv_no] = s_rootStartSector[drv_no] + s_rootSectors[drv_no];

        /*
         * cache size initial (cluster size if large memory,
         * otherwise SECTOR_SIZE)
         */
#ifndef WRITE_CACHE_DISABLE
        #if (MEMORY_LEVEL >= MEMORY_VERY_LARGE)
                s_cacheSize[drv_no]     = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
        #else
                s_cacheSize[drv_no] = SECTOR_SIZE;
        #endif
#endif
        /* initial flag */
        s_fatFlag[drv_no] = INITIALIZED;

        return SM_OK;
}


/**********
 * Function: sm_PreFAT
 * Remarks:
 *      - called at the first of FAT APIs
 **********/
ERR_CODE sm_PreFAT(udword drv_no)
{
#ifdef SM_MULTITASK
        SM_LOCK(drv_no);
#endif

        if (s_fatFlag[drv_no] != INITIALIZED)
        {
                s_smErr = sm_FATInit(drv_no);
                if (s_smErr != SM_OK)
                {
                        if ((s_smErr == ERR_INVALID_MBR) || (s_smErr == ERR_INVALID_PBR) || (s_smErr == ERR_INVALID_FAT))
                                return ERR_NOT_FORMATTED;
                        else
                                return s_smErr;
                }
        }

        return SM_OK;
}


/**********
 * Function: sm_PostFAT
 * Remarks:
 *      - called at the end of FAT APIs
 **********/
ERR_CODE sm_PostFAT(udword drv_no)
{
#ifdef SM_MULTITASK
        SM_UNLOCK(drv_no);
#endif

        return SM_OK;
}


/**********
 * Function: sm_WriteSMInfo
 * Remarks:
 *      - write MBR & PBR into the specified location
 *      - called from smFormatVol()
 * Parameters:
 *      . p_sm_info: pointer of structure that contains MBR & PBR
 **********/
ERR_CODE sm_WriteSMInfo(udword drv_no, const sSM_INFO* p_sm_info)
{
        udword pbr_block, pbr_sector;
        udword changed_pblock;
        udword i;
        udword write_size;
        ubyte* acFATBuf;

        /* prepare MBR block */
        s_smErr = smlGetNewSpecifiedBlock(drv_no, 0, &changed_pblock);
        if (s_smErr != SM_OK)
                return s_smErr;

        if (changed_pblock != UNUSED_LBLOCK)
                smlDiscardPhysBlock(drv_no, changed_pblock);

        /* prepare PBR block */
        pbr_block = p_sm_info->mbr.partition_entry[0].start_lba_sector / p_sm_info->pbr.bpb.sectors_per_cluster;
        pbr_sector = p_sm_info->mbr.partition_entry[0].start_lba_sector % p_sm_info->pbr.bpb.sectors_per_cluster;

        if (pbr_block != 0)
        {
                s_smErr = smlGetNewSpecifiedBlock(drv_no, pbr_block, &changed_pblock);
                if (s_smErr != SM_OK)
                        return s_smErr;

                if (changed_pblock != UNUSED_LBLOCK)
                        smlDiscardPhysBlock(drv_no, changed_pblock);
        }

        /*
         * write MBR
         */
        acFATBuf = SMB_ALLOC_BUF();
        if (!acFATBuf)
                return ERR_OUT_OF_MEMORY;

        SM_MEMSET(acFATBuf, 0, SECTOR_SIZE);

        /*
         * write dummy data to rest sectors(1th ~ end)
         * in 0th block for spare area data write
         */
        write_size = SECTOR_SIZE * p_sm_info->pbr.bpb.sectors_per_cluster;
        for (i = SECTOR_SIZE; i < write_size; i += SECTOR_SIZE)
        {
                s_smErr = smlWriteBlock(drv_no, 0, i, acFATBuf, SECTOR_SIZE);
                if (s_smErr != SM_OK)
                {
                        SMB_FREE_BUF(acFATBuf);
                        return s_smErr;
                }
        }

        /* default boot partition */
        acFATBuf[446] = (ubyte)p_sm_info->mbr.partition_entry[0].def_boot;
        /* start head */
        acFATBuf[447] = (ubyte)p_sm_info->mbr.partition_entry[0].start_head;
        /* start sector */
        acFATBuf[448] = (ubyte)(((p_sm_info->mbr.partition_entry[0].start_cyl >> 2)     & 0xc0)
                                        | (p_sm_info->mbr.partition_entry[0].start_sector & 0x3f));
        /* start cylinder */
        acFATBuf[449] = (ubyte)p_sm_info->mbr.partition_entry[0].start_cyl;
        /* partition type (1=?) */
        acFATBuf[450] = (ubyte)p_sm_info->mbr.partition_entry[0].pt_type;
        /* end head */
        acFATBuf[451] = (ubyte)p_sm_info->mbr.partition_entry[0].end_head;
        /* end sector */
        acFATBuf[452] = (ubyte)(((p_sm_info->mbr.partition_entry[0].end_cyl >> 2) & 0xc0)
                                        | (p_sm_info->mbr.partition_entry[0].end_sector & 0x3f));
        /* end cyliner */
        acFATBuf[453] = (ubyte)p_sm_info->mbr.partition_entry[0].end_cyl;
        /* 4 byte start lba sector (little endian) */
        acFATBuf[454] = (ubyte)p_sm_info->mbr.partition_entry[0].start_lba_sector;
        acFATBuf[455] = (ubyte)(p_sm_info->mbr.partition_entry[0].start_lba_sector >> 8);
        acFATBuf[456] = (ubyte)(p_sm_info->mbr.partition_entry[0].start_lba_sector >> 16);
        acFATBuf[457] = (ubyte)(p_sm_info->mbr.partition_entry[0].start_lba_sector >> 24);
        /* 4 byte sector numbers (little endian) */
        acFATBuf[458] = (ubyte)p_sm_info->mbr.partition_entry[0].total_available_sectors;
        acFATBuf[459] = (ubyte)(p_sm_info->mbr.partition_entry[0].total_available_sectors >> 8);
        acFATBuf[460] = (ubyte)(p_sm_info->mbr.partition_entry[0].total_available_sectors >> 16);
        acFATBuf[461] = (ubyte)(p_sm_info->mbr.partition_entry[0].total_available_sectors >> 24);

        /* 2byte signature(0x55aa) */
        acFATBuf[510] = (ubyte)(p_sm_info->mbr.signature >> 8);
        acFATBuf[511] = (ubyte)(p_sm_info->mbr.signature);

        /* write MBR to the 0th lba sector */
        s_smErr = smlWriteBlock(drv_no, 0, 0, acFATBuf, SECTOR_SIZE);
        if (s_smErr != SM_OK)
        {
                SMB_FREE_BUF(acFATBuf);
                return s_smErr;
        }


        /*
         * write PBR
         */
        SM_MEMSET(acFATBuf, 0, SECTOR_SIZE);

        /*
         * write dummy data to former sectors(0th ~ pbr_sector-1) in
         * pbr_block for spare area data write
         */
        write_size = SECTOR_SIZE * pbr_sector;
        for (i = 0; i < write_size; i += SECTOR_SIZE)
        {
                s_smErr = smlWriteBlock(drv_no, pbr_block, i, acFATBuf, SECTOR_SIZE);
                if (s_smErr != SM_OK)
                {
                        SMB_FREE_BUF(acFATBuf);
                        return s_smErr;
                }
        }

        acFATBuf[0] = 0xe9;             /* JMP instruction (?) */
        acFATBuf[1] = 0;
        acFATBuf[2] = 0;

        /* OEMName and Version */
        SM_MEMCPY(&acFATBuf[3], &p_sm_info->pbr.oem_name[0], 8);

        /* BIOS Parameter Block */
                /* 2byte sector size */
        acFATBuf[11] = (ubyte)p_sm_info->pbr.bpb.bytes_per_sector;
        acFATBuf[12] = (ubyte)(p_sm_info->pbr.bpb.bytes_per_sector >> 8);
                /* Sector/Cluster */
        acFATBuf[13] = (ubyte)p_sm_info->pbr.bpb.sectors_per_cluster;
                /* 2byte reserved sectors (?) */
        acFATBuf[14] = (ubyte)p_sm_info->pbr.bpb.reserved_sectors;
        acFATBuf[15] = (ubyte)(p_sm_info->pbr.bpb.reserved_sectors >> 8);
                /* numbers of FAT */
        acFATBuf[16] = (ubyte)p_sm_info->pbr.bpb.fat_num;
                /* 2byte RootDirEntries */
        acFATBuf[17] = (ubyte)p_sm_info->pbr.bpb.root_entry;
        acFATBuf[18] = (ubyte)(p_sm_info->pbr.bpb.root_entry >> 8);
                /* total sector numbers */
        acFATBuf[19] = (ubyte)p_sm_info->pbr.bpb.total_sectors;
        acFATBuf[20] = (ubyte)(p_sm_info->pbr.bpb.total_sectors >> 8);
                /* MedialDByte => hard disk, any size */
        acFATBuf[21] = (ubyte)p_sm_info->pbr.bpb.format_type;
                /* 2byte NumFATSectors */
        acFATBuf[22] = (ubyte)(p_sm_info->pbr.bpb.sectors_in_fat);
        acFATBuf[23] = (ubyte)(p_sm_info->pbr.bpb.sectors_in_fat >> 8);
                /* 2byte SectorsPerTrack (== SpH (?)) */
        acFATBuf[24] = (ubyte)(p_sm_info->pbr.bpb.sectors_per_track);
        acFATBuf[25] = (ubyte)(p_sm_info->pbr.bpb.sectors_per_track >> 8);
                /* 2byte NumHeads (== HpC) */
        acFATBuf[26] = (ubyte)(p_sm_info->pbr.bpb.head_num);
        acFATBuf[27] = (ubyte)(p_sm_info->pbr.bpb.head_num >> 8);
                /* 4byte HiddenSectors */
        acFATBuf[28] = (ubyte)(p_sm_info->pbr.bpb.hidden_sectors);
        acFATBuf[29] = (ubyte)(p_sm_info->pbr.bpb.hidden_sectors >> 8);
        acFATBuf[30] = (ubyte)(p_sm_info->pbr.bpb.hidden_sectors >> 16);
        acFATBuf[31] = (ubyte)(p_sm_info->pbr.bpb.hidden_sectors >> 24);
                /* huge total sector numbers */
        acFATBuf[32] = (ubyte)(p_sm_info->pbr.bpb.huge_sectors);
        acFATBuf[33] = (ubyte)(p_sm_info->pbr.bpb.huge_sectors >> 8);
        acFATBuf[34] = (ubyte)(p_sm_info->pbr.bpb.huge_sectors >> 16);
        acFATBuf[35] = (ubyte)(p_sm_info->pbr.bpb.huge_sectors >> 24);
        /* BIOS Parameter Block End */

        /* DriveNumber */
        acFATBuf[36] = (ubyte)(p_sm_info->pbr.drive_num);
        /* ExtBootSignature */
        acFATBuf[38] = (ubyte)(p_sm_info->pbr.ext_boot_signature);
        /* 4byte VolumeID or Serial Number */
        acFATBuf[39] = (ubyte)(p_sm_info->pbr.vol_id);
        acFATBuf[40] = (ubyte)(p_sm_info->pbr.vol_id >> 8);
        acFATBuf[41] = (ubyte)(p_sm_info->pbr.vol_id >> 16);
        acFATBuf[42] = (ubyte)(p_sm_info->pbr.vol_id >> 24);
        /* 11byte VolumeLabel */
        SM_MEMCPY(&acFATBuf[43], p_sm_info->pbr.vol_label, 11);
        /* 8byte FileSysType */
        if (p_sm_info->pbr.file_sys_type == FS_FAT12)
                SM_MEMCPY(&acFATBuf[54], "FAT12   ", 8);
        else
                SM_MEMCPY(&acFATBuf[54], "FAT16   ", 8);

        /* 2byte Signature */
        acFATBuf[510] = (ubyte)(p_sm_info->pbr.signature >> 8);
        acFATBuf[511] = (ubyte)(p_sm_info->pbr.signature);

        /* write to the PBR_SECTOR */
        s_smErr = smlWriteBlock(drv_no, pbr_block, pbr_sector * SECTOR_SIZE, acFATBuf, SECTOR_SIZE);
        if (s_smErr != SM_OK)
        {
                SMB_FREE_BUF(acFATBuf);
                return s_smErr;
        }

        SMB_FREE_BUF(acFATBuf);
        return SM_OK;
}


/**********
 * Function: sm_UpdateBlock
 * Remarks:
 *      - overwrites the contents of the old_block with the contents of
 *              the p_buf
 *              from "offset" to "offset+count"
 * Parameters:
 *      . new_block: empty block that should be filled with updated contents
 *      . old_pblock: physical block that has original contents
 *      . offset: offset from the block_begin. the contents of the p_buf
 *              will be overwritten from this offset
 *      . count: the amount of updating contents
 *      . p_buf: pointer that indicates updating contents
 **********/
ERR_CODE sm_UpdateBlock(udword drv_no, udword new_block, udword old_pblock, udword offset, udword count, const ubyte* p_buf, bool touchAllSectors)
{
        ubyte* temp_buf;
        ubyte i;
        udword read_offset = 0;
        udword buf_offset = 0;
        udword write_count;
        const sDEV_INFO* devInfo;

        s_smErr = smlGetDeviceInfo(drv_no, &devInfo);
        if (s_smErr != SM_OK)
                return s_smErr;

        temp_buf = SMB_ALLOC_BUF();
        if (!temp_buf)
                return ERR_OUT_OF_MEMORY;

        for (i = 0; i < devInfo->SpB; ++i, read_offset += SECTOR_SIZE)
        {
                bool doWrite = FALSE;

                /* read 1 sector */
                if (old_pblock != UNUSED_LBLOCK && !smlIsPhysSectorErased(drv_no, old_pblock, i))
                {
                        doWrite = TRUE;

                        s_smErr = smlReadPhysBlock(drv_no, old_pblock, read_offset, temp_buf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(temp_buf);
                                return s_smErr;
                        }
                }

                /*
                 * check if the updating contents is within this read sector.
                 *      if so, updates the read contents with the contents of p_buf,
                 *      and then changes the offset, p_buf offset, count values
                 */
                if (count && (offset < read_offset + SECTOR_SIZE))
                {
                        doWrite = TRUE;

                        write_count = (read_offset + SECTOR_SIZE - offset > count) ? count : read_offset + SECTOR_SIZE - offset;
                        SM_MEMCPY(temp_buf + (offset - read_offset), p_buf + buf_offset, write_count);

                        offset += write_count;
                        buf_offset += write_count;
                        count -= write_count;
                }

                /* write the updated sector */
                if (doWrite || touchAllSectors)
                {
                        s_smErr = smlWriteBlock(drv_no, new_block, read_offset, temp_buf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(temp_buf);
                                return s_smErr;
                        }
                }
        }

        SMB_FREE_BUF(temp_buf);
        return SM_OK;
}


/**********
 * Function: sm_GetDrvNo
 * Remarks:
 *      - get drive number(0, 1, ...) by its mounted drive name.
 *              Default name is "dev0", ..
 * Return Value:
 *      - drive number if the drive name is found. If not, it returns
 *              MAX_DRIVE
 * Parameters
 *      . p_name: drive name. It must contain the drive name
 *              discriminator(:) at the end
 **********/
udword sm_GetDrvNo(const ubyte* p_name)
{
        udword i, j;
        ubyte drv_name[MAX_DRIVE_NAME_LEN + 1];
        udword len;

        len = SM_STRLEN((char*)p_name);
        for (i = 0; i < len; ++i)
        {
                if (p_name[i] == ':')
                        break;
        }

        if ((i < len) && (i < MAX_DRIVE_NAME_LEN))
        {
                SM_MEMCPY(drv_name, p_name, i);
                drv_name[i] = 0;

                for (j = 0; j<MAX_DRIVE; ++j)
                {
                        if (SM_STRCMP((char*)drv_name, (char*)s_drvName[j]) == 0)
                                return j;
                }
        }
#ifdef CHANGE_DIR_ENABLE
        else
        {
                /*
                 * not implemented yet. later (when smChangeDir is implemented)
                 */
        }
#endif

        return MAX_DRIVE;
}


/**********
 * Function: sm_CheckPath
 * Remarks:
 *      - searches the directory tree with the p_file_name, and returns the
 *              file(directory) handle
 * Parameters
 *      . p_file_name: "dev_name:\path_name\file_name(dir_name)" that
 *              should be searched for
 *      . check_mode:
 *              - PATH_FULL: searches to the last name in the p_file_name
 *              - PATH_EXCEPT_LAST: searches to the second from the last in
 *                      the p_file_name
 *      . p_handle (result): file(directory) handle if found.
 *              Invalid value if not found.
 **********/
ERR_CODE sm_CheckPath(udword drv_no, const ubyte* p_file_name, udword check_mode, F_HANDLE* p_handle)
{
        udword len;
        ubyte* p_str;
        ubyte* p_found;
        ubyte lname[MAX_FILE_NAME_LEN + 1];
        F_HANDLE lhandle;
        F_HANDLE lhandle_found;
        F_HANDLE lhandle_old;
        ubyte file_info[32];
        udword dummy_cluster;
        udword dummy_offset;

        p_str = (ubyte*)p_file_name;
        lhandle = lhandle_old = ROOT_HANDLE;

        if (sm_FindChar(p_str, ':', &p_found) == TRUE)
        {
                p_str = p_found + 1;
                if (p_str[0] != '\\')
                        return ERR_INVALID_PARAM;

                /* now, p_str indicates the posision after "dev0:\" */
                ++p_str;

                /*
                 * case that the p_file_name has only device name (ex. "dev0:\)
                 */
                if (p_str[0] == 0)
                {
                        if (check_mode == PATH_EXCEPT_LAST)
                                return ERR_INVALID_PARAM;
                        else if (check_mode == PATH_FULL)
                        {
                                *p_handle = ROOT_HANDLE;
                                return SM_OK;
                        }
                }

                for (;;)
                {
                        if (sm_FindChar(p_str, '\\', &p_found) == FALSE)
                        {
                                len = SM_STRLEN((char*)p_str);
                                if (len > MAX_FILE_NAME_LEN)
                                        return ERR_INVALID_PARAM;

#ifndef LONG_FILE_NAME_ENABLE
                                for(i=0; i<len; i++)
                                {
                                        if(p_str[i] == '.')
                                        {
                                                if((i > 8) || (len - (i+1) > 3))
                                                        return ERR_INVALID_PARAM;
                                        }
                                        if(p_str[i] == ' ')
                                                return ERR_INVALID_PARAM;
                                }
#endif
                                if (p_str[0] == 0)
                                {
                                        if (check_mode == PATH_EXCEPT_LAST)
                                                *p_handle = lhandle_old;
                                        else if (check_mode == PATH_FULL)
                                                *p_handle = lhandle;
                                        else
                                                return ERR_INVALID_PARAM;
                                }
                                else
                                {
                                        if (check_mode == PATH_EXCEPT_LAST)
                                                *p_handle = lhandle;
                                        else if (check_mode == PATH_FULL)
                                        {
                                                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, lhandle, (udword)p_str, file_info, &dummy_cluster, &dummy_offset);
                                                if (s_smErr != SM_OK)
                                                        return s_smErr;

                                                lhandle_found = file_info[26] | ((udword)file_info[27] << 8);
                                                *p_handle = lhandle_found;
                                        }
                                        else
                                        {
                                                return ERR_INVALID_PARAM;
                                        }
                                }

                                return SM_OK;
                        }

                        len = (udword)p_found - (udword)p_str;
                        if (len > MAX_FILE_NAME_LEN)
                                return ERR_INVALID_PARAM;

#ifndef LONG_FILE_NAME_ENABLE
                        for(i=0; i<len; i++)
                        {
                                if(p_str[i] == '.')
                                {
                                        if((i > 8) || (len - (i+1) > 3))
                                                return ERR_INVALID_PARAM;
                                }
                                if(p_str[i] == ' ')
                                        return ERR_INVALID_PARAM;
                        }
#endif

                        SM_MEMCPY(lname, p_str, len);
                        lname[len] = 0;

                        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, lhandle, (udword)lname, file_info, &dummy_cluster, &dummy_offset);
                        if (s_smErr != SM_OK)
                                return s_smErr;

                        lhandle_found = file_info[26] | ((udword)file_info[27] << 8);

                        lhandle_old = lhandle;
                        lhandle = lhandle_found;
                        p_str = p_found + 1;
                }

        }
#ifdef CHANGE_DIR_ENABLE
        else
        {
                /*
                 * not implemented yet. later (when smChangeDir is implemented)
                 */
                 ;
        }
#endif

        return SM_OK;
}


/**********
 * Function: sm_FATUpdate
 * Remarks:
 *      - write updated pFAT[] table to the FAT cluster in the disk
 * Notes:
 *      - assume that only up to 2 FATs are supported. This is checked in
 *              sm_FATInit()
 *      - 2 FATs are always in a cluster because root_entry is fixed
 *              8 KBytes (256 entries * 32 bytes)
 **********/
ERR_CODE sm_FATUpdate(udword drv_no)
{
        udword i, j, k;
        udword fat_start_lblock;
        udword fat_start_offset;
        udword lblock_size;
        udword read_offset;
        udword buf_offset;
        ubyte* temp_buf;
        udword lblock;
        udword offset;
        const sDEV_INFO* s_devinfo;
        udword surplus = 0;

        if(bNoFATUpdate[drv_no])
                return SM_OK;

        s_smErr = smlGetDeviceInfo(drv_no, &s_devinfo);
        if (s_smErr != SM_OK)
                return s_smErr;

        fat_start_lblock = s_fat1StartSector[drv_no] / s_devinfo->SpB;
        fat_start_offset = (s_fat1StartSector[drv_no] % s_devinfo->SpB) * SECTOR_SIZE;
        lblock_size = s_devinfo->SpB * SECTOR_SIZE;

        buf_offset = 0;
        temp_buf = SMB_ALLOC_BUF();
        if (!temp_buf)
                return ERR_OUT_OF_MEMORY;

        for (read_offset = 0; read_offset < s_fat1Sectors[drv_no] * SECTOR_SIZE; read_offset += SECTOR_SIZE)
        {
                if (s_smInfo[drv_no].pbr.file_sys_type == FS_FAT16)
                {
                        for (j = 0, k = 0; j < SECTOR_SIZE; j += 2, ++k)
                        {
                                temp_buf[j] = GET_FAT_TBL(drv_no, buf_offset + k) & 0xff;
                                temp_buf[j + 1] = (GET_FAT_TBL(drv_no, buf_offset + k) >> 8) & 0xff;
                        }
                        buf_offset += k;
                }
                else    /* FS_FAT12 */
                {
                        /*
                         * calculation could be beyond the section boundary.
                         * The surplus calculation is saved in the end of the
                         * buffer (offset 512, 513), and must be taken in the next
                         * loop if surplus variable is not 0
                         */

                        /* head is copied from previously calculated value */
                        temp_buf[0] = temp_buf[512];
                        temp_buf[1] = temp_buf[513];

                        /*
                         * tail(up to offset 512, 513) can be stuffed with the
                         * presently calculated values and if so, it is saved in
                         * the "surplus" variable, and must be taken in the next
                         * loop
                         */
                        for (j = surplus, k = 0; j < SECTOR_SIZE; j += 3, k += 2)
                        {
                                temp_buf[j]     = GET_FAT_TBL(drv_no, buf_offset + k) & 0xff;
                                temp_buf[j + 1] = ((GET_FAT_TBL(drv_no, buf_offset + k) >> 8) & 0xf)
                                                                        | ((GET_FAT_TBL(drv_no, buf_offset + k + 1) << 4) & 0xf0);
                                temp_buf[j + 2] = (GET_FAT_TBL(drv_no, buf_offset + k + 1) >> 4) & 0xff;
                        }

                        surplus = j - SECTOR_SIZE;
                        buf_offset += k;
                }

                /* write the updated sector */
                for (i = 0; i < s_smInfo[drv_no].pbr.bpb.fat_num; ++i)
                {
                        lblock = fat_start_lblock;
                        offset = fat_start_offset + read_offset + i * s_fat1Sectors[drv_no]     * SECTOR_SIZE;

                        if (offset > lblock_size)
                        {
                                lblock += offset / lblock_size;
                                offset %= lblock_size;
                        }

                        s_smErr = smcUpdateBlock(drv_no, lblock, offset, temp_buf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(temp_buf);
                                return s_smErr;
                        }
                }
        }

        SMB_FREE_BUF(temp_buf);
        return SM_OK;
}


/**********
 * Function: sm_RemoveFile
 * Remarks:
 *      - remove file, erase used block, restore it, and update FAT table
 * Parameters
 *      . h_dir: handle of parent directory
 *      . p_name: file name that should be deleted
 * Notes:
 *      - p_name can be directory name, and it is treated as if it were
 *              a file name
 **********/
ERR_CODE sm_RemoveFile(udword drv_no, F_HANDLE h_dir, const ubyte* p_name)
{
        ubyte file_info[32];
        udword cluster;
        udword offset;
        F_HANDLE h_file;
        udword end_cluster;
        udword old_cluster;
        udword file_size;

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)p_name, file_info, &cluster, &offset);
        if (s_smErr != SM_OK)
                return s_smErr;

        h_file = file_info[26] | ((udword)file_info[27] << 8);
        file_size = file_info[28] | ((udword)file_info[29] << 8) | ((udword)file_info[30] << 16) | ((udword)file_info[31] << 24);

        /* delete file entry from directory info */
        s_smErr = sm_DeleteFromDirEntry(drv_no, h_dir, p_name);
        if (s_smErr != SM_OK)
                FAT_RETURN(drv_no, s_smErr);

        /* update the FAT table from the end of chain */
        /*
         * 1. Do not erase blocks that has contents
         *              (it may be used in undelete routine)
         * 2. when the h_file is directory handle, its size is 0 and it
         *              must be cleared from FAT
         */
        if ((file_size != 0) || (file_info[11] & 0x10))
        {
                end_cluster = h_file & 0xffff;
                while(1)
                {
                        if(end_cluster == LAST_CLUSTER)
                                break;
                        else if(end_cluster == UNUSED_CLUSTER)
                                return ERR_INCORRECT_FAT;

                        old_cluster = end_cluster;
                        end_cluster = GET_FAT_TBL(drv_no, end_cluster);
                        SET_FAT_TBL(drv_no, old_cluster, UNUSED_CLUSTER);
                }
        }

        sm_FATUpdate(drv_no);

        return SM_OK;
}


/**********
 * Function: sm_RemoveFirstBottommostDir
 * Remarks:
 *      - remove all the files in the first bottommost directory and then
 *              the directory itself
 *      - if the "h_dir" has no subdirectory, it deletes all the files in
 *              the "h_dir" and  returns "ERR_NO_MORE_ENTRY". "h_dir" entry
 *              must be deleted from its parent entries by caller
 * Parameters
 *      . h_parent: handle of parent directory
 *      . p_name: file name that should be deleted
 *      . ddel_mode: NOT_IF_NOT_EMPTY, ALWAYS_DELETE
 * Notes:
 *      - this function can be called by itself
 *      - if ddel_mode is ALWAYS_DELETE, and subdirectory depth is so deep,
 *              it could cause an fatal error
 */
ERR_CODE sm_RemoveFirstBottommostDir(udword drv_no, F_HANDLE h_dir)
{
        ubyte short_name[20];
        ubyte file_info[32];
        udword cluster;
        udword offset;
        F_HANDLE handle;
        F_HANDLE h_parent = 0;
        ubyte dir_name[13];
        udword i, k;
        ubyte* buf;
        F_HANDLE h_file;
        udword file_size;
        udword old_cluster;
        udword next_cluster;
        udword cluster_size;


        dir_name[11] = 0;
        handle = h_dir;

        /*
         * find the bottommost directory that has not any subdirectory,
         *      and saves the parent handle, and its name for later use
         */
        for (;;)
        {
                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_SUBDIR_INDEX, handle, 0, file_info, &cluster, &offset);
                if (s_smErr != SM_OK)
                {
                        if (s_smErr == ERR_NOT_FOUND)
                                break;
                        else
                                return s_smErr;
                }

                h_parent = handle;
                SM_MEMCPY(dir_name, file_info, 11);

                handle = file_info[26] | ((udword)file_info[27] << 8);
        }

        /*
         * delete all the files in the bottommost directory
         */
        buf = SMB_ALLOC_BUF();
        if (!buf)
                return ERR_OUT_OF_MEMORY;

        cluster = handle & 0xffff;
        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster     * SECTOR_SIZE;

        while (cluster != LAST_CLUSTER)
        {
                for (offset = 0; offset < cluster_size; offset += SECTOR_SIZE)
                {
                        s_smErr = smcReadCluster(drv_no, cluster, offset, buf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(buf);
                                return s_smErr;
                        }

                        for(i=0; i<SECTOR_SIZE; i+=32)
                        {
                                /* skip current/parent directory info */
                                if(buf[i] == '.')
                                        continue;

                                /* skip deleted file */
                                if(buf[i] == 0xe5)
                                        continue;

                                /* stop processing if end of entry */
                                if(buf[i] == 0)
                                        break;

                                /* process information for long file name */
                                if(buf[i+11] == 0x0f)
                                {
                                        buf[i] = 0xe5;
                                        continue;
                                }

                                /* delete normal files & update FAT table */
                                buf[i] = 0xe5;

                                h_file = buf[i+26] | ((udword)buf[i+27] << 8);
                                file_size = buf[i+28] | ((udword)buf[i+29] << 8) | ((udword)buf[i+30] << 16) | ((udword)buf[i+31] << 24);

                                if ((file_size != 0) || (buf[i+11] & 0x10))
                                {
                                        next_cluster = h_file;
                                        while(1)
                                        {
                                                if(next_cluster == LAST_CLUSTER)
                                                        break;
                                                else if(next_cluster == UNUSED_CLUSTER)
                                                {
                                                        SMB_FREE_BUF(buf);
                                                        return ERR_INCORRECT_FAT;
                                                }

                                                old_cluster = next_cluster;
                                                next_cluster = GET_FAT_TBL(drv_no, next_cluster);
                                                SET_FAT_TBL(drv_no, old_cluster, UNUSED_CLUSTER);
                                        }
                                }
                        }

                        s_smErr = smcWriteCluster(drv_no, cluster, offset, buf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(buf);
                                return s_smErr;
                        }
                }

                cluster = GET_FAT_TBL(drv_no, cluster);
        }

        SMB_FREE_BUF(buf);

        /*
         * if the bottommost directory is the directory that is given in
         * the parameter, it returns ERR_NO_MORE_ENTRY, and let the caller
         * deletes it.
         */
        if (handle == h_dir)
                return ERR_NO_MORE_ENTRY;

        /*
         * delete thd bottommost directory
         */

        /* make name string from FAT */
        k = 0;
        for (i = 0; i <= 7; ++i)
        {
                if (dir_name[i] == ' ')
                        break;

                short_name[k++] = dir_name[i];
        }
        short_name[k++] = '.';
        for (i = 8; i <= 10; ++i)
        {
                if (dir_name[i] == ' ')
                        break;

                short_name[k++] = dir_name[i];
        }

        /* check if sting ends with '.', and set NULL to end string */
        if (short_name[k - 1] == '.')
                short_name[k - 1] = 0;
        else
                short_name[k] = 0;

        s_smErr = sm_RemoveFile(drv_no, h_parent, short_name);
        if (s_smErr != SM_OK)
                return s_smErr;

        return SM_OK;
}


/**********
 * Function: sm_FindEntryInDirectory
 * Remarks:
 *      - searches according to 'find_mode' in the directory that has "h_parent" handle,
 *              and returns 32byte file information, cluster_no, offset_in_cluster
 * Parameters
 *      . find_mode: FIND_FILE_NAME, FIND_UNUSED, FIND_DELETED,
 *              FIND_CLUSTER, FIND_FILE_INDEX, FIND_SUBDIR_INDEX
 *              (except for ".", ".."), FIND_PREVIOUS, FIND_NEXT
 *      . h_parent: directory handle that is searched from
 *      . var_par:      - FIND_FILE_NAME -> file(directory) name that is searched for (ubyte*)
 *                              - FIND_FAT_FILE_NAME -> FAT file(directory) name that is searched for (ubyte*)
 *                              - FIND_CLUSTER -> cluster in which file starts (udword)
 *                              - FIND_FILE_INDEX -> index of file (0:".", 1st:"..", 2nd:....)
 *                              - FIND_SUBDIR_INDEX -> index of directory (except for ".", "..")
 *                              - FIND_PREVIOUS -> cluster(high 16bit) | offset(low 16bit)
 *                              - FIND_NEXT -> cluster(high 16bit) | offset(low 16bit)
 *                              - FIND_DELETED -> continuous deleted block count
 *                              - FIND_UNUSED -> continuous unused block count
 *                              - else -> not used
 *      . p_info (result): 32byte file(directory) info if found
 *      . p_cluster (result): cluster number in which the information
 *              resides
 *      . p_offset (result): offset in the cluster in which the
 *              information resides
 * Notes:
 *      - the caller must always prepare the space for p_info(32bytes),
 *              p_cluster(4byte), p_offset(4byte)
 */
ERR_CODE sm_FindEntryInDirectory(udword drv_no, udword find_mode, F_HANDLE h_parent, udword var_par,
                ubyte* p_info, udword* p_cluster, udword* p_offset)
{
//        udword i;
        udword j;
        ubyte short_name[13];
        uword* p_unicode = NULL;
        uword* p_unicode_read = NULL;
        int     unicode_name_len = 0;
        ubyte* buf;
        udword offset;
        udword cluster_size;
        udword cluster;
        uword cur_cluster;
        udword cur_offset;
        uword end_cluster;
        uword old_cluster;
//        udword sector_count;
        udword index = 0;
        udword index_after_tilde = 0;
//        udword sector_num;
//        udword sector_offset;
        udword max_offset;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster     * SECTOR_SIZE;

        /* pre-processing before entering for-loop */
        if (find_mode == FIND_FILE_NAME)
        {
#ifdef  LONG_FILE_NAME_ENABLE
                p_unicode = (uword*)SM_MALLOC(MAX_FILE_NAME_LEN+2);
                if(p_unicode == NULL)
                        return ERR_OUT_OF_MEMORY;

                p_unicode_read = (uword*)SM_MALLOC(MAX_FILE_NAME_LEN+2);
                if(p_unicode_read == NULL)
                {
                        SM_FREE(p_unicode);
                        return ERR_OUT_OF_MEMORY;
                }


                if(sm_ConvertToFATName((const ubyte*)var_par, short_name))
                {
                        unicode_name_len = SM_MBS_TO_UNICODE(p_unicode, (ubyte *)var_par, SM_STRLEN((char*)var_par) + 1);
                        /* case insensitive comparison for unicode name */
                        sm_UnicodeStrToUpper(p_unicode, p_unicode, unicode_name_len);

                        for(index_after_tilde=7; index_after_tilde>0; index_after_tilde--)
                        {
                                if((short_name[index_after_tilde] == '1') && (short_name[index_after_tilde-1] == '~'))
                                        break;
                        }
                }
                else
                {
                        p_unicode[0] = 0;
                        unicode_name_len = 0;
                }
#else
                sm_ConvertToFATName((const ubyte*)var_par, short_name);
#endif
        }
        else if (find_mode == FIND_FILE_FAT_NAME)
        {
                SM_MEMCPY(short_name, (const void*)var_par, 11);
                short_name[11] = '\0';
        }


        /* Use another routine for FIND_PREVIOUS & FIND_NEXT  */
        /* These are not needed to search from the start */
        if((find_mode == FIND_PREVIOUS) || (find_mode == FIND_NEXT))
        {
                cur_cluster = (uword)((var_par >> 16) & 0xffff);
                cur_offset = var_par & 0xffff;

                /* check if cur_offset is on the boundary of 32 bytes */
                if(cur_offset & 0x1f)
                        return ERR_INTERNAL;

                if(find_mode == FIND_PREVIOUS)
                {
                        if(cur_offset < 32)
                        {
                                if(h_parent == ROOT_HANDLE)
                                        return ERR_NOT_FOUND;
                                else
                                {
                                        end_cluster = h_parent & 0xffff;
                                        while(1)
                                        {
                                                old_cluster = end_cluster;
                                                end_cluster = GET_FAT_TBL(drv_no, end_cluster);

                                                if((end_cluster == cur_cluster) || (end_cluster == LAST_CLUSTER) || (end_cluster == UNUSED_CLUSTER))
                                                        break;
                                        }

                                        if(end_cluster == cur_cluster)
                                        {
                                                cur_cluster = old_cluster;
                                                cur_offset = cluster_size - 32;
                                        }
                                        else if(end_cluster == LAST_CLUSTER)
                                                return ERR_NOT_FOUND;
                                        else if(end_cluster == UNUSED_CLUSTER)
                                                return ERR_INVALID_FAT;
                                }
                        }
                        else
                        {
                                cur_offset -= 32;
                        }
                }
                else    /* FIND_NEXT */
                {
                        if (cur_offset >= cluster_size - 32)
                        {
                                if(h_parent == ROOT_HANDLE)
                                {
                                        if(cur_offset >= s_rootSectors[drv_no] * SECTOR_SIZE - 32)
                                                return ERR_NOT_FOUND;
                                        else
                                                cur_offset += 32;
                                }
                                else
                                {
                                        cur_offset = 0;
                                        cur_cluster = GET_FAT_TBL(drv_no, cur_cluster);

                                        if (cur_cluster == LAST_CLUSTER)
                                                return ERR_NOT_FOUND;
                                        else if(cur_cluster == UNUSED_CLUSTER)
                                                return ERR_INTERNAL;
                                }
                        }
                        else
                        {
                                cur_offset += 32;
                        }
                }

                *p_cluster = cur_cluster;
                *p_offset = cur_offset;

                s_smErr = smcReadCluster(drv_no, cur_cluster, cur_offset, p_info, 32);
                if (s_smErr != SM_OK)
                        return s_smErr;

                return SM_OK;
        }


        /* find routine except FIND_PREVIOUS & FIND_NEXT */
        buf = SMB_ALLOC_BUF();
        if (!buf)
        {
                if(p_unicode)
                        SM_FREE(p_unicode);
                if(p_unicode_read)
                        SM_FREE(p_unicode_read);

                return ERR_OUT_OF_MEMORY;
        }

        cluster = h_parent & 0xffff;
        if(h_parent == ROOT_HANDLE)
                max_offset = s_rootSectors[drv_no] * SECTOR_SIZE;
        else
                max_offset = cluster_size;

        while (1)
        {
                for (offset = 0; offset < max_offset; offset += SECTOR_SIZE)
                {
                        s_smErr = smcReadCluster(drv_no, cluster, offset, buf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(buf);

                                if(p_unicode)
                                        SM_FREE(p_unicode);
                                if(p_unicode_read)
                                        SM_FREE(p_unicode_read);

                                return s_smErr;
                        }

                        for (j = 0; j < SECTOR_SIZE; j += 32)
                        {
                                if(find_mode == FIND_FILE_NAME)
                                {
#ifdef  LONG_FILE_NAME_ENABLE
                                        if(unicode_name_len)
                                        {
                                                if((SM_MEMCMP(short_name, buf + j, index_after_tilde) != 0)
                                                        || (SM_MEMCMP(short_name+8, buf + j + 8, 3) != 0))
                                                        continue;

                                                /***
                                                * sm_GetLongName() calls sm_FindEntryInDirectory() again.
                                                * But its find_mod is FIND_PREVIOUS, and it is processed before this loop
                                                ***/
                                                sm_GetLongName(drv_no, h_parent, cluster, offset+j, p_unicode_read);
                                                sm_UnicodeStrToUpper(p_unicode_read, p_unicode_read, unicode_name_len);

                                                if(SM_MEMCMP(p_unicode, p_unicode_read, unicode_name_len<<1) == 0)
                                                        break;
                                                else
                                                        continue;
                                        }
                                        else
                                        {
                                                if (SM_MEMCMP(short_name, buf + j, 11) == 0)
                                                        break;
                                        }
#else
                                        if (SM_MEMCMP(short_name, buf + j, 11) == 0)
                                                break;
#endif
                                }
                                else if(find_mode == FIND_FILE_FAT_NAME)
                                {
                                        if (SM_MEMCMP(short_name, buf + j, 11) == 0)
                                                break;
                                }
                                else if (find_mode == FIND_CLUSTER)
                                {
                                        if ((buf[j] != 0xe5) && ((buf[j + 26] | (uword)buf[j + 27] << 8) == (uword)var_par)
                                                        && ((buf[j + 11] & 0xf) != 0xf))
                                        {
                                                break;
                                        }
                                }
                                else if (find_mode == FIND_FILE_INDEX)
                                {
                                        /* exclude no file, deleted file, entry for long file name */
                                        if ((buf[j] != 0) && (buf[j] != 0xe5) && ((buf[j + 11] & 0xf) != 0xf))
                                        {
                                                if (index >= var_par)
                                                        break;

                                                ++index;
                                        }
                                }
                                else if (find_mode == FIND_SUBDIR_INDEX)
                                {
                                        if (buf[j + 11] & 0x10)
                                        {
                                                /* exclude no file, deleted file, parent_dir, current_dir */
                                                if ((buf[j] != 0) && (buf[j] != 0xe5) && (buf[j] != '.')
                                                                && ((buf[j + 11] & 0xf) != 0xf))
                                                {
                                                        if (index >= var_par)
                                                                break;

                                                        ++index;
                                                }
                                        }
                                }
                                else if (find_mode == FIND_UNUSED)
                                {
                                        if (buf[j] == 0)
                                                break;
                                }
                                else if (find_mode == FIND_DELETED)
                                {
                                        if (buf[j] == 0xe5)
                                        {
                                                udword count=1;

                                                if(var_par > 1)
                                                {
                                                        for (j += 32; j < SECTOR_SIZE; j += 32)
                                                        {
                                                                if(buf[j] == 0xe5)
                                                                        count++;
                                                                else
                                                                        break;

                                                                if(count >= var_par)
                                                                        break;
                                                        }
                                                }

                                                if(count >= var_par)
                                                        break;
                                        }
                                }
                                else
                                {
                                        SMB_FREE_BUF(buf);

                                        if(p_unicode)
                                                SM_FREE(p_unicode);
                                        if(p_unicode_read)
                                                SM_FREE(p_unicode_read);

                                        return ERR_INVALID_PARAM;
                                }

                                if (buf[j] == 0)
                                {
                                        SMB_FREE_BUF(buf);

                                        if(p_unicode)
                                                SM_FREE(p_unicode);
                                        if(p_unicode_read)
                                                SM_FREE(p_unicode_read);

                                        return ERR_NOT_FOUND;
                                }
                        }       /* for (j = 0; j < SECTOR_SIZE; j += 32) */

                        if (j < SECTOR_SIZE)
                        {
                                SM_MEMCPY(p_info, buf + j, 32);
                                *p_cluster = cluster;
                                *p_offset = offset + j;

                                SMB_FREE_BUF(buf);

                                if(p_unicode)
                                        SM_FREE(p_unicode);
                                if(p_unicode_read)
                                        SM_FREE(p_unicode_read);

                                return SM_OK;
                        }
                }       /* for (offset = 0; offset < max_offset; offset += SECTOR_SIZE) */


                if(h_parent == ROOT_HANDLE)
                        break;
                else
                {
                        cluster = GET_FAT_TBL(drv_no, cluster);

                        /* this is the case that FAT table is not updated correctly */
                        if (cluster == UNUSED_CLUSTER)
                        {
                                SMB_FREE_BUF(buf);

                                if(p_unicode)
                                        SM_FREE(p_unicode);
                                if(p_unicode_read)
                                        SM_FREE(p_unicode_read);

                                return ERR_INCORRECT_FAT;
                        }

                        if(cluster == LAST_CLUSTER)
                                break;
                }
        }       /* while(1) */

        SMB_FREE_BUF(buf);

        if(p_unicode)
                SM_FREE(p_unicode);
        if(p_unicode_read)
                SM_FREE(p_unicode_read);

        return ERR_NOT_FOUND;
}


/**********
 * Function: sm_AddToDirEntry
 * Remarks:
 *      - add 32 byte file information into the directory list table
 * Parameters:
 *      . h_dir: directory handle(cluster) into which file information
 *              should be added
 *      . long_name: adding file name
 *      . p_unicode_name: unicode file name. If NULL, it does not add long file name
 *      . p_stat : pointer of sFILE_STAT
 *      . b_insert : add entry to the specified cluster & offset
 *      . cluster : valid only if b_insert is not 0
 *      . offset : valid only if b_insert is not 0
 **********/
ERR_CODE sm_AddToDirEntry(udword drv_no, F_HANDLE h_dir, const ubyte* long_name, const uword* p_unicode_name,
                        sFILE_STAT* p_stat, udword b_insert, udword cluster, udword offset)
{
        ubyte file_info[32*((MAX_FILE_NAME_LEN - 1) / 13 + 2)];
        ubyte dummy_info[32];
        ubyte fat_name[13];
        udword new_cluster;
        udword new_offset;
        udword empty_cluster = 0;
        udword len = 0;
        udword entry_count;
        udword buf_size;
        udword offset_short_name;
        ubyte checksum;
        udword i, j, k;
//        udword b_found_deleted_cluster;
        udword write_cluster;
        udword write_offset;
        udword first_copy_size;
        udword index_of_tilde;
        udword bConverted;
        udword empty_entry_count;
        udword prev_empty_entry_count;
        udword next_empty_entry_count;
        udword max_offset;
        uword year;

        if(p_unicode_name == NULL)
        {
                entry_count = 1;
        }
        else
        {
                /* check if the file name should be processed */
                for (len = 0; len < MAX_FILE_NAME_LEN; ++len)
                {
                        if (p_unicode_name[len] == 0)
                                break;
                }

                if (len >= MAX_FILE_NAME_LEN)
                        return ERR_INVALID_PARAM;

                ++len;          /* space for NULL */

                /* 1 for name less than 13, and 1 for name of 8.3 type */
                entry_count = (len - 1) / 13 + 2;
        }

        buf_size = entry_count * 32;
        offset_short_name = buf_size - 32;

        bConverted = sm_ConvertToFATName(long_name, fat_name);
        if(bConverted == FALSE)
        {
                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_FAT_NAME, h_dir, (udword)fat_name, dummy_info, &new_cluster, &new_offset);
                if(s_smErr == SM_OK)
                        return ERR_FILE_EXIST;
                else if(s_smErr != ERR_NOT_FOUND)
                        return s_smErr;
        }
        else
        {
                /* find "~1" in fat name */
                for(i=0; i<8; i++)
                {
                        if(fat_name[i] == ' ')
                                break;
                }

                if((fat_name[i-1] == '1') && (fat_name[i-2] == '~'))
                        index_of_tilde = i-2;
                else            /* same file name (without longfile_name) exist already */
                        return ERR_FILE_EXIST;

                /* find unused short name */
                for(k = 0; k < 10; k++)
                {
                        if(k != 0)
                        {
                                /* this is the case that needs to check index_of_tilde */
                                if(k == 1)
                                {
                                        if(index_of_tilde == 5)         /* else, it is smaller => no need to change index_of_tilde */
                                        {
                                                if(fat_name[index_of_tilde - 1] & 0x80)
                                                {
                                                        index_of_tilde -= 2;
                                                        fat_name[index_of_tilde + 2] = ' ';             /* clear last garbage character */
                                                }
                                                else
                                                        index_of_tilde -= 1;

                                                fat_name[index_of_tilde] = '~';
                                        }
                                }

                                fat_name[index_of_tilde + 1] = (ubyte)('0' + k);
                        }

                        for(j = 0; j < 10; j++)
                        {
                                if((j == 0) && (k == 0))
                                        i = 1;
                                else
                                {
                                        i = 0;

                                        /* this is the case that needs to check index_of_tilde */
                                        if((j == 1) && (k == 0))
                                        {
                                                if(index_of_tilde == 6)         /* else, it is smaller => no need to change index_of_tilde */
                                                {
                                                        if(fat_name[index_of_tilde - 1] & 0x80)
                                                        {
                                                                index_of_tilde -= 2;
                                                                fat_name[index_of_tilde + 2] = ' ';             /* clear last garbage character */
                                                        }
                                                        else
                                                                index_of_tilde -= 1;

                                                        fat_name[index_of_tilde] = '~';
                                                }
                                        }

                                        if(k != 0)
                                                fat_name[index_of_tilde + 2] = (ubyte)('0' + j);
                                        else
                                                fat_name[index_of_tilde + 1] = (ubyte)('0' + j);
                                }

                                for (; i < 10; ++i)
                                {
                                        if(k != 0)
                                                fat_name[index_of_tilde+3] = (ubyte)('0' + i);
                                        else if(j != 0)
                                                fat_name[index_of_tilde+2] = (ubyte)('0' + i);
                                        else
                                                fat_name[index_of_tilde+1] = (ubyte)('0' + i);

                                        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_FAT_NAME, h_dir, (udword)fat_name, dummy_info, &new_cluster, &new_offset);
                                        if (s_smErr == ERR_NOT_FOUND)
                                                break;
                                        else if(s_smErr != SM_OK)
                                                return s_smErr;
                                }
                                if(i < 10)
                                        break;
                        }
                        if(j < 10)
                                break;
                }

                /* if ~1 - ~999 are all used */
                if(k >= 10)
                        return ERR_FILE_EXIST;
        }


        /* set long name */
        if(p_unicode_name != NULL)
        {
                for (i = 0, checksum = 0; i < 11; ++i)
                {
                        checksum = (((checksum & 1) << 7) | ((checksum & 0xfe) >> 1)) + fat_name[i];
                }

                SM_MEMSET(file_info, 0xff, buf_size - 32);

                for(i=0, k=0; i < entry_count-1; i++)
                {
                        write_offset = buf_size - (i+2) * 32;
                        for(j = 0; j < 13; j++)
                        {
                                if(k < len)
                                {
                                        if(j >= 11)
                                        {
                                                file_info[write_offset + 29 + (j - 11) * 2] = (p_unicode_name[k] >> 8) & 0xff;
                                                file_info[write_offset + 28 + (j - 11) * 2] = p_unicode_name[k++] & 0xff;
                                        }
                                        else if(j >= 5)
                                        {
                                                file_info[write_offset + 15 + (j - 5) * 2] = (p_unicode_name[k] >> 8) & 0xff;
                                                file_info[write_offset + 14 + (j - 5) * 2] = p_unicode_name[k++] & 0xff;
                                        }
                                        else
                                        {
                                                file_info[write_offset + 2 + j * 2] = (p_unicode_name[k] >> 8) & 0xff;
                                                file_info[write_offset + 1 + j * 2] = p_unicode_name[k++] & 0xff;
                                        }
                                }
                        }

                        if (write_offset == 0)          /* flag */
                                file_info[write_offset] = (ubyte)((i + 1) | 0x40);
                        else
                                file_info[write_offset] = (ubyte)(i + 1);

                        file_info[write_offset + 11] = 0x0f;            /* attribute */
                        file_info[write_offset + 12] = 0;
                        file_info[write_offset + 13] = checksum;        /* checksum */
                        file_info[write_offset + 26] = 0;                       /* first cluster */
                        file_info[write_offset + 27] = 0;                       /*     "         */
                }
        }

        /* file_info[0 - 10] <= file_name.ext_name in capital */
        SM_MEMSET(file_info + offset_short_name, 0, 32);
        SM_MEMCPY(file_info + offset_short_name, fat_name, 11);

        /* file_info[11] <= attribute */
        file_info[offset_short_name + 11] = p_stat->attr;

        year = p_stat->time.year;
        if (year > 1980)
                year -= 1980;
        else
                year = 0;


        /* file_info[22-23] <= time */
        /*      Hour | Min | Sec => 5bit | 6bit | 5bit(half value) */
        file_info[offset_short_name + 22] = ((p_stat->time.sec >> 1) & 0x1f) | ((p_stat->time.min << 5) & 0xe0);
        file_info[offset_short_name + 23] = ((p_stat->time.min >> 3) & 0x07) | ((p_stat->time.hour << 3) & 0xf8);

        /* file_info[24-25] <= date */
        /*      Year | Month | Day => 7bit | 4bit | 5bit */
        file_info[offset_short_name + 24] = ((p_stat->time.day) & 0x1f) | ((p_stat->time.month << 5) & 0xe0);
        file_info[offset_short_name + 25] = ((p_stat->time.month >> 3) & 0x01) | ((year << 1) & 0xfe);

        /* file_info[26-27] <= cluster number */
        file_info[offset_short_name + 26] = (byte)(p_stat->cluster & 0xff);
        file_info[offset_short_name + 27] = (byte)(p_stat->cluster >> 8);

        /* file_info[28-31] <= file size */
        file_info[offset_short_name + 28] = (byte)(p_stat->size & 0xff);
        file_info[offset_short_name + 29] = (byte)((p_stat->size >> 8) & 0xff);
        file_info[offset_short_name + 30] = (byte)((p_stat->size >> 16) & 0xff);
        file_info[offset_short_name + 31] = (byte)(p_stat->size >> 24);

        if(h_dir == ROOT_HANDLE)
                max_offset = s_rootSectors[drv_no] * SECTOR_SIZE;
        else
                max_offset = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;        /* cluster_size */

        /* prepare for the space of new entry */
        if(!b_insert)           /* add to any place that is available */
        {
                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_DELETED, h_dir, entry_count, dummy_info, &new_cluster, &new_offset);
                if (s_smErr != SM_OK)
                {
                        if (s_smErr == ERR_NOT_FOUND)
                        {
                                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_UNUSED, h_dir, 0, dummy_info, &new_cluster, &new_offset);
                                if (s_smErr != SM_OK)
                                {
                                        if (s_smErr == ERR_NOT_FOUND)
                                        {
                                                if(h_dir == ROOT_HANDLE)
                                                        return ERR_OUT_OF_ROOT_ENTRY;
                                                else
                                                {
                                                        new_cluster = h_dir;
                                                        while(1)
                                                        {
                                                                if(GET_FAT_TBL(drv_no, new_cluster) == LAST_CLUSTER)
                                                                        break;

                                                                new_cluster = GET_FAT_TBL(drv_no, new_cluster);
                                                        }

                                                        new_offset = max_offset;
                                                }
                                        }
                                        else
                                                return s_smErr;
                                }
                        }
                        else
                                return s_smErr;
                }

                /* now, new_cluster & new_offset indicates the place that file_info should be written */

                if (new_offset + entry_count * 32 > max_offset)
                {
                        /* file information cannot be written in 1 cluster */
                        if(h_dir == ROOT_HANDLE)
                                return ERR_OUT_OF_ROOT_ENTRY;
                        else
                        {
                                first_copy_size = max_offset - new_offset;

                                if (GET_FAT_TBL(drv_no, new_cluster) == LAST_CLUSTER)
                                {
                                        /* prepare new cluster for the directory */
                                        s_smErr = smcAllocCluster(drv_no, &empty_cluster);
                                        if (s_smErr != SM_OK)
                                                return s_smErr;

                                        /* directory contents initialize */
                                        s_smErr = smcSetCluster(drv_no, empty_cluster, 0);
                                        if (s_smErr != SM_OK)
                                                return s_smErr;

                                        /* fat table update */
                                        SET_FAT_TBL(drv_no, new_cluster, (uword)empty_cluster);
                                        SET_FAT_TBL(drv_no, empty_cluster, LAST_CLUSTER);

                                        sm_FATUpdate(drv_no);
                                }
                        }
                }
                else
                {
                        first_copy_size = entry_count * 32;
                }

                s_smErr = smcWriteCluster(drv_no, new_cluster, new_offset, file_info, first_copy_size);
                if (s_smErr != SM_OK)
                        return s_smErr;

                if (first_copy_size != entry_count * 32)
                {
                        /* new_cluster is not ROOT_HANDLE here */
                        new_cluster = GET_FAT_TBL(drv_no, new_cluster);
                        new_offset = 0;

                        s_smErr = smcWriteCluster(drv_no, new_cluster, new_offset, file_info + first_copy_size, entry_count * 32 - first_copy_size);
                        if (s_smErr != SM_OK)
                                return s_smErr;
                }
        }
        else    /* add to the specified cluster & offset */
        {
                new_cluster = cluster;
                new_offset = offset;
                empty_entry_count = 0;
                prev_empty_entry_count = 0;
                next_empty_entry_count = 0;

                s_smErr = smcReadCluster(drv_no, cluster, offset, dummy_info, 32);
                if (s_smErr != SM_OK)
                        return s_smErr;

                if(dummy_info[0] == 0xe5)
                {
                        empty_entry_count++;

                        for (;;)
                        {
                                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_NEXT, h_dir,     (new_cluster << 16) | (new_offset & 0xffff), dummy_info, &new_cluster, &new_offset);
                                if (s_smErr != SM_OK)
                                {
                                        if (s_smErr == ERR_NOT_FOUND)
                                                break;
                                        else
                                                return s_smErr;
                                }

                                if ((file_info[0] == 0xe5) || (file_info[0] == 0))
                                {
                                        next_empty_entry_count++;
                                        continue;
                                }
                                else
                                        break;
                        }
                }

                new_cluster = cluster;
                new_offset = offset;

                for (;;)
                {
                        write_cluster = new_cluster;
                        write_offset = new_offset;

                        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_PREVIOUS, h_dir, (new_cluster << 16) | (new_offset & 0xffff), dummy_info, &new_cluster, &new_offset);
                        if (s_smErr != SM_OK)
                        {
                                if (s_smErr == ERR_NOT_FOUND)
                                        break;
                                else
                                        return s_smErr;
                        }

                        if ((file_info[0] == 0xe5) || (file_info[0] == 0))
                        {
                                prev_empty_entry_count++;
                                continue;
                        }
                        else
                                break;
                }

                empty_entry_count = empty_entry_count + prev_empty_entry_count + next_empty_entry_count;
                if(empty_entry_count < entry_count)
                {
                        s_smErr = sm_ExpandClusterSpace(drv_no, h_dir, cluster, offset, (entry_count - empty_entry_count)*32);
                        if(s_smErr != SM_OK)
                                return s_smErr;
                }

                if(write_offset + entry_count*32 > max_offset)
                {
                        if(h_dir == ROOT_HANDLE)
                                return ERR_OUT_OF_ROOT_ENTRY;

                        first_copy_size = max_offset - write_offset;
                }
                else
                        first_copy_size = entry_count*32;

                s_smErr = smcWriteCluster(drv_no, write_cluster, write_offset, file_info, first_copy_size);
                if (s_smErr != SM_OK)
                        return s_smErr;

                if(first_copy_size != entry_count*32)
                {
                        /* write_cluster is not ROOT_HANDLE here */
                        write_cluster = GET_FAT_TBL(drv_no, write_cluster);
                        write_offset = 0;

                        s_smErr = smcWriteCluster(drv_no, write_cluster, write_offset, file_info + first_copy_size, entry_count*32 - first_copy_size);
                        if (s_smErr != SM_OK)
                                return s_smErr;
                }
        }

        return SM_OK;
}


/**********
 * Function: sm_ExpandClusterSpace
 * Remarks:
 *      - expand cluster size by amount of size, starting from cluster, offset
 * Parameters:
 *      . handle: file/directory handle(cluster) into which should be expanded
 *      . cluster : expand start cluster
 *      . offset : expand start offset
 *      . size : expand size
 **********/
ERR_CODE sm_ExpandClusterSpace(udword drv_no, F_HANDLE handle, udword cluster, udword offset, udword size)
{
        udword new_cluster;
        udword new_offset;
        ubyte dummy_info[32];
        ubyte* buf;
        udword empty_cluster = 0;
        udword first_copy_size;
//        udword i;
        udword next_cluster;
        udword next_offset;
        udword proc_cluster;
        udword old_cluster;
        udword cluster_size;
        udword copy_offset;
        udword copy_size;
        udword b_loop_end;
        udword max_offset;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
        if(handle == ROOT_HANDLE)
                max_offset = s_rootSectors[drv_no] * SECTOR_SIZE;
        else
                max_offset = cluster_size;

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_UNUSED, handle, 0, dummy_info, &new_cluster, &new_offset);
        if (s_smErr != SM_OK)
        {
                if (s_smErr == ERR_NOT_FOUND)
                {
                        if(handle == ROOT_HANDLE)
                                return ERR_OUT_OF_ROOT_ENTRY;
                        else
                        {
                                new_cluster = handle;
                                while(1)
                                {
                                        if(GET_FAT_TBL(drv_no, new_cluster) == LAST_CLUSTER)
                                                break;

                                        new_cluster = GET_FAT_TBL(drv_no, new_cluster);
                                }

                                new_offset = max_offset;
                        }
                }
                else
                        return s_smErr;
        }

        if(new_offset + size > max_offset)
        {
                if(handle == ROOT_HANDLE)
                {
                        return ERR_OUT_OF_ROOT_ENTRY;
                }
                else
                {
                        if(GET_FAT_TBL(drv_no, new_cluster) == LAST_CLUSTER)
                        {
                                s_smErr = smcAllocCluster(drv_no, &empty_cluster);
                                if (s_smErr != SM_OK)
                                        return s_smErr;

                                /* directory contents initialize */
                                s_smErr = smcSetCluster(drv_no, empty_cluster, 0);
                                if (s_smErr != SM_OK)
                                        return s_smErr;

                                /* fat table update */
                                SET_FAT_TBL(drv_no, new_cluster, (uword)empty_cluster);
                                SET_FAT_TBL(drv_no, empty_cluster, LAST_CLUSTER);

                                s_smErr = sm_FATUpdate(drv_no);
                                if (s_smErr != SM_OK)
                                        return s_smErr;
                        }
                }
        }

        /****
        *       now, new_cluster & new_offset indicates the end of the area that will be copied,
        *       and the space for the shift is prepared.
        *       If the space is not available, this function is returned already
        *****/

        buf = SMB_ALLOC_BUF();

        copy_size = SECTOR_SIZE;
        copy_offset = 0;
        b_loop_end = FALSE;

        while(1)
        {
                for(new_offset = max_offset - SECTOR_SIZE; (dword)new_offset >= 0; new_offset -= SECTOR_SIZE)
                {
                        s_smErr = smcReadCluster(drv_no, new_cluster, new_offset, buf, SECTOR_SIZE);
                        if (s_smErr != SM_OK)
                        {
                                SMB_FREE_BUF(buf);
                                return s_smErr;
                        }

                        if((new_cluster <= cluster) && (new_offset <= offset))
                        {
                                copy_size = SECTOR_SIZE - (offset - new_offset);
                                copy_offset = offset - new_offset;
                                b_loop_end = TRUE;
                        }

                        if(new_offset + copy_offset + size + copy_size > max_offset)
                        {
                                if(new_offset + copy_offset + size < max_offset)
                                {
                                        first_copy_size =  max_offset - (new_offset + copy_offset + size);
                                        s_smErr = smcWriteCluster(drv_no, new_cluster, new_offset+copy_offset+size, buf+copy_offset, first_copy_size);
                                        if (s_smErr != SM_OK)
                                        {
                                                SMB_FREE_BUF(buf);
                                                return s_smErr;
                                        }

                                        next_offset = 0;
                                }
                                else
                                {
                                        first_copy_size = 0;
                                        next_offset = new_offset + copy_offset + size - max_offset;
                                }

                                /****
                                *       copying size is calculated, and its required block is calculated and prepared already
                                * if the next block is not available, the area does not need to be copied
                                ****/
                                if(handle == ROOT_HANDLE)
                                        continue;
                                else
                                {
                                        next_cluster = GET_FAT_TBL(drv_no, new_cluster);
                                        if(next_cluster == LAST_CLUSTER)
                                                continue;
                                }

                                s_smErr = smcWriteCluster(drv_no, next_cluster, next_offset, buf+copy_offset+first_copy_size, copy_size-first_copy_size);
                                if (s_smErr != SM_OK)
                                {
                                        SMB_FREE_BUF(buf);
                                        return s_smErr;
                                }
                        }
                        else
                        {
                                s_smErr = smcWriteCluster(drv_no, new_cluster, new_offset+copy_offset+size, buf+copy_offset, copy_size);
                                if (s_smErr != SM_OK)
                                {
                                        SMB_FREE_BUF(buf);
                                        return s_smErr;
                                }
                        }

                        if(b_loop_end)
                                break;
                }

                /* all the needed area is copied */
                if(b_loop_end)
                        break;

                /* find previous cluster in FAT table, and set it to new_cluster */
                if(handle == ROOT_HANDLE)
                {
                        /* if handle is ROOT_HANDLE, all the needed area is copied already */
                        SMB_FREE_BUF(buf);
                        return ERR_INTERNAL;
                }
                else
                {
                        proc_cluster = handle;
                        while(1)
                        {
                                old_cluster = proc_cluster;
                                proc_cluster = GET_FAT_TBL(drv_no, proc_cluster);

                                if(proc_cluster == new_cluster)
                                        break;
                        }

                        new_cluster = old_cluster;
                }
        }

        SMB_FREE_BUF(buf);
        return SM_OK;
}


/**********
 * Function: sm_DeleteFromDirEntry
 * Remarks:
 *      - search file name and its long unicode name in the directory list table, and mark it deleted
 * Parameters
 *      . h_dir: directory handle(cluster) from which file should be removed
 *      . long_name: removing file name
 **********/
ERR_CODE sm_DeleteFromDirEntry(udword drv_no, F_HANDLE h_dir, const ubyte* long_name)
{
        udword cluster;
        udword offset;
        ubyte file_info[32];

        s_smErr = sm_FindEntryInDirectory(drv_no, FIND_FILE_NAME, h_dir, (udword)long_name, file_info, &cluster, &offset);
        if (s_smErr != SM_OK)
                return s_smErr;

        file_info[0] = 0xe5;    /* mark to deleted file */

        s_smErr = smcWriteCluster(drv_no, cluster, offset, file_info, 32);
        if (s_smErr != SM_OK)
                return s_smErr;

        for (;;)
        {
                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_PREVIOUS, h_dir, (cluster << 16) | (offset & 0xffff), file_info, &cluster, &offset);
                if (s_smErr != SM_OK)
                {
                        if (s_smErr == ERR_NOT_FOUND)
                                break;
                        else
                                return s_smErr;
                }

                if (file_info[11] == 0x0f)
                {
                        file_info[0] = 0xe5;

                        s_smErr = smcWriteCluster(drv_no, cluster, offset, file_info, 32);
                        if (s_smErr != SM_OK)
                                return s_smErr;
                }
                else
                        break;
        }

        return SM_OK;
}


/**********
 * Function: sm_AddToOpenedFileList
 * Remarks:
 *      - add p_list to the opened file list that is managed by FAT layer
 *      - the first list has NULL value with its "prev" field, and the last
 *              has NULL value with its "next" field
 *      - pOpenedList points to the first list, if any. otherwise NULL
 *      - this is called from smCreateFile, smOpenFile
 * Parameters
 *      . p_list: newly opened file property structure that should be
 *              added to the list its "prev" and "next" field should be set in
 *              this function
 **********/
ERR_CODE sm_AddToOpenedFileList(udword drv_no, sOPENED_LIST* p_list)
{
        sOPENED_LIST* p_opened;
        udword i;

        if (s_openedFile[drv_no] == NULL)
        {
                s_openedFile[drv_no] = p_list;
        }
        else
        {
                p_opened = s_openedFile[drv_no];
                for (i = 0; i < MAX_OPENED_FILE_NUM; ++i)
                {
                        if (p_opened->next == NULL)
                        {
                                p_opened->next = p_list;

                                p_list->prev = p_opened;
                                p_list->next = NULL;
                                break;
                        }

                        p_opened = p_opened->next;
                }

                if (i >= MAX_OPENED_FILE_NUM)
                        return ERR_SYSTEM_PARAMETER;
        }

        return SM_OK;
}


/**********
 * Function: sm_DeleteFromOpenedFileList
 * Remarks:
 *      - search h_file in the opened file list, and delete it from the list
 * Parameters
 *      . cluster: cluster in which the file to be deleted from the
 *              opened file list starts
 * Notes:
 *      - the cache buffer that was used by this file must be freed here
 **********/
ERR_CODE sm_DeleteFromOpenedFileList(udword drv_no, udword cluster)
{
        sOPENED_LIST* p_opened;
        udword i;

        p_opened = s_openedFile[drv_no];
        for (i = 0; i < MAX_OPENED_FILE_NUM; ++i)
        {
                if (p_opened == NULL)
                        return ERR_FILE_NOT_OPENED;

                if (p_opened->cluster == cluster)
                {
                        if (p_opened->prev == NULL)
                        {
                                s_openedFile[drv_no] = p_opened->next;
                                if (p_opened->next != NULL)
                                        ((sOPENED_LIST*)p_opened->next)->prev = NULL;
                        }
                        else
                        {
                                ((sOPENED_LIST*)p_opened->prev)->next = p_opened->next;
                                if (p_opened->next != NULL)
                                        ((sOPENED_LIST*)p_opened->next)->prev = p_opened->prev;
                        }

#ifndef WRITE_CACHE_DISABLE
                        if (p_opened->cache.p_buf != NULL)
                                SM_FREE(p_opened->cache.p_buf);
#endif

                        SM_FREE(p_opened);

                        break;
                }

                p_opened = p_opened->next;
        }

        if (i >= MAX_OPENED_FILE_NUM)
                return ERR_FILE_NOT_OPENED;

        return SM_OK;
}


#ifndef WRITE_CACHE_DISABLE
/**********
 * Function: sm_WriteCacheToDisk
 * Remarks:
 *      - write cache data to disk, and initialize cache variables
 * Parameters
 *      . p_list: pointer of opened file information
 **********/
ERR_CODE sm_WriteCacheToDisk(udword drv_no, sOPENED_LIST* p_list)
{
        udword i, j;
        ubyte* p_write;
        udword write_count;
        udword cluster;
        udword offset;
        udword cluster_start_count;
        udword cluster_end_count;
        udword cluster_size;
        udword old_cluster;
        udword copy_count;
        udword remained_count;
        ubyte* buf;
        bool b_fat_changed = FALSE;
        bool b_new_cluster = FALSE;

        p_write = p_list->cache.p_buf;
        write_count = p_list->cache.count;

        if (write_count == 0)
                return SM_OK;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
        cluster_start_count = p_list->cache.f_ptr / cluster_size;
        cluster_end_count = (p_list->cache.f_ptr + write_count) / cluster_size;
        offset = p_list->cache.f_ptr % cluster_size;

        cluster = p_list->cluster;
        buf = SMB_ALLOC_BUF();
        if (!buf)
                return ERR_OUT_OF_MEMORY;

        for (i = 0; i <= cluster_end_count; ++i)
        {
                if (i >= cluster_start_count)
                {
                        copy_count = (write_count > cluster_size - offset) ? (cluster_size - offset) : write_count;

                        if (b_new_cluster)
                        {
                                /* here, offset is always 0 */

                                remained_count = 0;

                                /*
                                 * smlWriteBlock() must be used on the sector boundary
                                 * remained_count may be not 0 in the last block
                                 */
                                if (copy_count % SECTOR_SIZE)
                                {
                                        remained_count = copy_count % SECTOR_SIZE;
                                        SM_MEMCPY(buf, p_write+copy_count-remained_count, remained_count);
                                }

                                s_smErr = smcWriteCluster(drv_no, cluster, 0, p_write, copy_count - remained_count);
                                if (s_smErr != SM_OK)
                                {
                                        SMB_FREE_BUF(buf);
                                        return s_smErr;
                                }

                                if (remained_count)
                                {
                                        s_smErr = smcWriteCluster(drv_no, cluster, copy_count-remained_count, buf, SECTOR_SIZE);
                                        if (s_smErr != SM_OK)
                                        {
                                                SMB_FREE_BUF(buf);
                                                return s_smErr;
                                        }
                                }

                                /*
                                 * if the cluster is newly made and there is some area
                                 * in which no data is written, fill the area with
                                 * dummy data for the spare data area to be valid
                                 */
                                if (i == cluster_end_count)
                                {
                                        if (copy_count)
                                        {
                                                SM_MEMSET(buf, 0, SECTOR_SIZE);
                                                for (j = ((copy_count - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE; j < cluster_size; j += SECTOR_SIZE)
                                                {
                                                        s_smErr = smcWriteCluster(drv_no, cluster, j, buf, SECTOR_SIZE);
                                                        if (s_smErr != SM_OK)
                                                        {
                                                                SMB_FREE_BUF(buf);
                                                                return s_smErr;
                                                        }
                                                }
                                        }
                                }
                        }
                        else
                        {
                                s_smErr = smcWriteCluster(drv_no, cluster, offset, p_write, copy_count);
                                if (s_smErr != SM_OK)
                                {
                                        SMB_FREE_BUF(buf);
                                        return s_smErr;
                                }
                        }

                        p_write += copy_count;
                        write_count -= copy_count;
                        offset = 0;
                }
                else
                {
                        /*
                         * if the cluster is newly made and there is no data to
                         * write, fill the cluster with dummy data for the spare
                         * data area to be valid
                         */
                        if (b_new_cluster)
                        {
                                SM_MEMSET(buf, 0, SECTOR_SIZE);
                                for (j = 0; j < cluster_size; j += SECTOR_SIZE)
                                {
                                        s_smErr = smcWriteCluster(drv_no, cluster, j, buf, SECTOR_SIZE);
                                        if (s_smErr != SM_OK)
                                        {
                                                SMB_FREE_BUF(buf);
                                                return s_smErr;
                                        }
                                }
                        }
                }

                /* prepare next cluster if this is the last loop */
                if (i < cluster_end_count)
                {
                        old_cluster = cluster;
                        cluster = GET_FAT_TBL(drv_no, old_cluster);
                        if (cluster == LAST_CLUSTER)
                        {
                                s_smErr = smcAllocCluster(drv_no, &cluster);
                                if (s_smErr != SM_OK)
                                {
                                        SMB_FREE_BUF(buf);
                                        return s_smErr;
                                }

                                SET_FAT_TBL(drv_no, old_cluster, (uword)cluster);
                                SET_FAT_TBL(drv_no, cluster, LAST_CLUSTER);
                                b_fat_changed = TRUE;
                                b_new_cluster = TRUE;
                        }
                }
        }
        SMB_FREE_BUF(buf);

        p_list->cache.f_ptr = p_list->f_ptr;
        p_list->cache.count = 0;

#ifndef FAT_UPDATE_WHEN_FILE_CLOSE
        if (b_fat_changed == TRUE)
        {
                sm_FATUpdate(drv_no);
        }
#endif

        return SM_OK;
}
#endif


/**********
 * Function: sm_WriteToDisk
 * Remarks:
 *      - write data to disk, and updates the p_list variables
 * Parameters
 *      . p_list: pointer of opened file information
 *      . p_buf: data pointer that should be written to the disk
 *      . count: data size that shoul be written to the disk
 **********/
ERR_CODE sm_WriteToDisk(udword drv_no, sOPENED_LIST* p_list, const ubyte* p_buf, udword count)
{
        udword i;
        udword write_count;
        udword cluster;
        udword offset;
        udword cluster_start_count;
        udword cluster_end_count;
        udword cluster_size;
        udword old_cluster;
        udword copy_count;
        bool b_fat_changed = FALSE;
        udword walked_cluster_count;

        if (count == 0)
                return SM_OK;

        write_count = count;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster     * SECTOR_SIZE;
        cluster_start_count = p_list->f_ptr / cluster_size;
        cluster_end_count = (p_list->f_ptr + write_count + cluster_size - 1) / cluster_size;
        offset = p_list->f_ptr % cluster_size;

        cluster = p_list->cur_cluster;
        if (cluster == LAST_CLUSTER)
        {
                s_smErr = smcAllocCluster(drv_no, &cluster);
                if (s_smErr != SM_OK)
                        return s_smErr;

                SET_FAT_TBL(drv_no, p_list->old_cur_cluster, (uword)cluster);
                SET_FAT_TBL(drv_no, cluster, LAST_CLUSTER);
                b_fat_changed = TRUE;

                p_list->cur_cluster = cluster;
        }

        for (i = cluster_start_count; i < cluster_end_count; ++i)
        {
                copy_count = (write_count > cluster_size) ? cluster_size : write_count; //??? dalma(2000.2.25)
                smcWriteCluster(drv_no, cluster, offset, p_buf, copy_count);
                offset = 0;
                p_buf += copy_count;
                write_count -= copy_count;

                if (i < cluster_end_count - 1)
                {
                        old_cluster = cluster;
                        cluster = GET_FAT_TBL(drv_no, old_cluster);
                        if (cluster == LAST_CLUSTER)
                        {
                                s_smErr = smcAllocCluster(drv_no, &cluster);
                                if (s_smErr != SM_OK)
                                        return s_smErr;

                                SET_FAT_TBL(drv_no, old_cluster, (uword)cluster);
                                SET_FAT_TBL(drv_no, cluster, LAST_CLUSTER);
                                b_fat_changed = TRUE;
                        }
                }
        }

        /* update current cluster */
        walked_cluster_count = (p_list->f_ptr % cluster_size + count) / cluster_size;
        if (walked_cluster_count > 0)
        {
                while (walked_cluster_count-- > 0)
                {
                        p_list->old_cur_cluster = p_list->cur_cluster;
                        p_list->cur_cluster = GET_FAT_TBL(drv_no, p_list->cur_cluster);
                }
        }

        p_list->f_ptr += count;
        if (p_list->f_ptr > p_list->size)
                p_list->size = p_list->f_ptr;

#ifndef FAT_UPDATE_WHEN_FILE_CLOSE
        if (b_fat_changed == TRUE)
        {
                sm_FATUpdate(drv_no);
        }
#endif

        return SM_OK;
}


/**********
 * Function: sm_ReadFromDisk
 * Remarks:
 *      - read data from disk
 * Parameters
 *      . p_list: pointer of opened file information
 *      . p_buf: data buffer pointer that should be read from the disk
 *      . count: data size that shoul be read from the disk
 *      . p_read_count (result): data size that was read from the disk
 * Notes:
 *      - p_buf must be prepared for the bytes lager than "count"
 **********/
ERR_CODE sm_ReadFromDisk(udword drv_no, sOPENED_LIST* p_list, void* p_buf, udword count, udword* p_read_count)
{
        udword i;
        udword read_count;
        udword remaining_count;
        udword cluster;
        udword offset;
        udword cluster_start_count;
        udword cluster_end_count;
        udword cluster_size;
        udword old_cluster;
        udword copy_count;
        udword walked_cluster_count;

        read_count = count;
        if (p_list->f_ptr + read_count > p_list->size)
        {
                read_count = p_list->size - p_list->f_ptr;
        }

        *p_read_count = 0;

        if (read_count == 0)
                return SM_OK;

        remaining_count = read_count;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster     * SECTOR_SIZE;
        cluster_start_count = p_list->f_ptr / cluster_size;
        cluster_end_count = (p_list->f_ptr + read_count + cluster_size - 1)     / cluster_size;
        offset = p_list->f_ptr % cluster_size;

        cluster = p_list->cur_cluster;
        for (i = cluster_start_count; i < cluster_end_count; ++i)
        {
                copy_count = (remaining_count > cluster_size - offset) ? (cluster_size - offset) : remaining_count;

                s_smErr = smcReadCluster(drv_no, cluster, offset, p_buf, copy_count);
                if (s_smErr != SM_OK)
                        return s_smErr;

                p_buf = (ubyte*)p_buf + copy_count;
                remaining_count -= copy_count;
                offset = 0;

                if (i < cluster_end_count - 1)
                {
                        old_cluster = cluster;
                        cluster = GET_FAT_TBL(drv_no, old_cluster);
                        if (cluster == LAST_CLUSTER)
                                return ERR_INTERNAL;
                }
        }

        /* update current cluster */
        walked_cluster_count = (p_list->f_ptr % cluster_size + read_count) / cluster_size;
        if (walked_cluster_count > 0)
        {
                while (walked_cluster_count-- > 0)
                {
                        p_list->old_cur_cluster = p_list->cur_cluster;
                        p_list->cur_cluster = GET_FAT_TBL(drv_no, p_list->cur_cluster);
                }
        }

        p_list->f_ptr += read_count;
        *p_read_count = read_count;
        if (read_count < count)
                return ERR_EOF;

        return SM_OK;
}


/**********
 * Function: sm_ConvertToFATName
 * Remarks:
 *      - converts the long file name to the short file name that is saved
 *              in the standard FAT file system
 *      - if the name has more than 8 characters, it converts to the form
 *              of "xxxxxx~1"
 *      - all characters are changed to the capital characters
 * Parameters
 *      . p_name: original file name. It may be short or
 *              long file(directory) name
 *      . p_short_name (result): short file(directory) name
 *              that is converted. It must have more than 13 character space
 *              from the calling function.
 * Return Values
 *      - TRUE if converted because it's not 8.3-format name, otherwise FALSE
 **********/
bool sm_ConvertToFATName(const ubyte* p_name, ubyte* p_short_name)
{
        ubyte ext[3];
        ubyte base[8];
        int len;
        int extLen = 0;
        int baseLen = 0;
        int extIndex;
        bool converted = FALSE;
        bool ext_converted = FALSE;
        int i;

        SM_MEMSET(p_short_name, ' ', 11);
        p_short_name[11] = '\0';
        len = SM_STRLEN((char*)p_name);

        /* "." and ".." are special cases */
        if (SM_STRCMP((char*)p_name, ".") == 0 || SM_STRCMP((char*)p_name, "..") == 0)
        {
                SM_MEMCPY(p_short_name, p_name, len);
                return FALSE;
        }

        /* find extension */
        for (extIndex = len - 1; extIndex >= 0; --extIndex)
        {
                if (p_name[extIndex] == '.')
                        break;
        }
        /* convert extension */
        if (extIndex >= 0)
        {
                for (i = extIndex + 1; i < len; ++i)
                {
                        ubyte c;

                        c = p_name[i];
                        if (c != ' ')
                        {
                                ext[extLen] = sm_toUpper(c);
                                if (++extLen == 3)
                                {
                                        if(p_name[i+1] != 0)
                                                ext_converted = TRUE;

                                        break;
                                }
                        }
                        else
                                converted = TRUE;
                }
        }
        else
        {
                extIndex = len;
        }

        /* convert base name */
        for (i = 0; i < extIndex; ++i)
        {
                ubyte c;

                c = p_name[i];
                if (c != ' ' && c != '.')
                {
                        base[baseLen] = sm_toUpper(c);
                        if (++baseLen == 8)
                                break;
                }
                else
                        converted = TRUE;
        }

        if (extIndex > 8)
                converted = TRUE;

        SM_MEMCPY(p_short_name, base, baseLen);
        if (extLen > 0)
                SM_MEMCPY(p_short_name + 8, ext, extLen);

        /* if converted to a short name, decorate it */
        if (converted || ext_converted)
        {
                int convertedBaseLen;

                convertedBaseLen = (baseLen > 6) ? 6 : baseLen;

                /* find multi-byte character boundary */
                for(i=convertedBaseLen-1; i >= 0; i--)
                {
                        if(base[i] < 0x80)
                                break;
                }

                if(i >= 0)
                {
                        if(!(i & 1))
                        {
                                convertedBaseLen--;
                                p_short_name[convertedBaseLen + 2] = ' ';
                        }
                }

                p_short_name[convertedBaseLen] = '~';
                p_short_name[convertedBaseLen + 1] = '1';

                return TRUE;
        }

        return FALSE;
}

ubyte sm_toUpper(ubyte c)
{
        if (c >= 'a' && c <= 'z')
        {
                c -= 'a' - 'A';
        }

        return c;
}


void sm_UnicodeStrToUpper(uword* p_dest, uword* p_src, udword len)
{
        udword i;
        uword unicode_char;

        for(i=0; i<len; i++)
        {
                unicode_char = *(p_src + i);

                if((unicode_char >= (uword)'a') && (unicode_char <= 'z'))
                        *(p_dest + i) = *(p_src + i) - (uword)('a' - 'A');
                else
                        *(p_dest + i) = *(p_src + i);

                if(unicode_char == 0)
                        break;
        }
}


/**********
 * Function: sm_GetOpenedList
 * Remarks:
 *      - get the list pointer from the file handle
 *      - if the file is not opened, it returns ERR_FILE_NOT_OPENED
 * Parameters
 *      . cluster: cluster in which the file starts
 *      . id: identifier when the same files are opened for read_only mode
 *      . pp_list (result): list pointer that corresponds to the h_file
 **********/
ERR_CODE sm_GetOpenedList(udword drv_no, udword cluster, udword id, sOPENED_LIST** pp_list)
{
        sOPENED_LIST* p_opened;

        p_opened = s_openedFile[drv_no];

        while (p_opened != NULL)
        {
                if ((p_opened->cluster == cluster) && (p_opened->id == id))
                {
                        *pp_list = p_opened;
                        return SM_OK;
                }

                p_opened = (sOPENED_LIST*)p_opened->next;
        }

        return ERR_FILE_NOT_OPENED;
}


/**********
 * Function: sm_ExtractLastName
 * Remarks:
 *      - extract file name in the end of full path name "p_name"
 *      - if the name is end with '\', this is replaced with NULL
 * Parameters
 *      . p_name: full path name
 *      . p_last_name (result): file name in the end of "p_name"
 * Notes:
 *      - the calling function must prepare (MAX_FILE_NAME_LEN+1) bytes
 *              to the p_last_name
 **********/
bool sm_ExtractLastName(const ubyte* p_name, ubyte* p_last_name)
{
        udword len;
        int i;

        len = SM_STRLEN((char*)p_name);

        for (i = len - 2; i >= 0; --i)
        {
                if (p_name[i] == '\\')
                        break;
        }

        SM_STRCPY((char*)p_last_name, (char*)&p_name[i + 1]);

        len = SM_STRLEN((char*)p_last_name);
        if (p_last_name[len - 1] == '\\')
                p_last_name[len - 1] = 0;

        return TRUE;
}


/**********
 * Function: sm_FindChar
 * Remarks:
 *      - find 'ch' from "p_str", and returns the position with p_found
 * Return Value:
 *      - TRUE if found, FALSE if not found
 * Parameters
 *      . p_str: string that should be searched in
 *      . ch: character that should be searched for
 *      . p_found (result): the position in the p_str where 'ch' is
 **********/
bool sm_FindChar(const ubyte* p_str, ubyte ch, ubyte** p_found)
{
        udword i;
        udword len;

        len = SM_STRLEN((char*)p_str);
        for (i = 0; i < len; ++i)
        {
                if (p_str[i] == ch)
                {
                        *p_found = (ubyte*)&p_str[i];
                        break;
                }
        }

        if (i >= len)
                return FALSE;

        return TRUE;
}


bool sm_GetTime(sTIME* p_time)
{
        ubyte ac_time[8];
        bool ret_val;

        ret_val = smfsGetTime(ac_time);
        if (ret_val != TRUE)
                return FALSE;

        p_time->year = ((uword)ac_time[7] << 8) | ac_time[6];
        p_time->month = ac_time[5];
        p_time->day = ac_time[4];
        p_time->hour = ac_time[3];
        p_time->min = ac_time[2];
        p_time->sec = ac_time[1];
        p_time->msec = ac_time[0];

        return TRUE;
}

#ifdef LONG_FILE_NAME_ENABLE
/**********
 * Function: smGetLongName
 * Remarks:
 *      - get the long name
 * Parameters
 *      . h_parent: handle of parent directory
 *      . short_cluster: cluster number of the short_name entry
 *      . offset: offset in the cluster of the short_name entry
 *      . p_lname (result): pointer for the long name. Unicode is returned
 **********/
ERR_CODE sm_GetLongName(udword drv_no, udword h_parent, udword short_cluster, udword offset, uword* p_lname)
{
        udword i, j, k;
        udword loop_cnt;
        udword cluster;
        udword loffset;
        udword copy_count;
        ubyte file_info[32];
        udword index = 0;

        cluster = short_cluster;
        loffset = offset;

        loop_cnt = (MAX_FILE_NAME_LEN>>1) / 13 + 1;
        for (i = 0; i < loop_cnt; ++i)
        {
                s_smErr = sm_FindEntryInDirectory(drv_no, FIND_PREVIOUS, h_parent, (cluster << 16) | (loffset & 0xffff), file_info, &cluster, &loffset);
                if (s_smErr != SM_OK)
                {
                        if (s_smErr == ERR_NOT_FOUND)
                        {
                                if(i == 0)              /* for the case that file exist in the first valid block */
                                {
                                        p_lname[0] = 0;
                                        return ERR_NO_LONG_NAME;
                                }
                                else
                                        break;
                        }
                        else
                                return s_smErr;
                }

                if (file_info[11] == 0x0f)
                {
                        copy_count = (index + 13 > (MAX_FILE_NAME_LEN>>1) - 2) ? (MAX_FILE_NAME_LEN>>1) - 2 - index : 13;
                        for (j = 0, k = 1; j < copy_count; ++j, k += 2)
                        {
                                if (k == 11)
                                        k += 3;
                                else if (k == 26)
                                        k += 2;

                                p_lname[index++] = ((uword)file_info[k + 1] << 8) | file_info[k];

                        }

                        p_lname[index] = 0;

                        if (file_info[0] & 0x40)
                                break;
                }
                else
                {
                        if (i == 0)
                        {
                                p_lname[0] = 0;
                                return ERR_NO_LONG_NAME;
                        }
                        else
                                return ERR_INVALID_NAME;
                }
        }

        /* long name must be end with 0x40 flag at the first byte */
        /* if not, it exceeds the MAX_FILE_NAME_LEN     */
        if (!(file_info[0] & 0x40))
        {
                return ERR_FILE_NAME_LEN_TOO_LONG;
        }

        return SM_OK;
}
#endif


/*****
 * Following codes are for the cluster layer.
 * These codes are contained in this FAT layer because the cluster
 * layer must access several FAT informations.
 *****/
SM_EXPORT ERR_CODE smcAllocCluster(udword drv_no, udword* p_cluster)
{
        uword totalClusters;
        uword i;
        sBPB* bpb = &s_smInfo[drv_no].pbr.bpb;

        totalClusters = (uword)(bpb->total_sectors / bpb->sectors_per_cluster);
        for (i = 2; i < totalClusters; ++i)
        {
                if (GET_FAT_TBL(drv_no, i) == UNUSED_CLUSTER)
                {
                        *p_cluster = (udword)i;
                        return SM_OK;
                }
        }

        return ERR_NO_EMPTY_CLUSTER;
}


SM_EXPORT ERR_CODE smcReadCluster(udword drv_no, udword cluster_no, udword offset, void* buf, udword size)
{
        udword start;
        udword end;
        udword blockStartNo;
        udword blockEndNo;
        udword topSize;
        udword bottomSize;
        const sDEV_INFO* devInfo;
        udword sectorsPerBlock;
        udword bytesPerBlock;
        udword i;
        byte* readBuf = (byte*)buf;
        udword startOffset;
        udword endOffset;
        dword signed_cluster_no;
        udword extra_sectors;

        s_smErr = smlGetDeviceInfo(drv_no, &devInfo);
        if (s_smErr != SM_OK)
                return s_smErr;

        if(cluster_no == ROOT_HANDLE)
        {
                signed_cluster_no = 2 - ((s_rootSectors[drv_no] - 1) / s_smInfo[drv_no].pbr.bpb.sectors_per_cluster + 1);
                extra_sectors = s_rootSectors[drv_no] % s_smInfo[drv_no].pbr.bpb.sectors_per_cluster;
                if(extra_sectors)
                        offset += (s_smInfo[drv_no].pbr.bpb.sectors_per_cluster - extra_sectors) * SECTOR_SIZE;
        }
        else
                signed_cluster_no = (dword)cluster_no;

        start = s_clusterStartSector[drv_no] + s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * (signed_cluster_no - 2);

        start += offset / SECTOR_SIZE;
        startOffset = offset % SECTOR_SIZE;

        sectorsPerBlock = devInfo->SpB;
        blockStartNo = start / sectorsPerBlock;

        end = start + (startOffset + size) / SECTOR_SIZE;
        endOffset = (startOffset + size) % SECTOR_SIZE;

        blockEndNo = end / sectorsPerBlock;

        bytesPerBlock = sectorsPerBlock * SECTOR_SIZE;

        topSize = bytesPerBlock - ((start - blockStartNo * sectorsPerBlock)     * SECTOR_SIZE + startOffset);

        if (blockStartNo == blockEndNo)         /* all within one block */
        {
                s_smErr = smlReadBlock(drv_no, blockStartNo, bytesPerBlock - topSize, readBuf, size);
                if (s_smErr != SM_OK)
                        return s_smErr;
        }
        else
        {
                bottomSize = (end - blockEndNo * sectorsPerBlock) * SECTOR_SIZE + endOffset;

                if (topSize == bytesPerBlock)
                {
                        topSize = 0;
                }

                /* upper part */
                if (topSize)
                {
                        s_smErr = smlReadBlock(drv_no, blockStartNo, bytesPerBlock - topSize, readBuf, topSize);
                        if (s_smErr != SM_OK)
                                return s_smErr;

                        readBuf += topSize;
                        ++blockStartNo;
                }

                /* body part */
                for (i = blockStartNo; i < blockEndNo; ++i)
                {
                        s_smErr = smlReadBlock(drv_no, i, 0, readBuf, bytesPerBlock);
                        if (s_smErr != SM_OK)
                                return s_smErr;

                        readBuf += bytesPerBlock;
                }

                /* lower part */
                if (bottomSize)
                {
                        s_smErr = smlReadBlock(drv_no, blockEndNo, 0, readBuf, bottomSize);
                        if (s_smErr != SM_OK)
                                return s_smErr;
                }
        }

        return SM_OK;
}

SM_EXPORT ERR_CODE smcWriteCluster(udword drv_no, udword cluster_no, udword offset, const void* buf, udword size)
{
        udword start;
        udword end;
        udword blockStartNo;
        udword blockEndNo;
        udword topSize;
        udword bottomSize;
        const sDEV_INFO* devInfo;
        udword sectorsPerBlock;
        udword bytesPerBlock;
        udword i;
        const byte* writeBuf = (const byte*)buf;
        udword startOffset;
        udword endOffset;
        dword signed_cluster_no;
        udword extra_sectors;

        s_smErr = smlGetDeviceInfo(drv_no, &devInfo);
        if (s_smErr != SM_OK)
                return s_smErr;

        if(cluster_no == ROOT_HANDLE)
        {
                signed_cluster_no = 2 - ((s_rootSectors[drv_no] - 1) / s_smInfo[drv_no].pbr.bpb.sectors_per_cluster + 1);
                extra_sectors = s_rootSectors[drv_no] % s_smInfo[drv_no].pbr.bpb.sectors_per_cluster;
                if(extra_sectors)
                        offset += (s_smInfo[drv_no].pbr.bpb.sectors_per_cluster - extra_sectors) * SECTOR_SIZE;
        }
        else
                signed_cluster_no = (dword)cluster_no;

        start = s_clusterStartSector[drv_no] + s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * (signed_cluster_no - 2);

        start += offset / SECTOR_SIZE;
        startOffset = offset % SECTOR_SIZE;

        sectorsPerBlock = devInfo->SpB;
        blockStartNo = start / sectorsPerBlock;

        end = start + (startOffset + size) / SECTOR_SIZE;
        endOffset = (startOffset + size) % SECTOR_SIZE;

        blockEndNo = end / sectorsPerBlock;

        bytesPerBlock = sectorsPerBlock * SECTOR_SIZE;

        /* topSize <= writable bytes in the present block - starting from the current write pointer(cluster, offset)    */
        topSize = bytesPerBlock - ((start - blockStartNo * sectorsPerBlock)     * SECTOR_SIZE + startOffset);

        if (blockStartNo == blockEndNo)         /* all within one block */
        {
                s_smErr = smcUpdateBlock(drv_no, blockStartNo, bytesPerBlock - topSize, writeBuf, size);
                if (s_smErr != SM_OK)
                        return s_smErr;
        }
        else
        {
                bottomSize = (end - blockEndNo * sectorsPerBlock) * SECTOR_SIZE + endOffset;

                if (topSize == bytesPerBlock)
                        topSize = 0;

                /* upper part */
                if (topSize)
                {
                        s_smErr = smcUpdateBlock(drv_no, blockStartNo, bytesPerBlock - topSize, writeBuf, topSize);
                        if (s_smErr != SM_OK)
                                return s_smErr;

                        writeBuf += topSize;
                        ++blockStartNo;
                }

                /* body part */
                for (i = blockStartNo; i < blockEndNo; ++i)
                {
                        s_smErr = smcUpdateBlock(drv_no, i, 0, writeBuf, bytesPerBlock);
                        if (s_smErr != SM_OK)
                                return s_smErr;

                        writeBuf += bytesPerBlock;
                }

                /* lower part */
                if (bottomSize)
                {
                        s_smErr = smcUpdateBlock(drv_no, blockEndNo, 0, writeBuf, bottomSize);
                        if (s_smErr != SM_OK)
                                return s_smErr;
                }
        }

        return SM_OK;
}

ERR_CODE smcSetCluster(udword drv_no, udword cluster_no, ubyte val)
{
#if (MEMORY_LEVEL < MEMORY_LARGE)
        udword cluster_size;
        udword offset;
        ubyte* buf = SMB_ALLOC_BUF();

        if (!buf)
                return ERR_OUT_OF_MEMORY;

        if(cluster_no == ROOT_HANDLE)
                return ERR_INTERNAL;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
        SM_MEMSET(buf, val, SECTOR_SIZE);

        for (offset = 0; offset < cluster_size; offset += SECTOR_SIZE)
        {
                s_smErr = smcWriteCluster(drv_no, cluster_no, offset, buf, SECTOR_SIZE);
                if (s_smErr != SM_OK)
                {
                        SMB_FREE_BUF(buf);
                        return s_smErr;
                }
        }

        SMB_FREE_BUF(buf);
        return SM_OK;
#else   /* MEMORY_LEVEL < MEMORY_LARGE */
        udword cluster_size;
        ubyte* buf;

        s_smErr = SM_OK;

        if(cluster_no == ROOT_HANDLE)
                return ERR_INTERNAL;

        cluster_size = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster * SECTOR_SIZE;
        buf = (ubyte*)SM_MALLOC(cluster_size);
        if (!buf)
                s_smErr = ERR_OUT_OF_MEMORY;
        else
        {
                SM_MEMSET(buf, val, cluster_size);

                s_smErr = smcWriteCluster(drv_no, cluster_no, 0, buf, cluster_size);

                SM_FREE(buf);
        }

        return s_smErr;
#endif
}


ERR_CODE smcUpdateBlock(udword drv_no, udword block, udword offset, const void* buf, udword count)
{
        udword changedPBlock;
        bool touchAllSectors;

        if (smlIsSpaceAvailable(drv_no, block, offset, count))
        {
                changedPBlock = UNUSED_LBLOCK;
                touchAllSectors = FALSE;
        }
        else
        {
                s_smErr = smlGetNewSpecifiedBlock(drv_no, block, &changedPBlock);
                if (s_smErr != SM_OK)
                        return s_smErr;

                touchAllSectors = TRUE;
        }

        if (changedPBlock != UNUSED_LBLOCK)
        {
                s_smErr = sm_UpdateBlock(drv_no, block, changedPBlock, offset, count, (const ubyte*)buf, touchAllSectors);
                if (s_smErr != SM_OK)
                {
                        smlDiscardPhysBlock(drv_no, changedPBlock);
                        return s_smErr;
                }
                s_smErr = smlDiscardPhysBlock(drv_no, changedPBlock);
        }
        else
        {
                s_smErr = sm_UpdateBlock(drv_no, block, changedPBlock, offset, count, (const ubyte*)buf, touchAllSectors);
                if (s_smErr != SM_OK)
                        return s_smErr;
        }

        return s_smErr;
}


ERR_CODE sm_addClusters(udword drv_no, udword cluster, udword addedClusterCount)
{
        udword nextCluster;
        udword count;
        udword i;
        udword startSector;
        udword endSector;

        /* find largest contiguous clusters */
        count = addedClusterCount;
        nextCluster = sm_searchCluster(drv_no, count, &count);
        if (nextCluster == 0)
        {
                if (count == 0)
                        return ERR_NO_EMPTY_BLOCK;
                else
                {
                        nextCluster = sm_searchCluster(drv_no, count, NULL);
                        if (nextCluster == 0)
                                return ERR_INTERNAL;
                }
        }

        /* reserve count number of clusters */
        SET_FAT_TBL(drv_no, cluster, (uword)nextCluster);
        for (i = 0; i < count - 1; ++i)
        {
                SET_FAT_TBL(drv_no, nextCluster + i, (uword)(nextCluster + i + 1));
        }
        SET_FAT_TBL(drv_no, nextCluster + i, LAST_CLUSTER);

        startSector = s_clusterStartSector[drv_no] + (nextCluster - 2) * s_smInfo[drv_no].pbr.bpb.sectors_per_cluster;
        endSector = s_clusterStartSector[drv_no] + (nextCluster + count - 2) * s_smInfo[drv_no].pbr.bpb.sectors_per_cluster;

        s_smErr = sm_eraseSectors(drv_no, startSector, endSector - startSector);
        if (s_smErr != SM_OK)
                return s_smErr;

        /* repeat for the remaining clusters */
        addedClusterCount -= count;
        if (addedClusterCount)
                return sm_addClusters(drv_no, nextCluster + i, addedClusterCount);

        return SM_OK;
}


udword sm_searchCluster(udword drv_no, udword count, udword* maxCount)
{
        uword totalClusters;
        uword i;
        uword curCount;
        sBPB* bpb = &s_smInfo[drv_no].pbr.bpb;
        udword rv = 0;
        udword maxNum = 0;

        totalClusters = (uword)(bpb->total_sectors / bpb->sectors_per_cluster);
        curCount = 0;
        for (i = 2; i < totalClusters; ++i)
        {
                if (GET_FAT_TBL(drv_no, i) == UNUSED_CLUSTER)
                {
                        if (curCount++ == 0)
                                rv = i;

                        if (curCount == count)
                                return rv;
                }
                else
                {
                        if (maxNum < curCount)
                                maxNum = curCount;

                        curCount = 0;
                }
        }

        if (maxCount)
                *maxCount = maxNum;

        return 0;
}


ERR_CODE sm_eraseSectors(udword drv_no, udword startSector,     udword count)
{
        udword curBlock;
        udword endBlock;
        udword endSector;
        udword startOffset;
        udword endOffset;
        const sDEV_INFO* devInfo;
        udword sectorsPerBlock;

        s_smErr = smlGetDeviceInfo(drv_no, &devInfo);
        if (s_smErr != SM_OK)
                return s_smErr;

        sectorsPerBlock = devInfo->SpB;

        endSector = startSector + count;

        curBlock = startSector / sectorsPerBlock;
        endBlock = (endSector - 1) / sectorsPerBlock;

        startOffset = startSector % sectorsPerBlock;
        /* if startSector is not on the block boundary */
        if (startOffset != 0)
        {
                s_smErr = sm_erasePartialBlock(drv_no, curBlock, startOffset, sectorsPerBlock - startOffset);
                if (s_smErr != SM_OK)
                        return s_smErr;

                ++curBlock;
        }

        for ( ; curBlock < endBlock; ++curBlock)
        {
                s_smErr = sm_eraseWholeBlock(drv_no, curBlock);
                if (s_smErr != SM_OK)
                        return s_smErr;
        }

        endOffset = endSector % sectorsPerBlock;
        /* if endSector is not on the block boundary */
        if (endOffset != 0)
        {
                s_smErr = sm_erasePartialBlock(drv_no, endBlock, 0, endOffset);
                if (s_smErr != SM_OK)
                        return s_smErr;
        }
        else
        {
                s_smErr = sm_eraseWholeBlock(drv_no, endBlock);
                if (s_smErr != SM_OK)
                        return s_smErr;
        }

        return SM_OK;
}

ERR_CODE sm_erasePartialBlock(udword drv_no, udword block, udword startOffset, udword count)
{
        udword changedPBlock;

        s_smErr = smlGetNewSpecifiedBlock(drv_no, block, &changedPBlock);
        if (s_smErr != SM_OK)
                return s_smErr;

        /* old data should be copied */
        if (changedPBlock != UNUSED_LBLOCK)
        {
                ubyte* temp_buf;
                ubyte i;
                udword read_offset = 0;
                const sDEV_INFO* devInfo;

                s_smErr = smlGetDeviceInfo(drv_no, &devInfo);
                if (s_smErr != SM_OK)
                {
                        smlDiscardPhysBlock(drv_no, changedPBlock);
                        return s_smErr;
                }

                temp_buf = SMB_ALLOC_BUF();
                if (!temp_buf)
                {
                        smlDiscardPhysBlock(drv_no, changedPBlock);
                        return ERR_OUT_OF_MEMORY;
                }
                for (i = 0; i < devInfo->SpB; ++i, read_offset += SECTOR_SIZE)
                {
                        if ((i < startOffset) || (i >= startOffset + count))
                        {
                                /* read 1 sector */
                                s_smErr = smlReadPhysBlock(drv_no, changedPBlock, read_offset, temp_buf, SECTOR_SIZE);
                                if (s_smErr != SM_OK)
                                {
                                        SMB_FREE_BUF(temp_buf);
                                        smlDiscardPhysBlock(drv_no, changedPBlock);
                                        return s_smErr;
                                }
                                /* write the updated sector */
                                s_smErr = smlWriteBlock(drv_no, block, read_offset, temp_buf, SECTOR_SIZE);
                                if (s_smErr != SM_OK)
                                {
                                        SMB_FREE_BUF(temp_buf);
                                        smlDiscardPhysBlock(drv_no, changedPBlock);
                                        return s_smErr;
                                }
                        }
                }

                SMB_FREE_BUF(temp_buf);
                smlDiscardPhysBlock(drv_no, changedPBlock);
        }

        return SM_OK;
}


ERR_CODE sm_eraseWholeBlock(udword drv_no, udword block)
{
        udword changedPBlock;

        s_smErr = smlGetNewSpecifiedBlock(drv_no, block, &changedPBlock);
        if (s_smErr != SM_OK)
                return s_smErr;

        if (changedPBlock != UNUSED_LBLOCK)
                smlDiscardPhysBlock(drv_no, changedPBlock);

        return SM_OK;
}


void sm_prepareFileClose(udword drv_no, const sOPENED_LIST* p_list)
{
        udword curCluster;
        udword lastCluster = 0;
        udword curSize = 0;
        udword sectorsPerBlock;
        udword sectorsPerCluster;
        const sDEV_INFO* devInfo;
        udword lsector;
        udword lblock;

        s_smErr = smlGetDeviceInfo(drv_no, &devInfo);
        if (s_smErr != SM_OK)
                return;

        sectorsPerBlock = devInfo->SpB;

        sectorsPerCluster = s_smInfo[drv_no].pbr.bpb.sectors_per_cluster;

        curCluster = p_list->cluster;
        while (curCluster != LAST_CLUSTER)
        {
                curSize += sectorsPerCluster * SECTOR_SIZE;

                if (curSize >= p_list->size)
                {
                        udword nextCluster;

                        nextCluster = GET_FAT_TBL(drv_no, curCluster);
                        if (lastCluster)
                        {
                                SET_FAT_TBL(drv_no, curCluster, UNUSED_CLUSTER);
                        }
                        else
                        {
                                SET_FAT_TBL(drv_no, curCluster, LAST_CLUSTER);
                                lastCluster = curCluster;
                        }
                        curCluster = nextCluster;
                }
                else
                {
                        curCluster = GET_FAT_TBL(drv_no, curCluster);
                }
        }

        /* touch the last block */
        lsector = s_clusterStartSector[drv_no] + (lastCluster - 2) * sectorsPerCluster
                                + (p_list->size % (sectorsPerCluster * SECTOR_SIZE)) / SECTOR_SIZE;
        lblock = lsector / sectorsPerBlock;
        smlTouchBlock(drv_no, lblock);
}


/**********
 * Function: sm_InnerNoFATUpdate
 * Remarks:
 *      - prohibits that other internal routines update FAT table
 *      - this is helpful for speed-up when processing FAT table much
 *      - this function is used only for internal use compared to smNoFATUpdate
 * Parameters
 *      . p_vol_name: drive name
 * Return Value
 *      . TRUE : successfully entered to prohibit mode. sm_InnerFATUpdate() MUST be called later.
 *      . FALSE : the specified drive is already in prohibit mode. No need to call sm_InnerFATUpdate
 * Notes :
 *      - This function must always be followed by smFATUpdate()
 **********/
udword sm_InnerNoFATUpdate(udword drv_no)
{
        /* if already no_update mode, return FALSE */
        if(bNoFATUpdate[drv_no])
                return FALSE;

        if(bInnerNoFATUpdate[drv_no])
                return FALSE;

        bInnerNoFATUpdate[drv_no] = TRUE;
        return TRUE;
}


/**********
 * Function: sm_InnerFATUpdate
 * Remarks:
 *      - cancels the NoFATUpdate mode started by sm_InnerNoFATUpdate
 *      - updates FAT table by calling sm_FATUpdate()
 * Parameters
 *      . p_vol_name: drive name
 * Return Value
 *      . always TRUE
 * Notes
 *      - if it is in no_update mode by smNoFATUpdate(), this does not update FAT table
 **********/
udword sm_InnerFATUpdate(udword drv_no)
{
        bInnerNoFATUpdate[drv_no] = TRUE;
        sm_FATUpdate(drv_no);
        return TRUE;
}

