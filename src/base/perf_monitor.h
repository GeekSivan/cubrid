/*
 * Copyright (C) 2008 Search Solution Corporation. All rights reserved by Search Solution.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */


/*
 * perf_monitor.h - Monitor execution statistics at Client
 */

#ifndef _PERF_MONITOR_H_
#define _PERF_MONITOR_H_

#ident "$Id$"

#include <stdio.h>

#include "memory_alloc.h"
#include "storage_common.h"

#if defined (SERVER_MODE)
#include "dbtype.h"
#include "connection_defs.h"
#endif /* SERVER_MODE */

#include "thread.h"

#include <time.h>
#if !defined(WINDOWS)
#include <sys/time.h>
#endif /* WINDOWS */

#include "tsc_timer.h"

/* EXPORTED GLOBAL DEFINITIONS */
#define MAX_DIAG_DATA_VALUE     0xfffffffffffffLL

#define MAX_SERVER_THREAD_COUNT         500
#define MAX_SERVER_NAMELENGTH           256
#define SH_MODE 0644

/*
 * Enable these macros to add extended statistics information in statdump.
 * Some overhead is added, do not activate for production environment.
 * - PERF_ENABLE_DETAILED_BTREE_PAGE_STAT : 
 *    index pages are detailed by root, leaf, non-leaf 
 * - PERF_ENABLE_MVCC_SNAPSHOT_STAT
 *    partitioned information per snapshot function
 * - PERF_ENABLE_LOCK_OBJECT_STAT
 *    partitioned information per type of lock
 * - PERF_ENABLE_PB_HASH_ANCHOR_STAT
 *    count and time of data page buffer hash anchor
 */

#if 1
#define PERF_ENABLE_DETAILED_BTREE_PAGE_STAT
#define PERF_ENABLE_MVCC_SNAPSHOT_STAT
#define PERF_ENABLE_LOCK_OBJECT_STAT
#define PERF_ENABLE_PB_HASH_ANCHOR_STAT
#endif

/* PERF_MODULE_TYPE x PERF_PAGE_TYPE x PAGE_FETCH_MODE x HOLDER_LATCH_MODE x COND_FIX_TYPE */
#define PERF_PAGE_FIX_COUNTERS \
  ((PERF_MODULE_CNT) * (PERF_PAGE_CNT) * (PERF_PAGE_MODE_CNT) \
   * (PERF_HOLDER_LATCH_CNT) * (PERF_CONDITIONAL_FIX_CNT))

#define PERF_PAGE_PROMOTE_COUNTERS \
  ((PERF_MODULE_CNT) * (PERF_PAGE_CNT) * (PERF_PROMOTE_CONDITION_CNT) \
  * (PERF_HOLDER_LATCH_CNT) * (2 /* success */))

/* PERF_MODULE_TYPE x PAGE_TYPE x DIRTY_OR_CLEAN x DIRTY_OR_CLEAN x READ_OR_WRITE_OR_MIX */
#define PERF_PAGE_UNFIX_COUNTERS \
  ((PERF_MODULE_CNT) * (PERF_PAGE_CNT) * 2 * 2 * (PERF_HOLDER_LATCH_CNT))

#define PERF_PAGE_LOCK_TIME_COUNTERS PERF_PAGE_FIX_COUNTERS

#define PERF_PAGE_HOLD_TIME_COUNTERS \
    ((PERF_MODULE_CNT) * (PERF_PAGE_CNT) * (PERF_PAGE_MODE_CNT) \
   * (PERF_HOLDER_LATCH_CNT))

#define PERF_PAGE_FIX_TIME_COUNTERS PERF_PAGE_FIX_COUNTERS

#define PERF_PAGE_FIX_STAT_OFFSET(module,page_type,page_found_mode,latch_mode,\
				  cond_type) \
  ((module) * (PERF_PAGE_CNT) * (PERF_PAGE_MODE_CNT) * (PERF_HOLDER_LATCH_CNT) \
    * (PERF_CONDITIONAL_FIX_CNT) \
    + (page_type) * (PERF_PAGE_MODE_CNT) * (PERF_HOLDER_LATCH_CNT) \
       * (PERF_CONDITIONAL_FIX_CNT) \
    + (page_found_mode) * (PERF_HOLDER_LATCH_CNT) * (PERF_CONDITIONAL_FIX_CNT) \
    + (latch_mode) * (PERF_CONDITIONAL_FIX_CNT) + (cond_type))

#define PERF_PAGE_PROMOTE_STAT_OFFSET(module,page_type,promote_cond, \
				      holder_latch,success) \
  ((module) * (PERF_PAGE_CNT) * (PERF_PROMOTE_CONDITION_CNT) \
    * (PERF_HOLDER_LATCH_CNT) * 2 /* success */ \
    + (page_type) * (PERF_PROMOTE_CONDITION_CNT) * (PERF_HOLDER_LATCH_CNT) \
    * 2 /* success */ \
    + (promote_cond) * (PERF_HOLDER_LATCH_CNT) * 2 /* success */ \
    + (holder_latch) * 2 /* success */ \
    + success)

#define PERF_PAGE_UNFIX_STAT_OFFSET(module,page_type,buf_dirty,\
				    dirtied_by_holder,holder_latch) \
  ((module) * (PERF_PAGE_CNT) * 2 * 2 * (PERF_HOLDER_LATCH_CNT) + \
   (page_type) * 2 * 2 * (PERF_HOLDER_LATCH_CNT) + \
   (buf_dirty) * 2 * (PERF_HOLDER_LATCH_CNT) + \
   (dirtied_by_holder) * (PERF_HOLDER_LATCH_CNT) + (holder_latch))

#define PERF_PAGE_LOCK_TIME_OFFSET(module,page_type,page_found_mode,latch_mode,\
				  cond_type) \
	PERF_PAGE_FIX_STAT_OFFSET(module,page_type,page_found_mode,latch_mode,\
				  cond_type)

#define PERF_PAGE_HOLD_TIME_OFFSET(module,page_type,page_found_mode,latch_mode)\
  ((module) * (PERF_PAGE_CNT) * (PERF_PAGE_MODE_CNT) * (PERF_HOLDER_LATCH_CNT) \
    + (page_type) * (PERF_PAGE_MODE_CNT) * (PERF_HOLDER_LATCH_CNT) \
    + (page_found_mode) * (PERF_HOLDER_LATCH_CNT) \
    + (latch_mode))

#define PERF_PAGE_FIX_TIME_OFFSET(module,page_type,page_found_mode,latch_mode,\
				  cond_type) \
	PERF_PAGE_FIX_STAT_OFFSET(module,page_type,page_found_mode,latch_mode,\
				  cond_type)

