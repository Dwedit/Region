#include "Region.h"
#include <algorithm>
#include <assert.h>
using std::min;
using std::max;

#if REGION_USE_GLOBAL_TEMP_REGION
#if REGION_USE_VALID_FLAG
//Initializes fields of Region class (All constructors must call this)
#define Region_Initialize() {boundingBox={}; regionType = NULLREGION; hrgn = NULL; hrgnValid = false;}
#else
//Initializes fields of Region class (All constructors must call this)
#define Region_Initialize() {boundingBox={}; regionType = NULLREGION; hrgn = NULL; }
#endif
#else
#if REGION_USE_VALID_FLAG
//Initializes fields of Region class (All constructors must call this)
#define Region_Initialize() {boundingBox={}; regionType = NULLREGION; hrgn = NULL; hrgnTemp = NULL; hrgnValid = false;}
#else
//Initializes fields of Region class (All constructors must call this)
#define Region_Initialize() {boundingBox={}; regionType = NULLREGION; hrgn = NULL; hrgnTemp = NULL; }
#endif
#endif


#if REGION_USE_GLOBAL_TEMP_REGION
//Note that this variable is probably not going to be used by any thread other than the main thread.
//But just in case it is, it must be thread-local.

//Thread Local storage index for the global variable (temporary region)
DWORD _hrgnTls = TLS_OUT_OF_INDEXES;

//Sets the Temp Region (a global variable) to a rectangle so Win32 Region API functions can be used with the rectangle
HRGN SetTempRegion(const RECT& rect)
{
	//Use a TLS slot for the global variable
	if (_hrgnTls == TLS_OUT_OF_INDEXES)
	{
		_hrgnTls = TlsAlloc();
	}
	HRGN hrgnTemp = (HRGN)TlsGetValue(_hrgnTls);
	if (hrgnTemp == NULL)
	{
		//If temp region doesn't exist, create it
		hrgnTemp = CreateRectRgnIndirect(&rect);
		TlsSetValue(_hrgnTls, (LPVOID)hrgnTemp);
	}
	else
	{
		//Assign the rectangle to the temp region
		SetRectRgn(hrgnTemp, rect.left, rect.top, rect.right, rect.bottom);
	}
	return hrgnTemp;
}

//Frees the temp region
void FreeTempRegion()
{
	HRGN hrgnTemp = (HRGN)TlsGetValue(_hrgnTls);
	if (hrgnTemp != NULL)
	{
		DeleteObject(hrgnTemp);
		hrgnTemp = NULL;
		TlsSetValue(_hrgnTls, (LPVOID)hrgnTemp);
	}
}
#endif

Region::Region()
{
	Region_Initialize();
}

Region::~Region()
{
	if (this->hrgn != NULL)
	{
		DeleteObject(this->hrgn);
	}
#if !REGION_USE_GLOBAL_TEMP_REGION
	if (this->hrgnTemp != NULL)
	{
		DeleteObject(this->hrgnTemp);
	}
#endif
}

DWORD Region::GetRegionType() const
{
	return this->regionType;
}

const RECT& Region::GetBoundingBox() const
{
	return boundingBox;
}
void Region::GetBoundingBox(RECT* pRect) const
{
	*pRect = boundingBox;
}
void Region::GetBoundingBox(RECT& rect) const
{
	rect = boundingBox;
}
void Region::GetBoundingBox(int& x, int& y, int& w, int& h) const
{
	x = boundingBox.left;
	y = boundingBox.top;
	w = boundingBox.right - boundingBox.left;
	h = boundingBox.bottom - boundingBox.top;
}

HRGN Region::GetHrgn()
{
	if (this->hrgn == NULL)
	{
		//No HRGN?  Create it
		this->hrgn = CreateRectRgnIndirect(&this->boundingBox);
		SetHrgnValid();
	}
	else if (!HrgnIsValid())
	{
		//Set our HRGN to the current bounding box
		if (this->regionType == SIMPLEREGION)
		{
			SetRectRgn(this->hrgn, boundingBox.left, boundingBox.top, boundingBox.right, boundingBox.bottom);
			SetHrgnValid();
		}
		else if (this->regionType == NULLREGION)
		{
			SetRectRgn(this->hrgn, 0, 0, 0, 0);
			SetHrgnValid();
		}
	}
	return this->hrgn;
}
void Region::Clear()
{
	boundingBox = {};
	this->regionType = NULLREGION;
	SetHrgnInvalid();
}
void Region::SetHrgnInvalid()
{
#if REGION_USE_VALID_FLAG
	this->hrgnValid = false;
#else
	if (hrgn != NULL)
	{
		DeleteObject(hrgn);
		hrgn = NULL;
	}
#endif
}
void Region::SetHrgnValid()
{
#if REGION_USE_VALID_FLAG
	this->hrgnValid = true;
#endif
}
bool Region::HrgnIsValid() const
{
#if REGION_USE_VALID_FLAG
	return this->hrgnValid;
#else
	return hrgn != NULL;
#endif
}

