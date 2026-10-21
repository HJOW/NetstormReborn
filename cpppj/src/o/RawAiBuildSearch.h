// 사제 AI의 실제574 배치 탐색 상태/행별 예산/가장 먼 합법 후보를 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawCanonPlacement.h"
#include "o/RawPathRuntime.h"

namespace netstorm::o {
// 원본0040f660의 네 float 경계이며 별도 clamp나 inclusive 변환을 하지 않는다.
struct AiBuildSearchBounds { float left{},top{},right{},bottom{}; }; //왼쪽/위에는 현재 footprint를 더하고 오른쪽/아래는 그대로 절삭한다.
// 게임 시간과 다른 실제 벽시각과 전체 배치 검사를 빌린다.
struct AiBuildSearchHooks {
    std::function<double()> wall; //00460d70의 매 호출 현재 벽시각이다.프레임 snapshot으로 대신하지 않는다.
    std::function<bool(const CanonPlacementQuery&)> mayPlace; //0049b510 전체이며 cached TYPE/argument0/flags0/ownerBYTE/mode1이다.
};
// 별도 좌표 목록 없이 같은0xb08의574..5a7와 실제65536spot/현재188TYPE를 사용한다.
class RawAiBuildSearch {
public:
    RawAiBuildSearch(RawAiBrainMemory& memory,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,std::uint32_t typeBase,AiBuildSearchHooks hooks); //TYPE 포인터는 지정 opaque32 기준+500stride로 현재 실제 표에 대응한다.
    void Initialize(std::uint32_t token,std::uint32_t type,AiBuildSearchBounds bounds,RawPathPoint avoid,std::uint32_t owner) const; //0040f660:원본순서로34BYTE 상태만 쓰고 x/y 범위를 signed 절삭한다.
    void Scan(std::uint32_t token,std::uint32_t budgeted) const; //0040f700:0은 전체완료,비영은 행 시작의 벽시각이deadline보다크거나NaN이면중단한다.
    bool FreeFootprint(std::uint32_t type,std::int32_t x,std::int32_t y) const; //0040f130:현재 footprint의 모든 칸이(spot&3f)==2인지 x외부/y내부 순서로 검사한다.
private:
    static constexpr std::size_t kState=0x574; //실제본체안의0040f660/0040f700상태시작이다.
    static constexpr std::uint32_t kTypeStride=500; //0049a840의실제TYPE레코드pointer산술stride다.
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아 있는 실제0xb08만 허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //state 상대 DWORD를 현재 읽는다.
    void Field(std::uint32_t token,std::size_t at,std::uint32_t value) const; //지정 state DWORD만 쓴다.
    const RiftTypeRecord& Type(std::uint32_t type) const; //현재188TYPE를 물리 범위 검사 뒤 읽는다.
    std::uint32_t Handle(std::uint32_t type) const; //현재 실제 표의 index를 opaque32 TYPE포인터로 표현한다.원본 가상주소를 주장하지 않는다.
    std::uint32_t TypeFromHandle(std::uint32_t pointer) const; //cached18의 현재 핸들을 현재 실제 표 index로 역해석한다.
    std::uint8_t Spot(std::int32_t x,std::int32_t y) const; //실제 row-major spot을 읽고 손상된 지도 밖 주소는 거부한다.
    RawAiBrainMemory& memory_; //다른 AI와 같은 본체/목록 저장소다.
    std::span<const RiftTypeRecord> types_; //재조회/캐시포인터가 같은 실제 현재 표를 본다.
    std::span<const std::uint8_t> spots_; //월드의 같은65536BYTE spot이다.
    std::uint32_t typeBase_; //호스트 포인터 절단 없이500stride를 보존하는 opaque32 TYPE기준이다.
    AiBuildSearchHooks hooks_; //선택된 실제 벽시각/전체 배치 효과다.
};
}
