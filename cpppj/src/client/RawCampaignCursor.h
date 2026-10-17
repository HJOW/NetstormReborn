// 같은 캠페인 월드의 실제 커서 수명/위치/미리보기/내려놓기/로컬 예측을 소유한다.
#pragma once
#include "client/RawCampaignConstructionBindings.h"
#include "client/RawCursorPosition.h"
#include "client/RawCursorReturnPiece.h"

namespace netstorm::client {
class RawCampaignCursorDisplay; // 같은 커서의 실제 수집/Draw/픽셀 출력 수명이다.
// UI/장치/송신만 외부에서 받는다. 실제 계산/생성/삭제 콜백은 내부에서 덮어쓴다.
struct CampaignCursorEffects {
    CursorLifecycleHooks lifecycle; // 강조/커서 선택/팝업/번역/측정/마우스만 소비한다.
    CursorPlacementHooks placement; // Gump 조회/수신/부모와 dirty 사각형만 소비한다.
    CursorDropHooks drop; // 기록/금액 표시/GUI 수신/표시 갱신/닫기만 소비한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> send; // 목적지9의 실제25바이트를 전송기에 넘긴다.
};
// 세션의 장기 게임 몸체와 현재 공유 상태다. 장치 효과와 별도로 공급한다.
struct CampaignCursorGame {
    CursorLifecycleHooks lifecycle; // 같은 Form 부모/분리/복귀/음향/SP/마나/사정거리다.
    CursorPlacementHooks placement; // 같은 에너지/전체 MayPlace/Graph/프로세스/투영이다.
    std::function<void()> prepare; // 각 공개 명령 직전에 같은 현재 소유자/모드를 반영한다.
    std::function<void(std::uint32_t,o::Sid,std::uint32_t,std::uint32_t)> moveBuilder; // 같은 실제134/23/39를 시작한다.
    std::function<void()> playDrop; // 단일 실제 이름표의 droppiece.wav를 재생한다.
};
// 모든 참조는 이 커서보다 오래 살아야 한다. 월드나 카메라의 사본을 소유하지 않는다.
struct CampaignCursorData {
    RawSurfaceWorld& world; // 같은 Factory/공간/가상 삭제/연결 생성기를 빌린다.
    const GameAssets& assets; // 같은 현재 TYPE/SHP 자산이다.
    const o::PriestPlacementGeometryState& geometry; // 같은 실제 Canon 패턴 입력이다.
    std::span<const o::PriestPlainCanonType> frames; // 같은 실제 타입별 물리 프레임이다.
    o::ContentFormHost& forms; // 실제 정렬 부착/제거 알림을 실행한다.
    o::RawContainedCreate& contained; // 반환 조각의 실제 타입6 생성기다.
    o::SquidReward& reward; // 같은 실제 HP/SP 저장소다.
    o::GameRandom& random; // 반환 다리와 게임이 공유하는 실제 난수다.
    RawCampaignConstructionBindings& construction; // 같은 실제 Place를 로컬 예측에도 공급한다.
    CursorDropState& drop; // 기존 마나/사정거리/내용물 수명이 공유하는 현재 커서다.
    const std::array<std::uint8_t,144>& grid; // 실제 현재 MayPlace가 쓴 열 우선 격자다.
    o::ScrambledSpStore& sp; // 현재 단일 SP와 추적 offset/순열이다.
    const std::uint32_t& playerKnown; // 실제595344 DWORD다.
    const std::array<std::uint32_t,2>& camera; // 현재497490 카메라 float비트를 매 이동마다 읽는다.
};
// 원본 각각의 몸체를 같은 상태에 조립하며 미연결 UI를 빈 성공으로 처리하지 않는다.
class RawCampaignCursor {
public:
    // 생성 시 UI는 필요하지 않으며 선택된 외부 경계가 없으면 그 자리에서 오류를 낸다.
    RawCampaignCursor(CampaignCursorData data,CampaignCursorGame game);
    ~RawCampaignCursor(); // 고정 주소 callback 대상인 내부 몸체를 함께 해제한다.
    RawCampaignCursor(const RawCampaignCursor&)=delete; // callback 주소를 보존한다.
    RawCampaignCursor& operator=(const RawCampaignCursor&)=delete; // 다른 세션으로 복사하지 않는다.
    // 같은 장기 몸체를 유지하면서 현재 장치/GUI/전송기를 교체한다.
    void SetEffects(CampaignCursorEffects effects);
    RawCampaignCursorDisplay& Display(); // 같은 현재 커서/격자를 수집하고 그리는 단일 표시기다.
    void SetPalette(std::span<const ScreenColor> palette); // 실제 파일 팔레트와 Native Take의 두 현재 색을 연결한다.
    void SetView(SoundView view); // 현재 화면 픽셀 원점을 Query/Prepare/Draw가 공유하게 한다.
    void Take(o::Sid form,std::uint32_t flags); // 실제00447000의 전체 DWORD flags를 전달한다.
    void Cancel(); // 잠긴 상태에서도 실제 꼬리 정리/프로세스 반납을 실행한다.
    void Update(std::int32_t x,std::int32_t y); // 실제00446b50으로 현재 마우스/카메라를 반영한다.
    void Drop(); // 실제004473e0→4433b0→같은 Place/Builder→25바이트 송신을 실행한다.
    void Bind(PriestMenuHooks& hooks); // 실제 사제 메뉴 취득을 이 단일 커서에 연결한다.
    CursorLifecycleState& Lifecycle(); // 현재30SID/추상 타입/측정 상태를 빌린다.
    CursorPlacementState& Placement(); // 현재 허용 DWORD/dirty 범위를 빌린다.
    // 실제 생산 GUI가 같은 정렬 Form 생성기를 사용한다. 생산 완료/준비 조각 생성을 대신하지 않는다.
    const o::RawContainedCreate& Contents() const;
    CursorReturnPieceState& ReturnSettings(); // 현재 유지/소멸 옵션을 직접 조작한다.
    std::uint32_t& SnapToGrid(); // 실제 snapToGrid DWORD를 직접 조작한다.
private:
    struct Impl; // 순환 callback들이 참조하는 단일 고정 주소 소유자다.
    std::unique_ptr<Impl> impl_; // 세션의 Kernel 정리 이후에 해제한다.
};
}
