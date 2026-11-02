// 전체파일 읽기의 원문현재값/모듈러추가폭과 ctor완료 전후 반납을 구별한다.
#include "o/RawBaseFileWholeRead.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o { // FILE/키는 caller의 동일한 실제 IO 장치를 사용한다.
namespace { // 실제 선택한 하위만 요구하며 빈 장치를 성공으로 대신하지 않는다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("BaseFile 전체읽기의 선택된 장치 미연결");return callback; } // 존재조회가 꺼져 있으면 probe는 필요 없다.
}
// 41baa0의 현재 출처/할당/출력 순서를 유지하며 완료된 File의 예외 반납만 수행한다.
std::uintptr_t ReadWholeBaseFile(RawBaseFileState& state,std::span<const std::uint8_t> name,std::uint32_t checkExists,std::int32_t* lengthOut,std::int32_t extra,const RawBaseFileIOHooks& io,const BaseFileWholeReadHooks& hooks,std::span<std::uint8_t> scratch) {
    if (checkExists && !Need(hooks.probe)(name)) return 0; // 사전미발견은key/lengthOut/할당/FILE을 전혀 건드리지 않는다.
    state.key=0;state.keyLength=0;Need(hooks.open)(state,name,0,1); // 원문은file/flags/base/length를 먼저0으로 만들지 않는다. 이open예외에는아직outerFile소멸자가등록되지않았다.
    if (!state.file) { DestructBaseFile(state,io);return 0; } // 성공ctor 뒤현재FILE0인 분기도key를 정리하지만lengthOut은 쓰지 않는다.
    std::uintptr_t result{}; // 원문 반환토큰은 정상소멸 뒤에만 caller로 전달한다.
    try {
        const auto length=BaseFileLength(state,io);const auto sum=static_cast<std::uint32_t>(length)+static_cast<std::uint32_t>(extra);const auto requested=std::bit_cast<std::int32_t>(sum)<4 ? 4U:sum;
        const auto allocation=Need(hooks.allocate)(requested);const auto readSize=static_cast<std::uint32_t>(length); // 원문의DWORD 요청은 유지하고 음수/할당밖 미정 메모리는 호스트에서 거부한다.
        if (allocation.bytes.size()<requested || readSize>allocation.bytes.size()) throw std::out_of_range("BaseFile 전체읽기의 원문 요청/할당 물리 범위");
        ReadBaseFile(state,allocation.bytes.first(readSize),-1,io,scratch);if (lengthOut) *lengthOut=length;result=allocation.token;
    } catch (...) { DestructBaseFile(state,io);throw; } // 완료ctor의 길이/할당/읽기 예외는현재File/키만반납한다. 원문에없는출력버퍼free는추가하지않는다.
    DestructBaseFile(state,io);return result; // 정상destructor예외는다시destructor로진입하지않는다.
}
}
