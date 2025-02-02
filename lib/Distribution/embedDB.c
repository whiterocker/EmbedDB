#include "./embedDB.h"

/******************************************************************************/
/**
 * @file        EmbedDB-Amalgamation
 * @author      EmbedDB Team (See Authors.md)
 * @brief       Source code amalgamated into one file for easy distribution
 * @copyright   Copyright 2024
 *              EmbedDB Team
 * @par Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 * @par 1.Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * @par 2.Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * @par 3.Neither the name of the copyright holder nor the names of its contributors
 *  may be used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 * @par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/  
/************************************************************spline.c************************************************************/
/******************************************************************************/
/**
 * @file        spline.c
 * @author      EmbedDB Team (See Authors.md)
 * @brief       Implementation of spline.
 * @copyright   Copyright 2024
 *              EmbedDB Team
 * @par Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 * @par 1.Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * @par 2.Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * @par 3.Neither the name of the copyright holder nor the names of its contributors
 *  may be used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 * @par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/    

#if defined(ARDUINO) 
#endif

/**
 * @brief    Initialize a spline structure with given maximum size and error.
 * @param    spl        Spline structure
 * @param    size       Maximum size of spline
 * @param    maxError   Maximum error allowed in spline
 * @param    keySize    Size of key in bytes
 */
void
splineInit(spline * spl,
	   pgid_t   size,
	   size_t   maxError,
	   uint8_t  keySize)
{
  uint8_t pointSize = sizeof(uint32_t) + keySize;
  if (spl && EDB_WITH_HEAP) {
    spl->count = 0;
    spl->pointsStartIndex = 0;
    spl->eraseSize = 1;
    spl->size = size;
    spl->maxError = maxError;
    spl->points = (void *)malloc(pointSize * size);
    spl->tempLastPoint = 0;
    spl->keySize = keySize;
    spl->lastKey = malloc(keySize);
    spl->lower = malloc(pointSize);
    spl->upper = malloc(pointSize);
    spl->firstSplinePoint = malloc(pointSize);
    spl->numAddCalls = 0;
  }
}

/**
 * @brief    Check if first line is to the left (counter-clockwise) of the second.
 */
static inline int8_t
splineIsLeft(uint64_t x1, int64_t y1,
	     uint64_t x2, int64_t y2)
{
  return y1 * x2 > y2 * x1;
}

/**
 * @brief    Check if first line is to the right (clockwise) of the second.
 */
static inline int8_t
splineIsRight(uint64_t x1, int64_t y1,
	      uint64_t x2, int64_t y2)
{
  return y1 * x2 < y2 * x1;
}

/**
 * @brief   Adds point to spline structure
 * @param   spl     Spline structure
 * @param   key     Data key to be added (must be incrementing)
 * @param   page    Page number for spline point to add
 */
void
splineAdd(spline * spl,
	  void *   key,
	  uint32_t page)
{
  spl->numAddCalls++;
  /* Check if no spline points are currently empty */
  if (spl->numAddCalls == 1) {
    /* Add first point in data set to spline. */
    void *firstPoint = splinePointLocation(spl, 0);
    memcpy(firstPoint, key, spl->keySize);
    memcpy(((int8_t *)firstPoint + spl->keySize), &page, sizeof(uint32_t));
    /* Log first point for wrap around purposes */
    memcpy(spl->firstSplinePoint, key, spl->keySize);
    memcpy(((int8_t *)spl->firstSplinePoint + spl->keySize), &page, sizeof(uint32_t));
    spl->count++;
    memcpy(spl->lastKey, key, spl->keySize);
    return;
  }
  
  /* Check if there is only one spline point (need to initialize upper
     and lower limits using 2nd point) */
  if (spl->numAddCalls == 2) {
    /* Initialize upper and lower limits using second (unique) data point */
    memcpy(spl->lower, key, spl->keySize);
    uint32_t lowerPage = page < spl->maxError ? 0 : page - spl->maxError;
    memcpy(((int8_t *)spl->lower + spl->keySize), &lowerPage, sizeof(uint32_t));
    memcpy(spl->upper, key, spl->keySize);
    uint32_t upperPage = page + spl->maxError;
    memcpy(((int8_t *)spl->upper + spl->keySize), &upperPage, sizeof(uint32_t));
    memcpy(spl->lastKey, key, spl->keySize);
    spl->lastLoc = page;
  }
  
  /* Skip duplicates */
  uint64_t keyVal = 0, lastKeyVal = 0;
  memcpy(&keyVal, key, spl->keySize);
  memcpy(&lastKeyVal, spl->lastKey, spl->keySize);
  
  if (keyVal <= lastKeyVal && spl->numAddCalls != 2)
    return;
  
  /* Last point added to spline, check if previous point is temporary
     - overwrite previous point if temporary */
  if (spl->tempLastPoint != 0) {
    spl->count--;
  }
  
  uint32_t lastPage = 0;
  uint64_t lastPointKey = 0, upperKey = 0, lowerKey = 0;
  void *lastPointLocation = splinePointLocation(spl, spl->count - 1);
  memcpy(&lastPointKey, lastPointLocation, spl->keySize);
  memcpy(&upperKey, spl->upper, spl->keySize);
  memcpy(&lowerKey, spl->lower, spl->keySize);
  memcpy(&lastPage, (int8_t *)lastPointLocation + spl->keySize, sizeof(uint32_t));
  
  uint64_t xdiff, upperXDiff, lowerXDiff = 0;
  uint32_t ydiff, upperYDiff = 0;
  int64_t lowerYDiff = 0; /* This may be negative */
  
  xdiff = keyVal - lastPointKey;
  ydiff = page - lastPage;
  upperXDiff = upperKey - lastPointKey;
  memcpy(&upperYDiff, (int8_t *)spl->upper + spl->keySize, sizeof(uint32_t));
  upperYDiff -= lastPage;
  lowerXDiff = lowerKey - lastPointKey;
  memcpy(&lowerYDiff, (int8_t *)spl->lower + spl->keySize, sizeof(uint32_t));
  lowerYDiff -= lastPage;
  
  if (spl->count >= spl->size) {
    (void) splineErase(spl, spl->eraseSize);
  }
  
  /* Check if next point still in error corridor */
  if (splineIsLeft(xdiff, ydiff, upperXDiff, upperYDiff) == 1 ||
      splineIsRight(xdiff, ydiff, lowerXDiff, lowerYDiff) == 1) {
    /* Point is not in error corridor. Add previous point to spline. */
    void *nextSplinePoint = splinePointLocation(spl, spl->count);
    memcpy(nextSplinePoint, spl->lastKey, spl->keySize);
    memcpy((int8_t *)nextSplinePoint + spl->keySize, &spl->lastLoc, sizeof(uint32_t));
    spl->count++;
    spl->tempLastPoint = 0;
    
    /* Update upper and lower limits. */
    memcpy(spl->lower, key, spl->keySize);
    uint32_t lowerPage = page < spl->maxError ? 0 : page - spl->maxError;
    memcpy((int8_t *)spl->lower + spl->keySize, &lowerPage, sizeof(uint32_t));
    memcpy(spl->upper, key, spl->keySize);
    uint32_t upperPage = page + spl->maxError;
    memcpy((int8_t *)spl->upper + spl->keySize, &upperPage, sizeof(uint32_t));
    
    /* If we add a point, we might need to erase again */
    if (spl->count >= spl->size) {
      (void) splineErase(spl, spl->eraseSize);
    }
    
  }
  else {
    /* Check if must update upper or lower limits */
    
    /* Upper limit */
    if (splineIsLeft(upperXDiff, upperYDiff, xdiff, page + spl->maxError - lastPage) == 1) {
      memcpy(spl->upper, key, spl->keySize);
      uint32_t upperPage = page + spl->maxError;
      memcpy((int8_t *)spl->upper + spl->keySize, &upperPage, sizeof(uint32_t));
    }
    
    /* Lower limit */
    if (splineIsRight(lowerXDiff, lowerYDiff, xdiff,
		      (page < spl->maxError ? 0 : page - spl->maxError) - lastPage) == 1) {
      memcpy(spl->lower, key, spl->keySize);
      uint32_t lowerPage = page < spl->maxError ? 0 : page - spl->maxError;
      memcpy((int8_t *)spl->lower + spl->keySize, &lowerPage, sizeof(uint32_t));
    }
  }
  
  spl->lastLoc = page;
  
  /* Add last key on spline if not already there. */
  /* This will get overwritten the next time a new spline point is added */
  memcpy(spl->lastKey, key, spl->keySize);
  void *tempSplinePoint = splinePointLocation(spl, spl->count);
  memcpy(tempSplinePoint, spl->lastKey, spl->keySize);
  memcpy((int8_t *)tempSplinePoint + spl->keySize, &spl->lastLoc, sizeof(uint32_t));
  spl->count++;
  
  spl->tempLastPoint = 1;
}

/**
 * @brief   Removes points from the spline
 * @param   spl         The spline structure to search
 * @param   numPoints   The number of points to remove from the spline
 * @return  Returns zero if successful and one if not
 */
int
splineErase(spline * spl,
	    uint32_t numPoints)
{
  /* If the user tries to delete more points than they allocated or
     deleting would only leave one spline point */
  if (numPoints > spl->count || spl->count - numPoints == 1)
    return 1;
  if (numPoints == 0)
    return 0;
  
  spl->count -= numPoints;
  spl->pointsStartIndex = (spl->pointsStartIndex + numPoints) % spl->size;
  if (spl->count == 0)
    spl->numAddCalls = 0;
  return 0;
}

/**
 * @brief	Builds a spline structure given a sorted data set. GreedySplineCorridor
 * implementation from "Smooth interpolating histograms with error guarantees"
 * (BNCOD'08) by T. Neumann and S. Michel.
 * @param	spl			Spline structure
 * @param	data		Array of sorted data
 * @param	size		Number of values in array
 * @param	maxError	Maximum error for each spline
 */
void
splineBuild(spline * spl,
	    void **  data,
	    pgid_t   size,
	    size_t   maxError)
{
  spl->maxError = maxError;
  
  for (pgid_t i = 0; i < size; i++) {
    void *key;
    memcpy(&key, data + i, sizeof(void *));
    splineAdd(spl, key, i);
  }
}

/**
 * @brief    Print a spline structure.
 * @param    spl     Spline structure
 */
void
splinePrint(spline *spl)
{
  if (!spl) {
    EDB_PRINTF("No spline to print.\n");
    return;
  }
  EDB_PRINTF("Spline max error (%" PRIu32 "):\n", spl->maxError);
  EDB_PRINTF("Spline points (%zu):\n", spl->count);
  uint64_t keyVal = 0;
  uint32_t page = 0;
  for (uint32_t i = 0; i < spl->count; i++) {
    void *point = splinePointLocation(spl, i);
    memcpy(&keyVal, point, spl->keySize);
    memcpy(&page, (int8_t *)point + spl->keySize, sizeof(uint32_t));
    EDB_PRINTF("[%" PRIu32 "]: (%" PRIu64 ", %" PRIu32 ")\n", i, keyVal, page);
  }
  EDB_PRINTF("\n");
}

/**
 * @brief    Return spline structure size in bytes.
 * @param    spl     Spline structure
 * @return   size of the spline in bytes
 */
uint32_t
splineSize(spline * spl)
{
  return sizeof(spline) + (spl->size * (spl->keySize + sizeof(uint32_t)));
}

/**
 * @brief	Performs a recursive binary search on the spine points for a key
 * @param	arr			Array of spline points to search through
 * @param	low		    Lower search bound (Index of spline point)
 * @param	high	    Higher search bound (Index of spline point)
 * @param	key		    Key to search for
 * @param	compareKey	Function to compare keys
 * @return	Index of spline point that is the upper end of the spline segment that contains the key
 */
static size_t
pointsBinarySearch(spline * spl,
		   int      low,
		   int      high,
		   void *   key,
		   int8_t   compareKey(void *, void *)) {
  int32_t mid;
  if (high >= low) {
    mid = low + (high - low) / 2;
    
    // If mid is zero, then low = 0 and high = 1. Therefore there is
    // only one spline segment and we return 1, the upper bound.
    if (mid == 0) {
      return 1;
    }
    
    void *midSplinePoint = splinePointLocation(spl, mid);
    void *midSplineMinusOnePoint = splinePointLocation(spl, mid - 1);
    
    if (compareKey(midSplinePoint, key) >= 0 && compareKey(midSplineMinusOnePoint, key) <= 0)
      return mid;
    
    if (compareKey(midSplinePoint, key) > 0)
      return pointsBinarySearch(spl, low, mid - 1, key, compareKey);
    
    return pointsBinarySearch(spl, mid + 1, high, key, compareKey);
  }
  
  mid = low + (high - low) / 2;
  if (mid >= high) {
    return high;
  } else {
    return low;
  }
}

/**
 * @brief	Estimate the page number of a given key
 * @param	spl			The spline structure to search
 * @param	key			The key to search for
 * @param	compareKey	Function to compare keys
 * @param	loc			A return value for the best estimate of which page the key is on
 * @param	low			A return value for the smallest page that it could be on
 * @param	high		A return value for the largest page it could be on
 */
void
splineFind(spline * spl,
	   void *   key,
	   int8_t   compareKey(void *, void *),
	   pgid_t * loc,
	   pgid_t * low,
	   pgid_t * high)
{
  size_t pointIdx;
  uint64_t keyVal = 0, smallestKeyVal = 0, largestKeyVal = 0;
  void *smallestSplinePoint = splinePointLocation(spl, 0);
  void *largestSplinePoint = splinePointLocation(spl, spl->count - 1);
  memcpy(&keyVal, key, spl->keySize);
  memcpy(&smallestKeyVal, smallestSplinePoint, spl->keySize);
  memcpy(&largestKeyVal, largestSplinePoint, spl->keySize);
  
  if (compareKey(key, splinePointLocation(spl, 0)) < 0 || spl->count <= 1) {
    // Key is smaller than any we have on record
    uint32_t lowEstimate, highEstimate, locEstimate = 0;
    memcpy(&lowEstimate, (int8_t *)spl->firstSplinePoint + spl->keySize, sizeof(uint32_t));
    memcpy(&highEstimate, (int8_t *)smallestSplinePoint + spl->keySize, sizeof(uint32_t));
    locEstimate = (lowEstimate + highEstimate) / 2;
    
    memcpy(loc, &locEstimate, sizeof(uint32_t));
    memcpy(low, &lowEstimate, sizeof(uint32_t));
    memcpy(high, &highEstimate, sizeof(uint32_t));
    return;
  } else if (compareKey(key, splinePointLocation(spl, spl->count - 1)) > 0) {
    memcpy(loc, (int8_t *)largestSplinePoint + spl->keySize, sizeof(uint32_t));
    memcpy(low, (int8_t *)largestSplinePoint + spl->keySize, sizeof(uint32_t));
    memcpy(high, (int8_t *)largestSplinePoint + spl->keySize, sizeof(uint32_t));
    return;
  } else {
    // Perform a binary seach to find the spline point above the key
    // we're looking for
    pointIdx = pointsBinarySearch(spl, 0, spl->count - 1, key, compareKey);
  }
  
  // Interpolate between two spline points
  void *downKey = splinePointLocation(spl, pointIdx - 1);
  uint32_t downPage = 0;
  memcpy(&downPage, (int8_t *)downKey + spl->keySize, sizeof(uint32_t));
  void *upKey = splinePointLocation(spl, pointIdx);
  uint32_t upPage = 0;
  memcpy(&upPage, (int8_t *)upKey + spl->keySize, sizeof(uint32_t));
  uint64_t downKeyVal = 0, upKeyVal = 0;
  memcpy(&downKeyVal, downKey, spl->keySize);
  memcpy(&upKeyVal, upKey, spl->keySize);
  
  // Estimate location as page number
  // Keydiff * slope + y
  pgid_t locationEstimate =
    (pgid_t)((keyVal - downKeyVal) * (upPage - downPage) /
	   (long double)(upKeyVal - downKeyVal)) + downPage;
  memcpy(loc, &locationEstimate, sizeof(pgid_t));
  
  // Set error bounds based on maxError from spline construction
  pgid_t lowEstiamte = (spl->maxError > locationEstimate) ? 0 : locationEstimate - spl->maxError;
  memcpy(low, &lowEstiamte, sizeof(pgid_t));
  void *lastSplinePoint = splinePointLocation(spl, spl->count - 1);
  uint32_t lastSplinePointPage = 0;
  memcpy(&lastSplinePointPage, (int8_t *)lastSplinePoint + spl->keySize, sizeof(uint32_t));
  pgid_t highEstimate = (locationEstimate + spl->maxError > lastSplinePointPage) ?
    lastSplinePointPage : locationEstimate + spl->maxError;
  memcpy(high, &highEstimate, sizeof(pgid_t));
}

/**
 * @brief    Free memory allocated for spline structure.
 * @param    spl        Spline structure
 */
void
splineClose(spline * spl)
{
  if (spl && EDB_WITH_HEAP) {
    free(spl->points);
    free(spl->lastKey);
    free(spl->lower);
    free(spl->upper);
    free(spl->firstSplinePoint);
  }
}

/**
 * @brief  Returns a pointer to the location of the specified spline
 *         point in memory. Note that this method does not check if
 *         there is a point there, so it may be garbage data.
 * @param   spl         The spline structure that contains the points
 * @param   pointIndex  The index of the point to return a pointer to
 */
void *
splinePointLocation(spline * spl,
		    size_t   pointIndex)
{
  return (int8_t *)spl->points + (((pointIndex + spl->pointsStartIndex) % spl->size) *
				  (spl->keySize + sizeof(uint32_t)));
}

/************************************************************embedDB.c************************************************************/
/******************************************************************************/
/**
 * @file        embedDB.c
 * @author      EmbedDB Team (See Authors.md)
 * @brief       Source code for EmbedDB.
 * @copyright   Copyright 2024
 *              EmbedDB Team
 * @par Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 * @par 1.Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * @par 2.Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * @par 3.Neither the name of the copyright holder nor the names of its contributors
 *  may be used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 * @par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/         

#if defined(ARDUINO) 
#endif

/* Helper Functions */
static int8_t   embedDBInitData(embedDBState *state);
static int8_t   embedDBInitDataFromFile(embedDBState *state);
static int8_t   embedDBInitDataFromFileWithRecordLevelConsistency(embedDBState *state);
static int8_t   embedDBInitIndex(embedDBState *state);
static int8_t   embedDBInitIndexFromFile(embedDBState *state);
static int8_t   embedDBInitVarData(embedDBState *state);
static int8_t   embedDBInitVarDataFromFile(embedDBState *state);
static int8_t   shiftRecordLevelConsistencyBlocks(embedDBState *state);
static void     embedDBInitSplineFromFile(embedDBState *state);
static int32_t  getMaxError(embedDBState *state, void *buffer);
static void     updateMaxiumError(embedDBState *state, void *buffer);
static int8_t   embedDBSetupVarDataStream(embedDBState *state, void *key,
					  embedDBVarDataStream **varData, pgid_t recordNumber);
static uint32_t cleanSpline(embedDBState *state, uint32_t minPageNumber);
static void     readToWriteBuf(embedDBState *state);
static void     readToWriteBufVar(embedDBState *state);

static void
printBitmap(char *bm)
{
  for (int8_t i = 0; i <= 7; i++) {
    EDB_PRINTF(" " BYTE_TO_BINARY_PATTERN "", BYTE_TO_BINARY(*(bm + i)));
  }
  EDB_PRINTF("\n");
}

/**
 * @brief	Determine if two bitmaps have any overlapping bits
 * @return	1 if there is any overlap, else 0
 */
static int8_t
bitmapOverlap(uint8_t * bm1,
	      uint8_t * bm2,
	      int8_t    size)
{
  for (int8_t i = 0; i < size; i++)
    if ((*((uint8_t *)(bm1 + i)) & *((uint8_t *)(bm2 + i))) >= 1)
      return 1;
  
  return 0;
}

static void
initBufferPage(embedDBState * state,
	       int            pageNum)
{
  /* Initialize page */
  uint16_t i = 0;
  void *buf = (char *)state->buffer + pageNum * state->pageSize;
  
  for (i = 0; i < state->pageSize; i++) {
    ((int8_t *)buf)[i] = 0;
  }
  
  if (pageNum != EMBEDDB_VAR_WRITE_BUFFER(state->parameters)) {
    /* Initialize header key min. Max and sum is already set to zero by the
     * for-loop above */
    void *min = EMBEDDB_GET_MIN_KEY(buf);
    /* Initialize min to all 1s */
    for (i = 0; i < state->keySize; i++) {
      ((int8_t *)min)[i] = 1;
    }
    
    /* Initialize data min. */
    min = EMBEDDB_GET_MIN_DATA(buf, state);
    /* Initialize min to all 1s */
    for (i = 0; i < state->dataSize; i++) {
      ((int8_t *)min)[i] = 1;
    }
  }
}

