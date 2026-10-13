// 실제 Player Handler/State/Notify와 같은 Host의0→5→65초 예약 사슬을 원본 관찰과 대조한다.
#include "DudeSpawnSupport.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawPlayerRegular.h"
#include "o/RawPlayerTemplateState.h"
#include "o/Template.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
using namespace netstorm::test::dudespawn;
namespace {
// fixture 현재 입력의 고정열이며 기대 처리 결과를 포함하지 않는다.
enum PlayerRegularField:std::size_t {
    Kind, // 직접Handler/State/Notify를선택한다.
    Owner, // 실제Player raw owner BYTE다.
    Local, // 현재battleOwner DWORD다.
    TemplateOwner, // 현재Template owner DWORD다.
    Battle, // 현재inBattleMode다.
    Event, // 실제1/2/미지정Handler 사건번호다.
    Count, // 실제Regular의현재호출횟수다.
    Reserve, // new40 성공/실패입력이다.
    Payload, // 원래float의전체32비트다.
    Mine, // 현재Template+8 DWORD다.
    Transient, // 현재Template+12 DWORD다.
    Prevent, // 현재Template+16 DWORD다.
    Mission, // 실제inMission 전역이다.
    Dot, // manager.fileName이'.'로시작하는가다.
    End, // 현재State MemStream.end DWORD다.
    Mutation // 하위효과뒤현재전역변경을선택한다.
};
// 이 직접Handler 범위 밖의 Player 최초Pop/전송/실제UI를 기본성공으로 바꾸지 않는다.
[[noreturn]] void MissingRegularEffect() { throw std::logic_error("Player Regular 선택 밖의 효과 미복원"); }
// 직접Handler 검사는 Player wrapper의 생성/Pop을 호출하지 않으므로 경계는 모두 명시 거부다.
PlayerWorldHooks UnselectedPlayerHooks() {
    PlayerWorldHooks hooks;hooks.assertion=[](PlayerWorldAssertion) { MissingRegularEffect(); };hooks.find=[](float,float,std::uint32_t)->Sid { MissingRegularEffect(); };
    hooks.create=[](std::uint32_t,std::uint32_t)->Sid { MissingRegularEffect(); };hooks.setOwner=[](Sid,std::uint32_t) { MissingRegularEffect(); };hooks.pop=[](Sid,float,float,std::uint32_t) { MissingRegularEffect(); };
    hooks.reserveRegular=[]()->bool { MissingRegularEffect(); };hooks.addRegular=[](Sid,std::uint32_t,float) { MissingRegularEffect(); };return hooks;
}
// native와 같은 manager 주소 비트/State 데이터/현재cursor를 입력으로 준비한다. 주소를 역참조하지 않는다.
void PrepareTemplateState(std::span<std::uint8_t> headers,std::array<std::vector<std::uint8_t>,9>& data,
    std::array<PlayerTemplateStateStream,9>& streams,std::array<std::string,9>& names,const std::vector<std::uint32_t>& input) {
    // 실제9기록의 각각 다른바이트와 stale cursor를 보존한다.
    for (std::size_t owner=0;owner<9;++owner) {
        Put(headers,owner*20,0x12009000U+static_cast<std::uint32_t>(owner)*0x100U);Put(headers,owner*20+8,input[Mine]);Put(headers,owner*20+12,input[Transient]);Put(headers,owner*20+16,input[Prevent]);
        data[owner].resize(64);data[owner][0]=static_cast<std::uint8_t>(0x70U+owner);
        // 관찰된State의꼬리를기대OR결과없이명시입력으로준비한다.
        for (std::size_t index=1;index<64;++index) data[owner][index]=static_cast<std::uint8_t>(index*7+owner);
        streams[owner]={data[owner],7,input[End],true};names[owner]=input[Dot] ? ".map" : "map.fort";
    }
}
// 새 두 몸체와 기존Player/Factory/Regular/Form/Kernel를 같은 현재저장소에 구성한다.
struct PlayerRegularScene {
    std::vector<std::uint32_t> input; // 독립 원본에 전달한16개DWORD다.
    SidPool pool; // 신규Player5/실제Form6의유일한raw풀이다.
    const DudeAssets& assets; // 현재보호152 ctor/type/frame/SHP를읽기재사용한다.
    SquidHash hash; // 직접Handler는공간을변경하지않는현재해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 같은Frame/수명의점유저장소다.
    SquidUnpop unpop; // 실제Form수명과같은공간계약이다.
    SquidPostPopState book; // 이직접body는공통Pop장부를호출하지않는다.
    SquidPostPop post; // 실제Player 참조/VT 판정을 위한 공통모듈이다.
    SquidFactory factory; // 보호152 생성자를 실제실행한다.
    RawSquidDestroy destroy; // 실제Form삭제가쓰는공통수명이다.
    Kernel kernel; // 새Regular2 PID와포인터소유권이다.
    SquidProcessState clock; // 실제초기17.25 double시각이다.
    SquidProcessHost host; // 실제Regularctor/ProcessForm부모부착이다.
    SquidOwnerMode mode; // 매로그후재조회하는inBattleMode다.
    PlayerWorldState playerState; // getter/Notify 전 현재두owner를공유한다.
    std::vector<std::uint8_t> players; // 실제9개Player BYTE 저장소다.
    RawPlayerWorld player; // 실제VT/현재상태접근을재사용한다.
    std::array<std::uint8_t,180> headers{}; // 같은현재Template9 header다.
    std::array<std::vector<std::uint8_t>,9> data; // 각기록의현재물리State64 BYTE다.
    std::array<PlayerTemplateStateStream,9> streams; // getter의currentpos/end를보존한다.
    std::array<std::string,9> names; // 실제Notify의현재manager이름이다.
    std::uint32_t depth{0xffffffff}; // CD즉시증감의DWORD감김입력이다.
    bool mission{}; // Notify의실제미션전역이다.
    std::vector<std::string> events; // 실제로그/진단/예약/State/Notify 순서다.
    std::uint32_t assertions{}; // 미복원거부와다른원본진단수다.
    RegularProcess* current{}; // 실제Host가만든Regular2를실행후읽는다.
    std::unique_ptr<RawPlayerTemplateState> templateState; // 현재9stream/header를직접빌린다.
    std::unique_ptr<RawPlayerRegular> regular; // 위상태/Host의수명안에서실제handler를제공한다.
    Sid sid{5}; // 첫실제Factory가배정한client번호다.
    // 기대출력없이현재입력만구성한다.
    explicit PlayerRegularScene(const std::vector<std::string>& row):pool(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,32768,true),assets(Assets(pool.Edition())),unpop(pool,hash,spots),post(pool,assets.types,book),factory(pool,assets.types,false,&unpop),destroy(pool,unpop,assets.types),
        kernel(pool.Edition()==OriginalEdition::Patch1078 ? kProcessCapacity : kCdProcessCapacity),host(pool,assets.types,kernel,destroy,clock,[](Sid,std::uint32_t,std::uint32_t,float)->float { MissingRegularEffect(); }),players(9*(pool.Edition()==OriginalEdition::Patch1078 ? 0xb4U : 0xacU)),player(pool,post,players,playerState,UnselectedPlayerHooks()) {
        // 원본전달순서대로각필드를읽는다.
        for (const auto& item:Split(row[1],',')) input.push_back(Number(item));CHECK(input.size()==16);playerState.battleOwner=input[Local];playerState.templateOwner=input[TemplateOwner];playerState.playerKnown=1;mode.battle=input[Battle]!=0;mission=input[Mission]!=0;clock.now=17.25;
        sid=factory.Create(152,2);pool.AllocatedBytes(sid)[pool.Edition()==OriginalEdition::Patch1078 ? 34U : 32U]=static_cast<std::uint8_t>(input[Owner]);PrepareTemplateState(headers,data,streams,names,input);
        templateState=std::make_unique<RawPlayerTemplateState>(pool.Edition(),headers,streams,names,depth,mission,[this](PlayerTemplateStateAssertion reason) { const bool patch=pool.Edition()==OriginalEdition::Patch1078;
            const auto line=reason==PlayerTemplateStateAssertion::DotFileName ? (patch ? 0x1c1U : 0x1b9U) : reason==PlayerTemplateStateAssertion::MineTransientFlag ? (patch ? 0x1c3U : 0x1bbU) : reason==PlayerTemplateStateAssertion::InMissionSave ? (patch ? 0x1c6U : 0x1beU) : reason==PlayerTemplateStateAssertion::MissingStateTemplate ? (patch ? 0x1fcU : 0x1f4U) : (patch ? 0x1c7U : 0x1bfU);events.push_back("A:"+std::to_string(line));++assertions;
        },[this](std::uint32_t value) { CHECK(value==depth); }); // 실제현재depth 참조의즉시증감값을현재저장소와대조한다.
        PlayerRegularHooks hooks;hooks.assertion=[this](PlayerRegularAssertion) { events.push_back("A:"+std::to_string(pool.Edition()==OriginalEdition::Patch1078 ? 0x41dU : 0x3c9U));++assertions; };
        hooks.log=[this](std::uint32_t group,std::string_view text) { events.push_back("L:"+std::to_string(group)+':'+Hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())));if (text=="SORE LOSER TEST: " && input[Mutation]==1) mode.battle=false;if (text=="SORE LOSER TEST: " && input[Mutation]==2) mode.battle=true; };
        hooks.reserveRegular=[this] { events.push_back("N:40");return input[Reserve]!=0; };hooks.addRegular=[this](Sid parent,std::uint32_t event,float payload) { current=host.AddRegular(parent,event,payload);events.push_back("B:46:"+std::to_string(parent.value)+":0:80"); };
        hooks.templateState=[this](std::uint32_t slot) { events.push_back("T:"+std::to_string(slot));auto result=templateState->Get(slot);if (input[Mutation]==3) playerState.templateOwner=3;return result; };
        hooks.templateChanged=[this](std::uint32_t slot) { events.push_back("C:"+std::to_string(slot));if (input[Mutation]==4) Put(headers,slot*20+8,0);templateState->Changed(slot); };
        regular=std::make_unique<RawPlayerRegular>(player,mode,std::move(hooks));
    }
    // 실제Form/Kernel을참조모듈보다먼저정리한다.
    ~PlayerRegularScene() { kernel.Clear(); }
    // 실제선택body를실행하며미복원경계에는반환float를채우지않는다.
    std::vector<std::string> Run() {
        std::string status="return",returned="-",boundary="-",actions,states,fields="-",active;
        try { if (input[Kind]==0) returned=std::to_string(std::bit_cast<std::uint32_t>(regular->Handle(sid,input[Event],input[Count],std::bit_cast<float>(input[Payload]))));else if (input[Kind]==1) { events.push_back("T:"+std::to_string(input[TemplateOwner]));templateState->Get(input[TemplateOwner]); }else { events.push_back("C:"+std::to_string(input[TemplateOwner]));templateState->Changed(input[TemplateOwner]); } }
        catch (const std::logic_error& error) {
            const std::string message=error.what();const bool patch=pool.Edition()==OriginalEdition::Patch1078;status="boundary";returned="-";
            if (message=="Player 없는State초기화/stack미정BYTE/힙생성미복원") boundary=patch ? "0044d1b0" : "0044c3c0";
            else if (message=="Player shortState실제DataManager힙확장미복원") boundary=patch ? "004e4391" : "004f1650";
            else if (message=="Player Template실제DataManager파일저장44d8d0/CD44cc40미복원") boundary=patch ? "0044d8d0" : "0044cc40";
            else throw;
        }
        // getter뒤현재pos/end와전체State28BYTE를이전padding까지포함해보존한다.
        for (std::size_t owner=0;owner<9;++owner) { if (!states.empty()) states+=';';states+=std::to_string(streams[owner].position)+','+std::to_string(streams[owner].end)+','+Hex(std::span(data[owner]).first(28)); }
        // 원본 effect순서와실제Kernel슬롯을그대로직렬화한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        for (ProcessId pid=1;pid<=kernel.Capacity();++pid) if (kernel.Get(pid)) { if (!active.empty()) active+=',';active+=std::to_string(pid); }
        if (current) fields=std::to_string(std::bit_cast<std::uint64_t>(current->Time()))+','+std::to_string(current->Event())+','+std::to_string(std::bit_cast<std::uint32_t>(current->Payload()))+','+std::to_string(current->Count());const auto form=current ? current->Form() : Sid{};
        return {status,returned,boundary,actions.empty() ? "-" : actions,Hex(headers),states,std::to_string(playerState.battleOwner),std::to_string(playerState.templateOwner),mode.battle ? "1" : "0",std::to_string(depth),fields,Hex(pool.Slot(sid)),std::to_string(form.value),form.value ? Hex(pool.Slot(form)) : "-",active.empty() ? "-" : active,std::to_string(clock.typeRotation),std::to_string(assertions)};
    }
};
// 각판본의고유원본행을정상/assert/미복원거부까지정확비교한다.
void ReplayPlayerRegular(std::string_view edition) {
    const auto fixture=LoadFixture(NETSTORM_PLAYER_REGULAR_FIXTURE);std::size_t count=0;int failures=0;
    // 출력은실제body실행뒤에만비교한다.
    for (const auto& row:fixture.rows) { if (row[0]!=edition) continue;CHECK(row.size()==19);PlayerRegularScene scene(row);const auto output=scene.Run();const auto expected=std::vector<std::string>(row.begin()+2,row.end());CHECK(output==expected);
        if (output!=expected) { for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("PlayerRegular %s row%zu col%zu expected%s actual%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());if (++failures>=5) break; }++count;
    }
    CHECK(failures==0 && count==329);
}
}
// 패치Handler/Statehelper와실제Regular/Form/Kernel의독립전수다.
TEST_CASE(PlayerRegular_PatchNativeStateAndActualHost) { ReplayPlayerRegular("originals"); }
// CD의즉시depth증감/상태/각진단을같은현재자료로전수대조한다.
TEST_CASE(PlayerRegular_CdNativeStateAndActualHost) { ReplayPlayerRegular("originalCD"); }
// 같은실제월드에서Player첫예약→5초후새Regular2→같은Kernelpass의count0→65초State/Notify/제거다.
TEST_CASE(PlayerRegular_SameWorldInitialFiveAndSixtySecondChain) {
    // 두판본은현재보호Player자료와현재State28을각각소유한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto& assets=Assets(edition);netstorm::client::RawSurfaceWorld world(edition,assets.types,assets.frames,assets.shapes,{},97);auto core=world.Core();core.ownerMode.battle=true;
        PlayerWorldState state{1,1,2,152,false,false};std::vector<std::uint8_t> players(9*(edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU));std::array<std::uint8_t,180> headers{};
        std::array<std::vector<std::uint8_t>,9> data;std::array<PlayerTemplateStateStream,9> streams;std::array<std::string,9> names;std::uint32_t depth=1,diagnostics=0,changed=0;bool mission=true;core.processState.now=17.25;
        const auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());const auto stored=fort.Section("State");CHECK(stored.size()==28 && stored[0]==0x70);
        // 원본두owner를다르게설정하고currentTemplate의existing28/isMine1을그대로공급한다.
        for (std::size_t slot=0;slot<9;++slot) { Put(headers,slot*20,1);Put(headers,slot*20+8,1);data[slot].assign(stored.begin(),stored.end());streams[slot]={data[slot],7,28,true};names[slot]="thewarbegins.fort"; }
        RawPlayerTemplateState templateState(edition,headers,streams,names,depth,mission,[&](PlayerTemplateStateAssertion) { ++diagnostics; },[&](std::uint32_t value) { CHECK(depth==value); });
        PlayerWorldHooks playerHooks;playerHooks.assertion=[&](PlayerWorldAssertion) { ++diagnostics; };playerHooks.find=[&](float x,float y,std::uint32_t type) { return world.Find(x,y,type); };playerHooks.create=[&](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
        playerHooks.setOwner=[&](Sid sid,std::uint32_t owner) { core.owner.Set(sid,owner); };playerHooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };playerHooks.reserveRegular=[] { return true; };playerHooks.addRegular=[&](Sid sid,std::uint32_t event,float payload) { core.processes.AddRegular(sid,event,payload); };
        RawPlayerWorld player(core.pool,core.postPop,players,state,std::move(playerHooks));core.postPop.SetPlayerPostPop(&player);std::vector<std::string> logs;
        PlayerRegularHooks hooks;hooks.assertion=[&](PlayerRegularAssertion) { ++diagnostics; };hooks.log=[&](std::uint32_t group,std::string_view message) { CHECK(group==29);logs.emplace_back(message); };
        hooks.reserveRegular=[] { return true; };hooks.addRegular=[&](Sid sid,std::uint32_t event,float payload) { core.processes.AddRegular(sid,event,payload); };hooks.templateState=[&](std::uint32_t slot) { return templateState.Get(slot); };hooks.templateChanged=[&](std::uint32_t slot) { ++changed;templateState.Changed(slot); };
        RawPlayerRegular regular(player,core.ownerMode,std::move(hooks));core.processes.SetRegularHandler([&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return regular.Handle(sid,event,count,payload); });const auto sid=player.Create(1);CHECK(core.kernel.Size()==1);
        core.kernel.RunFrame();const auto* first=core.processes.FindEvent(sid,1);CHECK(first && first->Time()==22.25 && first->Count()==1 && first->Payload()==5.0f && logs.empty() && data[2][0]==0x70);
        core.processState.now=22.25;core.kernel.RunFrame();const auto* second=core.processes.FindEvent(sid,2);CHECK(core.processes.FindEvent(sid,1)==nullptr && second && second->Time()==82.25 && second->Count()==1 && second->Payload()==60.0f && core.kernel.Size()==1);
        CHECK(logs==std::vector<std::string>({"SORE LOSER TEST: ","STARTING.\n"}) && data[2][0]==0x70 && changed==0);
        core.processState.now=82.25;core.kernel.RunFrame();CHECK(core.kernel.Size()==0 && core.processes.FindEvent(sid,2)==nullptr && Get(core.pool.Slot(sid),6,2)==0 && data[2][0]==0x74 && data[1][0]==0x70 && state.templateOwner==2 && state.battleOwner==1 && changed==1 && diagnostics==0 && depth==1 && streams[2].position==0 && streams[2].end==28);
        world.ClearProcesses();core.postPop.SetPlayerPostPop(nullptr);
    }
}
