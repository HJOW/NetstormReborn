// Template9의 실제 소유/빌림 분기와20BYTE 초기화·반납 순서다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
namespace netstorm::o {
// 현재 DataManager의 실제 소멸과 할당 반납을 별개 효과로 연결한다.
struct TemplateLifetimeHooks {
    std::function<void(std::uint32_t)> destroy; // 004bd330의 실제 현재 manager 소멸이다.
    std::function<void(std::uint32_t)> release; // 같은 저장 포인터의CRT free다. 소멸 뒤 다시 헤더 포인터를 읽지 않는다.
};
// 004bd2e0: 이전 포인터를 반납하지 않고 정확한20BYTE를0으로 쓴다.
void InitializeTemplateHeader(std::span<std::uint8_t> header);
// 004bf690: 소유 manager만 소멸/free한 뒤 포인터가0이어도20BYTE를 비운다.
void ReleaseTemplateHeader(std::span<std::uint8_t> header,const TemplateLifetimeHooks& hooks);
// 004bfb00:9칸의 비영 포인터인 기록만 반납한다. 포인터0인 기록의 stale 필드는 보존한다.
void ClearTemplateHeaders(std::span<std::uint8_t> headers,const TemplateLifetimeHooks& hooks);
}
