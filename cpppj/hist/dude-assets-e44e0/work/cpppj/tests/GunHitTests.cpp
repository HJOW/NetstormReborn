// 실제 두 PE의 packed 총 시각/DPS/표적 피해 전달을 독립 fixture로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGunHit.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 관찰만 읽고 기대값을 복원 코드에서 만들지 않는다.
const std::vector<std::vector<std::string>>& Rows() { static const auto rows=LoadFixture(NETSTORM_GUNHIT_FIXTURE).rows;return rows; }
// 원본 입력의 현재 풀/타입/시각과 외부 가상 피해 관찰을 보관한다.
struct HitScene {
    // pool: 원본 발사/표적 SID의 실제 슬롯이다.
    SidPool pool;
    // types/hitTypes: 실제 group/genus와 입력된 float DPS다.
    std::vector<RiftTypeRecord> types;
    std::vector<GunHitType> hitTypes;
    // output: native helper의 외부 double 결과 버퍼다.
    std::array<std::uint8_t,8> output{};
    // events: 실제 피해 전달/공간 통지의 인자와 순서다.
    std::vector<std::string> events;
    // patch: 현재 원본 판본별 owner/slot 폭을 구별한다.
    bool patch;
    // now: 현재 원본 double 게임 시각이다.
    double now{};
    // 실제 입력 SID를 미리 확보한다.
    explicit HitScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),hitTypes(types.size()),patch(edition==OriginalEdition::Patch1078) {
        // 같은 고유 입력 번호를 다음 행에서도 유지한다.
        for (std::uint16_t id=5;id<=60;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // free/dead native 입력을 다시 만들 때만 사용하는 검사 전용 raw 접근이다.
    std::span<std::uint8_t> FixtureSlot(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // independent 행의 숫자 입력으로 실제 필드를 준비한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==12);events.clear();now=std::bit_cast<double>(std::stoull(row[3]));
        // helper 결과의 이전 바이트도 native와 같은 패턴으로 유지한다.
        for (std::size_t i=0;i<output.size();++i) output[i]=static_cast<std::uint8_t>(i*7+3);
        // 원본의 raw 발사/표적 padding 전체를 보존한다.
        for (const auto [sid,type]:{std::array<std::uint16_t,2>{50,128},{60,71}}) {
            auto raw=FixtureSlot(Sid{sid});
            // 각 독립 입력은 이전 행의 free 상태도 덮어써 재구성한다.
            for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*11+5);
            Put(raw,0,sid==50 ? 0x11318000U : 0x11318100U);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(sid==60 ? Number(row[6]) : 0);raw[patch ? 34U : 32U]=1;if (sid==50) Put(raw,12,Number(row[2]),2);
        }
        types[128].group=1;types[128].flags2=0;types[71].flags2=Number(row[8]);hitTypes[128].damagePerSecond=std::bit_cast<float>(Number(row[4]));hitTypes[128].specialFlyerDamage=Number(row[7])!=0;hitTypes[128].flyerDamagePerSecond=16;
    }
    // 같은 실제 가상 경계의 인자와 순서를 기록한다.
    GunHitHooks Hooks() {
        GunHitHooks hooks;
        hooks.notifySurface=[this](Sid firing) { events.push_back("N:"+std::to_string(firing.value)); };
        hooks.damage=[this](Sid target,std::int32_t damage,std::uint8_t owner,Sid firing) { events.push_back("D:"+std::to_string(target.value)+':'+std::to_string(damage)+':'+std::to_string(owner)+':'+std::to_string(firing.value)); };
        return hooks;
    }
    // 각 실제 body와 동일한 가상 경계를 실행한다.
    std::vector<std::string> Run(const std::vector<std::string>& row) {
        RawGunHit logic(pool,types,hitTypes,Hooks());
        switch (Number(row[1])) {
        case 0: { const auto bits=std::bit_cast<std::uint64_t>(logic.Time(Sid{50}));Put(output,0,static_cast<std::uint32_t>(bits));Put(output,4,static_cast<std::uint32_t>(bits>>32));break; }
        case 1:case 4:logic.SetTime(Sid{50},now);break;
        case 2:logic.Hit(Sid{50},Sid{static_cast<std::uint16_t>(Number(row[5]))},now);break;
        case 3:logic.Apply(Sid{50},Sid{static_cast<std::uint16_t>(Number(row[5]))},17);break;
        default:throw std::logic_error("GunHit fixture body 오류");
        }
        std::string actions;
        // native 효과 순서를 그대로 직렬화한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        return {actions,Hex(pool.Slot(Sid{50})),Hex(output)};
    }
};
// 각 판본의 모든 고유 native 입력에서 전체 raw/사건/결과 버퍼를 대조한다.
void Replay(const char* edition) {
    HitScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0;
    // 저장된 실제 원본 관찰을 직접 소비한다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;scene.Prepare(row);const auto output=scene.Run(row);const auto expected=std::vector<std::string>(row.begin()+9,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초 차이를 원본 값과 함께 출력한다.
            for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i]) std::printf("  %s Hit 행%zu/body%s 결과%zu 기대%s / 실제%s\n",edition,count,row[1].c_str(),i,expected[i].c_str(),output[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==Rows().size()/2);
}
}
// 패치 실제 DPS getter·30초 제한·Packed WORD·가상 피해 전달을 대조한다.
TEST_CASE(GunHit_PatchNativeTimeAndDamage) { Replay("originals"); }
// CD 실제30초초과 제한·곱float저장과 Packed WORD 쓰기/감김을 대조한다.
TEST_CASE(GunHit_CdNativeTimeAndDamage) { Replay("originalCD"); }
// 실제 피해 처리기 누락을 성공/시각 갱신으로 우회하지 않는다.
TEST_CASE(GunHit_MissingVictimDamageRejected) {
    HitScene scene(OriginalEdition::Patch1078);scene.Prepare(Rows().front());RawGunHit logic(scene.pool,scene.types,scene.hitTypes,{});const auto before=Get(scene.pool.Slot(Sid{50}),12,2);
    CHECK(Throws([&] { logic.Apply(Sid{50},Sid{60},17); }));CHECK(Get(scene.pool.Slot(Sid{50}),12,2)==before && scene.events.empty());
}
