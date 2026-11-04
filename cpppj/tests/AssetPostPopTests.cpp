// 초기 요새 건물의 실제 postPop body와 두 PE가 정한 사건·raw·목록·Player 결과를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawAssetPostPop.h"
#include <cstdio>
#include <filesystem>

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
        hooks.checkForms=[](Sid) { return true; };
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
void Replay(const char* edition,bool sharedAuthority=false,bool oppositeBattle=false) {
    RawAuthorityState authority;AssetScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0; // 같은 owner 주소를 Scene과 모든 모듈보다 오래 보관한다.
    // 새 body의 정상 반환마다 raw·목록·Player·사건을 비교한다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);authority={scene.state.inBattle,scene.state.boss}; // 원본 입력을 현재 owner로 공급한다.
        if (sharedAuthority) { scene.state.sharedAuthority=&authority;scene.state.boss=!authority.boss;if (oppositeBattle) scene.state.inBattle=!authority.server; } // Vortex는 반대 fallback 대신 실제 권한을 소비해야 한다.
        SquidPostPop base(scene.pool,scene.types,scene.baseState);RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));
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

// 동일 독립 Native 결과로 Vortex의 현재 shared 권한과 fallback 격리를 확인한다.
TEST_CASE(AssetPostPop_SharedAuthorityReplaysBothNativeBodies) { Replay("originals",true);Replay("originalCD",true); }

// 전투 표시 상태를 반대로 둬도 같은 현재 서버/권한 owner가 기존 두 판본 원문 출력을 결정한다.
TEST_CASE(AssetPostPop_ServerAuthorityIsIndependentFromBattleView) { Replay("originals",true,true);Replay("originalCD",true,true); }

// 각도 조회가 서버 권한을 바꾸면 Gun 생성 관문은 그 뒤의 현재 owner를 다시 읽는다.
TEST_CASE(AssetPostPop_ServerAuthorityRecheckedAfterFrameLookup) {
    const auto& rows=Rows();const auto found=std::find_if(rows.begin(),rows.end(),[](const auto& row) { return row[0]=="originals" && Number(row[1])==1 && Number(row[2])==2 && Number(row[3])==0 && Number(row[4])==0 && Number(row[5])==0; });CHECK(found!=rows.end());if (found==rows.end()) return;
    RawAuthorityState authority;AssetScene scene(OriginalEdition::Patch1078);scene.Prepare(*found);authority={true,scene.state.boss};scene.state.sharedAuthority=&authority;scene.state.inBattle=true;auto hooks=scene.Hooks();const auto frameLookup=hooks.gunFrame;
    hooks.gunFrame=[&,frameLookup](std::int32_t frame) { authority.server=false;return frameLookup(frame); }; // 원문은 이 두 각도/그래프 자식 호출 뒤540bc0을 읽는다.
    SquidPostPop base(scene.pool,scene.types,scene.baseState);RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));RawAssetPostPop assets(scene.pool,scene.types,base,damageable,scene.state,std::move(hooks));base.SetAssetPostPop(&assets);base.Activate(kSource,2);
    CHECK(!authority.server && scene.state.inBattle);CHECK(scene.Output()==std::vector<std::string>(found->begin()+15,found->end())); // 같은 관문의 기존 server0 원문 출력과 비교하며 새 Native 관측으로 합산하지 않는다.
}

