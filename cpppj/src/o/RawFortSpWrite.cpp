// 일반MemStream쓰기부모를대신호출하지않고실제인라인폭/현재필드/부분효과를보존한다.
#include "o/RawFortSpWrite.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr double kSigned64Limit=9223372036854775808.0; //masked_ftol의signed64변환한계이며float32는double로정확히옮겨진다.
// 실제선택한하위만그위치에서요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Fort SP writer의 실제 선택장치 누락");return function; }
// 같은실제물리의선택폭만검사하며범위밖접근은명시host계약오류다.
std::span<std::uint8_t> Range(std::span<std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("Fort SP writer의 실제 물리 범위");return bytes.subspan(offset,width);
}
// 지정한비정렬DWORD를littleendian으로읽으며다른필드를복제하지않는다.
std::uint32_t Read(std::span<std::uint8_t> bytes,std::size_t offset) {
    const auto field=Range(bytes,offset,4);std::uint32_t value{}; //같은필드의현재네BYTE다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(field[byte])<<(byte*8);return value; //원문DWORD의하위부터조합한다.
}
// 현재필드나출력의같은BYTE/WORD/DWORD폭만쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    auto field=Range(bytes,offset,width);
    for (std::size_t byte=0;byte<width;++byte) field[byte]=static_cast<std::uint8_t>(value>>(byte*8)); //폭밖의인접필드는보존한다.
}
// 실제004e49c0의0방향signed64절삭과EAX하위DWORD계약이다.호스트의범위밖정수변환을실행하지않는다.
std::uint32_t Integer(float input) {
    const double value=input;if (!std::isfinite(value) || value>=kSigned64Limit || value< -kSigned64Limit) return 0;
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value)));
}
}
// 같은입력bit는잡음OR에서제외하지만16난수의소비자체는생략하지않는다.
void WriteFortSp(std::span<std::uint8_t> stream,float value,const FortSpWriteHooks& hooks) {
    const auto integer=Integer(value); //caller의float32를한번만실제_ftol계약으로읽는다.
    for (std::uint32_t bit=0;bit<32;++bit) { //원문EBP0..31순서이며bit별일반writer호출은없다.
        auto word=integer&(1U<<bit); //해당원본bit만고정한다.
        for (std::uint32_t noise=0;noise<16;++noise) { //동일한32범위난수를매bit정확히16회소비한다.
            const auto selected=Need(hooks.nextRandom)(32);
            if (selected!=bit) word|=1U<<(selected&31U);
        }
        const std::uint32_t width=bit<8?1U:bit<16?2U:4U;std::uint32_t capacity{},sum{}; //원문의세인라인쓰기폭과비교값이다.
        if (bit<8) { sum=Read(stream,4)+width;capacity=Read(stream,12); }
        else { capacity=Read(stream,12);sum=Read(stream,4)+width; } //WORD/DWORD경로는원문처럼alloc을pos앞에읽는다.
        if (std::bit_cast<std::int32_t>(sum)>std::bit_cast<std::int32_t>(capacity)) Need(hooks.stream.assertion)(MemStreamAssertion::WriteCapacity);
        const auto position=Read(stream,4),pointer=Read(stream,0); //진단callback뒤현재pos/data를그순서로다시읽는다.
        if (!pointer) throw std::logic_error("Fort SP writer의 현재 버퍼 포인터 누락");
        const auto bytes=Need(hooks.stream.buffer)(pointer);Put(bytes,position,word,width); //같은stream자체를덮는실제버퍼도그대로쓴다.
        const auto next=Read(stream,4)+width,end=Read(stream,8); //출력쓰기효과뒤pos/size를다시읽고size는pos store앞에보관한다.
        Put(stream,4,next);
        if (std::bit_cast<std::int32_t>(next)>std::bit_cast<std::int32_t>(end)) Put(stream,8,next);
    }
}
}
