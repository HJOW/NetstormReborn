// 원문 부모와 실제 공장 연결만 창 없이 대조한다. 게임/의식/공중3종/MP 실행은 하지 않는다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "RawSceneSupport.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawSquidCreation.h"
#include "o/SquidFactory.h"
#include "o/SquidForms.h"
#include <memory>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 명시 장치 입력을 원문과 같은 물리 토큰으로 조립한다.
constexpr std::uint32_t kParentPool=0x13000000,kParentVt=0x14000000,kParentCtor=0x10001000; // 격리 원문 관측기의 입력 주소이며 호스트 포인터가 아니다.
template<class F> bool CreationFails(F&& operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } // 선택된 미연결 장치에서 멈췄는지만 관찰한다.
std::span<std::uint8_t> Physical(SidPool& pool,Sid sid) { return {const_cast<std::uint8_t*>(pool.Bytes().data())+sid.value*pool.Layout().stride,pool.Layout().stride}; } // 검사 입력을 free/예약 슬롯에도 직접 설치한다.
}

TEST_CASE(SquidCreationParent_Native588BothEditionsAndReturnedObjectOrder) { // 부모의 독립 원문 추적/반환/전체 바이트를 대조한다.
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_SQUID_CREATION_PARENT_FIXTURE).rows) { // signed SID/전체DWORD TYPE/진단/재조회 입력을 그대로 읽는다.
        CHECK(row.size()==5);const auto input=Split(row[1],',');CHECK(input.size()==5);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        const bool patch=edition==OriginalEdition::Patch1078;const std::size_t stride=patch ? 50 : 36;
        const auto type=Number(input[0]),flags=Number(input[1]),sid=Number(input[2]),profile=Number(input[3]);const bool base=Number(input[4])!=0;
        const std::uint32_t baseVt=patch ? 0x501dd0 : 0x5058d8;std::uint32_t constructor=base ? 0 : kParentCtor;
        std::vector<std::uint8_t> bytes(128*stride);std::string trace;
        for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(index*37+19); // 원문 실행 전의 독립 sentinel 입력이다.
        const auto raw=[&](std::uint32_t object) { CHECK(object>=kParentPool && object<kParentPool+bytes.size());return std::span(bytes).subspan(object-kParentPool,stride); }; // 객체 토큰을 검사 소유 버퍼에만 해석한다.
        const auto emit=[&](std::string event) { if (!trace.empty()) trace+=';';trace+=event; }; // 원문 관측과 같은 경계 순서를 기록한다.
        RawSquidCreationHooks hooks;
        hooks.fakeSurfaceType=[] { return 162U; };
        hooks.assertion=[&](auto expression,auto file,auto line) { emit("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line)); };
        hooks.allocate=[&](auto value) { emit("A"+std::to_string(value));if (profile==1) constructor=kParentCtor+0x10;return sid; };
        hooks.constructor=[&](auto number) { emit("T"+std::to_string(number)+':'+std::to_string(constructor));return constructor; };
        hooks.construct=[&](auto address,auto number) { // 명시 자식 반환 경계이며 실제 자산 생성자는 기존 fixture가 별도로 검증한다.
            if (!address) { const auto object=static_cast<std::uint32_t>(kParentPool+number*stride);Put(raw(object),0,baseVt);emit("B"+std::to_string(object)+':'+std::to_string(baseVt));return object; }
            emit("C"+std::to_string(address)+':'+std::to_string(number));const auto object=static_cast<std::uint32_t>(kParentPool+(profile==1 ? 9 : 5)*stride);Put(raw(object),0,kParentVt);return object;
        };
        hooks.vtable=[&](auto object) { const auto table=Get(raw(object),0);emit("V"+std::to_string(object)+':'+std::to_string(table));return table; };
        hooks.storeType=[&](auto object,auto value) { emit("W"+std::to_string(object)+':'+std::to_string(value));raw(object)[10]=value;if (profile==2) Put(raw(object),0,kParentVt+0x100); };
        hooks.postCreate=[&](auto object,auto table) { emit("P"+std::to_string(object)+':'+std::to_string(table)); };
        const auto result=CreateRawSquid(edition,type,flags,hooks);CHECK(result==Number(row[2]) && trace==row[3]);
        CHECK(row[4].size()==stride*2);
        for (std::size_t index=0;index<stride;++index) CHECK(raw(result)[index]==std::stoul(row[4].substr(index*2,2),nullptr,16)); // 반환 객체의 보존된 모든 바이트도 원문과 같아야 한다.
        ++rows;
    }
    CHECK(rows==588);
}

TEST_CASE(SquidCreationParent_ActualFactoryUsesCurrentConstructorAndRealFormVtable) { // 자산 TYPE도 생성자의 실제 Form VT면 빈+28을 선택한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 실제 풀에서 두 판본의 네 Form을 생성한다.
        SidPool pool(edition,32768);std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);
        SquidFactory factory(pool,types);factory.BindDiagnostics({types,{},{},{}});
        for (const auto type:{1U,2U,3U,6U}) { // 일반Create를 통해 VT/type/void만 쓰고 자산HP/깊이를 추가하지 않는다.
            types[type].constructorAddress=TypeConstructorAddress(edition,type);types[type].zOrder=99;types[type].flags1=16;types[type].maxHitPoints=777;
            const auto sid=factory.Create(type);const auto raw=pool.Slot(sid);CHECK(Get(raw,0)==SquidForms(pool).Vtable(type) && raw[10]==type && raw[11]==4 && Get(raw,26)==0 && raw[edition==OriginalEdition::Patch1078 ? 35 : 33]==0);
        }
        types[74].constructorAddress=TypeConstructorAddress(edition,6);types[74].zOrder=79;types[74].flags1=16;types[74].maxHitPoints=999;
        const auto sid=factory.Create(74);CHECK(pool.Slot(sid)[10]==74 && Get(pool.Slot(sid),0)==SquidForms(pool).Vtable(6) && Get(pool.Slot(sid),26)==0);
        types[74].constructorAddress=0;const auto next=factory.Create(74);CHECK(Get(pool.Slot(next),26)==999 && pool.Slot(next)[edition==OriginalEdition::Patch1078 ? 35 : 33]==79);
    }
}

