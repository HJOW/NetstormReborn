// 10.78 DataManager의 연결 섹션과 MemStream 버퍼를 원본 소멸 순서로 반납한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
namespace netstorm::o {
// 실제 호스트 포인터 대신 DWORD 핸들로 현재 섹션과 두 종류의 할당을 조회한다.
struct DataManagerLifetimeHooks {
    std::function<std::span<std::uint8_t>(std::uint32_t)> stream; // 현재 노드의 이름/next를 포함한60BYTE 이상을 빌린다.
    std::function<void(std::uint32_t)> releaseBuffer; // MemStream 첫DWORD의 실제 버퍼 할당만 반납한다.
    std::function<void(std::uint32_t)> releaseStream; // 버퍼 소멸 뒤 같은60BYTE 노드의 할당을 반납한다.
};
// 0044d040: 비영 버퍼를 먼저 free한 뒤 pointer/alloc/end/pos만0으로 만든다.
void DestroyMemStream(std::span<std::uint8_t> stream,const std::function<void(std::uint32_t)>& releaseBuffer);
// 004bd330→0044da60: base vptr를 쓰고 현재 head를 먼저 unlink한 뒤 각 노드를 소멸한다.
// manager는 최소12BYTE, stream은 최소60BYTE다. manager.current와 섹션 이름/next는 보존한다.
void DestroyDataManager(std::span<std::uint8_t> manager,const DataManagerLifetimeHooks& hooks);
}
