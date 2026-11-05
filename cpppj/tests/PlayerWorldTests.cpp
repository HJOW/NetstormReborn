// 실제 Player wrapper/Post/getter를 보호 자료와 같은 Pool/Host에 연결해 전체 원본 상태를 비교한다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "DudeSpawnSupport.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawPlayerWorld.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
using namespace netstorm::test::dudespawn;
namespace {
// 기대 상태는 실행 뒤에만 읽으며 현재 입력에는 독립 fixture의 첫 두 열만 사용한다.
const auto& PlayerRows() { static const auto rows=LoadFixture(NETSTORM_PLAYER_WORLD_FIXTURE).rows;return rows; }
// 선택되지 않은 가상 수명/송신/Regular 처리기를 실제 완료로 대체하지 않는다.
[[noreturn]] void MissingPlayerEffect() { throw std::logic_error("Player 선택 범위 밖의 실제 후속 효과 미복원"); }
// 단일타입 검색의 실제 기존 몸체에 필요한 연결 효과를 명시 거부로 공급한다.
BridgeConnectHooks PlayerFindHooks() {
    BridgeConnectHooks hooks;hooks.create=[](std::uint32_t,std::uint32_t)->Sid { MissingPlayerEffect(); };
    hooks.setOwner=[](Sid,std::uint32_t) { MissingPlayerEffect(); };hooks.pop=[](Sid,float,float,std::uint32_t) { MissingPlayerEffect(); };
    hooks.setFrame=[](Sid,std::int32_t,std::uint32_t) { MissingPlayerEffect(); };hooks.notifySurface=[](Sid) { MissingPlayerEffect(); };return hooks;
}
// 원본 초기9기록의 포인터 비트/Technology DWORD를 입력으로 준비한다. 호스트 포인터로 역참조하지 않는다.
void PlayerRecords(std::span<std::uint8_t> records,OriginalEdition edition) {
    const bool patch=edition==OriginalEdition::Patch1078;const auto stride=patch ? 0xb4U : 0xacU;
    // 각 실제 기록의 빈 목록 포인터와 서로 다른 Technology 저장 비트를 채운다.
    for (std::uint32_t owner=0;owner<9;++owner) { Put(records,owner*stride,0x12003000U+owner*0x100U);Put(records,owner*stride+(patch ? 0x9cU : 0x94U),0xfedc0000U+owner); }
}
// 보호 Player152의 생성자/blank SHP/foot1와 같은 실제 장부·공간·Form Host를 소유한다.
struct PlayerReplayScene {
    std::vector<std::string> input; // 행의 명시 입력10개이며 원본 관찰 출력과 분리한다.
    SidPool pool; // 실제 서버 풀에서 flags2가 client SID5를 배정한다.
    const DudeAssets& assets; // 보호 자산의 현재152/2프레임/1×1 발자국과 생성자를 읽기 재사용한다.
    SquidHash hash; // 같은 Pop/finder의 네 단계 실제 머리 목록이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 현재 빈 원본 점유 지도다.
    SquidPostPopState book; // 실제 전체 비용/세 표/목록/Pop 깊이다.
    DudeSink sink; // 같은1024×768 표시 변경 저장소다.
    SquidDisplay display; // 보호 Player의 빈 SHP 경계를 그대로 계산한다.
    SquidUnpop unpop; // 동일 실제 공간 해제 몸체다.
    SquidPostPop post; // 실제 Player 전체 후처리를 가상 분배한다.
    SquidPop pop; // 실제 첫 Pop의 좌표/해시/후처리다.
    SquidFactory factory; // 기존152 생성자를 재사용하는 실제 Factory다.
    SquidOwnerMode mode; // 현재fort/battle 두 모드를 별도로 공급한다.
    SquidOwner owner; // 실제 공통 가상 Owner 몸체다.
    RawSquidNeighbors neighbors; // 같은 Pool/Hash/spot/types를 직접 조회한다.
    BridgeConnectState bridge; // 단일타입 검색의 현재 전역 계약이다.
    RawBridgeConnect finder; // 기존 FindTypeAt 전체몸체를 재사용한다.
    RawSquidDestroy destroy; // 실제 ProcessForm의 공통 수명 계약이다.
    Kernel kernel; // 실제 ProcessForm PID 슬롯과 C++ 객체 소유권이다.
    SquidProcessState clock; // 초기Regular는 이 현재double시각을 그대로 저장한다.
    SquidProcessHost host; // 실제 event1/Regularctor/부모 부착/Form/Kernel다.
    std::vector<std::uint8_t> records; // 현재9개 Player의 같은 raw BYTE 저장소다.
    PlayerWorldState state; // PlayerKnown/두owner/fort/special을 독립적으로 공유한다.
    std::vector<std::string> events; // 진단/힙 요청/actual attach를 같은 순서로 기록한다.
    RegularProcess* regular{}; // 실제 생성된 Host 소유 프로세스를 실행 후 읽는다.
    std::unique_ptr<RawPlayerWorld> player; // 모든 참조 저장소 뒤에 생성하고 먼저 해제한다.
    // 최초Reset부터 현재 입력을 채우며 사전에 가짜 SID를 생성하지 않는다.
    explicit PlayerReplayScene(const std::vector<std::string>& row):input(Split(row[1],',')),pool(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,true),assets(Assets(pool.Edition())),
        display(pool.Edition(),assets.types,assets.shapes,sink,{0,0,65536,{0,0,1024,768}}),unpop(pool,hash,spots,&display),post(pool,assets.types,book),pop(pool,hash,spots,&display,&post),factory(pool,assets.types,false,&unpop),
        owner(pool,assets.types,book,mode),neighbors(pool,hash,spots,assets.types,assets.frames),finder(pool,neighbors,bridge,PlayerFindHooks()),destroy(pool,unpop,assets.types),kernel(pool.Edition()==OriginalEdition::Patch1078 ? kProcessCapacity : kCdProcessCapacity),
        host(pool,assets.types,kernel,destroy,clock,[](Sid,std::uint32_t,std::uint32_t,float)->float { MissingPlayerEffect(); }),records(9*(pool.Edition()==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)) {
        CHECK(input.size()==10);state={Number(input[4]),Number(input[2]),Number(input[3]),152,Number(input[5])!=0,Number(input[6])!=0};mode.fort=state.fortMode;mode.battle=!state.fortMode;
        CHECK(assets.types[152].footX==1 && assets.types[152].footY==1 && assets.types[152].maxHitPoints==0 && assets.shapes[152].frameCount==2);
        book.localOwner=static_cast<std::uint8_t>(state.battleOwner);clock.now=std::bit_cast<double>(std::stoull(input[9]));PlayerRecords(records,pool.Edition());
        const std::array<std::span<std::uint32_t>,3> tables{book.localCounts,book.localSecondaryCounts,book.globalCounts}; // 원본 seed0/표별17 오프셋 입력이다.
        // 현재 세 표 전체를 독립 입력으로 채우며 후처리 기대 증가를 대입하지 않는다.
        for (std::size_t table=0;table<tables.size();++table) for (std::size_t index=0;index<tables[table].size();++index) tables[table][index]=static_cast<std::uint32_t>(index)*2654435761U+static_cast<std::uint32_t>(table)*17U;
        PlayerWorldHooks hooks;hooks.assertion=[this](PlayerWorldAssertion assertion) { const bool patch=pool.Edition()==OriginalEdition::Patch1078;const auto line=assertion==PlayerWorldAssertion::PlayerAlreadyExists ? (patch ? 0x354U : 0x304U) : Number(input[0])==3 ? (patch ? 0x29dU : 0x24dU) : (patch ? 0x351U : 0x301U);events.push_back("A:"+std::to_string(line)); };
        hooks.find=[this](float x,float y,std::uint32_t type) { return finder.FindTypeAt(x,y,type); };hooks.create=[this](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
        hooks.setOwner=[this](Sid sid,std::uint32_t value) { owner.Set(sid,value); };hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { const auto size=display.FrameSize(pool.Edition(),pool.Slot(sid),0);if (pop.Pop(sid,assets.types[152],size[0],size[1],x,y,flags)!=RawPopResult::Registered) throw std::logic_error("Player 실제 첫 공간Pop 실패"); };
        hooks.reserveRegular=[this] { events.push_back("N:40");return Number(input[7])!=0; };hooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { regular=host.AddRegular(sid,event,payload);events.push_back("B:46:"+std::to_string(sid.value)+":0:80"); };
        player=std::make_unique<RawPlayerWorld>(pool,post,records,state,std::move(hooks));post.SetPlayerPostPop(player.get());
    }
    // C++ Kernel이 프로세스의 훅 참조보다 먼저 정리되게 한다.
    ~PlayerReplayScene() { kernel.Clear();post.SetPlayerPostPop(nullptr); }
    // 네 actualbody를 실행한 뒤 전체raw/Player/장부/공간/Form/Kernel을 원본 열 순서로 직렬화한다.
    std::vector<std::string> Run() {
        const auto kind=Number(input[0]);Sid sid{5};std::uint32_t result=0,assertions=0;
        if (kind==0) { sid=player->Create(std::bit_cast<std::int32_t>(Number(input[1])));result=sid.value; }
        else if (kind==1) { sid=factory.Create(152,2);pool.AllocatedBytes(sid)[pool.Edition()==OriginalEdition::Patch1078 ? 34U : 32U]=static_cast<std::uint8_t>(Number(input[1]));book.depth=1;player->PostPop(sid,Number(input[8])); }
        else result=kind==2 ? player->CurrentTechnology() : player->Technology(std::bit_cast<std::int32_t>(Number(input[1])));
        std::string actions,counts="0,0",active,fields="-"; // 현재 실제 목록/관찰 및 Regular 원본 저장 비트다.
        // 같은 명시 경계의 호출 순서와 실제 진단 횟수를 기록한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event;if (event.starts_with("A:")) ++assertions; }
        // 전체9개 빈 owner 목록의 count도 원본과 비교한다.
        for (const auto& list:book.ownerFactories) counts+=','+std::to_string(list.count);
        // Kernel 슬롯 순서를 그대로 읽어 실제 PID만 기록한다.
        for (ProcessId pid=1;pid<=kernel.Capacity();++pid) if (kernel.Get(pid)) { if (!active.empty()) active+=',';active+=std::to_string(pid); }
        if (regular) fields=std::to_string(std::bit_cast<std::uint64_t>(regular->Time()))+','+std::to_string(regular->Event())+','+std::to_string(std::bit_cast<std::uint32_t>(regular->Payload()))+','+std::to_string(regular->Count());
        const auto form=regular ? regular->Form() : Sid{};std::vector<std::uint16_t> heads; // 실제 네 배열 전체의 WORD 체크섬 입력이다.
        // 현재hash를X/Y 재정렬 없이 원본 네 단계 배열 순서로 직렬화한다.
        for (int level=0;level<4;++level) { const auto entries=hash.Entries(level);heads.insert(heads.end(),entries.begin(),entries.end()); }
        return {std::to_string(result),actions.empty() ? "-" : actions,Hex(records),Hex(pool.Slot(sid)),std::to_string(WordAdler<std::uint16_t>(heads)),std::to_string(Adler(spots)),
            std::to_string(book.totalCost),std::to_string(book.productionDirty),std::to_string(book.depth),std::to_string(WordAdler<std::uint32_t>(book.localCounts)),std::to_string(WordAdler<std::uint32_t>(book.localSecondaryCounts)),std::to_string(WordAdler<std::uint32_t>(book.globalCounts)),
            counts,"1",fields,std::to_string(form.value),form.value ? Hex(pool.Slot(form)) : "-",active.empty() ? "-" : active,std::to_string(clock.typeRotation),std::to_string(assertions)};
    }
};
// 동일 판본의 원본 고유행 모두를 같은 실제 소비자에서 재생한다.
void ReplayPlayer(std::string_view edition) {
    std::size_t count=0;int failures=0;
    // 기대 출력은 Run 후에만 비교하며 입력 쪽으로 복사하지 않는다.
    for (const auto& row:PlayerRows()) { if (row[0]!=edition) continue;CHECK(row.size()==22);PlayerReplayScene scene(row);const auto output=scene.Run();const auto expected=std::vector<std::string>(row.begin()+2,row.end());CHECK(output==expected);
        if (output!=expected) { for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s Player 행%zu 열%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());if (++failures>=5) break; }++count;
    }
    CHECK(failures==0 && count==840);
}
}
// 패치 네 몸체840개를 실제 Factory/Pop/Owner/ProcessForm/Kernel와 대조한다.
TEST_CASE(PlayerWorld_PatchNativeAndActualHost) { ReplayPlayer("originals"); }
// CD의 BYTE frame·실제36BYTE Form·별도9기록 getter840개를 대조한다.
TEST_CASE(PlayerWorld_CdNativeAndActualHost) { ReplayPlayer("originalCD"); }
// 실제 RawSurfaceWorld.Core의 같은Factory/Pool/Hash와 두owner/현재9기록/Host를 연결한다.
TEST_CASE(PlayerWorld_SameSurfaceCoreRecordsAndInitialRegular) {
    // 두 판본의 동일 월드 저장소를 각각 직접 소비한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto& assets=Assets(edition);BridgeConnectState bridge;bridge.bridgeType=82;bridge.islandType=94;bridge.stalagType=95;bridge.connectorType=155;bridge.noIslandType=157;bridge.battle=true;bridge.mission=true;
        netstorm::client::RawSurfaceWorld world(edition,assets.types,assets.frames,assets.shapes,bridge,97);auto core=world.Core();std::vector<std::uint8_t> records(9*(edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU));PlayerWorldState state{1,1,8,152,false,false};std::uint32_t diagnostics=0;core.processState.now=17.25;
        PlayerWorldHooks hooks;hooks.assertion=[&](PlayerWorldAssertion) { ++diagnostics; };hooks.find=[&](float x,float y,std::uint32_t type) { return world.Find(x,y,type); };hooks.create=[&](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        hooks.setOwner=[&](Sid sid,std::uint32_t value) { core.owner.Set(sid,value); };hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };hooks.reserveRegular=[] { return true; };hooks.addRegular=[&](Sid sid,std::uint32_t event,float payload) { core.processes.AddRegular(sid,event,payload); };
        RawPlayerWorld player(core.pool,core.postPop,records,state,std::move(hooks));core.postPop.SetPlayerPostPop(&player);const auto sid=player.Create(1);const auto* regular=core.processes.FindEvent(sid,1);
        CHECK(sid==Sid{5} && player.CurrentTechnology()==5 && player.Technology(1)==5 && player.Technology(8)==0 && state.templateOwner==8 && diagnostics==0);
        CHECK(regular && regular->Time()==17.25 && regular->Payload()==5.0f && regular->Count()==0 && core.kernel.Size()==1 && core.processes.FromForm(regular->Form())==regular);
        const auto raw=core.pool.Slot(sid);CHECK(raw[10]==152 && std::bit_cast<float>(Get(raw,14))==11.0f && std::bit_cast<float>(Get(raw,18))==1.0f && core.bookkeeping.depth==0);
        CHECK(&player.Pool()==&core.factory.Pool() && &player.Pool()==&core.processes.Pool());world.ClearProcesses();CHECK(core.kernel.Size()==0 && core.processes.FindEvent(sid,1)==nullptr);core.postPop.SetPlayerPostPop(nullptr);
    }
}