#define PERF_MVCC_SNAPSHOT_COUNTERS \
  (PERF_SNAPSHOT_CNT * PERF_SNAPSHOT_RECORD_TYPE_CNT \
   * PERF_SNAPSHOT_VISIBILITY_CNT)

#define PERF_MVCC_SNAPSHOT_OFFSET(snapshot,rec_type,visibility) \
  ((snapshot) * PERF_SNAPSHOT_RECORD_TYPE_CNT * PERF_SNAPSHOT_VISIBILITY_CNT \
   + (rec_type) * PERF_SNAPSHOT_VISIBILITY_CNT + (visibility))

#define PERF_OBJ_LOCK_STAT_OFFSET(module,lock_mode) \
  ((module) * (SCH_M_LOCK + 1) + (lock_mode))

#define PERF_OBJ_LOCK_STAT_COUNTERS (PERF_MODULE_CNT * (SCH_M_LOCK + 1))

typedef enum
{
  PERF_MODULE_SYSTEM = 0,
  PERF_MODULE_USER,
  PERF_MODULE_VACUUM,

  PERF_MODULE_CNT
} PERF_MODULE_TYPE;

typedef enum
{
  PERF_HOLDER_LATCH_READ = 0,
  PERF_HOLDER_LATCH_WRITE,
  PERF_HOLDER_LATCH_MIXED,

  PERF_HOLDER_LATCH_CNT
} PERF_HOLDER_LATCH;

typedef enum
{
  PERF_CONDITIONAL_FIX = 0,
  PERF_UNCONDITIONAL_FIX_NO_WAIT,
  PERF_UNCONDITIONAL_FIX_WITH_WAIT,

  PERF_CONDITIONAL_FIX_CNT
} PERF_CONDITIONAL_FIX_TYPE;

typedef enum
{
  PERF_PROMOTE_ONLY_READER,
  PERF_PROMOTE_SHARED_READER,

  PERF_PROMOTE_CONDITION_CNT
} PERF_PROMOTE_CONDITION;

typedef enum
{
  PERF_PAGE_MODE_OLD_LOCK_WAIT = 0,
  PERF_PAGE_MODE_OLD_NO_WAIT,
  PERF_PAGE_MODE_NEW_LOCK_WAIT,
  PERF_PAGE_MODE_NEW_NO_WAIT,
  PERF_PAGE_MODE_OLD_IN_BUFFER,

  PERF_PAGE_MODE_CNT
} PERF_PAGE_MODE;

/* extension of PAGE_TYPE (storage_common.h) - keep value compatibility */
typedef enum
{
  PERF_PAGE_UNKNOWN = 0,	/* used for initialized page */
  PERF_PAGE_FTAB,		/* file allocset table page */
  PERF_PAGE_HEAP,		/* heap page */
  PERF_PAGE_VOLHEADER,		/* volume header page */
  PERF_PAGE_VOLBITMAP,		/* volume bitmap page */
  PERF_PAGE_XASL,		/* xasl stream page */
  PERF_PAGE_QRESULT,		/* query result page */
  PERF_PAGE_EHASH,		/* ehash bucket/dir page */
  PERF_PAGE_LARGEOBJ,		/* large object/dir page */
  PERF_PAGE_OVERFLOW,		/* overflow page (with ovf_keyval) */
  PERF_PAGE_AREA,		/* area page */
  PERF_PAGE_CATALOG,		/* catalog page */
  PERF_PAGE_BTREE_GENERIC,	/* b+tree index (uninitialized) */
  PERF_PAGE_LOG,		/* NONE - log page (unused) */
  PERF_PAGE_DROPPED_FILES,	/* Dropped files page.  */
#if defined(PERF_ENABLE_DETAILED_BTREE_PAGE_STAT)
  PERF_PAGE_BTREE_ROOT,		/* b+tree root index page */
  PERF_PAGE_BTREE_OVF,		/* b+tree overflow index page */
  PERF_PAGE_BTREE_LEAF,		/* b+tree leaf index page */
  PERF_PAGE_BTREE_NONLEAF,	/* b+tree nonleaf index page */
#endif /* PERF_ENABLE_DETAILED_BTREE_PAGE_STAT */
  PERF_PAGE_CNT
} PERF_PAGE_TYPE;

typedef enum
{
  PERF_SNAPSHOT_SATISFIES_DELETE = 0,
  PERF_SNAPSHOT_SATISFIES_DIRTY,
  PERF_SNAPSHOT_SATISFIES_SNAPSHOT,
  PERF_SNAPSHOT_SATISFIES_VACUUM,

  PERF_SNAPSHOT_CNT
} PERF_SNAPSHOT_TYPE;

typedef enum
{
  PERF_SNAPSHOT_RECORD_INSERTED_VACUUMED = 0,	/* already vacuumed */
  PERF_SNAPSHOT_RECORD_INSERTED_CURR_TRAN,	/* needs commit and vacuum */
  PERF_SNAPSHOT_RECORD_INSERTED_OTHER_TRAN,	/* needs commit and vacuum */
  PERF_SNAPSHOT_RECORD_INSERTED_COMMITED,	/* commited, needs vacuum */
  PERF_SNAPSHOT_RECORD_INSERTED_COMMITED_LOST,	/* commited, unvacuumed/lost */

  PERF_SNAPSHOT_RECORD_INSERTED_DELETED,	/* inserted, than deleted */

  PERF_SNAPSHOT_RECORD_DELETED_CURR_TRAN,	/* deleted by current tran */
  PERF_SNAPSHOT_RECORD_DELETED_OTHER_TRAN,	/* deleted by other active tran */
  PERF_SNAPSHOT_RECORD_DELETED_COMMITTED,	/* deleted, and committed */
  PERF_SNAPSHOT_RECORD_DELETED_COMMITTED_LOST,	/* deleted, and committed, unvacuumed/lost */

  PERF_SNAPSHOT_RECORD_TYPE_CNT
} PERF_SNAPSHOT_RECORD_TYPE;

typedef enum
{
  PERF_SNAPSHOT_INVISIBLE = 0,
  PERF_SNAPSHOT_VISIBLE,

  PERF_SNAPSHOT_VISIBILITY_CNT
} PERF_SNAPSHOT_VISIBILITY;

