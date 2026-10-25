// 실제 PE의 독립64BYTE 관측과 장기owner/첫메뉴 Project·dirty 장치를 창 없이 대조한다.
#include "RawSceneSupport.h"
#include "client/RawCameraView.h"
#include "client/ClientCameraGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientMain.h"
#include "client/CampaignCursorClientEffects.h"
#include <cstring>

using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace { // 기대64BYTE는 새 C++ 계산이 아닌 격리 PE의 원시 출력이다.
// 리틀엔디언16DWORD를 산술 변환 없이 원시바이트에서 복원한다.
RawCameraViewState CameraImage(std::string_view hex) {
    CHECK(hex.size()==128);std::array<std::uint8_t,64> bytes{};
    for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index*2,2)),nullptr,16)); // 저장된 각 바이트만 해독한다.
    RawCameraViewState result;std::memcpy(&result,bytes.data(),bytes.size());return result;
}
// padding이 없는 실제64BYTE를 Native fixture와 같은 표현으로 관측한다.
std::string CameraBytes(const RawCameraViewState& view) { return Hex({reinterpret_cast<const std::uint8_t*>(&view),sizeof(view)}); }
// 실제 HWND/DirectDraw/음향 장치를 열지 않는 Client 옵션이다.
ClientOptions CameraClientOptions() { ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;return options; }
// 검사 출력 비교는 사각형의 각 signed 좌표와 원래 flags를 보존한다.
bool CameraRect(ScreenRect rect,int l,int t,int r,int b) { return rect.left==l && rect.top==t && rect.right==r && rect.bottom==b; }
}

// 실제 world/pixel setter의64BYTE·분수/F32경계·무관DWORD와497220 투영을 독립 관측16입력에 대조한다.
TEST_CASE(CameraView_Native16WholeImageAndProjection) {
    std::size_t compared{};
    for (const auto& row:LoadFixture(NETSTORM_CAMERA_VIEW_FIXTURE).rows) { // 같은 fixture에 저장된 두 실제 정밀도는 원시로그에서 독립 일치를 확인했다.
        if (row[0]=="operation") continue;CHECK(row.size()==10);auto view=CameraImage(row[1]);const auto x=static_cast<std::uint32_t>(std::stoul(row[2],nullptr,16)),y=static_cast<std::uint32_t>(std::stoul(row[3],nullptr,16));std::vector<std::uint32_t> diagnostics;
        const auto assertion=[&](std::string_view message,std::uint32_t line) { CHECK(message=="scrollPosPixX >= 0" || message=="scrollPosPixY >= 0");diagnostics.push_back(line); };
        if (row[0]=="world") SetCameraViewWorld(view,std::bit_cast<float>(x),std::bit_cast<float>(y),assertion);
        else if (row[0]=="pixels") SetCameraViewPixels(view,std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y),assertion);
        else if (row[0]=="init") { ClientCameraGlobals owner;owner.RendererView()=view;std::uint32_t redraw=0x11111111,revision=0xffffffff;owner.InitializeNativeRenderer(1024,768,256,redraw,revision);view=owner.RendererView();CHECK(CameraBytes(owner.Snapshot())==row[5] && redraw==Number(row[7]) && revision==Number(row[8]));CHECK(owner.Target().limits[0]==3072 && owner.Target().limits[1]==2048 && owner.Target().pixels[0]==0 && owner.Target().pixels[1]==0 && owner.Target().zoom==65536); }
        else if (row[0]=="project") { const auto point=ProjectCameraView(view,std::bit_cast<float>(x),std::bit_cast<float>(y));std::memcpy(&view,point.data(),sizeof(point));std::memcpy(reinterpret_cast<std::uint8_t*>(&view)+8,point.data(),sizeof(point)); }
        else throw std::logic_error("카메라 Native operation 오류");
        CHECK(CameraBytes(view)==row[4]);const bool xDiagnostic=row[9].find("scrollPosPixX")!=std::string::npos,yDiagnostic=row[9].find("scrollPosPixY")!=std::string::npos;CHECK(diagnostics.size()==static_cast<std::size_t>(xDiagnostic)+static_cast<std::size_t>(yDiagnostic));if (xDiagnostic) CHECK(diagnostics.front()==0x344);if (yDiagnostic) CHECK(diagnostics.back()==0x347);++compared;
    }
    CHECK(compared==16);
}

