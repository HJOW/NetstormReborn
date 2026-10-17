// 원본 커서 미리보기의 부분 기록·쌍 제출과 패치판 SP 표시 상태를 복원한다.
#pragma once
#include "client/DrawSubmission.h"
#include "client/CursorPreviewSource.h"
#include <optional>
#include "o/SidPool.h"
#include "o/SpStore.h"

namespace netstorm::client {
// 원본 전역의 현재 목록/선택 타입/부모와 배치 칸, 패치 전용 표시 상태다.
struct PreviewCollectionState {
    // lockDepth: 중첩된 잠금 요청의 수.
    // type: 타입 표에서 객체 종류를 찾는 번호.
    // parentGump: 부모 Gump.
    // specialTypeA: 특수 타입 A.
    // specialTypeB: 특수 타입 B.
    // legal: 배치가능.
    std::uint32_t lockDepth{},type{},parentGump{},specialTypeA{},specialTypeB{},legal{};
    // pieceCount: 조각 개수.
    std::int32_t pieceCount{};std::vector<o::Sid> pieces; // count는 signed이며 각 반복에서 현재 값을 읽는다.
    float x{},y{}; // raw 각 조각 위치와 다른 커서 배치 판정 좌표다.
    // scrambledOffset: 분산된 오프셋.
    // copiedOffset: 복사된 오프셋.
    // now: 현재 판정에 사용하는 시각(초).
    // highlight: 강조.
    // countdown: 남은시간.
    // querySp: 조회 SP.
    // localPlayer: 로컬 플레이어.
    std::uint32_t scrambledOffset{},copiedOffset{},now{},highlight{},countdown{},querySp{},localPlayer{};
    // firstSpPoolPresent: 첫 SP 풀 존재 여부.
    bool firstSpPoolPresent{true};std::int16_t firstSpIndex{}; // 이미 준비한 패치 저장소의 첫 포인터/순열 상태다.
};
// 부모 깊이와 SP 읽기는 실제 UI/저장소에 연결한다. 사용하는 분기에서만 콜백이 필요하다.
struct PreviewCollectionHooks {
    // gumpDepth: 미리보기를 붙일 Gump의 현재 깊이를 조회하는 콜백.
    // readSp: 현재 플레이어의 SP 값을 조회하는 콜백.
    std::function<std::uint32_t(std::uint32_t)> gumpDepth,readSp;
};
// viewport와 무관하게 커서 조각을 kind 4의 본체/보조 레이어로 제출한다.
class PreviewCollection {
public:
    // 상태/풀/타입/spot은 이 객체보다 오래 살아야 한다. spot은 256×256 원본 표다.
    PreviewCollection(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
        std::span<const std::uint8_t> spots,PreviewCollectionState& state,PreviewCollectionHooks hooks={},std::uint32_t tokenBase=0);
    // limit<1일 때만 멈추므로 limit 1에서도 두 항목을 쓴다. 실제 span 부족은 예외로 거부한다.
    // 건너뛴 조각도 현재 칸의 x/y/token/depth 14바이트를 쓰며 kind/tail과 반환 수는 바꾸지 않는다.
    std::int32_t Collect(o::RenderWorldRect view,std::span<o::RenderCollectionEntry> output,std::int32_t limit) const;
    // PATCH의 실제 전역을 참조한다. 피드백 전역과 외부 경계는 기존 state/hooks를 유지한다.
    void BindCursor(CursorPreviewSource source);
    // 실제 sprite 경계가 대리 토큰을 같은 raw 풀의 SID로 되돌릴 때 사용한다.
    o::Sid Object(std::uint32_t token) const;
private:
    // 원본 float 0.9999를 x87에서 더한 뒤 CRT 64비트 절삭의 하위 DWORD를 반환한다.
    static std::int32_t SpotCoordinate(float coordinate);
    // 패치판의 offset/카운터/SP float/첫 포인터·순열 검사 순서로 표시 시각을 갱신한다.
    void PatchFeedback(std::uint32_t owner) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const o::SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const o::RiftTypeRecord> types_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // state_: 원본 전역의 현재 목록/선택 타입/부모와 배치 칸, 패치 전용 표시 상태다 (PreviewCollectionState).
    PreviewCollectionState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (PreviewCollectionHooks).
    PreviewCollectionHooks hooks_;
    std::optional<CursorPreviewSource> cursor_; // 연결했으면 중복 입력 대신 같은 실제 전역을 읽는다.
    // tokenBase_: 토큰 기본.
    std::uint32_t tokenBase_{};
};
// 초기화를 끝낸 SP 저장소의 비트 읽기를 연결한다. 상위 계층은 그 상태를 state에 공급해야 한다.
PreviewCollectionHooks MakePreviewSpHooks(o::ScrambledSpStore& store,PreviewCollectionHooks hooks={});
// 클립 전 영역과 잔여 작업 칸을 실제 미리보기 수집기로 전달한다.
void BindPreviewCollection(DrawCollectionHooks& hooks,const PreviewCollection& collector);
}
