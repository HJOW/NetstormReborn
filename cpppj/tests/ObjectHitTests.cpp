// 실제498000 관측의 반환/호출순서/버퍼를대조한다. 실제공중3종은생성하거나검사하지않는다.
#include "RawSceneSupport.h"
#include "client/RawObjectHit.h"
using namespace netstorm::test::rawscene;
using namespace netstorm::client;

TEST_CASE(ObjectHit_ReplaysNative608PixelBranchesAndCallOrder) { // 원문좌표/헤더/getter재호출/200경계/색255를독립fixture와대조한다.
    SidPool pool(OriginalEdition::Patch1078,24000,true);std::vector<RiftTypeRecord> types(188);std::vector<SquidDisplayShape> shapes(188);std::size_t count{},accepted{}; // 합성TYPE80만사용하는현재저장소다.
    for (const auto& row:LoadFixture(NETSTORM_OBJECT_HIT_FIXTURE).rows) { // 기대값은보호PE의실제명령실행결과다.
        CHECK(row.size()==4);const auto values=Split(row[0],',');CHECK(values.size()==17);const auto number=[&](std::size_t at) { return std::stoi(values[at]); }; // signed입력과float원문비트열을구별한다.
        const Sid sid{50};auto bytes=pool.Slot(sid);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=80;raw[40]=static_cast<std::uint8_t>(number(7));Put(raw,14,Number(values[0]));Put(raw,18,Number(values[1]));Put(raw,36,Number(values[8])); // 같은비정렬입력이다.
        types[80].flags1=Number(values[6]);types[80].hotspotX=3;types[80].hotspotY=-2;shapes[80].loaded=true;shapes[80].frameCount=4;
        shapes[80].frames.assign(4,{static_cast<std::int16_t>(number(12)),static_cast<std::int16_t>(number(13)),static_cast<std::int16_t>(number(14)),static_cast<std::int16_t>(number(15))});
        std::vector<std::uint8_t> capture(40000,173);std::string trace;std::size_t getter{};const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end()); // 미도달출력은이전전역픽셀을보존한다.
        const auto append=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 원문호출순서열이다.
        const auto pixel=[](std::span<std::uint8_t> target,int x,int y,std::uint8_t color) { if (x>=0 && x<200 && y>=0 && y<200) target[static_cast<std::size_t>(y)*200+x]=color; }; // 독립출력경계의명시픽셀만쓴다.
        ObjectHitHooks hooks;hooks.frame=[&](Sid current) { CHECK(current==sid);const auto value=number(9+std::min(getter++,std::size_t{2}));append("frame:"+std::to_string(value));return value; };
        hooks.plain=[&](auto type,auto frame,auto x,auto y,std::span<std::uint8_t> target) { CHECK(type==80 && target.size()==40000);append("plain:"+std::to_string(frame)+','+std::to_string(x)+','+std::to_string(y));constexpr std::array<std::uint8_t,4> colors{255,9,0,255};pixel(target,x,y,colors.at(static_cast<std::size_t>(frame)));pixel(target,x+1,y+1,colors.at(static_cast<std::size_t>(frame)));capture.assign(target.begin(),target.end()); }; // 실제RLE가아닌원문oracle의명시출력경계다.
        hooks.additional=[&](Sid current,auto x,auto y,std::span<std::uint8_t> target) { CHECK(current==sid);append("extra:"+std::to_string(x)+','+std::to_string(y));if (number(16)) pixel(target,x+2,y+2,number(16)==1 ? 7 : 255);capture.assign(target.begin(),target.end()); }; // 추가그림의255덮어쓰기도유지한다.
        const bool actual=RawObjectHit(pool,types,shapes,std::move(hooks)).Hit(sid,number(4),number(5),{number(2),number(3)});
        CHECK(actual==(Number(row[1])!=0));CHECK(trace==row[2]);CHECK(Adler(capture)==Number(row[3]));CHECK(std::equal(raw.begin(),raw.end(),before.begin()));accepted+=actual;++count;
    }
    CHECK(count==608 && accepted>0 && accepted<count);
}

TEST_CASE(ObjectHit_RequiresOnlyReachedDrawingAndRejectsInvalidHeader) { // opaqueは描画不要であり負点/200外の呼出差を検査する。
    SidPool pool(OriginalEdition::Patch1078,24000,true);std::vector<RiftTypeRecord> types(188);std::vector<SquidDisplayShape> shapes(188);const Sid sid{50}; // 選択対象の実体は合成TYPE80だ。
    auto bytes=pool.Slot(sid);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=80;shapes[80]={1,true,{{20,20,0,0}}};types[80].flags1=0x800000;
    ObjectHitHooks hooks;hooks.frame=[](Sid) { return 0; };CHECK(RawObjectHit(pool,types,shapes,hooks).Hit(sid,1,1,{0,0}));
    types[80].flags1=0;CHECK(!RawObjectHit(pool,types,shapes,hooks).Hit(sid,-1,-1,{0,0}));CHECK(Throws([&] { static_cast<void>(RawObjectHit(pool,types,shapes,hooks).Hit(sid,200,200,{0,0})); }));
    shapes[80].loaded=false;CHECK(Throws([&] { static_cast<void>(RawObjectHit(pool,types,shapes,hooks).Hit(sid,-1,-1,{0,0})); }));
}