#if !REGION_USE_GLOBAL_TEMP_REGION
void Region::SetTempRegion(const RECT& rect)
{
	if (hrgnTemp == NULL)
	{
		hrgnTemp = CreateRectRgnIndirect(&rect);
	}
	else
	{
		SetRectRgn(hrgnTemp, rect.left, rect.top, rect.right, rect.bottom);
	}
}
#endif

void Region::BecomeRectangle(int left, int top, int right, int bottom)
{
	boundingBox.left = left;
	boundingBox.top = top;
	boundingBox.right = right;
	boundingBox.bottom = bottom;
	this->regionType = SIMPLEREGION;
	SetHrgnInvalid();
}
void Region::BecomeRectangle(const RECT& rect)
{
	BecomeRectangle(rect.left, rect.top, rect.right, rect.bottom);
}
void Region::UnionBoundingBox(const RECT& rect)
{
	boundingBox.left = min(boundingBox.left, rect.left);
	boundingBox.top = min(boundingBox.top, rect.top);
	boundingBox.right = max(boundingBox.right, rect.right);
	boundingBox.bottom = max(boundingBox.bottom, rect.bottom);
}
void Region::IntersectBoundingbox(const RECT& rect)
{
	boundingBox.left = max(boundingBox.left, rect.left);
	boundingBox.top = max(boundingBox.top, rect.top);
	boundingBox.right = min(boundingBox.right, rect.right);
	boundingBox.bottom = min(boundingBox.bottom, rect.bottom);
}

void Region::UnionRectWithRect(const RECT& other)
{
	//only called when we know that this region is currently a rectangle
	const RECT& me = this->boundingBox;

	//Do we cover up the other?
	if (RectCoversUpOther(me, other))
	{
		//no assignments needed, can preserve the HRGN
		return;
	}

	//Does the other cover up us?
	if (RectCoversUpOther(other, me))
	{
		BecomeRectangle(other);
		return;
	}

	//Check if we are aligned vertically and overlapping
	if (me.top == other.top && me.bottom == other.bottom)
	{
		if (me.left > other.right || other.left > me.right)
		{
			//reject
		}
		else
		{
			BecomeRectangle(min(me.left, other.left), me.top, max(me.right, other.right), me.bottom);
			return;
		}
	}
	//check if we are aligned horizontally and overlapping
	else if (me.left == other.left && me.right == other.right)
	{
		if (me.top > other.bottom || other.top > me.bottom)
		{
			//reject
		}
		else
		{
			BecomeRectangle(me.left, min(me.top, other.top), me.right, max(me.bottom, other.bottom));
			return;
		}
	}
	UnionRectWithRectBecomeComplex(other);
}

void Region::UnionRectWithRectBecomeComplex(const RECT& other)
{
	GetHrgn();
	HRGN hrgnTemp = SetTempRegion(other);
	this->regionType = CombineRgn(this->hrgn, this->hrgn, hrgnTemp, RGN_OR);
	SetHrgnValid();
	UnionBoundingBox(other);
}

void Region::UnionWith(const RECT& other)
{
	if (regionType == NULLREGION)
	{
		//We are a null region, become the other rectangle
		BecomeRectangle(other);
	}
	else if (regionType == SIMPLEREGION)
	{
		//We are a rectangle
		UnionRectWithRect(other);
	}
	else if (regionType == COMPLEXREGION)
	{
		//Does the other cover up us?
		if (RectCoversUpOther(other, this->boundingBox))
		{
			BecomeRectangle(other);
			return;
		}
		UnionRectWithRectBecomeComplex(other);
	}
}
void Region::UnionWith(const RECT* pRect)
{
	UnionWith(*pRect);
}
/*static*/ bool Region::RectCoversUpOther(const RECT& rect1, const RECT& rect2)
{
	return rect2.left >= rect1.left && rect2.right <= rect1.right &&
		rect2.top >= rect1.top && rect2.bottom <= rect1.bottom;
}
/*static*/ bool Region::RectOverlaps(const RECT& rect1, const RECT& rect2)
{
	return !(rect1.right <= rect2.left || rect2.right <= rect1.left || rect1.bottom <= rect2.top || rect2.bottom <= rect1.top);
}
/*static*/ bool Region::RectEquals(const RECT& rect1, const RECT& rect2)
{
	return rect1.left == rect2.left && rect1.top == rect2.top && rect1.right == rect2.right && rect1.bottom == rect2.bottom;
}

