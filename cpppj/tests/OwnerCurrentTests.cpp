// 공통 소유자의 현재 TYPE/모드와 진단·실제 목록 경계를 확인하며 사제/MP는 실행하지 않는다.
#include "TestSupport.h"
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "o/SquidOwner.h"
#include "o/SquidFactory.h"
#include "o/RawCommonOwner.h"
#include "client/RawSurfaceWorld.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

TEST_CASE(OwnerCurrent_IndependentNative72CurrentValues) { // 구현 전에 고정한 두 PE의 현재값/진단/부분 중단과 전체 슬롯을 대조한다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_OWNER_CURRENT_FIXTURE).rows) { // battle0/challenge0만 실행하며 목록 자식은 명시 장치다.
        CHECK(row.size()==6);if (row.size()!=6) continue;const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");const auto args=Split(row[1],',');CHECK(args.size()==3);std::uint32_t fort=Number(args[0]),genus=0x4000;const auto player=Number(args[1]),profile=Number(args[2]);SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);const auto slot=pool.Slot(Sid{5});std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(slot.data()),slot.size()};
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*29+7); // Native와 같은 입력 payload이며 기대값은 계산하지 않는다.
        raw[10]=85;raw[11]=0;raw[patch ? 34:32]=3;raw[patch ? 40:35]=0;std::string events,cause="returned";const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 선택된 실제 호출 순서다.
        CommonOwnerHooks hooks;hooks.fort=[&] { return fort!=0; };hooks.battle=[] { return false; };hooks.challenge=[] { return false; };hooks.genus=[&](std::uint8_t type) { event("G"+std::to_string(type));return genus; };
        hooks.assertion=[&](auto expression,auto file,auto line) { event("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line));if (profile==1) { fort=1;raw[10]=86;genus=0; } };
        hooks.remove=[&](std::uint8_t owner,Sid sid) { event("M"+std::to_string(owner)+','+std::to_string(sid.value));if (profile==2) { raw[10]=86;genus=0; }if (profile==3) fort=0;if (profile==4) throw std::string("remove"); };
        hooks.append=[&](std::uint8_t owner,Sid sid) { event("A"+std::to_string(owner)+','+std::to_string(sid.value));if (profile==5) throw std::string("append"); };
        try { SetCommonSquidOwner(pool,Sid{5},player,hooks); }catch (const std::string& stop) { cause=stop; }
        CHECK(cause==row[2] && (events.empty()?"-":events)==row[3] && std::to_string(fort)==row[4]);CHECK(Hex(pool.Slot(Sid{5}))==row[5]);++count;
    }
    CHECK(count==72);
}

TEST_CASE(OwnerCurrent_RemovalRechecksCurrentMode) { // 실제 목록 개수 쓰기가 바꾼 모드를 소유자 BYTE 뒤에 다시 판정한다.
    SidPool pool(OriginalEdition::Patch1078,32768);auto types=netstorm::test::MakeFormTypeInputs(188);types[85].flags2=0x4000;SquidFactory factory(pool,types);const auto sid=factory.Create(85,2);pool.AllocatedBytes(sid)[34]=3;pool.AllocatedBytes(sid)[11]=0;SquidPostPopState book;SquidOwnerMode mode{true,false,false};SquidOwner owner(pool,types,book,mode);auto& previous=book.ownerFactories[3];previous.entries={sid.value,77,sid.value};previous.count=3;book.ownerFactories[4].entries.resize(3);
    previous.current={ [&] { return std::span(previous.entries); },[&] { return static_cast<std::uint32_t>(previous.entries.size()); },[&] { return previous.count; },[&](std::uint32_t value) { previous.count=value;mode.fort=false; } }; // 현재 Player 개수 쓰기의 실제 콜백 수명이다.
    owner.Set(sid,4);CHECK(previous.Count()==1 && previous.Storage()[0]==77 && book.ownerFactories[4].Count()==0 && pool.Slot(sid)[34]==4 && !mode.fort);
}

TEST_CASE(OwnerCurrent_InactiveModeDoesNotReadType) { // 편집/전투가 모두0이면 원문은 범위 밖 TYPE도 읽지 않고 소유자 BYTE만 바꾼다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 일반 비전투 입력만 사용한다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);const auto sid=factory.Create(85,2);auto raw=pool.AllocatedBytes(sid);raw[10]=255;const auto offset=edition==OriginalEdition::Patch1078 ? 34U : 32U;raw[offset]=3;SquidPostPopState book;SquidOwnerMode mode;SquidOwner owner(pool,types,book,mode);bool failed=false;try { owner.Set(sid,2); }catch (const std::exception&) { failed=true; }CHECK(!failed && raw[10]==255 && raw[offset]==2);
    }
}

TEST_CASE(OwnerCurrent_ActualWorldReadsSameLiveTypes) { // 실제 World의 생성 뒤 바뀐 genus를 공통 Owner도 같은 표에서 읽는다.
    auto types=netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr);BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();core.ownerMode={true,false,false};storage->types[85].flags2=0x4000;const auto sid=core.factory.Create(85,2);core.pool.AllocatedBytes(sid)[11]=0;auto& list=core.bookkeeping.ownerFactories[3];list.current={};list.entries.resize(3);list.count=0;
    core.owner.Set(sid,3);CHECK(list.Count()==1 && list.Storage()[0]==sid.value && core.pool.Slot(sid)[34]==3);
}

TEST_CASE(OwnerCurrent_ActualWorldDiagnosticAndLatePhysicalListBoundary) { // 범위 밖 번호의 진단은 정상 복귀할 수 있고 선택한 목록만 물리 범위를 요구한다.
    for (const bool listed:{false,true}) { // challenge0의 무효9를 사용하며 MP 번호 검사는 실행하지 않는다.
        auto types=netstorm::test::MakeFormTypeInputs(188);types[85].flags2=listed ? 0x4000U : 0U;std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr);std::optional<std::string> detail;SidPool* pool{};Sid sid{};std::size_t diagnostics{};
        storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(pool && file=="\\Ns\\O\\Squid.cpp" && line==0xa02 && expression=="newPlayerId == INVALID_PLAYER_ID ||( FIRST_REAL_PLAYER <= newPlayerId && newPlayerId < MAX_PLAYERS)");++diagnostics;if (!listed) pool->AllocatedBytes(sid)[10]=255; });
        BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();pool=&core.pool;core.ownerMode={listed,false,false};sid=core.factory.Create(85,2);core.pool.AllocatedBytes(sid)[34]=3;core.pool.AllocatedBytes(sid)[11]=0;auto& previous=core.bookkeeping.ownerFactories[3];previous.current={};previous.entries={sid.value};previous.count=1;bool failed=false;try { core.owner.Set(sid,9); }catch (const std::out_of_range&) { failed=true; }catch (const std::exception&) { failed=true; }
        CHECK(failed==listed && diagnostics==1 && core.pool.Slot(sid)[34]==9 && previous.Count()==(listed ? 0U : 1U));pool=nullptr;
    }
}