/**
 * @brief   Return the smallest key in the node
 * @param   state   embedDB algorithm state structure
 * @param   buffer  In memory page buffer with node data
 */
static void *
embedDBGetMinKey(embedDBState * state,
		 void *         buffer)
{
  return (void *)((int8_t *)buffer + state->headerSize);
}

/**
 * @brief   Return the largest key in the node
 * @param   state   embedDB algorithm state structure
 * @param   buffer  In memory page buffer with node data
 */
static void *
embedDBGetMaxKey(embedDBState * state,
		 void *         buffer)
{
  int16_t count = EMBEDDB_GET_COUNT(buffer);
  return (void *)((int8_t *)buffer + state->headerSize + (count - 1) * state->recordSize);
}

/**
 * @brief   Initialize embedDB structure.
 * @param   state           embedDB algorithm state structure
 * @param   indexMaxError   max error of indexing structure (spline)
 * @return  Return 0 if success. Non-zero value if error.
 */
int8_t
embedDBInit(embedDBState * state,
	    size_t         indexMaxError)
{
  if (state->keySize > 8) {
    EDB_PERRF("ERROR: Key size is too large. Max key size is 8 bytes.\n");
    return -1;
  }
  
  /* check the number of allocated pages is a multiple of the erase size */
  if (state->numDataPages % state->eraseSizeInPages) {
    EDB_PERRF("ERROR: The number of allocated data pages must be "
	      "divisible by the erase size in pages.\n");
    return -1;
  }
  
  if (state->numDataPages <
      (EMBEDDB_USING_RECORD_LEVEL_CONSISTENCY(state->parameters) ? 4 : 2) *
      state->eraseSizeInPages) {
    EDB_PERRF("ERROR: The minimum number of data pages is twice "
	      "the eraseSizeInPages or 4 times the eraseSizeInPages "
	      "if using record-level consistency.\n");
    return -1;
  }
  
  state->recordSize = state->keySize + state->dataSize;
  if (EMBEDDB_USING_VDATA(state->parameters)) {
    if (state->numVarPages % state->eraseSizeInPages != 0) {
      EDB_PERRF("ERROR: The number of allocated variable data pages must "
		"be divisible by the erase size in pages.\n");
      return -1;
    }
    state->recordSize += 4;
  }
  
  state->indexMaxError = indexMaxError;
  
  /* Calculate block header size */
  
  /* Header size depends on bitmap size: 6 + X bytes: 4 byte id, 2 for record count, X for bitmap. */
  state->headerSize = 6;
  if (EMBEDDB_USING_INDEX(state->parameters)) {
    if (state->numIndexPages % state->eraseSizeInPages != 0) {
      EDB_PERRF("ERROR: The number of allocated index pages must be "
		"divisible by the erase size in pages.\n");
      return -1;
    }
    state->headerSize += state->bitmapSize;
  }
  
  if (EMBEDDB_USING_MAX_MIN(state->parameters))
    state->headerSize += state->keySize * 2 + state->dataSize * 2;
  
  /* Flags to show that these values have not been initalized with actual data yet */
  state->bufferedPageId = -1;
  state->bufferedIndexPageId = -1;
  state->bufferedVarPage = -1;
  
  /* Calculate number of records per page */
  state->maxRecordsPerPage = (state->pageSize - state->headerSize) / state->recordSize;
  
  /* Initialize max error to maximum records per page */
  state->maxError = state->maxRecordsPerPage;
  
  /* Allocate first page of buffer as output page */
  initBufferPage(state, 0);
  
  if (state->numDataPages < (EMBEDDB_USING_INDEX(state->parameters) * 2 + 2) * state->eraseSizeInPages) {
    EDB_PERRF("ERROR: Number of pages allocated must be at least twice "
	      "erase block size for embedDB and four times when using indexing. "
	      "Memory pages: %" PRIu32 "\n", state->numDataPages);
    return -1;
  }
  
  /* Initalize the spline structure if being used */
  if (EMBEDDB_USING_SPLINE(state->parameters)) {
    if (EDB_WITH_HEAP) {
      if (state->numSplinePoints < 4) {
	EDB_PERRF("ERROR: Unable to setup spline with less than 4 points.");
	return -1;
      }
      state->spl = malloc(sizeof(spline));
      splineInit(state->spl, state->numSplinePoints, indexMaxError, state->keySize);
    }
    else {
      EDB_PERRF("ERROR: EDB_NO_HEAP: dynamically-allocated splines not available.");
    }
  }

  /* Allocate file for data*/
  int8_t dataInitResult = 0;
  dataInitResult = embedDBInitData(state);
  
  if (dataInitResult != 0) {
    return dataInitResult;
  }
  
  /* Allocate file and buffer for index */
  int8_t indexInitResult = 0;
  if (EMBEDDB_USING_INDEX(state->parameters)) {
    if (state->bufferSizeInBlocks < 4) {
      EDB_PERRF("ERROR: embedDB using index requires at least 4 page buffers.\n");
      return -1;
    } else {
      indexInitResult = embedDBInitIndex(state);
    }
  } else {
    state->indexFile = NULL;
    state->numIndexPages = 0;
  }
  
  if (indexInitResult != 0) {
    return indexInitResult;
  }
  
  /* Allocate file and buffer for variable data */
  int8_t varDataInitResult = 0;
  if (EMBEDDB_USING_VDATA(state->parameters)) {
    if (state->bufferSizeInBlocks < 4 + (EMBEDDB_USING_INDEX(state->parameters) ? 2 : 0)) {
      EDB_PERRF("ERROR: embedDB using variable records requires at least "
		"4 page buffers if there is no index and 6 if there is.\n");
      return -1;
    } else {
      varDataInitResult = embedDBInitVarData(state);
    }
    return varDataInitResult;
  } else {
    state->varFile = NULL;
    state->numVarPages = 0;
  }
  
  embedDBResetStats(state);
  return 0;
}

static int8_t
embedDBInitData(embedDBState *state)
{
  state->nextDataPageId = 0;
  state->nextDataPageId = 0;
  state->numAvailDataPages = state->numDataPages;
  state->minDataPageId = 0;
  
  if (state->dataFile == NULL) {
    EDB_PERRF("ERROR: No data file provided!\n");
    return -1;
  }
  
  if (EMBEDDB_USING_RECORD_LEVEL_CONSISTENCY(state->parameters)) {
    state->numAvailDataPages -= (state->eraseSizeInPages * 2);
    state->nextRLCPhysicalPageLocation = state->eraseSizeInPages;
    state->rlcPhysicalStartingPage = state->eraseSizeInPages;
  }
  
  /* Setup data file. */
  int8_t openStatus = 0;
  if (!EMBEDDB_RESETING_DATA(state->parameters)) {
    openStatus = state->fileInterface->open(state->dataFile, EMBEDDB_FILE_MODE_R_PLUS_B);
    if (openStatus) {
      if (EMBEDDB_USING_RECORD_LEVEL_CONSISTENCY(state->parameters)) {
	return embedDBInitDataFromFileWithRecordLevelConsistency(state);
      } else {
	return embedDBInitDataFromFile(state);
      }
    }
  } else {
    openStatus = state->fileInterface->open(state->dataFile, EMBEDDB_FILE_MODE_W_PLUS_B);
  }
  
  if (!openStatus) {
    EDB_PERRF("Error: Can't open data file!\n");
    return -1;
  }
  
  return 0;
}