// 같은 형식이라는 이유로 목표/논리Renderer/snapshot을 alias하지 않으며 명시 복사 위치만 현재값을 바꾼다.
TEST_CASE(CameraView_BssExplicitInitializeTargetSnapshotAndDenominatorStayDistinct) {
    ClientCameraGlobals first,second;const std::string zero(128,'0');CHECK(CameraBytes(first.Snapshot())==zero && CameraBytes(first.RendererView())==zero);std::uint32_t redraw=9,revision=31;first.InitializeNativeRenderer(1024,768,256,redraw,revision);
    CHECK(redraw==1 && revision==32 && first.Denominator().defaultValue==15 && first.Denominator().current==15);const auto initial=CameraBytes(first.Snapshot());SetCameraViewWorld(first.RendererView(),1.25F,2.5F);first.RequestPixels(71,83);
    CHECK(CameraBytes(first.Snapshot())==initial && first.RendererView().worldBits[0]==0x3fa00000U && first.Target().pixels[0]==71);first.CopyRendererSnapshot();CHECK(first.Snapshot().worldBits[0]==0x3fa00000U && first.Snapshot().pixels[0]==20 && first.Snapshot().pixels[1]==27);first.UpdateTargetLimits(1280,720,256,256);CHECK(first.Snapshot().worldBits[1]==0x40200000U && first.Target().limits[0]==2816 && first.Target().limits[1]==2096);
    first.ApplyPixelsToRendererAndSnapshot(17,29);CHECK(first.Snapshot().worldBits[0]==0x3f880000U && first.Snapshot().pixels[0]==17 && first.Target().pixels[0]==71);const auto before=CameraBytes(first.Snapshot());redraw=7;first.SelectDenominator(0,redraw);CHECK(redraw==7);first.SelectDenominator(23,redraw);CHECK(redraw==1 && first.Denominator().current==23 && first.Denominator().defaultValue==15 && CameraBytes(first.Snapshot())==before);CHECK(CameraBytes(second.Snapshot())==zero && second.Denominator().current==15);
}

// World/Session이 없는 실제 Client의 원래 Prepare도 현재Project와 교체 가능한 실제 Renderer를 사용한다.
TEST_CASE(CameraView_FirstMenuActualPrepareRequeriesSnapshotAndRenderer) {
    Client client(CameraClientOptions());auto& camera=client.GameplayGlobals()->Camera();std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);Scene scene(OriginalEdition::Patch1078);CursorDropState drop;drop.x=10;drop.y=10;CursorLifecycleState lifecycle;CursorPlacementState state;state.sharedScreenOrigin=&camera.Snapshot().pixels;
    Renderer first(128,128),second(128,128);Renderer* current=&first;std::vector<std::uint8_t> pixels(128*128);first.Draw(pixels,128);first.Present([](ScreenRect) {});second.Draw(pixels,128);second.Present([](ScreenRect) {});
    auto hooks=MakeClientCursorPrepareHooks(client,[&]() -> Renderer& { return *current; });const auto actualInvalidate=hooks.invalidate;std::vector<ScreenRect> requests;std::vector<std::uint32_t> flags;
    hooks.invalidate=[&](ScreenRect rect,std::uint32_t flag) { requests.push_back(rect);flags.push_back(flag);actualInvalidate(rect,flag); };RawCursorPlacement placement(scene.pool,scene.types,{},drop,lifecycle,state,hooks);
    SetCameraViewWorld(camera.Snapshot(),5.0F,5.0F);placement.Prepare();CHECK(requests.size()==2 && CameraRect(requests[0],78,53,86,69) && CameraRect(requests[1],78,53,80,55) && flags[0]==2 && flags[1]==0);auto dirty=first.Draw(pixels,128);CHECK(dirty.size()==1 && CameraRect(dirty[0],78,53,86,69));CHECK(second.Draw(pixels,128).empty());first.Present([](ScreenRect) {});
    current=&second;requests.clear();flags.clear();SetCameraViewWorld(camera.Snapshot(),6.0F,6.0F);placement.Prepare();CHECK(requests.size()==2 && CameraRect(requests[0],62,42,70,58));CHECK(first.Draw(pixels,128).empty());dirty=second.Draw(pixels,128);CHECK(dirty.size()==1 && CameraRect(dirty[0],62,42,70,58));CHECK(camera.Snapshot().worldBits[0]==0x40c00000U && state.ScreenOriginX()==96 && state.ScreenOriginY()==66);
}

