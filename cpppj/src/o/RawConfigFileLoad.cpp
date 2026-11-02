// 원문 파일 부모의 미발견·짧은 최소할당·조건해독과 정상free 순서를 같은 버퍼에서 유지한다.
#include "o/RawConfigFileLoad.h"
#include "o/BaseFile.h"
#include <stdexcept>

namespace netstorm::o { // File/키의 완료ctor 정리는 기존41baa0 몸체가 맡는다.
namespace { // 선택한 효과만 요구하며 미발견에는 하위 장치를 쓰지 않는다.
template<class F> const F& ConfigNeed(const F& hook) { if (!hook) throw std::logic_error("Config 파일 부모의 선택된 실제 장치 미연결");return hook; } // 빈 성공으로 미연결 효과를 숨기지 않는다.
}
void LoadConfigFile(RawBaseFileState& file,std::span<const std::uint8_t> name,const RawBaseFileIOHooks& io,const BaseFileWholeReadHooks& whole,const ConfigFileLoadBodyHooks& body,std::span<std::uint8_t> scratch) {
    std::int32_t length{};const auto token=ReadWholeBaseFile(file,name,1,&length,1,io,whole,scratch);if (!token) return;
    auto bytes=ConfigNeed(body.buffer)(token);if (length<0 || bytes.size()<3 || static_cast<std::uint32_t>(length)>=bytes.size()) throw std::out_of_range("Config 파일 부모의 헤더/끝NUL 실제 물리 범위");
    if (bytes[0]==0 && bytes[2]==0 && length>0) ApplyXor(bytes.first(static_cast<std::size_t>(length))); // 원문은 논리길이3 이상 조건을 추가하지 않고 최소4 할당의 첫째/셋째 BYTE를 읽는다.
    bytes[static_cast<std::size_t>(length)]=0;ConfigNeed(body.append)(token,bytes.first(static_cast<std::size_t>(length)));ConfigNeed(body.release)(token); // 원문에는 Append 예외의 free/unwind가 없으며 host 추가 RAII는 바깥 경계다.
}
}
