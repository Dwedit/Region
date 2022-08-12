#pragma once

struct IUnknown;
#define NOMINMAX
#include <Windows.h>

#include <vector>
using std::vector;
typedef unsigned char byte;

#if defined(_MSC_VER) && _MSC_VER < 1600
#define CPP98 1
#endif


#ifndef REGION_USE_VALID_FLAG
//option to use a "valid flag" instead of using the hrgn variable itself as the valid flag
//(reduces api calls if region switches between simple and complex more than once)
#define REGION_USE_VALID_FLAG 1
#endif

#ifndef REGION_USE_GLOBAL_TEMP_REGION
//Option to use a global variable instead of an instance variable for the temporary region
#define REGION_USE_GLOBAL_TEMP_REGION 1
#endif

#if REGION_USE_GLOBAL_TEMP_REGION
//Frees the temporary region used for combining GDI regions
void FreeTempRegion();
#endif

//Wraps a GDI Region (avoiding Win32 API calls whenever possible), but if the Region can be represented as a RECT, wraps that instead.
class Region
{
private:
	//For rectangluar regions, the entire region.  For Complex regions, the tightest bounding box that encloses the region.
	//For empty regions, a (0, 0, 0, 0) rectangle.
	RECT boundingBox;
	//When this Region becomes a complex region, an hrgn object is created.
	//If this HRGN becomes a rectangle or empty region, it remains valid
	//until this Region object becomes a different rectangle or empty region.
	HRGN hrgn;
#if !REGION_USE_GLOBAL_TEMP_REGION
	//Temporary region, used when a rectangle must be used with Win32 API calls
	HRGN hrgnTemp;
#endif
	//Region type (1 = NULLREGION, 2 = SIMPLEREGION, 3 = COMPLEXREGION)
	byte regionType;
#if REGION_USE_VALID_FLAG
	//Indicates that the HRGN is accurate to the current state of the Region object.
	//HRGN becomes valid when region becomes complex.
	//Can remain valid if the last Win32 region API call turned this into a rectangle or empty region.
	//Will become invalid if it becomes a different rectangle or empty region.
	bool hrgnValid;
#endif
private:
	//Sets hrgn as invalid (Done when becoming a rectangle or null region)
	void SetHrgnInvalid();
	//Sets hrgn as valid (Done after becoming a complex region, which uses the Win32 region API calls)
	void SetHrgnValid();
	//Returns true if the HRGN is valid (Region is complex, or just became a rectangle or empty region)
	bool HrgnIsValid() const;
#if !REGION_USE_GLOBAL_TEMP_REGION
	//Sets the temp region to a rectangle
	HRGN SetTempRegion(const RECT& rect);
#endif
	//Turns this region into a rectangle (sets bounding box, sets region type, invalidates hrgn)
	void BecomeRectangle(int left, int top, int right, int bottom);
	//Turns this region into a rectangle (sets bounding box, sets region type, invalidates hrgn)
	void BecomeRectangle(const RECT& rect);
	//Turns this region into a copy of the other region (possibly a rectangle or null region)
	void BecomeRegion(const Region& otherRegion);
	//Sets the bounding box a rectangle which contains both the current bounding box and the rect argument.
	//This is the min of left/top, and the max of right/bottom.
	//Call SetHrgnInvalid if the region is becoming that rectangle.
	//Don't call SetHrgnInvalid for a complex region having its bounding box recalculated.
	void UnionBoundingBox(const RECT& rect);
	//Sets the bounding box to the intersection of the current bounding box and the rect argument.
	//This is the max of right/bottom, and the min of left/top.
	//After calling this, must check if the dimensions have become zero or negative, and must call SetHrgnInvalid.
	//Used for Intersection with two rectangles
	void IntersectBoundingbox(const RECT& rect);
	//True if rect1 covers up or is equal to rect2
	static bool RectCoversUpOther(const RECT& rect1, const RECT& rect2);
	//True if any part of rect1 overlaps any part of rect2 (adjacent is not overlapping)
	//Only used for Intersection with rectangle
	static bool RectOverlaps(const RECT& rect1, const RECT& rect2);
	//static bool RectOverlapsOrEdgeTouches(const RECT& rect1, const RECT& rect2);
	//static bool RectsAreAlignedHorizontallyOrVertically(const RECT& rect1, const RECT& rect2);
	
