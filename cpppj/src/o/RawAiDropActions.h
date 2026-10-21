// AI의 현재 배치TYPE/예산 스캔과 생산·기한·사제 관문을 실제 후보 배치 행동으로 연결한다.
#pragma once
#include "o/RawAiDropScan.h"

namespace netstorm::o {
//004105b0의 일곱 원래 알림이며 TYPE/방향/좌표는 각 원문 위치의 snapshot이다.
enum class AiDropActionLog { LostAbility,ScanComplete,PartialScan,CannotAfford,BadChoice,Laying,Target };
// 출력 여부와 별개로 원래 TYPE 조회/ftol을 먼저 평가한 로그 인자다.
struct AiDropActionNotice {
    AiDropActionLog action{}; //원래 알림 형식이다.
    std::uint32_t type{}; //시설 TYPE 또는 표적의 TYPE BYTE이며 표적 SID와 구분한다.
    std::int32_t direction{},x{},y{}; //스캔 방향 또는 y→x ftol로 구한 실제 알림 좌표다.
};
// 원본 현재 시각/지도크기/진단 전역을 같은 저장소에서 빌린다.
struct AiDropActionGlobals {
    const double& now; //0055b4d0의 현재 QWORD 시각이다.각 기한 비교에서 다시 읽는다.
    const std::int32_t& mapLimit; //00531928의 현재 strict 좌표 유효성 상한이다.
    const bool& checking; //005e4794의 빈 후보/점수 모순 진단 관문이다.
};
// 이미 복원한 같은 실제 선택/스캔/생산·비용/배치/부족좌표/후보 압축과 원래 로그다.
struct AiDropActionHooks {
    std::function<Sid(std::uint32_t,std::uint32_t)> product; //004103c0의 현재 owner 기술부모 mask8 TYPE 조회다.
    std::function<void(std::uint32_t)> select; //00413100의 같은 실제 TYPE 선택/회전이다.
    std::function<bool(std::uint32_t,std::uint32_t,std::int32_t)> scan; //00412e10의 같은 본체3e8/현재 후보 스캔다.true는 미완료다.
    std::function<bool(std::uint32_t,std::uint32_t)> canAfford; //00410280의 실제 signed 비용/지갑 비교다.
    std::function<bool(const CanonPlacementQuery&)> mayPlace; //00411a10→0049b510 전체이며 argument0/flags0/ownerBYTE/mode1이다.
    std::function<void(std::uint32_t,std::uint32_t,RawPathPoint,std::uint32_t,std::uint32_t)> deploy; //004108f0의 실제 생산/비용·배치/통지/음향이다.반환 성공값을 만들지 않는다.
    std::function<void(std::uint32_t)> fake; //00415fe0의 같은 실제 전체 가짜 부족좌표 갱신이다.
    std::function<void(std::uint32_t)> seed; //004133e0의 같은 공용 난수/8방향 좌표 선택이다.
    std::function<void(std::uint32_t,std::int32_t)> remove; //0040f390의 같은 실제30후보 안정 압축이다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //원본 Ai.cpp의 복귀 가능한 진단/줄번호다.
    std::function<void(std::uint32_t,const AiDropActionNotice&)> log; //원래 b00비영 위치에서만 실제 장치를 요청한다.
};
// 별도 구매/좌표/성공 사본 없이 같은 실제 본체/원본 공용 난수로 행동한다.
class RawAiDropActions {
public:
    RawAiDropActions(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,AiDropActionGlobals globals,AiDropActionHooks hooks); //같은 PATCH188 TYPE/본체/실제 하위를 빌린다.
    void Run(std::uint32_t token) const; //004162e0 전체 배치 행동이며 원래 선택/관문/후보 난수·배치/성공표적·압축 순서를 보존한다.
    const SidPool& Pool() const; //같은 실제 월드 연결을 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아있는 실제0xb08 본체만 허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //하위 효과 뒤 현재 DWORD를 읽는다.
    void Field(std::uint32_t token,std::size_t at,std::uint32_t value) const; //지정 DWORD만 쓰고 주변을 보존한다.
    const RiftTypeRecord& Type(std::uint32_t type) const; //원래 TYPE 조회 위치에서 현재 TYPE0도 허용하며 물리 표 밖만 거부한다.
    bool Ready(std::uint32_t token,std::size_t at) const; //현재 QWORD 기한≤현재 시각이며 NaN은 미도달이다.
    Sid AvailableProduct(std::uint32_t token,std::uint32_t type) const; //00410580:같은 실제 생산 조회 뒤 현재 pending100만 제외한다.
    bool MayPlace(std::uint32_t token,std::uint32_t type,RawPathPoint point) const; //00411a10의 원래 ownerBYTE/나머지 인자와 TYPE 조회다.
    std::size_t PointOffset(std::int32_t index) const; //정상30후보의 물리 좌표 범위만 검사한다.
    RawPathPoint Point(std::uint32_t token,std::int32_t index) const; //현재 선택 배열 X/Y를 각 원문 호출 위치에서 다시 읽는다.
    std::uint32_t Target(std::uint32_t token,std::int32_t index) const; //현재 선택 배열의 표적 SID DWORD다.
    Sid Object(std::uint32_t number) const; //SID0도 읽되 전체 DWORD의 물리 범위를 먼저 검사한다.
    void Log(std::uint32_t token,AiDropActionNotice notice) const; //Partial 외 TYPE를 먼저 조회하고 현재 b00에서 실제 알림을 분배한다.
    void PointLog(std::uint32_t token,AiDropActionLog action,std::int32_t index) const; //현재 후보Y→X ftol 뒤 현재 시설TYPE의 알림을 만든다.
    RawAiBrainMemory& memory_; //기술/선택·스캔/후보/지갑과 같은 실제 본체다.
    const SidPool& pool_; //생산 조각과 표적 raw 슬롯의 같은 PATCH 풀이다.
    std::span<const RiftTypeRecord> types_; //현재 TYPE flags2/로그 이름의 실제 표다.
    GameRandom& random_; //같은 게임 공용 난수이며 후보count로 한 번씩 소비한다.
    AiDropActionGlobals globals_; //현재 시각/지도크기/진단 참조다.
    AiDropActionHooks hooks_; //선택된 실제 하위/원래 알림 장치다.
};
}
