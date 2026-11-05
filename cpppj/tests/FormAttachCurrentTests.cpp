// 공통 부착의 첫 PE 관측과 실제 현재 Factory/World 소비자를 검사한다. 게임/통신은 실행하지 않는다.
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "o/RawFormAttach.h"
#include "o/RawSquidContainedUnpop.h"
#include "o/GumpFormHost.h"
#include "client/RawSurfaceWorld.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 명시 기계어 입력과 같은 물리 슬롯만 구성한다.
constexpr std::uint32_t kAttachVt=0x14000000; // 원문 관측기의 가상 setter 토큰 시작값이다.
std::span<std::uint8_t> FormAttachRaw(SidPool& pool,std::uint32_t sid) { const auto raw=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 검사에서만 free/예약 입력을 직접 준비한다.
BridgeConnectState FormAttachLinks() { BridgeConnectState state;state.bridgeType=82;state.islandType=94;state.stalagType=95;state.connectorType=155;state.noIslandType=157;return state; } // 실제 World의 정상 TYPE 번호다.
}
TEST_CASE(FormAttachCurrent_IndependentPeParentFixture864) { // 현재 VT/저장 head·부모/진단 복귀와 네 raw 슬롯 전체를 원문에 대조한다.
    std::ifstream input(NETSTORM_FORM_ATTACH_CURRENT_FIXTURE);CHECK(input.good());if (!input) throw std::runtime_error("공통 부착 최초 관측 없음");std::string line;std::array<std::size_t,2> counts{};
    while (std::getline(input,line)) { // 모든 최초 관측 입력은 독립 풀에서 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;const auto row=Split(line,'\t');CHECK(row.size()==5);if (row.size()!=5) throw std::runtime_error("부착 관측 열 수 오류");const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");++counts[patch ? 0 : 1];const auto args=Split(row[1],',');CHECK(args.size()==6);const auto parent=Number(args[0]),number=Number(args[1]),genus=Number(args[2]),head=Number(args[3]),profile=Number(args[4]),flags=Number(args[5]);std::uint32_t content=6;SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);std::string events;
        for (std::uint32_t sid=0;sid<16;++sid) { auto raw=FormAttachRaw(pool,sid);for (std::size_t offset=0;offset<raw.size();++offset) raw[offset]=static_cast<std::uint8_t>((sid*raw.size()+offset)*37+19); } // 합성 payload만 준비하고 결과는 계산하지 않는다.
        for (std::uint32_t index=0;index<3;++index) Put(FormAttachRaw(pool,5+index*2),0,kAttachVt+index*0x100); // 명시 세 가상 장치의 초기 토큰이다.
        auto raw=FormAttachRaw(pool,5);raw[10]=static_cast<std::uint8_t>(number);raw[11]=4;Put(FormAttachRaw(pool,parent),6,head,2);
        const auto append=[&](std::string event) { if (!events.empty()) events+=';';events+=event; }; // 실제 호출 순서만 직렬화한다.
        FormAttachHooks hooks;hooks.genus=[&](auto current) { append("G"+std::to_string(current));return genus; };hooks.contentType=[&] { return content; }; // 명시 getter의 현재 입력이다.
        hooks.assertion=[&](auto expression,auto file,auto sourceLine) { append("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(sourceLine));if (profile==1) { Put(raw,0,kAttachVt+0x100);raw[10]=69; }if (profile==3 && expression=="n != getSid()") { Put(FormAttachRaw(pool,parent),6,7,2);Put(raw,16,11,2); }if (profile==5 && expression=="getType()!=contentFormType") raw[10]=71; }; // 정상 복귀 후 현재 VT/TYPE와 저장 head의 차이를 만드는 입력이다.
        hooks.setParent=[&](Sid sid,auto table,Sid value) { append("P"+std::to_string(sid.value)+','+std::to_string(table)+','+std::to_string(value.value));Put(FormAttachRaw(pool,sid.value),16,value.value,2);if (profile==2) { Put(raw,0,kAttachVt+0x200);Put(FormAttachRaw(pool,parent),6,7,2);Put(raw,16,11,2); } }; // 실제 setter로 주장하지 않는 명시 자식이다.
        hooks.setPrevious=[&](Sid sid,auto table,Sid value) { append("R"+std::to_string(sid.value)+','+std::to_string(table)+','+std::to_string(value.value));Put(FormAttachRaw(pool,sid.value),14,value.value,2);if (profile==4 && value.value==5) { Put(raw,0,kAttachVt+0x100);Put(raw,16,11,2);Put(FormAttachRaw(pool,parent),6,13,2); } }; // 이웃 콜백 후 자신의 현재 VT를 다시 선택해야 한다.
        hooks.pop=[&](Sid,auto value) { append("O"+std::to_string(value)+','+std::to_string(raw[11])+','+std::to_string(Get(raw,4,2))+','+std::to_string(Get(raw,14,2))+','+std::to_string(Get(raw,16,2))+','+std::to_string(Get(FormAttachRaw(pool,parent),6,2)));raw[11]&=static_cast<std::uint8_t>(~4U);if (profile==5) { raw[10]=70;content=70; } }; // Pop 이전의 완료 링크도 관측한다.
        AttachCommonForm(pool,Sid{5},Sid{static_cast<std::uint16_t>(parent)},flags,hooks);CHECK(events==row[2]);CHECK(std::to_string(content)==row[3]);CHECK(Hex(pool.Slot(Sid{0}))+";"+Hex(pool.Slot(Sid{5}))+";"+Hex(pool.Slot(Sid{7}))+";"+Hex(pool.Slot(Sid{9}))==row[4]);
    }
    CHECK(counts[0]==432 && counts[1]==432);
}
TEST_CASE(FormAttachCurrent_WorldUsesLiveGenusAndWeakSessionContentNumber) { // Gump도 현재 TYPE 진단→실제 VT setter→현재 Session 꼬리 진단을 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 장기 TYPE와 반납 가능한 Session 번호를 양판본에 연결한다.
        const bool patch=edition==OriginalEdition::Patch1078;auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto storage=std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes,nullptr);std::optional<std::string> detail;SidPool* pool{};Sid form{};std::vector<std::uint32_t> lines;
        storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(pool && file==(patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp"));lines.push_back(line);auto raw=FormAttachRaw(*pool,form.value);if (expression=="!(getGenus() & (gEMPLACEMENT|gVORTEX |gFACTORY))") { CHECK(raw[11]==4);Put(raw,0,SquidForms(*pool).Vtable(1));raw[10]=69; }else if (expression=="getType()!=contentFormType") { CHECK(raw[11]==8);raw[10]=70; }else { CHECK(expression=="isForm()");raw[10]=1; } }); // 같은 실제 진단 장치가 정상 복귀 후 현재 슬롯을 고친다.
        RawSurfaceWorld world(edition,storage->types,frames,shapes,FormAttachLinks(),83,nullptr,storage);auto core=world.Core();pool=&core.pool;GumpFormHost gumps(core.pool,core.factory,storage->types,core.destroy);const auto parent=core.factory.Create(74,2);form=gumps.Create(2);FormAttachRaw(core.pool,form.value)[10]=7;storage->types[7].flags2=0x44200;std::uint32_t content=69,gump=3,event=0;auto session=std::make_shared<RawSquidContainedUnpop>(core.pool,SquidContainedUnpopState{content,gump,event,{}, {}, {}},SquidContainedUnpopHooks{});world.BindContainedUnpop(session);
        gumps.Attach(form,parent,0x10);CHECK(lines==std::vector<std::uint32_t>({patch ? 0x3d0U : 0x3c3U,patch ? 0x3e0U : 0x3d2U,patch ? 0x3e3U : 0x3d5U}));CHECK(core.pool.Slot(form)[10]==1 && core.pool.Slot(form)[11]==8 && CommonContainedParent(core.pool,form)==parent && Get(core.pool.Slot(parent),6,2)==form.value);world.BindContainedUnpop({});session.reset();core.factory.UnpopForm(form);CHECK(Get(core.pool.Slot(parent),6,2)==0);const auto next=gumps.Create(2);gumps.Attach(next,parent,0x10);CHECK(lines.size()==3 && core.pool.Slot(next)[11]==8); // 반납한 Session의 content69를 재사용하지 않는다.
    }
}
TEST_CASE(FormAttachCurrent_LateContentDiagnosticAndMissingNeighborKeepPrefix) { // 실제 공통 부모의 선택된 부재는 앞선 완료 바이트를 취소하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 진단 미연결/미지원 현재 VT를 같은 raw 풀에서 검사한다.
        auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SidPool pool(edition,32768);SquidFactory factory(pool,types);const auto parent=factory.Create(74,2),content=factory.CreateContained(2);const auto free=pool.FreeCount();CHECK(Throws([&] { factory.AttachForm(content,parent,0x10); }));CHECK(pool.Slot(content)[11]==8 && CommonContainedParent(pool,content)==parent && Get(pool.Slot(parent),6,2)==content.value && pool.FreeCount()==free);
        const auto form=factory.CreateGumpForm(2),neighbor=factory.Create(74,2);auto raw=FormAttachRaw(pool,form.value);Put(FormAttachRaw(pool,parent.value),6,neighbor.value,2);Put(FormAttachRaw(pool,neighbor.value),0,0xdeadbeef);CHECK(Throws([&] { factory.AttachForm(form,parent,0x10); }));CHECK(Get(raw,16,2)==parent.value && Get(raw,4,2)==neighbor.value && raw[11]==4 && Get(pool.Slot(parent),6,2)==neighbor.value && Get(raw,14,2)==0);
    }
}

TEST_CASE(FormAttachCurrent_ActualVtableSelectsMixedLinkWords) { // 바뀐 raw TYPE와 무관하게 자산/실제 Form VT의 서로 다른 WORD 위치를 쓴다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // PE 전수 setter 조회에 대응하는 두 실제 생성자 입력이다.
        auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SidPool pool(edition,32768);SquidFactory factory(pool,types);const auto parent=factory.Create(74,2),asset=factory.Create(74,2),form=factory.CreateGumpForm(2);auto assetRaw=FormAttachRaw(pool,asset.value),formRaw=FormAttachRaw(pool,form.value);assetRaw[10]=3;formRaw[10]=7;Put(assetRaw,14,0x12345678);Put(FormAttachRaw(pool,parent.value),6,asset.value,2);factory.AttachForm(form,parent,0x10);
        CHECK(Get(assetRaw,14,2)==0x5678 && Get(assetRaw,16,2)==form.value && Get(formRaw,4,2)==asset.value && Get(formRaw,14,2)==0 && Get(formRaw,16,2)==parent.value && formRaw[11]==8);CommonContainedSetParent(pool,asset,Sid{9});CHECK(Get(assetRaw,14,2)==9 && CommonContainedPrevious(pool,asset)==form);
        formRaw[10]=74;CommonContainedSetParent(pool,form,Sid{7});CommonContainedSetPrevious(pool,form,Sid{9});CHECK(CommonContainedParent(pool,form)==Sid{7} && CommonContainedPrevious(pool,form)==Sid{9} && Get(formRaw,4,2)==asset.value && formRaw[11]==8); // 직접 setter는 raw TYPE의 Form/자산 분류를 강제하지 않는다.
    }
}
TEST_CASE(FormAttachCurrent_ContentVtableReallyUsesCommonThunk) { // TYPE7의 실제 Content 클래스도 정렬 API 대체 없이 공통 부착을 수행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 현재 content 번호6과 raw TYPE7이 다르면 꼬리 진단을 선택하지 않는다.
        auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);types[7].constructorAddress=TypeConstructorAddress(edition,6);SidPool pool(edition,32768);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);DependFormHost host(pool,destroy);std::uint32_t current=7;host.BindCreation(factory,current);const auto watched=factory.Create(74,2),watcher=factory.Create(74,2);const auto pair=host.Create(watched,watcher);
        for (const auto sid:{pair.watcherForm,pair.watchedForm}) CHECK(Get(pool.Slot(sid),0)==SquidForms(pool).Vtable(6) && pool.Slot(sid)[10]==7 && pool.Slot(sid)[11]==8); // 실제 +74 thunk는 클래스6 선행 거부를 추가하지 않는다.
        CHECK(CommonContainedParent(pool,pair.watcherForm)==watcher && CommonContainedParent(pool,pair.watchedForm)==watched && Get(pool.Slot(watcher),6,2)==pair.watcherForm.value && Get(pool.Slot(watched),6,2)==pair.watchedForm.value);
    }
}
