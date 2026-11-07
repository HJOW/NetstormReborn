// 원문의 로컬256BYTE와 장기 키128BYTE 경계를 구별하고 물리 밖 쓰기는 거부한다.
#include "client/SystemNewsTime.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace netstorm::client { //네트워크 연결 없이 같은 현재 서버/설정만 소비한다.
namespace { //선택되지 않은 설정 하위는 미리 요구하지 않는다.
// 누락한 실제 하위에서 앞선 키/시각 쓰기를 유지하며 중단한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("시스템 뉴스 시각의 실제 장치 미연결");return function; }
}
// 원본 strcpy의 NUL 뒤 바이트와 '.' 외 문자를 보존한다.
std::string_view BuildSystemNewsTimeKey(SystemNewsTimeGlobals g,const SystemNewsTimeHooks& h) {
    auto name=Need(h.serverName)();name=name.substr(0,name.find('\0'));const std::string formatted=std::string(name)+"NewsTime";
    if (formatted.size()>=256 || formatted.size()>=g.key.size()) throw std::out_of_range("시스템 뉴스 키의 실제 로컬/공유 물리 부족");
    std::copy(formatted.begin(),formatted.end(),g.key.begin());g.key[formatted.size()]=0;
    // 실제 작성된 C문자열만 순회하며 완성 목록/버퍼 뒤쪽은 변경하지 않는다.
    for (std::size_t at=0;at<formatted.size();++at) if (g.key[at]=='.') g.key[at]='_';
    return {g.key.data(),formatted.size()};
}
// 원문의 signed 반환 비트를 같은 DWORD에 보존하며 누락 값을 기존 시각으로 대체하지 않는다.
void ReadSystemNewsTime(SystemNewsTimeGlobals g,const SystemNewsTimeHooks& h) { const auto key=BuildSystemNewsTimeKey(g,h);g.value=std::bit_cast<std::uint32_t>(Need(h.read)(key)); }
// 현재값 캡처는 서버 이름 getter보다 먼저다. 설정문은 원문처럼 키를 별도로 이스케이프하지 않는다.
void WriteSystemNewsTime(SystemNewsTimeGlobals g,const SystemNewsTimeHooks& h) {
    const auto value=g.value;const std::string key(BuildSystemNewsTimeKey(g,h));std::array<char,256> expression;
    const auto count=std::snprintf(expression.data(),expression.size(),"%s=\"%d\"",key.c_str(),std::bit_cast<std::int32_t>(value));
    if (count<0 || static_cast<std::size_t>(count)>=expression.size()) throw std::out_of_range("시스템 뉴스 설정문256BYTE 물리 부족");
    Need(h.configure)({expression.data(),static_cast<std::size_t>(count)});
}
}
