// 공통 토큰 helper는 HTML/Config의 실제 제한과 포인터 소비 순서를 공유한다.
#include "o/RawTextToken.h"
#include <stdexcept>

namespace netstorm::o {
// NUL은 입력/구분자 C문자열의 끝이며 span 뒤로 읽지 않는다.
RawTextToken ReadRawTextToken(std::string_view source,std::size_t offset,std::string_view delimiters) {
    constexpr std::size_t kInputLimit=78; // 원본80바이트 지역 버퍼의 입력 바이트 상한이다.
    if (offset>source.size()) throw std::out_of_range("원본 토큰의 입력 위치 범위 오류");
    source=source.substr(0,source.find('\0'));delimiters=delimiters.substr(0,delimiters.find('\0'));
    if (offset>source.size()) throw std::out_of_range("원본 토큰의 NUL 뒤 입력 위치");
    // 첫 구분자를 찾기 전에 space/tab만 건너뛴다. 개행과 나머지 공백은 보존한다.
    while (offset<source.size() && (source[offset]==' ' || source[offset]=='\t')) ++offset;
    RawTextToken result;bool quoted=false;std::size_t remaining=kInputLimit;
    // 인용부호도 원본 입력 제한을 하나 소비하지만 출력에는 쓰지 않는다.
    while (offset<source.size()) {
        const char byte=source[offset]; // 같은 현재 입력의 한 바이트다.
        if (!remaining || (!quoted && delimiters.find(byte)!=std::string_view::npos)) {
            // 제한 때문에 멈춘 경우에도 현재 연속 구분자는 인용 상태와 무관하게 건너뛴다.
            while (offset<source.size() && delimiters.find(source[offset])!=std::string_view::npos) ++offset;
            break;
        }
        if (byte=='"') quoted=!quoted;else result.text+=byte;
        ++offset;--remaining;
    }
    // 원본은 뒤 공백을 지워도 첫 출력 바이트는 남긴다. 인용한 공백 한 글자도 빈 문자열이 아니다.
    while (result.text.size()>1 && (result.text.back()==' ' || result.text.back()=='\t')) result.text.pop_back();
    result.next=offset;return result;
}
}
