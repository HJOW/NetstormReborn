// 원본10.78 Cursor.cpp446b50의 정수 마우스 입력과4971e0 역투영·41d770 절삭을 연결한다.
#pragma once
#include "client/RawCursorPreviews.h"

namespace netstorm::client {
// 좌표 갱신 뒤 실제 수명/미리보기/하드웨어 커서로 전달하는 효과다.
struct CursorPositionHooks {
    std::function<void(std::uint32_t)> activate,selectCursor; // 445aa0(0)과4a4410(2/3)이다.
    std::function<void()> createPreviews; // 같은 상태의445b30 전체 몸체다.
};
// 카메라/스냅과 기존 Drop/Placement 전역을 빌리며 새로운 위치 사본을 소유하지 않는다.
class RawCursorPosition {
public:
    // 참조는 처리기보다 오래 살아야 한다. camera는 MetadisplayDirtyView.camera와 같은 현재 float 비트다.
    RawCursorPosition(CursorDropState& drop,const CursorPlacementState& placement,const std::uint32_t& snapToGrid,
        const std::array<std::uint32_t,2>& camera,CursorPositionHooks hooks,
        o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 실제 두 cdecl 인자는 int다. 위치가 바뀔 때 잠금을 다시 읽어 정리/생성/커서 선택을 수행한다.
    void Update(std::int32_t mouseX,std::int32_t mouseY) const;
    // 4971e0: 정수 감김 뒤 원본 float 역배율/현재 카메라를 x87 중간 정밀도로 합한다.
    static std::array<float,2> ScreenToWorld(std::int32_t x,std::int32_t y,const std::array<std::uint32_t,2>& camera);
    static std::array<std::uint32_t,2> CameraFromPixels(std::int32_t x,std::int32_t y); // 실제497490: scroll pixel을 먼저 float로 좁히지 않고 현재 float 카메라 비트를 만든다.
    // 41d770: _ftol signed64 결과의 하위signed DWORD를 float로 다시 저장한다. 반올림/floor가 아니다.
    static std::array<float,2> TruncatePoint(std::array<float,2> point);
private:
    CursorDropState& drop_; // 입력 화면점/월드 위치/잠금/수신 Gump의 기존 공유 전역이다.
    const CursorPlacementState& placement_; // Query가 갱신한 첫 그림의 현재 크기/기준점이다.
    const std::uint32_t& snapToGrid_; // 55a2d8이며 teleporter 진단5e4794와 별도다.
    const std::array<std::uint32_t,2>& camera_; // 현재59a900/904의 float 비트를 직접 빌린다.
    CursorPositionHooks hooks_; // 복원된 수명/생성과 실제 커서 선택 효과다.
};
// 같은 풀의 실제 정리/생성 두 몸체를 연결한다. selectCursor는 기존 Cursor의 실제 선택기다.
CursorPositionHooks MakeCursorPositionHooks(RawCursorLifecycle& lifecycle,RawCursorPreviews& previews,
    std::function<void(std::uint32_t)> selectCursor);
// Take가 꼬리에서 읽은 현재 마우스를 같은 실제 위치 갱신 몸체에 전달한다.
void BindCursorPosition(CursorLifecycleHooks& lifecycle,RawCursorPosition& position);
}
