#include "Region.h"
#include "RectEquals.h"
#include <assert.h>

bool RegionDataHeaderOkay(const vector<byte> &bytes, int rectCount, const RECT &boundingBox)
{
	if (bytes.size() < sizeof(RGNDATAHEADER)) return false;
	const RGNDATAHEADER* header = (RGNDATAHEADER*)(&bytes[0]);
	if (header->dwSize != sizeof(RGNDATAHEADER)) return false;
	if (header->iType != RDH_RECTANGLES) return false;
	if (header->nCount != rectCount) return false;
	if (header->nRgnSize != rectCount * sizeof(RECT)) return false;
	if (header->rcBound != boundingBox) return false;
	return true;
}

int main()
{
	int regionType;
	RECT rect;

	//basic construction
	RECT rectA = { 0, 0, 50, 50 };
	RECT rectB = { 50, 0, 100, 50 };
	RECT rectC = { 0, 50, 50, 100 };
	RECT rectD = { 50, 50, 100, 100 };
	RECT rectAB = { 0, 0, 100, 50 };
	RECT rectCD = { 0, 50, 100, 100};
	RECT rectABCD = { 0, 0, 100, 100 };
	RECT rectEmpty = { 0,0,0,0 };

	Region A(rectA);
	Region B(&rectB);
	Region C;
	C.UnionWith(rectC);
	Region D = rectD;
	
	//equality operator on rectangles
	bool aEquals = A == rectA;
	bool bEquals = B == rectB;
	bool cEquals = C == rectC;
	bool dEquals = D == rectD;

	bool aEqualsB = A == B;
	bool cNotEqualsD = C != D;

	assert(aEquals && bEquals && cEquals && dEquals);
	assert(!aEqualsB && cNotEqualsD);

	regionType = A.GetRegionType();
	assert(regionType == SIMPLEREGION);
	rect = A.GetBoundingBox();
	assert(rect == rectA);

	Region AD(A, D);
	Region AD2(A, D);
	//equality operator on complex regions
	bool adEquals = AD == AD2;
	bool adNotEquals = AD != AD2;
	assert(adEquals && !adNotEquals);

	regionType = AD.GetRegionType();
	assert(regionType == COMPLEXREGION);
	AD.GetBoundingBox(rect);
	assert(rect == rectABCD);

	//equality operator on empty regions
	Region empty1;
	Region empty2;
	bool emptyEquals = empty1 == empty2;
	bool emptyNotEquals = empty1 != empty2;

	regionType = empty1.GetRegionType();
	assert(regionType == NULLREGION);
	empty1.GetBoundingBox(&rect);
	assert(rect == rectEmpty);

	bool mismatch1Equals = empty1 == AD;
	bool mismatch2Equals = empty2 == A;
	assert(!mismatch1Equals && !mismatch2Equals);

	//getting two rects that make up region AD
	vector<RECT> rects;
	vector<byte> bytes;
	rects = AD.GetRegionRects();
	assert(rects.size() == 2);
	assert(rects[0] == rectA && rects[1] == rectD);
	bytes = AD.GetRegionData();
	assert(bytes.size() == sizeof(RGNDATAHEADER) + sizeof(RECT) * 2);
	assert(RegionDataHeaderOkay(bytes, (int)rects.size(), AD.GetBoundingBox()));

	//simple region
	rects = A.GetRegionRects();
	assert(rects.size() == 1);
	assert(rects[0] == rectA);
	bytes = A.GetRegionData();
	assert(bytes.size() == sizeof(RGNDATAHEADER) + sizeof(RECT));
	assert(RegionDataHeaderOkay(bytes, (int)rects.size(), A.GetBoundingBox()));

	//empty region
	rects = empty1.GetRegionRects();
	assert(rects.size() == 0);
	bytes = empty1.GetRegionData();
	assert(bytes.size() == sizeof(RGNDATAHEADER));
	assert(RegionDataHeaderOkay(bytes, (int)rects.size(), empty1.GetBoundingBox()));


	
	//for coverage
	FreeTempRegion();

	//testing GetBoundingBox with 4 ints
	int x, y, w, h;
	C.GetBoundingBox(x, y, w, h);
	assert(x == 0 && y == 50 && w == 50 && h == 50);

	//testing UnionWith and Clear
	Region R;
	//building in ADBC order
	R.UnionWith(A);
	assert(R.GetRegionType() == SIMPLEREGION);
	assert(R.GetBoundingBox() == rectA);
	R.UnionWith(D);
	assert(R.GetRegionType() == COMPLEXREGION);
	assert(R.GetBoundingBox() == rectABCD);
	R.UnionWith(B);
	assert(R.GetRegionType() == COMPLEXREGION);
	assert(R.GetBoundingBox() == rectABCD);
	R.UnionWith(C);
	assert(R.GetRegionType() == SIMPLEREGION);
	assert(R.GetBoundingBox() == rectABCD);

	//Clearing region and trying again
	//building in AB, CD order
	R.Clear();
	R.UnionWith(A);
	assert(R.GetRegionType() == SIMPLEREGION);
	assert(R.GetBoundingBox() == rectA);
	R.UnionWith(B);
	assert(R.GetRegionType() == SIMPLEREGION);
	assert(R.GetBoundingBox() == rectAB);

	Region R2;
	R2.UnionWith(C);
	assert(R2.GetRegionType() == SIMPLEREGION);
	assert(R2.GetBoundingBox() == rectC);
	R2.UnionWith(D);
	assert(R2.GetRegionType() == SIMPLEREGION);
	assert(R2.GetBoundingBox() == rectCD);
	R.UnionWith(R2);
	assert(R.GetRegionType() == SIMPLEREGION);
	assert(R.GetBoundingBox() == rectABCD);

	//Clearing region and trying again
	R.Clear();
	//make a complex region, after having made a complex region before, will trigger a hrgn existing but not being valid
	R.UnionWith(A);
	R.UnionWith(D);
	R.UnionWith(C);
	R.UnionWith(B);
	R.Clear();
	//trigger GetHrgn for existing but not valid NULLREGION
	HRGN dummyHrgn = R.DetachHrgn();
	DeleteObject(dummyHrgn); dummyHrgn = NULL;

	//AttachHrgn
	R.Clear();
	R.UnionWith(A);
	R.UnionWith(D);
	HRGN hrgnAD = R.DetachHrgn();
	R.AttachHrgn(&hrgnAD);
	hrgnAD = R.DetachHrgn();
	//trigger AttachHrgn's DeleteObject call
	R.Clear();
	R.UnionWith(A);
	R.UnionWith(D);
	R.AttachHrgn(&hrgnAD);

	//attach an invalid HRGN for testing
	hrgnAD = (HRGN)0x12345678;
	R.AttachHrgn(&hrgnAD);
	hrgnAD = (HRGN)0x12345678;
	R.UnionWith(hrgnAD);
	hrgnAD = (HRGN)0x12345678;
	R.UnionWith(A);
	R.IntersectWith(hrgnAD);

	//Test conditions inside of UnionRectWithRect
	R.Clear();
	R.UnionWith(0, 0, 50, 50);
	//Test "we cover up other"
	R.UnionWith(1, 1, 48, 48);
	//Test "other covers up us"
	R.UnionWith(-1, -1, 52, 52);
	//test disjoint horizontally
	R.Clear();
	R.UnionWith(0, 0, 50, 50);
	R.UnionWith(51, 0, 50, 50);
	R.Clear();
	R.UnionWith(0, 0, 50, 50);
	R.UnionWith(0, 51, 50, 50);

	//test "we are complex, other covers us up"
	R.Clear();
	R.UnionWith(A);
	R.UnionWith(D);
	R.UnionWith(rectABCD);

	//for coverage
	R.Clear();
	R.UnionWith(&rectA);

	//Test unioning complex regions
	R.Clear();
	R.UnionWith(A);
	R.UnionWith(D);
	R2.Clear();
	R2.UnionWith(C);
	R2.UnionWith(B);
	R.UnionWith(R2);
	//trigger "we are simple and cover up other region"
	R.UnionWith(R2);
	R.Clear();
	//trigger Empty union Complex (copies bounding box instead of combining it)
	R.UnionWith(R2);

	//test Union with HRGN
	HRGN hrgnR2 = R2.DetachHrgn();
	R.Clear();
	R.UnionWith(hrgnR2);
	DeleteObject(hrgnR2); hrgnR2 = NULL;

	//test Become Complex Region
	R2.Clear();
	R2.UnionWith(C);
	R2.UnionWith(B);
	R.Clear();
	R = R2;

	//test become NULL region
	R = empty1;

	//test various constructors and operator=
	R.Clear();
	R.UnionWith(A);
	R.UnionWith(D);

	Region R3(R);
	R3 = R;
	R3 = rectA;
	R3 = &rectA;
	R3 = R;
	Region R4(0, 0, 50, 50);
	HRGN hrgnR3 = R3.DetachHrgn();
	Region R5(hrgnR3);
	DeleteObject(hrgnR3); hrgnR3 = NULL;
	R3 = R;
	hrgnR3 = R3.DetachHrgn();
	Region R6(&hrgnR3);
	Region AB2(rectA, rectB);

	//R value assignment and constructor
	R3 = A.Union(B);
	Region R7(A.Union(B));
	R3 = A.Union(rectB);
	R3 = R;
	R3 = R3.Union(rectB);
	R3 = R3.Union(&rectB);

	//union of complex regions again
	R.Clear();
	R.UnionWith(A);
	R.UnionWith(D);
	R2.Clear();
	R2.UnionWith(B);
	R2.UnionWith(C);
	R3 = R.Union(R2);
	R3 = R3.Union(0, 0, 100, 100);
	R3 = R;
	R3 = R3.Union(0, 0, 100, 100);
	R3 = R;
	hrgnR3 = R3.DetachHrgn();
	R3 = R;
	R3 = R3.Union(hrgnR3);
	DeleteObject(hrgnR3); hrgnR3 = NULL;

	R3 = R;
	R3.IntersectWith(A);
	R3 = R;
	R3.IntersectWith(B);
	R3 = R;
	R3.IntersectWith(C);
	R3 = R;
	R3.IntersectWith(D);
	R3 = R;
	R3.IntersectWith(0, 0, 100, 100);
	R3.Clear();
	R3.UnionWith(A);
	R3.UnionWith(D);
	R3.IntersectWith(A);
	R3.IntersectWith(0, 0, 50, 50);
	R3.IntersectWith(-1, -1, 52, 52);
	R3.IntersectWith(1, 1, 48, 48);
	R3.IntersectWith(0, 0, 0, 0);
	R3.UnionWith(A);
	R3.IntersectWith(empty1);
	R3.UnionWith(A);
	hrgnR3 = R3.DetachHrgn();
	R3.Clear();
	R3.UnionWith(A);
	R3.IntersectWith(hrgnR3);
	DeleteObject(hrgnR3); hrgnR3 = NULL;
	R3.Clear();
	R3.UnionWith(A);
	hrgnR3 = R3.DetachHrgn();
	R3.Clear();
	R3.IntersectWith(hrgnR3);
	DeleteObject(hrgnR3); hrgnR3 = NULL;
	R3.Clear();
	R3.IntersectWith(0, 0, 0, 0);
	R3.IntersectWith(&rectA);

	R3.Clear();
	R2.Clear();
	R.Clear();
	R2.UnionWith(B);
	R2.UnionWith(C);
	R.UnionWith(A);
	R.UnionWith(D);
	R3 = R.Intersect(R2);
	R3 = R.Intersect(rectA);
	R3 = R.Intersect(&rectA);
	hrgnR3 = R3.DetachHrgn();
	R3 = R.Intersect(hrgnR3);
	DeleteObject(hrgnR3); hrgnR3 = NULL;
	R3 = R.Intersect(0, 0, 50, 50);
	R3.Clear();
	R3.IntersectWith(A);
	R3 = R;
	R3.IntersectWith(A);
	empty1.IntersectWith(R);
	R3.Clear();
	R3.UnionWith(-1, -1, 1, 1);
	R3 = R.Intersect(R3);
	int dummy = 0;

	hrgnR3 = R.DetachHrgnCopy();
	DeleteObject(hrgnR3); hrgnR3 = NULL;
}
