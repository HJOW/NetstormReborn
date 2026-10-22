// 세 Wind 소환 주문의 가상bc가 생산 프로세스를 만들고0을 반환하는 실제 동작이다.
#pragma once
#include "o/RawWindAviaryState.h"
#include "o/SidPool.h"
#include <functional>
#include <optional>

namespace netstorm::o {
// 고정 TYPE 포인터의 spawns를 생성/부착 뒤에도 같은 현재 저장소에서 읽는다.
struct WindSpellState {
    std::uint32_t processType{59}; // 54115c의 현재 생산 프로세스 종류다.
    std::array<std::uint32_t,3> spells{171,172,173}; // 541304/308/30c의 소환 주문 종류다.
    std::span<const std::int32_t> counts; // 현재188TYPE+1c8의 signed spawns다.
};
// 선택한 조회·미정 메모리 확보·실제 부착만 현재 Session에 연결한다.
struct WindSpellHooks {
    std::function<bool(Sid,std::uint32_t)> exists; // 4b1a90(운반체,현재59,0,0)의 첫 nondead 존재 여부다.
    std::function<std::optional<WindAviaryProcessFields>()> allocate; // 원본new50의 미정80BYTE 또는 실패다.
    std::function<void(WindAviaryProcessFields,Sid,std::uint32_t,std::uint32_t)> attach; // inner+38/현재TYPE/현재 운반체/flags8의41bd20이다.
};
class RawWindSpell {
public:
    RawWindSpell(const SidPool& pool,const bool& server,const double& now,WindSpellState& state,WindSpellHooks hooks); // 같은 PATCH 풀/현재 서버·시계·188TYPE spawns를 빌린다.
    bool MayTarget(Sid spell) const; // 41ecf0/41fb60/41fcc0의 부수 효과를 실행하며 항상false를 반환한다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; // 부모WORD와 현재VT를 원본 폭으로 읽는다.
    const SidPool& pool_; // 주문의 운반체WORD+c를 현재 시점마다 조회한다.
    const bool& server_; // 최초 생성 관문과 생성자의 현재 서버 입력이다.
    const double& now_; // 45d730이 생성할 때 읽는 게임 시각이다.
    WindSpellState& state_; // 현재 종류/같은 spawns 레코드의 장기 상태다.
    WindSpellHooks hooks_; // 원본이 도달한 큰 하위만 요구한다.
};
}
