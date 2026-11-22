#pragma once
#include "transport_move.h"
namespace carryon::transport {
// SubEntitySpawnInfo offsets are local to a uniformly scaled root. World-space
// offsets/conditional or attached children are rejected by the template parser.
inline bool restChildPlacement(const Placement& root,const std::array<float,3>& local,
                               const std::array<float,4>& orientation,Placement& out) noexcept {
 if(!root.movable())return false;
 double q[4]{},r[4]{},qn=0,rn=0;
 for(size_t i=0;i<4;++i){q[i]=root.rotation[i];r[i]=orientation[i];qn+=q[i]*q[i];rn+=r[i]*r[i];}
 if(!std::isfinite(rn)||std::abs(rn-1)>0.001)return false;
 for(size_t i=0;i<4;++i){q[i]/=std::sqrt(qn);r[i]/=std::sqrt(rn);}
 const double x=q[0],y=q[1],z=q[2],w=q[3];
 const double m[3][3]={{1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)},
  {2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)},
  {2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)}};
 Placement result{};result.scale=root.scale;
 for(size_t a=0;a<3;++a){double delta=0;for(size_t b=0;b<3;++b)delta+=m[a][b]*local[b]*root.scale[0];
  const double fixed=double(root.position[a])+delta*4294967296.0;
  if(!std::isfinite(fixed)||fixed<=double(INT64_MIN)||fixed>=double(INT64_MAX))return false;
  result.position[a]=static_cast<int64_t>(std::round(fixed));
 }
 result.rotation={float(w*r[0]+x*r[3]+y*r[2]-z*r[1]),float(w*r[1]-x*r[2]+y*r[3]+z*r[0]),
  float(w*r[2]+x*r[1]-y*r[0]+z*r[3]),float(w*r[3]-x*r[0]-y*r[1]-z*r[2])};
 if(!result.movable())return false;out=result;return true;
}
inline bool restChildMatches(const Placement& actual,const Placement& expected) noexcept {
 if(!actual.movable()||!expected.movable()||actual.scale!=expected.scale)return false;
 for(size_t i=0;i<3;++i)if(std::abs(double(actual.position[i])-double(expected.position[i]))>8589935.0)return false; // 2 mm
 double dot=0,a=0,b=0;
 for(size_t i=0;i<4;++i){dot+=double(actual.rotation[i])*expected.rotation[i];a+=double(actual.rotation[i])*actual.rotation[i];b+=double(expected.rotation[i])*expected.rotation[i];}
 return std::abs(dot)/std::sqrt(a*b)>0.9999976201773518; // native quantization, 0.25 degrees
}
}