void Region::BecomeRegion(const Region& region)
{
	if (region.regionType == SIMPLEREGION)
	{
		BecomeRectangle(region.boundingBox);
	}
	else if (region.regionType == COMPLEXREGION)
	{
		Clear();
		GetHrgn();
		this->regionType = CombineRgn(this->hrgn, this->hrgn, region.hrgn, RGN_OR);
		this->boundingBox = region.boundingBox;
	}
	else if (region.regionType == NULLREGION)
	{
		Clear();
	}
}

void Region::UnionWith(const Region& region)
{
	if (region.regionType == SIMPLEREGION)
	{
		UnionWith(region.boundingBox);
	}
	else if (region.regionType == COMPLEXREGION)
	{
		//if we cover up the other region, nothing to do for union operation
		if (this->regionType == SIMPLEREGION && RectCoversUpOther(boundingBox, region.boundingBox))
		{
			return;
		}
		bool haveBoundingBox = this->regionType != NULLREGION;
		GetHrgn();
		this->regionType = CombineRgn(this->hrgn, this->hrgn, region.hrgn, RGN_OR);
		if (haveBoundingBox)
		{
			UnionBoundingBox(region.boundingBox);
		}
		else
		{
			this->boundingBox = region.boundingBox;
		}
	}
}
void Region::UnionWith(HRGN hrgn)
{
	GetHrgn();
	this->regionType = CombineRgn(this->hrgn, this->hrgn, hrgn, RGN_OR);
	//Check if provided HRGN was invalid
	if (this->regionType == 0)
	{
		Clear();
		return;
	}
	this->regionType = GetRgnBox(this->hrgn, &this->boundingBox);
}
void Region::UnionWith(int x, int y, int w, int h)
{
	RECT rect{ x, y, x + w, y + h };
	UnionWith(rect);
}
void Region::IntersectWith(const RECT& other)
{
	//are we a rectangle?
	if (this->regionType == SIMPLEREGION)
	{
		const RECT& me = boundingBox;
		//If other covers up us (and we are saving a valid HRGN) no assignments needed (preserves HRGN too)
		if (HrgnIsValid() && RectCoversUpOther(other, me))
		{
			return;
		}
		IntersectBoundingbox(other);
		SetHrgnInvalid();
		int w = me.right - me.left;
		int h = me.bottom - me.top;
		if (w <= 0 || h <= 0) Clear();
	}
	//are we complex?
	else if (this->regionType == COMPLEXREGION)
	{
		if (RectCoversUpOther(other, boundingBox))
		{
			return;
		}
		GetHrgn();
		HRGN hrgnTemp = SetTempRegion(other);
		this->regionType = CombineRgn(hrgn, hrgn, hrgnTemp, RGN_AND);
		this->regionType = GetRgnBox(hrgn, &boundingBox);
	}
	//are we empty?
	else if (this->regionType == NULLREGION)
	{
		//already null, intersection will be nothing
	}
}
void Region::IntersectWith(const RECT* pRect)
{
	IntersectWith(*pRect);
}
void Region::IntersectWith(const Region& otherRegion)
{
	if (otherRegion.regionType == SIMPLEREGION)
	{
		IntersectWith(otherRegion.boundingBox);
	}
	else if (otherRegion.regionType == COMPLEXREGION)
	{
		if (this->regionType == NULLREGION)
		{
			return;
		}
		if (!RectOverlaps(this->boundingBox, otherRegion.boundingBox))
		{
			Clear();
			return;
		}
		GetHrgn();
		this->regionType = CombineRgn(hrgn, hrgn, otherRegion.hrgn, RGN_AND);
		this->regionType = GetRgnBox(hrgn, &boundingBox);
	}
	else if (otherRegion.regionType == NULLREGION)
	{
		Clear();
	}
}
void Region::IntersectWith(HRGN other)
{
	if (this->regionType == NULLREGION)
	{
		return;
	}
	GetHrgn();
	this->regionType = CombineRgn(hrgn, hrgn, other, RGN_AND);
	//check if provided HRGN was invalid
	if (this->regionType == 0)
	{
		Clear();
		return;
	}
	this->regionType = GetRgnBox(hrgn, &boundingBox);
	SetHrgnValid();
}
void Region::IntersectWith(int x, int y, int w, int h)
{
	RECT rect{ x, y, x + w, y + h };
	IntersectWith(rect);
}