typedef enum
{
  /* Execution statistics for the file io */
  PSTAT_FILE_NUM_CREATES = 0,
  PSTAT_FILE_NUM_REMOVES,
  PSTAT_FILE_NUM_IOREADS,
  PSTAT_FILE_NUM_IOWRITES,
  PSTAT_FILE_NUM_IOSYNCHES,
  PSTAT_FILE_NUM_PAGE_ALLOCS,
  PSTAT_FILE_NUM_PAGE_DEALLOCS,

  /* Execution statistics for the page buffer manager */
  PSTAT_PB_NUM_FETCHES,
  PSTAT_PB_NUM_DIRTIES,
  PSTAT_PB_NUM_IOREADS,
  PSTAT_PB_NUM_IOWRITES,
  PSTAT_PB_NUM_VICTIMS,
  PSTAT_PB_NUM_REPLACEMENTS,
  PSTAT_PB_NUM_HASH_ANCHOR_WAITS,
  PSTAT_PB_TIME_HASH_ANCHOR_WAIT,
  /* peeked stats */
  PSTAT_PB_FIXED_CNT,
  PSTAT_PB_DIRTY_CNT,
  PSTAT_PB_LRU1_CNT,
  PSTAT_PB_LRU2_CNT,
  PSTAT_PB_AIN_CNT,
  PSTAT_PB_AVOID_DEALLOC_CNT,
  PSTAT_PB_AVOID_VICTIM_CNT,
  PSTAT_PB_VICTIM_CAND_CNT,

  /* Execution statistics for the log manager */
  PSTAT_LOG_NUM_FETCHES,
  PSTAT_LOG_NUM_FETCH_IOREADS,
  PSTAT_LOG_NUM_IOREADS,
  PSTAT_LOG_NUM_IOWRITES,
  PSTAT_LOG_NUM_APPENDRECS,
  PSTAT_LOG_NUM_ARCHIVES,
  PSTAT_LOG_NUM_START_CHECKPOINTS,
  PSTAT_LOG_NUM_END_CHECKPOINTS,
  PSTAT_LOG_NUM_WALS,
  PSTAT_LOG_NUM_REPLACEMENTS,

  /* Execution statistics for the lock manager */
  PSTAT_LK_NUM_ACQUIRED_ON_PAGES,
  PSTAT_LK_NUM_ACQUIRED_ON_OBJECTS,
  PSTAT_LK_NUM_CONVERTED_ON_PAGES,
  PSTAT_LK_NUM_CONVERTED_ON_OBJECTS,
  PSTAT_LK_NUM_RE_REQUESTED_ON_PAGES,
  PSTAT_LK_NUM_RE_REQUESTED_ON_OBJECTS,
  PSTAT_LK_NUM_WAITED_ON_PAGES,
  PSTAT_LK_NUM_WAITED_ON_OBJECTS,
  PSTAT_LK_NUM_WAITED_TIME_ON_OBJECTS,	/* include this to avoid client-server compat issue even if extended stats are
					 * disabled */

  /* Execution statistics for transactions */
  PSTAT_TRAN_NUM_COMMITS,
  PSTAT_TRAN_NUM_ROLLBACKS,
  PSTAT_TRAN_NUM_SAVEPOINTS,
  PSTAT_TRAN_NUM_START_TOPOPS,
  PSTAT_TRAN_NUM_END_TOPOPS,
  PSTAT_TRAN_NUM_INTERRUPTS,

  /* Execution statistics for the btree manager */
  PSTAT_BT_NUM_INSERTS,
  PSTAT_BT_NUM_DELETES,
  PSTAT_BT_NUM_UPDATES,
  PSTAT_BT_NUM_COVERED,
  PSTAT_BT_NUM_NONCOVERED,
  PSTAT_BT_NUM_RESUMES,
  PSTAT_BT_NUM_MULTI_RANGE_OPT,
  PSTAT_BT_NUM_SPLITS,
  PSTAT_BT_NUM_MERGES,
  PSTAT_BT_NUM_GET_STATS,

  /* Execution statistics for the heap manager */
  PSTAT_HEAP_NUM_STATS_SYNC_BESTSPACE,

  /* Execution statistics for the query manager */
  PSTAT_QM_NUM_SELECTS,
  PSTAT_QM_NUM_INSERTS,
  PSTAT_QM_NUM_DELETES,
  PSTAT_QM_NUM_UPDATES,
  PSTAT_QM_NUM_SSCANS,
  PSTAT_QM_NUM_ISCANS,
  PSTAT_QM_NUM_LSCANS,
  PSTAT_QM_NUM_SETSCANS,
  PSTAT_QM_NUM_METHSCANS,
  PSTAT_QM_NUM_NLJOINS,
  PSTAT_QM_NUM_MJOINS,
  PSTAT_QM_NUM_OBJFETCHES,
  PSTAT_QM_NUM_HOLDABLE_CURSORS,

  /* Execution statistics for external sort */
  PSTAT_SORT_NUM_IO_PAGES,
  PSTAT_SORT_NUM_DATA_PAGES,

  /* Execution statistics for network communication */
  PSTAT_NET_NUM_REQUESTS,

  /* flush control stat */
  PSTAT_FC_NUM_PAGES,
  PSTAT_FC_NUM_LOG_PAGES,
  PSTAT_FC_TOKENS,

  /* prior lsa info */
  PSTAT_PRIOR_LSA_LIST_SIZE,		/* kbytes */
  PSTAT_PRIOR_LSA_LIST_MAXED,
  PSTAT_PRIOR_LSA_LIST_REMOVED,

  /* best space info */
  PSTAT_HF_NUM_STATS_ENTRIES,
  PSTAT_HF_NUM_STATS_MAXED,

  /* HA replication delay */
  PSTAT_HA_REPL_DELAY,

  /* Execution statistics for Plan cache */
  PSTAT_PC_NUM_ADD,
  PSTAT_PC_NUM_LOOKUP,
  PSTAT_PC_NUM_HIT,
  PSTAT_PC_NUM_MISS,
  PSTAT_PC_NUM_FULL,
  PSTAT_PC_NUM_DELETE,
  PSTAT_PC_NUM_INVALID_XASL_ID,
  PSTAT_PC_NUM_QUERY_STRING_HASH_ENTRIES,
  PSTAT_PC_NUM_XASL_ID_HASH_ENTRIES,
  PSTAT_PC_NUM_CLASS_OID_HASH_ENTRIES,

  PSTAT_VAC_NUM_VACUUMED_LOG_PAGES,
  PSTAT_VAC_NUM_TO_VACUUM_LOG_PAGES,
  PSTAT_VAC_NUM_PREFETCH_REQUESTS_LOG_PAGES,
  PSTAT_VAC_NUM_PREFETCH_HITS_LOG_PAGES,

  /* Track heap modify counters. */
  PSTAT_HEAP_HOME_INSERTS,
  PSTAT_HEAP_BIG_INSERTS,
  PSTAT_HEAP_ASSIGN_INSERTS,
  PSTAT_HEAP_HOME_DELETES,
  PSTAT_HEAP_HOME_MVCC_DELETES,
  PSTAT_HEAP_HOME_TO_REL_DELETES,
  PSTAT_HEAP_HOME_TO_BIG_DELETES,
  PSTAT_HEAP_REL_DELETES,
  PSTAT_HEAP_REL_MVCC_DELETES,
  PSTAT_HEAP_REL_TO_HOME_DELETES,
  PSTAT_HEAP_REL_TO_BIG_DELETES,
  PSTAT_HEAP_REL_TO_REL_DELETES,
  PSTAT_HEAP_BIG_DELETES,
  PSTAT_HEAP_BIG_MVCC_DELETES,
  PSTAT_HEAP_NEW_VER_INSERTS,
  PSTAT_HEAP_HOME_UPDATES,
  PSTAT_HEAP_HOME_TO_REL_UPDATES,
  PSTAT_HEAP_HOME_TO_BIG_UPDATES,
  PSTAT_HEAP_REL_UPDATES,
  PSTAT_HEAP_REL_TO_HOME_UPDATES,
  PSTAT_HEAP_REL_TO_REL_UPDATES,
  PSTAT_HEAP_REL_TO_BIG_UPDATES,
  PSTAT_HEAP_BIG_UPDATES,
  PSTAT_HEAP_HOME_VACUUMS,
  PSTAT_HEAP_BIG_VACUUMS,
  PSTAT_HEAP_REL_VACUUMS,
  PSTAT_HEAP_INSID_VACUUMS,
  PSTAT_HEAP_REMOVE_VACUUMS,

  /* Track heap modify timers. */
  PSTAT_HEAP_INSERT_PREPARE,
  PSTAT_HEAP_INSERT_EXECUTE,
  PSTAT_HEAP_INSERT_LOG,
  PSTAT_HEAP_DELETE_PREPARE,
  PSTAT_HEAP_DELETE_EXECUTE,
  PSTAT_HEAP_DELETE_LOG,
  PSTAT_HEAP_UPDATE_PREPARE,
  PSTAT_HEAP_UPDATE_EXECUTE,
  PSTAT_HEAP_UPDATE_LOG,
  PSTAT_HEAP_VACUUM_PREPARE,
  PSTAT_HEAP_VACUUM_EXECUTE,
  PSTAT_HEAP_VACUUM_LOG,

  /* B-tree ops detailed statistics. */
  PSTAT_BT_FIX_OVF_OIDS,
  PSTAT_BT_UNIQUE_RLOCKS,
  PSTAT_BT_UNIQUE_WLOCKS,
  PSTAT_BT_LEAF,
  PSTAT_BT_TRAVERSE,
  PSTAT_BT_FIND_UNIQUE,
  PSTAT_BT_FIND_UNIQUE_TRAVERSE,
  PSTAT_BT_RANGE_SEARCH,
  PSTAT_BT_RANGE_SEARCH_TRAVERSE,
  PSTAT_BT_INSERT,
  PSTAT_BT_INSERT_TRAVERSE,
  PSTAT_BT_DELETE,
  PSTAT_BT_DELETE_TRAVERSE,
  PSTAT_BT_MVCC_DELETE,
  PSTAT_BT_MVCC_DELETE_TRAVERSE,
  PSTAT_BT_MARK_DELETE,
  PSTAT_BT_MARK_DELETE_TRAVERSE,
  PSTAT_BT_UNDO_INSERT,
  PSTAT_BT_UNDO_INSERT_TRAVERSE,
  PSTAT_BT_UNDO_DELETE,
  PSTAT_BT_UNDO_DELETE_TRAVERSE,
  PSTAT_BT_UNDO_MVCC_DELETE,
  PSTAT_BT_UNDO_MVCC_DELETE_TRAVERSE,
  PSTAT_BT_VACUUM,
  PSTAT_BT_VACUUM_TRAVERSE,
  PSTAT_BT_VACUUM_INSID,
  PSTAT_BT_VACUUM_INSID_TRAVERSE,

  /* Vacuum master/worker timers. */
  PSTAT_VAC_MASTER,
  PSTAT_VAC_JOB,
  PSTAT_VAC_WORKER_PROCESS_LOG,
  PSTAT_VAC_WORKER_EXECUTE,

  /* Other statistics (change MNT_COUNT_OF_SERVER_EXEC_CALC_STATS) */
  /* ((pb_num_fetches - pb_num_ioreads) x 100 / pb_num_fetches) x 100 */
  PSTAT_PB_HIT_RATIO,
  /* ((log_num_fetches - log_num_fetch_ioreads) x 100 / log_num_fetches) x 100 */
  PSTAT_LOG_HIT_RATIO,
  /* ((fetches of vacuum - fetches of vacuum not found in PB) x 100 / fetches of vacuum) x 100 */
  PSTAT_VACUUM_DATA_HIT_RATIO,
  /* (100 x Number of unfix with of dirty pages of vacuum / total num of unfixes from vacuum) x 100 */
  PSTAT_PB_VACUUM_EFFICIENCY,
  /* (100 x Number of unfix from vacuum / total num of unfix) x 100 */
  PSTAT_PB_VACUUM_FETCH_RATIO,
  /* total time to acquire page lock (stored as 10 usec unit, displayed as miliseconds) */
  PSTAT_PB_PAGE_LOCK_ACQUIRE_TIME_10USEC,
  /* total time to acquire page hold (stored as 10 usec unit, displayed as miliseconds) */
  PSTAT_PB_PAGE_HOLD_ACQUIRE_TIME_10USEC,
  /* total time to acquire page fix (stored as 10 usec unit, displayed as miliseconds) */
  PSTAT_PB_PAGE_FIX_ACQUIRE_TIME_10USEC,
  /* ratio of time required to allocate a buffer for a page : (100 x (fix_time - lock_time - hold_time) / fix_time) x
   * 100 */
  PSTAT_PB_PAGE_ALLOCATE_TIME_RATIO,
  /* total successful promotions */
  PSTAT_PB_PAGE_PROMOTE_SUCCESS,
  /* total failed promotions */
  PSTAT_PB_PAGE_PROMOTE_FAILED,
  /* total promotion time */
  PSTAT_PB_PAGE_PROMOTE_TOTAL_TIME_10USEC,

  /* array counters : do not include in MNT_SIZE_OF_SERVER_EXEC_SINGLE_STATS */
  /* change MNT_COUNT_OF_SERVER_EXEC_ARRAY_STATS and MNT_SIZE_OF_SERVER_EXEC_ARRAY_STATS */
  PSTAT_PBX_FIX_COUNTERS,
  PSTAT_PBX_PROMOTE_COUNTERS,
  PSTAT_PBX_PROMOTE_TIME_COUNTERS,
  PSTAT_PBX_UNFIX_COUNTERS,
  PSTAT_PBX_LOCK_TIME_COUNTERS,
  PSTAT_PBX_HOLD_TIME_COUNTERS,
  PSTAT_PBX_FIX_TIME_COUNTERS,
  PSTAT_MVCC_SNAPSHOT_COUNTERS,
  PSTAT_OBJ_LOCK_TIME_COUNTERS,
  PSTAT_LOG_SNAPSHOT_TIME_COUNTERS,
  PSTAT_LOG_SNAPSHOT_RETRY_COUNTERS,
  PSTAT_LOG_TRAN_COMPLETE_TIME_COUNTERS,
  PSTAT_LOG_OLDEST_MVCC_TIME_COUNTERS,
  PSTAT_LOG_OLDEST_MVCC_RETRY_COUNTERS,

  PSTAT_COUNT = PSTAT_LOG_OLDEST_MVCC_RETRY_COUNTERS + 1
} PERF_STAT_ID;