namespace { // 실제9기록/물리 슬롯/공통 부모에 선택 효과만 연결한다.
struct PlayerCurrentScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true};std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188); // 현재 PATCH 물리 슬롯과 공통 타입 표다.
    SquidPostPopState book;SquidPostPop base{pool,types,book}; // 같은 공통 부모와 실제 중첩 깊이다.
    std::vector<std::uint8_t> records=std::vector<std::uint8_t>(9*0xb4);PlayerWorldState state; // 같은 현재9기록과 예약/생성 전역이다.
    std::span<std::uint8_t> raw;std::vector<std::string> events; // 관찰 대상5의 현재 뷰와 도달한 하위 순서다.
    PlayerCurrentScene() { CHECK(pool.Allocate(2)==Sid{5});raw=pool.AllocatedBytes(Sid{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x50ee08);raw[10]=74;raw[34]=1;Put(raw,36,0xdeadbeef);book.depth=1;book.suppressed=true;base.BindCurrentTypes(types); } // 결과를 대입하지 않고 별도 TYPE/프레임을 가진 입력만 준비한다.
    PlayerWorldHooks Hooks() { PlayerWorldHooks hooks;hooks.reserveRegular=[this] { events.push_back("N");return true; };hooks.addRegular=[this](Sid sid,std::uint32_t event,float payload) { CHECK(sid==Sid{5} && event==1 && payload==5);events.push_back("R"); };return hooks; } // 예약 인자/순서만 관찰하고 기록/프레임 쓰기는 실제 제품에 맡긴다.
};
}
TEST_CASE(PlayerParentCurrent_PhysicalVtableNoFirstAndNoUnusedDevices) { // 실제 PATCH VT는 다른 TYPE/free/contained에서도 기록을 먼저 쓰며 별도 Validate는 기존 계약이다.
    PlayerCurrentScene scene;scene.raw[11]=1;scene.raw[40]=8;RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,{});scene.base.SetPlayerPostPop(&body);CHECK(body.Handles(Sid{5}) && Throws([&] { body.Validate(Sid{5},0); }));scene.base.PostPopVirtual(Sid{5},0);CHECK(Get(scene.records,0xb4+0x9c)==5 && scene.book.depth==0 && Get(scene.raw,36)==0xdeadbeef);
}
TEST_CASE(PlayerParentCurrent_CurrentGetterSkipsUnknownInvalidOwner) { // 아직 PlayerKnown이 없으면 현재 owner/장치를 읽지 않고0을 반환한다.
    PlayerCurrentScene scene;scene.state.battleOwner=65535;RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,{});CHECK(body.CurrentTechnology()==0);scene.state.playerKnown=1;CHECK(Throws([&] { body.CurrentTechnology(); }));
}
TEST_CASE(PlayerParentCurrent_MissingReserveKeepsTechnologyBeforeFrame) { // 도달한 확보 실패는 먼저 쓴 Technology SID를 보존하고 special 프레임에는 도달하지 않는다.
    PlayerCurrentScene scene;scene.state.special=1;RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,{});CHECK(Throws([&] { body.PostPop(Sid{5},1); }) && Get(scene.records,0xb4+0x9c)==5 && Get(scene.raw,36)==0xdeadbeef && scene.book.depth==1);
}
TEST_CASE(PlayerParentCurrent_FailedReserveUsesCurrentSpecialAndPhysicalFrame) { // 확보 효과 뒤 바뀐 special을 읽고 현재 free/TYPE/VT와 무관하게 직접 frame1을 쓴다.
    PlayerCurrentScene scene;auto hooks=scene.Hooks();hooks.addRegular={};hooks.reserveRegular=[&] { scene.events.push_back("N");CHECK(Get(scene.records,0xb4+0x9c)==5);scene.state.special=256;scene.state.battleOwner=8;scene.raw[34]=8;scene.raw[11]=1;scene.raw[40]=8;scene.raw[10]=255;Put(scene.raw,0,0);return false; };RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,std::move(hooks));body.PostPop(Sid{5},1);CHECK(scene.events==std::vector<std::string>{"N"} && Get(scene.records,0xb4+0x9c)==5 && Get(scene.records,8*0xb4+0x9c)==0 && Get(scene.raw,36)==1 && scene.book.depth==0 && scene.raw[11]==1);
}
TEST_CASE(PlayerParentCurrent_MissingConstructorKeepsSuccessfulReservation) { // 성공한 확보 뒤 생성자 장치 실패는 기록/확보를 남기고 special/공통 부모를 실행하지 않는다.
    PlayerCurrentScene scene;auto hooks=scene.Hooks();hooks.addRegular={};RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,std::move(hooks));CHECK(Throws([&] { body.PostPop(Sid{5},1); }) && scene.events==std::vector<std::string>{"N"} && Get(scene.records,0xb4+0x9c)==5 && scene.book.depth==1);
}
TEST_CASE(PlayerParentCurrent_RegularMutationCanDisableLaterSpecialFrame) { // 생성 효과 뒤 special을 다시 읽으며 처음 선택한 local/fort 관문을 재검사하지 않는다.
    PlayerCurrentScene scene;scene.state.special=1;auto hooks=scene.Hooks();hooks.addRegular=[&](auto,auto,auto) { scene.events.push_back("R");scene.state.special=0;scene.state.fortMode=true;scene.raw[34]=0;scene.raw[11]=1; };RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,std::move(hooks));body.PostPop(Sid{5},1);CHECK(scene.events.size()==2 && scene.events[0]=="N" && scene.events[1]=="R" && Get(scene.raw,36)==0xdeadbeef && Get(scene.records,0xb4+0x9c)==5 && Get(scene.records,0x9c)==0 && scene.book.depth==0);
}
TEST_CASE(PlayerParentCurrent_FullLocalOwnerDwordDoesNotSelectReservation) { // BYTE owner1과 현재 DWORD257은 다르지만 special 첫 프레임은 독립해서 실행한다.
    PlayerCurrentScene scene;scene.state.battleOwner=257;scene.state.special=1;RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,{});body.PostPop(Sid{5},1);CHECK(Get(scene.raw,36)==1 && Get(scene.records,0xb4+0x9c)==5 && scene.book.depth==0);
}
TEST_CASE(PlayerParentCurrent_FortAndNonFirstNeedNoReservation) { // fort 또는 최초 비트가 없는 입력은 예약 장치 없이 현재 기록과 공통 부모를 갱신한다.
    for (const auto flags:{1U,2U}) { // 서로 다른 두 미선택 원문 관문을 독립 장면에서 확인한다.
        PlayerCurrentScene scene;scene.state.fortMode=flags==1;scene.state.special=flags==2;RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,{});body.PostPop(Sid{5},flags);CHECK(Get(scene.records,0xb4+0x9c)==5 && Get(scene.raw,36)==0xdeadbeef && scene.book.depth==0);
    }
}
TEST_CASE(PlayerParentCurrent_LateCommonFailureKeepsLinkReservationAndFrame) { // 공통 부모의 선택 재검사 실패 앞에 실제 Player 기록/확보/special 쓰기와 공통 통계가 남는다.
    PlayerCurrentScene scene;scene.book.suppressed=false;scene.book.pendingPlacement=true;scene.book.localOwner=1;scene.state.special=1;auto hooks=scene.Hooks();hooks.reserveRegular=[&] { scene.events.push_back("N");return false; };RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,std::move(hooks));CHECK(Throws([&] { body.PostPop(Sid{5},0x2000001); }) && Get(scene.records,0xb4+0x9c)==5 && Get(scene.raw,36)==1 && scene.book.GlobalCounts()[74]==1 && scene.book.depth==1 && scene.events.size()==1);
}
TEST_CASE(PlayerParentCurrent_InvalidPhysicalOwnerFailsBeforeReservation) { // 실제9기록 밖 owner를 원문 진단으로 꾸미지 않고 물리 저장 직전에 거부한다.
    PlayerCurrentScene scene;scene.raw[34]=9;const auto before=scene.records;RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,scene.Hooks());CHECK(Throws([&] { body.PostPop(Sid{5},1); }) && scene.records==before && scene.events.empty() && scene.book.depth==1);
}
TEST_CASE(PlayerParentCurrent_CreateUsesCurrentTypeAfterDiagnosticsAndPhysicalFrame) { // 진단 복귀 뒤 현재 TYPE를 재조회하고 Owner 효과 뒤 물리 frame0을 쓴 다음 저장한 좌표로 Pop한다.
    PlayerCurrentScene scene;auto hooks=scene.Hooks();hooks.assertion=[&](auto kind) { scene.events.push_back(kind==PlayerWorldAssertion::PlayerAlreadyExists ? "A2" : "A1");if (kind==PlayerWorldAssertion::PlayerAlreadyExists) scene.state.playerType=76;else { scene.state.playerType=74;scene.state.special=1; } };
    hooks.find=[&](float x,float y,auto type) { CHECK(x==18 && y==10 && type==74);scene.events.push_back("F");scene.state.playerType=75;scene.state.special=0;return Sid{6}; };hooks.create=[&](auto type,auto flags) { CHECK(type==76 && flags==2);scene.events.push_back("C");return Sid{5}; };hooks.setOwner=[&](auto sid,auto owner) { CHECK(sid==Sid{5} && owner==8);scene.events.push_back("O");scene.raw[11]=1;scene.raw[40]=8; };
    hooks.pop=[&](auto sid,float x,float y,auto flags) { CHECK(sid==Sid{5} && x==18 && y==10 && flags==0 && Get(scene.raw,36)==0 && scene.raw[11]==1);scene.events.push_back("P"); };RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,std::move(hooks));CHECK(body.Create(8)==Sid{5} && scene.events.size()==6 && scene.events[0]=="A1" && scene.events[1]=="F" && scene.events[2]=="A2" && scene.events[3]=="C" && scene.events[4]=="O" && scene.events[5]=="P");
}
TEST_CASE(PlayerParentCurrent_CreateMissingPopKeepsOwnerAndFrameStore) { // 실제 생성/Owner 뒤 미연결 Pop에 도달하기 전에 직접 frame0이 남는다.
    PlayerCurrentScene scene;scene.state.playerKnown=1;PlayerWorldHooks hooks;hooks.find=[](auto,auto,auto) { return Sid{}; };hooks.create=[](auto,auto) { return Sid{5}; };hooks.setOwner=[&](auto,auto owner) { scene.raw[34]=static_cast<std::uint8_t>(owner);scene.raw[11]=1; };RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,std::move(hooks));CHECK(Throws([&] { body.Create(8); }) && scene.raw[34]==8 && scene.raw[11]==1 && Get(scene.raw,36)==0);
}
TEST_CASE(PlayerParentCurrent_TechnologyDiagnosticReturnsCurrentExplicitRecord) { // 진단 콜백이 갱신한 명시 owner 기록을 읽으며 현재 local owner로 바꾸지 않는다.
    PlayerCurrentScene scene;PlayerWorldHooks hooks;hooks.assertion=[&](auto kind) { CHECK(kind==PlayerWorldAssertion::PlayerIdKnown);scene.events.push_back("A");Put(scene.records,8*0xb4+0x9c,0x12345678);scene.state.battleOwner=1; };RawPlayerWorld body(scene.pool,scene.base,scene.records,scene.state,std::move(hooks));CHECK(body.Technology(8)==0x12345678 && scene.events.size()==1 && body.CurrentTechnology()==0);
}
TEST_CASE(PlayerParentCurrent_CdAndPhysicalStorageContractsRemain) { // CD 구성의 전수 장치와 PATCH 실제 저장 크기 경계를 유지한다.
    PlayerCurrentScene scene;CHECK(Throws([&] { RawPlayerWorld invalid(scene.pool,scene.base,std::span(scene.records).first(8*0xb4),scene.state,{}); }));SidPool pool(OriginalEdition::Cd1072,24000,true);std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(171);SquidPostPopState book;SquidPostPop base(pool,types,book);std::vector<std::uint8_t> records(9*0xac);CHECK(Throws([&] { RawPlayerWorld missing(pool,base,records,scene.state,{}); }));
}