TEST_CASE(SquidCreationParent_DiagnosisResumesBeforeAllocationAndCurrentLookup) { // fake TYPE 진단의 정상 복귀를 실패로 바꾸지 않고 같은 현재 표를 읽는다.
    SidPool pool(OriginalEdition::Patch1078,32768);std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);SquidFactory factory(pool,types);std::uint32_t fake=74,calls{};factory.BindFakeSurfaceType(fake);
    factory.BindDiagnostics({types,[&](auto expression,auto file,auto line) {
        CHECK(expression=="type != fakeThreeByThreeSurfaceType" && file=="\\Ns\\O\\Squid.cpp" && line==0x189 && pool.FreeCount()==32763);
        ++calls;types[74].constructorAddress=TypeConstructorAddress(pool.Edition(),3);
    },{},{}});
    const auto sid=factory.Create(74);CHECK(calls==1 && pool.FreeCount()==32762 && pool.Slot(sid)[10]==74 && Get(pool.Slot(sid),0)==0x512188);
    fake=75;CHECK(factory.Create(74).value==sid.value+1 && calls==1);
    types[76].constructorAddress=0x12345678;const auto previous=pool.FreeCount();CHECK(CreationFails([&] { factory.Create(76); }) && pool.FreeCount()==previous-1);
    CHECK(pool.Slot(Sid{static_cast<std::uint16_t>(sid.value+2)})[10]==0); // 미연결 cdecl 전에는 TYPE BYTE를 쓰지 않았다.
}

