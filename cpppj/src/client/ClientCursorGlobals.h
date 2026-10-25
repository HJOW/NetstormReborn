// Client수명의커서수명/배치/격자데이터만소유하고실행기/Session콜백은보관하지않는다.
#pragma once
#include "client/RawCursorPlacement.h"

namespace netstorm::client { // Native전역주소의수명과짧은RawCampaignCursor실행기를분리한다.
class ClientCursorGlobals {
public:
    ClientCursorGlobals()=default; // 같은기존PE초기값/DMI를최초한번구성한다.
    ClientCursorGlobals(const ClientCursorGlobals&)=delete;ClientCursorGlobals& operator=(const ClientCursorGlobals&)=delete; // 살아있는전역주소는복제하지않는다.
    CursorLifecycleState& Lifecycle() { return lifecycle_; } // 실제30DWORD/count/측정/현재Gump를직접빌린다.
    const CursorLifecycleState& Lifecycle() const { return lifecycle_; } // 현재값을복사한표시사본을만들지않는다.
    CursorPlacementState& Placement() { return placement_; } // 실제위치판정/경고/그림/설명을직접빌린다.
    const CursorPlacementState& Placement() const { return placement_; } // 비용사각형도같은현재값을읽는다.
    std::uint32_t& SnapToGrid() { return snapToGrid_; } //0055a2d8의전체DWORD다.
    const std::uint32_t& SnapToGrid() const { return snapToGrid_; } // 위치입력에서호출때마다다시읽는다.
    void ResetForGeneralWorld() { const auto* pixels=placement_.sharedScreenOrigin;lifecycle_=CursorLifecycleState{};placement_=CursorPlacementState{};placement_.sharedScreenOrigin=pixels;snapToGrid_=0; } // 기존일반Runtime의fresh/반납호환이며NativeCancel의효과를대신하지않는다.
private:
    CursorLifecycleState lifecycle_; // Native임시목록과측정값이며pendingTimer/실행함수가없다.
    CursorPlacementState placement_; // 같은장기snapshot픽셀포인터만빌릴수있고Session포인터는없다.
    std::uint32_t snapToGrid_{}; // 최초NativeBSS0이며prepared대여에서는현재비트를유지한다.
};
}
