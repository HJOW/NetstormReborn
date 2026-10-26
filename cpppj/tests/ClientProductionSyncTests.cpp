// 실제 Client/Player/Pool/공통 UI로 World 없는 HUD mode1과 다음 Native 정리를 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientPlayerRuntime.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientProductionSync.h"
#include "client/ClientStateRuntimeRetirement.h"
#include "client/RawButtonControls.h"
#include "client/RawProductionControls.h"
#include <algorithm>
#include <stdexcept>

using namespace netstorm::client;
namespace { // 보호 자산은 읽기만 하며 창/World/Session을 생성하지 않는다.
ClientOptions NoWorldProductionOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; } //실제 Client의 장기 전역만 만든다.
std::uint32_t NoWorldProductionRead(std::span<const std::uint8_t> bytes,std::size_t offset) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; } //인접 필드를 섞지 않는 명시 검사 읽기다.
void NoWorldProductionPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) { for (std::size_t byte=0;byte<width;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //손상/선택 경계의 실제 raw 입력만 준비한다.
template<class Operation> std::string NoWorldProductionFailure(Operation operation) { try { operation(); }catch (const std::logic_error& error) { return error.what(); }return {}; } //현재 실제 실패 위치만 비교한다.
struct NoWorldProductionScene {
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); //같은 보호 TYPE/SHP/글꼴이다.
    Client client{NoWorldProductionOptions()};FontStore fonts{assets.files,"Arial"}; //가상 장치/World 없이 실제 장기 Player/게임 전역을 소유한다.
    std::shared_ptr<RawSurfaceStorage> storage=MakeRawSurfaceStorage(assets.game,client.GameplayGlobals()->Authority(),client.GameplayGlobals()->GameAux()); //Client 전역과 같은 실제 물리/권한이다.
    std::array<std::uint8_t,87> options{};RawButtonControls buttons{OriginalEdition::Patch1078,{[] { return 100.0; }}};RawProductionControls* panel{}; //실제 공통 트리와 생산 파생 UI다.
    NoWorldProductionScene() {
        InitializeClientPlayerRecords(client);storage->Processes().BindDevices(client.ProcessKernelOwner(),client.FrameClockOwner()); //원문 공통 시작의 현재9기록과 같은 실제 Kernel만 연결한다.
        RawMenuHooks menus;menus.command=[](auto,auto,auto) { throw std::logic_error("검사에서 미선택 메뉴 명령"); };menus.readInt=[](auto,std::int32_t& value) { value=0; };menus.sound=[](auto) { throw std::logic_error("검사에서 미선택 메뉴 음향"); };menus.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };menus.cursor=[](auto) { throw std::logic_error("검사에서 미선택 커서 장치"); };menus.gameSeconds=[] { return 0.0; };buttons.EnableMenus(assets.game,fonts,std::move(menus));
        const auto& background=assets.game.Find("fortGump");const auto size=assets.game.Shapes().SquidMetrics(background.block,0);const std::array<GumpControl,0> controls{};const std::array<std::string,0> labels{};buttons.SetControls(controls,labels,640,480,{static_cast<std::uint32_t>(background.block+70),0,size.width,size.height,{}}); //실제 배경과 공통 등록표다.
        RawProductionHooks hooks;hooks.now=[] { return 100.0; };hooks.sound=[](auto) { throw std::logic_error("검사에서 미선택 생산 음향"); };hooks.tell=[](auto) { throw std::logic_error("검사에서 미선택 Tell"); };hooks.configInt=[](auto,auto fallback) { return fallback; };hooks.translate=[](auto text) { return std::string(text); };hooks.gameSeconds=[] { return 0.0; };panel=&buttons.ConstructProduction(Context(),assets.game,std::move(hooks));
    } //생성 시 실제 Deck getter를 앞당기지 않는다.
    ProductionControlContext Context() { return {storage,client.GameplayGlobals(),client.PlayerRecords(),options,client.SpGlobals()}; } //모든 호출이 같은 현재 실제 owner를 빌린다.
    void ModeOne() { CHECK(panel->Handle(panel->RootToken(),0x0200003dU,1,0)==0); } //43ec00의 실제 생산 입력으로43df80에 도달한다.
    std::uint32_t Token(std::uint32_t owner=1) { return NoWorldProductionRead(client.PlayerRecords()->Records(),std::size_t(owner)*180+0x68); } //현재 Player+68을 직접 관측한다.
};
}

TEST_CASE(ClientProductionSync_WorldlessHudModeOneUsesOneActualClientDeck) {
    NoWorldProductionScene scene;const auto base=scene.client.PlayerRecords()->Base();auto memory=scene.client.PlayerRecords()->ProductionDeckMemory();const auto poolBefore=std::vector<std::uint8_t>(scene.storage->pool.Bytes().begin(),scene.storage->pool.Bytes().end());CHECK(scene.Token()==0);
    scene.ModeOne();const auto token=scene.Token();CHECK(token && !scene.storage->Borrowed() && !scene.client.GameplayGlobals()->Borrowed());auto deck=memory->Bytes(token);CHECK(deck.size()==352);
    for (const auto offset:{0x14cU,0x150U,0x154U,0x158U,0x15cU}) CHECK(NoWorldProductionRead(deck,offset)==0); //실제 생성자의 다섯 DWORD만 기대한다.
    deck[0]=0x5a;scene.ModeOne();CHECK(scene.Token()==token && memory->Bytes(token).data()==deck.data() && deck[0]==0x5a); //State3의 첫/두 번째 HUD1은 재할당/초기화하지 않는다.
    scene.client.GameplayGlobals()->player.templateOwner=2;scene.ModeOne();const auto second=scene.Token(2);CHECK(second && second!=token && scene.Token()==token && deck[0]==0x5a);scene.client.GameplayGlobals()->player.templateOwner=1;
    scene.panel->Idle(scene.panel->RootToken());CHECK(scene.client.PlayerRecords()->Base()==base && !scene.client.GameplayGlobals()->Borrowed() && std::ranges::equal(scene.storage->pool.Bytes(),poolBefore)); //기술0의 실제 Idle 관문은 유지 몸체를 요구하지 않는다.
}

