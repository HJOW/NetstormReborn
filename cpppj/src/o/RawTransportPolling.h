// 여러 실제 전송 owner를 원문 하나의 현재 ID 순회로 묶을 때 쓰는 작은 결과/조회 도구다.
#pragma once
#include "o/RawConnectionRegistry.h"
#include <stdexcept>

namespace netstorm::o { // 실제 노드/표는 기존 Registry가 계속 소유한다.
// 무효 프레임의 소비와 길이0 sentinel의 반복 중단을 구별한다.
struct TransportPollResult {
    bool consumed{}; // 한 완성 프레임을 실제 입력에서 꺼냈는지다.
    bool again{}; // 같은 현재 정상 ID를 다시 조회하여 읽어야 하는지다.
};
// 원문 -31..-1 pending과0..21 정상 칸의 현재 실제 토큰만 읽는다.
inline std::uint32_t CurrentTransportToken(RawConnectionRegistry& registry,std::int32_t id) {
    auto table=registry.Cleanup();
    if (id<=-static_cast<std::int32_t>(table.pending.size()) || id>=static_cast<std::int32_t>(table.transceivers.size())) throw std::out_of_range("원문 현재 전송 ID 범위 오류");
    return id<0 ? table.pending[static_cast<std::size_t>(-id)] : table.transceivers[static_cast<std::size_t>(id)];
}
}
