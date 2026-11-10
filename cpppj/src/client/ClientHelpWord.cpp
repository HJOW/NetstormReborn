// helpWord 캐시의 갱신 규칙(원문 441300/4413c0)을 구현한다.
#include "client/ClientHelpWord.h"

namespace netstorm::client {
// ASCII 영문자만 소문자로 접어 비교한다. 128 이상 바이트는 변환하지 않는다.
bool ClientHelpWord::EqualsIgnoreCase(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    // 같은 위치의 두 바이트를 순서대로 비교하는 반복이다.
    for (std::size_t index = 0; index < left.size(); ++index) {
        auto a = static_cast<unsigned char>(left[index]), b = static_cast<unsigned char>(right[index]);
        if (a >= 'A' && a <= 'Z') a = static_cast<unsigned char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = static_cast<unsigned char>(b - 'A' + 'a');
        if (a != b) return false;
    }
    return true;
}
// 설정값이 없거나 비어 있으면 캐시를 건드리지 않는다. 같은 문자열(대소문자 무시)이면 옛 표기를 유지한다.
bool ClientHelpWord::Interpret(const std::optional<std::string>& configuredDisplay) {
    // 441300은 설정이 없거나 scratch(빈 문자열)와 같은 결과이면 0을 반환하고 기존 캐시 포인터를 읽지 않는다.
    if (!configuredDisplay || configuredDisplay->empty()) return false;
    // 기존 캐시가 있고 _stricmp가 0이면 옛 할당/표기를 유지하고 성공으로 끝낸다.
    if (value_ && EqualsIgnoreCase(*value_, *configuredDisplay)) return true;
    // 다르거나 최초이면 새 문자열로 교체한다.
    value_ = *configuredDisplay;
    ++generation_;
    return true;
}
}
