// 파생 소유자의 원문128행과 실제 생성자/World 생산 장부를 대조한다. 게임/MP는 실행하지 않는다.
#include "TestSupport.h"
#include "DudeSpawnSupport.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawAssetOwner.h"

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

// 원본 wrapper의 전체 payload/감김/공통 부모 중단 순서를 최초 PE 관측과 비교한다.
TEST_CASE(AssetOwnerCurrent_IndependentNative128LocalInputs) {
    std::size_t count{}; // 실제로 대조한 고유 관측 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_ASSET_OWNER_CURRENT_FIXTURE).rows) { // 두 판본의 생산 접두/중립 몸체만 재생한다.
        CHECK(row.size()==6);if (row.size()!=6) continue;
        const bool patch=row[0]=="originals";const auto args=Split(row[1],','); // 입력을 읽으며 기대 결과는 계산하지 않는다.
        CHECK(args.size()==5);if (args.size()!=5) continue;
        const auto kind=Number(args[0]),extra=Number(args[1]),owner=Number(args[2]);auto dirty=Number(args[3]);const bool stop=Number(args[4])!=0;
        SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);
        const auto slot=pool.Slot(Sid{5});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()}; // 원문은 free/dead를 선행 검사하지 않는다.
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*29+7); // Native의 명시 입력 payload만 만든다.
        raw[patch ? 34U : 32U]=3;raw[patch ? 40U : 35U]=static_cast<std::uint8_t>(extra);
        // 실제 VT 분배를 위해 원문 입력의 첫DWORD만 기록한다. 결과 비교 때 같은 입력 DWORD를 복원한다.
        const auto originalTable=Get(raw,0);Put(raw,0,kind ? (patch ? 0x50a8e8U : 0x500b10U) : (patch ? 0x509148U : 0x503298U));
        std::string events,cause="returned";BridgeConnectState state; // 외부 공통 부모와 최종 원문 상태의 관측이다.
        const auto dispatch=MakeAssetOwnerDispatch(pool,state,dirty,[&](Sid sid,std::uint32_t player) {
            CHECK(sid==Sid{5});events="B"+std::to_string(player)+','+std::to_string(dirty);
            if (stop) throw std::string("base");raw[patch ? 34U : 32U]=static_cast<std::uint8_t>(player);
        }); // 공통 부모의 명시 BYTE 변경/중단은 Native 장치와 동일한 입력이다.
        try { dispatch(Sid{5},owner); }catch (const std::string& value) { cause=value; }
        Put(raw,0,originalTable);CHECK(cause==row[2] && (events.empty()?"-":events)==row[3] && std::to_string(dirty)==row[4] && Hex(raw)==row[5]);++count;
    }
    CHECK(count==128);
}

// 실제 보호 생성자가 쓰는 두 Factory VT·젬·섬 받침/종유석을 같은 World 분배기에 연결한다.
TEST_CASE(AssetOwnerCurrent_ActualWorldConstructorsAndProductionStorage) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 공중 유닛/통신/포획을 생성하거나 실행하지 않는다.
        const auto& assets=netstorm::test::dudespawn::Assets(edition);BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;links.battle=true;links.ownerColors[2]=6;
        RawSurfaceWorld world(edition,assets.types,assets.frames,assets.shapes,links,97);auto core=world.Core();const bool patch=edition==OriginalEdition::Patch1078;
        core.ownerMode={false,false,false};core.bridgeState.mission=false;core.bridgeState.battle=true;core.bridgeState.ownerColors[2]=6;
        for (const auto type:{85U,126U,121U,94U,95U}) { // 실제 두 Factory VT·젬·받침·종유석 생성자를 선택한다.
            const auto sid=world.Create(type,2);auto raw=core.pool.AllocatedBytes(sid);const auto table=Get(raw,0);core.bookkeeping.productionDirty=0xffffffffU;
            world.SetAssetOwner(sid,2);CHECK(raw[patch ? 34U : 32U]==(type==121 ? 0 : 2));CHECK(Get(raw,0)==table);
            CHECK(core.bookkeeping.productionDirty==((type==85 || type==126) ? 0U : 0xffffffffU));
            if (type==94 || type==95) CHECK(Get(raw,patch ? 36U : 34U,patch ? 4U : 1U)==5U);
        }
        world.ClearProcesses();
    }
}

// 생산 접두는 현재 VT/extra를 매호출 읽고 공통 부모 실패 뒤에도 앞선 증가를 유지한다.
TEST_CASE(AssetOwnerCurrent_LiveTableAndFailureKeepPrefix) {
    SidPool pool(OriginalEdition::Patch1078,32768);const auto slot=pool.Slot(Sid{5});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()};
    BridgeConnectState state;std::uint32_t dirty=7,calls{}; // 같은 분배기가 빌리는 현재 카운터와 관측 수다.
    const auto dispatch=MakeAssetOwnerDispatch(pool,state,dirty,[&](Sid,std::uint32_t) { ++calls;throw std::string("base"); });
    Put(raw,0,0x509148);raw[40]=0;try { dispatch(Sid{5},2); }catch (const std::string&) {}CHECK(dirty==8 && calls==1);
    raw[40]=8;try { dispatch(Sid{5},2); }catch (const std::string&) {}CHECK(dirty==8 && calls==2);
    Put(raw,0,0x50a8e8);raw[34]=3;dispatch(Sid{5},0xffffffffU);CHECK(dirty==8 && calls==2 && raw[34]==0);
}
