// 사제 소유자 재정의를 실제 세 PE의 독립 관찰과 공통 소유자/생성자 통합으로 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestOwner.h"
#include "o/SquidFactory.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 저장된 판본별 관찰 행 수다. 패치의 범위 밖 요청과 CD의 0 처리 차이로 수가 다르다.
constexpr std::size_t kPatchRows=3528,kCdRows=1944;
// 원본 PE/Python 없이 fixture만 읽어 사용한다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_PRIESTOWNER_FIXTURE);return data.rows;
}
struct PriestScene {
    SidPool pool;
    SquidOwnerMode mode;
    PriestOwnerState state;
    SquidPostPopState bookkeeping;
    std::vector<RiftTypeRecord> types;
    std::unique_ptr<SquidOwner> base;
    std::vector<std::string> events;
    bool patch;
    // 관찰 SID 50을 준비하고 실제 공통 소유자 지정에 사제 genus를 공급한다.
    explicit PriestScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        patch(edition==OriginalEdition::Patch1078) {
        // client 영역의 앞 번호를 차례대로 확보한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
        types[kPriestType].flags2=0x00210000;base=std::make_unique<SquidOwner>(pool,types,bookkeeping,mode);
    }
    // 판본별 현재 소유자·extra 위치다.
    std::size_t OwnerOffset() const { return patch ? 34 : 32; }
    std::size_t ExtraOffset() const { return patch ? 40 : 35; }
    // 입력 행을 raw 슬롯/모드에 넣는다. 기대 결과나 호출 여부는 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        events.clear();mode.fort=Number(row[2])!=0;mode.battle=Number(row[3])!=0;mode.challenge=false;
        state.loadingDepth=Number(row[4]);state.fortPlayer=Number(row[5]);state.battlePlayer=Number(row[6]);
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[11]=static_cast<std::uint8_t>(Number(row[10]));
        Put(raw,12,Number(row[7]),2);raw[OwnerOffset()]=static_cast<std::uint8_t>(Number(row[8]));
        raw[ExtraOffset()]=static_cast<std::uint8_t>(Number(row[11]));
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
    }
    // 원본 진입에서 관찰한 단어/현재 소유자 순서로 필드를 기록한다.
    std::string Fields(Sid sid) const {
        const auto raw=pool.Slot(sid);return std::to_string(Get(raw,12,2))+':'+std::to_string(raw[OwnerOffset()]);
    }
    // 공통 몸체는 실제 SquidOwner::Set을 사용하고 무상태 알림만 호출 당시 필드를 기록한다.
    PriestOwnerHooks Hooks() {
        return {
            [this](Sid sid,std::uint32_t player) {
                events.push_back("B:"+std::to_string(sid.value)+':'+std::to_string(player)+':'+Fields(sid));base->Set(sid,player);
            },
            [this](Sid sid) { events.push_back("N:"+std::to_string(sid.value)+':'+Fields(sid)); }
        };
    }
    // 진입 사건의 순서를 fixture와 같은 문자열로 만든다.
    std::string Events() const {
        std::string result;
        // 사건 사이만 세미콜론을 넣는다.
        for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }
        return result.empty() ? "-" : result;
    }
};
// 판본마다 자신의 PE에서 얻은 사건/슬롯 전체와 C++을 비교한다.
void Replay(const char* edition) {
    const bool patch=std::string_view(edition)=="originals";PriestScene scene(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    RawPriestOwner priest(scene.pool,scene.mode,scene.state,scene.Hooks());std::size_t count=0;int failures=0;
    CHECK(Fixture().size()==kPatchRows+2*kCdRows);
    // 모드·로딩·요청·원래 단어·현재 소유자·상태/extra의 모든 관찰을 재생한다.
    for (const auto& row:Fixture()) {
        if (row.at(1)!=edition) continue;
        CHECK(row.size()==14 && row[0]=="Owner");scene.Prepare(row);priest.Set(kSource,Number(row[9]));
        const bool same=scene.Events()==row[12] && Hex(scene.pool.Slot(kSource))==row[13];CHECK(same);
        if (!same) {
            std::printf("  %s 사제 행 %zu: 기대 %s / 실제 %s\n",edition,count,row[12].c_str(),scene.Events().c_str());
            if (++failures>=6) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==(patch ? kPatchRows : kCdRows));
    CHECK(scene.bookkeeping.providers.count==0 && scene.bookkeeping.factories.count==0 && scene.bookkeeping.totalCost==0);
}
}
// 패치의 범위 밖 요청 시 공통 지정 생략도 실제 관찰과 비교한다.
TEST_CASE(priest_owner_patch_x86_full_body) { Replay("originals"); }
// CD의 요청 0/word 폭 차이를 별도 관찰로 비교한다.
TEST_CASE(priest_owner_cd_x86_full_body) { Replay("originalCD"); }
// 추가 10.37은 별도 PE SHA를 가진 실행 결과다.
TEST_CASE(priest_owner_1037_x86_full_body) { Replay("original1037"); }

// missing 효과와 원본 assert에 해당하는 요청/상태는 효과 전에 거부한다.
TEST_CASE(priest_owner_rejects_missing_hooks_and_assert_inputs_before_mutation) {
    // 두 판본의 서로 다른 원본 assert 경로를 나누어 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestScene scene(edition);
        const std::vector<std::string> row{"Owner","", "1","0","1","3","1","43781","2","3","0","0","-","-"};
        scene.Prepare(row);
        CHECK(Throws([&] { RawPriestOwner invalid(scene.pool,scene.mode,scene.state,{}); }));
        auto hooks=scene.Hooks();hooks.notify={};CHECK(Throws([&] { RawPriestOwner invalid(scene.pool,scene.mode,scene.state,hooks); }));
        RawPriestOwner priest(scene.pool,scene.mode,scene.state,scene.Hooks());
        // 실패가 전체 raw 단어/소유자와 호출 기록을 보존하는지 확인한다.
        const auto reject=[&](Sid sid,std::uint32_t player) {
            const auto before=Hex(scene.pool.Slot(kSource));
            CHECK(Throws([&] { priest.Set(sid,player); }));CHECK(scene.events.empty() && Hex(scene.pool.Slot(kSource))==before);
        };
        reject(kSource,scene.patch ? 0 : 9);reject(Sid{0},3);reject(Sid{51},3);
        scene.pool.AllocatedBytes(kSource)[10]=82;reject(kSource,3);scene.pool.AllocatedBytes(kSource)[10]=kPriestType;
        Put(scene.pool.AllocatedBytes(kSource),0,kVtable);reject(kSource,3);
        Put(scene.pool.AllocatedBytes(kSource),0,scene.patch ? kPatchPriestVtable : kCdPriestVtable);
        // challenge의 공통 지정은 0을 거부한다. 전투 요청은 무시되어도 유효 소유자 검사는 적용한다.
        scene.mode.battle=true;scene.mode.challenge=true;scene.state.loadingDepth=0;Put(scene.pool.AllocatedBytes(kSource),12,0,2);reject(kSource,3);
        CHECK(Throws([&] { auto invalid=MakePriestOwnerDispatch(priest,{}); }));
    }
}

