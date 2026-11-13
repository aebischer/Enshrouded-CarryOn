#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace carryon::content {
using Bytes=std::vector<uint8_t>;
using Guid=std::array<uint8_t,16>;
inline Guid guid(const char* hex) {
    Guid out{};
    auto digit=[](char c)->uint8_t{
        if(c>='0'&&c<='9')return static_cast<uint8_t>(c-'0');
        if(c>='a'&&c<='f')return static_cast<uint8_t>(c-'a'+10);
        throw std::runtime_error("invalid GUID");
    };
    if(std::strlen(hex)!=32)throw std::runtime_error("invalid GUID length");
    for(size_t i=0;i<16;++i)out[i]=static_cast<uint8_t>(digit(hex[2*i])*16+digit(hex[2*i+1]));
    return out;
}
inline const Guid Item=guid("25f0306f96e4484998ae7be76dbb24a1");
inline const Guid Recipe=guid("027bfcad48de5943b64eb85b4b15fd63");
inline const Guid Icon=guid("af2631b44931884b97f5addff63b774a");
inline const Guid Scrap=guid("6698bf4f397d7344b3f1b9d03a55ca83");
inline const Guid Hammer=guid("873d587c00f3174da27e9a77fbbfaeba");
inline const Guid Wand=guid("6bb2cf30ab2c7340a4d0f4afb2045004");
inline const Guid HammerRecipe=guid("a8bb031e7c147c41935ecff24bc8e15c");
inline const Guid WandRecipe=guid("d3e91910b612cc4087df5f3ae7fc7808");
inline uint32_t hash(const Guid& g) noexcept {
    uint32_t value=2166136261u;
    for(auto b:g)value=(value^b)*16777619u;
    return value;
}
inline void range(const Bytes& b,size_t at,size_t size) {
    if(at>b.size()||size>b.size()-at)throw std::runtime_error("truncated native resource");
}
inline uint32_t u32(const Bytes& b,size_t at) {
    range(b,at,4);uint32_t v;std::memcpy(&v,b.data()+at,4);return v;
}
inline void put32(Bytes& b,size_t at,uint32_t v) {
    range(b,at,4);std::memcpy(b.data()+at,&v,4);
}
inline void putGuid(Bytes& b,size_t at,const Guid& v) {
    range(b,at,16);std::memcpy(b.data()+at,v.data(),16);
}
inline bool equal(const Bytes& b,size_t at,const Guid& v) {
    range(b,at,16);return std::memcmp(b.data()+at,v.data(),16)==0;
}
struct Array {size_t at;uint32_t count;};
inline Array array(const Bytes& b,size_t field,size_t stride) {
    const auto offset=u32(b,field),count=u32(b,field+4);
    if(count>1000000 || !stride)throw std::runtime_error("native array limit");
    const size_t at=field+offset;
    range(b,at,static_cast<size_t>(count)*stride);
    return {at,count};
}
inline size_t append(Bytes& b,const Bytes& data) {
    if(b.size()>16*1024*1024 || data.size()>16*1024*1024-b.size())
        throw std::runtime_error("native resource size limit");
    b.resize((b.size()+3)&~size_t(3));
    const size_t at=b.size();b.insert(b.end(),data.begin(),data.end());return at;
}
inline void point(Bytes& b,size_t field,size_t at,uint32_t count) {
    if(at<field || at-field>UINT32_MAX)throw std::runtime_error("relative offset overflow");
    put32(b,field,static_cast<uint32_t>(at-field));put32(b,field+4,count);
}
inline Bytes slice(const Bytes& b,size_t at,size_t count) {
    range(b,at,count);return Bytes(b.begin()+at,b.begin()+at+count);
}
inline void string(Bytes& b,size_t field,const char* value) {
    Bytes text(value,value+std::strlen(value));text.push_back(0);
    const size_t at=append(b,text);point(b,field,at,static_cast<uint32_t>(text.size()-1));
}
// Pure resource transforms for the pinned client. No process memory or game files written.
inline Bytes add_item_registry(const Bytes& source) {
    const auto refs=array(source,0,16),names=array(source,56,8);
    if(refs.count!=names.count)throw std::runtime_error("registry name mismatch");
    for(uint32_t i=0;i<refs.count;++i) {
        Guid existing{};std::memcpy(existing.data(),source.data()+refs.at+i*16,16);
        if(hash(existing)==hash(Item))throw std::runtime_error("item ID already present");
    }
    Bytes out=source,newRefs=slice(source,refs.at,static_cast<size_t>(refs.count)*16);
    newRefs.insert(newRefs.end(),Item.begin(),Item.end());
    const size_t refsAt=append(out,newRefs);point(out,0,refsAt,refs.count+1);
    const size_t namesAt=append(out,Bytes(static_cast<size_t>(names.count+1)*8));
    for(uint32_t i=0;i<names.count;++i) {
        const auto old=array(source,names.at+i*8,1);
        Bytes text=slice(source,old.at,old.count);text.push_back(0);
        const size_t at=append(out,text);point(out,namesAt+i*8,at,old.count);
    }
    string(out,namesAt+names.count*8,"CarryOn_TransportPincer");
    point(out,56,namesAt,names.count+1);return out;
}
inline Bytes add_recipe_registry(const Bytes& source) {
    constexpr size_t stride=168;
    const auto recipes=array(source,8,stride);
    size_t model=SIZE_MAX;bool wandFound=false;
    for(uint32_t i=0;i<recipes.count;++i) {
        const size_t row=recipes.at+i*stride;
        if(u32(source,row+16)==hash(Recipe))throw std::runtime_error("recipe ID already present");
        if(equal(source,row,HammerRecipe))model=row;
        if(equal(source,row,WandRecipe))wandFound=true;
    }
    if(model==SIZE_MAX||!wandFound||u32(source,model+36)!=0)
        throw std::runtime_error("manual crafting anchors missing");
    Bytes out=source;
    const size_t rowsAt=append(out,Bytes(static_cast<size_t>(recipes.count+1)*stride));
    // Moving a row requires copying its relative children after the new row array.
    for(uint32_t i=0;i<recipes.count;++i) {
        const size_t old=recipes.at+i*stride,row=rowsAt+i*stride;
        std::memcpy(out.data()+row,source.data()+old,stride);
        const size_t fields[]={40,48,64,120},sizes[]={48,24,4,1};
        for(size_t f=0;f<4;++f) {
            const auto child=array(source,old+fields[f],sizes[f]);
            Bytes data=slice(source,child.at,static_cast<size_t>(child.count)*sizes[f]);
            if(fields[f]==120)data.push_back(0);
            const size_t at=append(out,data);point(out,row+fields[f],at,child.count);
        }
    }
    const size_t row=rowsAt+recipes.count*stride;
    std::memcpy(out.data()+row,source.data()+model,stride);
    putGuid(out,row,Recipe);put32(out,row+16,hash(Recipe));
    // Preserve native manual-craft timing and progression checks of the hammer.
    Bytes ingredient(48);putGuid(ingredient,0,Scrap);
    put32(ingredient,16,hash(Scrap));put32(ingredient,20,1);
    size_t at=append(out,ingredient);point(out,row+40,at,1);
    Bytes product(24);putGuid(product,0,Item);
    put32(product,16,hash(Item));put32(product,20,1);
    at=append(out,product);point(out,row+48,at,1);
    at=append(out,{});point(out,row+64,at,0); // no station/prop required
    string(out,row+120,"Recipe_CarryOn_TransportPincer_Manual");
    point(out,8,rowsAt,recipes.count+1);return out;
}
inline Bytes add_crafting_menu(const Bytes& source) {
    constexpr size_t collection=59920+1600+72;
    const auto workshops=array(source,collection,4),trees=array(source,collection+8,8);
    if(workshops.count!=trees.count)throw std::runtime_error("crafting tree mismatch");
    size_t target=SIZE_MAX;uint32_t insert=0;Array entries{};
    for(uint32_t t=0;t<trees.count;++t) {
        if(u32(source,workshops.at+t*4)!=0)continue; // manual crafting only
        const auto groups=array(source,trees.at+t*8,56);
        for(uint32_t g=0;g<groups.count;++g) {
            const auto sets=array(source,groups.at+g*56+48,40);
            size_t groupCount=0;bool groupTarget=false;
            for(uint32_t s=0;s<sets.count;++s) {
                const size_t field=sets.at+s*40+32;const auto list=array(source,field,4);
                groupCount+=list.count;int hammer=-1,wand=-1;
                for(uint32_t i=0;i<list.count;++i) {
                    const auto id=u32(source,list.at+i*4);
                    if(id==hash(Recipe))throw std::runtime_error("menu entry already present");
                    if(id==hash(HammerRecipe))hammer=static_cast<int>(i);
                    if(id==hash(WandRecipe))wand=static_cast<int>(i);
                }
                if(hammer>=0&&wand>=0) {
                    if(target!=SIZE_MAX)throw std::runtime_error("ambiguous crafting anchors");
                    groupTarget=true;target=field;entries=list;insert=static_cast<uint32_t>((hammer>wand?hammer:wand)+1);
                }
            }
            if(groupTarget&&groupCount>=512)throw std::runtime_error("native menu group full");
        }
    }
    if(target==SIZE_MAX)throw std::runtime_error("manual crafting position not found");
    Bytes out=source,data=slice(source,entries.at,static_cast<size_t>(entries.count)*4);
    const uint32_t id=hash(Recipe);const auto* bytes=reinterpret_cast<const uint8_t*>(&id);
    data.insert(data.begin()+insert*4,bytes,bytes+4);
    const size_t at=append(out,data);point(out,target,at,entries.count+1);return out;
}
} // namespace carryon::content