typedef struct diag_sys_config DIAG_SYS_CONFIG;
struct diag_sys_config
{
  int Executediag;
  int DiagSM_ID_server;
  int server_long_query_time;	/* min 1 sec */
};

typedef struct t_diag_monitor_db_value T_DIAG_MONITOR_DB_VALUE;
struct t_diag_monitor_db_value
{
  INT64 query_open_page;
  INT64 query_opened_page;
  INT64 query_slow_query;
  INT64 query_full_scan;
  INT64 conn_cli_request;
  INT64 conn_aborted_clients;
  INT64 conn_conn_req;
  INT64 conn_conn_reject;
  INT64 buffer_page_write;
  INT64 buffer_page_read;
  INT64 lock_deadlock;
  INT64 lock_request;
};

typedef struct t_diag_monitor_cas_value T_DIAG_MONITOR_CAS_VALUE;
struct t_diag_monitor_cas_value
{
  INT64 reqs_in_interval;
  INT64 transactions_in_interval;
  INT64 query_in_interval;
  int active_sessions;
};

/* Monitor config related structure */

typedef struct monitor_cas_config MONITOR_CAS_CONFIG;
struct monitor_cas_config
{
  char head;
  char body[2];
};

typedef struct monitor_server_config MONITOR_SERVER_CONFIG;
struct monitor_server_config
{
  char head[2];
  char body[8];
};

