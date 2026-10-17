// 원본 커서의 중첩 잠금/해제/회전을 같은 현재 전역과 기존 몸체에 연결한다.
#pragma once
#include "client/RawCursorLifecycle.h"

namespace netstorm::client {
// 실제 하위 몸체와 장치 효과다. 현재 커서 상태의 사본이나 대체 계산을 만들지 않는다.
struct CursorActionHooks {
    std::function<void(std::uint32_t)> activate,selectCursor; // 445aa0(force)와4a4410(cursor)다.
    std::function<void()> createPreviews; // 같은445b30의 현재 목록 생성/재사용이다.
    std::function<void(std::string_view)> sound; // rotatepiece.wav의 실제 이름 재생 경계다.
    std::function<void(o::Sid,std::uint32_t)> removeRange; // 4b1a90(parent,type,0,1)의 기존 조회/삭제다.
    std::function<std::uint32_t(o::Sid,std::uint32_t)> createRange; // 474900(firstPreview,0)의 현재 프로세스 토큰이다.
};
// 세 원본 부모만 복원하며 모든 callback 뒤 실제 저장소를 다시 읽는다.
class RawCursorActions {
public:
    // 현재188TYPE/커서/수명/특별타입/프로세스타입은 처리기보다 오래 살아야 한다.
    RawCursorActions(std::span<const o::RiftTypeRecord> types,CursorDropState& drop,CursorLifecycleState& lifecycle,
        const std::uint32_t& specialType,const std::uint32_t& rangeProcessType,CursorActionHooks hooks);
    // 446a10: 처음 잠글 때만 Activate0하고 skipCursor가0일 때 커서1을 선택한다.
    void Lock(std::uint32_t skipCursor) const;
    // 446a40: DWORD 깊이를 감소시키고0이 되면 실제 목록과 현재 Gump 커서를 다시 만든다.
    void Unlock() const;
    // 446a70: 원본 signed 마스크 꼬리와 음향/잠금/그림/사정거리 재생성 순서를 보존한다.
    void Rotate(std::int32_t step) const;
private:
    std::span<const o::RiftTypeRecord> types_; // 현재 abstract TYPE의 group을 매 분기 읽는다.
    CursorDropState& drop_; // 같은 잠금 깊이/방향/Gump/현재 선택 타입이다.
    CursorLifecycleState& lifecycle_; // 같은 논리 TYPE 포인터/첫 임시 SID다.
    const std::uint32_t& specialType_; // 원본541288의 현재 Wind 타입이다.
    const std::uint32_t& rangeProcessType_; // 원본54113c의 현재51 프로세스 타입이다.
    CursorActionHooks hooks_; // 실제 하위 몸체/현재 장치의 장기 연결이다.
};
}
