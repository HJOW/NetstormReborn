// 공통 객체 Take 부모와 실제 CD 진단 복귀만 검사한다. 게임/MP/패킷/통신은 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawSquidTake.h"
#include "o/SquidFactory.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 원문 장치 주소/물리 입력만 가지며 판단 기대값은 fixture에서 읽는다.
constexpr std::uint32_t kTakePool=0x13000000,kTakeVt=0x14000000,kTakeCtor=0x10001000; // Native의 객체/가상 표/cdecl 장치 토큰이다.
}

TEST_CASE(SquidTakeParentCurrent_Native720ReturnedObjectsAndCachedVtableOrder) { // 실제 두 부모의 전체 읽기/쓰기 순서와 두 객체를 대조한다.
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_SQUID_TAKE_CURRENT_FIXTURE).rows) { // 입력/관측은 최초 동결한 원문 결과만 사용한다.
        CHECK(row.size()==5);const auto input=Split(row[1],',');CHECK(input.size()==5);const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;const std::size_t width=patch ? 50 : 36;
        const auto type=Number(input[0]),sid=Number(input[1]),profile=Number(input[3]);const bool base=Number(input[4])!=0;std::array<std::vector<std::uint8_t>,2> slots{std::vector<std::uint8_t>(width),std::vector<std::uint8_t>(width)};std::array<std::uint32_t,2> objects{kTakePool+static_cast<std::uint32_t>(5*width),kTakePool+static_cast<std::uint32_t>(9*width)};std::uint32_t ctor=base ? 0 : kTakeCtor;std::string trace;
        const auto event=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 명시 장치와 물리 읽기/쓰기를 같은 순서로 기록한다.
        const auto raw=[&](std::uint32_t object)->std::vector<std::uint8_t>& { CHECK(object==objects[0] || object==objects[1]);return slots[object==objects[0] ? 0 : 1]; }; // 객체 토큰을 인자의 SID로 다시 해석하지 않는다.
        for (std::size_t i=0;i<slots.size();++i) { for (std::size_t j=0;j<width;++j) slots[i][j]=static_cast<std::uint8_t>((std::size_t(i ? 9 : 5)*width+j)*37+19);Put(slots[i],0,kTakeVt);slots[i][10]=74;slots[i][11]=static_cast<std::uint8_t>(Number(input[2])); } // Native의 두 전체 payload 입력이다.
        RawSquidTakeHooks hooks;hooks.capacity=[&] { event("Q32768");return 32768U; };hooks.slot=[&](auto) { return objects[0]; };
        hooks.state=[&](auto object) { const auto value=raw(object)[11];event("R"+std::to_string(object)+':'+std::to_string(value));return value; };hooks.type=[&](auto object) { const auto value=raw(object)[10];event("T"+std::to_string(object)+':'+std::to_string(value));return value; };
        hooks.vtable=[&](auto object) { const auto value=Get(raw(object),0);event("V"+std::to_string(object)+':'+std::to_string(value));return value; };
        hooks.detail=[&](auto server,auto client) { event("F"+std::to_string(server)+':'+std::to_string(client));event("L"); };hooks.assertion=[&](auto expression,auto file,auto line) { event("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(line)); };
        hooks.unpop=[&](auto object,auto table) { event("U"+std::to_string(object)+':'+std::to_string(table));if (profile==1) { raw(object)[11]=0x8f;ctor=base ? 0 : kTakeCtor+0x10; } }; //  실제 자식으로 주장하지 않는 정상 복귀 입력이다.
        hooks.storeState=[&](auto object,auto value) { event("W"+std::to_string(object)+':'+std::to_string(value));raw(object)[11]=value; };hooks.storeType=[&](auto object,auto value) { event("Y"+std::to_string(object)+':'+std::to_string(value));raw(object)[10]=value;if (profile==2) { Put(raw(object),0,kTakeVt+0x100);raw(object)[11]=0x80; } };
        hooks.constructor=[&](auto number) { event("C"+std::to_string(number)+':'+std::to_string(ctor));return ctor; };hooks.construct=[&](auto address,auto number) { // base 쓰기 또는 명시 cdecl 반환 객체를 공급한다.
            if (!address) { const auto table=patch ? 0x501dd0U : 0x5058d8U;event("B"+std::to_string(objects[0])+':'+std::to_string(table));Put(raw(objects[0]),0,table);return objects[0]; }
            event("A"+std::to_string(address)+':'+std::to_string(number));const auto object=objects[profile==1 ? 1 : 0];Put(raw(object),0,kTakeVt);return object;
        };hooks.postTake=[&](auto object,auto table) { event("P"+std::to_string(object)+':'+std::to_string(table)); };
        const auto returned=TakeRawSquid(edition,type,sid,hooks);const auto bytes=Hex(slots[0])+';'+Hex(slots[1]);const bool same=returned==Number(row[2]) && trace==row[3] && bytes==row[4];
        if (!same) { std::printf("%s %s\nactual %s\nnative %s\n",row[0].c_str(),row[1].c_str(),trace.c_str(),row[3].c_str());CHECK(same);return; }++rows;
    }
    CHECK(rows==720);
}

TEST_CASE(SquidTakeParentCurrent_CdActualDiagnosticCurrentConstructorAndPayload) { // 실제 공장은 CD 진단 복귀 뒤 바뀐 생성자와 공통 후처리를 사용한다.
    SidPool pool(OriginalEdition::Cd1072,32768);std::vector<RiftTypeRecord> types(171);types[74].name="SERVER";types[75].name="CLIENT";SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);SquidFactory factory(pool,types,false,&unpop);const auto sid=factory.Create(74);auto raw=pool.AllocatedBytes(sid);raw[10]=75;raw[26]=1;raw[27]=2;raw[28]=3;raw[29]=4;raw[32]=9;std::string detail;std::uint32_t calls{};
    factory.BindDiagnostics({types,[&](auto expression,auto file,auto line) { CHECK(expression=="0" && file=="\\ns\\o\\Squid.cpp" && line==0x1b1 && detail=="Invalid take. Server type SERVER, client type CLIENT");++calls;types[74].constructorAddress=TypeConstructorAddress(OriginalEdition::Cd1072,161);types[74].zOrder=0x144; },[&](auto text) { detail=text; }});
    const auto free=pool.FreeCount();CHECK(factory.Take(74,sid)==sid);CHECK(calls==1 && raw[10]==74 && raw[11]==4 && raw[26]==0 && raw[27]==0 && raw[28]==3 && raw[29]==4 && raw[32]==9 && raw[33]==0x44 && pool.FreeCount()==free);
}

TEST_CASE(SquidTakeParentCurrent_SelectedFailureLeavesPriorStateStores) { // 선택되지 않은 capacity/상세/해제 장치를 요구하지 않고 실패 위치를 보존한다.
    RawSquidTakeHooks hooks;std::uint8_t state=3;std::vector<std::uint8_t> writes;hooks.assertion=[](auto,auto,auto) {};hooks.slot=[](auto) { return 5U; };hooks.state=[&](auto) { return state; };hooks.storeState=[&](auto,auto value) { state=value;writes.push_back(value); };
    CHECK(Throws([&] { TakeRawSquid(OriginalEdition::Cd1072,74,5,hooks); }) && (writes==std::vector<std::uint8_t>{2,0}) && state==0); // 생성자 조회 누락 전 두 실제 상태 쓰기가 남는다.
}
