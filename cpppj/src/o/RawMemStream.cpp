// 할당/진단/free 뒤의 원본 재조회와 DWORD 단위의 겹치는 복사를 보존한다.
#include "o/RawMemStream.h"
#include "o/RawDataManagerLifetime.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::o {
namespace {
// 인접 바이트에 영향 없이 비정렬DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 원본 little endian 네 바이트만 합친다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);return value;
}
// 원본 필드의 네 바이트만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 지정DWORD 밖의 이름/링크를 보존한다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 같은16BYTE 계약을 본문 첫 쓰기 전에 확인한다.
void Physical(std::span<std::uint8_t> stream) { if (stream.size()<16) throw std::invalid_argument("MemStream 실제16BYTE 누락"); }
// 진단을 무시하는 빈 소비자를 자동으로 추가하지 않는다.
void Assert(const MemStreamHooks& hooks,MemStreamAssertion kind) { if (!hooks.assertion) throw std::logic_error("MemStream 실제 진단 미연결");hooks.assertion(kind); }
// 원본이 실제 읽거나 쓰는 범위만 빌리며0길이는 버퍼 조회 없이 허용한다.
std::span<std::uint8_t> Range(const MemStreamHooks& hooks,std::uint32_t pointer,std::uint32_t position,std::uint32_t length) {
    if (!length) return {};
    if (!pointer || !hooks.buffer) throw std::logic_error("MemStream 실제 버퍼 조회 미연결");auto bytes=hooks.buffer(pointer);
    if (position>bytes.size() || length>bytes.size()-position) throw std::out_of_range("MemStream 실제 버퍼 물리 범위");return bytes.subspan(position,length);
}
}
// allocator가alloc을 바꾸면 원본처럼 그 현재 값만큼0으로 채운다.
void AllocateMemStream(std::span<std::uint8_t> stream,std::uint32_t size,const MemStreamHooks& hooks) {
    Physical(stream);if (Read(stream,0)) Assert(hooks,MemStreamAssertion::ExistingData);Put(stream,12,size);
    if (!hooks.allocate) throw std::logic_error("MemStream 실제 할당 미연결");const auto pointer=hooks.allocate(size);Put(stream,0,pointer);
    const auto length=Read(stream,12);auto target=Range(hooks,pointer,0,length);std::fill(target.begin(),target.end(),std::uint8_t{});Put(stream,4,0);Put(stream,8,0);
}
// 일반 memcpy/memmove로 바꾸지 않고 원본 한DWORD 읽기/쓰기를 반복한다.
void WriteMemStream(std::span<std::uint8_t> stream,std::span<const std::uint8_t> source,std::uint32_t length,const MemStreamHooks& hooks) {
    Physical(stream);const auto sum=Read(stream,4)+length;
    if (std::bit_cast<std::int32_t>(Read(stream,12))<std::bit_cast<std::int32_t>(sum)) Assert(hooks,MemStreamAssertion::WriteCapacity);
    if (length>source.size()) throw std::out_of_range("MemStream 실제 원문 물리 범위");auto target=Range(hooks,Read(stream,0),Read(stream,4),length);std::uint32_t position{};
    // 겹치는 버퍼도 원본처럼 한DWORD를 완전히 읽은 뒤 같은 네 바이트를 쓴다.
    for (;length-position>=4;position+=4) { const auto value=Read(source,position);Put(target,position,value); }
    // 남은1~3BYTE는 앞에서부터 한 바이트씩 읽고 쓴다.
    for (;position<length;++position) target[position]=source[position];
    const auto current=Read(stream,4)+length;Put(stream,4,current);
    if (std::bit_cast<std::int32_t>(Read(stream,8))<std::bit_cast<std::int32_t>(current)) Put(stream,8,current);
}
// 양의 재할당에서는 free 뒤의 필드 변이를 다시0으로 덮지 않는다.
void CompactMemStream(std::span<std::uint8_t> stream,const MemStreamHooks& hooks) {
    Physical(stream);const auto end=Read(stream,8);if (end==Read(stream,12)) return;const auto previous=Read(stream,0);
    if (std::bit_cast<std::int32_t>(end)>0) {
        Put(stream,0,0);AllocateMemStream(stream,end,hooks);const auto length=Read(stream,12);const auto source=Range(hooks,previous,0,length);WriteMemStream(stream,source,length,hooks);
        if (!hooks.release) throw std::logic_error("MemStream 옛 버퍼 반납 미연결");hooks.release(previous);
    } else DestroyMemStream(stream,hooks.release);
}
}