typedef struct t_client_monitor_config T_CLIENT_MONITOR_CONFIG;
struct t_client_monitor_config
{
  MONITOR_CAS_CONFIG cas;
  MONITOR_SERVER_CONFIG server;
};

/* Shared memory data struct */

typedef struct t_shm_diag_info_server T_SHM_DIAG_INFO_SERVER;
struct t_shm_diag_info_server
{
  int magic;
  int num_thread;
  int magic_key;
  char servername[MAX_SERVER_NAMELENGTH];
  T_DIAG_MONITOR_DB_VALUE thread[MAX_SERVER_THREAD_COUNT];
};

enum t_diag_shm_mode
{
  DIAG_SHM_MODE_ADMIN = 0,
  DIAG_SHM_MODE_MONITOR = 1
};
typedef enum t_diag_shm_mode T_DIAG_SHM_MODE;

enum t_diag_server_type
{
  DIAG_SERVER_DB = 00000,
  DIAG_SERVER_CAS = 10000,
  DIAG_SERVER_DRIVER = 20000,
  DIAG_SERVER_RESOURCE = 30000
};
typedef enum t_diag_server_type T_DIAG_SERVER_TYPE;

/* number of fields of MNT_SERVER_EXEC_STATS structure (includes computed stats) */
#define MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS 196	/* TODO: Get rid of this thing! */

/* number of array stats of MNT_SERVER_EXEC_STATS structure */
#define MNT_COUNT_OF_SERVER_EXEC_ARRAY_STATS 14

/* number of computed stats of MNT_SERVER_EXEC_STATS structure */
#define MNT_COUNT_OF_SERVER_EXEC_CALC_STATS 12

#define MNT_SIZE_OF_SERVER_EXEC_SINGLE_STATS \
  MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS

#define MNT_SIZE_OF_SERVER_EXEC_CALC_STATS \
  MNT_COUNT_OF_SERVER_EXEC_CALC_STATS

#define MNT_SERVER_EXEC_STATS_COUNT \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS \
  + MNT_COUNT_OF_SERVER_EXEC_ARRAY_STATS)

#define MNT_TOTAL_EXEC_STATS_COUNT \
  (MNT_SERVER_EXEC_STATS_COUNT \
  + MNT_COUNT_OF_SERVER_EXEC_CALC_STATS)

#define MNT_SIZE_OF_SERVER_EXEC_ARRAY_STATS \
  (PERF_PAGE_FIX_COUNTERS + PERF_PAGE_PROMOTE_COUNTERS \
  + PERF_PAGE_PROMOTE_COUNTERS + PERF_PAGE_UNFIX_COUNTERS \
  + PERF_PAGE_LOCK_TIME_COUNTERS + PERF_PAGE_HOLD_TIME_COUNTERS \
  + PERF_PAGE_FIX_TIME_COUNTERS + PERF_MVCC_SNAPSHOT_COUNTERS \
  + PERF_OBJ_LOCK_STAT_COUNTERS + PERF_MODULE_CNT + PERF_MODULE_CNT \
  + PERF_MODULE_CNT + PERF_MODULE_CNT + PERF_MODULE_CNT)

#define MNT_SIZE_OF_SERVER_EXEC_STATS \
  (MNT_SIZE_OF_SERVER_EXEC_SINGLE_STATS \
  + MNT_SIZE_OF_SERVER_EXEC_ARRAY_STATS)

#define MNT_TOTAL_SIZE_EXEC_STATS \
  (MNT_SIZE_OF_SERVER_EXEC_SINGLE_STATS \
  + MNT_SIZE_OF_SERVER_EXEC_CALC_STATS  \
  + MNT_SIZE_OF_SERVER_EXEC_ARRAY_STATS)

/* The exact size of mnt_server_exec_stats structure */
#define MNT_SERVER_EXEC_STATS_SIZEOF \
  (offsetof (MNT_SERVER_EXEC_STATS, enable_local_stat) + sizeof (bool))


