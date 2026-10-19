// 원본41e4b0의 실제 네 단계 해시 탐색과 주문 대상 거리/저장 규칙이다.
#pragma once
#include "o/RawSquidFinder.h"
#include <array>

namespace netstorm::o {
// 원본 동적 배열의 현재 헤더와 실제 호스트 버퍼를 구별한다.
struct BombTargetList {
    std::span<std::uint32_t> storage; // 실제 쓰기가 가능한 DWORD 버퍼다.
    std::int32_t capacity{},count{}; // 원본은 자격 콜백 뒤 signed 비교로 이 두 현재 값을 다시 읽는다.
};
// 검색에서 이미 계산한 값을 반환해 원본 기하를 검증/조회할 수 있게 한다.
struct BombTargetGeometry {
    std::array<float,2> center{}; // 스냅→0.5차감→1..255 제한을 거친 캡처 중심이다.
    std::int32_t radius{}; // 가상b4가 반환한 signed 정수 반경이다.
    std::array<float,4> area{}; // 정수 finder 변환 전의 left/top/right/bottom float다.
};
// 같은 세션의 현재 권한 전역이다.
struct BombTargetState {
    std::uint32_t boss{}; // 540bc4가0이면 복귀 가능한 원본 진단을 먼저 호출한다.
};
// 파생별 가상 몸체와 복귀 진단만 외부에 남긴다. 공간 탐색/기하는 직접 실행한다.
struct BombTargetHooks {
    std::function<std::int32_t(Sid,std::uint32_t)> radius; // 현재 가상 표+b4의 실제 반경 getter다.
    std::function<bool(Sid,std::uint32_t,Sid)> eligible; // 현재 가상 표+bc의 파생 대상 자격이다.
    std::function<void(std::uint32_t)> assertion; // Bomb.cpp130의 복귀 가능한 권한 진단이다.
};
// 원본radius/대상 검색 공용 몸체를 같은 물리 풀/TYPE/해시에 연결한다.
class RawBombTargets {
public:
    RawBombTargets(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const float> ranges,BombTargetState& state,BombTargetHooks hooks); // 현재10.78/188TYPE와 네 해시를 빌린다.
    std::int32_t DefaultRadius(Sid parent) const; // 실제41e3c0의 현재TYPE+44→_ftol 하위DWORD 몸체다. 가상b4 분배는 호출자가 확인한다.
    std::array<float,2> Center(Sid parent) const; // 실제41e460과 검색이 공유하는 스냅/0.5차감/보드 제한 중심이다.
    BombTargetGeometry Find(Sid parent,BombTargetList& output) const; // 현재 자격/목록을 읽으며 실제 RawSquidFinder로 순회한다.
    const SidPool& Pool() const; // 다른 세션의 연결을 섞지 않도록 확인한다.
private:
    const RiftTypeRecord& Type(Sid sid) const; // 현재 raw TYPE를 조회한다.
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; // 비정렬 원문 필드를 지정 폭으로 읽는다.
    const SidPool& pool_; // 부모/후보의 현재 물리50바이트 저장소다.
    const SquidHash& hash_; // 실제 네 단계의 현재 머리/체인 입력이다.
    std::span<const RiftTypeRecord> types_; // 효과 뒤 바뀐 후보 발자국도 여기서 다시 읽는다.
    std::span<const float> ranges_; // 현재TYPE+44의 float 반경이다.
    BombTargetState& state_; // 실제 권한 전역을 빌린다.
    BombTargetHooks hooks_; // 실제 도달한 파생/진단만 요구한다.
};
}
