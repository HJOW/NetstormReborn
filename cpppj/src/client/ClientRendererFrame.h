// 실제 Client Frame의 원문 앞단·지형 배경 판정·구름 표시/정리를 같은 장기 owner에 연결한다.
#pragma once
#include "client/ClientRendererFrameGlobals.h"
#include "client/Renderer.h"
#include "client/RawCameraView.h"
#include "o/RawRenderCollection.h"
#include "client/DrawSubmission.h"
#include <memory>
#include <string>

namespace netstorm::client { // OS 화면 장치와 실제 게임 전역 사이의 프레임 조립 계층이다.
class Client;class RawSurfaceStorage;class TileGroupCollection;class ObjectCollection; //동일 Client/Pool/해시의 실제 소유자를 빌린다.
o::RenderWorldRect RendererCollectionViewport(ScreenRect dirty,ScreenRect map,const RawCameraViewState& view); //현재 분수 카메라로 두 사각형을 환산하고 원문 순서로 지도 안에 제한한다.
std::int32_t RendererCollectionRowLimit(std::int32_t width,std::int32_t height); //현재 화면 크기로 원문 정수 곱셈/나눗셈의 지형 행 제한을 계산한다.
// 각 실제 PresentScene 호출에서 Prepare→Drawing→Finish 순서로 사용한다. 생성은 장치/난수를 실행하지 않는다.
class ClientRendererFrame {
public:
    explicit ClientRendererFrame(Client& client); //같은 Client만 저장하고 BSS0 owner를 전체 Client 수명 동안 유지한다.
    ~ClientRendererFrame(); //빌린 collector를 실제 Pool/해시의 공유 소유권보다 먼저 반납한다.
    void ReadOptions(); //Interpret Options 위치에서 showSoftAsserts의 현재 성공한 값만 저장한다.
    bool Prepare(); //실제10.78 메뉴가 있을 때4994b0 앞단을 실행하며 독립 표시/10.72는 false다.
    RendererWorldDrawing Drawing(); //같은 실제 dirty의 지형 수집/배경 장치이며 별도 장면/난수를 만들지 않는다.
    void Finish(); //정상 Draw의 화면 잠금 반납 뒤470ca0을 실행한다. 실패 경로에서는 부르지 않는다.
    std::string Diagnostics() const; //현재 실제 owner/cache·요청/반환을 읽을 뿐 게임 상태를 바꾸지 않는다.
private:
    RendererCloudHooks CloudDevices(); //같은 Screen/색/파일/난수의 현재 표시 장치이며 별도 DirectDraw 표면 경계를 유지한다.
    std::uint32_t Collect(ScreenRect clip,std::uint32_t flags); //원문 현재 지도/카메라 환산과 실제 해시0→2의 지형 수집/배경 덮임이다.
    void Background(ScreenRect clip,std::uint32_t flags,std::uint32_t covered); //같은 dirty를 지도 clip으로 자르고 배경 선택/픽셀 함수를 호출한다.
    void Submit(std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip); //배경 뒤 같은 원문5,000칸을 정렬하고 종류0..4를 실제 현재 출력기에 분배한다.
    Client& client_; //Frame/옵션/수명 전체에서 같은 실제 Client를 빌린다.
    ClientRendererFrameGlobals globals_; //19DWORD/구름4칸/count/묶음 숨김의 유일한 현재 저장소다.
    std::shared_ptr<RawSurfaceStorage> storage_; //collector가 빌린 Pool/해시를 UI/Session 교체와 Frame 소멸까지 생존시킨다.
    std::unique_ptr<TileGroupCollection> groups_; //재사용 표시 작업 배열이며 게임 객체를 복제하지 않는다.
    std::unique_ptr<ObjectCollection> objects_; //실제1..3단계 해시를 조회하며 지형 수집기와 같은 Pool/TYPE/SHP를 빌린다.
    std::array<o::RenderCollectionEntry,kDrawSubmissionCapacity> entries_{}; //원문16바이트 작업5,000칸이며 다음 dirty 전에 같은 수집/정렬/출력만 실행한다.
    std::int32_t entryCount_{};ScreenRect mapClip_{}; //현재 dirty 수집의 실제 개수와 UI 수집 전에 캡처한 지도 clip이다.
    std::array<std::uint64_t,5> collectedKinds_{},submittedKinds_{}; //종류별 실제 호출 관측이며 게임 성공 판정에는 사용하지 않는다.
    int collectorHeight_{}; //작업 공간의 화면 높이다. 해상도 변경 때만 작업 공간을 다시 만든다.
    std::int32_t softAsserts_{}; //showSoftAsserts/005e4794의 Frame 입력이며 최초BSS0/누락 유지다.
    std::uint64_t prepared_{},collected_{},backgrounds_{},cloudCalls_{},cleared_{},finished_{}; //관측 전용 호출 수이며 Native 게임 카운터와 섞지 않는다.
    std::uint64_t fullDirty_{},dirtyPixels_{}; //실제 dirty의 전체 화면 요청 수/누적 논리 픽셀 면적이며 부분 갱신 관측에만 사용한다.
    std::uint32_t lastCovered_{},lastOverride_{}; //마지막 실제 하위 반환을 검사 보고에만 보존한다.
    ScreenRect lastArea_{};std::int32_t lastOffsetX_{},lastOffsetY_{}; //마지막 구름 요청 좌표/시차이며 성공값을 주입하지 않는다.
};
}