TEST_CASE(ClientProductionSync_WorldlessSidZeroFinderAndSelectedFailureAreReal) {
    NoWorldProductionScene scene;auto game=scene.client.GameplayGlobals();auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(scene.storage->pool.Bytes().data()),scene.storage->pool.Bytes().size()); //기존 raw replay와 같이 예약SID0의 명시 입력만 준비한다.
    NoWorldProductionPut(raw,6,65535,2);CHECK(NoWorldProductionFailure([&] { scene.ModeOne(); })=="contained finder 종속 체인 손상");const auto token=scene.Token();CHECK(token);NoWorldProductionPut(raw,6,0,2); //Deck 확보 뒤 실제 SID0 head를 읽은 실패 순서를 확인한다.
    CHECK(!(scene.storage->types[83].flags2&4U)); //실제 isle은 Temple 관문을 먼저 선택하는 bridge와 달리 Deck power 분기에 도달한다.
    auto deck=scene.client.PlayerRecords()->ProductionDeckMemory()->Bytes(token);NoWorldProductionPut(deck,0x14c,1);deck[0]=83;deck[2]=1;const auto sid=scene.storage->pool.Allocate();auto form=scene.storage->pool.AllocatedBytes(sid);form[10]=6;NoWorldProductionPut(form,4,0,2);NoWorldProductionPut(form,18,83,2);NoWorldProductionPut(form,20,8,2);NoWorldProductionPut(raw,6,sid.value,2); //실제 SID0 종속의 seen이 현재 Deck 생성을 억제하는 입력이다.
    scene.ModeOne();CHECK(scene.Token()==token && NoWorldProductionRead(deck,0x14c)==1 && form[20]==8);deck[2]=0;NoWorldProductionPut(form,20,0x108,2);
    CHECK(NoWorldProductionFailure([&] { scene.ModeOne(); })=="선택한 생산 효과의 현재 게임 Runtime 미연결" && scene.Token()==token && form[20]==8 && form[21]==1); //실제 선택 취소/삭제에는 가짜 성공을 공급하지 않는다.
    NoWorldProductionPut(raw,6,0,2);raw[11]|=2;game->finder.checkingDead=true;game->Boss()=false;CHECK(game->finder.sharedBoss==&game->Boss());CHECK(NoWorldProductionFailure([&] { scene.ModeOne(); })=="권한 없는 dead 부모 종속 조회 assert 경로"); //World 없이도 현재 실제 권한을 재조회한다.
    raw[11]&=0xfd;game->finder.checkingDead=false;game->Boss()=true;NoWorldProductionPut(deck,0x14c,0); //명시 실패 입력의 생존 필드를 정상 종료 가능한 상태로 돌린다.
}

TEST_CASE(ClientProductionSync_WorldlessDeckFreedBeforeNextActualPlayerInitialization) {
    NoWorldProductionScene scene;scene.ModeOne();const auto token=scene.Token(),base=scene.client.PlayerRecords()->Base();auto memory=scene.client.PlayerRecords()->ProductionDeckMemory();CHECK(token);
    ClientStateRuntimeRetirement retire(scene.client,scene.storage);retire.ClearPlayerLinks();CHECK(scene.Token()==0 && NoWorldProductionFailure([&] { static_cast<void>(memory->Bytes(token)); })=="현재PlayerDeck의실제할당블록누락");
    for (std::size_t owner=0;owner<9;++owner) { const auto players=scene.client.PlayerRecords()->Records();CHECK(NoWorldProductionRead(players,owner*180+0x68)==0 && NoWorldProductionRead(players,owner*180+0x9c)==0 && NoWorldProductionRead(players,owner*180+8)==0); } //실제 원문 부모의 모든9행 세 DWORD를 확인한다.
    InitializeClientPlayerRecords(scene.client);CHECK(scene.client.PlayerRecords()->Base()!=base && scene.client.PlayerRecords()->ProductionDeckMemory()==memory && scene.Token()==0);scene.ModeOne();CHECK(scene.Token() && scene.Token()!=token && NoWorldProductionFailure([&] { static_cast<void>(memory->Bytes(token)); })=="현재PlayerDeck의실제할당블록누락"); //새 Player/Session이 같은 저장소를 되감거나 이전 블록을 되살리지 않는다.
}

TEST_CASE(ClientProductionSync_BorrowedRuntimeCannotUseWorldlessFallback) {
    NoWorldProductionScene scene;CHECK(scene.Token()==0);
    { auto borrowed=scene.client.GameplayGlobals()->AcquirePrepared();CHECK(NoWorldProductionFailure([&] { scene.ModeOne(); })=="선택한 생산 효과의 현재 게임 Runtime 미연결" && scene.Token()==0); } //미공개/부분 Session 대여를 실제 장기 경로로 우회하지 않는다.
    { auto borrowed=scene.storage->AcquirePrepared();CHECK(NoWorldProductionFailure([&] { scene.ModeOne(); })=="선택한 생산 효과의 현재 게임 Runtime 미연결" && scene.Token()==0); } //Player와 다른 World 대여의 불일치도 가짜 Runtime으로 통과시키지 않는다.
    scene.ModeOne();CHECK(scene.Token()!=0 && !scene.client.GameplayGlobals()->Borrowed() && !scene.storage->Borrowed()); //실제 두 대여가 없어진 뒤에만 메뉴 경로가 선택된다.
}
