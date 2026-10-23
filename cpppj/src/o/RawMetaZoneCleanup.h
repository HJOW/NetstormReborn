// 메타맵51구역의 고정 기록표를 정리하는0042b1b0 부모를 복원한다.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
inline constexpr std::uint32_t kMetaZoneCleanupCount=51; // 원문 index0..50이며 실제 자식 호출을 생략하지 않는다.
inline constexpr std::size_t kMetaZoneCleanupStride=144,kMetaZoneCleanupBytes=7344; //00546908의51×144BYTE다. State 다섯 블록과 다른 표다.
// 외곽 실제 owner를 빌리며 독립 0 저장소나 장부 사본을 만들지 않는다.
struct MetaZoneCleanupGlobals {
    std::uint32_t& firstReset; //00531760의 실제 DWORD다. 새 Client 최초값1은 외곽 소유자의 책임이다.
    const std::span<std::uint8_t> records; //00546908의 고정 물리 시작을 빌린다. callback은 내용만 변경할 수 있고 주소를 교체하지 않는다.
};
// marker 가상소멸 전체가 복원되거나 실제 owner에 연결될 때까지 선택한 자식은 필수다.
struct MetaZoneCleanupHooks {
    std::function<void(std::uint32_t)> clearMarker; //00479520(index)의 실제 자식이다. 전체51회 호출과 예외 부분효과를 유지한다.
};
//0042b1b0:진입flag비영이면표0→flag0, 진입0이면자식0..50→표0이며 바뀐flag는보존한다.
void ClearMetaZoneRecords(const MetaZoneCleanupGlobals& globals,const MetaZoneCleanupHooks& hooks);
}
