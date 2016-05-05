#include "Helper/mdbHashTable.h"
#include "Helper/mdbCacheTable.h"

int HTAB::set_cache_table(TMdbCacheTable * _table)
{
	table_data = _table;
	return 0;
}
int HTAB::hash_search_with_hash_value(uint32  hashvalue,
												  int row_num,
												  int & find_node)
{
	//HASHHDR    *hctl = hashp->hctl;
	//Size		keysize;
	uint32 bucket;
	long		segment_num;
	long		segment_ndx;
	HASHSEGMENT segp = NULL;
	HASHBUCKET	currBucket = NULL;
	HASHBUCKET *prevBucketPtr = NULL;

	/*
	 * If inserting, check if it is time to split a bucket.
	 *
	 * NOTE: failure to expand table is not a fatal error, it just means we
	 * have to run at higher fill factor than we wanted.
	 */
	if (hctl->nentries / (long) (hctl->max_bucket + 1) >= hctl->ffactor)
	{
		expand_table();
	}


	/*
	 * Do the initial lookup
 	*/
	bucket = calc_bucket(hctl, hashvalue);

	segment_num = bucket >> sshift;
	segment_ndx = MOD(bucket, ssize);


	segp = dir[segment_num];

	if (segp == NULL)
		return -1;

	prevBucketPtr = &segp[segment_ndx];
	currBucket = *prevBucketPtr;


	while (currBucket != NULL)
	{
		if(currBucket->hashvalue == hashvalue &&
		table_data->CompareGroupBy(currBucket->data) == 0)
			break;
		
		prevBucketPtr = &(currBucket->link);
		currBucket = *prevBucketPtr;

	}

	
	if(currBucket != NULL)
	{
		find_node = currBucket->data;
		return 0;
	}



	currBucket = get_hash_entry();
	if (currBucket == NULL)
	{
		//out of memory
		TADD_ERROR(ERR_OS_NO_MEMROY," Mem Not Enough,get_hash_entry failed");
        return ERR_OS_NO_MEMROY;
		
	}

	/* link into hashbucket chain */
	*prevBucketPtr = currBucket;
	currBucket->link = NULL;

		
	currBucket->hashvalue = hashvalue;
	currBucket->data = row_num;


	return 0;	
}


bool HTAB::expand_table()
{
	HASHSEGMENT old_seg = NULL,
				new_seg = NULL;
	long		old_bucket,
				new_bucket;
	long		new_segnum,
				new_segndx;
	long		old_segnum,
				old_segndx;
	HASHBUCKET *oldlink = NULL,
			   *newlink = NULL;
	HASHBUCKET	currElement = NULL,
				nextElement = NULL;


	new_bucket = hctl->max_bucket + 1;
	new_segnum = new_bucket >> sshift;
	new_segndx = MOD(new_bucket, ssize);

	if (new_segnum >= hctl->nsegs)
	{
		/* Allocate new segment if necessary -- could fail if dir full */
		if (new_segnum >= hctl->dsize)
			if (!dir_realloc())
			{
				TADD_ERROR(ERR_OS_NO_MEMROY," Mem Not Enough,expand_table:dir_realloc failed");
				return false;
			}
		if (!(dir[new_segnum] = seg_alloc()))
		{
			TADD_ERROR(ERR_OS_NO_MEMROY," Mem Not Enough,expand_table:seg_alloc failed");
			return false;
		}
		hctl->nsegs++;
	}

	/* OK, we created a new bucket */
	hctl->max_bucket++;

	/*
	 * *Before* changing masks, find old bucket corresponding to same hash
	 * values; values in that bucket may need to be relocated to new bucket.
	 * Note that new_bucket is certainly larger than low_mask at this point,
	 * so we can skip the first step of the regular hash mask calc.
	 */
	old_bucket = (new_bucket & hctl->low_mask);

	/*
	 * If we crossed a power of 2, readjust masks.
	 */
	if ((unsigned int) new_bucket > hctl->high_mask)
	{
		hctl->low_mask = hctl->high_mask;
		hctl->high_mask = (unsigned int ) new_bucket | hctl->low_mask;
	}

	/*
	 * Relocate records to the new bucket.  NOTE: because of the way the hash
	 * masking is done in calc_bucket, only one old bucket can need to be
	 * split at this point.  With a different way of reducing the hash value,
	 * that might not be true!
	 */
	old_segnum = old_bucket >> sshift;
	old_segndx = MOD(old_bucket, ssize);

	old_seg = dir[old_segnum];
	new_seg = dir[new_segnum];

	oldlink = &old_seg[old_segndx];
	newlink = &new_seg[new_segndx];

	for (currElement = *oldlink;
		 currElement != NULL;
		 currElement = nextElement)
	{
		nextElement = currElement->link;
		if ((long) calc_bucket(hctl, currElement->hashvalue) == old_bucket)
		{
			*oldlink = currElement;
			oldlink = &currElement->link;
		}
		else
		{
			*newlink = currElement;
			newlink = &currElement->link;
		}
	}
	/* don't forget to terminate the rebuilt hash chains... */
	*oldlink = NULL;
	*newlink = NULL;

	return true;
}