// 실제 사제 생성자·공통 소유자와 순차 로딩/전투 상태 전환을 연결한다. Pop/postPop은 실행하지 않는다.
TEST_CASE(priest_owner_real_factory_load_and_battle_ownership_sequence) {
    // 두 판본의 실제 생성자 기록값을 모두 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);SquidPostPopState bookkeeping;SquidOwnerMode mode;mode.battle=true;
        PriestOwnerState state{1,3,1};std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
        types[kPriestType].constructorAddress=TypeConstructorAddress(edition,kPriestType);types[kPriestType].flags2=0x00210000;
        SquidFactory factory(pool,types);SquidOwner base(pool,types,bookkeeping,mode);const auto sid=factory.Create(kPriestType);
        // 현재 소유자 바이트는 프레임/extra와 분리돼 있다.
        const std::size_t ownerOffset=edition==OriginalEdition::Patch1078 ? 34 : 32;
        Put(pool.AllocatedBytes(sid),12,0xab00,2);std::vector<std::string> order;
        RawPriestOwner priest(pool,mode,state,{
            [&](Sid object,std::uint32_t player) { order.push_back("base");base.Set(object,player); },
            [&](Sid object) { CHECK(object==sid);order.push_back("notify"); }
        });
        CHECK(priest.Handles(sid));priest.Set(sid,3);
        CHECK(pool.Slot(sid)[ownerOffset]==3 && Get(pool.Slot(sid),12,2)==0xab03);
        CHECK(order==std::vector<std::string>({"notify","base","notify"}));
        // 로딩이 끝난 전투에서는 요청값 1을 무시하고 원래 소유자 3을 유지한다.
        state.loadingDepth=0;order.clear();priest.Set(sid,1);
        CHECK(pool.Slot(sid)[ownerOffset]==3 && Get(pool.Slot(sid),12,2)==0xab03 && order==std::vector<std::string>({"base","notify"}));
        // 현재 전투 플레이어가 원래 소유자일 때만 표식 비트 7을 켠다.
        state.battlePlayer=3;order.clear();priest.Set(sid,0xffffffffU);
        CHECK(pool.Slot(sid)[ownerOffset]==3 && Get(pool.Slot(sid),12,2)==0xab83);
        CHECK(order==std::vector<std::string>({"base","notify"}));
        // 로딩 중 비전투는 원래 소유자만 바꾸고 표식 비트는 그대로 둔다.
        mode.battle=false;mode.fort=true;state.loadingDepth=1;order.clear();priest.Set(sid,1);
        CHECK(pool.Slot(sid)[ownerOffset]==1 && Get(pool.Slot(sid),12,2)==0xab81 && order==std::vector<std::string>({"notify","base"}));
        state.loadingDepth=0;order.clear();priest.Set(sid,3);
        CHECK(pool.Slot(sid)[ownerOffset]==3 && Get(pool.Slot(sid),12,2)==0xab83);
        CHECK(order==std::vector<std::string>({"notify","base","notify"}) && pool.FreeCount()==kCapacity-6);
    }
}

