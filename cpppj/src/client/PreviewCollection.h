// 원본 커서 미리보기의 부분 기록·쌍 제출과 패치판 SP 표시 상태를 복원한다.
#pragma once
#include "client/DrawSubmission.h"
#include "o/SidPool.h"
#include "o/SpStore.h"

namespace netstorm::client {
// 원본 전역의 현재 목록/선택 타입/부모와 배치 칸, 패치 전용 표시 상태다.
struct PreviewCollectionState {
    std::uint32_t lockDepth{},type{},parentGump{},specialTypeA{},specialTypeB{},legal{};
    std::int32_t pieceCount{};std::vector<o::Sid> pieces; // count는 signed이며 각 반복에서 현재 값을 읽는다.
    float x{},y{}; // raw 각 조각 위치와 다른 커서 배치 판정 좌표다.
    std::uint32_t scrambledOffset{},copiedOffset{},now{},highlight{},countdown{},querySp{},localPlayer{};
    bool firstSpPoolPresent{true};std::int16_t firstSpIndex{}; // 이미 준비한 패치 저장소의 첫 포인터/순열 상태다.
};
// 부모 깊이와 SP 읽기는 실제 UI/저장소에 연결한다. 사용하는 분기에서만 콜백이 필요하다.
struct PreviewCollectionHooks {
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
    // 실제 sprite 경계가 대리 토큰을 같은 raw 풀의 SID로 되돌릴 때 사용한다.
    o::Sid Object(std::uint32_t token) const;
private:
    // 원본 float 0.9999를 x87에서 더한 뒤 CRT 64비트 절삭의 하위 DWORD를 반환한다.
    static std::int32_t SpotCoordinate(float coordinate);
    // 패치판의 offset/카운터/SP float/첫 포인터·순열 검사 순서로 표시 시각을 갱신한다.
    void PatchFeedback() const;
    const o::SidPool& pool_;
    std::span<const o::RiftTypeRecord> types_;
    std::span<const std::uint8_t> spots_;
    PreviewCollectionState& state_;
    PreviewCollectionHooks hooks_;
    std::uint32_t tokenBase_{};
};
// 초기화를 끝낸 SP 저장소의 비트 읽기를 연결한다. 상위 계층은 그 상태를 state에 공급해야 한다.
PreviewCollectionHooks MakePreviewSpHooks(o::ScrambledSpStore& store,PreviewCollectionHooks hooks={});
// 클립 전 영역과 잔여 작업 칸을 실제 미리보기 수집기로 전달한다.
void BindPreviewCollection(DrawCollectionHooks& hooks,const PreviewCollection& collector);
}
