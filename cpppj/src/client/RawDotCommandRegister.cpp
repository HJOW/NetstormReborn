// 부모가 직접 쓰는 인접 물리와 호출 뒤 현재count 재조회를 원문 순서로 보존한다.
#include "client/RawDotCommandRegister.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kCallbackOffset=0x800,kVisibleOffset=0x900,kCountOffset=0xa04; // 같은 전역표 시작에 대한 세 실제 오프셋이다.
// 원문 DWORD 접근이 실제 할당을 벗어나는 경우만 명시적으로 중단한다.
std::span<std::uint8_t> Physical(std::span<std::uint8_t> state,std::uint32_t offset,std::uint32_t count) {
    if (offset>state.size() || count>state.size()-offset) throw std::out_of_range("점 명령 등록의 실제 인접 표 범위");
    return state.subspan(offset,count);
}
// host 정렬·alias 규칙에 기대지 않고 원문의 같은 little endian DWORD를 읽는다.
std::uint32_t Read(std::span<std::uint8_t> state,std::uint32_t offset) {
    const auto bytes=Physical(state,offset,4);std::uint32_t value{}; // 실제 네BYTE만 읽는다.
    for (std::uint32_t index=0;index<4;++index) value|=std::uint32_t(bytes[index])<<(index*8); // 낮은BYTE부터원래DWORD를구성한다.
    return value;
}
// CPU DWORD store와 같은 물리 범위를 먼저 확인한 뒤 같은 네BYTE를 쓴다.
void Write(std::span<std::uint8_t> state,std::uint32_t offset,std::uint32_t value) {
    auto bytes=Physical(state,offset,4);
    for (std::uint32_t index=0;index<4;++index) bytes[index]=static_cast<std::uint8_t>(value>>(index*8)); // 인접 필드와 겹치는 실제 쓰기도 자동분리하지 않는다.
}
}

// 이름31BYTE와 NUL 뒤 padding은 CRT 하위이고 마지막32번째BYTE는 등록 부모가 쓴다.
void CopyDotCommandName(std::span<std::uint8_t> destination,std::span<const std::uint8_t> source,std::uint32_t count) {
    if (destination.size()<count) throw std::out_of_range("점 명령 이름 복사의 목적 물리 범위");
    bool ended{}; // NUL을 한 번 읽은 뒤에는 source를 더 읽지 않는다.
    for (std::uint32_t index=0;index<count;++index) { // 원문 strncpy의 count만큼 순방향으로 복사하거나 padding한다.
        if (!ended && index>=source.size()) throw std::out_of_range("점 명령 이름 복사의 원본 물리 범위");
        const auto value=ended ? std::uint8_t{} : source[index];ended=ended || value==0;destination[index]=value;
    }
}

// 복사대상은진입count로계산하고복사뒤count한번을PE의ECX처럼끝까지보관한다.
std::uint32_t RegisterDotCommand(std::span<std::uint8_t> state,std::span<const std::uint8_t> name,std::uint32_t callback,std::uint32_t flag,const DotCommandRegisterHooks& hooks) {
    const auto initial=Read(state,kCountOffset);const auto signedCount=std::bit_cast<std::int32_t>(initial); //PE의mov/cmp/jl은같은최초DWORD를signed비교한다.
    if (signedCount>=64) return 0;
    if (signedCount<0) throw std::out_of_range("점 명령 음수 초기count의 원시 주소는 지원 물리 계약 밖");
    const auto target=initial*32U;
    if (!hooks.copy) throw std::logic_error("점 명령 등록의 실제 strncpy 장치 미연결");
    hooks.copy(Physical(state,target,31),name,31);
    const auto selected=Read(state,kCountOffset); //0043f683의현재count를단한번캡처하며인접store로덮여도다시읽지않는다.
    Write(state,kCallbackOffset+selected*4U,callback);
    Physical(state,selected*32U+31U,1)[0]=0;
    Write(state,kVisibleOffset+selected*4U,flag ? kDotCommandNamesToken+selected*32U : kDotCommandEmptyToken);
    const auto updated=selected+1U;Write(state,kCountOffset,updated); //원문은현재메모리가아닌보관ECX를증가한다.
    Write(state,kVisibleOffset+updated*4U,0); //count64는count자체를0덮기/count65는66쓰기뒤다음표밖종단접근으로명시중단한다.
    return 1;
}
}
