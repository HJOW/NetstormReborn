// 세 템플의 실제 기본 반환과 전용 그림/소리 순서를 동결 원문264입력에 대조한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "o/RawTempleDraw.h"
#include "o/RawRainVortexDraw.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// TYPE 파서의baseFrame과 가상Draw가 원본 순서에 일치해야 한다.
TEST_CASE(TempleDraw_Native264) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game;std::vector<std::int32_t> bases(188,-1);std::size_t count{}; // 같은 타입 번호의 현재 기본 그림 입력이다.
    // 제품에서 파싱한 실제 TYPE 값을 원본 관측 결과와 독립 비교한다.
    for (const auto& type:assets.Types()) bases[type.block+kFirstAssetTypeNumber]=type.definition.specialFrames.baseFrame;
    // 모든 실제 기본 반환/원본Draw 호출을 한 행도 생략하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_TEMPLE_DRAW_FIXTURE).rows) {
        CHECK(row.size()==6);if (row.size()!=6) return;std::array<std::uint8_t,50> raw{}; // 원본에서 불변으로 확인한 같은 현재 입력이다.
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(std::stoul(row[5].substr(index*2,2),nullptr,16)); // 각 raw 바이트를 원문 그대로 읽는다.
        const auto before=raw;const auto main=PatchTempleMainFrame(raw,bases);CHECK(std::bit_cast<std::uint32_t>(main)==Number(row[3]));
        const auto frames=assets.Find(row[0]).definition.FrameTable();std::string trace; // 기대 제출은 fixture에서만 읽는다.
        const auto append=[&](std::string value) { if (!trace.empty()) trace+=',';trace+=value; }; // 중복 기본 그림도 지우지 않는다.
        const auto frame=[&](std::int32_t number) { append("F"+std::to_string(number)); }; // 원본 helper의 부호 프레임을 기록한다.
        if (row[0]=="rainVortex") DrawRainVortex(OriginalEdition::Patch1078,raw,frames,{frame,[&] { append("S"); }});
        else DrawWindThunderTemple(raw,frames,main,frame);
        CHECK((trace.empty() ? "-" : trace)==row[4]);CHECK(raw==before);++count;
    }
    CHECK(count==264);
}
