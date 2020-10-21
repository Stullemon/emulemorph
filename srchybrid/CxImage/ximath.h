#if !defined(__ximath_h)
#define __ximath_h

#include "ximadef.h"

//***bd*** simple floating point CxPoint2
class DLL_EXP CxPoint2
{
public:
  CxPoint2();
  CxPoint2(float const x_, float const y_);
  CxPoint2(CxPoint2 const &p);
  CxPoint2& operator=(const CxPoint2& p);

  float Distance(const CxPoint2& p2) const;
  float Distance(const float x_, const float y_) const;

  float x,y;
};

//and simple rectangle
class DLL_EXP CxRect2
{
public:
  CxRect2();
  CxRect2(float const x1_, float const y1_, float const x2_, float const y2_);
  CxRect2(CxPoint2 const &bl, CxPoint2 const &tr);
  CxRect2(CxRect2 const &p);
  CxRect2& operator=(const CxRect2& r);

  float Surface() const;
  CxRect2 CrossSection(CxRect2 const &r2) const;
  CxPoint2 Center() const;
  float Width() const;
  float Height() const;

  CxPoint2 botLeft;
  CxPoint2 topRight;
};

#endif
