// Native커서데이터의장기수명/DWORD목록/실제첫메뉴Cancel꼬리를창없이검사한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "client/ClientCursorGlobals.h"
#include "client/ClientCursorStateCleanup.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientMain.h"
#include "client/RawCursorPreviews.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>
#include <type_traits>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 실제장치없이Native전역과선택된외부효과만관측한다.
// Client::Run을시작하지않는보호파일읽기옵션이다.
ClientOptions CursorOwnerOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; }
// 현재원래사각형을네좌표로그대로대조한다.
bool CursorRect(ScreenRect rect,int l,int t,int r,int b) { return rect.left==l && rect.top==t && rect.right==r && rect.bottom==b; }
}

//30칸의높은WORD도같은장기dataowner에보관하며prepared대여/반납은변경하지않는다.
TEST_CASE(ClientCursorGlobals_DwordWidthAndPreparedOwnerIdentity) {
    static_assert(sizeof(CursorLifecycleState::previews)==30*4);static_assert(std::is_same_v<decltype(CursorLifecycleState::previews),std::array<std::uint32_t,30>>);ClientGameplayGlobals first,second;auto& state=first.CursorGlobals();auto* address=state.Lifecycle().previews.data();
    state.Lifecycle().previews[0]=0x12345678U;state.Lifecycle().previews[29]=0xffffffffU;state.Lifecycle().previewCount=-7;state.Lifecycle().currentGump=0x76543210U;state.Placement().description="same-native-state";state.SnapToGrid()=0x80000000U;first.cursor.cursorGump=0x13579bdfU;first.cursor.legalGump=0x2468ace0U;
    { auto use=first.AcquirePrepared();CHECK(first.Borrowed() && state.Lifecycle().previews.data()==address && state.Lifecycle().previews[0]==0x12345678U && state.Lifecycle().previews[29]==0xffffffffU && state.Lifecycle().previewCount==-7 && state.SnapToGrid()==0x80000000U); }
    CHECK(!first.Borrowed() && state.Lifecycle().previews.data()==address && state.Placement().description=="same-native-state" && first.cursor.cursorGump==0x13579bdfU && first.cursor.legalGump==0x2468ace0U && state.Lifecycle().currentGump==0x76543210U);CHECK(second.CursorGlobals().Lifecycle().previews[0]==0 && second.CursorGlobals().SnapToGrid()==0);
}

// 기존일반Runtime의fresh/반납호환은같은주소에서수행하며NativeCancel효과와구별한다.
TEST_CASE(ClientCursorGlobals_GeneralFreshKeepsAddressesAndCameraBinding) {
    ClientGameplayGlobals globals;auto& state=globals.CursorGlobals();const auto* pixels=&globals.Camera().Snapshot().pixels;state.Placement().sharedScreenOrigin=pixels;auto* values=state.Lifecycle().previews.data();state.Lifecycle().previews[29]=0xffffffffU;state.Lifecycle().previewCount=13;state.Placement().description="old";state.SnapToGrid()=17;
    { auto use=globals.Acquire();CHECK(state.Lifecycle().previews.data()==values && state.Lifecycle().previews[29]==0 && state.Lifecycle().previewCount==0 && state.SnapToGrid()==0 && state.Placement().description.empty() && state.Placement().sharedScreenOrigin==pixels);state.Lifecycle().previewCount=7;state.SnapToGrid()=19; }
    CHECK(state.Lifecycle().previews.data()==values && state.Lifecycle().previewCount==0 && state.SnapToGrid()==0 && state.Placement().sharedScreenOrigin==pixels);
}

// 잠금1/원본preview0에서도Prepare뒤현재count를읽고삭제효과뒤에도현재count/다음DWORD를읽는다.
TEST_CASE(ClientCursorGlobals_NativeCancelRequeriesCountAndProcessThenClearsAllDwords) {
    Scene scene(OriginalEdition::Patch1078);CursorDropState drop;drop.lockDepth=1;drop.cursorGump=77;ClientCursorGlobals state;state.Lifecycle().currentGump=17;drop.legalGump=19;state.Lifecycle().previews[29]=0x87654321U;std::vector<std::uint32_t> order;CursorLifecycleHooks hooks;
    hooks.prepareCursor=[&] { order.push_back(1);state.Lifecycle().previewCount=1;state.Lifecycle().previews[0]=100; };
    hooks.destroy=[&](Sid sid,std::uint32_t flags) { CHECK(flags==0);order.push_back(sid.value);if (sid.value==100) { state.Lifecycle().previewCount=2;state.Lifecycle().previews[1]=101;drop.cursorGump=99; }else state.Lifecycle().previewCount=1; };
    hooks.removeProcess=[&](std::uint32_t process,std::uint32_t flags) { CHECK(process==99 && flags==0);order.push_back(process); };RawCursorLifecycle lifecycle(scene.pool,scene.types,drop,state.Lifecycle(),hooks);lifecycle.Cancel();
    const std::vector<std::uint32_t> expected{1,100,101,99};CHECK(order==expected && drop.lockDepth==1 && drop.preview==0 && drop.cursorGump==0 && drop.legalGump==19 && state.Lifecycle().currentGump==17 && state.Lifecycle().previewCount==0);CHECK(std::all_of(state.Lifecycle().previews.begin(),state.Lifecycle().previews.end(),[](auto value) { return value==0; }));
}