static int8_t
embedDBInitDataFromFile(embedDBState *state)
{
  pgid_t logicalPageId = 0;
  pgid_t maxLogicalPageId = 0;
  pgid_t physicalPageId = 0;
  uint32_t count = 0;
  count_t blockSize = state->eraseSizeInPages;
  bool validData = false;
  bool hasData = false;
  void *buffer = (int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_READ_BUFFER;
  
  /* This will become zero if there is no more to read */
  int8_t moreToRead = !(readPage(state, physicalPageId));
  
  /* this handles the case where the first page may have been erased,
     so has junk data and we actually need to start from the second
     page */
  uint32_t i = 0;
  int8_t numRecords = 0;
  while (moreToRead && i < 2) {
    memcpy(&logicalPageId, buffer, sizeof(pgid_t));
    validData = logicalPageId % state->numDataPages == count;
    numRecords = EMBEDDB_GET_COUNT(buffer);
    if (validData && numRecords > 0 && numRecords < state->maxRecordsPerPage + 1) {
      hasData = true;
      maxLogicalPageId = logicalPageId;
      physicalPageId++;
      updateMaxiumError(state, buffer);
      count++;
      i = 2;
    } else {
      physicalPageId += blockSize;
      count += blockSize;
    }
    moreToRead = !(readPage(state, physicalPageId));
    i++;
  }
  
  /* if we have no valid data, we just have an empty file can can start from the scratch */
  if (!hasData)
    return 0;
  
  while (moreToRead && count < state->numDataPages) {
    memcpy(&logicalPageId, buffer, sizeof(pgid_t));
    validData = logicalPageId % state->numDataPages == count;
    if (validData && logicalPageId == maxLogicalPageId + 1) {
      maxLogicalPageId = logicalPageId;
      physicalPageId++;
      updateMaxiumError(state, buffer);
      moreToRead = !(readPage(state, physicalPageId));
      count++;
    } else {
      break;
    }
  }
  
  /*
   * Now we need to find where the page with the smallest key that is
   * still valid.  The default case is we have not wrapped and the
   * page number for the physical page with the smallest key is 0.
   */
  pgid_t physicalPageIDOfSmallestData = 0;
  
  /* check if data exists at this location */
  if (moreToRead && count < state->numDataPages) {
    /* find where the next block boundary is */
    pgid_t pagesToBlockBoundary = blockSize - (count % blockSize);
    
    /* go to the next block boundary */
    physicalPageId = (physicalPageId + pagesToBlockBoundary) % state->numDataPages;
    moreToRead = !(readPage(state, physicalPageId));
    
    /* there should have been more to read becuase the file should not
       be empty at this point if it was not empty at the previous
       block */
    if (!moreToRead) {
      return -1;
    }
    
    /* check if data is valid or if it is junk */
    memcpy(&logicalPageId, buffer, sizeof(pgid_t));
    validData = logicalPageId % state->numDataPages == physicalPageId;
    
    /* this means we have wrapped and our start is actually here */
    if (validData) {
      physicalPageIDOfSmallestData = physicalPageId;
    }
  }
  
  state->nextDataPageId = maxLogicalPageId + 1;
  readPage(state, physicalPageIDOfSmallestData);
  memcpy(&(state->minDataPageId), buffer, sizeof(pgid_t));
  state->numAvailDataPages = state->numDataPages + state->minDataPageId - maxLogicalPageId - 1;
  
  /* Put largest key back into the buffer */
  readPage(state, (state->nextDataPageId - 1) % state->numDataPages);
  
  if (EMBEDDB_USING_SPLINE(state->parameters)) {
    embedDBInitSplineFromFile(state);
  }
  
  return 0;
}

static int8_t
embedDBInitDataFromFileWithRecordLevelConsistency(embedDBState *state)
{
  pgid_t   logicalPageId = 0;
  pgid_t   maxLogicalPageId = 0;
  pgid_t   physicalPageId = 0;
  uint32_t count = 0;
  count_t  blockSize = state->eraseSizeInPages;
  bool     validData = false;
  bool     hasPermanentData = false;
  void *   buffer = (int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_READ_BUFFER;
  
  /* This will become zero if there is no more to read */
  int8_t moreToRead = !(readPage(state, physicalPageId));
  
  /* This handles the case that the first three pages may not have
   * valid data in them.  They may be either an erased page or pages
   * for record-level consistency.
   */
  uint32_t i = 0;
  int8_t numRecords = 0;
  while (moreToRead && i < 4) {
    memcpy(&logicalPageId, buffer, sizeof(pgid_t));
    validData = logicalPageId % state->numDataPages == count;
    numRecords = EMBEDDB_GET_COUNT(buffer);
    if (validData && numRecords > 0 && numRecords < state->maxRecordsPerPage + 1) {
      /* Setup for next loop so it does not have to worry about setting the initial values */
      hasPermanentData = true;
      maxLogicalPageId = logicalPageId;
      physicalPageId++;
      updateMaxiumError(state, buffer);
      count++;
      i = 4;
    } else {
      physicalPageId += blockSize;
      count += blockSize;
    }
    moreToRead = !(readPage(state, physicalPageId));
    i++;
  }
  
  if (hasPermanentData) {
    while (moreToRead && count < state->numDataPages) {
      memcpy(&logicalPageId, buffer, sizeof(pgid_t));
      validData = logicalPageId % state->numDataPages == count;
      if (validData && logicalPageId == maxLogicalPageId + 1) {
	maxLogicalPageId = logicalPageId;
	physicalPageId++;
	updateMaxiumError(state, buffer);
	moreToRead = !(readPage(state, physicalPageId));
	count++;
      } else {
	break;
      }
    }
  } else {
    /* Case where the there is no permanent pages written, but we may
       still have record-level consistency records in block 2 */
    count = 0;
    physicalPageId = 0;
  }
  
  /* find where the next block boundary is */
  pgid_t pagesToBlockBoundary = blockSize - (count % blockSize);
  /* if we are on a block-boundary, we erase the next page in case the
     erase failed and then skip to the start of the next block */
  if (pagesToBlockBoundary == blockSize) {
    int8_t eraseSuccess = state->fileInterface->erase(count, count + blockSize,
						      state->pageSize, state->dataFile);
    if (!eraseSuccess) {
      EDB_PERRF("Error: Unable to erase data page during recovery!\n");
      return -1;
    }
  }
  
  /* go to the next block boundary */
  physicalPageId = (physicalPageId + pagesToBlockBoundary) % state->numDataPages;
  state->rlcPhysicalStartingPage = physicalPageId;
  state->nextRLCPhysicalPageLocation = physicalPageId;
  
  /* record-level consistency recovery algorithm */
  uint32_t numPagesRead = 0;
  uint32_t numPagesToRead = blockSize * 2;
  uint32_t rlcMaxLogicialPageNumber = UINT32_MAX;
  uint32_t rlcMaxRecordCount = UINT32_MAX;
  uint32_t rlcMaxPage = UINT32_MAX;
  moreToRead = !(readPage(state, physicalPageId));
  while (moreToRead && numPagesRead < numPagesToRead) {
    memcpy(&logicalPageId, buffer, sizeof(pgid_t));
    /* If the next logical page number is not the one after the max
     * data page, we can just skip to the next page.  We also need to
     * read the page if there are no permanent records but the
     * logicalPageId is zero, as this indicates we have record-level
     * consistency records
     */
    if (logicalPageId == maxLogicalPageId + 1 || (logicalPageId == 0 && !hasPermanentData)) {
      uint32_t numRecords = EMBEDDB_GET_COUNT(buffer);
      if (rlcMaxRecordCount == UINT32_MAX || numRecords > rlcMaxRecordCount) {
	rlcMaxRecordCount = numRecords;
	rlcMaxLogicialPageNumber = logicalPageId;
	rlcMaxPage = numPagesRead;
      }
    }
    physicalPageId = (physicalPageId + 1) % state->numDataPages;
    moreToRead = !(readPage(state, physicalPageId));
    numPagesRead++;
  }
  
  /* need to find larged record-level consistency page to place back
     into the buffer and either one or both of the record-level
     consistency pages */
  uint32_t eraseStartingPage = 0;
  uint32_t eraseEndingPage = 0;
  uint32_t numBlocksToErase = 0;
  if (rlcMaxLogicialPageNumber == UINT32_MAX) {
    eraseStartingPage = state->rlcPhysicalStartingPage % state->numDataPages;
    numBlocksToErase = 2;
  } else {
    state->nextRLCPhysicalPageLocation = (state->rlcPhysicalStartingPage + rlcMaxPage + 1) % state->numDataPages;
    /* need to read the max page into read buffer again so we can copy
       into the write buffer */
    int8_t readSuccess = readPage(state, (state->rlcPhysicalStartingPage + rlcMaxPage) % state->numDataPages);
    if (readSuccess != 0) {
      EDB_PERRF("Error: Can't read page in data file that was previously read!\n");
      return -1;
    }
    memcpy(state->buffer, buffer, state->pageSize);
    eraseStartingPage =
      (state->rlcPhysicalStartingPage + (rlcMaxPage < blockSize ? blockSize : 0)) % state->numDataPages;
    numBlocksToErase = 1;
  }
  
  for (uint32_t i = 0; i < numBlocksToErase; i++) {
    eraseEndingPage = eraseStartingPage + blockSize;
    int8_t eraseSuccess = state->fileInterface->erase(eraseStartingPage, eraseEndingPage,
						      state->pageSize, state->dataFile);
    if (!eraseSuccess) {
      EDB_PERRF("Error: Unable to erase pages in data file!\n");
      return -1;
    }
    eraseStartingPage = eraseEndingPage % state->numDataPages;
  }
  
  /* if we don't have any permanent data, we can just return now that
     the record-level consistency records have been handled */
  if (!hasPermanentData) {
    return 0;
  }
  
  /* Now check if we have wrapped after the record level consistency.
   * The default case is we start at beginning of data file.
   */
  pgid_t physicalPageIDOfSmallestData = 0;
  
  physicalPageId = (state->rlcPhysicalStartingPage + 2 * blockSize) % state->numDataPages;
  int8_t readSuccess = readPage(state, physicalPageId);
  if (readSuccess == 0) {
    memcpy(&logicalPageId, buffer, sizeof(pgid_t));
    validData = logicalPageId % state->numDataPages == physicalPageId;
    
    /* this means we have wrapped and our start is actually here */
    if (validData) {
      physicalPageIDOfSmallestData = physicalPageId;
    }
  }
  
  state->nextDataPageId = maxLogicalPageId + 1;
  readPage(state, physicalPageIDOfSmallestData);
  memcpy(&(state->minDataPageId), buffer, sizeof(pgid_t));
  state->numAvailDataPages =
    state->numDataPages + state->minDataPageId - maxLogicalPageId - 1 - (2 * blockSize);
  
  /* Put largest key back into the buffer */
  readPage(state, (state->nextDataPageId - 1) % state->numDataPages);
  if (EMBEDDB_USING_SPLINE(state->parameters)) {
    embedDBInitSplineFromFile(state);
  }
  
  return 0;
}

static void
embedDBInitSplineFromFile(embedDBState *state)
{
  pgid_t pageNumberToRead = state->minDataPageId;
  void * buffer = (int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_READ_BUFFER;
  pgid_t pagesRead = 0;
  pgid_t numberOfPagesToRead = state->nextDataPageId - state->minDataPageId;
  while (pagesRead < numberOfPagesToRead) {
    readPage(state, pageNumberToRead % state->numDataPages);
    splineAdd(state->spl, embedDBGetMinKey(state, buffer), pageNumberToRead++);
    pagesRead++;
  }
}

static int8_t
embedDBInitIndex(embedDBState *state)
{
  /* Setup index file. */
  
  /* 4 for id, 2 for count, 2 unused, 4 for minKey (pageId), 4 for maxKey (pageId) */
  state->maxIdxRecordsPerPage = (state->pageSize - 16) / state->bitmapSize;
  
  /* Allocate third page of buffer as index output page */
  initBufferPage(state, EMBEDDB_INDEX_WRITE_BUFFER);
  
  /* Add page id to minimum value spot in page */
  void *buf = (int8_t *)state->buffer + state->pageSize * (EMBEDDB_INDEX_WRITE_BUFFER);
  pgid_t *ptr = ((pgid_t *)((int8_t *)buf + 8));
  *ptr = state->nextDataPageId;
  
  state->nextIdxPageId = 0;
  state->numAvailIndexPages = state->numIndexPages;
  state->minIndexPageId = 0;
  
  if (state->numIndexPages < state->eraseSizeInPages * 2) {
    EDB_PERRF("ERROR: Minimum index space is two erase blocks\n");
    return -1;
  }
  
  if (state->numIndexPages % state->eraseSizeInPages != 0) {
    EDB_PERRF("ERROR: Ensure index space is a multiple of erase block size\n");
    return -1;
  }
  
  if (state->indexFile == NULL) {
    EDB_PERRF("ERROR: No index file provided!\n");
    return -1;
  }
  
  if (!EMBEDDB_RESETING_DATA(state->parameters)) {
    int8_t openStatus = state->fileInterface->open(state->indexFile, EMBEDDB_FILE_MODE_R_PLUS_B);
    if (openStatus) {
      return embedDBInitIndexFromFile(state);
    }
  }
  
  int8_t openStatus = state->fileInterface->open(state->indexFile, EMBEDDB_FILE_MODE_W_PLUS_B);
  if (!openStatus) {
    EDB_PERRF("Error: Can't open index file!\n");
    return -1;
  }
  
  return 0;
}

static int8_t
embedDBInitIndexFromFile(embedDBState *state)
{
  pgid_t logicalIndexPageId = 0;
  pgid_t maxLogicaIndexPageId = 0;
  pgid_t physicalIndexPageId = 0;
  
  /* This will become zero if there is no more to read */
  int8_t moreToRead = !(readIndexPage(state, physicalIndexPageId));
  
  bool haveWrappedInMemory = false;
  int count = 0;
  void *buffer = (int8_t *)state->buffer + state->pageSize * EMBEDDB_INDEX_READ_BUFFER;
  
  while (moreToRead && count < state->numIndexPages) {
    memcpy(&logicalIndexPageId, buffer, sizeof(pgid_t));
    if (count == 0 || logicalIndexPageId == maxLogicaIndexPageId + 1) {
      maxLogicaIndexPageId = logicalIndexPageId;
      physicalIndexPageId++;
      moreToRead = !(readIndexPage(state, physicalIndexPageId));
      count++;
    } else {
      haveWrappedInMemory = logicalIndexPageId == maxLogicaIndexPageId - state->numIndexPages + 1;
      break;
    }
  }
  
  if (count == 0)
    return 0;
  
  state->nextIdxPageId = maxLogicaIndexPageId + 1;
  pgid_t physicalPageIDOfSmallestData = 0;
  if (haveWrappedInMemory) {
    physicalPageIDOfSmallestData = logicalIndexPageId % state->numIndexPages;
  }
  readIndexPage(state, physicalPageIDOfSmallestData);
  memcpy(&(state->minIndexPageId), buffer, sizeof(pgid_t));
  state->numAvailIndexPages = state->numIndexPages + state->minIndexPageId - maxLogicaIndexPageId - 1;
  
  return 0;
}

static int8_t
embedDBInitVarData(embedDBState *state)
{
  // Initialize variable data outpt buffer
  initBufferPage(state, EMBEDDB_VAR_WRITE_BUFFER(state->parameters));
  
  state->variableDataHeaderSize = state->keySize + sizeof(pgid_t);
  state->currentVarLoc = state->variableDataHeaderSize;
  state->minVarRecordId = UINT64_MAX;
  state->numAvailVarPages = state->numVarPages;
  state->nextVarPageId = 0;
  
  if (!EMBEDDB_RESETING_DATA(state->parameters) &&
      (state->nextDataPageId > 0 || EMBEDDB_USING_RECORD_LEVEL_CONSISTENCY(state->parameters))) {
    int8_t openResult = state->fileInterface->open(state->varFile, EMBEDDB_FILE_MODE_R_PLUS_B);
    if (openResult) {
      return embedDBInitVarDataFromFile(state);
    }
  }
  
  int8_t openResult = state->fileInterface->open(state->varFile, EMBEDDB_FILE_MODE_W_PLUS_B);
  if (!openResult) {
    EDB_PERRF("Error: Can't open variable data file!\n");
    return -1;
  }
  
  return 0;
}

static int8_t
embedDBInitVarDataFromFile(embedDBState *state)
{
  pgid_t logicalVariablePageId = 0;
  pgid_t maxLogicalVariablePageId = 0;
  pgid_t physicalVariablePageId = 0;
  pgid_t count = 0;
  count_t blockSize = state->eraseSizeInPages;
  bool validData = false;
  bool hasData = false;
  void *buffer = (int8_t *)state->buffer + state->pageSize * EMBEDDB_VAR_READ_BUFFER(state->parameters);
  
  /* This will equal 0 if there are no pages to read */
  int8_t moreToRead = !(readVariablePage(state, physicalVariablePageId));
  
  /* this handles the case where the first page may have been erased,
     so has junk data and we actually need to start from the second
     page */
  uint32_t i = 0;
  while (moreToRead && i < 2) {
    memcpy(&logicalVariablePageId, buffer, sizeof(pgid_t));
    validData = logicalVariablePageId % state->numVarPages == count;
    if (validData) {
      uint64_t largestVarRecordId = 0;
      /* Fetch the largest key value for which we have data on this page */
      memcpy(&largestVarRecordId, (int8_t *)buffer + sizeof(pgid_t), state->keySize);
      /*
       * Since 0 is a valid first page and a valid record key, we may
       * have a case where this data is valid.  So we go to the next
       * page to check if it is valid as well.
       */
      if (logicalVariablePageId != 0 || largestVarRecordId != 0) {
	i = 2;
	hasData = true;
	maxLogicalVariablePageId = logicalVariablePageId;
      }
      physicalVariablePageId++;
      count++;
    } else {
      pgid_t pagesToBlockBoundary = blockSize - (count % blockSize);
      physicalVariablePageId += pagesToBlockBoundary;
      count += pagesToBlockBoundary;
      i++;
    }
    moreToRead = !(readVariablePage(state, physicalVariablePageId));
  }
  
  /* if we have no valid data, we just have an empty file can can start from the scratch */
  if (!hasData)
    return 0;
  
  while (moreToRead && count < state->numVarPages) {
    memcpy(&logicalVariablePageId, buffer, sizeof(pgid_t));
    validData = logicalVariablePageId % state->numVarPages == count;
    if (validData && logicalVariablePageId == maxLogicalVariablePageId + 1) {
      maxLogicalVariablePageId = logicalVariablePageId;
      physicalVariablePageId++;
      moreToRead = !(readVariablePage(state, physicalVariablePageId));
      count++;
    } else {
      break;
    }
  }
  
  /*
   * Now we need to find where the page with the smallest key that is
   * still valid.  The default case is we have not wrapped and the
   * page number for the physical page with the smallest key is 0.
   */
  pgid_t physicalPageIDOfSmallestData = 0;
  
  /* check if data exists at this location */
  if (moreToRead && count < state->numVarPages) {
    /* find where the next block boundary is */
    pgid_t pagesToBlockBoundary = blockSize - (count % blockSize);
    
    /* go to the next block boundary */
    physicalVariablePageId = (physicalVariablePageId + pagesToBlockBoundary) % state->numVarPages;
    moreToRead = !(readVariablePage(state, physicalVariablePageId));
    
    /* there should have been more to read becuase the file should not
       be empty at this point if it was not empty at the previous
       block */
    if (!moreToRead) {
      return -1;
    }
    
    /* check if data is valid or if it is junk */
    memcpy(&logicalVariablePageId, buffer, sizeof(pgid_t));
    validData = logicalVariablePageId % state->numVarPages == physicalVariablePageId;
    
    /* this means we have wrapped and our start is actually here */
    if (validData) {
      physicalPageIDOfSmallestData = physicalVariablePageId;
    }
  }
  
  state->nextVarPageId = maxLogicalVariablePageId + 1;
  pgid_t minVarPageId = 0;
  int8_t readResult = readVariablePage(state, physicalPageIDOfSmallestData);
  if (readResult != 0) {
    EDB_PERRF("Error reading variable page with smallest data. \n");
    return -1;
  }
  
  memcpy(&minVarPageId, buffer, sizeof(pgid_t));
  
  /* If the smallest varPageId is 0, nothing was ever overwritten, so
     we have all the data */
  if (minVarPageId == 0) {
    void *dataBuffer;
    /* Using record level consistency where nothing was written to
       permanent storage yet but */
    if (EMBEDDB_USING_RECORD_LEVEL_CONSISTENCY(state->parameters) &&
	state->nextDataPageId == 0) {
      /* check the buffer for records  */
      dataBuffer = (int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_WRITE_BUFFER;
    } else {
      /* read page with smallest data we still have */
      dataBuffer = (int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_READ_BUFFER;
      readResult = readPage(state, state->minDataPageId % state->numDataPages);
      if (readResult != 0) {
	EDB_PERRF("Error reading page in data file when recovering variable data. \n");
	return -1;
      }
    }
    
    /* Get smallest key from page and put it into the minVarRecordId */
    uint64_t minKey = 0;
    memcpy(&minKey, embedDBGetMinKey(state, dataBuffer), state->keySize);
    state->minVarRecordId = minKey;
  } else {
    /* We lose some records, but know for sure we have all records larger than this*/
    memcpy(&(state->minVarRecordId), (int8_t *)buffer + sizeof(pgid_t), state->keySize);
    state->minVarRecordId++;
  }
  
  state->numAvailVarPages = state->numVarPages + minVarPageId - maxLogicalVariablePageId - 1;
  state->currentVarLoc = state->nextVarPageId % state->numVarPages * state->pageSize +
    state->variableDataHeaderSize;
  
  return 0;
}

/**
 * @brief   Prints the initialization stats of the given embedDB state
 * @param   state   embedDB state structure
 */
void
embedDBPrintInit(embedDBState *state)
{
  EDB_PRINTF("EmbedDB State Initialization Stats:\n");
  EDB_PRINTF("Buffer size: %" PRId8 "  Page size: %" PRId16 "\n",
	     state->bufferSizeInBlocks, state->pageSize);
  EDB_PRINTF("Key size: %" PRId8 " Data size: %" PRId8 " %sRecord size: %" PRId8 "\n",
	     state->keySize, state->dataSize,
	     EMBEDDB_USING_VDATA(state->parameters) ? "Variable data pointer size: 4 " : "",
	     state->recordSize);
  EDB_PRINTF("Use index: %d  Max/min: %d Sum: %d Bmap: %d\n",
	     EMBEDDB_USING_INDEX(state->parameters),
	     EMBEDDB_USING_MAX_MIN(state->parameters),
	     EMBEDDB_USING_SUM(state->parameters),
	     EMBEDDB_USING_BMAP(state->parameters));
  EDB_PRINTF("Header size: %" PRId8 "  Records per page: %" PRId16 "\n",
	     state->headerSize,
	     state->maxRecordsPerPage);
}

/**
 * @brief	Given a state, uses the first and last keys to estimate a slope of keys
 * @param	state	embedDB algorithm state structure
 * @param	buffer	Pointer to in-memory buffer holding node
 * @return	Returns slope estimate float
 */
static float
embedDBCalculateSlope(embedDBState * state,
		      void *         buffer)
{
  // simplistic slope calculation where the first two entries are used, should be improved
  
  uint32_t slopeX1, slopeX2;
  slopeX1 = 0;
  slopeX2 = EMBEDDB_GET_COUNT(buffer) - 1;
  
  if (state->keySize <= 4) {
    uint32_t slopeY1 = 0, slopeY2 = 0;
    
    // check if both points are the same
    if (slopeX1 == slopeX2) {
      return 1;
    }
    
    // convert to keys
    memcpy(&slopeY1, ((int8_t *)buffer + state->headerSize + state->recordSize * slopeX1), state->keySize);
    memcpy(&slopeY2, ((int8_t *)buffer + state->headerSize + state->recordSize * slopeX2), state->keySize);
    
    // return slope of keys
    return (float)(slopeY2 - slopeY1) / (float)(slopeX2 - slopeX1);
  } else {
    uint64_t slopeY1 = 0, slopeY2 = 0;
    
    // check if both points are the same
    if (slopeX1 == slopeX2) {
      return 1;
    }
    
    // convert to keys
    memcpy(&slopeY1, ((int8_t *)buffer + state->headerSize + state->recordSize * slopeX1), state->keySize);
    memcpy(&slopeY2, ((int8_t *)buffer + state->headerSize + state->recordSize * slopeX2), state->keySize);
    
    // return slope of keys
    return (float)(slopeY2 - slopeY1) / (float)(slopeX2 - slopeX1);
  }
}

/**
 * @brief	Returns the maximum error for current page.
 * @param	state	embedDB algorithm state structure
 * @return	Returns max error integer.
 */
static int32_t
getMaxError(embedDBState * state,
	    void *         buffer)
{
  if (state->keySize <= 4) {
    int32_t maxError = 0, currentError;
    uint32_t minKey = 0, currentKey = 0;
    memcpy(&minKey, embedDBGetMinKey(state, buffer), state->keySize);
    
    // get slope of keys within page
    float slope = embedDBCalculateSlope(state, buffer);
    
    for (int i = 0; i < state->maxRecordsPerPage; i++) {
      // loop all keys in page
      memcpy(&currentKey, ((int8_t *)buffer + state->headerSize + state->recordSize * i), state->keySize);
      
      // make currentKey value relative to current page
      currentKey = currentKey - minKey;
      
      // Guard against integer underflow
      if ((currentKey / slope) >= i) {
	currentError = (currentKey / slope) - i;
      } else {
	currentError = i - (currentKey / slope);
      }
      if (currentError > maxError) {
	maxError = currentError;
      }
    }
    
    if (maxError > state->maxRecordsPerPage) {
      return state->maxRecordsPerPage;
    }
    
    return maxError;
  } else {
    int32_t maxError = 0, currentError;
    uint64_t currentKey = 0, minKey = 0;
    memcpy(&minKey, embedDBGetMinKey(state, buffer), state->keySize);
    
    // get slope of keys within page
    float slope = embedDBCalculateSlope(state, state->buffer);  // this is incorrect, should be buffer. TODO: fix
    
    for (int i = 0; i < state->maxRecordsPerPage; i++) {
      // loop all keys in page
      memcpy(&currentKey, ((int8_t *)buffer + state->headerSize + state->recordSize * i), state->keySize);
      
      // make currentKey value relative to current page
      currentKey = currentKey - minKey;
      
      // Guard against integer underflow
      if ((currentKey / slope) >= i) {
	currentError = (currentKey / slope) - i;
      } else {
	currentError = i - (currentKey / slope);
      }
      if (currentError > maxError) {
	maxError = currentError;
      }
    }
    
    if (maxError > state->maxRecordsPerPage) {
      return state->maxRecordsPerPage;
    }
    
    return maxError;
  }
}

/**
 * @brief	Adds an entry for the current page into the search structure
 * @param	state	embedDB algorithm state structure
 */
static void
indexPage(embedDBState * state,
	  uint32_t       pageNumber)
{
  if (EMBEDDB_USING_SPLINE(state->parameters)) {
    splineAdd(state->spl, embedDBGetMinKey(state, state->buffer), pageNumber);
  }
}

/**
 * @brief	Puts a given key, data pair into structure.
 * @param	state	embedDB algorithm state structure
 * @param	key		Key for record
 * @param	data	Data for record
 * @return	Return 0 if success. Non-zero value if error.
 */
int8_t
embedDBPut(embedDBState * state,
	   void *         key,
	   void *         data)
{
  /* Copy record into block */
  
  count_t count = EMBEDDB_GET_COUNT(state->buffer);
  if (state->nextDataPageId > 0 || count > 0) {
    void *previousKey = NULL;
    if (count == 0) {
      readPage(state, (state->nextDataPageId - 1) % state->numDataPages);
      previousKey = ((int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_READ_BUFFER) +
	(state->recordSize * (state->maxRecordsPerPage - 1)) + state->headerSize;
    } else {
      previousKey = (int8_t *)state->buffer + (state->recordSize * (count - 1)) + state->headerSize;
    }
    if (state->compareKey(key, previousKey) != 1) {
      EDB_PERRF("Keys must be strictly ascending order. Insert Failed.\n");
      return 1;
    }
  }
  
  /* Write current page if full */
  bool wrotePage = false;
  if (count >= state->maxRecordsPerPage) {
    // As the first buffer is the data write buffer, no manipulation is required
    pgid_t pageNum = writePage(state, state->buffer);
    
    indexPage(state, pageNum);
    
    /* Save record in index file */
    if (state->indexFile != NULL) {
      void *buf = (int8_t *)state->buffer + state->pageSize * (EMBEDDB_INDEX_WRITE_BUFFER);
      count_t idxcount = EMBEDDB_GET_COUNT(buf);
      if (idxcount >= state->maxIdxRecordsPerPage) {
	/* Save index page */
	writeIndexPage(state, buf);
	
	idxcount = 0;
	initBufferPage(state, EMBEDDB_INDEX_WRITE_BUFFER);
	
	/* Add page id to minimum value spot in page */
	pgid_t *ptr = (pgid_t *)((int8_t *)buf + 8);
	*ptr = pageNum;
      }
      
      EMBEDDB_INC_COUNT(buf);
      
      /* Copy record onto index page */
      void *bm = EMBEDDB_GET_BITMAP(state->buffer);
      memcpy((void *)((int8_t *)buf + EMBEDDB_IDX_HEADER_SIZE + state->bitmapSize * idxcount),
	     bm, state->bitmapSize);
    }
    
    updateMaxiumError(state, state->buffer);
    
    count = 0;
    initBufferPage(state, 0);
    wrotePage = true;
  }
  
  /* Copy record onto page */
  memcpy((int8_t *)state->buffer + (state->recordSize * count) + state->headerSize,
	 key, state->keySize);
  memcpy((int8_t *)state->buffer + (state->recordSize * count) + state->headerSize + state->keySize,
	 data, state->dataSize);
  
  /* Copy variable data offset if using variable data*/
  if (EMBEDDB_USING_VDATA(state->parameters)) {
    uint32_t dataLocation;
    if (state->recordHasVarData) {
      dataLocation = state->currentVarLoc % (state->numVarPages * state->pageSize);
    } else {
      dataLocation = EMBEDDB_NO_VAR_DATA;
    }
    memcpy((int8_t *)state->buffer + (state->recordSize * count) +
	   state->headerSize + state->keySize + state->dataSize,
	   &dataLocation, sizeof(uint32_t));
  }
  
  /* Update count */
  EMBEDDB_INC_COUNT(state->buffer);
  
  if (EMBEDDB_USING_MAX_MIN(state->parameters)) {
    /* Update MIN/MAX */
    void *ptr;
    if (count != 0) {
      /* Since keys are inserted in ascending order, every insert will
       * update max. Min will never change after first record. */
      ptr = EMBEDDB_GET_MAX_KEY(state->buffer, state);
      memcpy(ptr, key, state->keySize);
      
      ptr = EMBEDDB_GET_MIN_DATA(state->buffer, state);
      if (state->compareData(data, ptr) < 0)
	memcpy(ptr, data, state->dataSize);
      ptr = EMBEDDB_GET_MAX_DATA(state->buffer, state);
      if (state->compareData(data, ptr) > 0)
	memcpy(ptr, data, state->dataSize);
    } else {
      /* First record inserted */
      ptr = EMBEDDB_GET_MIN_KEY(state->buffer);
      memcpy(ptr, key, state->keySize);
      ptr = EMBEDDB_GET_MAX_KEY(state->buffer, state);
      memcpy(ptr, key, state->keySize);
      
      ptr = EMBEDDB_GET_MIN_DATA(state->buffer, state);
      memcpy(ptr, data, state->dataSize);
      ptr = EMBEDDB_GET_MAX_DATA(state->buffer, state);
      memcpy(ptr, data, state->dataSize);
    }
  }
  
  if (EMBEDDB_USING_BMAP(state->parameters)) {
    /* Update bitmap */
    char *bm = (char *)EMBEDDB_GET_BITMAP(state->buffer);
    state->updateBitmap(data, bm);
  }
  
  /* If using record level consistency, we need to immediately write
     the updated page to storage */
  if (EMBEDDB_USING_RECORD_LEVEL_CONSISTENCY(state->parameters)) {
    /* Need to move record level consistency pointers if on a block boundary */
    if (wrotePage && state->nextDataPageId % state->eraseSizeInPages == 0) {
      /* move record-level consistency blocks */
      shiftRecordLevelConsistencyBlocks(state);
    }
    return writeTemporaryPage(state, state->buffer);
  }
  
  return 0;
}

static int8_t
shiftRecordLevelConsistencyBlocks(embedDBState *state)
{
  /* erase the record-level consistency blocks */
  
  /* TODO: Likely an optimisation here where we don't always need to
     erase the second block, but that will make this algorithm more
     complicated and the savings could be minimal */
  uint32_t numRecordLevelConsistencyPages = state->eraseSizeInPages * 2;
  uint32_t eraseStartingPage = state->rlcPhysicalStartingPage;
  uint32_t eraseEndingPage = 0;
  
  /* if we have wraped, we need to erase an additional block as the
     block we are shifting into is not empty */
  bool haveWrapped = (state->minDataPageId % state->numDataPages) ==
    ((state->rlcPhysicalStartingPage + numRecordLevelConsistencyPages) % state->numDataPages);
  uint32_t numBlocksToErase = haveWrapped ? 2 : 3;
  
  /* Erase pages to make space for new data */
  for (size_t i = 0; i < numBlocksToErase; i++) {
    eraseEndingPage = eraseStartingPage + state->eraseSizeInPages;
    int8_t eraseSuccess = state->fileInterface->erase(eraseStartingPage, eraseEndingPage,
						      state->pageSize, state->dataFile);
    if (!eraseSuccess) {
      EDB_PERRF("Error: Unable to erase pages in data file when shifting record level consistency blocks!\n");
      return -1;
    }
    eraseStartingPage = eraseEndingPage % state->numDataPages;
  }
  
  /* shift min data page if needed */
  if (haveWrapped) {
    /* Flag the pages as usable to EmbedDB */
    state->numAvailDataPages += state->eraseSizeInPages;
    state->minDataPageId += state->eraseSizeInPages;
    
    /* remove any spline points related to these pages */
    if (!EMBEDDB_DISABLED_SPLINE_CLEAN(state->parameters)) {
      cleanSpline(state, state->minDataPageId);
    }
  }
  
  /* shift record-level consistency blocks */
  state->rlcPhysicalStartingPage =
    (state->rlcPhysicalStartingPage + state->eraseSizeInPages) % state->numDataPages;
  state->nextRLCPhysicalPageLocation = state->rlcPhysicalStartingPage;
  
  return 0;
}

static void
updateMaxiumError(embedDBState * state,
		  void *         buffer)
{
  // Calculate error within the page
  int32_t maxError = getMaxError(state, buffer);
  if (state->maxError < maxError) {
    state->maxError = maxError;
  }
}

/**
 * @brief	Puts the given key, data, and variable length data into the structure.
 * @param	state			embedDB algorithm state structure
 * @param	key				Key for record
 * @param	data			Data for record
 * @param	variableData	Variable length data for record
 * @param	length			Length of the variable length data in bytes
 * @return	Return 0 if success. Non-zero value if error.
 */
int8_t
embedDBPutVar(embedDBState * state,
	      void *         key,
	      void *         data,
	      void *         variableData,
	      uint32_t       length)
{
  if (!EMBEDDB_USING_VDATA(state->parameters)) {
    EDB_PERRF("Error: Can't insert variable data because it is not enabled\n");
    return -1;
  }
  
  // Insert their data
  
  /*
   * Check that there is enough space remaining in this page to start
   * the insert of the variable data here and if the data page will be
   * written in embedDBGet
   */
  void * buf = (int8_t *)state->buffer + state->pageSize * (EMBEDDB_VAR_WRITE_BUFFER(state->parameters));
  if (state->currentVarLoc % state->pageSize > state->pageSize - 4 ||
      (!(EMBEDDB_USING_RECORD_LEVEL_CONSISTENCY(state->parameters)) &&
       EMBEDDB_GET_COUNT(state->buffer) >= state->maxRecordsPerPage)) {
    writeVariablePage(state, buf);
    initBufferPage(state, EMBEDDB_VAR_WRITE_BUFFER(state->parameters));
    // Move data writing location to the beginning of the next page, leaving the room for the header
    state->currentVarLoc +=
      state->pageSize - state->currentVarLoc % state->pageSize + state->variableDataHeaderSize;
  }
  
  if (variableData == NULL) {
    // Var data enabled, but not provided
    state->recordHasVarData = 0;
    return embedDBPut(state, key, data);
  }

  // Perform the regular insert
  state->recordHasVarData = 1;
  int8_t r;
  if ((r = embedDBPut(state, key, data)) != 0) {
    return r;
  }
  
  if (state->minVarRecordId == UINT64_MAX) {
    memcpy(&state->minVarRecordId, key, state->keySize);
  }
  
  // Update the header to include the maximum key value stored on this page
  memcpy((int8_t *)buf + sizeof(pgid_t), key, state->keySize);
  
  // Write the length of the data item into the buffer
  memcpy((uint8_t *)buf + state->currentVarLoc % state->pageSize, &length, sizeof(uint32_t));
  state->currentVarLoc += 4;
  
  // Check if we need to write after doing that
  if (state->currentVarLoc % state->pageSize == 0) {
    writeVariablePage(state, buf);
    initBufferPage(state, EMBEDDB_VAR_WRITE_BUFFER(state->parameters));
    
    // Update the header to include the maximum key value stored on this page
    memcpy((int8_t *)buf + sizeof(pgid_t), key, state->keySize);
    state->currentVarLoc += state->variableDataHeaderSize;
  }
  
  int amtWritten = 0;
  while (length > 0) {
    // Copy data into the buffer. Write the min of the space left in
    // this page and the remaining length of the data
    uint16_t amtToWrite = min(state->pageSize - state->currentVarLoc % state->pageSize, length);
    memcpy((uint8_t *)buf + (state->currentVarLoc % state->pageSize),
	   (uint8_t *)variableData + amtWritten, amtToWrite);
    length -= amtToWrite;
    amtWritten += amtToWrite;
    state->currentVarLoc += amtToWrite;
    
    // If we need to write the buffer to file
    if (state->currentVarLoc % state->pageSize == 0) {
      writeVariablePage(state, buf);
      initBufferPage(state, EMBEDDB_VAR_WRITE_BUFFER(state->parameters));
      
      // Update the header to include the maximum key value stored on
      // this page and account for page number
      memcpy((int8_t *)buf + sizeof(pgid_t), key, state->keySize);
      state->currentVarLoc += state->variableDataHeaderSize;
    }
  }
  
  if (EMBEDDB_USING_RECORD_LEVEL_CONSISTENCY(state->parameters)) {
    embedDBFlushVar(state);
  }
  
  return 0;
}

/**
 * @brief	Given a key, estimates the location of the key within the node.
 * @param	state	embedDB algorithm state structure
 * @param	buffer	Pointer to in-memory buffer holding node
 * @param	key		Key for record
 */
static int16_t
embedDBEstimateKeyLocation(embedDBState * state,
			   void *         buffer,
			   void *          key)
{
  // get slope to use for linear estimation of key location
  // return estimated location of the key
  float slope = embedDBCalculateSlope(state, buffer);
  
  uint64_t minKey = 0, thisKey = 0;
  memcpy(&minKey, embedDBGetMinKey(state, buffer), state->keySize);
  memcpy(&thisKey, key, state->keySize);
  
  return (thisKey - minKey) / slope;
}

/**
 * @brief Given a key, searches the node for the key. If interior
 *        node, returns child record number containing next page id to
 *        follow. If leaf node, returns if of first record with that
 *        key or (<= key). Returns -1 if key is not found.
 * @param	state	embedDB algorithm state structure
 * @param	buffer	Pointer to in-memory buffer holding node
 * @param	key		Key for record 
 * @param range  1 if range query so return pointer to first record <=
 *               key, 0 if exact query so much return first exact
 *               match record
 */
static pgid_t
embedDBSearchNode(embedDBState * state,
		  void *         buffer,
		  void *         key,
		  int8_t         range)
{
  int16_t first, last, middle, count;
  int8_t compare;
  void *mkey;
  
  count = EMBEDDB_GET_COUNT(buffer);
  middle = embedDBEstimateKeyLocation(state, buffer, key);
  
  // check that maxError was calculated and middle is valid (searches full node otherwise)
  if (state->maxError == -1 || middle >= count || middle <= 0) {
    first = 0;
    last = count - 1;
    middle = (first + last) / 2;
  } else {
    first = 0;
    last = count - 1;
  }
  
  if (middle > last) {
    middle = last;
  }
  
  while (first <= last) {
    mkey = (int8_t *)buffer + state->headerSize + (state->recordSize * middle);
    compare = state->compareKey(mkey, key);
    if (compare < 0) {
      first = middle + 1;
    } else if (compare == 0) {
      return middle;
    } else {
      last = middle - 1;
    }
    middle = (first + last) / 2;
  }
  if (range)
    return middle;
  return -1;
}

/**
 * @brief	Linear search function to be used with an approximate range of pages.
 * 			If the desired key is found, the page containing that record is loaded
 * 			into the passed buffer pointer.
 * @param	state		embedDB algorithm state structure
 * @param 	numReads	Tracks total number of reads for statistics
 * @param	buf			buffer to store page with desired record
 * @param	key			Key for the record to search for
 * @param	pageId		Page id to start search from
 * @param 	low			Lower bound for the page the record could be found on
 * @param 	high		Uper bound for the page the record could be found on
 * @return	Return 0 if success. Non-zero value if error.
 */
static int8_t
linearSearch(embedDBState *state,
	     void *buf,
	     void *key,
	     int32_t pageId,
	     int32_t low,
	     int32_t high)
{
  int32_t pageError = 0;
  int32_t physPageId;
  while (1) {
    /* Move logical page number to physical page id based on location of first data page */
    physPageId = pageId % state->numDataPages;
    
    if (pageId > high || pageId < low || low > high ||
	pageId < state->minDataPageId || pageId >= state->nextDataPageId) {
      return -1;
    }
    
    /* Read page into buffer. If 0 not returned, there was an error */
    if (readPage(state, physPageId) != 0) {
      return -1;
    }
    
    if (state->compareKey(key, embedDBGetMinKey(state, buf)) < 0) {
      /* Key is less than smallest record in block. */
      high = --pageId;
      pageError++;
    }
    else if (state->compareKey(key, embedDBGetMaxKey(state, buf)) > 0) {
      /* Key is larger than largest record in block. */
      low = ++pageId;
      pageError++;
    } else {
      /* Found correct block */
      return 0;
    }
  }
}

static int8_t
binarySearch(embedDBState * state,
	     void *         buffer,
	     void *         key)
{
  int8_t retval = -1;
  uint32_t first = state->minDataPageId, last = state->nextDataPageId - 1;
  uint32_t pageId = (first + last) / 2;
  while (1) {
    /* Read page into buffer */
    if (readPage(state, pageId % state->numDataPages) != 0) {
      break;
    }
    
    if (first >= last) {
      break;
    }
    
    if (state->compareKey(key, embedDBGetMinKey(state, buffer)) < 0) {
      /* Key is less than smallest record in block. */
      last = pageId - 1;
      pageId = (first + last) / 2;
    } else if (state->compareKey(key, embedDBGetMaxKey(state, buffer)) > 0) {
      /* Key is larger than largest record in block. */
      first = pageId + 1;
      pageId = (first + last) / 2;
    } else {
      /* Found correct block */
      retval = 0;
      break;
    }
  }

  return retval;
}

static int8_t
splineSearch(embedDBState * state,
	     void *         buffer,
	     void *         key)
{
  /* Spline search */
  uint32_t location, lowbound, highbound;
  splineFind(state->spl, key, state->compareKey, &location, &lowbound, &highbound);
  
  /* If the spline thinks the data is on a page smaller than the
     smallest data page we have, we know we don't have the data */
  if (highbound < state->minDataPageId) {
    return -1;
  }
  
  /* if the spline returns a lowbound lower than than the smallest
     page we have, we can move the lowbound and location up */
  if (lowbound < state->minDataPageId) {
    lowbound = state->minDataPageId;
    location = (lowbound + highbound) / 2;
  }
  
  // Check if the currently buffered page is the correct one
  if (!(lowbound <= state->bufferedPageId &&
	highbound >= state->bufferedPageId &&
	state->compareKey(embedDBGetMinKey(state, buffer), key) <= 0 &&
	state->compareKey(embedDBGetMaxKey(state, buffer), key) >= 0)) {
    if (linearSearch(state, buffer, key, location, lowbound, highbound) == -1) {
      return -1;
    }
  }
  return 0;
}

/**
 * @brief	Given a key, searches for data associated with
 *          that key in embedDB buffer using embedDBSearchNode.
 *          Note: Space for data must be already allocated.
 * @param	state	embedDB algorithm state structure
 * @param   buffer  pointer to embedDB buffer
 * @param	key		Key for record
 * @param	data	Pre-allocated memory to copy data for record
 * @return	Return non-negative integer representing offset if success. Non-zero value if error.
 */
static int8_t
searchBuffer(embedDBState * state,
	     void *         buffer,
	     void *         key,
	     void *         data)
{
  // return -1 if there is nothing in the buffer
  if (EMBEDDB_GET_COUNT(buffer) == 0) {
    return NO_RECORD_FOUND;
  }
  // find index of record inside of the write buffer
  pgid_t nextId = embedDBSearchNode(state, buffer, key, 0);
  // return 0 if found
  if (nextId != NO_RECORD_FOUND) {
    // Key found
    memcpy(data, (void *)((int8_t *)buffer + state->headerSize +
			  state->recordSize * nextId + state->keySize), state->dataSize);
    return nextId;
  }
  // Key not found
  return NO_RECORD_FOUND;
}

/**
 * @brief	Given a key, returns data associated with key.
 * 			Note: Space for data must be already allocated.
 * 			Data is copied from database into data buffer.
 * @param	state	embedDB algorithm state structure
 * @param	key		Key for record
 * @param	data	Pre-allocated memory to copy data for record 
 * @return   Return 0 if success. Returns -2 if requested key is less
 *           than the minimum stored key. Non-zero value if error.
 */
int8_t
embedDBGet(embedDBState * state,
	   void *         key,
	   void *         data)
{
  void *outputBuffer = state->buffer;
  if (state->nextDataPageId == 0) {
    int8_t success = searchBuffer(state, outputBuffer, key, data);
    if (success != NO_RECORD_FOUND) {
      return 0;
    }
    return -1;
  }
  
  uint64_t thisKey = 0;
  memcpy(&thisKey, key, state->keySize);
  
  void *buf = (int8_t *)state->buffer + state->pageSize;
  
  // if write buffer is not empty
  if ((EMBEDDB_GET_COUNT(outputBuffer) != 0)) {
    // get the max/min key from output buffer
    uint64_t bufMaxKey = 0;
    uint64_t bufMinKey = 0;
    memcpy(&bufMaxKey, embedDBGetMaxKey(state, outputBuffer), state->keySize);
    memcpy(&bufMinKey, embedDBGetMinKey(state, outputBuffer), state->keySize);
    
    // return -1 if key is not in buffer
    if (thisKey > bufMaxKey) return -1;
    
    // if key >= buffer's min, check buffer
    if (thisKey >= bufMinKey) {
      return (searchBuffer(state, outputBuffer, key, data) != NO_RECORD_FOUND) ? 0 : NO_RECORD_FOUND;
    }
  }
  
  int8_t searchResult = 0;
  if (EMBEDDB_USING_BINARY_SEARCH(state->parameters)) {
    /* Regular binary search */
    searchResult = binarySearch(state, buf, key);
  } else {
    /* Spline search */
    searchResult = splineSearch(state, buf, key);
  }
  
  if (searchResult != 0) {
    EDB_PERRF("ERROR: embedDBGet was unable to find page to search for record\n");
    return -1;
  }
  
  pgid_t nextId = embedDBSearchNode(state, buf, key, 0);
  
  if (nextId != -1) {
    /* Key found */
    memcpy(data, (void *)((int8_t *)buf + state->headerSize +
			  state->recordSize * nextId + state->keySize), state->dataSize);
    return 0;
  }
  // Key not found
  return -1;
}

/**
 * @brief	Given a key, returns data associated with key.
 * 			Data is copied from database into data buffer.
 * @param	state	embedDB algorithm state structure
 * @param	key		Key for record
 * @param	data	Pre-allocated memory to copy data for record
 * @param varData  Return variable for variable data as a
 *                 embedDBVarDataStream (Unallocated). Returns NULL if
 *                 no variable data. **Be sure to free the stream
 *                 after you are done with it**
 * @return	Return 0 if success. Non-zero value if error.
 * 			-1 : Error reading file or failed memory allocation
 * 			1  : Variable data was deleted to make room for newer data
 */
int8_t
embedDBGetVar(embedDBState *          state,
	      void *                  key,
	      void *                  data,
	      embedDBVarDataStream ** varData)
{
  if (!EMBEDDB_USING_VDATA(state->parameters)) {
    EDB_PERRF("ERROR: embedDBGetVar called when not using variable data\n");
    return 0;
  }
  void *outputBuffer = (int8_t *)state->buffer;
  
  // search output buffer for record, mem copy fixed record into data
  int8_t recordNum = searchBuffer(state, outputBuffer, key, data);
  
  // if there are records found in the output buffer
  if (recordNum != NO_RECORD_FOUND) {
    // flush variable record buffer to storage to read later on
    embedDBFlushVar(state);
    // copy contents of write buffer to read buffer for embedDBSetupVarDataStream()
    readToWriteBuf(state);
    // else if there are records in the file system, mem cpy fixed record into data
  } else if (embedDBGet(state, key, data) == RECORD_FOUND) {
    // get pointer from the read buffer
    void *buf = (int8_t *)state->buffer + (state->pageSize * EMBEDDB_DATA_READ_BUFFER);
    // retrieve offset
    recordNum = embedDBSearchNode(state, buf, key, 0);
  } else {
    return NO_RECORD_FOUND;
  }
  
  int8_t setupResult = embedDBSetupVarDataStream(state, key, varData, recordNum);
  
  switch (setupResult) {
  case 0:
    /* code */
    return 0;
  case 1:
    return 1;
  case 2:
  case 3:
    return -1;
  }
  
  return -1;
}

/**
 * @brief	Initialize iterator on embedDB structure.
 * @param	state	embedDB algorithm state structure
 * @param	it		embedDB iterator state structure
 */
void
embedDBInitIterator(embedDBState *    state,
		    embedDBIterator * it)
{
  /* Build query bitmap (if used) */
  it->queryBitmap = NULL;
  if (EMBEDDB_USING_BMAP(state->parameters)) {
    /* Verify that bitmap index is useful (must have set either min or max data value) */
    if ((it->minData || it->maxData) && EDB_WITH_HEAP) {
      it->queryBitmap = calloc(1, state->bitmapSize);
      state->buildBitmapFromRange(it->minData, it->maxData, it->queryBitmap);
    }
  }
  
  if (!EMBEDDB_USING_BMAP(state->parameters)) {
    EDB_PERRF("WARN: Iterator not using index. If this is not intended, "
	      "ensure that the embedDBState is using a bitmap and was "
	      "initialized with an index file.\n");
  }
  else if (!EMBEDDB_USING_INDEX(state->parameters)) {
    EDB_PERRF("WARN: Iterator not using index to full extent. If this is not intended, "
	      "ensure that the embedDBState was initialized with an index file.\n");
  }
  
  /* Determine which data page should be the first examined if there
     is a min key and that we have spline points */
  if ((state->spl->count != 0) &&
      it->minKey &&
      EMBEDDB_USING_SPLINE(state->parameters)) {
    /* Spline search */
    uint32_t location, lowbound, highbound = 0;
    splineFind(state->spl, it->minKey, state->compareKey, &location, &lowbound, &highbound);
    
    // Use the low bound as the start for our search
    it->nextDataPage = max(lowbound, state->minDataPageId);
  }
  else {
    it->nextDataPage = state->minDataPageId;
  }
  it->nextDataRec = 0;
}

/**
 * @brief	Close iterator after use.
 * @param	it		embedDB iterator structure
 */
void
embedDBCloseIterator(embedDBIterator * it)
{
  if (it && it->queryBitmap && EDB_WITH_HEAP) {
    free(it->queryBitmap);
  }
}

/**
 * @brief	Flushes output buffer.
 * @param	state	algorithm state structure
 * @returns 0 if successul and a non-zero value otherwise
 */
int8_t
embedDBFlushVar(embedDBState *state)
{
  /* Check if we actually have any variable data in the buffer */
  if (state->currentVarLoc % state->pageSize == state->variableDataHeaderSize) {
    return 0;
  }
  
  // only flush variable buffer
  pgid_t writeResult = writeVariablePage(state, (int8_t *)state->buffer +
					 EMBEDDB_VAR_WRITE_BUFFER(state->parameters) * state->pageSize);
  if (writeResult == -1) {
    EDB_PERRF("Failed to write variable data page during embedDBFlushVar.");
    return -1;
  }
  
  state->fileInterface->flush(state->varFile);
  // init new buffer
  initBufferPage(state, EMBEDDB_VAR_WRITE_BUFFER(state->parameters));
  // determine how many bytes are left
  int temp = state->pageSize - (state->currentVarLoc % state->pageSize);
  // create new offset
  state->currentVarLoc += temp + state->variableDataHeaderSize;
  return 0;
}

/**
 * @brief	Flushes output buffer.
 * @param	state	algorithm state structure
 * @returns 0 if successul and a non-zero value otherwise
 */
int8_t
embedDBFlush(embedDBState *state)
{
  // As the first buffer is the data write buffer, no address change is required
  int8_t *buffer = (int8_t *)state->buffer + EMBEDDB_DATA_WRITE_BUFFER * state->pageSize;
  if (EMBEDDB_GET_COUNT(buffer) < 1)
    return 0;
  
  pgid_t pageNum = writePage(state, buffer);
  if (pageNum == -1) {
    EDB_PERRF("Failed to write page during embedDBFlush.");
    return -1;
  }
  
  state->fileInterface->flush(state->dataFile);
  
  indexPage(state, pageNum);
  
  if (EMBEDDB_USING_INDEX(state->parameters)) {
    void *buf = (int8_t *)state->buffer + state->pageSize * (EMBEDDB_INDEX_WRITE_BUFFER);
    count_t idxcount = EMBEDDB_GET_COUNT(buf);
    EMBEDDB_INC_COUNT(buf);
    
    /* Copy record onto index page */
    void *bm = EMBEDDB_GET_BITMAP(state->buffer);
    memcpy((void *)((int8_t *)buf + EMBEDDB_IDX_HEADER_SIZE +
		    state->bitmapSize * idxcount), bm, state->bitmapSize);
    
    pgid_t writeResult = writeIndexPage(state, buf);
    if (writeResult == -1) {
      EDB_PERRF("Failed to write index page during embedDBFlush.");
      return -1;
    }
    
    state->fileInterface->flush(state->indexFile);
    
    /* Reinitialize buffer */
    initBufferPage(state, EMBEDDB_INDEX_WRITE_BUFFER);
  }
  
  /* Reinitialize buffer */
  initBufferPage(state, EMBEDDB_DATA_WRITE_BUFFER);
  
  // Flush var data page
  if (EMBEDDB_USING_VDATA(state->parameters)) {
    int8_t varFlushResult = embedDBFlushVar(state);
    if (varFlushResult != 0) {
      EDB_PERRF("Failed to flush variable data page");
      return -1;
    }
  }
  return 0;
}

/**
 * @brief	Return next key, data pair for iterator.
 * @param	state	embedDB algorithm state structure
 * @param	it		embedDB iterator state structure
 * @param	key		Return variable for key (Pre-allocated)
 * @param	data	Return variable for data (Pre-allocated)
 * @return	1 if successful, 0 if no more records
 */
int8_t
embedDBNext(embedDBState *    state,
	    embedDBIterator * it,
	    void *            key,
	    void *            data)
{
  int searchWriteBuf = 0;
  while (1) {
    if (it->nextDataPage > state->nextDataPageId) {
      return 0;
    }
    if (it->nextDataPage == state->nextDataPageId) {
      searchWriteBuf = 1;
    }
    
    // If we are just starting to read a new page and we have a query
    // bitmap
    if (it->nextDataRec == 0 && it->queryBitmap != NULL) {
      // Find what index page determines if we should read the data
      // page
      uint32_t indexPage = it->nextDataPage / state->maxIdxRecordsPerPage;
      uint16_t indexRec = it->nextDataPage % state->maxIdxRecordsPerPage;
      
      if (state->indexFile != NULL &&
	  indexPage >= state->minIndexPageId &&
	  indexPage < state->nextIdxPageId) {
	// If the index page that contains this data page exists, else
	// we must read the data page regardless cause we don't have
	// the index saved for it
	
	if (readIndexPage(state, indexPage % state->numIndexPages) != 0) {
	  EDB_PERRF("ERROR: Failed to read index page %" PRIu32 " (%" PRIu32 ")\n",
		    indexPage,
		    indexPage % state->numIndexPages);
	  return 0;
	}
	
	// Get bitmap for data page in question
	void *indexBM = (int8_t *)state->buffer + EMBEDDB_INDEX_READ_BUFFER * state->pageSize +
	  EMBEDDB_IDX_HEADER_SIZE + indexRec * state->bitmapSize;
	
	// Determine if we should read the data page
	if (!bitmapOverlap(it->queryBitmap, indexBM, state->bitmapSize)) {
	  // Do not read this data page, try the next one
	  it->nextDataPage++;
	  continue;
	}
      }
    }
    
    if (searchWriteBuf == 0 && readPage(state, it->nextDataPage % state->numDataPages) != 0) {
      EDB_PERRF("ERROR: Failed to read data page %" PRIu32 " (%" PRIu32 ")\n",
		it->nextDataPage,
		it->nextDataPage % state->numDataPages);
      return 0;
    }
    
    // Keep reading record until we find one that matches the query
    int8_t *buf = searchWriteBuf == 0 ?
      (int8_t *)state->buffer + EMBEDDB_DATA_READ_BUFFER * state->pageSize :
      (int8_t *)state->buffer + EMBEDDB_DATA_WRITE_BUFFER * state->pageSize;
    uint32_t pageRecordCount = EMBEDDB_GET_COUNT(buf);
    while (it->nextDataRec < pageRecordCount) {
      // Get record
      memcpy(key, buf + state->headerSize + it->nextDataRec * state->recordSize, state->keySize);
      memcpy(data, buf + state->headerSize + it->nextDataRec * state->recordSize + state->keySize,
	     state->dataSize);
      it->nextDataRec++;
      
      // Check record
      if (it->minKey != NULL && state->compareKey(key, it->minKey) < 0)
	continue;
      if (it->maxKey != NULL && state->compareKey(key, it->maxKey) > 0)
	return 0;
      if (it->minData != NULL && state->compareData(data, it->minData) < 0)
	continue;
      if (it->maxData != NULL && state->compareData(data, it->maxData) > 0)
	continue;
      
      // If we make it here, the record matches the query
      return 1;
    }
    
    // Finished reading through whole data page and didn't find a match
    it->nextDataPage++;
    it->nextDataRec = 0;
    
    // Try next data page by looping back to top
  }
}

/**
 * @brief	Return next key, data, variable data set for iterator
 * @param	state	embedDB algorithm state structure
 * @param	it		embedDB iterator state structure
 * @param	key		Return variable for key (Pre-allocated)
 * @param	data	Return variable for data (Pre-allocated)
 * @param varData  Return variable for variable data as a
 *                 embedDBVarDataStream (Unallocated). Returns NULL if
 *                 no variable data. **Be sure to free the stream
 *                 after you are done with it**
 * @return	1 if successful, 0 if no more records
 */
int8_t
embedDBNextVar(embedDBState *          state,
	       embedDBIterator *       it,
	       void *                  key,
	       void *                  data,
	       embedDBVarDataStream ** varData)
{
  if (!EMBEDDB_USING_VDATA(state->parameters)) {
    EDB_PERRF("ERROR: embedDBNextVar called when not using variable data\n");
    return 0;
  }
  
  // ensure record exists
  int8_t r = embedDBNext(state, it, key, data);
  if (!r) {
    return 0;
  }
  
  void *outputBuffer = (int8_t *)state->buffer;
  if (it->nextDataPage == 0 && (EMBEDDB_GET_COUNT(outputBuffer) > 0)) {
    readToWriteBuf(state);
    embedDBFlushVar(state);
  }
  
  // Get the vardata address from the record
  count_t recordNum = it->nextDataRec - 1;
  int8_t setupResult = embedDBSetupVarDataStream(state, key, varData, recordNum);
  switch (setupResult) {
  case 0:
  case 1:
    return 1;
  case 2:
  case 3:
    return 0;
  }
  
  return 0;
}

/**
 * @brief Setup varDataStream object to return the variable data for a record
 * @param	state	embedDB algorithm state structure
 * @param   key     Key for the record
 * @param varData  Return variable for variable data as a
 *                 embedDBVarDataStream (Unallocated). Returns NULL if
 *                 no variable data. **Be sure to free the stream
 *                 after you are done with it**
 * @return Returns  0 if sucessfull or no variable data for the record,
 *                  1 if the records variable data was overwritten, 2
 *                  if the page failed to read, and 3 if the memorey
 *                  failed to allocate.
 
 */
static int8_t
embedDBSetupVarDataStream(embedDBState *          state,
			  void *                  key,
			  embedDBVarDataStream ** varData,
			  pgid_t                  recordNumber)
{
  void *dataBuf = (int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_READ_BUFFER;
  void *record = (int8_t *)dataBuf + state->headerSize + recordNumber * state->recordSize;
  
  uint32_t varDataAddr = 0;
  memcpy(&varDataAddr, (int8_t *)record + state->keySize + state->dataSize, sizeof(uint32_t));
  if (varDataAddr == EMBEDDB_NO_VAR_DATA) {
    *varData = NULL;
    return 0;
  }
  
  // Check if the variable data associated with this key has been overwritten due to file wrap around
  if (state->compareKey(key, &state->minVarRecordId) < 0) {
    *varData = NULL;
    return 1;
  }
  
  uint32_t pageNum = (varDataAddr / state->pageSize) % state->numVarPages;
  
  // Read in page
  if (readVariablePage(state, pageNum) != 0) {
    EDB_PERRF("ERROR: embedDB failed to read variable page\n");
    return 2;
  }
  
  // Get length of variable data
  void *varBuf = (int8_t *)state->buffer + state->pageSize * EMBEDDB_VAR_READ_BUFFER(state->parameters);
  uint32_t pageOffset = varDataAddr % state->pageSize;
  uint32_t dataLen = 0;
  memcpy(&dataLen, (int8_t *)varBuf + pageOffset, sizeof(uint32_t));
  
  // Move var data address to the beginning of the data, past the data length
  varDataAddr = (varDataAddr + sizeof(uint32_t)) % (state->numVarPages * state->pageSize);
  
  // If we end up on the page boundary, we need to move past the header
  if (varDataAddr % state->pageSize == 0) {
    varDataAddr += state->variableDataHeaderSize;
    varDataAddr %= (state->numVarPages * state->pageSize);
  }
  
  // Create varDataStream
  embedDBVarDataStream *varDataStream = NULL;
  if (EDB_WITH_HEAP) {
    varDataStream = malloc(sizeof(embedDBVarDataStream));
  }
  if (!varDataStream) {
    EDB_PERRF("ERROR: Failed to alloc memory for embedDBVarDataStream\n");
    return 3;
  }
  
  varDataStream->dataStart = varDataAddr;
  varDataStream->totalBytes = dataLen;
  varDataStream->bytesRead = 0;
  varDataStream->fileOffset = varDataAddr;
  
  *varData = varDataStream;
  return 0;
}

/**
 * @brief	Reads data from variable data stream into the given buffer.
 * @param	state	embedDB algorithm state structure
 * @param	stream	Variable data stream
 * @param	buffer	Buffer to read data into
 * @param	length	Number of bytes to read (Must be <= buffer size)
 * @return	Number of bytes read
 */
uint32_t
embedDBVarDataStreamRead(embedDBState *         state,
			 embedDBVarDataStream * stream,
			 void *                 buffer,
			 uint32_t               length)
{
  if (buffer == NULL) {
    EDB_PERRF("ERROR: Cannot pass null buffer to embedDBVarDataStreamRead\n");
    return 0;
  }
  
  // Read in var page containing the data to read
  uint32_t pageNum = (stream->fileOffset / state->pageSize) % state->numVarPages;
  if (readVariablePage(state, pageNum) != 0) {
    EDB_PERRF("ERROR: Couldn't read variable data page %" PRIu32 "\n", pageNum);
    return 0;
  }
  
  // Keep reading in data until the buffer is full
  void *varDataBuf = (int8_t *)state->buffer +
    state->pageSize * EMBEDDB_VAR_READ_BUFFER(state->parameters);
  uint32_t amtRead = 0;
  while (amtRead < length && stream->bytesRead < stream->totalBytes) {
    uint16_t pageOffset = stream->fileOffset % state->pageSize;
    uint32_t amtToRead = min(stream->totalBytes - stream->bytesRead,
			     min(state->pageSize - pageOffset, length - amtRead));
    memcpy((int8_t *)buffer + amtRead, (int8_t *)varDataBuf + pageOffset, amtToRead);
    amtRead += amtToRead;
    stream->bytesRead += amtToRead;
    stream->fileOffset += amtToRead;
    
    // If we need to keep reading, read the next page
    if (amtRead < length && stream->bytesRead < stream->totalBytes) {
      pageNum = (pageNum + 1) % state->numVarPages;
      if (readVariablePage(state, pageNum) != 0) {
	EDB_PERRF("ERROR: Couldn't read variable data page %" PRIu32 "\n", pageNum);
	return 0;
      }
      // Skip past the header
      stream->fileOffset += state->variableDataHeaderSize;
    }
  }
  
  return amtRead;
}

/**
 * @brief	Prints statistics.
 * @param	state	embedDB state structure
 */
void
embedDBPrintStats(embedDBState *state)
{
  EDB_PRINTF("Num reads: %" PRIu32 "\n", state->numReads);
  EDB_PRINTF("Buffer hits: %" PRIu32 "\n", state->bufferHits);
  EDB_PRINTF("Num writes: %" PRIu32 "\n", state->numWrites);
  EDB_PRINTF("Num index reads: %" PRIu32 "\n", state->numIdxReads);
  EDB_PRINTF("Num index writes: %" PRIu32 "\n", state->numIdxWrites);
  EDB_PRINTF("Max Error: %" PRId32 "\n", state->maxError);
  
  if (EMBEDDB_USING_SPLINE(state->parameters)) {
    splinePrint(state->spl);
  }
}

/**
 * @brief	Writes page in buffer to storage. Returns page number.
 * @param	state	embedDB algorithm state structure
 * @param	buffer	Buffer for writing out page
 * @return	Return page number if success, -1 if error.
 */
pgid_t
writePage(embedDBState * state,
	  void *         buffer)
{
  if (state->dataFile == NULL)
    return -1;
  
  /* Always writes to next page number. Returned to user. */
  pgid_t pageNum = state->nextDataPageId++;
  pgid_t physicalPageNum = pageNum % state->numDataPages;
  
  /* Setup page number in header */
  memcpy(buffer, &(pageNum), sizeof(pgid_t));
  
  if (state->numAvailDataPages <= 0) {
    /* Erase pages to make space for new data */
    int8_t eraseResult =
      state->fileInterface->erase(physicalPageNum,
				  physicalPageNum + state->eraseSizeInPages,
				  state->pageSize, state->dataFile);
    if (eraseResult != 1) {
      EDB_PERRF("Failed to erase data page: %" PRIu32 " (%" PRIu32 ")\n",
		pageNum, physicalPageNum);
      return -1;
    }
    
    /* Flag the pages as usable to EmbedDB */
    state->numAvailDataPages += state->eraseSizeInPages;
    state->minDataPageId += state->eraseSizeInPages;
    
    /* remove any spline points related to these pages */
    if (!EMBEDDB_DISABLED_SPLINE_CLEAN(state->parameters)) {
      cleanSpline(state, state->minDataPageId);
    }
  }
  
  /* Seek to page location in file */
  int32_t val = state->fileInterface->write(buffer, physicalPageNum, state->pageSize, state->dataFile);
  if (val == 0) {
    EDB_PERRF("Failed to write data page: %" PRIu32 " (%" PRIu32 ")\n",
	      pageNum, physicalPageNum);
    return -1;
  }
  
  state->numAvailDataPages--;
  state->numWrites++;
  
  return pageNum;
}

int8_t
writeTemporaryPage(embedDBState * state,
		   void *         buffer)
{
  if (state->dataFile == NULL) {
    EDB_PERRF("The dataFile in embedDBState was null.");
    return -3;
  }
  
  /* Setup page number in header */
  /* TODO: Maybe talk to Ramon about optimizing this */
  memcpy(buffer, &(state->nextDataPageId), sizeof(pgid_t));
  
  /* Wrap if needed */
  state->nextRLCPhysicalPageLocation %= state->numDataPages;
  
  /* If the nextPhysicalPage wrapped, we need to add the numDataPages
     to it to properly compare the page numbers below */
  uint32_t nextPage = state->nextRLCPhysicalPageLocation +
    (state->nextRLCPhysicalPageLocation < state->rlcPhysicalStartingPage ? state->numDataPages : 0);
  
  /* if the nextRLC physical page number would be outside the block,
     we wrap to the start of our record-level consistency blocks */
  if (nextPage - state->rlcPhysicalStartingPage >= state->eraseSizeInPages * 2) {
    state->nextRLCPhysicalPageLocation = state->rlcPhysicalStartingPage;
  }
  
  /* If in pageNum is second page in block, we erase the other
     record-level consistency block */
  if (state->nextRLCPhysicalPageLocation % state->eraseSizeInPages == 1) {
    uint32_t eraseStartingPage = state->rlcPhysicalStartingPage;
    count_t blockSize = state->eraseSizeInPages;
    if (state->nextRLCPhysicalPageLocation == eraseStartingPage + 1) {
      eraseStartingPage = (eraseStartingPage + blockSize) % state->numDataPages;
    }
    uint32_t eraseEndingPage = eraseStartingPage + blockSize;
    
    int8_t eraseSuccess = state->fileInterface->erase(eraseStartingPage, eraseEndingPage,
						      state->pageSize, state->dataFile);
    if (!eraseSuccess) {
      EDB_PERRF("Failed to erase block starting at physical page %" PRIu32 " in the data file.",
		state->nextRLCPhysicalPageLocation);
      return -2;
    }
  }

  /* Write temporary page to storage */
  int8_t writeSuccess = state->fileInterface->write(buffer, state->nextRLCPhysicalPageLocation++,
						    state->pageSize, state->dataFile);
  if (!writeSuccess) {
    EDB_PERRF("Failed to write temporary page for record-level-consistency:"
	      " Logical Page Number %" PRIu32 " - Physical Page (%" PRIu32 ")\n",
	      state->nextDataPageId,
	      state->nextRLCPhysicalPageLocation - 1);
    return -1;
  }
  
  return 0;
}

/**
 * @brief	Calculates the number of spline points not in use by embedDB and deletes them
 * @param	state	embedDB algorithm state structure
 * @param	key 	The minimim key embedDB still needs points for
 * @return	Returns the number of points deleted
 */
static uint32_t
cleanSpline(embedDBState * state,
	    uint32_t       minPageNumber)
{
  uint32_t numPointsErased = 0;
  void *nextPoint;
  uint32_t currentPageNumber = 0;
  for (size_t i = 0; i < state->spl->count; i++) {
    nextPoint = splinePointLocation(state->spl, i + 1);
    memcpy(&currentPageNumber, (int8_t *)nextPoint + state->keySize, sizeof(uint32_t));
    if (currentPageNumber < minPageNumber) {
      numPointsErased++;
    } else {
      break;
    }
  }
  if (state->spl->count - numPointsErased < 2)
    numPointsErased -= 2 - (state->spl->count - numPointsErased);
  if (numPointsErased <= 0)
    return 0;
  splineErase(state->spl, numPointsErased);
  return numPointsErased;
}

/**
 * @brief	Writes index page in buffer to storage. Returns page number.
 * @param	state	embedDB algorithm state structure
 * @param	buffer	Buffer to use for writing index page
 * @return	Return page number if success, -1 if error.
 */
pgid_t
writeIndexPage(embedDBState * state,
	       void *         buffer)
{
  if (state->indexFile == NULL)
    return -1;
  
  /* Always writes to next page number. Returned to user. */
  pgid_t pageNum = state->nextIdxPageId++;
  pgid_t physicalPageNumber = pageNum % state->numIndexPages;
  
  /* Setup page number in header */
  memcpy(buffer, &(pageNum), sizeof(pgid_t));
  
  if (state->numAvailIndexPages <= 0) {
    // Erase index pages to make room for new page
    int8_t eraseResult = state->fileInterface->erase(physicalPageNumber,
						     physicalPageNumber + state->eraseSizeInPages,
						     state->pageSize, state->indexFile);
    if (eraseResult != 1) {
      EDB_PERRF("Failed to erase data page: %" PRIu32 " (%" PRIu32 ")\n",
		pageNum, physicalPageNumber);
      return -1;
    }
    state->numAvailIndexPages += state->eraseSizeInPages;
    state->minIndexPageId += state->eraseSizeInPages;
  }
  
  /* Seek to page location in file */
  int32_t val = state->fileInterface->write(buffer, physicalPageNumber,
					    state->pageSize, state->indexFile);
  if (val == 0) {
    EDB_PERRF("Failed to write index page: %" PRIu32 " (%" PRIu32 ")\n",
	      pageNum, physicalPageNumber);
    return -1;
  }
  
  state->numAvailIndexPages--;
  state->numIdxWrites++;
  
  return pageNum;
}

/**
 * @brief	Writes variable data page in buffer to storage. Returns page number.
 * @param	state	embedDB algorithm state structure
 * @param	buffer	Buffer to use to write page to storage
 * @return	Return page number if success, -1 if error.
 */
pgid_t
writeVariablePage(embedDBState * state,
		  void *         buffer)
{
  if (state->varFile == NULL) {
    return -1;
  }
  
  // Make sure the address being witten to wraps around
  pgid_t physicalPageId = state->nextVarPageId % state->numVarPages;
  
  // Erase data if needed
  if (state->numAvailVarPages <= 0) {
    int8_t eraseResult =
      state->fileInterface->erase(physicalPageId, physicalPageId + state->eraseSizeInPages,
				  state->pageSize, state->varFile);
    if (eraseResult != 1) {
      EDB_PERRF("Failed to erase data page: %" PRIu32 " (%" PRIu32 ")\n",
		state->nextVarPageId, physicalPageId);
      return -1;
    }
    state->numAvailVarPages += state->eraseSizeInPages;
    // Last page that is deleted
    pgid_t pageNum = (physicalPageId + state->eraseSizeInPages - 1) % state->numVarPages;
    
    // Read in that page so we can update which records we still have the data for
    if (readVariablePage(state, pageNum) != 0) {
      return -1;
    }
    void *buf = (int8_t *)state->buffer +
      state->pageSize * EMBEDDB_VAR_READ_BUFFER(state->parameters) + sizeof(pgid_t);
    memcpy(&state->minVarRecordId, buf, state->keySize);
    state->minVarRecordId += 1;  // Add one because the result from the last line is a record that is erased
  }
  
  // Add logical page number to data page
  void *buf = (int8_t *)state->buffer + state->pageSize * EMBEDDB_VAR_WRITE_BUFFER(state->parameters);
  memcpy(buf, &state->nextVarPageId, sizeof(pgid_t));
  
  // Write to file
  uint32_t val = state->fileInterface->write(buffer, physicalPageId, state->pageSize, state->varFile);
  if (val == 0) {
    EDB_PERRF("Failed to write vardata page: %" PRIu32 "\n", state->nextVarPageId);
    return -1;
  }
  
  state->nextVarPageId++;
  state->numAvailVarPages--;
  state->numWrites++;
  
  return state->nextVarPageId - 1;
}

/**
 * @brief	Memcopies write buffer to the read buffer.
 * @param	state	embedDB algorithm state structure
 */
void
readToWriteBuf(embedDBState * state)
{
  // point to read buffer
  void *readBuf = (int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_READ_BUFFER;
  // point to write buffer
  void *writeBuf = (int8_t *)state->buffer + state->pageSize * EMBEDDB_DATA_WRITE_BUFFER;
  // copy write buffer to the read buffer.
  memcpy(readBuf, writeBuf, state->pageSize);
}

/**
 * @brief	Memcopies variable write buffer to the read buffer.
 * @param	state	embedDB algorithm state structure
 */
void
readToWriteBufVar(embedDBState *state)
{
  // point to read buffer
  void *readBuf = (int8_t *)state->buffer + state->pageSize * EMBEDDB_VAR_READ_BUFFER(state->parameters);
  // point to write buffer
  void *writeBuf = (int8_t *)state->buffer + state->pageSize * EMBEDDB_VAR_WRITE_BUFFER(state->parameters);
  // copy write buffer to the read buffer.
  memcpy(readBuf, writeBuf, state->pageSize);
}

/**
 * @brief	Reads given page from storage.
 * @param	state	embedDB algorithm state structure
 * @param	pageNum	Page number to read
 * @return	Return 0 if success, -1 if error.
 */
int8_t
readPage(embedDBState * state,
	 pgid_t         pageNum)
{
  /* Check if page is currently in buffer */
  if (pageNum == state->bufferedPageId) {
    state->bufferHits++;
    return 0;
  }
  
  void *buf = (int8_t *)state->buffer + state->pageSize;
  
  /* Page is not in buffer. Read from storage. */
  /* Read page into start of buffer 1 */
  if (0 == state->fileInterface->read(buf, pageNum, state->pageSize, state->dataFile))
    return -1;
  
  state->numReads++;
  state->bufferedPageId = pageNum;
  return 0;
}

/**
 * @brief	Reads given index page from storage.
 * @param	state	embedDB algorithm state structure
 * @param	pageNum	Page number to read
 * @return	Return 0 if success, -1 if error.
 */
int8_t
readIndexPage(embedDBState * state,
	      pgid_t         pageNum)
{
  /* Check if page is currently in buffer */
  if (pageNum == state->bufferedIndexPageId) {
    state->bufferHits++;
    return 0;
  }
  
  void *buf = (int8_t *)state->buffer + state->pageSize * EMBEDDB_INDEX_READ_BUFFER;
  
  /* Page is not in buffer. Read from storage. */
  /* Read page into start of buffer */
  if (0 == state->fileInterface->read(buf, pageNum, state->pageSize, state->indexFile))
    return -1;
  
  state->numIdxReads++;
  state->bufferedIndexPageId = pageNum;
  return 0;
}

/**
 * @brief	Reads given variable data page from storage
 * @param 	state 	embedDB algorithm state structure
 * @param 	pageNum Page number to read
 * @return 	Return 0 if success, -1 if error
 */
int8_t
readVariablePage(embedDBState * state,
		 pgid_t         pageNum)
{
  // Check if page is currently in buffer
  if (pageNum == state->bufferedVarPage) {
    state->bufferHits++;
    return 0;
  }
  
  // Get buffer to read into
  void *buf = (int8_t *)state->buffer + EMBEDDB_VAR_READ_BUFFER(state->parameters) * state->pageSize;
  
  // Read in one page worth of data
  if (state->fileInterface->read(buf, pageNum, state->pageSize, state->varFile) == 0) {
    return -1;
  }
  
  // Track stats
  state->numReads++;
  state->bufferedVarPage = pageNum;
  return 0;
}

/**
 * @brief	Resets statistics.
 * @param	state	embedDB state structure
 */
void
embedDBResetStats(embedDBState *state)
{
  state->numReads     = 0;
  state->numWrites    = 0;
  state->bufferHits   = 0;
  state->numIdxReads  = 0;
  state->numIdxWrites = 0;
}

/**
 * @brief	Closes structure and frees any dynamic space.
 * @param	state	embedDB state structure
 */
void
embedDBClose(embedDBState * state)
{
  if (state->dataFile != NULL) {
    state->fileInterface->close(state->dataFile);
  }
  if (state->indexFile != NULL) {
    state->fileInterface->close(state->indexFile);
  }
  if (state->varFile != NULL) {
    state->fileInterface->close(state->varFile);
  }
  if (EMBEDDB_USING_SPLINE(state->parameters)) {
    splineClose(state->spl);
    if (EDB_WITH_HEAP) {
      free(state->spl);
    }
    state->spl = NULL;
  }
}

/************************************************************schema.c************************************************************/
/******************************************************************************/
/**
 * @file        schema.c
 * @author      EmbedDB Team (See Authors.md)
 * @brief       Source code file for the schema for EmbedDB query interface
 * @copyright   Copyright 2024
 *              EmbedDB Team
 * @par Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 * @par 1.Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * @par 2.Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * @par 3.Neither the name of the copyright holder nor the names of its contributors
 *  may be used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 * @par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/    

#if defined(ARDUINO) 
#endif

/**
 * @brief	Create an embedDBSchema from a list of column sizes including both key and data
 * @param	numCols			The total number of columns in table
 * @param	colSizes		An array with the size of each column. Max size is 127
 * @param	colSignedness	An array describing if the data in the column is signed or unsigned. Use the defined constants embedDB_COLUMNN_SIGNED or embedDB_COLUMN_UNSIGNED
 */
embedDBSchema *
embedDBCreateSchema(uint8_t  numCols,
		    int8_t * colSizes,
		    int8_t * colSignedness)
{
  embedDBSchema* schema = NULL;
  if (EDB_WITH_HEAP) {
    schema              = malloc(sizeof(embedDBSchema));
    schema->columnSizes = malloc(numCols * sizeof(int8_t));
    schema->numCols     = numCols;
    
    for (unsigned i=0; (i<numCols) && schema; i++) {
      int8_t sign     = colSignedness[i];
      uint8_t colSize = colSizes[i];
      if (colSize <= 0) {
	EDB_PERRF("ERROR: Column size must be greater than zero.\n");
	free(schema->columnSizes);
	free(schema);
	schema = NULL;
      }
      else if (sign == embedDB_COLUMN_SIGNED) {
	schema->columnSizes[i] = -colSizes[i];
      }
      else if (sign == embedDB_COLUMN_UNSIGNED) {
	schema->columnSizes[i] = colSizes[i];
      }
      else {
	EDB_PERRF("ERROR: Must only use embedDB_COLUMN_SIGNED or "
		  "embedDB_COLUMN_UNSIGNED to describe column signedness.\n");
	free(schema->columnSizes);
	free(schema);
	schema = NULL;
      }
    }
  }
  
  return schema;
}

/**
 * @brief	Free a schema. Sets the schema pointer to NULL.
 */
void
embedDBFreeSchema(embedDBSchema** schema)
{
  if (*schema == NULL) return;
  free((*schema)->columnSizes);
  free(*schema);
  *schema = NULL;
}

/**
 * @brief	Uses schema to determine the length of buffer to allocate and callocs that space
 */
void *
createBufferFromSchema(embedDBSchema* schema)
{
  void *retval = NULL;

  if (EDB_WITH_HEAP) {
    uint16_t totalSize = 0;
    for (uint8_t i = 0; i < schema->numCols; i++) {
      totalSize += abs(schema->columnSizes[i]);
    }
    retval = calloc(1, totalSize);
  }

  return retval;
}

/**
 * @brief	Deep copy schema and return a pointer to the copy
 */
embedDBSchema *
copySchema(const embedDBSchema* schema)
{
  embedDBSchema* copy = NULL;

  if (EDB_WITH_HEAP) {    
    copy = malloc(sizeof(embedDBSchema));
    if (!copy) {
      EDB_PERRF("ERROR: malloc failed while copying schema.\n");
    }
    else {
      copy->numCols     = schema->numCols;
      copy->columnSizes = malloc(schema->numCols * sizeof(int8_t));
      if (!copy->columnSizes) {
        EDB_PERRF("ERROR: malloc failed while copying schema\n");
        free(copy);
	copy = NULL;
      }
      else {
	memcpy(copy->columnSizes, schema->columnSizes,
	       schema->numCols * sizeof(int8_t));
      }
    }
  }
  
  return copy;
}

/**
 * @brief	Finds byte offset of the column from the beginning of the record
 */
uint16_t
getColOffsetFromSchema(embedDBSchema * schema,
		       uint8_t         colNum)
{
  uint16_t pos = 0;
  for (uint8_t i = 0; i < colNum; i++) {
    pos += abs(schema->columnSizes[i]);
  }
  return pos;
}

/**
 * @brief	Calculates record size from schema
 */
uint16_t
getRecordSizeFromSchema(embedDBSchema * schema)
{
  uint16_t size = 0;
  for (uint8_t i = 0; i < schema->numCols; i++) {
    size += abs(schema->columnSizes[i]);
  }
  return size;
}

void
printSchema(embedDBSchema * schema)
{
  for (uint8_t i = 0; i < schema->numCols; i++) {
    if (i) {
      EDB_PRINTF(", ");
    }
    int8_t col = schema->columnSizes[i];
    EDB_PRINTF("%sint%d", embedDB_IS_COL_SIGNED(col) ? "" : "u", abs(col));
  }
  EDB_PRINTF("\n");
}

/************************************************************advancedQueries.c************************************************************/
/******************************************************************************/
/**
 * @file        advancedQueries.c
 * @author      EmbedDB Team (See Authors.md)
 * @brief       Source code file for the advanced query interface for EmbedDB
 * @copyright   Copyright 2024
 *              EmbedDB Team
 * @par Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 * @par 1.Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * @par 2.Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * @par 3.Neither the name of the copyright holder nor the names of its contributors
 *  may be used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 * @par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/  

#if defined(ARDUINO) 
#endif

/**
 * @return	Returns -1, 0, 1 as a comparator normally would
 */
static int8_t
compareUnsignedNumbers(const void * num1,
		       const void * num2,
		       int8_t       numBytes)
{
  // Cast the pointers to unsigned char pointers for byte-wise comparison
  const uint8_t* bytes1 = (const uint8_t*)num1;
  const uint8_t* bytes2 = (const uint8_t*)num2;
  
  for (int8_t i = numBytes - 1; i >= 0; i--) {
    if (bytes1[i] < bytes2[i]) {
      return -1;
    } else if (bytes1[i] > bytes2[i]) {
      return 1;
    }
  }
  
  // Both numbers are equal
  return 0;
}

/**
 * @return	Returns -1, 0, 1 as a comparator normally would
 */
static int8_t
compareSignedNumbers(const void * num1,
		     const void * num2,
		     int8_t       numBytes)
{
  // Cast the pointers to unsigned char pointers for byte-wise comparison
  const uint8_t* bytes1 = (const uint8_t*)num1;
  const uint8_t* bytes2 = (const uint8_t*)num2;
  
  // Check the sign bits of the most significant bytes
  int sign1 = bytes1[numBytes - 1] & 0x80;
  int sign2 = bytes2[numBytes - 1] & 0x80;
  
  if (sign1 != sign2) {
    // Different signs, negative number is smaller
    return (sign1 ? -1 : 1);
  }
  
  // Same sign, perform regular byte-wise comparison
  for (int8_t i = numBytes - 1; i >= 0; i--) {
    if (bytes1[i] < bytes2[i]) {
      return -1;
    } else if (bytes1[i] > bytes2[i]) {
      return 1;
    }
  }
  
  // Both numbers are equal
  return 0;
}

/**
 * @return	0 or 1 to indicate if inequality is true
 */
static int8_t
compare(void *  a,
	uint8_t operation,
	void *  b,
	int8_t  isSigned,
	int8_t  numBytes)
{
  int8_t (*compFunc)(const void* num1, const void* num2, int8_t numBytes) =
    isSigned ? compareSignedNumbers : compareUnsignedNumbers;
  switch (operation) {
  case SELECT_GT:
    return compFunc(a, b, numBytes) > 0;
  case SELECT_LT:
    return compFunc(a, b, numBytes) < 0;
  case SELECT_GTE:
    return compFunc(a, b, numBytes) >= 0;
  case SELECT_LTE:
    return compFunc(a, b, numBytes) <= 0;
  case SELECT_EQ:
    return compFunc(a, b, numBytes) == 0;
  case SELECT_NEQ:
    return compFunc(a, b, numBytes) != 0;
  default:
    return 0;
  }
}

/**
 * @brief	Extract a record from an operator
 * @return	1 if a record was returned, 0 if there are no more rows to return
 */
int8_t
exec(embedDBOperator* op) {
  return op->next(op);
}

static void
initTableScan(embedDBOperator* op)
{
  if (op->input != NULL) {
    EDB_PERRF("WARNING: TableScan operator should not have an input operator\n");
  }
  if (op->schema == NULL) {
    EDB_PERRF("ERROR: TableScan operator needs its schema defined\n");
    return;
  }
  
  if (op->schema->numCols < 2) {
    EDB_PERRF("ERROR: When creating a table scan, you must include at least two columns:"
	      " one for the key and one for the data from the iterator\n");
    return;
  }
  
  // Check that the provided key schema matches what is in the state
  embedDBState* embedDBstate = (embedDBState*)(((void**)op->state)[0]);
  if ((op->schema->columnSizes[0] <= 0) ||
      (abs(op->schema->columnSizes[0]) != embedDBstate->keySize)) {
    EDB_PERRF("ERROR: Make sure the the key column is at index 0 of the schema "
	      "initialization and that it matches the keySize in the state and is unsigned.\n");
    return;
  }
  if (getRecordSizeFromSchema(op->schema) != (embedDBstate->keySize + embedDBstate->dataSize)) {
    EDB_PERRF("ERROR: Size of provided schema doesn't match the size that "
	      "will be returned by the provided iterator.\n");
    return;
  }

  // Init buffer
  if (op->recordBuffer == NULL) {
    op->recordBuffer = createBufferFromSchema(op->schema);
    if (op->recordBuffer == NULL) {
      EDB_PERRF("ERROR: Failed to allocate buffer for TableScan operator.\n");
      return;
    }
  }
}

static int8_t
nextTableScan(embedDBOperator* op)
{
  // Check that a schema was set
  if (op->schema == NULL) {
    EDB_PERRF("ERROR: Must provide a base schema for a table scan operator\n");
    return 0;
  }
  
  // Get next record
  embedDBState* state = (embedDBState*)(((void**)op->state)[0]);
  embedDBIterator* it = (embedDBIterator*)(((void**)op->state)[1]);
  if (!embedDBNext(state, it, op->recordBuffer, (int8_t*)op->recordBuffer + state->keySize)) {
    return 0;
  }
  
  return 1;
}

static void
closeTableScan(embedDBOperator* op)
{
  embedDBFreeSchema(&op->schema);
  if (EDB_WITH_HEAP) {
    free(op->recordBuffer);
    op->recordBuffer = NULL;
    free(op->state);
    op->state = NULL;
  }
}

/**
 * @brief	Used as the bottom operator that will read records from the database
 * @param	state		The state associated with the database to read from
 * @param	it			An initialized iterator setup to read relevent records for this query
 * @param	baseSchema	The schema of the database being read from
 */
embedDBOperator*
createTableScanOperator(embedDBState *    state,
			embedDBIterator * it,
			embedDBSchema *   baseSchema)
{
  embedDBOperator * op = NULL;
  
  // Ensure all fields are not NULL
  if (!state || !it || !baseSchema) {
    EDB_PERRF("ERROR: All parameters must be provided to create a TableScan operator.\n");
  }
  else {
    if (EDB_WITH_HEAP) {
      op = malloc(sizeof(embedDBOperator));
    }
    if (!op) {
      EDB_PERRF("ERROR: no memory for TableScan operator.\n");
    }
    else {
      op->state = NULL;
      if (EDB_WITH_HEAP) {
	op->state = malloc(2 * sizeof(void*));
      }
      if (!op->state) {
	EDB_PERRF("ERROR: no memory for TableScan operator state.\n");
	free(op);
	op = NULL;
      }
      else {
	memcpy(op->state, &state, sizeof(void*));
	memcpy((int8_t*)op->state + sizeof(void*), &it, sizeof(void*));
	
	op->schema = copySchema(baseSchema);
	op->input = NULL;
	op->recordBuffer = NULL;
	
	op->init = initTableScan;
	op->next = nextTableScan;
	op->close = closeTableScan;
      }
    }
  }
  
  return op;
}

static void
initProjection(embedDBOperator* op)
{
  if (!op->input) {
    EDB_PERRF("ERROR: Projection needs an input operator.\n");
  }
  else {
    // Init input
    op->input->init(op->input);

    // Get state
    uint8_t   numCols = *(uint8_t*)op->state;
    uint8_t * cols    = (uint8_t*)op->state + 1;
    const embedDBSchema* inputSchema = op->input->schema;

    // Init output schema
    if (EDB_WITH_HEAP) {
      if (!op->schema) {
        op->schema = malloc(sizeof(embedDBSchema));
      }
      if (!op->schema) {
	EDB_PERRF("ERROR: no memory for projection schema.\n");
      }
      else {
        op->schema->numCols = numCols;
	op->schema->columnSizes = malloc(numCols * sizeof(int8_t));
        if (!op->schema->columnSizes) {
	  EDB_PERRF("ERROR: no memory for projection while building schema.\n");
	  free(op->schema);
	  op->schema = NULL;
        }
	else {
	  for (unsigned i=0; i<numCols; i++) {
	    op->schema->columnSizes[i] = inputSchema->columnSizes[cols[i]];
	  }
	}
      }

      // Init output buffer
      if (!op->recordBuffer) {
        op->recordBuffer = createBufferFromSchema(op->schema);
        if (!op->recordBuffer) {
	  EDB_PERRF("ERROR: no memory for record buffer in TableScan operator.\n");
        }
      }
    }
  }
}

static int8_t
nextProjection(embedDBOperator* op)
{
  uint8_t numCols = *(uint8_t*)op->state;
  uint8_t* cols = (uint8_t*)op->state + 1;
  embedDBSchema* inputSchema = op->input->schema;
  
  // Get next record
  if (op->input->next(op->input)) {
    uint16_t curColPos = 0;
    for (uint8_t colIdx = 0; colIdx < numCols; colIdx++) {
      uint8_t col = cols[colIdx];
      uint8_t colSize = abs(inputSchema->columnSizes[col]);
      uint16_t srcColPos = getColOffsetFromSchema(inputSchema, col);
      memcpy((int8_t*)op->recordBuffer + curColPos, (int8_t*)op->input->recordBuffer + srcColPos, colSize);
      curColPos += colSize;
    }
    return 1;
  } else {
    return 0;
  }
}

static void
closeProjection(embedDBOperator* op)
{
  op->input->close(op->input);
  
  embedDBFreeSchema(&op->schema);
  if (EDB_WITH_HEAP) {
    free(op->state);
    op->state = NULL;
    free(op->recordBuffer);
    op->recordBuffer = NULL;
  }
}

/**
 * @brief	Creates an operator capable of projecting the specified columns. Cannot re-order columns
 * @param	input	The operator that this operator can pull records from
 * @param	numCols	How many columns will be in the final projection
 * @param cols The indexes of the columns to be outputted. Zero
 *             indexed. Column indexes must be strictly increasing
 *             i.e. columns must stay in the same order, can only
 *             remove columns from input
 */
embedDBOperator *
createProjectionOperator(embedDBOperator * input,
			 uint8_t           numCols,
			 uint8_t *         cols)
{
  embedDBOperator * op = NULL;

  if (EDB_WITH_HEAP) {
    op = malloc(sizeof(embedDBOperator));

    if (!op) {
      EDB_PERRF("ERROR: no memory for Projection operator.\n");
    }
    else {
      // Create state
      uint8_t * state = malloc(numCols + 1);
      if (!state) {
	EDB_PERRF("ERROR: no memory for state of Projection operator.\n");
	free(op);
	op = NULL;
      }
      else {
	state[0] = numCols;
	memcpy(state + 1, cols, numCols);

	op->state = state;
	op->input = input;
	op->schema = NULL;
	op->recordBuffer = NULL;
	op->init = initProjection;
	op->next = nextProjection;
	op->close = closeProjection;
      }
    }
  }
  
  return op;
}

// -------------------------------------------------------------------------

struct selectionInfo {
  int8_t colNum;
  int8_t operation;
  void * compVal;
};

static void
initSelection(embedDBOperator* op)
{
  if (op->input == NULL) {
    EDB_PERRF("ERROR: Projection operator needs an input operator\n");
    return;
  }
  
  // Init input
  op->input->init(op->input);
  
  // Init output schema
  if (op->schema == NULL) {
    op->schema = copySchema(op->input->schema);
  }
  
  // Init output buffer
  if (op->recordBuffer == NULL) {
    op->recordBuffer = createBufferFromSchema(op->schema);
    if (op->recordBuffer == NULL) {
      EDB_PERRF("ERROR: Failed to allocate buffer for TableScan operator\n");
      return;
    }
  }
}

static int8_t
nextSelection(embedDBOperator* op)
{
  embedDBSchema* schema = op->input->schema;
  struct selectionInfo* state = op->state;
  
  int8_t colNum = state->colNum;
  uint16_t colPos = getColOffsetFromSchema(schema, colNum);
  int8_t operation = state->operation;
  int8_t colSize = schema->columnSizes[colNum];
  int8_t isSigned = 0;
  if (colSize < 0) {
    colSize = -colSize;
    isSigned = 1;
  }
  
  while (op->input->next(op->input)) {
    void* colData = (int8_t*)op->input->recordBuffer + colPos;
    if (compare(colData, operation, state->compVal, isSigned, colSize)) {
      memcpy(op->recordBuffer, op->input->recordBuffer, getRecordSizeFromSchema(op->schema));
      return 1;
    }
  }
  
  return 0;
}

static void
closeSelection(embedDBOperator* op)
{
  op->input->close(op->input);
  
  embedDBFreeSchema(&op->schema);
  if (EDB_WITH_HEAP) {
    free(op->state);
    op->state = NULL;
    free(op->recordBuffer);
    op->recordBuffer = NULL;
  }
}

/**
 * @brief Creates an operator that selects records based on simple selection rules
 * @param input     The operator that this operator can pull records from
 * @param colNum    The index (zero-indexed) of the column base the
 *                  select on
 * @param operation A constant representing which comparison operation
 *                  to perform. (e.g. SELECT_GT, SELECT_EQ, etc)
 * @param compVal   A pointer to the value to compare with. Make sure
 *                  the size of this is the same number of bytes as is
 *                  described in the schema
 */
embedDBOperator *
createSelectionOperator(embedDBOperator * input,
			int8_t            colNum,
			int8_t            operation,
			void *            compVal)
{
  embedDBOperator * op = NULL;

  if (EDB_WITH_HEAP) {
    op = malloc(sizeof(embedDBOperator));
    if (!op) {
      EDB_PERRF("ERROR: no memory for Selection operator.\n");
    }
    else {    
      struct selectionInfo * state = malloc(sizeof(struct selectionInfo));
      if (!state) {
	EDB_PERRF("ERROR: no memory for state of Selection operator.\n");
	free(op);
	op = NULL;
      }
      else {
	state->colNum = colNum;
	state->operation = operation;
	memcpy(&state->compVal, &compVal, sizeof(void*));
  
	op->state = state;
	op->input = input;
	op->schema = NULL;
	op->recordBuffer = NULL;
	op->init = initSelection;
	op->next = nextSelection;
	op->close = closeSelection;
      }
    }
  }
  
  return op;
}

// -------------------------------------------------------------------------

typedef int8_t (*embedDBGroupFunc)(const void* lastRecord, const void* record);

/**
 * @brief	A private struct to hold the state of the aggregate operator
 */
struct aggregateInfo {
  embedDBGroupFunc       groupfunc;        // Function that determins if both records are in the same group
  embedDBAggregateFunc * functions;        // An array of aggregate functions
  uint32_t               functionsLength;  // The length of the functions array
  void *                 lastRecordBuffer; // Buffer for the last record read by input->next
  uint16_t               bufferSize;       // Size of the input buffer (and lastRecordBuffer)
  int8_t                 isLastRecordUsable;  // Is the data in
					      // lastRecordBuffer
					      // usable for checking
					      // if the recently read
					      // record is in the same
					      // group? Is set to 0 at
					      // start, and also after
					      // the last record
};

static void
initAggregate(embedDBOperator * op)
{
  if (!op->input) {
    EDB_PERRF("ERROR: Aggregate operator needs an input operator.\n");
  }
  else {
    // Init input
    op->input->init(op->input);
    
    struct aggregateInfo* state = op->state;
    state->isLastRecordUsable = 0;

    if (EDB_WITH_HEAP) {
      // Init output schema
      if (!op->schema) {
	op->schema = malloc(sizeof(embedDBSchema));
	if (!op->schema) {
	  EDB_PERRF("ERROR: no memory for aggregate operator\n");
	}
	else {
	  op->schema->numCols     = state->functionsLength;
	  op->schema->columnSizes = malloc(state->functionsLength);
	  if (!op->schema->columnSizes) {
	    EDB_PERRF("ERROR: no memory for column sizes in aggregate operator\n");
	    free(op->schema);
	    op->schema = NULL;
	  }
	  else {
	    for (unsigned i=0; i<state->functionsLength; i++) {
	      op->schema->columnSizes[i] = state->functions[i].colSize;
	      state->functions[i].colNum = i;
	    }
	  }
	}
      }
    }
    
    // Init buffers
    state->bufferSize = getRecordSizeFromSchema(op->input->schema);
    if (!op->recordBuffer) {
      op->recordBuffer = createBufferFromSchema(op->schema);
      if (!op->recordBuffer) {
	EDB_PERRF("ERROR: no memory for record buffer in aggregate operator.\n");
      }
    }

    if (EDB_WITH_HEAP) {
      if (!state->lastRecordBuffer) {
	state->lastRecordBuffer = malloc(state->bufferSize);
	if (!state->lastRecordBuffer) {
	  EDB_PERRF("ERROR: no memory for last record buffer in  aggregate operator.\n");
	}
      }
    }
  }
}

static int8_t
nextAggregate(embedDBOperator* op)
{
  struct aggregateInfo* state = op->state;
  embedDBOperator* input = op->input;
  
  // Reset each operator
  for (int i = 0; i < state->functionsLength; i++) {
    if (state->functions[i].reset != NULL) {
      state->functions[i].reset(state->functions + i, input->schema);
    }
  }
  
  int8_t recordsInGroup = 0;
  
  // Check flag used to indicate whether the last record read has been added to a group
  if (state->isLastRecordUsable) {
    recordsInGroup = 1;
    for (int i = 0; i < state->functionsLength; i++) {
      if (state->functions[i].add != NULL) {
	state->functions[i].add(state->functions + i, input->schema, state->lastRecordBuffer);
      }
    }
  }
  
  int8_t exitType = 0;
  while (input->next(input)) {
    // Check if record is in the same group as the last record
    if (!state->isLastRecordUsable || state->groupfunc(state->lastRecordBuffer, input->recordBuffer)) {
      recordsInGroup = 1;
      for (int i = 0; i < state->functionsLength; i++) {
	if (state->functions[i].add != NULL) {
	  state->functions[i].add(state->functions + i, input->schema, input->recordBuffer);
	}
      }
    } else {
      exitType = 1;
      break;
    }
    
    // Save this record
    memcpy(state->lastRecordBuffer, input->recordBuffer, state->bufferSize);
    state->isLastRecordUsable = 1;
  }
  
  if (!recordsInGroup) {
    return 0;
  }
  
  if (exitType == 0) {
    // Exited because ran out of records, so all read records have been added to a group
    state->isLastRecordUsable = 0;
  }
  
  // Perform final compute on all functions
  for (int i = 0; i < state->functionsLength; i++) {
    if (state->functions[i].compute != NULL) {
      state->functions[i].compute(state->functions + i, op->schema, op->recordBuffer, state->lastRecordBuffer);
    }
  }
  
  // Put last read record into lastRecordBuffer
  memcpy(state->lastRecordBuffer, input->recordBuffer, state->bufferSize);
  
  return 1;
}

static void
closeAggregate(embedDBOperator* op)
{
  op->input->close(op->input);
  op->input = NULL;
  embedDBFreeSchema(&op->schema);
  
  if (EDB_WITH_HEAP) {
    free(((struct aggregateInfo*)op->state)->lastRecordBuffer);
    free(op->state);
    op->state = NULL;
    free(op->recordBuffer);
    op->recordBuffer = NULL;
  }
}

/**
 * @brief Creates an operator that will find groups and preform aggregate functions over each group.
 * @param input           The operator that this operator can pull records from
 * @param groupfunc       A function that returns whether or not the @c
 *                        record is part of the same group as the @c
 *                        lastRecord. Assumes that records in groups
 *                        are always next to each other and sorted
 *                        when read in (i.e. Groups need to be
 *                        1122333, not 13213213)
 * @param functions       An array of aggregate functions, each of which
 *                        will be updated with each record read from
 *                        the iterator
 * @param functionsLength  The number of embedDBAggregateFuncs in @c functions
 */
embedDBOperator *
createAggregateOperator(embedDBOperator *      input,
			embedDBGroupFunc       groupfunc,
			embedDBAggregateFunc * functions,
			uint32_t               functionsLength)
{
  embedDBOperator* op = NULL;

  if (EDB_WITH_HEAP) {
    op = malloc(sizeof(embedDBOperator));
    if (!op) {
      EDB_PERRF("ERROR: no memory for aggregate operator.\n");      
    }
    else {  
      struct aggregateInfo * state = malloc(sizeof(struct aggregateInfo));
      if (!state) {
	EDB_PERRF("ERROR: no memory for state of aggregate operator.\n");
	free(op);
	op = NULL;
      }
      else {  
	state->groupfunc = groupfunc;
	state->functions = functions;
	state->functionsLength = functionsLength;
	state->lastRecordBuffer = NULL;
  
	op->state = state;
	op->input = input;
	op->schema = NULL;
	op->recordBuffer = NULL;
	op->init = initAggregate;
	op->next = nextAggregate;
	op->close = closeAggregate;
      }
    }
  }
  
  return op;
}

// -------------------------------------------------------------------------

struct keyJoinInfo {
  embedDBOperator * input2;
  int8_t            firstCall;
};

static void
initKeyJoin(embedDBOperator * op)
{
  struct keyJoinInfo* state = op->state;
  embedDBOperator* input1 = op->input;
  embedDBOperator* input2 = state->input2;
  
  // Init inputs
  input1->init(input1);
  input2->init(input2);
  
  embedDBSchema* schema1 = input1->schema;
  embedDBSchema* schema2 = input2->schema;
  
  // Check that join is compatible
  if ((schema1->columnSizes[0] != schema2->columnSizes[0]) ||
      (schema1->columnSizes[0] < 0)                        ||
      (schema2->columnSizes[0] < 0)) {
    EDB_PERRF("ERROR: The first columns of the two tables must be the key "
	      "and must be the same size. Make sure you haven't projected them out.\n");
  }
  else {
    if (EDB_WITH_HEAP) {
      // Setup schema
      if (!op->schema) {
	op->schema = malloc(sizeof(embedDBSchema));
	if (!op->schema) {
	  EDB_PERRF("ERROR: no memory for join operator.\n");
	}
	else {
	  op->schema->numCols = schema1->numCols + schema2->numCols;
	  op->schema->columnSizes = malloc(op->schema->numCols * sizeof(int8_t));
	  if (!op->schema->columnSizes) {
	    EDB_PERRF("ERROR: no memory for columns sizes of join operator.\n");
	    free(op->schema);
	    op->schema = NULL;
	  }
	  else {
	    memcpy(op->schema->columnSizes, schema1->columnSizes, schema1->numCols);
	    memcpy(op->schema->columnSizes + schema1->numCols, schema2->columnSizes, schema2->numCols);
	  }
	}
      }
    
      // Allocate recordBuffer
      op->recordBuffer = malloc(getRecordSizeFromSchema(op->schema));
      if (!op->recordBuffer) {
	EDB_PERRF("ERROR: no memory for recordBuffer of join operator.\n");
      }
    }
    
   state->firstCall = 1;
  }
}

static int8_t
nextKeyJoin(embedDBOperator* op)
{
  struct keyJoinInfo * state   = op->state;
  embedDBOperator *    input1  = op->input;
  embedDBOperator *    input2  = state->input2;
  embedDBSchema *      schema1 = input1->schema;
  embedDBSchema *      schema2 = input2->schema;

  // We've already used this match
  void * record1 = input1->recordBuffer;
  void * record2 = input2->recordBuffer;
  
  int8_t colSize = abs(schema1->columnSizes[0]);
  
  if (state->firstCall) {
    state->firstCall = 0;
    
    if (!input1->next(input1) || !input2->next(input2)) {
      // If this case happens, you goofed, but I'll handle it anyway
      return 0;
    }
    goto check;
  }
  
  while (1) {
    // Advance the input with the smaller value
    int8_t comp = compareUnsignedNumbers(record1, record2, colSize);
    if (comp == 0) {
      // Move both forward because if they match at this point,
      // they've already been matched
      if (!input1->next(input1) || !input2->next(input2)) {
	return 0;
      }
    }
    else if (comp < 0) {
      // Move record 1 forward
      if (!input1->next(input1)) {
	// We are out of records on one side. Given the assumption
	// that the inputs are sorted, there are no more possible
	// joins
	return 0;
      }
    }
    else {
      // Move record 2 forward
      if (!input2->next(input2)) {
	// We are out of records on one side. Given the assumption
	// that the inputs are sorted, there are no more possible
	// joins
	return 0;
      }
    }
    
  check:
    // See if these records join
    if (compareUnsignedNumbers(record1, record2, colSize) == 0) {
      // Copy both records into the output
      uint16_t record1Size = getRecordSizeFromSchema(schema1);
      memcpy(op->recordBuffer, input1->recordBuffer, record1Size);
      memcpy((int8_t*)op->recordBuffer + record1Size,
	     input2->recordBuffer, getRecordSizeFromSchema(schema2));
      return 1;
    }
    // Else keep advancing inputs until a match is found
  }
  
  return 0;
}

static void
closeKeyJoin(embedDBOperator* op)
{
  struct keyJoinInfo* state = op->state;
  embedDBOperator* input1 = op->input;
  embedDBOperator* input2 = state->input2;
  input1->close(input1);
  input2->close(input2);
  
  embedDBFreeSchema(&op->schema);
  if (EDB_WITH_HEAP) {
    free(op->state);
    op->state = NULL;
    free(op->recordBuffer);
    op->recordBuffer = NULL;
  }
}

/**
 * @brief Creates an operator for perfoming an equijoin on the keys
 *        (sorted and distinct) of two tables
 */
embedDBOperator *
createKeyJoinOperator(embedDBOperator * input1,
		      embedDBOperator * input2)
{
  embedDBOperator* op = NULL;

  if (EDB_WITH_HEAP) {
    op = malloc(sizeof(embedDBOperator));
    if (!op) {
      EDB_PERRF("ERROR: no memory for join operator\n");
    }
    else {
      struct keyJoinInfo * state = malloc(sizeof(struct keyJoinInfo));
      if (!state) {
	EDB_PERRF("ERROR: no memory for state of join operator.\n");
      }
      else {
	state->input2 = input2;
  
	op->input = input1;
	op->state = state;
	op->recordBuffer = NULL;
	op->schema = NULL;
	op->init = initKeyJoin;
	op->next = nextKeyJoin;
	op->close = closeKeyJoin;
      }
    }
  }
  
  return op;
}

// -------------------------------------------------------------------------

static void
countReset(embedDBAggregateFunc * aggFunc,
	   embedDBSchema *        inputSchema)
{
  *(uint32_t*)aggFunc->state = 0;
}

static void
countAdd(embedDBAggregateFunc * aggFunc,
	 embedDBSchema *        inputSchema,
	 const void *           recordBuffer)
{
  (*(uint32_t*)aggFunc->state)++;
}

static void
countCompute(embedDBAggregateFunc * aggFunc,
	     embedDBSchema *        outputSchema,
	     void *                 recordBuffer,
	     const void *           lastRecord)
{
  // Put count in record
  memcpy((int8_t*)recordBuffer + getColOffsetFromSchema(outputSchema, aggFunc->colNum),
	 aggFunc->state, sizeof(uint32_t));
}

/**
 * @brief Creates an aggregate function to count the number of records
 *        in a group. To be used in combination with an
 *        embedDBOperator produced by createAggregateOperator
 */
embedDBAggregateFunc *
createCountAggregate(void) {
  embedDBAggregateFunc* aggFunc = NULL;
  if (EDB_WITH_HEAP) {
    aggFunc = malloc(sizeof(embedDBAggregateFunc));
    if (aggFunc) {
      aggFunc->reset = countReset;
      aggFunc->add = countAdd;
      aggFunc->compute = countCompute;
      aggFunc->state = malloc(sizeof(uint32_t));
      aggFunc->colSize = 4;
    }
  }
  return aggFunc;
}

// -------------------------------------------------------------------------

static void
sumReset(embedDBAggregateFunc * aggFunc,
	 embedDBSchema *        inputSchema)
{
  if (abs(inputSchema->columnSizes[*((uint8_t*)aggFunc->state + sizeof(int64_t))]) > 8) {
    EDB_PERRF("WARNING: Can't use this sum function for columns bigger than 8 bytes.\n");
  }
  *(int64_t*)aggFunc->state = 0;
}

static void
sumAdd(embedDBAggregateFunc * aggFunc,
       embedDBSchema *        inputSchema,
       const void *           recordBuffer)
{
  uint8_t colNum   = *((uint8_t*)aggFunc->state + sizeof(int64_t));
  int8_t  colSize  = inputSchema->columnSizes[colNum];
  int8_t  isSigned = embedDB_IS_COL_SIGNED(colSize);
  void*   colPos   = (int8_t*)recordBuffer + getColOffsetFromSchema(inputSchema, colNum);

  colSize = min(abs(colSize), sizeof(int64_t));
  
  if (isSigned) {
    // Get val to sum from record
    int64_t val = 0;
    memcpy(&val, colPos, colSize);
    // Extend two's complement sign to fill 64 bit number if val is negative
    int64_t sign = val & (128 << ((colSize - 1) * 8));
    if (sign != 0) {
      memset(((int8_t*)(&val)) + colSize, 0xff, sizeof(int64_t) - colSize);
    }
    (*(int64_t*)aggFunc->state) += val;
  }
  else {
    uint64_t val = 0;
    memcpy(&val, colPos, colSize);
    (*(uint64_t*)aggFunc->state) += val;
  }
}

static void
sumCompute(embedDBAggregateFunc * aggFunc,
	   embedDBSchema *        outputSchema,
	   void *                 recordBuffer,
	   const void *           lastRecord)
{
  // Put count in record
  memcpy((int8_t*)recordBuffer + getColOffsetFromSchema(outputSchema, aggFunc->colNum),
	 aggFunc->state, sizeof(int64_t));
}

/**
 * @brief Creates an aggregate function to sum a column over a
 *        group. To be used in combination with an embedDBOperator
 *        produced by createAggregateOperator. Column must be no
 *        bigger than 8 bytes.
  * @param colNum The index (zero-indexed) of the column which you
  *               want to sum. Column must be <= 8 bytes
 */
embedDBAggregateFunc *
createSumAggregate(uint8_t colNum)
{
  embedDBAggregateFunc* aggFunc = NULL;
  if (EDB_WITH_HEAP) {
    aggFunc = malloc(sizeof(embedDBAggregateFunc));
    if (aggFunc) {
      aggFunc->reset = sumReset;
      aggFunc->add = sumAdd;
      aggFunc->compute = sumCompute;
      aggFunc->state = malloc(sizeof(int8_t) + sizeof(int64_t));
      *((uint8_t*)aggFunc->state + sizeof(int64_t)) = colNum;
      aggFunc->colSize = -8;
    }
  }
  return aggFunc;
}

// -------------------------------------------------------------------------

struct minMaxState {
    uint8_t colNum;   // Which column of input to use
    void *  current;  // The value currently regarded as the min/max
};

static void
minReset(embedDBAggregateFunc * aggFunc,
	 embedDBSchema *        inputSchema)
{
  struct minMaxState * state   = aggFunc->state;
  int8_t               colSize = inputSchema->columnSizes[state->colNum];
  if (aggFunc->colSize != colSize) {
    EDB_PERRF("WARNING: Your provided column size for min aggregate function "
	      "doesn't match the column size in the input schema\n");
  }
  int8_t isSigned = embedDB_IS_COL_SIGNED(colSize);
  colSize = abs(colSize);
  memset(state->current, 0xff, colSize);
  if (isSigned) {
    // If the number is signed, flip MSB else it will read as -1, not MAX_INT
    memset((int8_t*)state->current + colSize - 1, 0x7f, 1);
  }
}

static void
minAdd(embedDBAggregateFunc * aggFunc,
       embedDBSchema *        inputSchema,
       const void *           record)
{
  struct minMaxState * state    = aggFunc->state;
  int8_t               colSize  = inputSchema->columnSizes[state->colNum];
  int8_t               isSigned = embedDB_IS_COL_SIGNED(colSize);
  void*                newValue = (int8_t*)record + getColOffsetFromSchema(inputSchema, state->colNum);
  colSize = abs(colSize);
  if (compare(newValue, SELECT_LT, state->current, isSigned, colSize)) {
    memcpy(state->current, newValue, colSize);
  }
}

static void
minMaxCompute(embedDBAggregateFunc * aggFunc,
	      embedDBSchema *        outputSchema,
	      void *                 recordBuffer,
	      const void *           lastRecord)
{
  // Put count in record
  memcpy((int8_t*)recordBuffer + getColOffsetFromSchema(outputSchema, aggFunc->colNum),
	 ((struct minMaxState*)aggFunc->state)->current,
	 abs(outputSchema->columnSizes[aggFunc->colNum]));
}

/**
 * @brief Creates an aggregate function to find the min value in a group
 * @param colNum   The zero-indexed column to find the min of
 * @param colSize  The size, in bytes, of the column to find the min
 *                 of. Negative number represents a signed number,
 *                 positive is unsigned.
 */
embedDBAggregateFunc *
createMinAggregate(uint8_t colNum,
		   int8_t  colSize)
{
  embedDBAggregateFunc * aggFunc = NULL;
  
  if (EDB_WITH_HEAP) {
    aggFunc = malloc(sizeof(embedDBAggregateFunc));
    if (!aggFunc) {
      EDB_PERRF("ERROR: no memory for min aggregate function.\n");
    }
    else {
      struct minMaxState * state = malloc(sizeof(struct minMaxState));
      if (!state) {
        EDB_PERRF("ERROR: no memory for state of min aggregate function.\n");
	free(aggFunc);
	aggFunc = NULL;
      }
      else {
	state->colNum = colNum;
	state->current = malloc(abs(colSize));
	if (!state->current) {
	  EDB_PERRF("ERROR: no memory for current attribute of min aggregate function.\n");
	  free(state);
	  free(aggFunc);
	  aggFunc = NULL;
	}
	else {
	  aggFunc->state = state;
	  aggFunc->colSize = colSize;
	  aggFunc->reset = minReset;
	  aggFunc->add = minAdd;
	  aggFunc->compute = minMaxCompute;
	}
      }
    }
  }

  return aggFunc;
}

// -------------------------------------------------------------------------

static void
maxReset(embedDBAggregateFunc * aggFunc,
	 embedDBSchema *        inputSchema)
{
  struct minMaxState * state = aggFunc->state;
  int8_t colSize = inputSchema->columnSizes[state->colNum];
  if (aggFunc->colSize != colSize) {
    EDB_PERRF("WARNING: Your provided column size for max aggregate function doesn't match the column size in the input schema\n");
  }
  int8_t isSigned = embedDB_IS_COL_SIGNED(colSize);
  colSize = abs(colSize);
  memset(state->current, 0, colSize);
  if (isSigned) {
    // If the number is signed, flip MSB else it will read as 0, not MIN_INT
    memset((int8_t*)state->current + colSize - 1, 0x80, 1);
  }
}

static void
maxAdd(embedDBAggregateFunc * aggFunc,
       embedDBSchema *        inputSchema,
       const void *           record)
{
  struct minMaxState * state    = aggFunc->state;
  int8_t               colSize  = inputSchema->columnSizes[state->colNum];
  int8_t               isSigned = embedDB_IS_COL_SIGNED(colSize);
  void *               newValue = (int8_t*)record + getColOffsetFromSchema(inputSchema, state->colNum);

  colSize = abs(colSize);
  
  if (compare(newValue, SELECT_GT, state->current, isSigned, colSize)) {
    memcpy(state->current, newValue, colSize);
  }
}

/**
 * @brief Creates an aggregate function to find the max value in a group
 * @param colNum The zero-indexed column to find the max of
 * @param colSize The size, in bytes, of the column to find the max
 *                of. Negative number represents a signed number,
 *                positive is unsigned.
 */
embedDBAggregateFunc *
createMaxAggregate(uint8_t colNum,
		   int8_t  colSize)
{
  embedDBAggregateFunc* aggFunc = NULL;

  if (EDB_WITH_HEAP) {
    aggFunc = malloc(sizeof(embedDBAggregateFunc));
    if (!aggFunc) {
      EDB_PERRF("ERROR: no memory for max aggregate function.\n");
    }
    else {
      struct minMaxState * state = malloc(sizeof(struct minMaxState));
      if (!state) {
	EDB_PERRF("ERROR: no memory for state of max aggregate function.\n");
	free(aggFunc);
	aggFunc = NULL;
      }
      else {
	state->colNum = colNum;
	state->current = malloc(abs(colSize));
	if (!state->current) {
	  EDB_PERRF("ERROR: no memory for current attribute of max aggregate function.\n");
	  free(state);
	  free(aggFunc);
	  aggFunc = NULL;
	}
	else {
	  aggFunc->state   = state;
	  aggFunc->colSize = colSize;
	  aggFunc->reset   = maxReset;
	  aggFunc->add     = maxAdd;
	  aggFunc->compute = minMaxCompute;
	}
      }
    }
  }
  
  return aggFunc;
}

// -------------------------------------------------------------------------

struct avgState {
  uint32_t count    : 23; // Count of records seen in group so far
  uint32_t isSigned :  1; // Is input column signed?
  uint32_t colNum   :  8; // Column to take avg of
  int64_t  sum;           // Sum of records seen in group so far
};

static void
avgReset(struct embedDBAggregateFunc * aggFunc,
	 embedDBSchema *               inputSchema)
{
  struct avgState * state = aggFunc->state;
  if (abs(inputSchema->columnSizes[state->colNum]) > 8) {
    EDB_PERRF("WARNING: Can't use this sum function for columns bigger than 8 bytes.\n");
  }
  state->count    = 0;
  state->sum      = 0;
  state->isSigned = embedDB_IS_COL_SIGNED(inputSchema->columnSizes[state->colNum]);
}

static void
avgAdd(struct embedDBAggregateFunc * aggFunc,
       embedDBSchema *               inputSchema,
       const void *                  record)
{
  struct avgState * state    = aggFunc->state;
  uint8_t           colNum   = state->colNum;
  int8_t            colSize  = inputSchema->columnSizes[colNum];
  int8_t            isSigned = embedDB_IS_COL_SIGNED(colSize);
  void *            colPos   = (int8_t*)record + getColOffsetFromSchema(inputSchema, colNum);

  colSize = min(abs(colSize), sizeof(int64_t));
  
  if (isSigned) {
    // Get val to sum from record
    int64_t val = 0;
    memcpy(&val, colPos, colSize);
    // Extend two's complement sign to fill 64 bit number if val is negative
    int64_t sign = val & (128 << ((colSize - 1) * 8));
    if (sign != 0) {
      memset(((int8_t*)(&val)) + colSize, 0xff, sizeof(int64_t) - colSize);
    }
    state->sum += val;
  }
  else {
    uint64_t val = 0;
    memcpy(&val, colPos, colSize);
    val += (uint64_t)state->sum;
    memcpy(&state->sum, &val, sizeof(uint64_t));
  }
  state->count++;
}

static void
avgCompute(struct embedDBAggregateFunc * aggFunc,
	   embedDBSchema *               outputSchema,
	   void *                        recordBuffer,
	   const void *                  lastRecord)
{
  struct avgState * state = aggFunc->state;
  if (aggFunc->colSize == 8) {
    double avg = state->sum / (double)state->count;
    if (state->isSigned) {
      avg = state->sum / (double)state->count;
    }
    else {
      avg = (uint64_t)state->sum / (double)state->count;
    }
    memcpy((int8_t*)recordBuffer + getColOffsetFromSchema(outputSchema, aggFunc->colNum),
	   &avg, sizeof(double));
  }
  else {
    float avg;
    if (state->isSigned) {
      avg = state->sum / (float)state->count;
    } else {
      avg = (uint64_t)state->sum / (float)state->count;
    }
    memcpy((int8_t*)recordBuffer + getColOffsetFromSchema(outputSchema, aggFunc->colNum),
	   &avg, sizeof(float));
  }
}

/**
 * @brief Creates an operator to compute the average of a column over
 *        a group. **WARNING: Outputs a floating point number that may
 *        not be compatible with other operators**
 * @param colNum          Zero-indexed column to take average of
 * @param outputFloatSize Size of float to output. Must be either 4 (float) or 8 (double)
 */
embedDBAggregateFunc *
createAvgAggregate(uint8_t colNum,
		   int8_t  outputFloatSize)
{
  embedDBAggregateFunc* aggFunc = NULL;

  if (EDB_WITH_HEAP) {
    aggFunc = malloc(sizeof(embedDBAggregateFunc));
    if (!aggFunc) {
      EDB_PERRF("ERROR: no memory for avg aggregate function.\n");
    }
    else {    
      struct avgState * state = malloc(sizeof(struct avgState));
      if (!state) {
	EDB_PERRF("ERROR: no memory for state of avg aggregate function.\n");
	free(aggFunc);
	aggFunc = NULL;
      }
      else {
	state->colNum = colNum;
	aggFunc->state = state;
	if ((outputFloatSize > 8) ||
	    ((outputFloatSize < 8) && (outputFloatSize > 4))) {
	  EDB_PERRF("WARNING: The size of the output float for AVG must be exactly 4 or 8. Defaulting to 8.\n");
	  aggFunc->colSize = 8;
	}
	else if (outputFloatSize < 4) {
	  EDB_PERRF("WARNING: The size of the output float for AVG must be exactly 4 or 8. Defaulting to 4.\n");
	  aggFunc->colSize = 4;
	}
	else {
	  aggFunc->colSize = outputFloatSize;
	}
	aggFunc->reset   = avgReset;
	aggFunc->add     = avgAdd;
	aggFunc->compute = avgCompute;
      }
    }
  }
  
  return aggFunc;
}

/**
 * @brief	Completely free a chain of functions recursively after it's already been closed.
 */
void embedDBFreeOperatorRecursive(embedDBOperator** op) {
  if ((*op)->input) {
    embedDBFreeOperatorRecursive(&(*op)->input);
  }
  if ((*op)->state && EDB_WITH_HEAP) {
    free((*op)->state);
    (*op)->state = NULL;
  }
  if ((*op)->schema) {
    embedDBFreeSchema(&(*op)->schema);
  }
  if (EDB_WITH_HEAP) {
    if ((*op)->recordBuffer) {
      free((*op)->recordBuffer);
      (*op)->recordBuffer = NULL;
    }
    free(*op);
  }
  (*op) = NULL;
}

/************************************************************embedDBUtility.c************************************************************/
/******************************************************************************/
/**
 * @file        embedDBUtility.c
 * @author      EmbedDB Team (See Authors.md)
 * @brief       This file contains some utility functions to be used with embedDB.
 *              These include functions required to use the bitmap option, and a
 *              comparator for comparing keys. They can be modified or implemented
 *              differently depending on the application.
 * @copyright   Copyright 2024
 *              EmbedDB Team
 * @par Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 * @par 1.Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * @par 2.Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * @par 3.Neither the name of the copyright holder nor the names of its contributors
 *  may be used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 * @par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/  

/* A bitmap with 8 buckets (bits). Range 0 to 100. */
void updateBitmapInt8(void *data, void *bm) {
    // Note: Assuming int key is right at the start of the data record
    int32_t val = *((int16_t *)data);
    uint8_t *bmval = (uint8_t *)bm;

    if (val < 10)
        *bmval = *bmval | 128;
    else if (val < 20)
        *bmval = *bmval | 64;
    else if (val < 30)
        *bmval = *bmval | 32;
    else if (val < 40)
        *bmval = *bmval | 16;
    else if (val < 50)
        *bmval = *bmval | 8;
    else if (val < 60)
        *bmval = *bmval | 4;
    else if (val < 100)
        *bmval = *bmval | 2;
    else
        *bmval = *bmval | 1;
}

/* A bitmap with 8 buckets (bits). Range 0 to 100. Build bitmap based on min and max value. */
void buildBitmapInt8FromRange(void *min, void *max, void *bm) {
    if (min == NULL && max == NULL) {
        *(uint8_t *)bm = 255; /* Everything */
    } else {
        uint8_t minMap = 0, maxMap = 0;
        if (min != NULL) {
            updateBitmapInt8(min, &minMap);
            // Turn on all bits below the bit for min value (cause the lsb are for the higher values)
            minMap = minMap | (minMap - 1);
            if (max == NULL) {
                *(uint8_t *)bm = minMap;
                return;
            }
        }
        if (max != NULL) {
            updateBitmapInt8(max, &maxMap);
            // Turn on all bits above the bit for max value (cause the msb are for the lower values)
            maxMap = ~(maxMap - 1);
            if (min == NULL) {
                *(uint8_t *)bm = maxMap;
                return;
            }
        }
        *(uint8_t *)bm = minMap & maxMap;
    }
}

int8_t inBitmapInt8(void *data, void *bm) {
    uint8_t *bmval = (uint8_t *)bm;

    uint8_t tmpbm = 0;
    updateBitmapInt8(data, &tmpbm);

    // Return a number great than 1 if there is an overlap
    return tmpbm & *bmval;
}

/* A 16-bit bitmap on a 32-bit int value */
void updateBitmapInt16(void *data, void *bm) {
    int32_t val = *((int32_t *)data);
    uint16_t *bmval = (uint16_t *)bm;

    /* Using a demo range of 0 to 100 */
    // int16_t stepSize = 100 / 15;
    int16_t stepSize = 450 / 15;  // Temperature data in F. Scaled by 10. */
    int16_t minBase = 320;
    int32_t current = minBase;
    uint16_t num = 32768;
    while (val > current) {
        current += stepSize;
        num = num / 2;
    }
    if (num == 0)
        num = 1; /* Always set last bit if value bigger than largest cutoff */
    *bmval = *bmval | num;
}

int8_t inBitmapInt16(void *data, void *bm) {
    uint16_t *bmval = (uint16_t *)bm;

    uint16_t tmpbm = 0;
    updateBitmapInt16(data, &tmpbm);

    // Return a number great than 1 if there is an overlap
    return tmpbm & *bmval;
}

/**
 * @brief	Builds 16-bit bitmap from (min, max) range.
 * @param	state	embedDB state structure
 * @param	min		minimum value (may be NULL)
 * @param	max		maximum value (may be NULL)
 * @param	bm		bitmap created
 */
void buildBitmapInt16FromRange(void *min, void *max, void *bm) {
    if (min == NULL && max == NULL) {
        *(uint16_t *)bm = 65535; /* Everything */
        return;
    } else {
        uint16_t minMap = 0, maxMap = 0;
        if (min != NULL) {
            updateBitmapInt16(min, &minMap);
            // Turn on all bits below the bit for min value (cause the lsb are for the higher values)
            minMap = minMap | (minMap - 1);
            if (max == NULL) {
                *(uint16_t *)bm = minMap;
                return;
            }
        }
        if (max != NULL) {
            updateBitmapInt16(max, &maxMap);
            // Turn on all bits above the bit for max value (cause the msb are for the lower values)
            maxMap = ~(maxMap - 1);
            if (min == NULL) {
                *(uint16_t *)bm = maxMap;
                return;
            }
        }
        *(uint16_t *)bm = minMap & maxMap;
    }
}

/* A 64-bit bitmap on a 32-bit int value */
void updateBitmapInt64(void *data, void *bm) {
    int32_t val = *((int32_t *)data);

    int16_t stepSize = 10;  // Temperature data in F. Scaled by 10. */
    int32_t current = 320;
    int8_t bmsize = 63;
    int8_t count = 0;

    while (val > current && count < bmsize) {
        current += stepSize;
        count++;
    }
    uint8_t b = 128;
    int8_t offset = count / 8;
    b = b >> (count & 7);

    *((char *)((char *)bm + offset)) = *((char *)((char *)bm + offset)) | b;
}

int8_t inBitmapInt64(void *data, void *bm) {
    uint64_t *bmval = (uint64_t *)bm;

    uint64_t tmpbm = 0;
    updateBitmapInt64(data, &tmpbm);

    // Return a number great than 1 if there is an overlap
    return tmpbm & *bmval;
}

/**
 * @brief	Builds 64-bit bitmap from (min, max) range.
 * @param	state	embedDB state structure
 * @param	min		minimum value (may be NULL)
 * @param	max		maximum value (may be NULL)
 * @param	bm		bitmap created
 */
void buildBitmapInt64FromRange(void *min, void *max, void *bm) {
    if (min == NULL && max == NULL) {
        *(uint64_t *)bm = UINT64_MAX; /* Everything */
        return;
    } else {
        uint64_t minMap = 0, maxMap = 0;
        if (min != NULL) {
            updateBitmapInt64(min, &minMap);
            // Turn on all bits below the bit for min value (cause the lsb are for the higher values)
            minMap = minMap | (minMap - 1);
            if (max == NULL) {
                *(uint64_t *)bm = minMap;
                return;
            }
        }
        if (max != NULL) {
            updateBitmapInt64(max, &maxMap);
            // Turn on all bits above the bit for max value (cause the msb are for the lower values)
            maxMap = ~(maxMap - 1);
            if (min == NULL) {
                *(uint64_t *)bm = maxMap;
                return;
            }
        }
        *(uint64_t *)bm = minMap & maxMap;
    }
}

int8_t int32Comparator(void *a, void *b) {
    int32_t i1, i2;
    memcpy(&i1, a, sizeof(int32_t));
    memcpy(&i2, b, sizeof(int32_t));
    int32_t result = i1 - i2;
    if (result < 0)
        return -1;
    if (result > 0)
        return 1;
    return 0;
}

int8_t int64Comparator(void *a, void *b) {
    int64_t result = *((int64_t *)a) - *((int64_t *)b);
    if (result < 0)
        return -1;
    if (result > 0)
        return 1;
    return 0;
}

