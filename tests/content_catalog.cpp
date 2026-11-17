#include "../src/content_catalog.h"
#include <fstream>
#include <iostream>
#include <iterator>
using namespace carryon::content;
#define CHECK(x) do {if(!(x))throw std::runtime_error(#x);}while(0)
Bytes read(const char* path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error(path);return Bytes(std::istreambuf_iterator<char>(f),{});}
template<class F> void rejects(F fn){bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}CHECK(rejected);}
int main(int argc,char** argv) {
 try {
    CHECK(argc==4);
    Bytes cap(16*1024*1024-1);const auto sizeBeforeAppend=cap.size();
    rejects([&]{append(cap,Bytes{1});});CHECK(cap.size()==sizeBeforeAppend);
    CHECK(append(cap,{})==16*1024*1024&&cap.size()==16*1024*1024);
    const Bytes items=read(argv[1]),recipes=read(argv[2]),menu=read(argv[3]);
    const auto newItems=add_item_registry(items),newRecipes=add_recipe_registry(recipes),newMenu=add_crafting_menu(menu);
    const auto oldRefs=array(items,0,16),newRefs=array(newItems,0,16);
    CHECK(newRefs.count==oldRefs.count+1);
    CHECK(slice(items,oldRefs.at,oldRefs.count*16)==slice(newItems,newRefs.at,oldRefs.count*16));
    CHECK(equal(newItems,newRefs.at+oldRefs.count*16,Item));
    const auto oldNames=array(items,56,8),newNames=array(newItems,56,8);
    CHECK(newNames.count==oldNames.count+1);
    for(uint32_t i=0;i<oldNames.count;++i){
        auto a=array(items,oldNames.at+i*8,1),b=array(newItems,newNames.at+i*8,1);
        CHECK(a.count==b.count);CHECK(slice(items,a.at,a.count)==slice(newItems,b.at,b.count));
    }
    const auto oldRows=array(recipes,8,168),newRows=array(newRecipes,8,168);
    CHECK(newRows.count==oldRows.count+1);
    const size_t offsets[]={40,48,64,120},sizes[]={48,24,4,1};
    for(uint32_t i=0;i<oldRows.count;++i) {
        Bytes before=slice(recipes,oldRows.at+i*168,168),after=slice(newRecipes,newRows.at+i*168,168);
        for(size_t f=0;f<4;++f) {
            auto a=array(recipes,oldRows.at+i*168+offsets[f],sizes[f]);
            auto b=array(newRecipes,newRows.at+i*168+offsets[f],sizes[f]);
            CHECK(a.count==b.count);
            CHECK(slice(recipes,a.at,a.count*sizes[f])==slice(newRecipes,b.at,b.count*sizes[f]));
            put32(before,offsets[f],0);put32(after,offsets[f],0);
        }
        CHECK(before==after);
    }
    const size_t row=newRows.at+oldRows.count*168;
    CHECK(equal(newRecipes,row,Recipe));CHECK(u32(newRecipes,row+36)==0);
    CHECK(u32(newRecipes,row+0x38)==0&&u32(newRecipes,row+0x3c)==0);
    CHECK(newRecipes[row+0xa0]==0&&newRecipes[row+0xa1]==0&&newRecipes[row+0xa2]==1);
    CHECK(u32(newRecipes,row+0x50)==0x6807c65f&&u32(newRecipes,row+0x5c)==0x71ee0e45);
    auto in=array(newRecipes,row+40,48),out=array(newRecipes,row+48,24);
    CHECK(in.count==1 && out.count==1);
    CHECK(equal(newRecipes,in.at,Scrap) && u32(newRecipes,in.at+20)==1);
    CHECK(equal(newRecipes,out.at,Item) && u32(newRecipes,out.at+20)==1);
    size_t changed=0;
    for(size_t i=0;i<menu.size();++i)if(menu[i]!=newMenu[i])++changed;
    CHECK(changed<=8); // Only the selected entries array header may change.
    constexpr size_t collection=59920+1600+72;
    const auto trees=array(newMenu,collection+8,8);bool found=false;
    for(uint32_t t=0;t<trees.count;++t){
        const auto groups=array(newMenu,trees.at+t*8,56);
        for(uint32_t g=0;g<groups.count;++g){
            auto sets=array(newMenu,groups.at+g*56+48,40);
            for(uint32_t s=0;s<sets.count;++s){
                auto list=array(newMenu,sets.at+s*40+32,4);
                for(uint32_t i=0;i<list.count;++i)if(u32(newMenu,list.at+i*4)==hash(Recipe)){
                    CHECK(!found && i>=2);found=true;
                    CHECK(u32(newMenu,list.at+(i-1)*4)==hash(WandRecipe));
                    CHECK(u32(newMenu,list.at+(i-2)*4)==hash(HammerRecipe));
                }
            }
        }
    }
    CHECK(found);
    rejects([&]{add_item_registry(newItems);});
    rejects([&]{add_recipe_registry(newRecipes);});
    rejects([&]{add_crafting_menu(newMenu);});
    rejects([&]{add_recipe_registry(Bytes(16));});
    rejects([&]{add_crafting_menu(Bytes(20));});
    rejects([&]{auto bad=items;put32(bad,4,UINT32_MAX);add_item_registry(bad);});
    std::cout<<"PASS: "<<oldRefs.count<<" items and "<<oldRows.count
             <<" recipes preserved; new recipe = 1 scrap -> 1 pincer; manual menu = hammer, wand, pincer; malformed/duplicate input rejected.\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