// 호스트SID16경계밖값을다른유효번호로접지않고부분효과/원래DWORD를보존해거부한다.
TEST_CASE(ClientCursorGlobals_HighWordDoesNotAliasValidPreview) {
    Scene scene(OriginalEdition::Patch1078);CursorDropState drop;ClientCursorGlobals state;state.Lifecycle().previews[0]=0x10064U;state.Lifecycle().previewCount=1;std::uint32_t prepared{},destroyed{};CursorLifecycleHooks hooks;hooks.prepareCursor=[&] { ++prepared; };hooks.destroy=[&](Sid,std::uint32_t) { ++destroyed; };
    RawCursorLifecycle lifecycle(scene.pool,scene.types,drop,state.Lifecycle(),hooks);CHECK(Throws([&] { lifecycle.Activate(1); }));CHECK(prepared==1 && destroyed==0 && state.Lifecycle().previewCount==1 && state.Lifecycle().previews[0]==0x10064U);
}

// Session없는실제Client도현재snapshot→실제Renderer dirty두요청→목록/잠금꼬리를전부실행한다.
TEST_CASE(ClientCursorGlobals_FirstMenuCancelUsesActualProjectAndCurrentRenderer) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client(CursorOwnerOptions());const auto globals=client.GameplayGlobals();auto surface=MakeRawSurfaceStorage(assets.game,globals->Authority());auto& state=globals->CursorGlobals();auto& camera=globals->Camera();std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);SetCameraViewWorld(camera.Snapshot(),5,5);globals->cursor.x=10;globals->cursor.y=10;globals->cursor.screenX=777;globals->cursor.screenY=888;globals->cursor.legalGump=0x12345678U;state.Lifecycle().currentGump=0x76543210U;state.Lifecycle().previews[29]=0xffffffffU;state.Placement().hotspotX=12;state.Placement().hotspotY=-7;globals->selection.mouse={-33,65};
    Renderer first(128,128),second(128,128);Renderer* current=&first;std::uint32_t deliveries{};std::vector<std::uint8_t> pixels(128*128);first.Draw(pixels,128);first.Present([](ScreenRect) {});second.Draw(pixels,128);second.Present([](ScreenRect) {});auto provider=[&]() -> Renderer& { ++deliveries;return *current; };
    CancelClientCursorForState(client,*surface,{},provider);CHECK(deliveries==2 && globals->cursor.lockDepth==1 && globals->cursor.preview==0 && globals->cursor.cursorGump==0 && state.Lifecycle().previewCount==0 && state.Placement().sharedScreenOrigin==&camera.Snapshot().pixels);auto dirty=first.Draw(pixels,128);CHECK(dirty.size()==1 && CursorRect(dirty[0],78,53,86,69));first.Present([](ScreenRect) {});CHECK(second.Draw(pixels,128).empty());
    current=&second;SetCameraViewWorld(camera.Snapshot(),6,6);CancelClientCursorForState(client,*surface,{},provider);CHECK(deliveries==4 && first.Draw(pixels,128).empty());dirty=second.Draw(pixels,128);CHECK(dirty.size()==1 && CursorRect(dirty[0],62,42,70,58));CHECK(globals->cursor.screenX==777 && globals->cursor.screenY==888 && globals->selection.mouse.x==-33 && globals->selection.mouse.y==65 && state.Lifecycle().currentGump==0x76543210U && globals->cursor.legalGump==0x12345678U && state.Placement().hotspotX==12 && state.Placement().hotspotY==-7);CHECK(std::all_of(state.Lifecycle().previews.begin(),state.Lifecycle().previews.end(),[](auto value) { return value==0; }));
}