// 네 실제 PostPop의 현재 checking·로딩·편집 상태와 진단 복귀를 독립 원문 1344행으로 비교한다.
TEST_CASE(GunPostPop_NativeDiagnosticOrderAndReturn1344) {
    const auto fixture=LoadFixture((std::filesystem::path(NETSTORM_ASSETPOSTPOP_FIXTURE).parent_path()/"gun-postpop-checking-x86.tsv").string().c_str());
    AssetScene scene(OriginalEdition::Patch1078);RawAuthorityState authority;std::size_t count{};
    // 원문 생성/판정/진단 장치의 입력만 재생하며 발사·실제 게임·통신은 실행하지 않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==9);if (row.size()!=9) throw std::logic_error("Gun PostPop fixture 필드 오류");
        const auto input=Split(row[0],','),control=Split(row[1],',');CHECK(input.size()==14 && control.size()==5);
        std::vector<std::string> legacy(21,"0");legacy[0]="originals";std::copy(input.begin(),input.end(),legacy.begin()+1);scene.Prepare(legacy);
        std::uint32_t checking=Number(control[0]);scene.state.checkingForms.Bind(checking);scene.state.fortMode=Number(control[4])!=0;
        authority.server=Number(input[3])!=0;scene.state.sharedAuthority=&authority;scene.state.inBattle=!authority.server;
        const auto mutation=Number(control[3]);auto hooks=scene.Hooks();const auto create=hooks.createGunProcess,remove=hooks.removeAnimation;
        hooks.createGunProcess=[&,create](Sid sid) {
            if (mutation==1 || mutation==4) { checking=mutation==1 ? 0U : 1U;scene.state.loadingDepth=mutation==1 ? 1U : 0U;scene.state.fortMode=mutation==1; }
            create(sid);
        }; // 실제 new 시점의 전역 변경은 실패 반환에도 남는다.
        hooks.removeAnimation=[&,remove](Sid sid) { remove(sid);if (mutation==5) authority.server=false; }; // 선택한 생성 분기는 서버가 바뀌어도 다시 취소하지 않는다.
        hooks.checkForms=[&](Sid sid) { scene.events.push_back("C:"+std::to_string(sid.value));if (mutation==2) checking=0;return Number(control[1])!=0; };
        hooks.assertion=[&](std::string_view expression,std::string_view file,std::uint32_t line) {
            scene.events.push_back("X:"+Hex({reinterpret_cast<const std::uint8_t*>(expression.data()),expression.size()})+':'+Hex({reinterpret_cast<const std::uint8_t*>(file.data()),file.size()})+':'+std::to_string(line));
            if (!Number(control[2])) throw std::runtime_error("명시 PostPop 진단 중단");
            if (mutation==3) { checking=0;scene.state.loadingDepth=1;scene.state.fortMode=true;Put(scene.pool.AllocatedBytes(kSource),12,0x1234,2);Put(scene.pool.AllocatedBytes(kSource),36,0x6789); }
        }; // 진단 뒤 현재 HP/extra와 전역을 다시 읽는지 전체 raw로 대조한다.
        SquidPostPop base(scene.pool,scene.types,scene.baseState);RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));
        RawAssetPostPop assets(scene.pool,scene.types,base,damageable,scene.state,std::move(hooks));std::string outcome="return";
        assets.Validate(kSource,Number(input[1]));CHECK(scene.events.empty()); // 생성 앞 사전검사에서 원문 진단을 실행하지 않는다.
        try { assets.PostPop(kSource,Number(input[1])); }
        catch (const std::runtime_error& error) { if (std::string_view(error.what())!="명시 PostPop 진단 중단") throw;outcome="assertion"; }
        const auto output=scene.Output();CHECK(output[0]==row[2]);CHECK(output[1]==row[3]);CHECK(outcome==row[4]);
        CHECK(checking==Number(row[5]) && scene.state.loadingDepth==Number(row[6]) && scene.state.fortMode==(Number(row[7])!=0) && authority.server==(Number(row[8])!=0));++count;
    }
    CHECK(count==1344);
}

// 누락된 진단·판정은 생성 뒤 원문 소비 위치에서만 중단하며 checking0은 판정 장치를 요구하지 않는다.
TEST_CASE(GunPostPop_MissingSelectedDiagnosticAfterCreation) {
    AssetScene scene(OriginalEdition::Patch1078);scene.Prepare(Rows().front());Put(scene.pool.AllocatedBytes(kSource),0,kPatchAssetTables[2]);
    scene.state.inBattle=true;std::uint32_t checking=0;scene.state.checkingForms.Bind(checking);
    SquidPostPop base(scene.pool,scene.types,scene.baseState);RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));
    auto hooks=scene.Hooks();hooks.checkForms={};RawAssetPostPop missingGetter(scene.pool,scene.types,base,damageable,scene.state,hooks);
    missingGetter.PostPop(kSource,1);CHECK(scene.events.size()==3 && scene.events.back()=="D:1");
    scene.events.clear();checking=1;missingGetter.Validate(kSource,1);CHECK(scene.events.empty());
    CHECK(Throws([&] { missingGetter.PostPop(kSource,1); }));CHECK(scene.events.size()==2 && scene.events.back()=="P:50");
    scene.events.clear();hooks.checkForms=[](Sid) { return false; };RawAssetPostPop missingAssertion(scene.pool,scene.types,base,damageable,scene.state,hooks);
    CHECK(Throws([&] { missingAssertion.PostPop(kSource,1); }));CHECK(scene.events.size()==2 && scene.events.back()=="P:50");
}