// 먼저 보낸 dirty 효과가 원점을 바꾼 경우 비용 사각형은 같은 snapshot의 변경 후 현재값을 읽는다.
TEST_CASE(CameraView_PlacementCostRequeriesSharedPixelsAfterDirtyEffect) {
    ClientCameraGlobals camera;std::uint32_t redraw{},revision{};camera.InitializeNativeRenderer(1024,768,256,redraw,revision);SetCameraViewWorld(camera.Snapshot(),5,5);Scene scene(OriginalEdition::Patch1078);scene.types[82].footX=1;CursorDropState drop;drop.x=10;drop.y=10;drop.costFlags=0x40;drop.cursorWidth=6;CursorLifecycleState lifecycle;lifecycle.abstractType=82;lifecycle.previews[0]=50;lifecycle.costHeight=8;Put(scene.pool.AllocatedBytes(Sid{50}),22,200,2);Put(scene.pool.AllocatedBytes(Sid{50}),24,150,2);CursorPlacementState state;state.descriptionAllowed=state.spaceAllowed=1;state.sharedScreenOrigin=&camera.Snapshot().pixels;
    std::vector<ScreenRect> requests;std::vector<std::uint32_t> flags;CursorPlacementHooks hooks;hooks.project=[&](float x,float y) { const auto point=ProjectCameraView(camera.Snapshot(),x,y);return ScreenPoint{point[0],point[1]}; };hooks.invalidate=[&](ScreenRect rect,std::uint32_t flag) { requests.push_back(rect);flags.push_back(flag);if (requests.size()==1) SetCameraViewWorld(camera.Snapshot(),6,6); };
    RawCursorPlacement placement(scene.pool,scene.types,{},drop,lifecycle,state,hooks);placement.Prepare();CHECK(requests.size()==2 && CameraRect(requests[0],78,53,86,69) && CameraRect(requests[1],93,84,101,94) && flags[0]==2 && flags[1]==0 && state.ScreenOriginX()==96 && state.ScreenOriginY()==66);
}

// X 진단 하위가 Y를 바꾸면 두번째 진단은 현재 원문 필드를 다시 읽으며 선행 여섯store는 이미 완료돼 있다.
TEST_CASE(CameraView_WorldSetterAssertionRequeriesCurrentFieldsAfterAllStores) {
    RawCameraViewState view;InitializeCameraView(view);std::uint32_t calls{};SetCameraViewWorld(view,std::bit_cast<float>(0x4d000001U),std::bit_cast<float>(0x4d800001U),[&](std::string_view message,std::uint32_t line) {
        CHECK(message=="scrollPosPixX >= 0" && line==0x344 && calls++==0 && view.worldBits[0]==0x4d000001U && view.worldBits[1]==0x4d800001U && view.pixels[0]<0 && view.pixels[1]<0);CHECK(view.pixelFloatBits[0]==std::bit_cast<std::uint32_t>(static_cast<float>(view.pixels[0])) && view.pixelFloatBits[1]==std::bit_cast<std::uint32_t>(static_cast<float>(view.pixels[1])));view.worldBits[1]=0xbf800000U;
    });CHECK(calls==1 && view.worldBits[1]==0xbf800000U);
}
