// 초기 요새 건물의 실제 postPop body와 두 PE가 정한 사건·raw·목록·Player 결과를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawAssetPostPop.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 +0x20이 각 테스트 body로 연결되는 가상 표다. 명령 실행용 주소가 아니다.
constexpr std::array<std::uint32_t,12> kPatchAssetTables{0x501ea0,0x50b6e8,0x50b7c0,0x50b9b8,0x50b888,0x50b570,0x50f540,0x50e940,0x50da88,0x5196f0,0x519860,0x519918};
constexpr std::array<std::uint32_t,12> kCdAssetTables{0x5022f8,0x5011e8,0x5012b0,0x501120,0x5014f8,0x501440,0x501b90,0x506168,0x500760,0x501d28,0x501de0,0x501e98};
// 기존 기대값 재계산 없이 저장된 원본 입력/관찰을 읽는다.
const std::vector<std::vector<std::string>>& Rows() { static const auto rows=LoadFixture(NETSTORM_ASSETPOSTPOP_FIXTURE).rows;return rows; }
// 실제 C++ body와 같은 외부 효과를 합성하는 검사 월드다. 판단은 fixture의 native 결과가 정한다.
struct AssetScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<RiftTypeRecord> types;
    // baseState: 기본 상태. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidPostPopState baseState;
    // carrierState: 캐리어 상태. 입력 구성 또는 실행 결과 대조에 사용한다.
    CarrierPostPopState carrierState;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    AssetPostPopState state;
    // events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::string> events;
    // patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool patch{};
    // allocated: 할당된. 입력 구성 또는 실행 결과 대조에 사용한다.
    // reserveMask: 예약 마스크. 입력 구성 또는 실행 결과 대조에 사용한다.
    // contentExists: 내용 존재. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint32_t allocated{},reserveMask{},contentExists{};
    // 기계어의 대상/지면/종유석/선택 form 번호를 client 구역에 확보한다.
    explicit AssetScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 원본 fixture의 모든 관찰 번호를 확보한다.
        for (std::uint16_t id=5;id<=127;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 판본별 raw 필드 배치를 반환한다.
    std::size_t Owner() const { return patch ? 34U : 32U; }
    std::size_t Extra() const { return patch ? 40U : 35U; }
    std::size_t Word() const { return patch ? 36U : 34U; }
    // 새 입력만 준비하며 body 결과는 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==21);events.clear();allocated=0;state={};baseState={};carrierState={};
        baseState.suppressed=true;carrierState.boss=true;
        state.inBattle=Number(row[4])!=0;state.loadingDepth=Number(row[5]);state.fortMode=state.loadingDepth!=0;
        state.boss=Number(row[6])!=0;reserveMask=Number(row[7]);state.residenceAnimation=Number(row[8])!=0;
        state.templeCount=4;state.selected=Number(row[12]) ? 62U : 0U;contentExists=Number(row[13]);
        // 목록은 capacity 밖의 과거 값도 유지한다.
        for (auto* list:{&state.residences,&state.temples,&state.geysers}) { list->entries={7,8,9,10};list->count=Number(row[14]); }
        types[70].flags1=Number(row[10]);types[70].flags2=0x200;types[70].cost=std::stof(row[11]);
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,(patch ? kPatchAssetTables : kCdAssetTables)[Number(row[1])]);raw[10]=70;raw[11]=0;raw[Owner()]=1;raw[Extra()]=static_cast<std::uint8_t>(Number(row[3]));
        Put(raw,12,0xabcd,2);Put(raw,14,0x42480000);Put(raw,18,0x42500000);Put(raw,Word(),Number(row[9]),patch ? 4U : 1U);
        // native 공간 iterator가 차례로 반환하는 두 실제 지면 번호다.
        for (const auto sid:{Sid{60},Sid{61}}) {
            auto neighbor=pool.AllocatedBytes(sid);std::fill(neighbor.begin(),neighbor.end(),std::uint8_t{});neighbor[10]=static_cast<std::uint8_t>(sid.value==60 ? 94 : 95);neighbor[Owner()]=2;
        }
        if (state.boss) { auto other=pool.AllocatedBytes(Sid{61});other[10]=70;other[Extra()]=static_cast<std::uint8_t>(contentExists*8); }
        auto selected=pool.AllocatedBytes(Sid{62});std::fill(selected.begin(),selected.end(),std::uint8_t{});Put(selected,18,70,2);
    }
    // allocator의 각 호출별 성공 비트다. 반환값 계산은 입력의 확보 여부만 표현한다.
    bool Allocate(std::uint32_t size) { events.push_back("A:"+std::to_string(size));return (reserveMask&(1U<<(allocated++%3)))!=0; }
    // 직접 Damageable body의 명시 base 경계를 원본과 같은 사건으로 관찰한다.
    CarrierPostPopHooks CarrierHooks(SquidPostPop& base) {
        return {
            [](Sid) { throw std::logic_error("이번 자산body는 Carrier 확인을 호출하지 않습니다"); },
            [](float,float,std::uint8_t) { throw std::logic_error("이번 관찰은 Damageable를 외부 경계로 둡니다"); },
            [&base](Sid sid,std::uint32_t flags) { base.ValidateBase(sid,flags); },
            [this,&base](Sid sid,std::uint32_t flags) { events.push_back("D:"+std::to_string(flags));base.PostPopBase(sid,flags); }
        };
    }
    // native가 대체한 효과와 같은 인자/호출 규약으로 사건과 raw 변경을 공급한다.
    AssetPostPopHooks Hooks() {
        AssetPostPopHooks hooks;
        hooks.reserveRegular=[this] { return Allocate(40); };
        hooks.addRegular=[this](Sid sid,std::uint32_t event,float delay) { events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(delay))); };
        hooks.createGunProcess=[this](Sid sid) { if (Allocate(56)) events.push_back("P:"+std::to_string(sid.value)); };
        hooks.removeAnimation=[this](Sid sid) { events.push_back("M:"+std::to_string(sid.value)); };
        hooks.checkForms=[](Sid) {};
        hooks.gunFrame=[this](std::int32_t angle) { events.push_back("F:"+std::to_string(angle));return 2; };
        hooks.setGraph=[this](Sid sid,std::int32_t graph) { events.push_back("Q:"+std::to_string(graph));pool.AllocatedBytes(sid)[patch ? 30 : 28]=static_cast<std::uint8_t>(graph); };
        hooks.surface=[](float,float) { return Sid{60}; };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) { events.push_back("O:"+std::to_string(sid.value)+':'+std::to_string(owner));pool.AllocatedBytes(sid)[Owner()]=static_cast<std::uint8_t>(owner); };
        hooks.notifySurface=[this](Sid sid) { events.push_back("N:"+std::to_string(sid.value)); };
        hooks.startBlocker=[this](Sid sid) { events.push_back("K:"+std::to_string(sid.value)); };
        hooks.geyserRegistration=[this](Sid sid,std::uint32_t value) { events.push_back("G:"+std::to_string(sid.value)+':'+std::to_string(value)); };
        hooks.atPosition=[](float,float) { return std::vector<Sid>{Sid{60},Sid{61}}; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { events.push_back("V:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.typeDword=[](Sid,std::uint32_t offset) { return offset==0x74 ? 17U : offset==0x130 ? 27U : 39U; };
        hooks.playImpactSound=[this](std::uint32_t value) { events.push_back("E:"+std::to_string(value)); };
        hooks.clearPlacement=[this] { events.push_back("C"); };
        hooks.islandNumber=[](float,float) { return 3; };
        hooks.islandExists=[](std::int32_t) { return true; };
        hooks.islandTemple=[](std::int32_t) { return Sid{}; };
        hooks.registerTemple=[this](std::int32_t island,Sid sid) { events.push_back("I:"+std::to_string(island)+':'+std::to_string(sid.value)); };
        hooks.regionNumber=[](float,float) { return 3; };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("X:"+std::to_string(sid.value)+':'+std::to_string(flags));pool.AllocatedBytes(sid)[11]|=2; };
        hooks.hasContent=[this](Sid,std::uint32_t) { return contentExists!=0; };
        hooks.createSlot=[this](Sid sid,std::uint32_t) { events.push_back("S:"+std::to_string(sid.value)); };
        hooks.refreshProduction=[this] { events.push_back("T"); };
        return hooks;
    }
    // 현재 관찰 결과를 기계어 fixture 형식으로 직렬화한다.
    std::vector<std::string> Output() const {
        std::string actions,lists;
        // 사건 순서 전체를 보존한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        // capacity까지 포함해 stale 목록 메모리도 대조한다.
        for (const auto* list:{&state.residences,&state.temples,&state.geysers}) {
            if (!lists.empty()) lists+='|';lists+=std::to_string(list->count)+':';
            // 남아 있는 네 슬롯을 모두 기록한다.
            for (std::size_t i=0;i<list->entries.size();++i) { if (i) lists+=',';lists+=std::to_string(list->entries[i]); }
        }
        const auto& player=state.players[1];const auto players=std::to_string(player.initialTemple)+','+std::to_string(player.initialTempleCopy)+','+std::to_string(player.newTemple)+','+std::to_string(player.newTempleCopy);
        const auto neighbors=std::to_string(Get(pool.Slot(Sid{60}),Word(),patch ? 4U : 1U))+'|'+std::to_string(Get(pool.Slot(Sid{61}),Word(),patch ? 4U : 1U));
        return {actions,Hex(pool.Slot(kSource)),lists,players,std::to_string(state.templeCount),neighbors};
    }
};
// 현재 C++ 경계와 원본 PE 결과를 각 body별로 독립 비교한다.
void Replay(const char* edition) {
    AssetScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0;
    // 새 body의 정상 반환마다 raw·목록·Player·사건을 비교한다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);SquidPostPop base(scene.pool,scene.types,scene.baseState);RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));
        RawAssetPostPop assets(scene.pool,scene.types,base,damageable,scene.state,scene.Hooks());base.SetAssetPostPop(&assets);
        CHECK(base.HandlesAsset(kSource));base.Activate(kSource,Number(row[2]));CHECK(scene.baseState.depth==0);
        const auto output=scene.Output();const auto expected=std::vector<std::string>(row.begin()+15,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 첫 실패의 관찰 차이를 출력해 fixture 기대값을 구현 추측으로 덮지 않는다.
            for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i]) std::printf("  %s asset 행 %zu/body%s 결과%zu 기대 %s / 실제 %s\n",edition,count,row[1].c_str(),i,expected[i].c_str(),output[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==Rows().size()/2);
}
}
// 패치 10.78의 모든 새 전용 몸체를 비교한다.
TEST_CASE(AssetPostPop_PatchNativeBodies) { Replay("originals"); }
// CD 10.72의 저장 폭·예약·직접 목록 분기를 비교한다.
TEST_CASE(AssetPostPop_CdNativeBodies) { Replay("originalCD"); }
// 직접 Damageable/Carrier 지원은 등록을 요구하며 모르는 파생 가상 표는 계속 거부한다.
TEST_CASE(AssetPostPop_DirectDamageableCarrierRegistration) {
    // 두 PE의 실제 직접 postPop 가상 주소만 호출한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        AssetScene scene(edition);scene.Prepare(Rows().front());SquidPostPop base(scene.pool,scene.types,scene.baseState);
        auto hooks=scene.CarrierHooks(base);std::uint32_t checks=0;scene.carrierState.boss=false;hooks.carrierCheck=[&checks](Sid) { ++checks; };
        RawCarrierPostPop carrier(scene.pool,scene.types,scene.carrierState,hooks);base.SetCarrierPostPop(&carrier);
        Put(scene.pool.AllocatedBytes(kSource),0,scene.patch ? 0x50a820U : 0x500a58U);CHECK(base.HandlesDamageable(kSource));base.Activate(kSource,0);CHECK(checks==0 && scene.baseState.depth==0);
        Put(scene.pool.AllocatedBytes(kSource),0,scene.patch ? 0x503d80U : 0x506ea0U);CHECK(base.HandlesCarrier(kSource));base.Activate(kSource,0);CHECK(checks==1 && scene.baseState.depth==0);
        base.SetCarrierPostPop(nullptr);CHECK(!base.HandlesCarrier(kSource));CHECK(Throws([&] { base.Activate(kSource,0); }));
        Put(scene.pool.AllocatedBytes(kSource),0,0x11111111U);CHECK(!base.HandlesDerived(kSource));CHECK(Throws([&] { base.Activate(kSource,0); }));
    }
}
// 외부 효과 누락은 최초 raw/목록/장부 쓰기 전에 거부한다.
TEST_CASE(AssetPostPop_MissingEffectsBeforeMutation) {
    AssetScene scene(OriginalEdition::Patch1078);scene.Prepare(Rows().front());Put(scene.pool.AllocatedBytes(kSource),0,kPatchAssetTables[9]);
    SquidPostPop base(scene.pool,scene.types,scene.baseState);RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));
    RawAssetPostPop assets(scene.pool,scene.types,base,damageable,scene.state,{});base.SetAssetPostPop(&assets);const auto before=scene.Output();
    CHECK(Throws([&] { base.Activate(kSource,1); }));CHECK(scene.Output()==before && scene.baseState.depth==0);
}
namespace {
// 기존 독립 PE 기대값을 공유 Player BYTE에 그대로 재생하며 기존 state.players 사본과 구별한다.
void ReplayPlayerRecords(const char* edition) {
    AssetScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    const auto stride=scene.patch ? 0xb4U : 0xacU; // 원본 Player 물리기록 크기다.
    const auto temple=scene.patch ? 0x7cU : 0x74U; // 기존 PE fixture 추출기가 확인한 첫 신전 DWORD 위치다.
    std::size_t count=0; // 해당 판본에서 원본 기대값과 대조한 입력 수다.
    // 원본 입력과 기대값은 바꾸지 않고 모든 자산 body를 같은9기록에 다시 재생한다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);scene.state.players[1]={901,902,903,904};
        std::vector<std::uint8_t> records(9*stride,0xa5); // 신전 이외의 필드 쓰기를 검출하는 실제 공유9기록이다.
        // 아홉 기록의 신전 네DWORD만 원본 입력과 같은0으로 준비한다.
        for (std::size_t owner=0;owner<9;++owner) std::fill_n(records.begin()+owner*stride+temple,16,std::uint8_t{});
        auto expectedRecords=records; // 기존 원본 출력의 신전 필드만 적용할 전체 BYTE 기대값이다.
        std::uint32_t owner=1; // 캠페인의 현재 battleOwner DWORD와 같은 장기 전역이다.
        SquidPostPop base(scene.pool,scene.types,scene.baseState);
        RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));
        RawAssetPostPop assets(scene.pool,scene.types,base,damageable,scene.state,scene.Hooks());
        assets.BindPlayerRecords(records,&owner);base.SetAssetPostPop(&assets);base.Activate(kSource,Number(row[2]));
        auto output=scene.Output();output[3].clear(); // 현재 raw 기록을 원본 fixture의 네DWORD 출력 형식으로 읽는다.
        const auto fields=Split(row[18],','); // 구현 계산이 아닌 기존 PE 출력의 네 신전 값이다.
        // 원본 출력대로 네DWORD만 기대기록에 쓰고 실제 BYTE 결과를 직렬화한다.
        for (std::size_t field=0;field<4;++field) {
            Put(expectedRecords,stride+temple+field*4,Number(fields[field]));
            if (field) output[3]+=',';
            output[3]+=std::to_string(Get(records,stride+temple+field*4));
        }
        CHECK(output==std::vector<std::string>(row.begin()+15,row.end()));CHECK(records==expectedRecords);
        CHECK(scene.state.players[1].initialTemple==901 && scene.state.players[1].initialTempleCopy==902 &&
            scene.state.players[1].newTemple==903 && scene.state.players[1].newTempleCopy==904 && scene.baseState.depth==0);
        ++count;
    }
    CHECK(count==Rows().size()/2);
}
}
// 패치의 동일 원본 입력을 실제9 Player에 직접 기록하며 stale 사본을 소비하지 않는다.
TEST_CASE(AssetPostPop_PatchSharedPlayerRecordsNativeBodies) { ReplayPlayerRecords("originals"); }
// CD의 다른 stride/신전DWORD 위치와 원본 몸체 전체를 같은 BYTE 저장소로 검증한다.
TEST_CASE(AssetPostPop_CdSharedPlayerRecordsNativeBodies) { ReplayPlayerRecords("originalCD"); }
// 잘못된9기록 바인딩은 기존 참조를 바꾸거나 Player 바이트를 쓰기 전에 거부한다.
TEST_CASE(AssetPostPop_PlayerRecordBindingRejectsWrongSize) {
    AssetScene scene(OriginalEdition::Patch1078);scene.Prepare(Rows().front());
    SquidPostPop base(scene.pool,scene.types,scene.baseState);
    RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));
    RawAssetPostPop assets(scene.pool,scene.types,base,damageable,scene.state,scene.Hooks());
    std::vector<std::uint8_t> records(9*0xb4U,0xa5); // 정상 연결을 유지하는 공유 물리기록이다.
    assets.BindPlayerRecords(records);const auto before=records; // 실패 전 전체 BYTE의 기준이다.
    CHECK(Throws([&] { assets.BindPlayerRecords(std::span<std::uint8_t>(records).first(records.size()-1)); }));
    CHECK(records==before && Throws([&] { assets.BindPlayerRecords({}); }));
}
namespace {
// 새 독립 PE 입력의 현재owner·음향callback 뒤 신전DWORD를 같은 공유기록에 재생한다.
void ReplayMutablePlayers(const char* edition) {
    const auto fixture=LoadFixture(NETSTORM_ASSETPLAYER_FIXTURE); // 구현과 독립된 실제 PE의 전체9기록 출력이다.
    AssetScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    const auto stride=scene.patch ? 0xb4U : 0xacU; // 실제 Player 한 기록의 크기다.
    const auto temple=scene.patch ? 0x7cU : 0x74U; // 첫 신전DWORD의 판본별 위치다.
    std::size_t count=0; // 해당 판본에서 대조한 새 고유 입력 수다.
    // 현재raw/전역을 변경하는 외부 경계 뒤의 실제 원본 반환을 전수 비교한다.
    for (const auto& row:fixture.rows) {
        if (row[0]!=edition) continue;
        CHECK(row.size()==17);scene.Prepare(Rows().front());scene.state.inBattle=true;
        scene.state.loadingDepth=1;scene.state.fortMode=true;scene.state.selected=62;scene.reserveMask=0;
        auto raw=scene.pool.AllocatedBytes(kSource); // 현재 원본 신전 객체의 입력 바이트다.
        Put(raw,0,(scene.patch ? kPatchAssetTables : kCdAssetTables)[Number(row[1])]);
        raw[scene.Owner()]=static_cast<std::uint8_t>(Number(row[4]));raw[scene.Extra()]=static_cast<std::uint8_t>(Number(row[3]));
        std::vector<std::uint8_t> records(9*stride,0xa5); // 새 Native 입력과 같은 stale BYTE다.
        // 실제 PE에 공급한 네DWORD 입력을 아홉 기록에 각각 준비한다.
        for (std::size_t owner=0;owner<9;++owner) {
            Put(records,owner*stride+temple,Number(row[8]));Put(records,owner*stride+temple+4,0x11112222);
            Put(records,owner*stride+temple+8,0x33334444);Put(records,owner*stride+temple+12,0x55556666);
        }
        std::uint32_t local=Number(row[6]); // 매 효과 뒤 재조회할 실제battleOwner DWORD다.
        SquidPostPop base(scene.pool,scene.types,scene.baseState);
        auto carrierHooks=scene.CarrierHooks(base);const auto common=carrierHooks.base; // 기존 독립 Damageable 경계다.
        carrierHooks.base=[&](Sid sid,std::uint32_t flags) { scene.pool.AllocatedBytes(sid)[scene.Owner()]=static_cast<std::uint8_t>(Number(row[5]));common(sid,flags); };
        RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,std::move(carrierHooks));
        auto hooks=scene.Hooks();const auto sound=hooks.playImpactSound; // Native에서 같은 외부 음향효과로 지정한 경계다.
        hooks.playImpactSound=[&](std::uint32_t token) { local=Number(row[7]);const auto value=std::stoi(row[9]);if (value>=0) Put(records,Number(row[5])*stride+temple,static_cast<std::uint32_t>(value));sound(token); };
        RawAssetPostPop assets(scene.pool,scene.types,base,damageable,scene.state,std::move(hooks));
        assets.BindPlayerRecords(records,&local);base.SetAssetPostPop(&assets);base.Activate(kSource,Number(row[2]));
        auto output=scene.Output();output[3]=Hex(records); // 전체 Player 바이트를 원본 반환과 직접 대조한다.
        CHECK(output==std::vector<std::string>(row.begin()+10,row.begin()+16));CHECK(local==Number(row[16]));
        ++count;
    }
    CHECK(count==fixture.rows.size()/2);
}
}
// 패치의 callback 뒤 owner 변경과 현재 신전DWORD를 실제 PE 전체바이트로 비교한다.
TEST_CASE(AssetPostPop_PatchMutablePlayerNativeRecords) { ReplayMutablePlayers("originals"); }
// CD의 다른 기록배치·원소wrapper·abstract/buried를 같은 독립 입력으로 비교한다.
TEST_CASE(AssetPostPop_CdMutablePlayerNativeRecords) { ReplayMutablePlayers("originalCD"); }
