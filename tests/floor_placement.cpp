#include "../src/floor_placement.h"
#include "../src/rest_furniture.h"
#include <fstream>
#include <iterator>
#include <map>
#include <iostream>
#include <cstdlib>
using namespace carryon::content;
#define CHECK(x) do {if(!(x)){std::cerr<<"FAIL "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(0)
static Bytes load(const char* file){std::ifstream stream(file,std::ios::binary);CHECK(stream.good());return {std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};}
int main(int argc,char** argv) {
 CHECK(argc==3);const auto profile=load(argv[1]),corpus=load(argv[2]);size_t at=0,accepted=0,total=0,seats=0,beds=0;bool desertBuffet=false,desertBed=false,cairnChair=false;
 std::map<std::string,size_t> refusals;
 while(at<corpus.size()) {
  range(corpus,at,20);Guid model{};std::memcpy(model.data(),corpus.data()+at,16);
  const auto size=u32(corpus,at+16);at+=20;const auto resource=slice(corpus,at,size);at+=size;++total;
  const auto rest=restFurnitureTemplate(resource);
  try {
   FloorBounds bounds;const auto result=makeFloorPlacement(profile,resource,model,&bounds);
   if(model==guid("6d5aeede8685234faf88ddabc54bc919"))desertBuffet=true;
   if(rest&RestSeat)++seats;if(rest&RestBed)++beds;
   if(model==guid("4b6fcb124a52de42aad04d0d767761d3")){CHECK(rest&RestBed);desertBed=true;}
   if(model==guid("e3d4d0024ff04d46a6f5077534be1d24")){CHECK(rest&RestSeat);cairnChair=true;}
   CHECK(equal(result,0x3c0,model)&&u32(result,0)==u32(profile,0));
   for(size_t a=0;a<3;++a)CHECK(std::isfinite(bounds.low[a])&&bounds.low[a]<bounds.high[a]);
   const auto colliders=array(result,0x400,56);CHECK(colliders.count);
   for(uint32_t i=0;i<colliders.count;++i) {
    const auto shapes=array(result,colliders.at+size_t(i)*56,12);
    for(uint32_t j=0;j<shapes.count;++j) {
     const auto row=shapes.at+size_t(j)*12;range(result,row+4+u32(result,row+4),u32(result,row+8));
    }
   }
   ++accepted;
  }catch(const std::exception& error){++refusals[error.what()];}
 }
 CHECK(total==6282);CHECK(accepted==5769);CHECK(desertBuffet&&desertBed&&cairnChair);
 std::cout<<"PASS native rest furniture geometry: "<<seats<<" seat templates, "<<beds<<" bed templates.\n";
 for(const auto& [reason,count]:refusals)std::cout<<"REFUSED "<<count<<": "<<reason<<'\n';
 CHECK(!refusals.count("unknown native placement collider shape"));
 CHECK(refusals.size()==3&&refusals["native template disables persistence"]==170&&
  refusals["no placement collider geometry"]==240&&refusals["no placement colliders"]==103);
 std::cout<<"PASS: "<<accepted<<" / "<<total<<" native Prop resources generate bounded floor placement descriptors.\n";
}
