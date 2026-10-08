// 원본 세 PE의 실제 종속 조회와 Carrier 전체 호출 결과를 재생한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierPreDestroy.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 세 PE 각각의 직접 조회/Carrier 관찰 수와 전체 raw 비교 슬롯이다.
constexpr std::size_t kRowsPerEdition=960;
constexpr std::array<Sid,5> kIds{{{50},{60},{61},{62},{63}}};
// 원본/Python 없이 저장된 독립 기대 출력을 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_CARRIERPREDESTROY_FIXTURE);return data.rows; }
struct CarrierScene {
    SidPool pool;
    CarrierPreDestroyState state;
    std::array<std::span<std::uint8_t>,5> slots;
    std::string events,after;
    int validations{};
    // free 상태 후보도 관찰하므로 준비 때 확보한 raw 범위를 이후 입력에 재사용한다.
    explicit CarrierScene(OriginalEdition edition):pool(edition,kCapacity,false) {
        // 번호는 기존 client FIFO 할당으로 확보한다.
        for (std::uint16_t id=5;id<=63;++id) CHECK(pool.Allocate(2)==Sid{id});
        // 입력 쓰기는 이후 합성 free 비트의 검사 API를 다시 호출하지 않는다.
        for (std::size_t i=0;i<kIds.size();++i) slots[i]=pool.AllocatedBytes(kIds[i]);
    }
    // 원본에서 대체한 두 하위 몸체만 같은 사건과 입력 변경으로 제공한다.
    CarrierPreDestroyHooks Hooks() {
        return {[this](Sid,std::uint32_t) { ++validations; },[this](Sid sid,std::uint32_t flags) {
            events="D:"+std::to_string(sid.value)+':'+std::to_string(flags)+':'+std::to_string(state.boss ? 1 : 0);
            if (after!="-") {
                const auto values=Split(after,',');state.boss=Number(values[0])!=0;
                Put(slots[0],6,Number(values[1]),2);Put(slots[4],20,Number(values[2]),2);
            }
        },[this] { events+=";G"; }};
    }
    // 입력 칸을 raw/state에 넣고 기대 분기나 반환을 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==16);events.clear();validations=0;after=row[9];state.boss=Number(row[3])!=0;state.containedType=Number(row[4]);
        // 지정하지 않은 바이트도 실행기와 같은 0xab로 채운다.
        for (auto raw:slots) { std::fill(raw.begin(),raw.end(),std::uint8_t{0xab});raw[10]=158;raw[11]=0;Put(raw,4,0); }
        slots[0][11]=static_cast<std::uint8_t>(Number(row[2]));Put(slots[0],6,Number(row[10]),2);
        // 각 종속은 번호·타입·상태·next·kind·mask 순서다.
        for (const auto& node:Split(row[11],';')) {
            if (node.empty()) continue;
            const auto values=Split(node,',');const auto sid=Number(values[0]);
            auto raw=slots.at(static_cast<std::size_t>(sid-60+1));raw[10]=static_cast<std::uint8_t>(Number(values[1]));raw[11]=static_cast<std::uint8_t>(Number(values[2]));
            Put(raw,4,Number(values[3]),2);Put(raw,18,Number(values[4]),2);Put(raw,20,Number(values[5]),2);
        }
    }
    // 현재 슬롯 전체를 실행기의 비교 순서로 직렬화한다.
    std::string Slots() const {
        std::string result;
        // raw 변화가 없는 후보도 전체 바이트를 비교한다.
        for (auto sid:kIds) { if (!result.empty()) result+='|';result+=Hex(pool.Slot(sid)); }
        return result;
    }
};
// 직접 조회와 Damageable 이후 조회를 각 PE의 독립 관찰에 대조한다.
void Replay(std::string_view edition) {
    CarrierScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==3*kRowsPerEdition);
    // 두 API는 같은 raw 배치를 사용하지만 기대값은 각각 실제 PE에서 얻었다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);RawCarrierPreDestroy carrier(scene.pool,scene.state,scene.Hooks());
        if (row[1]=="Q") CHECK(carrier.FindContained(kSource,Number(row[5]),Number(row[6]),Number(row[7])).value==Number(row[12]));
        else carrier.PreDestroy(kSource,Number(row[8]));
        const bool same=scene.events==row[14] && scene.Slots()==row[15] && scene.state.boss==(Number(row[13])!=0);
        CHECK(same && scene.validations==(row[1]=="P" ? 1 : 0));
        if (!same) { std::printf("Carrier %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[14].c_str(),scene.events.c_str());break; }
        ++count;
    }
    CHECK(count==kRowsPerEdition);
}
}
// 패치판은 실제 contained finder의 기본 true 가상 필터까지 실행한 기대값이다.
TEST_CASE(carrier_predestroy_patch_x86) { Replay("originals"); }
// CD의 별도 나눗셈/인라인 finder 경로도 같은 C++ raw 배치로 대조한다.
TEST_CASE(carrier_predestroy_cd_x86) { Replay("originalCD"); }
// 추가 10.37 실제 PE 자료를 독립적으로 재생한다.
TEST_CASE(carrier_predestroy_1037_x86) { Replay("original1037"); }