/*
 * Server execution statistic structure
 * If members are added or removed in this structure then changes must be made
 * also in MNT_SIZE_OF_SERVER_EXEC_STATS, STAT_SIZE_MEMORY and
 * MNT_SERVER_EXEC_STATS_SIZEOF
 */
typedef struct mnt_server_exec_stats MNT_SERVER_EXEC_STATS;
struct mnt_server_exec_stats
{
  /* Array of performance statistics */
  UINT64 perf_statistics[MNT_TOTAL_SIZE_EXEC_STATS];
  bool enable_local_stat;	/* used for local stats */
};

typedef struct mnt_metadata_exec_stats MNT_METADATA_EXEC_STATS;
struct mnt_metadata_exec_stats
{
  /* Gives the number of elements for the statistic */
  unsigned int statistic_size;
  /* Gives the start offset in the perf_statistics array */
  unsigned int statistic_offset;
  /* Flag that tells if the statistic is accumulative or not */
  bool accumulative;
};

#define MNT_SERVER_PBX_FIX_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 0)
#define MNT_SERVER_PROMOTE_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 1)
#define MNT_SERVER_PROMOTE_TIME_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 2)
#define MNT_SERVER_PBX_UNFIX_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 3)
#define MNT_SERVER_PBX_LOCK_TIME_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 4)
#define MNT_SERVER_PBX_HOLD_TIME_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 5)
#define MNT_SERVER_PBX_FIX_TIME_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 6)
#define MNT_SERVER_MVCC_SNAPSHOT_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 7)
#define MNT_SERVER_OBJ_LOCK_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 8)
#define MNT_SERVER_SNAPSHOT_TIME_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 9)
#define MNT_SERVER_SNAPSHOT_RETRY_CNT_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 10)
#define MNT_SERVER_TRAN_COMPLETE_TIME_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 11)
#define MNT_SERVER_OLDEST_MVCC_TIME_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 12)
#define MNT_SERVER_OLDEST_MVCC_RETRY_CNT_STAT_POSITION \
  (MNT_COUNT_OF_SERVER_EXEC_SINGLE_STATS + 13)

extern void mnt_server_dump_stats (const MNT_SERVER_EXEC_STATS * stats, FILE * stream, const char *substr);

extern void mnt_server_dump_stats_to_buffer (const MNT_SERVER_EXEC_STATS * stats, char *buffer, int buf_size,
					     const char *substr);

extern void mnt_get_current_times (time_t * cpu_usr_time, time_t * cpu_sys_time, time_t * elapsed_time);

extern int mnt_calc_diff_stats (MNT_SERVER_EXEC_STATS * stats_diff, MNT_SERVER_EXEC_STATS * new_stats,
				MNT_SERVER_EXEC_STATS * old_stats);
extern int perfmon_initialize (int num_trans);
extern void perfmon_finalize (void);

#if defined (SERVER_MODE) || defined (SA_MODE)
extern void xperfmon_start_watch (THREAD_ENTRY * thread_p);
extern void xperfmon_stop_watch (THREAD_ENTRY * thread_p);
#endif /* SERVER_MODE || SA_MODE */

extern void perfmon_add_stat (THREAD_ENTRY * thread_p, int amount, PERF_STAT_ID psid);
extern void perfmon_inc_stat (THREAD_ENTRY * thread_p, PERF_STAT_ID psid);
extern void perfmon_set_stat (THREAD_ENTRY * thread_p, int statval, PERF_STAT_ID psid);
extern void perfmon_time_stat (THREAD_ENTRY * thread_p, int timediff, PERF_STAT_ID psid);

#if defined(CS_MODE) || defined(SA_MODE)
/* Client execution statistic structure */
typedef struct mnt_client_stat_info MNT_CLIENT_STAT_INFO;
struct mnt_client_stat_info
{
  time_t cpu_start_usr_time;
  time_t cpu_start_sys_time;
  time_t elapsed_start_time;
  MNT_SERVER_EXEC_STATS *base_server_stats;
  MNT_SERVER_EXEC_STATS *current_server_stats;
  MNT_SERVER_EXEC_STATS *old_global_stats;
  MNT_SERVER_EXEC_STATS *current_global_stats;
};

extern bool mnt_Iscollecting_stats;

extern int mnt_start_stats (bool for_all_trans);
extern int mnt_stop_stats (void);
extern void mnt_reset_stats (void);
extern void mnt_print_stats (FILE * stream);
extern void mnt_print_global_stats (FILE * stream, bool cumulative, const char *substr);
extern MNT_SERVER_EXEC_STATS *mnt_get_stats (void);
extern MNT_SERVER_EXEC_STATS *mnt_get_global_stats (void);
extern int mnt_get_global_diff_stats (MNT_SERVER_EXEC_STATS * diff_stats);
#endif /* CS_MODE || SA_MODE */

#if defined (DIAG_DEVEL)
#if defined(SERVER_MODE)

typedef enum t_diag_obj_type T_DIAG_OBJ_TYPE;
enum t_diag_obj_type
{
  DIAG_OBJ_TYPE_QUERY_OPEN_PAGE = 0,
  DIAG_OBJ_TYPE_QUERY_OPENED_PAGE = 1,
  DIAG_OBJ_TYPE_QUERY_SLOW_QUERY = 2,
  DIAG_OBJ_TYPE_QUERY_FULL_SCAN = 3,
  DIAG_OBJ_TYPE_CONN_CLI_REQUEST = 4,
  DIAG_OBJ_TYPE_CONN_ABORTED_CLIENTS = 5,
  DIAG_OBJ_TYPE_CONN_CONN_REQ = 6,
  DIAG_OBJ_TYPE_CONN_CONN_REJECT = 7,
  DIAG_OBJ_TYPE_BUFFER_PAGE_READ = 8,
  DIAG_OBJ_TYPE_BUFFER_PAGE_WRITE = 9,
  DIAG_OBJ_TYPE_LOCK_DEADLOCK = 10,
  DIAG_OBJ_TYPE_LOCK_REQUEST = 11
};

typedef enum t_diag_value_settype T_DIAG_VALUE_SETTYPE;
enum t_diag_value_settype
{
  DIAG_VAL_SETTYPE_INC,
  DIAG_VAL_SETTYPE_DEC,
  DIAG_VAL_SETTYPE_SET
};

typedef int (*T_DO_FUNC) (int value, T_DIAG_VALUE_SETTYPE settype, char *err_buf);

typedef struct t_diag_object_table T_DIAG_OBJECT_TABLE;
struct t_diag_object_table
{
  char typestring[32];
  T_DIAG_OBJ_TYPE type;
  T_DO_FUNC func;
};

