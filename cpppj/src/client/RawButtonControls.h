// UberGump의 돌 버튼 입력을 복원된 Gump 트리/수명/캡처/프레임 처리기로 연결한다.
#pragma once
#include "client/Gump.h"
#include "client/GumpVisual.h"
#include "client/GumpBackground.h"
#include "o/RiftType.h"
#include <functional>
#include <memory>
#include <string>

namespace netstorm::client {
// 실제 fortGump A00 헤더와 현재 팔레트다. 픽셀 출력과 raw 배경 생성은 같은 자산을 사용한다.
struct RawButtonAssets {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    // frame: 타입에서 사용할 그림 프레임 번호.
    std::uint32_t type{},frame{};
    // width: 가로 크기.
    // height: 세로 크기.
    std::int16_t width{},height{};
    // palette: 그림의 색 인덱스를 실제 출력 색으로 바꿀 팔레트.
    ButtonPalette palette;
};
// 벽 시계/Win32 폴링/효과음은 실제 Client가 공급한다. 비어 있으면 독립 콘솔 관찰 상태를 사용한다.
struct RawButtonHooks {
    // now: 현재 시각을 초 단위로 공급하는 콜백.
    std::function<double()> now;
    // poll: 요청한 키·마우스 버튼의 현재 입력을 폴링하는 콜백.
    std::function<InputEvent(std::uint32_t)> poll;
    // sound: 해당 동작의 소리를 재생하는 콜백.
    std::function<void()> sound;
};
// 안정된 파생 저장소와 실제 원본 UI 처리기를 소유한다. 메뉴 행은 기존 메뉴 어댑터가 처리한다.
class RawButtonControls {
public:
    // 원본 판본별 가상 표/등록 계약을 선택한다. 생성 중 장치나 창을 열지 않는다.
    explicit RawButtonControls(o::OriginalEdition edition,RawButtonHooks hooks={});
    // 참조하는 처리기를 먼저 해제하고 마지막에 노드 저장소를 해제한다.
    ~RawButtonControls();
    RawButtonControls(const RawButtonControls&)=delete;
    RawButtonControls& operator=(const RawButtonControls&)=delete;
    // 기존 자식을 실제 Kill/Cleanup한 뒤 돌 버튼/배경을 생성·부착한다. 루트와 serial은 유지한다.
    void SetControls(std::span<const GumpControl> controls,std::span<const std::string> labels,
        int width,int height,RawButtonAssets assets);
    // 실제 GumpInput::Route와 부모 click 통지로 활성화된 화면 명령 번호를 반환한다.
    std::optional<int> Event(InputEvent event);
    // 원본 등록표 Broadcast/Idle에 현재 폴링 좌표를 전달하고 누름 그림 변경 여부를 반환한다.
    bool Poll(InputEvent event);
    // 화면 전환/포커스 상실 때 실제 캡처 처리기에 화면 밖 release를 전달한다.
    void Cancel();
    // 0047d890/CD004773a0: 현재 캡처/dialog/루트에 생산 갱신 사건을 원본 Route 순서로 분배한다.
    // 생산 패널 노드가 아직 없으면 실제 트리의 미처리 반환을 유지한다.
    void RefreshProduction();
    // 현재 raw value/pressed 중 하나라도 비영인 첫 돌 버튼 번호다.
    std::optional<int> Pressed() const;
    // 커서가 월드로 이동해도 release를 같은 raw 캡처에 먼저 전달할지 판정한다.
    bool HasCapture() const;
    // 파일 팔레트의 현재 전역 색을 갱신한다. 이미 생성된 노드의 문맥 복사는 원본처럼 유지한다.
    void SetPalette(ButtonPalette palette);
    // 실제 생성자의 63바이트 복사 결과를 표시/측정에 공급한다. UI의 긴 원문 사본을 다시 그리지 않는다.
    std::string Label(int id) const;
    // 표시와 입력이 같은 raw 경계/두 상태/비활성 플래그를 사용하는 출력 계획이다.
    ButtonDrawPlan DrawPlan(int id,std::int32_t textWidth,std::int32_t textHeight) const;
    // 생성된 실제 자식 배경의 경계/타일 크기/눌림 플래그로 출력 계획을 만든다.
    std::vector<GumpTexturePass> BackgroundPlan(int id,ScreenRect clip) const;
    // 등록 수·serial·focus·idle·노드 수를 자동 검사에서 읽는다.
    std::string Report() const;
private:
    struct Impl;
    // impl_: 공개 인터페이스 뒤의 구현 상태와 연결 객체.
    std::unique_ptr<Impl> impl_;
};
}
