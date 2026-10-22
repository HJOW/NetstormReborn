// 애시드 원문의 전체13영역/중복/효과 뒤 재조회와47개 슬롯을 독립 관측과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawAcidFence.h"
#include <filesystem>
#include <iostream>
using namespace netstorm::test::rawscene;
namespace {
// 원본이 받은 인수의 전체DWORD와 순서를 남긴다.
void Event(std::string& output,std::string_view name,std::initializer_list<std::uint32_t> values={}) {
    if (!output.empty()) output+=';';output+=name;
    for (const auto value:values) output+=':'+std::to_string(value); //부호도하위32비트로보존한다.
}
// 입력 숫자의 음수도 같은DWORD로 읽는다.
std::vector<std::uint32_t> Numbers(std::string_view input) {
    std::vector<std::uint32_t> result;
    for (const auto& value:Split(input,',')) result.push_back(static_cast<std::uint32_t>(std::stoll(value))); //float비트를해석하지않는다.
    return result;
}
}
// 실제 원문이 실행한 네 진입점을 기대값 재계산 없이 비교한다.
TEST_CASE(AcidFence_IndependentNativeBodies) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/acid-fence-x86.tsv").string();std::size_t rows{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { //동결원문은기존기대값과분리한다.
        CHECK(row.size()==5);if (row.size()!=5) continue;const auto a=Numbers(row[1]);const auto kind=row[0];
        SidPool pool(OriginalEdition::Patch1078,32768);const Sid parent{50};
        const auto writable=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(raw.data()),50); }; //free/dead입력도같은현재슬롯에설치한다.
        float x=10,y=10,bx=14,by=15,radius=6;std::uint32_t mask=3,mode=0,editor=0,mutation=0,allocation=1,fx=3,fy=4,total=7,localOwner=1;bool boss=true,friendly=false;
        if (kind=="P") allocation=a[2];
        if (kind=="A" || kind=="B") { radius=std::bit_cast<float>(a[0]);x=std::bit_cast<float>(a[1]);y=std::bit_cast<float>(a[2]);mask=a[3];mode=a[4];boss=a[5]!=0;editor=a[6];friendly=a[7]!=0;mutation=a[8];fx=a[9];fy=a[10]; }
        if (kind=="S" || kind=="C") { mode=a[2];boss=a[3]!=0;editor=a[4];friendly=a[5]!=0;mutation=a[6];x=std::bit_cast<float>(a[7]);y=std::bit_cast<float>(a[8]);bx=std::bit_cast<float>(a[9]);by=std::bit_cast<float>(a[10]);fx=a[11];fy=a[12];allocation=a[18]; }
        if (kind=="R") total=a[3];
        std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<std::uint8_t> spots(65536,1);std::array<std::uint32_t,81> alliances{};
        for (const auto [type,genus]:{std::pair{133U,0x50100U},{158U,0x210000U},{73U,0x100000U},{144U,0x100000U},{138U,0x100000U},{146U,0x20000U},{141U,0x20000U},{111U,0x20000U}}) types[type].flags2=genus; //순수메타입력이다.
        std::vector<Sid> ids{Sid{50},Sid{60},Sid{61}};
        for (std::uint16_t sid=70;sid<114;++sid) ids.push_back(Sid{sid}); //현재변경가능47개슬롯이다.
        for (const auto sid:ids) { //원문의50바이트시작값을독립설치한다.
            auto raw=writable(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=sid.value<70 ? 145 : 133;raw[34]=sid.value<70 ? 1 : 2;
            Put(raw,14,std::bit_cast<std::uint32_t>(sid.value==60 ? bx : x));Put(raw,18,std::bit_cast<std::uint32_t>(sid.value==60 ? by : sid.value==61 ? y+5 : y));
        }
        if (kind=="B") { Put(writable(Sid{60}),14,std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<int>(x)+4)));Put(writable(Sid{60}),18,std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<int>(y))));Put(writable(Sid{61}),14,std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<int>(x))));Put(writable(Sid{61}),18,std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<int>(y)+5))); } //기존1080행의입력은변경하지않는다.
        if (mode==2 || mode==5) { writable(Sid{71})[10]=158;writable(Sid{72})[10]=138;writable(Sid{73})[34]=1;writable(Sid{74})[40]=1;writable(Sid{75})[10]=90;types[90].flags2=0x200; }
        constexpr std::array<std::uint8_t,13> kinds{133,144,144,73,73,138,138,144,138,73,146,141,111}; //지역별후보의독립타입이다.
        if (mode==4) for (std::size_t index=0;index<kinds.size();++index) writable(Sid{static_cast<std::uint16_t>(70+index)})[10]=kinds[index]; //13개각각을통과시킨다.
        if (kind=="S" && mode==6) { types[a[14]].flags2=a[13];auto raw=writable(Sid{70});raw[10]=static_cast<std::uint8_t>(a[14]);raw[11]=static_cast<std::uint8_t>(a[15]);raw[40]=static_cast<std::uint8_t>(a[16]);raw[34]=static_cast<std::uint8_t>(a[17]); }
        for (const auto [first,second]:{std::pair{1U,1U},{2U,2U},{3U,3U},{1U,3U},{3U,1U}}) alliances[first*9+second]=1; //독립방향현재값이다.
        types[145].footX=static_cast<int>(fx);types[145].footY=static_cast<int>(fy);std::vector<FrameCode> codes;
        for (std::uint32_t index=0;index<total;++index) codes.push_back({'A',0,static_cast<std::uint8_t>(index),0}); //numFrames관측입력이다.
        frames[145]=RiftTypeFrames(std::move(codes));std::string effects;std::size_t scans{};AcidFenceHooks hooks;AcidFenceTypes special;
        hooks.range=[&](Sid sid) { CHECK(sid==parent);return radius; };
        hooks.find=[&](float px,float py,std::uint32_t type) { Event(effects,"F",{std::bit_cast<std::uint32_t>(px),std::bit_cast<std::uint32_t>(py),type});return (mask&1U) && px==static_cast<int>(x)+4 && py==static_cast<int>(y) ? Sid{60} : (mask&2U) && px==static_cast<int>(x) && py==static_cast<int>(y)+5 ? Sid{61} : Sid{}; };
        hooks.reserveRegular=[&] { Event(effects,"N",{40});if (mutation==8) boss=false;return allocation!=0; };
        hooks.regular=[&](Sid sid,std::uint32_t event,float delay) { Event(effects,"R",{event,sid.value,std::bit_cast<std::uint32_t>(delay)}); };
        hooks.iterate=[&](SquidSearchArea area,const std::function<void(Sid)>& consume) {
            const auto phase=scans++%13;Event(effects,"I",{static_cast<std::uint32_t>(area.left),static_cast<std::uint32_t>(area.top),static_cast<std::uint32_t>(area.right),static_cast<std::uint32_t>(area.bottom)});std::vector<Sid> victims;
            if (mode==1 || mode==6) victims={Sid{70}};else if (mode==2) victims={Sid{50},Sid{60},Sid{61},Sid{70},Sid{71},Sid{72},Sid{73},Sid{74},Sid{75}};
            else if (mode==3) for (std::uint16_t sid=70;sid<114;++sid) victims.push_back(Sid{sid}); //실제40개포화이후도검색된다.
            else if (mode==4) victims={Sid{static_cast<std::uint16_t>(70+phase)}};else if (mode==5) victims={Sid{70},Sid{70},Sid{70},Sid{71},Sid{72}};
            if (mutation==4 && scans==1) { Put(writable(parent),14,std::bit_cast<std::uint32_t>(11.25F));Put(writable(parent),18,std::bit_cast<std::uint32_t>(12.5F)); }
            if (mutation==5 && scans==13) writable(Sid{70})[10]=158;
            if (mutation==9 && scans==1) special.rainFlyer=0;
            for (const auto sid:victims) consume(sid); //정확한후보순서만주며C++필터를대체하지않는다.
        };
        hooks.damage=[&](Sid target,std::int32_t damage,std::uint8_t owner,Sid source) { Event(effects,"H",{target.value,std::bit_cast<std::uint32_t>(damage),owner,source.value});if (mutation==6) { writable(target)[11]=2;writable(parent)[34]=3; } };
        hooks.broadcast=[&](std::span<std::uint8_t> packet) { if (!effects.empty()) effects+=';';effects+="S:"+Hex(packet);const Sid victim{static_cast<std::uint16_t>(Get(packet,4,2))};if (mutation==1) writable(victim)[11]=1;if (mutation==2) writable(parent)[34]=3;if (mutation==7) Put(packet,4,71,2); };
        hooks.assertion=[&](std::uint32_t line) { Event(effects,"!",{line}); };
        hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) { Event(effects,"B",{std::bit_cast<std::uint32_t>(frame),flags});Put(writable(sid),36,std::bit_cast<std::uint32_t>(frame)); };
        AcidZapState state;FormPacketHeader header{0x61,1,11};AcidZapHooks receiver;
        receiver.sound=[&](float px,float py,std::string_view name,std::uint32_t loop) { Event(effects,"W",{std::bit_cast<std::uint32_t>(px),std::bit_cast<std::uint32_t>(py)});effects+=':'+std::string(name)+':'+std::to_string(loop); };
        receiver.dissolve=[&](Sid target,Sid first,Sid second,std::int32_t orientation) { Event(effects,"X",{target.value,first.value,second.value,std::bit_cast<std::uint32_t>(orientation)}); };
        receiver.destroy=[&](Sid target,std::uint32_t flags) { Event(effects,"K",{target.value,flags});writable(target)[11]|=2;if (mutation==3) { Put(writable(parent),14,std::bit_cast<std::uint32_t>(11.5F));Put(writable(parent),18,std::bit_cast<std::uint32_t>(12.25F)); } };
        RawAcidZap zap(pool,types,state,header,std::move(receiver));RawAcidFence acid(pool,types,frames,spots,alliances,boss,editor,friendly,localOwner,special,zap,std::move(hooks));std::uint32_t result{};
        if (kind=="P") { writable(parent)[40]=static_cast<std::uint8_t>(a[1]);PostPopAcidFence(pool,parent,a[0],[&] { Event(effects,"N",{40});return allocation!=0; },[&](Sid sid,std::uint32_t event,float delay) { Event(effects,"R",{event,sid.value,std::bit_cast<std::uint32_t>(delay)}); },[&](Sid,std::uint32_t flags) { Event(effects,"D",{flags}); }); }
        else if (kind=="R") { writable(parent)[40]=static_cast<std::uint8_t>(a[2]);result=std::bit_cast<std::uint32_t>(acid.Handle(parent,a[0],a[1],std::bit_cast<float>(a[4]))); }
        else if (kind=="A" || kind=="B") acid.Attack(parent);
        else { const std::array<Sid,2> neighbors{Sid{60},Sid{61}};acid.Strike(parent,std::span(neighbors).first(a[0]),std::bit_cast<std::int32_t>(a[1])); }
        std::string raws;
        for (const auto sid:ids) raws+=Hex(pool.Slot(sid)); //원문부수효과뒤47슬롯전체를비교한다.
        if ((effects.empty() ? "-" : effects)!=row[2] || raws!=row[3] || result!=Number(row[4])) { std::cout<<"애시드 원문행"<<rows<<"/"<<kind<<":"<<row[1]<<"\n실제:"<<effects<<"\n원본:"<<row[2]<<"\n반환:"<<result<<'/'<<row[4]<<"\n슬롯:"<<(raws==row[3])<<'\n';CHECK(false); }
        ++rows;
    }
    CHECK(rows==1125);
}