Region::Region(const Region& other)
{
	Region_Initialize();
	BecomeRegion(other);
}
Region& Region::operator=(const Region& other)
{
	BecomeRegion(other);
	return *this;
}
Region& Region::operator=(const RECT& rect)
{
	BecomeRectangle(rect);
	return *this;
}
Region& Region::operator=(const RECT* pRect)
{
	BecomeRectangle(*pRect);
	return *this;
}
void Region::Swap(Region& other)
{
	std::swap(this->boundingBox, other.boundingBox);
	std::swap(this->regionType, other.regionType);
	std::swap(this->hrgn, other.hrgn);
#if !REGION_USE_GLOBAL_TEMP_REGION
	std::swap(this->hrgnTemp, other.hrgnTemp);
#endif
#if REGION_USE_VALID_FLAG
	std::swap(this->hrgnValid, other.hrgnValid);
#endif
}

Region::Region(const RECT& other)
{
	Region_Initialize();
	BecomeRectangle(other);
}
Region::Region(const RECT* pOtherRect)
{
	Region_Initialize();
	BecomeRectangle(*pOtherRect);
}
Region::Region(HRGN otherRegion)
{
	Region_Initialize();
	UnionWith(otherRegion);
}
Region::Region(int x, int y, int w, int h)
{
	Region_Initialize();
	BecomeRectangle(x, y, x + w, y + h);
}
Region::Region(const Region& region1, const Region& region2)
{
	Region_Initialize();
	BecomeRegion(region1);
	UnionWith(region2);
}
Region::Region(const RECT& rect1, const RECT& rect2)
{
	Region_Initialize();
	BecomeRectangle(rect1);
	UnionRectWithRect(rect2);
}
Region::Region(HRGN *pRegion)
{
	Region_Initialize();
	AttachHrgn(pRegion);
}


#if !_NO_RVALUE_REFERENCE
Region::Region(Region&& other) noexcept
{
	Region_Initialize();
	Swap(other);
}
Region& Region::operator=(Region&& other) noexcept
{
	Swap(other);
	return *this;
}
#endif

Region Region::Union(const RECT& other)
{
	Region newRegion(other);
	if (regionType == SIMPLEREGION)
	{
		newRegion.UnionWith(this->boundingBox);
	}
	else
	{
		newRegion.UnionWith(*this);
	}
	return newRegion;
}
Region Region::Union(const RECT* pOtherRect)
{
	return Union(*pOtherRect);
}
Region Region::Union(const Region& otherRegion)
{
	if (regionType == SIMPLEREGION && otherRegion.regionType == SIMPLEREGION)
	{
		Region newRegion(otherRegion.boundingBox);
		newRegion.UnionRectWithRect(this->boundingBox);
		return newRegion;
	}
	else
	{
		Region newRegion(otherRegion);
		newRegion.UnionWith(*this);
		return newRegion;
	}
}
Region Region::Union(HRGN otherRegion)
{
	Region newRegion(otherRegion);
	newRegion.UnionWith(*this);
	return newRegion;
}
Region Region::Union(int x, int y, int w, int h)
{
	Region newRegion(x, y, w, h);
	if (regionType == SIMPLEREGION)
	{
		newRegion.UnionRectWithRect(this->boundingBox);
	}
	else
	{
		newRegion.UnionWith(*this);
	}
	return newRegion;
}

Region Region::Intersect(const RECT& other)
{
	Region newRegion(other);
	newRegion.IntersectWith(*this);
	return newRegion;
}
Region Region::Intersect(const RECT* pOtherRect)
{
	Region newRegion(pOtherRect);
	newRegion.IntersectWith(*this);
	return newRegion;
}
Region Region::Intersect(const Region& otherRegion)
{
	Region newRegion(otherRegion);
	newRegion.IntersectWith(*this);
	return newRegion;
}
Region Region::Intersect(HRGN otherRegion)
{
	Region newRegion(otherRegion);
	newRegion.IntersectWith(*this);
	return newRegion;
}
Region Region::Intersect(int x, int y, int w, int h)
{
	Region newRegion(x, y, w, h);
	newRegion.IntersectWith(*this);
	return newRegion;
}