// 기존 섬/종유석 분배기와 사제 분배기를 합성해 각 전용 경로와 공통 fallback을 분리한다.
TEST_CASE(priest_owner_dispatch_composes_with_island_and_common_owners) {
    PriestScene scene(OriginalEdition::Patch1078);BridgeConnectState links;links.battle=true;
    const std::vector<std::string> row{"Owner","", "0","1","0","3","1","43779","2","1","0","0","-","-"};
    scene.Prepare(row);RawPriestOwner priest(scene.pool,scene.mode,scene.state,scene.Hooks());
    const auto base=[&](Sid sid,std::uint32_t owner) { scene.base->Set(sid,owner); };
    const auto dispatch=MakeOwnerDispatch(scene.pool,links,MakePriestOwnerDispatch(priest,base));
    dispatch(kSource,1);CHECK(scene.pool.Slot(kSource)[scene.OwnerOffset()]==3 && scene.events.size()==2);
    // 공통 가상 표는 원래 단어를 바꾸지 않고 요청한 소유자를 그대로 받는다.
    scene.events.clear();Put(scene.pool.AllocatedBytes(kSource),0,kVtable);dispatch(kSource,1);
    CHECK(scene.pool.Slot(kSource)[scene.OwnerOffset()]==1 && Get(scene.pool.Slot(kSource),12,2)==0xab03 && scene.events.empty());
    // 섬 전용 가상 표는 원래 단어를 보존하고 소유자 색 프레임을 바꾼다.
    Put(scene.pool.AllocatedBytes(kSource),0,kPatchIslandVtable);dispatch(kSource,3);
    CHECK(scene.pool.Slot(kSource)[scene.OwnerOffset()]==3 && Get(scene.pool.Slot(kSource),36)==2 && Get(scene.pool.Slot(kSource),12,2)==0xab03);
}
