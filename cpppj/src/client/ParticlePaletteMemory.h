// 원본 heap mode1의 색 배열 요청과 실제 인접 DWORD 읽기를 Win32 메모리 경계로 유지한다.
#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace netstorm::client {
// 색수만큼만 할당한다. 추가 조회 DWORD는 생성한 색이 아니라 현재 할당 옆의 실제 메모리다.
class ParticlePaletteMemory {
public:
    ParticlePaletteMemory()=default; // 색 표가 필요하기 전에는 OS heap을 만들지 않는다.
    ~ParticlePaletteMemory(); // 배열을 반납한 뒤 소유 heap을 해제한다.
    ParticlePaletteMemory(const ParticlePaletteMemory&)=delete; // 실제 할당의 중복 반납을 막는다.
    ParticlePaletteMemory& operator=(const ParticlePaletteMemory&)=delete; // 실제 주소와 snapshot 수명을 유지한다.
    // HeapAlloc(heap,0,count*4) 뒤 실제 색 DWORD만 쓴다. 인접 값은 초기화하지 않는다.
    void Assign(std::span<const std::uint32_t> colors);
    // 현재 배열과 그 다음 DWORD를 ReadProcessMemory로 복사한다. 실패를 가짜 색으로 대체하지 않는다.
    std::span<const std::uint32_t> Snapshot();
    // 실제 요청 색 수이며 snapshot 길이보다1 작다.
    std::size_t Count() const;
    // 경계 검증에서만 현재 할당 주소를 읽는다. 소유권은 이 객체에 있다.
    const void* Address() const;
private:
    void* heap_{}; // 이 호스트의 실제 Win32 heap이다. 원본32비트 heap 내용과 동일하다고 가정하지 않는다.
    void* allocation_{}; // 색수*4바이트로 요청한 현재 실제 할당이다.
    std::size_t count_{}; // 실제11색 등의 요청 개수다.
    std::vector<std::uint32_t> readable_; // OS에서 읽은 전체 DWORD를 담는 호스트 snapshot이다.
};
}
