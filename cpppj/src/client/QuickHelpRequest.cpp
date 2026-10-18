// 실제 명령의 두 번 접미 비교와 signed lookup·가상 부착 순서를 유지한다.
#include "client/QuickHelpRequest.h"
#include "o/OriginalText.h"
#include <stdexcept>
#include <utility>
namespace netstorm::client {
namespace {
constexpr std::size_t kTopicBytes=80; // 원본 지역 주제 버퍼80바이트이며 마지막 NUL도 이 안에 들어가야 한다.
// 실행한 분기의 외부 몸체가 없으면 성공으로 가장하지 않는다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("Quick Help 요청 실제 경계 누락");return value; }
// 원본은 마지막4문자를 비교한다. 버퍼 앞을 읽는 정의되지 않은 입력은 호스트에서 거부한다.
void RemoveSuffix(std::string& topic,std::string_view suffix) {
    if (topic.size()<suffix.size()) throw std::out_of_range("Quick Help 접미 비교의 원본 버퍼 앞 읽기");
    if (o::AsciiLower(std::string_view(topic).substr(topic.size()-suffix.size()))==suffix) topic.resize(topic.size()-suffix.size());
}
}
// 같은 기본 Gump를 빌리며 구성 중 실제 창/조회/입력을 실행하지 않는다.
QuickHelpRequest::QuickHelpRequest(GumpCreateNode& window,QuickHelpRequestHooks hooks):window_(window),hooks_(std::move(hooks)) {}
// 이력/창에는 원래 주제를 쓰며 접미가 제거된 복사본은 타입 이름 조회에만 사용한다.
void QuickHelpRequest::Open(std::uint32_t type,std::string_view topic) const {
    topic=topic.substr(0,topic.find('\0'));
    if (!type) {
        if (topic.size()>=kTopicBytes) throw std::length_error("Quick Help 원본 주제80바이트 범위 초과");
        std::string stem(topic);RemoveSuffix(stem,"type");RemoveSuffix(stem,"help");
        const auto query=[&](std::string_view pattern) { return Need(hooks_.lookup)(Need(hooks_.print)(pattern,stem)); }; // 같은 원문 접두 서식과 signed 조회를 순서대로 호출한다.
        auto found=query("%s");if (found<0) found=query("sun%s");if (found<0) found=query("wind%s");type=found<0 ? 0U : static_cast<std::uint32_t>(found);
    }
    Need(hooks_.history)(topic);Need(hooks_.show)(type,topic);
    if (!(window_.flags&8U)) { Need(hooks_.push)();Need(hooks_.attach)(window_.x,window_.y,0,0); }
}
}