/* MACRO definition */
#define SET_DIAG_VALUE(DIAG_EXEC_FLAG, ITEM_TYPE, VALUE, SET_TYPE, ERR_BUF)          \
    do {                                                                             \
        if (DIAG_EXEC_FLAG == true) {                                             \
            set_diag_value(ITEM_TYPE , VALUE, SET_TYPE, ERR_BUF);                    \
        }                                                                            \
    } while(0)

#define DIAG_GET_TIME(DIAG_EXEC_FLAG, TIMER)                                         \
    do {                                                                             \
        if (DIAG_EXEC_FLAG == true) {                                             \
            gettimeofday(&TIMER, NULL);                                              \
        }                                                                            \
    } while(0)

#define SET_DIAG_VALUE_SLOW_QUERY(DIAG_EXEC_FLAG, START_TIME, END_TIME, VALUE, SET_TYPE, ERR_BUF)\
    do {                                                                                 \
        if (DIAG_EXEC_FLAG == true) {                                                 \
            struct timeval result = {0,0};                                               \
            ADD_TIMEVAL(result, START_TIME, END_TIME);                                   \
            if (result.tv_sec >= diag_long_query_time)                                   \
                set_diag_value(DIAG_OBJ_TYPE_QUERY_SLOW_QUERY, VALUE, SET_TYPE, ERR_BUF);\
        }                                                                                \
    } while(0)

#define SET_DIAG_VALUE_FULL_SCAN(DIAG_EXEC_FLAG, VALUE, SET_TYPE, ERR_BUF, XASL, SPECP) \
    do {                                                                                \
        if (DIAG_EXEC_FLAG == true) {                                                \
            if (((XASL_TYPE(XASL) == BUILDLIST_PROC) ||                                 \
                 (XASL_TYPE(XASL) == BUILDVALUE_PROC))                                  \
                && ACCESS_SPEC_ACCESS(SPECP) == SEQUENTIAL) {                           \
                set_diag_value(DIAG_OBJ_TYPE_QUERY_FULL_SCAN                            \
                        , 1                                                             \
                        , DIAG_VAL_SETTYPE_INC                                          \
                        , NULL);                                                        \
            }                                                                           \
        }                                                                               \
    } while(0)

extern int diag_long_query_time;
extern bool diag_executediag;

extern bool init_diag_mgr (const char *server_name, int num_thread, char *err_buf);
extern void close_diag_mgr (void);
extern bool set_diag_value (T_DIAG_OBJ_TYPE type, int value, T_DIAG_VALUE_SETTYPE settype, char *err_buf);
#endif /* SERVER_MODE */
#endif /* DIAG_DEVEL */

#ifndef DIFF_TIMEVAL
#define DIFF_TIMEVAL(start, end, elapsed) \
    do { \
      (elapsed).tv_sec = (end).tv_sec - (start).tv_sec; \
      (elapsed).tv_usec = (end).tv_usec - (start).tv_usec; \
      if ((elapsed).tv_usec < 0) \
        { \
          (elapsed).tv_sec--; \
          (elapsed).tv_usec += 1000000; \
        } \
    } while (0)
#endif

#define ADD_TIMEVAL(total, start, end) do {     \
  total.tv_usec +=                              \
    (end.tv_usec - start.tv_usec) >= 0 ?        \
      (end.tv_usec-start.tv_usec)               \
    : (1000000 + (end.tv_usec-start.tv_usec));  \
  total.tv_sec +=                               \
    (end.tv_usec - start.tv_usec) >= 0 ?        \
      (end.tv_sec-start.tv_sec)                 \
    : (end.tv_sec-start.tv_sec-1);              \
  total.tv_sec +=                               \
    total.tv_usec/1000000;                      \
  total.tv_usec %= 1000000;                     \
} while(0)

#define TO_MSEC(elapsed) \
  ((int)((elapsed.tv_sec * 1000) + (int) (elapsed.tv_usec / 1000)))

#if defined (EnableThreadMonitoring)
#define MONITOR_WAITING_THREAD(elapsed) \
    (prm_get_integer_value (PRM_ID_MNT_WAITING_THREAD) > 0 \
     && ((elapsed).tv_sec * 1000 + (elapsed).tv_usec / 1000) \
         > prm_get_integer_value (PRM_ID_MNT_WAITING_THREAD))
#else
#define MONITOR_WAITING_THREAD(elapsed) (0)
#endif

typedef struct perf_utime_tracker PERF_UTIME_TRACKER;
struct perf_utime_tracker
{
  bool is_perf_tracking;
  TSC_TICKS start_tick;
  TSC_TICKS end_tick;
};
#define PERF_UTIME_TRACKER_INITIALIZER { false, {0}, {0} }
#define PERF_UTIME_TRACKER_START(thread_p, track) \
  do \
    { \
      (track)->is_perf_tracking = mnt_is_perf_tracking (thread_p); \
      if ((track)->is_perf_tracking) tsc_getticks (&(track)->start_tick); \
    } \
  while (false)
#define PERF_UTIME_TRACKER_TIME(thread_p, track, psid) \
  do \
    { \
      if (!(track)->is_perf_tracking) break; \
      tsc_getticks (&(track)->end_tick); \
      perfmon_time_stat (thread_p, tsc_elapsed_utime ((track)->end_tick,  (track)->start_tick), psid); \
    } \
  while (false)
#define PERF_UTIME_TRACKER_TIME_AND_RESTART(thread_p, track, psid) \
  do \
    { \
      if (!(track)->is_perf_tracking) break; \
      tsc_getticks (&(track)->end_tick); \
      perfmon_time_stat (thread_p, tsc_elapsed_utime ((track)->end_tick,  (track)->start_tick), psid); \
      (track)->start_tick = (track)->end_tick; \
    } \
  while (false)

#if defined(SERVER_MODE) || defined (SA_MODE)
extern int mnt_Num_tran_exec_stats;

#define mnt_is_perf_tracking(thread_p) \
  ((mnt_Num_tran_exec_stats > 0) ? true : false)
/*
 * Statistics at file io level
 */

#define mnt_add_value_to_statistic(thread_p, value, statistic_id) \
  mnt_x_add_value_to_statistic (thread_p, value, statistic_id)

#define mnt_set_statistic(thread_p, value, statistic_id) \
  mnt_x_set_statistic (thread_p, value, statistic_id)

#define mnt_add_in_statistics_array(thread_p, value, statistic_id) \
  mnt_x_add_in_statistics_array (thread_p, value, statistic_id)


/*
 * Statistics at lock level
 */
