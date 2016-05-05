#ifndef _MDB_HASH_TABLE_H
#define _MDB_HASH_TABLE_H
#include <stdlib.h>
#include <string.h>
#include <vector>

/*
 * Constants
 *
 * A hash table has a top-level "directory", each of whose entries points
 * to a "segment" of ssize bucket headers.  The maximum number of hash
 * buckets is thus dsize * ssize (but dsize may be expansible).  Of course,
 * the number of records in the table can be larger, but we don't want a
 * whole lot of records per bucket or performance goes down.
 */
#define DEF_SEGSIZE			   256
#define DEF_SEGSIZE_SHIFT	   8	/* must be log2(DEF_SEGSIZE) */
#define DEF_DIRSIZE			   256
#define DEF_FFACTOR			   1	/* default fill factor */
#define NO_MAX_DSIZE			(-1)


#define MOD(x,y)			   ((x) & ((y)-1))

class TMdbCacheTable;

typedef unsigned int uint32;

typedef struct HASHELEMENT
{
	struct HASHELEMENT *link;	/* link to next entry in same bucket */
	uint32		hashvalue;		/* hash function result for this entry */
	int		data;
} HASHELEMENT;


/* A hash bucket is a linked list of HASHELEMENTs */
typedef HASHELEMENT *HASHBUCKET;

/* A hash segment is an array of bucket headers */
typedef HASHBUCKET *HASHSEGMENT;


typedef struct HASHHDR
{
	
	/* These fields change during entry addition/deletion */
	int		nentries;		/* number of entries in hash table */
	HASHELEMENT *freeList;		/* linked list of free elements */

	/* These fields can change, but not in a partitioned table */
	/* Also, dsize can't change in a shared table, even if unpartitioned */
	int		dsize;			/* directory size */
	int		nsegs;			/* number of allocated segments (<= dsize) */
	uint32		max_bucket;		/* ID of maximum bucket in use */
	uint32		high_mask;		/* mask to modulo into entire table */
	uint32		low_mask;		/* mask to modulo into lower half of table */
	int		ffactor;		/* target fill factor */
	int		max_dsize;		/* 'dsize' limit if directory is fixed size */
	int		ssize;			/* segment size --- must be power of 2 */
	int		sshift;			/* segment shift = log2(ssize) */
	int		nelem_alloc;	/* number of entries to allocate at once */

}HASHHDR;


/* Hash table header struct is an opaque type known only within dynahash.c */
//typedef struct HASHHDR HASHHDR;

/* Hash table control struct is an opaque type known only within dynahash.c */
//typedef struct HTAB HTAB;




class HTAB
{
	public:
		
		HASHHDR    *hctl;			/* => shared control information */
		HASHSEGMENT *dir;			/* directory of segment starts */
		/* We keep local copies of these fixed values to reduce contention */
		int		ssize;			/* segment size --- must be power of 2 */
		int		sshift;			/* segment shift = log2(ssize) */
		TMdbCacheTable * table_data;
		std::vector<void*> heap_addr;
		int hash_search_with_hash_value(uint32  hashvalue,
													int row_num,
													int & find_node);
		bool expand_table();
		bool dir_realloc();
		HASHSEGMENT seg_alloc();
		bool element_alloc(int nelem);
		int string_compare(const char *key1, const char *key2, int  keysize);
		int my_log2(long num);
		int next_pow2_int(long num);
		void hdefault();
		bool init_htab();
		void hash_destroy();
		uint32 calc_bucket(HASHHDR *hctl, uint32 hash_val);
		HASHBUCKET get_hash_entry();
		int hash_get_num_entries();
		int hash_create();
		int set_cache_table(TMdbCacheTable * _table);
};

#endif
