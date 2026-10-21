// 10.78 MemStream의 실제16BYTE에 할당/쓰기/압축을 원본 순서로 적용한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
namespace netstorm::o {
// 진단 뒤에도 원본 본문이 계속하는 두 조건을 구별한다.
enum class MemStreamAssertion { ExistingData,WriteCapacity };
// 실제 힙 포인터 대신 불투명DWORD로 같은 소유 버퍼를 찾는다.
struct MemStreamHooks {
    std::function<std::span<std::uint8_t>(std::uint32_t)> buffer; // 현재 물리 버퍼 전체이며 이 조회는 소유권을 바꾸지 않는다.
    std::function<std::uint32_t(std::uint32_t)> allocate; // 원본 operator new의 요청 크기와 반환 포인터다.
    std::function<void(std::uint32_t)> release; // 원본 CRT free이며 호출 때의 같은 포인터를 반납한다.
    std::function<void(MemStreamAssertion)> assertion; // 선택된 진단만 호출하며 반환 뒤 현재 필드를 다시 읽는다.
};
// 0044cc20: alloc 선행 쓰기→할당→현재 alloc만큼0→pos/end0 순서다.
void AllocateMemStream(std::span<std::uint8_t> stream,std::uint32_t size,const MemStreamHooks& hooks);
// 0044cc80: 기존 할당 반납→첫16BYTE 비움→현재 원문 end로 할당→현재 data/end 복사, pos는0이다.
void CopyMemStream(std::span<std::uint8_t> stream,std::span<const std::uint8_t> source,const MemStreamHooks& hooks);
// 0044cce0: signed 용량 진단 뒤 DWORD/나머지 BYTE를 앞에서부터 복사하고 현재 pos/end를 갱신한다.
void WriteMemStream(std::span<std::uint8_t> stream,std::span<const std::uint8_t> source,std::uint32_t length,const MemStreamHooks& hooks);
// 0044cd40: end==alloc은 무동작, 양의 end는 재할당/복사/옛 버퍼 free, 나머지는 실제 소멸이다.
void CompactMemStream(std::span<std::uint8_t> stream,const MemStreamHooks& hooks);
}