bool HTAB::dir_realloc()
{
	HASHSEGMENT *p = NULL;
	HASHSEGMENT *old_p = NULL;
	long		new_dsize;
	long		old_dirsize;
	long		new_dirsize;

	if (hctl->max_dsize != -1)
		return false;

	/* Reallocate directory */
	new_dsize = hctl->dsize << 1;
	old_dirsize = hctl->dsize * sizeof(HASHSEGMENT);
	new_dirsize = new_dsize * sizeof(HASHSEGMENT);

	old_p = dir;
	
	p = (HASHSEGMENT *) malloc(new_dirsize);

	if (p != NULL)
	{
		memcpy(p, old_p, old_dirsize);
		memset(((char *) p) + old_dirsize, 0, new_dirsize - old_dirsize);
		dir = p;
		hctl->dsize = new_dsize;
		free(old_p);

		return true;
	}

	return false;
}



HASHSEGMENT HTAB::seg_alloc()
{
	HASHSEGMENT segp = NULL;
	segp = (HASHSEGMENT) malloc(sizeof(HASHBUCKET) * ssize);

	if (!segp)
		return NULL;

	memset(segp, 0, sizeof(HASHBUCKET) * ssize);

	return segp;
}

/*
	* allocate some new elements and link them into the free list
 */


bool HTAB::element_alloc(int nelem)
{
	
	HASHHDR *hctlv = hctl;
	int		elementSize;
	HASHELEMENT *firstElement = NULL;
	HASHELEMENT *tmpElement = NULL;
	HASHELEMENT *prevElement = NULL;
	int			i;

	
	/* Each element has a HASHELEMENT header plus user data. */
	elementSize = sizeof(HASHELEMENT);
	
	firstElement = (HASHELEMENT *) malloc(nelem * elementSize);

	if (!firstElement)
		return false;

	//save address to free
	heap_addr.push_back(firstElement);
	
	/* prepare to link all the new entries into the freelist */
	prevElement = NULL;
	tmpElement = firstElement;
	for (i = 0; i < nelem; i++)
	{
		tmpElement->link = prevElement;
		prevElement = tmpElement;
		tmpElement = (HASHELEMENT *) (((char *) tmpElement) + elementSize);
	}

	firstElement->link = hctlv->freeList;
	hctlv->freeList = prevElement;

	return true;
}

int HTAB::string_compare(const char *key1, const char *key2, int  keysize)
{
	return strncmp(key1, key2, keysize - 1);
}


int HTAB::my_log2(long num)
{
	int			i;
	long		limit;


	for (i = 0, limit = 1; limit < num; i++, limit <<= 1)
		;
	return i;
}


int HTAB::next_pow2_int(long num)
{
	return 1 << my_log2(num);
}


void HTAB::hdefault()
{
	memset(hctl, 0, sizeof(HASHHDR));

	hctl->nentries = 0;
	hctl->freeList = NULL;

	hctl->dsize = DEF_DIRSIZE;
	hctl->nsegs = 0;

	hctl->ffactor = DEF_FFACTOR;

	/* table has no fixed maximum size */
	hctl->max_dsize = -1;

	hctl->ssize = DEF_SEGSIZE;
	hctl->sshift = DEF_SEGSIZE_SHIFT;

}


