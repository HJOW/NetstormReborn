// 공급 시설/현재 부족거리와 원본 양방향 탐색으로 AI 가짜 부족좌표를 갱신한다.
#pragma once
#include "o/RawAiDryGeometry.h"

namespace netstorm::o {
//004105b0의 두 원래 알림을 구분한다.종류/owner는 실제 출력 시점의 값을 사용한다.
enum class AiFakeDryLog { Discarded,Added };
// 원본 공급반경/지도크기/디버그 종류의 현재 저장소를 빌린다.
struct AiFakeDryGlobals {
    const float& manaRadius; //0052f488의 현재 반경이며 제곱반경이 아니다.
    const std::int32_t& mapLimit; //00531928을 각 좌표 유효성 검사에서 float로 바꾼다.
    const std::uint32_t& debug; //0052f3dc가29면 채택 뒤 실제 표식을 추가한다.
};
// 이미 복원한 실제 거리/중심/경로/목록과 원래 출력만 연결한다.
struct AiFakeDryHooks {
    std::function<float(std::uint32_t,RawPathPoint,std::int32_t&)> score; //00412640의 실제 부족거리와 선택 index다.
    std::function<RawPathPoint(Sid)> center; //004adea0의 현재 실제 시설 중심이다.
    std::function<RawPathPoint(RawPathPoint,RawPathPoint)> candidate; //00415bb0의 실제 양방향 탐색이다.
    std::function<bool(std::uint32_t,RawPathPoint,std::uint32_t)> add; //0040f1c0의 같은 Dry Spot Add다.반환은 무시한다.
    std::function<void(std::uint32_t,std::int32_t)> remove; //0040f260의 같은 index0 안정 압축이다.
    std::function<void(std::uint32_t)> seed; //004133e0의 같은 실제 공용 난수/경계 좌표 선택이다.
    std::function<void(RawPathPoint)> mark; //00410540의 현재TYPE/실제 표식 수명이다.
    std::function<void(std::uint32_t,AiFakeDryLog,std::int32_t,std::int32_t)> log; //004105b0의 현재b00비영인 위치에서만 요청한다.
};
// 실제 본체/목록만 읽고 선택된 하위 뒤 현재 필드를 다시 조회한다.
class RawAiFakeDrySpots {
public:
    RawAiFakeDrySpots(RawAiBrainMemory& memory,const SidPool& pool,AiFakeDryGlobals globals,AiFakeDryHooks hooks); //같은 PATCH 풀/소유 본체와 현재 전역을 빌린다.
    void Extend(std::uint32_t token) const; //00415fe0 전체:TYPE→full제거→신전/공급시설 선택→현재 경로/거리→등록·전략·보충·로그다.
    const SidPool& Pool() const; //같은 실제 월드 연결을 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아있는 실제0xb08 본체만 허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //하위 효과 뒤 현재 DWORD를 읽는다.
    void Field(std::uint32_t token,std::size_t at,std::uint32_t value) const; //지정 DWORD만 쓰고 주변을 보존한다.
    Sid Object(std::uint32_t number) const; //SID0도 원문처럼 허용하며 물리 WORD 풀 범위만 검사한다.
    RawPathPoint Position(Sid sid) const; //현재 슬롯의 raw X/Y float를 그대로 읽는다.
    void Log(std::uint32_t token,AiFakeDryLog action,RawPathPoint point) const; //b00을 읽기 전 y→x ftol을 수행하고 실제 알림을 요청한다.
    RawAiBrainMemory& memory_; //실제 본체/공급시설 목록/12바이트 Dry Spot의 같은 저장소다.
    const SidPool& pool_; //같은 실제 시설/신전 슬롯의 PATCH 풀이다.
    AiFakeDryGlobals globals_; //현재 반경/지도크기/디버그 참조다.
    AiFakeDryHooks hooks_; //선택된 실제 하위와 원래 로그다.
};
}