namespace { // 현재 두 부모의 실제 PATCH 진입과 공유 물리 입력을 묶는다.
void PrepareAssetCurrent(AssetScene& scene,std::uint32_t kind,std::uint32_t flags=1) { // 기대값을 만들지 않고 기존 장면에 명시 일반 입력만 공급한다.
    auto row=Rows().front();row[1]=std::to_string(kind);row[2]=std::to_string(flags);row[3]="0";row[4]="0";row[5]="0";row[6]="0";row[7]="7";row[8]="0";row[9]="25";row[10]="0";row[11]="1000";row[12]="0";row[13]="0";row[14]="0";scene.Prepare(row);
}
std::span<std::uint8_t> AssetCurrentSlot(AssetScene& scene,Sid sid=kSource) { const auto raw=scene.pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free 변경 뒤에도 같은 물리 바이트를 관찰한다.
struct AssetCurrentBody { // 같은 장면의 공통·Damageable·가상 분배를 실제 제품 몸체로 구성한다.
    AssetScene& scene; // 모든 처리기보다 오래 사는 입력/관찰 저장소다.
    SquidPostPop base; // 정확히 한 번 감소해야 하는 실제 공통 깊이다.
    RawCarrierPostPop damageable; // 간헐천의 직접 Damageable 부모다.
    RawAssetPostPop assets; // 거주 구역/간헐천의 실제 전체 몸체다.
    AssetCurrentBody(AssetScene& source,AssetPostPopHooks hooks):scene(source),base(scene.pool,scene.types,scene.baseState),damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base)),assets(scene.pool,scene.types,base,damageable,scene.state,std::move(hooks)) { base.SetAssetPostPop(&assets); } // 외부 효과만 입력으로 공급한다.
    void Run(std::uint32_t flags=1) { ++scene.baseState.depth;base.PostPopVirtual(kSource,flags); } // 실제 상위가 증가시킨 깊이와 현재 VT+20을 사용한다.
};
}
TEST_CASE(AssetParentsCurrent_ResidenceAndGeyserNative376Rows) { // 두 판본의 해당 두 원문 몸체만 전체 출력과 대조하며 다른 파생/공중/MP를 실행하지 않는다.
    std::size_t count{};
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 판본별 기존 입력·기대값을 그대로 읽는다.
        AssetScene scene(edition);
        for (const auto& row:Rows()) { // 거주 구역6/간헐천8 이외의 몸체는 선택하지 않는다.
            if (row[0]!=(scene.patch ? "originals" : "originalCD") || (row[1]!="6" && row[1]!="8")) continue;
            scene.Prepare(row);SquidPostPop base(scene.pool,scene.types,scene.baseState);RawCarrierPostPop damageable(scene.pool,scene.types,scene.carrierState,scene.CarrierHooks(base));RawAssetPostPop assets(scene.pool,scene.types,base,damageable,scene.state,scene.Hooks());base.SetAssetPostPop(&assets);
            if (scene.patch) { ++scene.baseState.depth;base.PostPopVirtual(kSource,Number(row[2])); }else base.Activate(kSource,Number(row[2]));
            const auto output=scene.Output();const auto expected=std::vector<std::string>(row.begin()+15,row.end());CHECK(output==expected && scene.baseState.depth==0);++count;
        }
    }
    CHECK(count==376);
}
TEST_CASE(AssetParentsCurrent_NoFirstFlagSkipsUnboundFrontDevices) { // 두 실제 부모 모두 최초가 아니면 TYPE/목록/예약 전수 검사를 추가하지 않는다.
    for (const auto kind:{6U,8U}) { // 간헐천은 뒤 Damageable가 flags0을 읽는 것만 관측한다.
        AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,kind,0);AssetCurrentSlot(scene)[10]=255;scene.state.temples.count=99;AssetCurrentBody body(scene,{});body.Run(0);CHECK(scene.baseState.depth==0 && scene.state.residences.count==0 && scene.state.geysers.count==0);
        CHECK(scene.events==(kind==6 ? std::vector<std::string>{} : std::vector<std::string>{"D:0"}));
    }
}
TEST_CASE(AssetParentsCurrent_MissingSurfaceStopsBeforeListAndRegular) { // 첫 거주 구역의 선택된 지면 장치가 없으면 앞선 쓰기는 없다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);auto hooks=scene.Hooks();hooks.surface={};AssetCurrentBody body(scene,std::move(hooks));CHECK(Throws([&] { body.Run(); }) && scene.state.residences.count==0 && scene.allocated==0 && scene.baseState.depth==1 && scene.events.empty());
}
TEST_CASE(AssetParentsCurrent_OnlyNonzeroSurfaceOwnerRequiresSetter) { // 지면 없음/owner0은 소유자 장치를 선택하지 않고 owner2만 그 위치에서 중단한다.
    for (const auto owner:{0U,2U}) { // 두 독립 입력의 부분 결과를 섞지 않는다.
        AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);AssetCurrentSlot(scene,Sid{60})[34]=static_cast<std::uint8_t>(owner);auto hooks=scene.Hooks();hooks.setOwner={};AssetCurrentBody body(scene,std::move(hooks));
        if (owner) CHECK(Throws([&] { body.Run(); }) && scene.state.residences.count==0 && scene.allocated==0 && scene.baseState.depth==1);
        else { body.Run();CHECK(scene.state.residences.count==1 && scene.allocated==3 && scene.baseState.depth==0); }
    }
}
TEST_CASE(AssetParentsCurrent_FailedAllocationsSkipMissingConstructorAndExtraGate) { // 거주 구역은 extra9여도 최초 앞단을 실행하며 세 확보 실패는 생성자 누락을 요구하지 않는다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);scene.reserveMask=0;AssetCurrentSlot(scene)[40]=9;scene.state.temples.count=99;auto hooks=scene.Hooks();hooks.surface=[](float,float) { return Sid{}; };hooks.setOwner={};hooks.addRegular={};AssetCurrentBody body(scene,std::move(hooks));body.Run();
    CHECK(scene.events==(std::vector<std::string>{"A:40","A:40","A:40","N:50"}) && scene.state.residences.count==1 && scene.baseState.depth==0 && Get(scene.pool.Slot(kSource),42)==25 && scene.pool.Slot(kSource)[12]==0xc0);
}
TEST_CASE(AssetParentsCurrent_LateNotifyFailureRetainsListReservationsAndNibble) { // 지면 통지가 없을 때 앞선 목록/예약/하위 nibble 쓰기를 보존하고 잔여 프레임/공통은 미도달이다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);auto hooks=scene.Hooks();hooks.surface=[](float,float) { return Sid{}; };hooks.notifySurface={};AssetCurrentBody body(scene,std::move(hooks));CHECK(Throws([&] { body.Run(); }));CHECK(scene.state.residences.count==1 && scene.allocated==3 && scene.events.size()==6 && scene.pool.Slot(kSource)[12]==0xc0 && Get(scene.pool.Slot(kSource),42)==0 && scene.baseState.depth==1);
}
TEST_CASE(AssetParentsCurrent_NotifyChangesCurrentFrameNibbleAndAnimation) { // 통지 뒤 현재 프레임·nibble·애니메이션 플래그로 잔여 값을 계산한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);scene.reserveMask=0;auto hooks=scene.Hooks();hooks.surface=[](float,float) { return Sid{}; };hooks.notifySurface=[&](Sid sid) { auto raw=AssetCurrentSlot(scene,sid);raw[12]|=5;Put(raw,36,17);scene.state.residenceAnimation=true; };AssetCurrentBody body(scene,std::move(hooks));body.Run();CHECK(Get(scene.pool.Slot(kSource),42)==3 && Get(scene.pool.Slot(kSource),36)==17 && scene.baseState.depth==0);
}
TEST_CASE(AssetParentsCurrent_OwnerCallbackKeepsPhysicalFreeContainedSlot) { // 소유자 콜백이 free/contained와 TYPE를 바꿔도 같은 물리 슬롯의 원문 쓰기를 유지한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);scene.reserveMask=0;auto hooks=scene.Hooks();hooks.setOwner=[&](Sid sid,auto owner) { auto raw=AssetCurrentSlot(scene,sid);raw[34]=static_cast<std::uint8_t>(owner);raw[11]=9;raw[10]=255; };AssetCurrentBody body(scene,std::move(hooks));body.Run();CHECK(scene.pool.Slot(kSource)[11]==9 && scene.pool.Slot(kSource)[10]==255 && scene.pool.Slot(kSource)[12]==0xc0 && Get(scene.pool.Slot(kSource),42)==25 && scene.baseState.depth==0);
}
TEST_CASE(AssetParentsCurrent_SignedFullListSkipsUnneededStorage) { // signed count>=capacity면 과거 배열 포인터와 쓰기 장치를 해석하지 않는다.
    for (const auto pair:{std::array<std::uint32_t,2>{7,4},{0,0xffffffffU},{0xffffffffU,0xfffffffeU}}) { // 양수 초과와 두 음수 용량 비교를 실제 jge에 맞춰 공급한다.
        AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);scene.reserveMask=0;SquidPostPopList current;current.BindCurrent({[]()->std::span<std::uint32_t> { throw std::logic_error("가득 찬 목록의 미도달 저장소"); },[&] { return pair[1]; },[&] { return pair[0]; },[](auto) { throw std::logic_error("가득 찬 목록의 미도달 쓰기"); }});scene.state.sharedResidences=&current;
        auto hooks=scene.Hooks();hooks.surface=[](float,float) { return Sid{}; };AssetCurrentBody body(scene,std::move(hooks));body.Run();CHECK(scene.allocated==3 && scene.baseState.depth==0);
    }
}
TEST_CASE(AssetParentsCurrent_NegativeStoreRejectsOnlyAtPhysicalWrite) { // signed 음수 위치가 실제 저장을 선택하면 앞선 지면 효과 뒤 호스트 물리 범위 오류로 중단한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);SquidPostPopList current;current.BindCurrent({[]()->std::span<std::uint32_t> { throw std::logic_error("음수 위치의 미도달 저장소"); },[] { return 4U; },[] { return 0xffffffffU; },[](auto) { throw std::logic_error("미도달 쓰기"); }});scene.state.sharedResidences=&current;auto hooks=scene.Hooks();hooks.surface=[&](float,float) { scene.events.push_back("S");return Sid{}; };AssetCurrentBody body(scene,std::move(hooks));CHECK(Throws([&] { body.Run(); }) && scene.events==std::vector<std::string>{"S"} && scene.allocated==0 && scene.baseState.depth==1);
}
TEST_CASE(AssetParentsCurrent_BoundListStoresBeforeIncrementAndKeepsDuplicates) { // 같은 현재 배열/count를 쓰며 중복 SID와 비활성 꼬리를 보존한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,6);scene.reserveMask=0;std::array<std::uint32_t,3> values{7,8,9};std::uint32_t count{};SquidPostPopList current;current.BindCurrent({[&] { return std::span(values); },[] { return 3U; },[&] { return count; },[&](auto value) { CHECK(values[count]==50);count=value; }});scene.state.sharedResidences=&current;auto hooks=scene.Hooks();hooks.surface=[](float,float) { return Sid{}; };AssetCurrentBody body(scene,std::move(hooks));body.Run();body.Run();CHECK(count==2 && values==(std::array<std::uint32_t,3>{50,50,9}) && scene.state.residences.count==0 && scene.baseState.depth==0);
}
TEST_CASE(AssetParentsCurrent_MissingGeyserRegistrationRetainsList) { // 실제 등록 장치 앞의 간헐천 목록 추가를 사전 누락 검사로 지우지 않는다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);auto hooks=scene.Hooks();hooks.geyserRegistration={};AssetCurrentBody body(scene,std::move(hooks));CHECK(Throws([&] { body.Run(); }) && scene.state.geysers.count==1 && scene.state.geysers.entries[0]==50 && scene.allocated==0 && scene.baseState.depth==1);
}
TEST_CASE(AssetParentsCurrent_GeyserExtraGateSkipsUnboundFront) { // extra9는 목록/등록/예약/이웃을 생략하고 실제 Damageable만 호출한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);AssetCurrentSlot(scene)[40]=9;AssetCurrentBody body(scene,{});body.Run();CHECK(scene.events==std::vector<std::string>{"D:1"} && scene.state.geysers.count==0 && scene.baseState.depth==0);
}
TEST_CASE(AssetParentsCurrent_GeyserRegistrationChangesCurrentTypeCost) { // 등록 콜백 뒤 현재 TYPE의 비용을 WORD로 저장하고 확보 실패 뒤에도 이웃/공통으로 진행한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);scene.reserveMask=0;auto hooks=scene.Hooks();hooks.geyserRegistration=[&](Sid sid,auto value) { CHECK(value==1);scene.events.push_back("G");AssetCurrentSlot(scene,sid)[10]=75;scene.types[75].cost=513.875F; };hooks.addRegular={};hooks.atPosition=[&](float,float) { scene.events.push_back("P");return std::vector<Sid>{}; };hooks.unpop={};hooks.repop={};AssetCurrentBody body(scene,std::move(hooks));body.Run();CHECK(Get(scene.pool.Slot(kSource),12,2)==513 && scene.events==(std::vector<std::string>{"G","A:40","P","D:1"}) && scene.baseState.depth==0);
}
TEST_CASE(AssetParentsCurrent_GeyserMissingRepopRetainsUnpopAndFrame) { // 현재 이웃 Unpop이 free로 바꾼 뒤에도 프레임8은 저장되며 미연결 Repop 위치에서 중단한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);auto hooks=scene.Hooks();hooks.unpop=[&](Sid sid,auto flags) { CHECK(sid==Sid{60} && flags==0);scene.events.push_back("U");AssetCurrentSlot(scene,sid)[11]=1; };hooks.repop={};AssetCurrentBody body(scene,std::move(hooks));CHECK(Throws([&] { body.Run(); }));CHECK(scene.state.geysers.count==1 && Get(scene.pool.Slot(kSource),12,2)==1000 && scene.pool.Slot(Sid{60})[11]==1 && Get(scene.pool.Slot(Sid{60}),36)==8 && Get(scene.pool.Slot(Sid{61}),36)==0 && scene.events.back()=="U" && scene.baseState.depth==1);
}
TEST_CASE(AssetParentsCurrent_GeyserBadCurrentTypeFollowsRegistration) { // 현재 TYPE 오류는 목록/등록 뒤 실제 비용 조회에서만 거부한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);auto hooks=scene.Hooks();hooks.geyserRegistration=[&](Sid sid,auto) { scene.events.push_back("G");AssetCurrentSlot(scene,sid)[10]=255; };AssetCurrentBody body(scene,std::move(hooks));CHECK(Throws([&] { body.Run(); }) && scene.state.geysers.count==1 && scene.events==std::vector<std::string>{"G"} && scene.allocated==0 && scene.baseState.depth==1);
}
TEST_CASE(AssetParentsCurrent_GeyserNoIslandSkipsUnboundSpaceDevices) { // 현재 후보가 받침 TYPE가 아니면 Unpop/Repop 장치를 요구하지 않는다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);AssetCurrentSlot(scene,Sid{60})[10]=75;auto hooks=scene.Hooks();hooks.atPosition=[](float,float) { return std::vector<Sid>{Sid{60}}; };hooks.unpop={};hooks.repop={};AssetCurrentBody body(scene,std::move(hooks));body.Run();CHECK(scene.baseState.depth==0 && scene.events.back()=="D:1");
}
TEST_CASE(AssetParentsCurrent_GeyserLiveFinderFollowsEffectsBetweenCandidates) { // 실제 일반 finder는 첫 효과가 바꾼 다음 후보의 링크를 이후 Next에서 읽는다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);SquidHash hash;hash.Cell(1,25,26)=60;
    for (const auto sid:{Sid{60},Sid{61},Sid{62},Sid{63}}) { // 입력 체인은60→61→62이며 같은 위치/발자국의63은 아직 체인 밖이다.
        auto raw=AssetCurrentSlot(scene,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=sid.value==60 ? 94 : 95;Put(raw,4,sid.value==60 ? 61U : sid.value==61 ? 62U : 0U,2);Put(raw,14,std::bit_cast<std::uint32_t>(50.0F));Put(raw,18,std::bit_cast<std::uint32_t>(52.0F));
    }
    scene.types[94].footX=scene.types[94].footY=scene.types[95].footX=scene.types[95].footY=1;auto hooks=scene.Hooks();hooks.visitAtPosition=MakeAssetPostPopFinder(scene.pool,hash,scene.types);hooks.atPosition={};
    hooks.unpop=[&](Sid sid,auto) { scene.events.push_back("U"+std::to_string(sid.value));if (sid.value==60) Put(AssetCurrentSlot(scene,Sid{61}),4,63,2); };hooks.repop=[&](Sid sid,auto flags) { CHECK(flags==0x40 && Get(scene.pool.Slot(sid),36)==8);scene.events.push_back("V"+std::to_string(sid.value)); };
    AssetCurrentBody body(scene,std::move(hooks));body.assets.Validate(kSource,1);CHECK(scene.events.empty());body.Run();CHECK(scene.events==(std::vector<std::string>{"G:50:1","A:40","R:50:0:1036831949","U60","V60","U61","V61","U63","V63","D:1"}) && Get(scene.pool.Slot(Sid{62}),36)==0 && scene.baseState.depth==0);
}
TEST_CASE(AssetParentsCurrent_GeyserWordSetterNotifiesCurrentBridgeBeforeRegular) { // 현재 다리 번호와 같으면 WORD 저장 뒤 통지를 실행하고 그 콜백 뒤 새 TYPE/좌표로 진행한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);std::uint32_t bridge=70;scene.state.currentBridgeType=&bridge;auto hooks=scene.Hooks();
    hooks.notifySurface=[&](Sid sid) { CHECK(Get(scene.pool.Slot(sid),12,2)==1000);scene.events.push_back("H");AssetCurrentSlot(scene,sid)[10]=75;Put(AssetCurrentSlot(scene,sid),14,std::bit_cast<std::uint32_t>(40.0F));scene.types[75].cost=12; };
    hooks.atPosition=[&](float x,float y) { CHECK(x==40.0F && y==52.0F);scene.events.push_back("P");return std::vector<Sid>{}; };AssetCurrentBody body(scene,std::move(hooks));body.Run();CHECK(scene.events==(std::vector<std::string>{"G:50:1","H","A:40","R:50:0:1036831949","P","D:1"}) && Get(scene.pool.Slot(kSource),12,2)==1000 && scene.baseState.depth==0);
}
TEST_CASE(AssetParentsCurrent_GeyserMissingBridgeNoticeRetainsWord) { // 선택된 다리 통지가 없으면 이미 끝난 목록/등록/WORD를 보존하고 예약 전에 중단한다.
    AssetScene scene(OriginalEdition::Patch1078);PrepareAssetCurrent(scene,8);scene.state.bridgeType=70;auto hooks=scene.Hooks();hooks.notifySurface={};AssetCurrentBody body(scene,std::move(hooks));CHECK(Throws([&] { body.Run(); }) && scene.state.geysers.count==1 && Get(scene.pool.Slot(kSource),12,2)==1000 && scene.events==std::vector<std::string>{"G:50:1"} && scene.allocated==0 && scene.baseState.depth==1);
}
