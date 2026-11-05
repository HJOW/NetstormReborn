// 실제CPU의 두 원문 Wind/Random/각도 시퀀스와 장기 Storage owner를 비교한다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "RawSceneSupport.h"
#include "o/RawAmbientWind.h"
#include "client/RawSurfaceWorld.h"
#include "client/ClientRandomGlobals.h"
#include <algorithm>
#include <bit>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 기대값은 독립CPU TSV에만 있으며 입력 비트/수명만 조립한다.
// 두 실제PE에서 기록한 입력과 결과를 불변으로 보관한다.
const std::vector<std::vector<std::string>>& WindRows() { static const auto rows=LoadFixture(NETSTORM_AMBIENT_WIND_FIXTURE).rows;return rows; }
// Native의 세 float 비트와 마지막 게임 난수를 직접 비교한다.
bool Matches(const AmbientWindState& state,const GameRandom& random,const std::vector<std::string>& row) {
    return std::bit_cast<std::uint32_t>(state.x)==Number(row[8]) && std::bit_cast<std::uint32_t>(state.y)==Number(row[9]) && std::bit_cast<std::uint32_t>(state.angle)==Number(row[10]) && random.State()==Number(row[11]);
}
// 같은 PE 최초0/1/0과55 시드의 실제CPU 연속 입력을 찾는다. 결과를 계산하지 않는다.
const std::vector<std::string>& InitialSequence(std::uint32_t steps) {
    const auto& rows=WindRows();const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto& row) { return row[0]=="originals" && Number(row[1])==0 && Number(row[2])==55 && Number(row[3])==steps && Number(row[7])==0; });
    if (found==rows.end()) throw std::logic_error("원문 최초 바람 입력 누락");return *found;
}
}
// 양판본/현재DWORD/0재시드/장기 감쇠·난수 소비1920행을 동일 물리 비트로 비교한다.
TEST_CASE(AmbientWind_ReplaysNativeCpuSequencesAndSharedRandom) {
    std::size_t count=0;
    // 각 행은 독립 초기 입력이며 행 안에서는 같은 바람/난수를 이어 소비한다.
    for (const auto& row:WindRows()) {
        CHECK(row.size()==14);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        AmbientWindState state{std::bit_cast<float>(Number(row[4])),std::bit_cast<float>(Number(row[5])),std::bit_cast<float>(Number(row[6]))};GameRandom random(Number(row[2]));std::uint32_t checking=Number(row[7]);std::string events;
        BoardAngleDiagnostics diagnostics{RawCheckingFlag{checking},[&](auto expression,auto file,auto line) { if (!events.empty()) events+='|';events+=std::to_string(line)+':'+std::string(expression)+':'+std::string(file); }};
        // 매 프레임 Native처럼 감쇠/같은 난수/선택 각도를 진행하며 시각을 따로 적분하지 않는다.
        for (std::uint32_t step=0;step<Number(row[3]);++step) StepAmbientWind(state,random,edition,diagnostics);
        const bool equal=Matches(state,random,row) && checking==Number(row[12]) && (events.empty() ? "-" : events)==row[13];
        if (!equal) std::printf("바람 불일치 %s scene=%s seed=%s steps=%s checking=%s expected=%s,%s,%s,%s actual=%u,%u,%u,%u\n",row[0].c_str(),row[1].c_str(),row[2].c_str(),row[3].c_str(),row[7].c_str(),row[8].c_str(),row[9].c_str(),row[10].c_str(),row[11].c_str(),std::bit_cast<std::uint32_t>(state.x),std::bit_cast<std::uint32_t>(state.y),std::bit_cast<std::uint32_t>(state.angle),random.State());
        CHECK(equal);++count;
    }
    CHECK(count==1920);
}
// 실제 Client Storage/난수 owner는 World 대여와 반납에서 바람/난수를 재생성하지 않는다.
TEST_CASE(AmbientWind_StoragePreservesStateAcrossWorldLeases) {
    using namespace netstorm::client;std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);
    RawSurfaceStorage storage(OriginalEdition::Patch1078,types,frames,shapes);ClientRandomGlobals random;const auto* address=&storage.wind;
    CHECK(std::bit_cast<std::uint32_t>(storage.wind.x)==0 && storage.wind.y==1 && std::bit_cast<std::uint32_t>(storage.wind.angle)==0);
    storage.UpdateAmbientWind(random.Game());CHECK(Matches(storage.wind,random.Game(),InitialSequence(1)));const auto once=random.Game().State();
    { const auto use=storage.Acquire();CHECK(address==&storage.wind && random.Game().State()==once && Matches(storage.wind,random.Game(),InitialSequence(1))); }
    { const auto use=storage.Acquire();CHECK(address==&storage.wind && random.Game().State()==once);storage.UpdateAmbientWind(random.Game());CHECK(Matches(storage.wind,random.Game(),InitialSequence(2))); }
}