TEST_CASE(SquidCreationParent_ConstructCurrentHasNoStateOrAssetGuard) { //004af720은 할당/TYPE/가상 후처리 없이 현재 생성자의 bare 쓰기만 한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 원문 물리 범위의 free/contained/예약 슬롯도 그대로 처리한다.
        SidPool pool(edition,32768);std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);factory.BindDiagnostics({types,{},{},{}});const auto free=pool.FreeCount();
        for (const auto sid:{Sid{0},Sid{5},Sid{17}}) { // 각 상태 입력의 VT 밖 모든 바이트를 보존한다.
            auto raw=Physical(pool,sid);
            for (const auto state:{1U,2U,8U,12U}) { // free/dead/contained/void를 선행 거부하지 않는다.
                for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*7+9); // 기대 출력과 독립인 payload다.
                raw[10]=1;raw[11]=static_cast<std::uint8_t>(state);types[1].constructorAddress=TypeConstructorAddress(edition,6);const auto before=std::vector(raw.begin(),raw.end());factory.ConstructCurrent(sid);
                CHECK(Get(raw,0)==SquidForms(pool).Vtable(6) && std::equal(raw.begin()+4,raw.end(),before.begin()+4));
                types[1].constructorAddress=0;factory.ConstructCurrent(sid);CHECK(Get(raw,0)==(edition==OriginalEdition::Patch1078 ? 0x501dd0U : 0x5058d8U) && std::equal(raw.begin()+4,raw.end(),before.begin()+4));
                if (edition==OriginalEdition::Patch1078) { types[1].constructorAddress=0x4429e0;const auto sentinel=std::vector(raw.begin(),raw.end());factory.ConstructCurrent(sid);CHECK(std::equal(raw.begin(),raw.end(),sentinel.begin())); }
            }
        }
        CHECK(pool.FreeCount()==free);
    }
}

TEST_CASE(SquidCreationParent_MissingDevicesPreserveEarlierParentEffects) { // 실패를 성공 SID나 취소로 위조하지 않고 선택 위치의 효과만 남긴다.
    std::string trace;RawSquidCreationHooks hooks;hooks.fakeSurfaceType=[] { return 162U; };
    hooks.allocate=[&](auto) { trace+='A';return 5U; };hooks.constructor=[&](auto) { trace+='T';return 123U; };
    CHECK(CreationFails([&] { CreateRawSquid(OriginalEdition::Patch1078,74,0,hooks); }) && trace=="AT");
    hooks.construct=[&](auto,auto) { trace+='C';return 0U; };hooks.vtable=[&](auto object)->std::uint32_t { CHECK(object==0);trace+='V';throw std::logic_error("명시 null 반환 객체"); };
    hooks.storeType=[&](auto,auto) { trace+='W'; };trace.clear();CHECK(CreationFails([&] { CreateRawSquid(OriginalEdition::Patch1078,74,0,hooks); }) && trace=="ATCV");
    trace.clear();CHECK(CreationFails([&] { CreateRawSquid(OriginalEdition::Cd1072,74,0,hooks); }) && trace=="ATCV"); // CD도 실제 ASM의 VT 읽기가 TYPE 저장보다 먼저다.
}

TEST_CASE(SquidCreationParent_ActualWorldKeepsLongLivedFakeTypeAcrossReuse) { // 실제 World의 공장이 같은 장기005412e0을 빌린다.
    std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);
    auto globals=std::make_shared<RawGameAuxState>();globals->FakeSurfaceType()=74;auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes,nullptr,std::shared_ptr<RawAuthorityState>{},globals);
    std::optional<std::string> detail;std::uint32_t calls{};storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="type != fakeThreeByThreeSurfaceType" && file=="\\Ns\\O\\Squid.cpp" && line==0x189);++calls; });
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    for (std::size_t generation=0;generation<2;++generation) { // 새 Session/공장에 지역 사본162를 연결하지 않는다.
        RawSurfaceWorld world(OriginalEdition::Patch1078,storage->types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();core.factory.Create(74);CHECK(globals->FakeSurfaceType()==74 && calls==generation+1);
    }
    globals->ResetForGeneralWorld();CHECK(globals->FakeSurfaceType()==74);
}
