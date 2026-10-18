// 독립 원본1050입력의604 전체 반환/호출/50바이트 raw를 비교한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "o/RawPriestRitualFrame.h"
using namespace netstorm::test::rawscene;
// 실제 사제 코드표와 선언한 짧은 코드 입력/효과 변이를 재생한다. 픽셀 장치는 경계다.
TEST_CASE(PriestRitualFrame_Native1050WholeState) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto actual=assets.game.Types()[158-70].definition.FrameTable();SidPool pool(OriginalEdition::Patch1078,24000,true);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::size_t count{}; // 원본과 같은 TYPE/frame 입력이다.
    for (const auto& row:LoadFixture(NETSTORM_PRIEST_RITUAL_FRAME_FIXTURE).rows) { // 결과는 원본 원문에서만 읽는다.
        CHECK(row.size()==4);if (row.size()!=4) continue;const auto input=Split(row[0],',');CHECK(input.size()==5);if (input.size()!=5) continue;const auto mode=Number(input[0]),current=Number(input[1]),side=Number(input[2]),exists=Number(input[3]),mutation=Number(input[4]); // 입력 DWORD의 비트를 보존한다.
        frames[158]=mode ? RiftTypeFrames({FrameCode{static_cast<std::uint8_t>(side),'P',1,0}}) : actual;frames[133]=frames[158];const auto view=pool.Slot(Sid{50});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(view.data()),view.size()};std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});raw[10]=158;Put(raw,14,0x41a40000);Put(raw,18,0x41ac0000);Put(raw,36,current);std::string trace; // callback만 바꾸는 현재 물리 입력이다.
        const auto event=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 원문과 같은 실제 요청 순서다.
        PriestRitualFrameHooks hooks; // setter/605/위치 음향의 명시 경계다.
        hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) { event("F:"+std::to_string(sid.value)+":"+std::to_string(static_cast<std::uint32_t>(frame))+":"+std::to_string(flags));Put(raw,36,static_cast<std::uint32_t>(frame)); };
        hooks.eventExists=[&](Sid sid,std::uint32_t number) { event("E:"+std::to_string(sid.value)+":"+std::to_string(number));if (mutation==1) { Put(raw,14,0x41b20000);Put(raw,18,0xc18c0000);Put(raw,36,99); }return exists!=0; };
        hooks.playAt=[&](float x,float y,std::string_view name,std::uint32_t flags) { event("S:"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":"+std::string(name)+":"+std::to_string(flags));if (mutation==2) { raw[10]=133;Put(raw,36,77); } };
        RawPriestRitualFrame ritual(pool,frames,std::move(hooks));const auto result=std::bit_cast<std::array<std::uint8_t,4>>(ritual.Tick(Sid{50}));CHECK(Hex(result)==row[1]);CHECK((trace.empty() ? std::string("-") : trace)==row[2]);CHECK(Hex(raw)==row[3]);++count;
    }
    CHECK(count==1050);
}
