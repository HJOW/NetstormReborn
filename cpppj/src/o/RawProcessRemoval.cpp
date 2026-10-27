// 머리부터 반복 검색하면 삭제 콜백의 신규 객체까지 지우거나 권한 없는 Form에서 끝나지 않을 수 있다.
#include "o/RawProcessRemoval.h"
#include <stdexcept>
#include <unordered_set>
namespace netstorm::o {
namespace {
// 정렬되지 않은 종속 링크 WORD만 현재 raw에서 읽는다.
Sid Link(std::span<const std::uint8_t> raw,std::size_t offset) {
    return Sid{static_cast<std::uint16_t>(raw[offset]|static_cast<std::uint16_t>(raw[offset+1])<<8)};
}
}
// 실제4b22d0/4b1b80은 ProcessFinder의 다음 링크를 반환 전에 저장하며 dead/free를 추가 제외하지 않는다.
void RemoveAttachedProcesses(SquidProcessHost& host,Sid parent,std::uint32_t type) {
    const auto& pool=host.Pool();if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("CD 프로세스 일괄 제거 미복원");
    auto current=Link(pool.Slot(parent),6);std::unordered_set<std::uint16_t> visited;
    // 현재 후보의 next를 먼저 저장하고 효과 뒤 그 번호의 현재 raw를 재조회한다.
    while (current.value) {
        if (current.value>=pool.Capacity() || !visited.insert(current.value).second) throw std::logic_error("프로세스 제거 종속 체인 손상");
        const auto raw=pool.Slot(current);const auto next=Link(raw,4);const auto candidate=raw[10];
        // Finder의 Form 상한 뒤 가상425e80→4b1bf0 조건(10 이상·현재 ProcessTypeEnd 미만)을 적용한다.
        if (candidate<70 && candidate>=10 && candidate<host.ProcessTypeEnd()) {
            auto* process=host.FromForm(current);if (!process) throw std::logic_error("실제 제거할 프로세스 부재");
            if (!type || pool.Slot(process->Form())[10]==type) host.Kill(*process,0x40);
        }
        current=next;
    }
}
}