// 하위 경계 누락·잘못된 자산과 손상 체인·원본 debug assert는 명시적으로 거부한다.
TEST_CASE(carrier_predestroy_guards_and_priest_binding) {
    // 패치 전용 debug 조건과 CD의 조회 허용 차이를 함께 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        CarrierScene scene(edition);scene.Prepare(Fixture().front());auto hooks=scene.Hooks();hooks.dependentFound={};
        CHECK(Throws([&] { RawCarrierPreDestroy missing(scene.pool,scene.state,hooks); }));
        RawCarrierPreDestroy carrier(scene.pool,scene.state,scene.Hooks());
        const auto reject=[&](Sid sid) { const auto before=scene.Slots();CHECK(Throws([&] { carrier.PreDestroy(sid,0); }));CHECK(scene.events.empty() && scene.Slots()==before); };
        reject({0});reject({1});reject({24000});reject({65535});
        scene.slots[0][11]=1;reject(kSource);scene.slots[0][11]=8;reject(kSource);scene.slots[0][11]=2;
        scene.slots[0][10]=6;reject(kSource);scene.slots[0][10]=255;reject(kSource);scene.slots[0][10]=158;
        hooks=scene.Hooks();hooks.validateDamageable=[](Sid,std::uint32_t) { throw std::logic_error("하위 효과 미복원"); };
        RawCarrierPreDestroy blocked(scene.pool,scene.state,hooks);const auto before=scene.Slots();CHECK(Throws([&] { blocked.PreDestroy(kSource,1); }));CHECK(scene.events.empty() && scene.Slots()==before);
        CHECK(Throws([&] { carrier.FindContained({0},1,0,0); }));
        Put(scene.slots[0],6,60,2);scene.slots[1][10]=6;Put(scene.slots[1],20,1,2);Put(scene.slots[1],4,60,2);
        CHECK(carrier.FindContained(kSource,1,0,0)==Sid{60});CHECK(Throws([&] { carrier.FindContained(kSource,1,0,1); }));
        Put(scene.slots[1],4,65535,2);CHECK(Throws([&] { carrier.FindContained(kSource,1,0,1); }));Put(scene.slots[1],4,0,2);
        scene.state.boss=false;scene.state.checkingDead=true;
        if (edition==OriginalEdition::Patch1078) CHECK(Throws([&] { carrier.FindContained(kSource,1,0,0); }));
        else CHECK(carrier.FindContained(kSource,1,0,0)==Sid{60});
        scene.state.checkingDead=false;
        SidPool other(edition,kCapacity,false);CHECK(Throws([&] { MakeCarrierPreDestroyHooks(other,carrier,{}); }));
        int protection=0;PriestPreDestroyHooks priestHooks;priestHooks.findForcefield=[&](Sid) { ++protection;return Sid{}; };
        const auto bound=MakeCarrierPreDestroyHooks(scene.pool,carrier,priestHooks);CHECK(bound.findForcefield(kSource)==Sid{} && protection==1);
        bound.validateCarrier(kSource,3);bound.carrierPre(kSource,3);CHECK(scene.events=="D:50:3:0");
    }
}