// 새 미리보기로 덮어쓸 DWORD는 SID16으로 먼저 접거나 진단하지 않는다.
TEST_CASE(ClientCursorGlobals_CreateOverwritesInactiveDwordsAtActualAppend) {
    Scene scene(OriginalEdition::Patch1078);CursorDropState drop;ClientCursorGlobals state;BridgeConnectState surfaces; // 같은 풀과 실제 데이터만 제공한다.
    state.Lifecycle().previews[1]=0xffffffffU;state.Lifecycle().previews[29]=0x87654321U;std::uint16_t born=100;std::uint32_t prepared{};CursorPreviewHooks hooks; // 이전 비활성 값과 실제 생성 번호를 구별한다.
    hooks.queryCursor=[] {};hooks.prepareCursor=[&] { ++prepared; };hooks.walk=[](CanonTypeQuery,const std::function<bool(CursorCanonCell)>& visit) { visit({0,0,0});visit({0,1,0}); }; // 명시된 두 Canon 칸만 공급한다.
    hooks.create=[&](std::uint32_t type,std::uint32_t flags) { CHECK(type==0 && flags==2);return Sid{born++}; };hooks.setWord=[](Sid,std::uint32_t) {};hooks.setOwner=[](Sid,std::uint32_t) {};hooks.hasHitPoints=[](Sid) { return false; }; // 가상 효과의 독립 입력이며 생성 산술을 대신하지 않는다.
    RawCursorPreviews previews(scene.pool,scene.types,drop,state.Lifecycle(),surfaces,hooks);previews.Create();
    CHECK(prepared==1 && born==102 && state.Lifecycle().previewCount==2 && state.Lifecycle().previews[0]==100 && state.Lifecycle().previews[1]==101 && state.Lifecycle().previews[29]==0x87654321U);
}

//단일현재번호의실제폭과Client장기주소를검사하며prepared대여는상위WORD도보존한다.
TEST_CASE(CursorPreviewDword_ClientOwnerPreparedIdentityAndWidth) {
    static_assert(std::is_same_v<decltype(CursorDropState::preview),std::uint32_t>);ClientGameplayGlobals first,second;first.cursor.preview=0x12345678U;const auto* address=&first.cursor.preview;first.CursorGlobals().Lifecycle().previews[0]=50;
    { auto use=first.AcquirePrepared();CHECK(&first.cursor.preview==address && first.cursor.preview==0x12345678U && first.CursorGlobals().Lifecycle().previews[0]==50); }CHECK(first.cursor.preview==0x12345678U && second.cursor.preview==0 && Throws([&] { static_cast<void>(CursorPreviewSid(first.cursor.preview)); }));
}

//현재미리보기의호스트검사는원문Cancel의강조·준비·커서선택·잠금뒤에오며잘못된DWORD를보존한다.
TEST_CASE(CursorPreviewDword_CancelRejectsAfterOriginalPrefix) {
    Scene scene(OriginalEdition::Patch1078);CursorDropState drop;ClientCursorGlobals state;drop.lockDepth=0;drop.preview=0x10032U;std::vector<std::uint32_t> trace;CursorLifecycleHooks hooks;hooks.highlight=[&](auto value) { CHECK(value==0);trace.push_back(1); };hooks.prepareCursor=[&] { trace.push_back(2); };hooks.selectCursor=[&](auto value) { CHECK(value==1);trace.push_back(3); };
    RawCursorLifecycle lifecycle(scene.pool,scene.types,drop,state.Lifecycle(),hooks);CHECK(Throws([&] { lifecycle.Cancel(); }));const std::vector<std::uint32_t> expected{1,2,3};CHECK(trace==expected && drop.preview==0x10032U && drop.lockDepth==1 && state.Lifecycle().previewCount==0);
}

//잠긴현재번호는역참조하지않고실제Client의첫메뉴Cancel꼬리가같은4BYTE를전부지운다.
TEST_CASE(CursorPreviewDword_FirstMenuActualCancelClearsWholeWord) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client(CursorOwnerOptions());auto globals=client.GameplayGlobals();auto surface=MakeRawSurfaceStorage(assets.game,globals->Authority(),globals->GameAux());auto& camera=globals->Camera();std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);SetCameraViewWorld(camera.Snapshot(),5,5);globals->cursor.preview=0xffffffffU;globals->cursor.lockDepth=1;const auto* address=&globals->cursor.preview;Renderer renderer(128,128);std::uint32_t queried{};
    CancelClientCursorForState(client,*surface,{},[&]() -> Renderer& { ++queried;return renderer; });CHECK(queried==2 && globals->cursor.preview==0 && &globals->cursor.preview==address && globals->cursor.lockDepth==1 && globals->CursorGlobals().Lifecycle().previewCount==0);
}
