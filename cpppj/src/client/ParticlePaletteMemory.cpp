// C++ 배열 범위 밖 역참조 없이 원본의 실제 할당/읽기 경계를 OS에 전달한다.
#include "client/ParticlePaletteMemory.h"
#include <windows.h>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <system_error>

namespace netstorm::client {
// 원본처럼 색 배열의 소유권만 반납한다. 외부 snapshot을 반납 대상 주소로 쓰지 않는다.
ParticlePaletteMemory::~ParticlePaletteMemory() {
    if (allocation_) HeapFree(heap_,0,allocation_);
    if (heap_) HeapDestroy(heap_);
}
// 모드1은 요청 크기를16바이트로 올리지 않고0 플래그로 그대로 요청한다.
void ParticlePaletteMemory::Assign(std::span<const std::uint32_t> colors) {
    if (colors.empty() || colors.size()>256) throw std::invalid_argument("입자 색 배열 요청 수 오류");
    if (!heap_) { heap_=HeapCreate(0,0x1000,0);if (!heap_) throw std::bad_alloc(); }
    if (allocation_) { HeapFree(heap_,0,allocation_);allocation_=nullptr; }
    count_=0;allocation_=HeapAlloc(heap_,0,colors.size()*sizeof(std::uint32_t));if (!allocation_) throw std::bad_alloc();
    std::memcpy(allocation_,colors.data(),colors.size_bytes());count_=colors.size();readable_.resize(count_+1);
}
// 이 호스트의 실제 인접 메모리를 읽을 뿐, 관찰된 원본 값1이나 마지막 색으로 바꾸지 않는다.
std::span<const std::uint32_t> ParticlePaletteMemory::Snapshot() {
    if (!allocation_ || !count_) throw std::logic_error("입자 색 배열 생성 전 조회");
    SIZE_T received=0;const auto bytes=readable_.size()*sizeof(std::uint32_t);
    if (!ReadProcessMemory(GetCurrentProcess(),allocation_,readable_.data(),bytes,&received) || received!=bytes)
        throw std::system_error(static_cast<int>(GetLastError()),std::system_category(),"입자 인접 메모리 읽기 실패");
    return readable_;
}
// 호스트 snapshot에 추가한 읽기 공간을 색수에 포함하지 않는다.
std::size_t ParticlePaletteMemory::Count() const { return count_; }
// 실제 OS 할당에 대한 읽기 전용 관찰 주소다.
const void* ParticlePaletteMemory::Address() const { return allocation_; }
}