bool Region::operator==(const Region& other) const
{
	if (this->regionType != other.regionType)
	{
		return false;
	}
	if (this->regionType == NULLREGION)
	{
		return true;
	}
	if (this->regionType == SIMPLEREGION)
	{
		return RectEquals(this->boundingBox, other.boundingBox);
	}
	return EqualRgn(this->hrgn, other.hrgn);
}
bool Region::operator!=(const Region& other) const
{
	return !(*this == other);
}

vector<RECT> Region::GetRegionRects() const
{
	vector<RECT> result;
	GetRegionRects(result);
	return result;
}
vector<byte> Region::GetRegionData() const
{
	vector<byte> result;
	GetRegionData(result);
	return result;
}
void Region::GetRegionRects(vector<RECT>& rects) const
{
	rects.clear();
	if (this->regionType == SIMPLEREGION)
	{
		rects.push_back(this->boundingBox);
	}
	else if (this->regionType == NULLREGION)
	{
		//no rects to add
	}
	else if (this->regionType == COMPLEXREGION)
	{
		vector<byte> regionData;
		GetRegionData(regionData);
		if (regionData.size() < sizeof(RGNDATAHEADER) + sizeof(RECT))
		{
			//error condition, only happens if Windows itself is broken or the HRGN is somehow in a bad state
			return;
		}
		RGNDATAHEADER& header = *((RGNDATAHEADER*)(&regionData[0]));
		if (!(header.nCount > 0 && header.dwSize == sizeof(RGNDATAHEADER) && header.nCount * sizeof(RECT) == header.nRgnSize))
		{
			//error condition, only happens if Windows itself is broken or the HRGN is somehow in a bad state
			return;
		}
		RECT* pRects = (RECT*)(&regionData[0] + sizeof(RGNDATAHEADER));
		rects.resize(header.nCount);
		memcpy(&rects[0], &pRects[0], sizeof(RECT) * rects.size());
	}
}
void Region::GetRegionData(vector<byte>& bytes) const
{
	if (this->regionType == NULLREGION)
	{
		bytes.resize(sizeof(RGNDATAHEADER));
		RGNDATAHEADER& header = *((RGNDATAHEADER*)&bytes[0]);
		header.dwSize = sizeof(RGNDATAHEADER);
		header.iType = RDH_RECTANGLES;
		header.nCount = 0;
		header.nRgnSize = 0;
		header.rcBound = {};
	}
	else if (this->regionType == SIMPLEREGION)
	{
		bytes.resize(sizeof(RGNDATAHEADER) + sizeof(RECT));
		RGNDATAHEADER& header = *((RGNDATAHEADER*)&bytes[0]);
		RECT* rects = (RECT*)(&bytes[0] + sizeof(RGNDATAHEADER));
		header.dwSize = sizeof(RGNDATAHEADER);
		header.iType = RDH_RECTANGLES;
		header.nCount = 1;
		header.nRgnSize = sizeof(RECT);
		header.rcBound = this->boundingBox;
		rects[0] = this->boundingBox;
	}
	else if (this->regionType == COMPLEXREGION)
	{
		DWORD size = ::GetRegionData(hrgn, 0, NULL);
		bytes.resize(size);
		if (size > 0)
		{
			DWORD success = ::GetRegionData(hrgn, (DWORD)bytes.size(), (LPRGNDATA)&bytes[0]);
			if (!success) bytes.resize(0);
		}
	}
}

void Region::AttachHrgn(HRGN *pOtherRegion)
{
	if (pOtherRegion == NULL) return;
	if (this->hrgn != NULL)
	{
		DeleteObject(this->hrgn);
		this->hrgn = NULL;
	}
	this->hrgn = *pOtherRegion;
	this->regionType = GetRgnBox(this->hrgn, &this->boundingBox);
	if (this->regionType == 0)
	{
		this->hrgn = NULL;
		Clear();
	}
	else
	{
		*pOtherRegion = NULL;
		SetHrgnValid();
	}
}
HRGN Region::DetachHrgn()
{
	GetHrgn();
	HRGN returnValue = this->hrgn;
	this->hrgn = NULL;
	Clear();
	return returnValue;
}
HRGN Region::DetachHrgnCopy() const
{
	Region R2(*this);
	return R2.DetachHrgn();
}