/*
* Compute derived fields of hctl and build the initial directory/segment
* arrays
*/
bool HTAB::init_htab()
{
	HASHSEGMENT *segp = NULL;
	int			nbuckets;
	int			nsegs;
	int 		nelem =  65536;
	
	/*
	 * Divide number of elements by the fill factor to determine a desired
	 * number of buckets.  Allocate space for the next greater power of two
	 * number of buckets
	 */
	nbuckets = next_pow2_int((nelem - 1) / hctl->ffactor + 1);

	

	hctl->max_bucket = hctl->low_mask = nbuckets - 1;
	hctl->high_mask = (nbuckets << 1) - 1;

	/*
	 * Figure number of directory segments needed, round up to a power of 2
	 */
	nsegs = (nbuckets - 1) / hctl->ssize + 1;
	nsegs = next_pow2_int(nsegs);

	/*
	 * Make sure directory is big enough. If pre-allocated directory is too
	 * small, choke (caller screwed up).
	 */
	if (nsegs > hctl->dsize)
	{
		if (!(dir))
			hctl->dsize = nsegs;
		else
			return false;
	}

	/* Allocate a directory */
	if (!(dir))
	{
		dir = (HASHSEGMENT *)malloc(hctl->dsize * sizeof(HASHSEGMENT));
		if (!dir)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY," Mem Not Enough,init_htab:dir malloc failed");
			return false;
		}
	}

	/* Allocate initial segments */
	//hctl->nsegs initial value 0
	for (segp = dir; hctl->nsegs < nsegs; hctl->nsegs++, segp++)
	{
		*segp = seg_alloc();
		if (*segp == NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY," Mem Not Enough,init_htab:seg_alloc failed");
			return false;
		}
	}

	/* Choose number of entries to allocate at a time */
	//hctl->nelem_alloc = choose_nelem_alloc(hctl->entrysize);
	hctl->nelem_alloc = 32;

	return true;
}

void HTAB::hash_destroy()
{
	int i;
	//hash table destroy
	for(i=0; i<hctl->nsegs; i++)
	{
		free(dir[i]);
		
	}

	
	free(dir);

	free(hctl);

	// element destroy
	for(i=0; i<heap_addr.size(); i++)
		free(heap_addr[i]);

				
	
}

/* Convert a hash value to a bucket number */
uint32 HTAB::calc_bucket(HASHHDR *hctl, uint32 hash_val)
{
	uint32		bucket;

	bucket = hash_val & hctl->high_mask;
	if (bucket > hctl->max_bucket)
		bucket = bucket & hctl->low_mask;

	return bucket;
}


/*
* create a new entry if possible
*/
HASHBUCKET HTAB::get_hash_entry()
{
	
	HASHHDR *hctlv = hctl;
	HASHBUCKET	newElement = NULL;

	for (;;)
	{
		

		/* try to get an entry from the freelist */
		newElement = hctlv->freeList;
		if (newElement != NULL)
			break;
		if (!element_alloc(hctlv->nelem_alloc))
		{
			/* out of memory */
			TADD_ERROR(ERR_OS_NO_MEMROY,"get_hash_entry:element_alloc failed");
			return NULL;
		}
	}

	/* remove entry from freelist, bump nentries */
	hctlv->freeList = newElement->link;
	hctlv->nentries++;
	return newElement;
}



/*
 * hash_get_num_entries -- get the number of entries in a hashtable
 */
int HTAB::hash_get_num_entries()
{
	
	return hctl->nentries;
}



int HTAB::hash_create()
{
	
	
	memset(this, 0, sizeof(HTAB));

	hctl = NULL;
	dir = NULL;

	heap_addr.clear();
	
	if (!hctl)
	{
		hctl = (HASHHDR *) malloc(sizeof(HASHHDR));
		if (hctl == NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"hash_create:HASHHDR malloc failed\n");
			return -1;
		}
	}

	hdefault();


	
	/* make local copies of heavily-used constant fields */
	ssize = hctl->ssize;
	sshift = hctl->sshift;
	
	/* Build the hash directory structure */
	if (!init_htab())
	{
		TADD_ERROR(ERR_OS_NO_MEMROY,"hash_create:init_htab failed\n");
		return -2;
	}
	
	if (!element_alloc(hctl->nelem_alloc))
	{
		TADD_ERROR(ERR_OS_NO_MEMROY,"hash_create:element_alloc failed\n");
		return -1;
	}
	
	return 0;
}









