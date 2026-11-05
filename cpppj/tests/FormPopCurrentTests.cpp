// 공통 Form Pop의 첫 PE 관측과 현재 World의 진단/공유 깊이를 검사한다. 게임/통신은 실행하지 않는다.
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "o/RawFormPop.h"
#include "o/ContentFormHost.h"
#include "client/RawSurfaceWorld.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 원문 관측과 같은 합성 슬롯/가상 토큰을 준비한다.
constexpr std::uint32_t kPopVt=0x14000000; // 명시 가상 PostPop의 두 현재 VT 토큰 시작값이다.
std::span<std::uint8_t> PopInputRaw(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 검사만 예약/free raw 입력을 직접 준비한다.
BridgeConnectState FormPopLinks() { BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;return links; } // 실제 World에 필요한 정상 TYPE 번호다.
}
TEST_CASE(FormPopCurrent_IndependentPeParentFixture2560) { // 원문 진단/flags/부분 상태·현재 VT·깊이와 전체 this 슬롯을 대조한다.
    std::ifstream input(NETSTORM_FORM_POP_CURRENT_FIXTURE);CHECK(input.good());if (!input) throw std::runtime_error("Form Pop 최초 관측 없음");std::string line;std::array<std::size_t,2> counts{};
    while (std::getline(input,line)) { // 각 첫 관측 입력은 독립 물리 풀/현재 전역에서 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;const auto row=Split(line,'\t');CHECK(row.size()==6);if (row.size()!=6) throw std::runtime_error("Pop 관측 열 수 오류");const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");++counts[patch ? 0 : 1];const auto args=Split(row[1],',');CHECK(args.size()==6);const Sid sid{static_cast<std::uint16_t>(Number(args[0]))};const auto flags=Number(args[2]),profile=Number(args[5]);bool server=Number(args[3])!=0;std::uint32_t depth=Number(args[4]);SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,server,&server);auto raw=PopInputRaw(pool,sid);std::string events;
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>((sid.value*raw.size()+i)*37+19); // 동일 합성 payload만 입력한다.
        Put(raw,0,kPopVt);raw[11]=static_cast<std::uint8_t>(Number(args[1]));const auto append=[&](std::string event) { if (!events.empty()) events+=';';events+=event; }; // 선택 호출 순서만 직렬화한다.
        FormPopHooks hooks;hooks.server=[&] { return server; };hooks.depth=[&] { return depth; };hooks.setDepth=[&](auto value) { depth=value; }; // 명시 장치는 같은 현재 전역을 읽고 쓴다.
        hooks.assertion=[&](auto expression,auto file,auto sourceLine) { append("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(sourceLine)+','+std::to_string(raw[11])+','+std::to_string(server)+','+std::to_string(depth));if (profile==1) { raw[11]=8;Put(raw,0,kPopVt+0x100);server=!server;depth=0xffffffff; } }; // 진단 복귀가 바꾼 현재 권한/상태/VT를 다음 명령이 읽어야 한다.
        hooks.transmit=[&](Sid actual,auto x,auto y,auto z,auto value) { CHECK(actual==sid);append("T"+std::to_string(x)+','+std::to_string(y)+','+std::to_string(z)+','+std::to_string(value)+','+std::to_string(raw[11]));if (profile==2) { raw[11]|=0x40;Put(raw,0,kPopVt+0x100);depth=7; } }; // 실제 패킷 작성/송신이 아닌 명시 자식 관측이다.
        hooks.postPop=[&](Sid actual,auto table,auto value) { CHECK(actual==sid);append("P"+std::to_string(table)+','+std::to_string(value)+','+std::to_string(raw[11])+','+std::to_string(depth));depth+=profile==3 ? 1U : 0xffffffffU; }; // 정상 감소/잘못 반납한 깊이 진단을 원문과 같은 입력으로 만든다.
        PopCommonForm(pool,sid,flags,hooks);CHECK(events==row[2]);CHECK(std::to_string(server)==row[3] && std::to_string(depth)==row[4]);CHECK(Hex(pool.Slot(sid))==row[5]);
    }
    CHECK(counts[0]==1280 && counts[1]==1280);
}
TEST_CASE(FormPopCurrent_ActualWorldReadsRepairedVtableAndSharedDepth) { // 진단 뒤의 현재 VT/서버와 같은 World의 깊이 wrap을 실제 공장이 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 장기 권한/저장소와 현재 실제 빈 Form 후처리를 연결한다.
        const bool patch=edition==OriginalEdition::Patch1078;auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto storage=std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes,nullptr);std::optional<std::string> detail;RawWorldCore* current{};Sid form{};std::size_t diagnostics{};
        storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(current && expression=="!xmitJustCreatedFlag" && file==(patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp") && line==(patch ? 0x35fU : 0x356U));++diagnostics;auto raw=PopInputRaw(current->pool,form);CHECK(raw[11]==0x10 && current->bookkeeping.depth==11);raw[11]=8;Put(raw,0,SquidForms(current->pool).Vtable(3));current->bookkeeping.depth=0xffffffff;storage->Authority()->server=false; }); // void는 이미 꺼졌고 다음 현재 권한/VT/깊이를 고친다.
        RawSurfaceWorld world(edition,storage->types,frames,shapes,FormPopLinks(),83,nullptr,storage);auto core=world.Core();current=&core;form=core.factory.CreateGumpForm(2);auto raw=PopInputRaw(core.pool,form);raw[10]=255;raw[11]=0x14;Put(raw,0,0xdeadbeef);core.bookkeeping.depth=11;core.factory.PopForm(form,8);CHECK(diagnostics==1 && raw[10]==255 && raw[11]==8 && core.bookkeeping.depth==0xffffffff && !core.pool.IsServer());current=nullptr; // TYPE 번호로 실제 빈 VT+20을 거부하지 않는다.
    }
}
TEST_CASE(FormPopCurrent_SelectedMissingDevicesKeepVoidAndDepthPrefix) { // 선택 전송/알 수 없는 후처리 실패는 앞선 void/비트/깊이 쓰기를 보존한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 실제 송신 장치는 연결하거나 호출하지 않는다.
        auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SidPool pool(edition,32768);SquidFactory factory(pool,types);std::uint32_t depth=7,other=9;factory.BindFormPopDepth(depth);CHECK(Throws([&] { factory.BindFormPopDepth(other); }));const auto form=factory.CreateGumpForm(0);auto raw=PopInputRaw(pool,form);const auto free=pool.FreeCount();CHECK(Throws([&] { factory.PopForm(form,5); }));CHECK(raw[11]==0x10 && depth==7 && pool.FreeCount()==free);
        raw[11]=4;Put(raw,0,0xdeadbeef);CHECK(Throws([&] { factory.PopForm(form,0x10); }));CHECK(raw[11]==0 && depth==8 && pool.FreeCount()==free); // 미지원 실제 VT+20은 깊이 증가 후 선택된다.
    }
}
TEST_CASE(FormPopCurrent_SortedContentBorrowsSameFactoryDepth) { // 정렬 부착 뒤 같은 실제 공장의 공통 Pop을 쓰며 사제 포획 경로는 선택하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 실제 전송/게임 없이 같은 정상 내용물 수명만 연결한다.
        auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SidPool pool(edition,32768);SquidFactory factory(pool,types);std::uint32_t depth=12;factory.BindFormPopDepth(depth);const auto parent=factory.Create(74,2),form=factory.CreateContained(2);auto raw=PopInputRaw(pool,form);Put(raw,18,154,2);Put(raw,20,0x20,2);std::size_t notices{},pops{};ContentFormHost content(pool,types,[&](Sid actualParent,Sid actualForm,std::uint32_t reason) { CHECK(actualParent==parent && actualForm==form && depth==12 && (reason==1 || reason==2));++notices; });content.BindPop([&](Sid actual,std::uint32_t flags) { CHECK(actual==form && flags==0x110 && depth==12);++pops;factory.PopForm(actual,flags); });content.Attach(form,parent,0x10);CHECK(pops==1 && notices==1 && depth==12 && raw[11]==8 && Get(pool.Slot(parent),6,2)==form.value);content.Unpop(form);CHECK(notices==2 && depth==12 && raw[11]==4 && Get(pool.Slot(parent),6,2)==0);
    }
}
