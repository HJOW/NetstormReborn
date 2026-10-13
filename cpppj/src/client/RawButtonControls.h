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
    std::uint32_t type{},frame{};
    std::int16_t width{},height{};
    ButtonPalette palette;
};
// 벽 시계/Win32 폴링/효과음은 실제 Client가 공급한다. 비어 있으면 독립 콘솔 관찰 상태를 사용한다.
struct RawButtonHooks {
    std::function<double()> now;
    std::function<InputEvent(std::uint32_t)> poll;
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
    std::unique_ptr<Impl> impl_;
};
}
