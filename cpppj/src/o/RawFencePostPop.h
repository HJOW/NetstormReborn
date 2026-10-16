// PATCH10.78 썬 펜스의 최초 후처리를 실제 이웃/연결선/Regular 소비자와 조합한다.
#pragma once
#include "o/RawGraph.h"
#include "o/SidPool.h"
#include <array>
#include <functional>

namespace netstorm::o {
// 00457560의 하위 몸체다. 선택된 효과는 반드시 연결하며 빈 성공으로 대체하지 않는다.
struct FencePostPopHooks {
    std::function<std::uint32_t(Sid)> theme; // 현재 TYPE+98의 원소 번호를 조회한다.
    std::function<Sid(Sid,std::uint32_t)> neighbor; // 00456220의 방향0/2/4/6 최근접 펜스다.
    std::function<bool()> reserveRegular; // 각 new(0x28)의 독립 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 동일 부모의 실제 사건을 예약한다.
    std::function<void(std::array<float,4>)> clearLine; // 00455b20(mode0)의 기존 연결선 삭제다.
    std::function<std::uint32_t(Sid)> connections; // 004573d0의 현재 유효 연결 개수다.
    std::function<void(Sid,std::uint32_t,const RawGraphPop*)> validateBase; // Pop 쓰기 전 공통 장부를 확인한다.
    std::function<void(Sid,std::uint32_t)> damageable; // 0044bf80의 같은 비가상 후처리 사슬이다.
};
// 입력 풀은 이 객체보다 오래 살아야 하며 후처리 중 효과가 바꾼 바이트를 다시 읽는다.
class RawFencePostPop {
public:
    RawFencePostPop(SidPool& pool,FencePostPopHooks hooks); // PATCH만 허용하고 하위 호출은 선택 시 확인한다.
    bool Handles(Sid sid) const; // 생성자00457e80의 실제 가상 표00509470을 확인한다.
    void Validate(Sid sid,std::uint32_t flags,const RawGraphPop* projected=nullptr) const; // 같은 공간/장부를 사전 확인한다.
    void PostPop(Sid sid,std::uint32_t flags) const; // 원본00457560의 전체 접두와 Damageable을 순서대로 실행한다.
    const SidPool& Pool() const; // 상위 분배기가 같은 풀인지 확인할 때 사용한다.
private:
    void Regular(Sid sid,std::uint32_t event) const; // 개별 확보 실패 뒤에도 다음 예약을 계속한다.
    void ClearPair(Sid first,Sid second) const; // 현재 두 끝점의 좌표를 읽어 mode0 삭제를 요청한다.
    SidPool& pool_; // 현재 원본50BYTE 슬롯을 소유한 실제 월드 풀이다.
    FencePostPopHooks hooks_; // 조회/힙/예약/연결선/공통 후처리의 실제 소비자다.
};
}