	//True if the two rectangles are equal
	static bool RectEquals(const RECT& rect1, const RECT& rect2);
	////True if the union of two rectangles would be a single rectangle
	//static bool RectUnionIsSimple(const RECT& rect1, const RECT& rect2);
	
	//Unions (adds another rectangle to) this Region object.
	//Only call this when this Region is guaranteed to be a rectangle region.
	void UnionRectWithRect(const RECT& other);
	//Called when performing a union with another rectangle would form a complex region, or we are already complex.
	void UnionRectWithRectBecomeComplex(const RECT& other);
	//Creates or Updates the HRGN for this Region object, so we can call Win32 region API functions.
	//Also validates HRGN.  Doesn't do anything if the HRGN was already valid.
	HRGN GetHrgn();
public:
	//Create an empty Region object
	Region();
	//Destructor, frees HRGN if needed
	~Region();
	//Gets the type of the region (1 = NULLREGION, 2 = SIMPLEREGION (rectangle), 3 = COMPLEXREGION)
	DWORD GetRegionType() const;
	//Gets the bounding box for the region.
	//For rectangluar regions, the entire region.  For Complex regions, the tightest bounding box that encloses the region.
	//For empty regions, a (0, 0, 0, 0) rectangle.
	//Returns a const reference that points inside of this object.
	const RECT& GetBoundingBox() const;
	//Copies the bounding box for the region to a rectangle.
	//For rectangluar regions, the entire region.  For Complex regions, the tightest bounding box that encloses the region.
	//For empty regions, a (0, 0, 0, 0) rectangle.
	void GetBoundingBox(RECT* pDestRect) const;
	//Copies the bounding box for the region to a rectangle.
	//For rectangluar regions, the entire region.  For Complex regions, the tightest bounding box that encloses the region.
	//For empty regions, a (0, 0, 0, 0) rectangle.
	void GetBoundingBox(RECT& destRect) const;
	//Copies the bounding box for the region to a rectangle. (3rd and 4th parameters are Width and Height)
	//For rectangluar regions, the entire region.  For Complex regions, the tightest bounding box that encloses the region.
	//For empty regions, a (0, 0, 0, 0) rectangle.
	void GetBoundingBox(int& x, int& y, int& w, int& h) const;
	//Sets this region to the null region
	void Clear();
	//Modifies this Region object, unions the region with a rectangle (adds a rectangle to the region)
	void UnionWith(const RECT& other);
	//Modifies this Region object, unions the region with a rectangle (adds a rectangle to the region)
	void UnionWith(const RECT* pOtherRect);
	//Modifies this Region object, unions the region with another region (adds another region)
	void UnionWith(const Region& otherRegion);
	//Modifies this Region object, unions the region with another region (adds another region)
	//If provided HRGN is bad, becomes a null region.  HRGN parameter is not modified.
	void UnionWith(HRGN otherRegion);
	//Modifies this Region object, unions the region with a rectangle (adds a rectangle to the region, 3rd and 4th parameters are Width and Height)
	void UnionWith(int x, int y, int w, int h);
	//Modifies this Region object, gets the area which intersects with the rectangle (returns only the area which overlaps)
	void IntersectWith(const RECT& other);
	//Modifies this Region object, gets the area which intersects with the rectangle (returns only the area which overlaps)
	void IntersectWith(const RECT* pOtherRect);
	//Modifies this Region object, gets the area which intersects with the other region (returns only the area which overlaps)
	void IntersectWith(const Region& otherRegion);
	//Modifies this Region object, gets the area which intersects with the other region (returns only the area which overlaps)
	//If provided HRGN is bad, becomes a null region.  HRGN parameter is not modified.
	void IntersectWith(HRGN hrgn);
	//Modifies this Region object, gets the area which intersects with the rectangle (returns only the area which overlaps, 3rd and 4th parameters are Width and Height)
	void IntersectWith(int x, int y, int w, int h);
	//Returns a new Region object, unions the region with a rectangle (region combined with new rectangle)
	Region Union(const RECT& otherRect);
	//Returns a new Region object, unions the region with a rectangle (region combined with new rectangle)
	Region Union(const RECT* pOtherRect);
	//Returns a new Region object, unions the region with another region (region combined with other region)
	Region Union(const Region& otherRegion);
	//Returns a new Region object, unions the region with another region (region combined with other region)
	//If provided HRGN is bad, returns a null region.  HRGN parameter is not modified.
	Region Union(HRGN otherRegion);
	//Returns a new Region object, unions the region with a rectangle (region combined with new rectangle, 3rd and 4th parameter are Width and Height)
	Region Union(int x, int y, int w, int h);
	//Returns a new Region object, gets the area which intersects with the rectangle (returns only the area which overlaps)
	Region Intersect(const RECT& otherRect);
	//Returns a new Region object, gets the area which intersects with the rectangle (returns only the area which overlaps)
	Region Intersect(const RECT* pOtherRect);
	//Returns a new Region object, gets the area which intersects with the other region (returns only the area which overlaps)
	Region Intersect(const Region& otherRegion);
	//Returns a new Region object, gets the area which intersects with the other region (returns only the area which overlaps)
	//If provided HRGN is bad, returns a null region
	Region Intersect(HRGN otherRegion);
	//Returns a new Region object, gets the area which intersects with the other region (returns only the area which overlaps, 3rd and 4th parameter are Width and Height)
	Region Intersect(int x, int y, int w, int h);