#define mnt_lk_waited_time_on_objects(thread_p, lock_mode, time_usec) \
  if (mnt_Num_tran_exec_stats > 0) mnt_x_lk_waited_time_on_objects(thread_p, \
								   lock_mode, \
								   time_usec)

/* Execution statistics for the heap manager */

#define mnt_pbx_fix(thread_p,page_type,page_found_mode,latch_mode,cond_type) \
  if (mnt_Num_tran_exec_stats > 0) mnt_x_pbx_fix(thread_p, page_type, \
						 page_found_mode,latch_mode, \
						 cond_type)
#define mnt_pbx_promote(thread_p,page_type,promote_cond,holder_latch,success, amount) \
  if (mnt_Num_tran_exec_stats > 0) mnt_x_pbx_promote(thread_p, page_type, \
						     promote_cond, \
						     holder_latch, \
						     success, amount)
#define mnt_pbx_unfix(thread_p,page_type,buf_dirty,dirtied_by_holder,holder_latch) \
  if (mnt_Num_tran_exec_stats > 0) mnt_x_pbx_unfix(thread_p, page_type, \
						   buf_dirty, \
						   dirtied_by_holder, \
						   holder_latch)
#define mnt_pbx_hold_acquire_time(thread_p,page_type,page_found_mode,latch_mode,amount) \
  if (mnt_Num_tran_exec_stats > 0) mnt_x_pbx_hold_acquire_time(thread_p, \
							       page_type, \
							       page_found_mode, \
							       latch_mode, \
							       amount)
#define mnt_pbx_lock_acquire_time(thread_p,page_type,page_found_mode,latch_mode,cond_type,amount) \
  if (mnt_Num_tran_exec_stats > 0) mnt_x_pbx_lock_acquire_time(thread_p, \
							       page_type, \
							       page_found_mode, \
							       latch_mode, \
							       cond_type, \
							       amount)
#define mnt_pbx_fix_acquire_time(thread_p,page_type,page_found_mode,latch_mode,cond_type,amount) \
  if (mnt_Num_tran_exec_stats > 0) mnt_x_pbx_fix_acquire_time(thread_p, \
							      page_type, \
							      page_found_mode, \
							      latch_mode, \
							      cond_type, \
							      amount)
#if defined(PERF_ENABLE_MVCC_SNAPSHOT_STAT)
#define mnt_mvcc_snapshot(thread_p,snapshot,rec_type,visibility) \
  if (mnt_Num_tran_exec_stats > 0) mnt_x_mvcc_snapshot(thread_p, \
						       snapshot, \
						       rec_type, \
						       visibility)
#endif /* PERF_ENABLE_MVCC_SNAPSHOT_STAT * /

          #define mnt_heap_update_log_time(thread_p,amount) \
          if (mnt_Num_tran_exec_stats > 0) \
          mnt_x_heap_update_log_time (thread_p, amount)

          extern bool mnt_server_is_stats_on (THREAD_ENTRY * thread_p);

          #if defined(ENABLE_UNUSED_FUNCTION)
          extern void mnt_server_print_stats (THREAD_ENTRY * thread_p, FILE * stream);
          #endif
          extern void mnt_x_add_value_to_statistic (THREAD_ENTRY * thread_p, const int value, const int statistic_id);
          extern void mnt_x_set_value_to_statistic (THREAD_ENTRY * thread_p, const int value, const int statistic_id);
          extern UINT64 mnt_get_from_statistic (THREAD_ENTRY * thread_p, const int statistic_id);
          extern void mnt_x_add_in_statistics_array (THREAD_ENTRY * thread_p, UINT64 value, const int statistic_id);

          extern void mnt_x_lk_waited_time_on_objects (THREAD_ENTRY * thread_p, int lock_mode, UINT64 amount);

          extern UINT64 mnt_x_get_stats_and_clear (THREAD_ENTRY * thread_p, const char *stat_name);

          extern void mnt_x_pbx_fix (THREAD_ENTRY * thread_p, int page_type, int page_found_mode, int latch_mode, int cond_type);
          extern void mnt_x_pbx_promote (THREAD_ENTRY * thread_p, int page_type, int promote_cond, int holder_latch, int success,
          UINT64 amount);
          extern void mnt_x_pbx_unfix (THREAD_ENTRY * thread_p, int page_type, int buf_dirty, int dirtied_by_holder,
          int holder_latch);
          extern void mnt_x_pbx_lock_acquire_time (THREAD_ENTRY * thread_p, int page_type, int page_found_mode, int latch_mode,
          int cond_type, UINT64 amount);
          extern void mnt_x_pbx_hold_acquire_time (THREAD_ENTRY * thread_p, int page_type, int page_found_mode, int latch_mode,
          UINT64 amount);
          extern void mnt_x_pbx_fix_acquire_time (THREAD_ENTRY * thread_p, int page_type, int page_found_mode, int latch_mode,
          int cond_type, UINT64 amount);
          #if defined(PERF_ENABLE_MVCC_SNAPSHOT_STAT)
          extern void mnt_x_mvcc_snapshot (THREAD_ENTRY * thread_p, int snapshot, int rec_type, int visibility);
          #endif /* PERF_ENABLE_MVCC_SNAPSHOT_STAT */

#else /* SERVER_MODE || SA_MODE */
#define mnt_add_value_to_statistic(thread_p, value, statistic_id)
#define mnt_set_statistic(thread_p, value, statistic_id)
#define mnt_add_in_statistics_array (thread_p, value, statistic_id)

#define mnt_lk_waited_time_on_objects(thread_p, lock_mode, time_usec)

#define mnt_pbx_fix(thread_p,page_type,page_found_mode,latch_mode,cond_type)
#define mnt_pbx_promote(thread_p,page_type,promote_cond,holder_latch,success,amount)
#define mnt_pbx_unfix(thread_p,page_type,buf_dirty,dirtied_by_holder,holder_latch)
#define mnt_pbx_hold_acquire_time(thread_p,page_type,page_found_mode,latch_mode,amount)
#define mnt_pbx_lock_acquire_time(thread_p,page_type,page_found_mode,latch_mode,cond_type,amount)
#define mnt_pbx_fix_acquire_time(thread_p,page_type,page_found_mode,latch_mode,cond_type,amount)
#if defined(PERF_ENABLE_MVCC_SNAPSHOT_STAT)
#define mnt_mvcc_snapshot(thread_p,snapshot,rec_type,visibility)
#endif /* PERF_ENABLE_MVCC_SNAPSHOT_STAT */

#define mnt_heap_update_log_time(thread_p,amount)

#endif /* CS_MODE */

#endif /* _PERF_MONITOR_H_ */
