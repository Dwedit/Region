# Region
This is a wrapper for Win32 GDI regions that minimizes calls to Win32 Region API functions.  The Region class will manage Simple regions (rectangles) or Null regions until they need to become complex regions (multiple rectangles), then Win32 API functions will be used.

This is intended to use when the Region will most likely be just a single rectangle.

Supports Union and Intersection.
