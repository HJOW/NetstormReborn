// 아이스 네몸체의 독립 원문과 모든 효과/세슬롯/float반환을 비교한다.
#include "RawSceneSupport.h"
#include "o/RawIceTower.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원문 요청의 전체DWORD를 호출순서대로 남긴다.
void Event(std::string& output,std::string_view name,std::initializer_list<std::uint32_t> values={}) { if (!output.empty()) output+=';';output+=name;for (const auto value:values) output+=':'+std::to_string(value); } //부호비트도보존한다.
// 동결입력의 부호/float비트를 동일DWORD로 읽는다.
std::vector<std::uint32_t> Numbers(std::string_view text) { std::vector<std::uint32_t> result;for (const auto& value:Split(text,',')) result.push_back(static_cast<std::uint32_t>(std::stoll(value)));return result; } //기대값은계산하지않는다.
}
// 최초예약/재생성/삭제/현재프레임주기를 보호PE1426행과 대조한다.
TEST_CASE(IceTower_IndependentNativeBodies1426) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/ice-tower-x86.tsv").string();std::size_t rows{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { //기존fixture를수정하지않고새관측을읽는다.
        CHECK(row.size()==5);if (row.size()!=5) continue;const auto a=Numbers(row[1]);const auto kind=row[0];
        SidPool pool(OriginalEdition::Patch1078,32768);const Sid parent{50},surfaceSid{60},born{61};
        const auto writable=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(raw.data()),50); }; //독립원문이준free/void/dead도보존한다.
        const auto position=[&](float x,float y) { Put(writable(parent),14,std::bit_cast<std::uint32_t>(x));Put(writable(parent),18,std::bit_cast<std::uint32_t>(y)); }; //경계가공급한현재좌표만바꾼다.
        const auto mutation=a.back();std::uint32_t flags{},extra{},total=7,loading{},surface{},found{},allocation=1,type=150,frame=4;bool battle=true,boss=true;IceTowerTypes types;
        if (kind=="P") { flags=a[0];extra=a[1];total=a[2];allocation=a[3]; }
        else if (kind=="R") { extra=a[2];total=a[3];boss=a[4]!=0;frame=a[5]; }
        else if (kind=="D") { flags=a[0];extra=a[1];type=a[2];loading=a[3];battle=a[4]!=0;surface=a[5]; }
        else { flags=a[0];extra=a[1];found=a[2]; }
        for (const auto sid:{parent,surfaceSid,born}) { //세슬롯의원래50바이트를명시설치한다.
            auto raw=writable(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000+(sid==born ? 0x100U : 0));raw[10]=static_cast<std::uint8_t>(sid==parent ? type : sid==surfaceSid ? surface : 0);raw[11]=static_cast<std::uint8_t>(sid==parent && (kind=="D" || kind=="G") ? 6 : sid==born ? 1 : 0);raw[34]=sid!=born ? 2 : 0;
            if (sid==parent) { Put(raw,12,0xa55a,2);Put(raw,14,std::bit_cast<std::uint32_t>(20.25F));Put(raw,18,std::bit_cast<std::uint32_t>(21.5F));raw[35]=0x44;raw[40]=static_cast<std::uint8_t>(extra);Put(raw,36,frame); }
        }
        std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));
        for (const auto pair:{std::pair{150U,total},std::pair{151U,9U}}) { //현재TYPE별numFrames입력이다.
            std::vector<FrameCode> codes;for (std::uint32_t index=0;index<pair.second;++index) codes.push_back({'A',0,static_cast<std::uint8_t>(index),0}); //개수만공급한다.
            frames[pair.first]=RiftTypeFrames(std::move(codes));
        }
        std::string effects;IceTowerHooks hooks;
        hooks.reserveRegular=[&] { Event(effects,"N",{40});if (mutation==1) writable(parent)[10]=151;return allocation!=0; };
        hooks.regular=[&](Sid sid,std::uint32_t event,float period) { Event(effects,"R",{event,sid.value,std::bit_cast<std::uint32_t>(period)});if (mutation==10) position(61,62); };
        hooks.basePop=[&](Sid sid,std::uint32_t value) { CHECK(sid==parent);Event(effects,"P",{value}); };
        hooks.damageablePost=[&](Sid sid,std::uint32_t value) { CHECK(sid==parent);Event(effects,"D",{value}); };
        hooks.basePost=[&](Sid sid,std::uint32_t value) { CHECK(sid==parent);Event(effects,"B",{value}); };
        hooks.grid=[&](float x,float y) { Event(effects,"Q",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)});if (mutation==6) { battle=false;position(30.5F,31.25F);writable(parent)[34]=4; }if (mutation==8) types.noIsland=95;return surface ? surfaceSid : Sid{}; };
        hooks.genusAt=[&](float x,float y,std::uint32_t genus) { Event(effects,"G",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),genus});if (mutation==11) { position(50,51);writable(parent)[34]=4; }return found ? surfaceSid : Sid{}; };
        hooks.dropIsland=[&](float x,float y,std::uint32_t value,std::uint32_t owner) { Event(effects,"J",{std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),value,owner}); };
        hooks.create=[&](std::uint32_t requested,std::uint32_t value) {
            Event(effects,"C",{requested,value});auto raw=writable(born);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200100);raw[10]=static_cast<std::uint8_t>(mutation==7 ? 82 : requested);raw[11]=4;raw[35]=0x99;Put(raw,12,0x7788,2);Put(raw,36,4);
            if (mutation==2) { writable(parent)[34]=3;position(11.5F,12.25F);Put(writable(parent),12,0x3344,2);writable(parent)[35]=77; }
            if (mutation==12) writable(parent)[40]=32;return born;
        };
        hooks.unpop=[&](Sid sid,std::uint32_t value) { Event(effects,"U",{sid.value,value});writable(sid)[11]|=4;if (mutation==5) battle=false; };
        hooks.destroy=[&](Sid sid,std::uint32_t value) { Event(effects,"K",{sid.value,value});writable(sid)[11]|=3;if (mutation==4) position(90,91); };
        hooks.owner=[&](Sid sid,std::uint8_t owner) { Event(effects,"O",{sid.value,owner});writable(sid)[34]=owner;if (mutation==3) { position(13.5F,14.25F);writable(parent)[35]=78; } };
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t value) { Event(effects,"V",{sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),value});Put(writable(sid),14,std::bit_cast<std::uint32_t>(x));Put(writable(sid),18,std::bit_cast<std::uint32_t>(y));writable(sid)[11]&=static_cast<std::uint8_t>(~4U); };
        hooks.word=[&](Sid sid,std::uint16_t value) { Put(writable(sid),12,value,2);if (writable(sid)[10]==82) { Event(effects,"T",{sid.value});writable(parent)[35]=88;writable(parent)[34]=4;position(19.5F,20.5F); } };
        hooks.setFrame=[&](Sid sid,std::int32_t value,std::uint32_t mode) { Event(effects,"F",{std::bit_cast<std::uint32_t>(value),mode});Put(writable(sid),36,std::bit_cast<std::uint32_t>(value));if (mutation==9) writable(sid)[10]=151; };
        RawIceTower ice(pool,frames,types,battle,boss,loading,std::move(hooks));std::uint32_t result{};
        if (kind=="P") ice.PostPop(parent,flags);else if (kind=="D") ice.PostDestroy(parent,flags);else if (kind=="G") ice.GrowingPostDestroy(parent,flags);else result=std::bit_cast<std::uint32_t>(ice.Handle(parent,a[0],a[1],std::bit_cast<float>(a[6])));
        std::string raws;for (const auto sid:{parent,surfaceSid,born}) raws+=Hex(pool.Slot(sid)); //세슬롯전체의원문출력을비교한다.
        if ((effects.empty() ? "-" : effects)!=row[2] || raws!=row[3] || result!=Number(row[4])) { std::cout<<"아이스 원문행"<<rows<<"/"<<kind<<":"<<row[1]<<"\n실제:"<<effects<<"\n원본:"<<row[2]<<"\n반환:"<<result<<'/'<<row[4]<<"/슬롯:"<<(raws==row[3])<<'\n';CHECK(false); }
        ++rows;
    }
    CHECK(rows==1426);
}
