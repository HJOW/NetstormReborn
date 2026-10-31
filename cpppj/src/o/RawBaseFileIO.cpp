// 선택된 CRT 경계만 호출하고 원문 DWORD 감김·현재 재조회·부분 효과를 보존한다.
#include "o/RawBaseFileIO.h"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::o { // 게임 실행 없이도 같은 파일 몸체를 재사용한다.
namespace { // 원문의 연산 폭과 필수 장치 요구를 한곳에 둔다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("BaseFile의 선택된 실제 I/O 장치 미연결");return callback; } // 미선택 장치는 선행 요구하지 않는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 원문의 signed CMP/long 인자로 해석한다.
std::uint32_t Bits(std::int32_t value) { return static_cast<std::uint32_t>(value); } // signed overflow 없이 ADD/SUB를 수행한다.
}
// 부분 파일의 origin2는 length+base-offset이며 일반 FILE의 SEEK_END와 부호가 다르다.
void SeekBaseFile(RawBaseFileState& state,std::int32_t offset,std::int32_t origin,const RawBaseFileIOHooks& hooks) {
    if (!(state.flags&1U)) { Need(hooks.seek)(state.file,offset,origin==0 ? 0 : (origin!=1)+1);return; }
    if (origin==0) Need(hooks.seek)(state.file,Signed(Bits(state.offset)+Bits(offset)),0);
    else if (origin==1) Need(hooks.seek)(state.file,offset,1);
    else if (origin==2) Need(hooks.seek)(state.file,Signed(Bits(state.length)+Bits(state.offset)-Bits(offset)),0);
}
// 원문 부분 EOF는 ftell 이후의 현재 base/length와 signed 비교한다.
std::uint32_t BaseFileEof(RawBaseFileState& state,const RawBaseFileIOHooks& hooks) {
    if (!(state.flags&1U)) return Need(hooks.eofFlags)(state.file)&0x10U;
    const auto position=Need(hooks.tell)(state.file);return state.length<Signed(Bits(position)-Bits(state.offset)) ? 1U : 0U;
}
// tell/seek 실패도 다른 성공 값으로 바꾸지 않고 그 다음 원문 호출까지 진행한다.
std::int32_t BaseFileLength(RawBaseFileState& state,const RawBaseFileIOHooks& hooks) {
    if (state.flags&1U) return state.length;
    const auto position=Need(hooks.tell)(state.file);Need(hooks.seek)(state.file,0,2);const auto length=Need(hooks.tell)(state.file);Need(hooks.seek)(state.file,position,0);return length;
}
// fclose 호출이 현재 필드를 바꿔도 원문의 뒤 store가 같은 필드를 덮어쓴다.
void CloseBaseFile(RawBaseFileState& state,const RawBaseFileIOHooks& hooks) {
    if (!state.file) return;Need(hooks.close)(state.file);state.flags&=~1U;state.offset=0;state.length=0;state.file=0;
}
// FILE 없는 객체의 키도 해제하며 원문은 keyLength를0으로 쓰지 않는다.
void DestructBaseFile(RawBaseFileState& state,const RawBaseFileIOHooks& hooks) {
    CloseBaseFile(state,hooks);if (state.key) { Need(hooks.freeKey)(state.key);state.key=0; }
}
// 직접 쓰기는 한 element이며 암호 쓰기는 요청 길이대로 진행한다. CRT 부분 쓰기에서 자동 재시도하지 않는다.
void WriteBaseFile(RawBaseFileState& state,std::span<const std::uint8_t> bytes,std::int32_t offset,const RawBaseFileIOHooks& hooks) {
    if (bytes.size()>std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("BaseFile의32비트 쓰기 길이 범위");
    if (state.flags&1U) Need(hooks.assertion)(0x3a4);
    if (offset!=-1) Need(hooks.seek)(state.file,offset,0);
    if (!state.key) { Need(hooks.write)(state.file,bytes,static_cast<std::uint32_t>(bytes.size()),1);return; }
    if (!state.keyLength) Need(hooks.assertion)(0x3aa);
    const bool subfile=(state.flags&1U)!=0;const auto position=Need(hooks.tell)(state.file);auto keyIndex=subfile ? Bits(position)-Bits(state.offset) : Bits(position);
    const auto size=static_cast<std::uint32_t>(bytes.size());std::uint32_t used{};std::array<std::uint8_t,4096> scratch; // 각 묶음의 실제 입력 전체를 복사한 뒤에만 읽는다.
    while (Signed(used)<Signed(size)) { // 원문은 signed 길이를 비교하며 부분 쓰기 뒤에도 요청한 묶음 길이만큼 진행한다.
        const auto count=std::min<std::uint32_t>(size-used,4096);std::copy_n(bytes.data()+used,count,scratch.data());
        const auto keyLength=state.keyLength;const auto key=Need(hooks.key)(state.key,keyLength);if (!keyLength || key.size()<keyLength) throw std::out_of_range("BaseFile의 현재 키 물리/길이 범위");
        for (std::uint32_t index=0;index<count;++index) { scratch[index]^=key[keyIndex%keyLength];++keyIndex; } // 파일 위치가4096 경계를 넘어도 같은 키 위치를 유지한다.
        Need(hooks.write)(state.file,std::span<const std::uint8_t>(scratch).first(count),count,1);used+=count;
    }
}
}
