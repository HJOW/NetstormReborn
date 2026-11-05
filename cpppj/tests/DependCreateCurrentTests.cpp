// 감시 쌍 생성 부모의 최초 PE 관측과 실제 현재 World 공장/번호를 검사한다. 게임·MP는 실행하지 않는다.
#include "FormTypeInputs.h"
#include "RawSceneSupport.h"
#include "o/RawDependCreate.h"
#include "o/RawGameAuxState.h"
#include "client/RawSurfaceWorld.h"
#include "o/SquidFormDispatch.h" // 실제 VT와 다른 raw TYPE 등록이 있는 실패 경계를 검사한다.
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 명시 입력의 물리 슬롯과 현재 번호만 구성한다.
constexpr std::uint32_t kDependVt=0x14000000; // Native 명시 가상 장치와 같은 VT 토큰 시작값이다.
std::span<std::uint8_t> DependRaw(SidPool& pool,std::uint32_t sid) { const auto raw=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 검사에서만 예약/free 물리 입력도 직접 준비한다.
BridgeConnectState DependLinks() { BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;return links; } // 실제 World 설치용 정상 TYPE 입력이다.
}
TEST_CASE(DependCreateCurrent_IndependentPeParentFixture1620) { // 진단/두 Create/WORD/저장·현재 VT와 두 raw 슬롯 전체를 원문에 대조한다.
    std::ifstream input(NETSTORM_DEPEND_CREATE_CURRENT_FIXTURE);CHECK(input.good());if (!input) throw std::runtime_error("Depend 부모 최초 관측 없음");std::array<std::size_t,2> counts{};std::string line;
    while (std::getline(input,line)) { // 각 최초 관측은 독립 물리 풀에서 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;const auto row=Split(line,'\t');CHECK(row.size()==6);if (row.size()!=6) throw std::runtime_error("Depend 관측 열 수 오류");const bool patch=row[0]=="originals";CHECK(patch || row[0]=="originalCD");++counts[patch ? 0 : 1];const auto args=Split(row[1],',');CHECK(args.size()==4);const auto watched=Number(args[0]),watcher=Number(args[1]),profile=Number(args[3]);bool server=Number(args[2])!=0;std::uint32_t number=7,creates{};std::string events;SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768);
        for (std::uint32_t slot=0;slot<16;++slot) { auto raw=DependRaw(pool,slot);for (std::size_t offset=0;offset<raw.size();++offset) raw[offset]=static_cast<std::uint8_t>((slot*raw.size()+offset)*37+19); } // Native와 같은 초기 합성 payload만 놓는다.
        const auto append=[&](std::string event) { if (!events.empty()) events+=';';events+=event; }; // 원문 호출/쓰기 순서만 직렬화한다.
        DependCreateHooks hooks;hooks.server=[&] { return server; };hooks.formType=[&] { return number; };hooks.sid=[](auto token) { return token; }; // 명시 Create 토큰은 실제 Native 객체의 SID5/9다.
        hooks.create=[&](auto type,auto flags) { const auto sid=creates==0 ? 5U : 9U;append("C"+std::to_string(type)+','+std::to_string(flags)+','+std::to_string(sid));auto raw=DependRaw(pool,sid);Put(raw,0,kDependVt+(sid==5 ? 0 : 0x100));raw[10]=static_cast<std::uint8_t>(type);raw[11]=4;if (profile==1 && creates==0) number=0x12345678;++creates;return sid; }; // 실제 생성자로 주장하지 않는 동일 명시 반환/현재 번호 변경 입력이다.
        hooks.word=[&](auto sid,auto offset,auto value) { append("W"+std::to_string(sid)+','+std::to_string(offset)+','+std::to_string(value));Put(DependRaw(pool,sid),offset,value,2);if (profile==3 && sid==9 && offset==18) Put(DependRaw(pool,5),0,kDependVt+0x200); }; // CD의 저장 VT와 PATCH의 뒤 재조회 차이를 만든다.
        hooks.vtable=[&](auto sid) { return Get(DependRaw(pool,sid),0); }; // 각 원문 선택 위치의 현재 VT를 읽는다.
        hooks.attach=[&](auto sid,auto table,auto parent,auto flags) { append("A"+std::to_string(sid)+','+std::to_string(table)+','+std::to_string(parent)+','+std::to_string(flags));if (profile==4 && sid==5) Put(DependRaw(pool,9),0,kDependVt+0x200); }; // 첫 가상 부착이 바꾼 두 번째 VT를 다음 호출이 읽어야 한다.
        hooks.assertion=[&](auto expression,auto file,auto sourceLine) { append("D"+std::string(expression)+'|'+std::string(file)+'|'+std::to_string(sourceLine));if (profile==2) { number=0xffffffff;server=true; } }; // 정상 복귀 뒤에도 저장 flags0을 다시 계산하지 않는다.
        const auto pair=CreateDependForms(pool.Edition(),watched,watcher,hooks);CHECK(pair.watched==9 && pair.watcher==5);CHECK(std::to_string(number)==row[2] && std::to_string(server)==row[3]);CHECK(events==row[4]);CHECK(Hex(pool.Slot(Sid{5}))+";"+Hex(pool.Slot(Sid{9}))==row[5]);
    }
    CHECK(counts[0]==810 && counts[1]==810);
}
TEST_CASE(DependCreateCurrent_WorldReadsNumberTwiceAndKeepsPreviousClassRoute) { // 첫 Create 진단이 번호/VT를 고치면 두 번째 Create와 실제 감시 삭제가 현재 값을 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 양판본의 같은 장기 owner로 두 World를 대여한다.
        const bool patch=edition==OriginalEdition::Patch1078;auto types=netstorm::test::MakeFormTypeInputs(patch ? 188 : 171);types[7].constructorAddress=0;types[8].constructorAddress=TypeConstructorAddress(edition,1);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());auto aux=std::make_shared<RawGameAuxState>();auto storage=std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes,nullptr,std::shared_ptr<RawAuthorityState>{},aux);std::optional<std::string> detail;RawWorldCore* current{};Sid expectedFirst{};std::size_t diagnostics{};
        storage->BindDisplayDiagnostics(detail,[&](auto expression,auto,auto sourceLine) { CHECK(current && expression=="!isForm()" && sourceLine==(patch ? 0x7d4U : 0x7a6U));++diagnostics;auto raw=current->pool.AllocatedBytes(expectedFirst);CHECK(raw[10]==7);Put(raw,0,SquidForms(current->pool).Vtable(1));aux->DependFormType()=8; }); // 원문 Base postCreate 진단의 정상 복귀/실제 VT 변경 입력이다.
        for (std::uint32_t generation=0;generation<2;++generation) { // World 준비/반납이 같은 현재 번호를 초기1로 되돌리지 않는다.
            aux->DependFormType()=7;RawSurfaceWorld world(edition,storage->types,frames,shapes,DependLinks(),83,nullptr,storage);auto core=world.Core();current=&core;const auto watched=core.factory.Create(74,2),watcher=core.factory.Create(74,2);expectedFirst=core.pool.FirstFree(true);const auto free=core.pool.FreeCount();const auto before=diagnostics;const auto pair=core.dependencies.Create(watched,watcher);
            CHECK(diagnostics==before+1 && aux->DependFormType()==8 && pair.watcherForm==expectedFirst);CHECK(core.pool.Slot(pair.watcherForm)[10]==7 && core.pool.Slot(pair.watchedForm)[10]==8);CHECK(core.dependencies.IsDependForm(pair.watcherForm) && core.dependencies.IsDependForm(pair.watchedForm) && core.dependencies.Has(watched,watcher));CHECK(CommonContainedParent(core.pool,pair.watcherForm)==watcher && CommonContainedParent(core.pool,pair.watchedForm)==watched);
            core.dependencies.Remove(watcher,watched,0x10);CHECK(!core.dependencies.Has(watched,watcher) && core.pool.FreeCount()==free && Get(core.pool.Slot(watched),6,2)==0 && Get(core.pool.Slot(watcher),6,2)==0);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0); // 이전 TYPE7 감시 Form도 현재 VT로 재진입해 정확히 한 번 반납한다.
            aux->DependFormType()=0xffffffff;const auto remaining=core.pool.FreeCount();CHECK(Throws([&] { core.dependencies.Create(watched,watcher); }) && core.pool.FreeCount()==remaining-1);current=nullptr; // 전체 DWORD 범위 오류는 첫 공통 할당 뒤에 발생하며 BYTE7/초기1로 대체하지 않는다.
        }
        aux->ResetForGeneralWorld();CHECK(aux->DependFormType()==0xffffffff);
    }
}
TEST_CASE(DependCreateCurrent_BindingAndSelectedAttachFailureKeepLinks) { // 다른 풀 연결은 거부하고 선택된 정렬 부착 부재는 앞선 두 할당/링크를 보존한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // client 부모 입력으로 실제 전송/통신을 선택하지 않는다.
        SidPool pool(edition,32768),other(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);types[7].constructorAddress=TypeConstructorAddress(edition,6);SquidFactory factory(pool,types),foreign(other,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);DependFormHost host(pool,destroy);std::uint32_t number=7;CHECK(Throws([&] { host.BindCreation(foreign,number); }));host.BindCreation(factory,number);const auto watched=factory.Create(74,2),watcher=factory.Create(74,2),first=pool.FirstFree(true);const auto free=pool.FreeCount();CHECK(Throws([&] { host.Create(watched,watcher); }) && pool.FreeCount()==free-2);const auto second=Get(pool.Slot(first),20,2);CHECK(Get(pool.Slot(first),18,2)==0 && Get(pool.Slot(Sid{static_cast<std::uint16_t>(second)}),18,2)==first.value && Get(pool.Slot(Sid{static_cast<std::uint16_t>(second)}),20,2)==0);CHECK(Get(pool.Slot(watched),6,2)==0 && Get(pool.Slot(watcher),6,2)==0);
    }
}
TEST_CASE(DependCreateCurrent_IndependentHostUsesCurrentInputConstructor) { // 독립 Host도 TYPE1 생성자를 읽으며 실제 Gump VT를 고정 Depend VT로 덮지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 같은 독립 TYPE 입력과 원문 공통 부착을 사용한다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);types[1].constructorAddress=TypeConstructorAddress(edition,3);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);DependFormHost host(pool,destroy);const auto watched=factory.Create(74,2),watcher=factory.Create(74,2);const auto pair=host.Create(watched,watcher);CHECK(Get(pool.Slot(pair.watcherForm),0)==SquidForms(pool).Vtable(3) && Get(pool.Slot(pair.watchedForm),0)==SquidForms(pool).Vtable(3));CHECK(pool.Slot(pair.watcherForm)[10]==1 && !host.IsDependForm(pair.watcherForm) && CommonContainedParent(pool,pair.watcherForm)==watcher && CommonContainedParent(pool,pair.watchedForm)==watched);
    }
}
TEST_CASE(DependCreateCurrent_UnregisteredClassDoesNotUseDifferentRawTypeRoute) { // 감시 VT가 선택되면 raw TYPE3의 Gump 처리기로 상대 삭제를 생략하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 미등록 가상 진입 오류는 공통 destroy의 dead/깊이 변경 뒤 발생한다.
        SidPool pool(edition,32768);auto types=netstorm::test::MakeFormTypeInputs(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);SquidDestroyHooks assets;assets.selected=[] { return Sid{}; };assets.emit=[](const SquidDestroyEvent&) {};SquidFormDispatch dispatch(pool,destroy,std::move(assets));GumpFormHost gumps(pool,factory,types,destroy,{},dispatch.Hooks());dispatch.Bind(gumps);
        const auto parent=factory.Create(74,2),source=gumps.Create(2),target=gumps.Create(2);gumps.Attach(source,parent);auto raw=pool.AllocatedBytes(source);Put(raw,0,SquidForms(pool).Vtable(1));Put(raw,20,target.value,2);const auto free=pool.FreeCount();CHECK(Throws([&] { dispatch.Destroy(source,0x10); }));CHECK((raw[11]&3U)==2U && pool.FreeCount()==free && Get(pool.Slot(parent),6,2)==source.value && !(pool.Slot(target)[11]&3U) && destroy.PreDepth()==1); // 이미 수행한 공통 접두는 보존하고 미구현 수명을 다른 클래스의 성공으로 숨기지 않는다.
    }
}
