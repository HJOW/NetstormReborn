// 아크 스파이어의 다섯 원본 몸체를 독립665입력과 전체 슬롯/효과로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawArcSpire.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;

namespace {
// 명시 효과의 모든 DWORD 인자를 원본과 같은 순서로 기록한다.
void ArcEvent(std::string& output,std::string_view name,std::initializer_list<std::uint32_t> values={}) {
    if (!output.empty()) output+=';';output+=name;
    for (const auto value:values) output+=':'+std::to_string(value); //원본 네 바이트 전체를 남긴다.
}
// 큰 finder가 원본과 C++ 양쪽에 공급하는 독립 순서 입력이다.
std::vector<Sid> ArcCandidates(std::uint32_t mode) {
    if (!mode) return {};
    if (mode==1) return {Sid{50},Sid{60},Sid{70},Sid{71},Sid{72}};
    if (mode==2) return {Sid{70},Sid{71},Sid{72}};
    if (mode==4) return {Sid{70},Sid{71},Sid{72},Sid{73},Sid{74},Sid{75}};
    std::vector<Sid> result;
    for (std::uint16_t index=70;index<114;++index) result.push_back(Sid{index}); //40개 포화 뒤의 추가 후보도 공급한다.
    return result;
}
}

// 창/OS를 구동하지 않고 원본 산술/선택/피해 순서/재조회를 C++와 비교한다.
TEST_CASE(ArcSpire_IndependentNativeBodies665) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/arc-spire-x86.tsv").string();std::size_t rows{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { //동결된 원문 행을 바꾸지 않는다.
        CHECK(row.size()==12);if (row.size()!=12) continue;
        const auto fields=Split(row[1],',');std::vector<std::uint32_t> a;
        for (const auto& field:fields) a.push_back(Number(field)); //각 입력의 float 비트/음수 count도 그대로 읽는다.
        for (std::size_t index=2;index<7;++index) CHECK(row[index]==row[index+5]); //두 x87 정밀도 결과의 일치를 확인한다.
        SidPool pool(OriginalEdition::Patch1078,32768);const Sid parent{50};
        const auto writable=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(raw.data()),50); }; //예약 슬롯을 명시 원문 입력으로 채운다.
        std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<std::uint8_t> spots(65536,row[0]=="A" ? 1 : 0);
        std::array<std::uint32_t,81> alliances{};std::uint32_t editor{},mask{},mode{},mutation{};float x=10,y=10,radius=6,rate=41.5F,period=.75F;
        if (row[0]=="A") { radius=std::bit_cast<float>(a[0]);x=std::bit_cast<float>(a[1]);y=std::bit_cast<float>(a[2]);mask=a[3];mode=a[4];editor=a[5];mutation=a[6];period=std::bit_cast<float>(a[7]);rate=std::bit_cast<float>(a[8]); }
        if (row[0]=="S" || row[0]=="T") { mode=a[1];editor=a[2];mutation=a[3]; }
        if (row[0]=="R") period=std::bit_cast<float>(a[4]);
        for (auto& type:types) type.flags1=0x10; //실제4adc60의 기본 damageable 메타다.
        const auto setup=[&](Sid sid) {
            auto raw=writable(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=sid.value<70 ? 149 : 78;raw[34]=sid.value<70 ? 1 : 2;
            Put(raw,14,std::bit_cast<std::uint32_t>(x+(sid.value==60 ? 4.0F : 0.0F)));Put(raw,18,std::bit_cast<std::uint32_t>(y+(sid.value==61 ? 5.0F : 0.0F)));
        }; //부모/이웃/후보는 원본과 같은50바이트를 사용한다.
        for (const auto sid:{Sid{50},Sid{60},Sid{61}}) setup(sid); //두 축의 이웃이다.
        for (std::uint16_t sid=70;sid<114;++sid) setup(Sid{sid}); //큰 finder가 읽을 모든 후보를 준비한다.
        if (mode==1) { writable(Sid{71})[10]=79;types[79].flags2=0x120000;writable(Sid{72})[40]=1; }
        if (mode==4) {
            types[78].flags1=0;writable(Sid{71})[40]=1;writable(Sid{72})[34]=1;
            writable(Sid{73})[10]=91;types[91].flags2=0x400000;writable(Sid{74})[10]=92;types[92].flags2=0x120000;writable(Sid{75})[10]=93;
        }
        if (row[0]=="T") {
            const auto offset=std::bit_cast<float>(a[5]);Put(writable(Sid{60}),14,std::bit_cast<std::uint32_t>(14.0F+offset));Put(writable(Sid{60}),18,std::bit_cast<std::uint32_t>(10.0F+offset));
        } //실제 정수 근처 float 입력만 경계에 공급한다.
        for (const auto pair:{std::pair{1U,1U},std::pair{2U,2U},std::pair{3U,3U},std::pair{1U,3U},std::pair{3U,1U}}) alliances[pair.first*9+pair.second]=1; //반대 방향도 별도 메타다.
        std::vector<FrameCode> codes;
        for (std::uint32_t index=0;index<(row[0]=="R" ? a[3] : 6U);++index) codes.push_back({'A',0,static_cast<std::uint8_t>(index),0}); //현재 TYPE.numFrames의 입력이다.
        frames[149]=RiftTypeFrames(std::move(codes));std::string effects;ArcSpireHooks hooks;
        hooks.range=[&](Sid sid) { CHECK(sid==parent);return radius; };
        hooks.damageRate=[&](Sid sid) { CHECK(sid==parent);ArcEvent(effects,"A",{std::bit_cast<std::uint32_t>(period)});return rate; };
        hooks.find=[&](float px,float py,std::uint32_t type) {
            ArcEvent(effects,"F",{std::bit_cast<std::uint32_t>(px),std::bit_cast<std::uint32_t>(py),type});
            if (row[0]=="N") return a[6] ? Sid{60} : Sid{};
            if ((mask&1U) && px==static_cast<int>(x)+4 && py==static_cast<int>(y)) return Sid{60};
            if ((mask&2U) && px==static_cast<int>(x) && py==static_cast<int>(y)+5) return Sid{61};
            return Sid{};
        };
        hooks.iterate=[&](SquidSearchArea area,const std::function<void(Sid)>& consume) {
            ArcEvent(effects,"I",{static_cast<std::uint32_t>(area.left),static_cast<std::uint32_t>(area.top),static_cast<std::uint32_t>(area.right),static_cast<std::uint32_t>(area.bottom)});
            for (const auto sid:ArcCandidates(mode)) consume(sid); //실제 finder 경계의 독립 후보 순서만 공급한다.
        };
        hooks.beam=[&](Sid source,Sid target) { ArcEvent(effects,"L",{source.value,target.value});if (mutation==1 && target.value==70) writable(target)[11]=1; };
        hooks.damage=[&](Sid target,std::int32_t value,std::uint8_t owner,Sid source) {
            ArcEvent(effects,"H",{target.value,std::bit_cast<std::uint32_t>(value),owner,source.value});
            if (mutation==2 && target.value==70) { writable(target)[11]=2;writable(parent)[34]=3; }
        };
        hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) { CHECK(sid==parent);ArcEvent(effects,"B",{std::bit_cast<std::uint32_t>(frame),flags});Put(writable(sid),36,std::bit_cast<std::uint32_t>(frame)); };
        hooks.damageable=[&](Sid sid,std::uint32_t event,std::uint32_t count,float value) { CHECK(sid==parent);ArcEvent(effects,"C",{event,count,std::bit_cast<std::uint32_t>(value)});return .75F; };
        RawArcSpire arc(pool,types,frames,spots,alliances,editor,std::move(hooks));std::uint32_t result{},count{};std::string buffer="-";
        if (row[0]=="P") {
            writable(parent)[40]=static_cast<std::uint8_t>(a[1]);
            PostPopArcSpire(pool,parent,a[0],[&] { ArcEvent(effects,"N",{40});return a[2]!=0; },
                [&](Sid sid,std::uint32_t event,float value) { ArcEvent(effects,"R",{event,sid.value,std::bit_cast<std::uint32_t>(value)}); },
                [&](Sid sid,std::uint32_t flags) { CHECK(sid==parent);ArcEvent(effects,"D",{flags}); });
        } else if (row[0]=="R") {
            writable(parent)[40]=static_cast<std::uint8_t>(a[2]);result=std::bit_cast<std::uint32_t>(arc.Handle(parent,a[0],a[1],period));
        } else if (row[0]=="N") {
            auto raw=writable(Sid{60});raw[40]=static_cast<std::uint8_t>(a[0]);raw[34]=3;Put(raw,14,std::bit_cast<std::uint32_t>(a[3] ? 14.25F : 14.0F));
            alliances[10]=a[1];alliances[12]=a[1];alliances[28]=a[2];count=a[5];std::array<Sid,20> values;values.fill(Sid{0xa5a5});
            result=arc.Collect(parent,std::span(values).first(a[4]),count,14,10,149);
            std::array<std::uint8_t,80> original;original.fill(0xa5);
            for (std::size_t index=0;index<values.size();++index) if (values[index].value!=0xa5a5) Put(original,index*4,values[index].value); //호스트 WORD SID를 원본 DWORD 항목으로 정규화한다.
            buffer=Hex(original);
        } else if (row[0]=="A") arc.Attack(parent,period);
        else { const std::array<Sid,2> neighbors{Sid{60},Sid{61}};arc.Strike(parent,std::span(neighbors).first(a[0]),std::bit_cast<std::int32_t>(a[4])); }
        std::string raws;
        for (const auto sid:{Sid{50},Sid{60},Sid{61},Sid{70},Sid{71},Sid{72}}) raws+=Hex(pool.Slot(sid)); //표시 변경과 효과 뒤 현재 owner/상태도 대조한다.
        if ((effects.empty() ? "-" : effects)!=row[2] || raws!=row[3] || result!=Number(row[4]) || count!=Number(row[5]) || buffer!=row[6]) {
            std::cout<<"Arc 원문 행"<<rows<<"/"<<row[0]<<":"<<row[1]<<"\n효과:"<<effects<<"\n원본:"<<row[2]<<"\n반환:"<<result<<"/"<<row[4]<<"\n";CHECK(false);
        }
        ++rows;
    }
    CHECK(rows==665);
}
