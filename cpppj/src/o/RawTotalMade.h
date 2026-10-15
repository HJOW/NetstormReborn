// 원본 Totalmade의 허용 표와 누적 통계를 같은 postPop 장부에서 조회한다.
#pragma once
#include "o/SquidPostPop.h"

namespace netstorm::o {
// 패치의 debug assert를 관찰하거나 실제 진단으로 전달하는 종류다.
enum class TotalMadeAssertion {
    InvalidType, // 현재 getter의 타입이1이상numTypes미만이 아닌 경우의 진단이다.
    ZeroGenus // 부류별 합계에0마스크를 넘긴 경우의 진단이다.
};
// 004c23c0~004c2560의 표 조작이며 건설/삭제가 이미 갱신하는 카운터를 복제하지 않는다.
class RawTotalMade {
public:
    // 타입 표/장부/허용 표는 이 객체보다 오래 살아야 한다. 허용 표의 첫255DWORD만 원본 초기화 대상이다.
    RawTotalMade(OriginalEdition edition,std::span<const RiftTypeRecord> types,SquidPostPopState& counts,
        std::span<std::uint32_t> restrictions,std::function<void(TotalMadeAssertion)> assertion={});
    // 004c23c0/CD0040ba30: nonzero면 첫255항목을0(허용), zero면1(금지)로 채운다.
    void AllowAll(std::int32_t allowed) const;
    // 004c23e0/CD0040ba50: 원본은 타입 유효성 없이 물리 표의 해당 칸에 allowed==0을 쓴다.
    void Allow(std::int32_t type,std::int32_t allowed) const;
    // 004c2400/CD0040ba70: 유효 타입의 현재 금지DWORD가0일 때만 참이다.
    bool Allowed(std::int32_t type) const;
    // 004c2450/CD0040ba90: 실제 세 집계 표의 첫255항목만 지운다. 256번째 물리 칸은 유지한다.
    void ResetCounts() const;
    // 004c2480/CD0040bac0: 원본 첫 로컬 집계 표의 DWORD 비트를 반환한다.
    std::uint32_t Local(std::int32_t type) const;
    // 004c24c0/CD0040bb30: 원본 두 번째 로컬 집계 표의 DWORD 비트를 반환한다.
    std::uint32_t Built(std::int32_t type) const;
    // 004c2560/CD0040bba0: 원본 전역 집계 표의 DWORD 비트를 반환한다.
    std::uint32_t Global(std::int32_t type) const;
    // 004c2500/CD0040bb50: 타입70부터 현재 끝까지 genus가 겹치는 두 번째 로컬 집계를 DWORD로 합산한다.
    std::int32_t BuiltForGenus(std::uint32_t genus) const;
private:
    // 패치만 원본 assert를 알리며 두 판본 모두 잘못된 getter 입력은 false/0으로 복귀한다.
    bool Valid(std::int32_t type) const;
    OriginalEdition edition_; // debug assert의 판본 차이를 선택한다.
    std::span<const RiftTypeRecord> types_; // 현재 타입별 genus와 원본 numTypes를 읽는다.
    SquidPostPopState& counts_; // Pop/Unpop이 직접 갱신하는 동일 세 카운터 표다.
    std::span<std::uint32_t> restrictions_; // 메뉴/튜토리얼 명령이 공유하는 금지DWORD 표다.
    std::function<void(TotalMadeAssertion)> assertion_; // 패치의 실제 진단 경계이며 미연결이면 예외로 드러낸다.
};
}