	//Creates a new region which is a rectangle
	Region(const RECT& otherRect);
	//Creates a new region which is a rectangle
	Region(const RECT* pOtherRect);
	//Creates a new region which is a copy of another region
	Region(const Region& other);
	//Creates a new region which is a copy of another region
	//If provided HRGN is bad, becomes a null region.  HRGN parameter is not modified, does not take ownership.
	Region(HRGN otherRegion);
	//Creates a new region which is a rectangle, 3rd and 4th parameters are Width and Height.
	Region(int x, int y, int w, int h);
	//Creates a new region which is a union of two regions (region combined with another region)
	Region(const Region& region1, const Region& region2);
	//Creates a new region which is a union of two rectangles (rectangle combined with another rectangle)
	Region(const RECT& rect1, const RECT& rect2);

	//Attaches an HRGN to a new Region object.  This region object becomes the new owner of the HRGN.
	//Value at pOtherRegion is set to NULL.  If a bad HRGN was provided, becomes an empty region.
	Region(HRGN* pRegion);

	//Assigns another region to this region
	Region& operator=(const Region& other);
	//Assigns a rectangle to this region
	Region& operator=(const RECT& rect);
	//Assigns a rectangle to this region
	Region& operator=(const RECT* pRect);
	//Swaps with another region
	void Swap(Region& other);

	//Checks if two Regions are equal
	bool operator==(const Region& other) const;
	//Checks if two Regions are not equal
	bool operator!=(const Region& other) const;
#if !CPP98
	//rvalue constructor, swaps with the other region
	Region(Region&& other) noexcept;
	//rvalue assignment, swaps with the other region
	Region& operator=(Region&& other) noexcept;
#endif
	//Returns a list of rectangles that make up the region
	vector<RECT> GetRegionRects() const;
	//Returns a set of bytes which make up the region, for use with Direct3D present calls
	vector<byte> GetRegionData() const;
	//Copies a list of rectangles that make up the region into the vector
	void GetRegionRects(vector<RECT>& rects) const;
	//Copies the bytes that make up the region into the vector
	void GetRegionData(vector<byte>& bytes) const;

	//Attaches an HRGN to this Region object.  This region object becomes the new owner of the HRGN.
	//Value at pOtherRegion is set to NULL.  If a bad HRGN was provided, becomes an empty region.
	void AttachHrgn(HRGN *pOtherRegion);
	//Removes ownership of a GDI region, after calling it, you must free the HRGN using DeleteObject.
	//Also clears the region object to null region
	HRGN DetachHrgn();
	//Returns a new HRGN which is a copy of this region. After calling this, you must free the HRGN using DeleteObject.
	//Does not affect contents of this region
	HRGN DetachHrgnCopy() const;
};
