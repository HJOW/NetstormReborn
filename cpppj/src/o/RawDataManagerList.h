// 같은 manager head/current와 섹션 next를 사용하는10.78 추가/제거 본문이다.
#pragma once
#include "o/RawDataManagerLifetime.h"
namespace netstorm::o {
// 조회는 같은 실제 노드이며 압축/소멸/반납은 현재 소유자의 실제 소비자다.
struct DataManagerListHooks {
    DataManagerLifetimeHooks lifetime; // 같은60BYTE 조회·버퍼 free·노드 free를 공유한다.
    std::function<void(std::uint32_t)> compact; // 마지막 노드의 실제0044cd40이며 그 뒤 현재 필드를 다시 읽는다.
    std::function<void()> missingPredecessor; // 0044d0f0의temp->next 진단이다. 반환 뒤 같은 선행 노드를 다시 읽는다.
};
// 0044ce10: 비어 있지 않은 목록은 마지막 stream 압축 뒤 next/current에 새 핸들을 쓴다.
void AppendDataManagerStream(std::span<std::uint8_t> manager,std::uint32_t stream,const DataManagerListHooks& hooks);
// 0044d0f0: 먼저 unlink하고 같은 노드의 MemStream 소멸→노드 반납을 수행한다. current는 보존한다.
void RemoveDataManagerStream(std::span<std::uint8_t> manager,std::uint32_t stream,const DataManagerListHooks& hooks);
}
