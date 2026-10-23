// 원본의 조건부 Cancel 조회와 콜백 뒤 현재 depth/serial/global 재조회를 보존한다.
#include "client/MissionLoadingMessage.h"
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::size_t kTemporaryBytes=1024; //cf570이b9320에 요청하는 논리 임시 버퍼 크기이며 끝NUL까지 포함한다.
constexpr std::string_view kButtonPrefix="$ButtonInstant=",kButtonSuffix=",LeaveBattle,3"; //PE00515ee0/00515ed0의 실제 원문 바이트다.
constexpr std::int32_t kRequestedWidth=400,kRequestedHeight=30; //cf270에 전달하는 요청 크기다. 최종 Fit 결과 크기와 구별한다.
// 실행 경로가 실제로 선택한 외부 효과만 요구하며 미연결을 정상 성공으로 대체하지 않는다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("진행 안내 HTML 실제 처리기 미연결");return value; }
// C문자열 의미를 보존하되 임시 버퍼 밖의 원본 미정 쓰기는 재현하지 않는다.
void CheckSize(std::string_view text) { if (text.size()>=kTemporaryBytes) throw std::length_error("진행 안내1024BYTE 임시 버퍼 범위 초과"); }
// 기존 부모의 갱신 깊이는 가상 효과 뒤 현재 값을 다시 읽어 감소시킨다.
void Update(std::uint32_t token,std::string& source,const MissionLoadingMessageHooks& hooks) {
    ++Need(hooks.updateDepth)(token);
    Need(hooks.metadata)(token,source);Need(hooks.body)(token,std::string_view(source).substr(0,source.find('\0'))); // 메타가 바꾼 같은 C문자열의 첫NUL 앞만 본문에 전달한다.
    --Need(hooks.updateDepth)(token);
}
}
// 원본은 첫 '~.' 또는 대소문자 구별 '$Button' 부분문자열이 있으면 기본 버튼을 추가하지 않는다.
std::string BuildMissionLoadingMessageBody(std::string_view formatted,const std::function<std::string()>& cancelText) {
    formatted=formatted.substr(0,formatted.find('\0'));CheckSize(formatted);std::string body(formatted);
    if (body.empty() || body.back()!='\n') body+='\n';CheckSize(body);
    if (!body.starts_with("~.") && body.find("$Button")==std::string::npos) {
        body+=kButtonPrefix;
        const auto cancel=Need(cancelText)();body+=std::string_view(cancel).substr(0,cancel.find('\0')); // 번역의 첫NUL 뒤 바이트는 복사하지 않는다.
        body+=kButtonSuffix;CheckSize(body);
    }
    return body;
}
// 신규 경로는 create의 반환 serial과 create가 바꾼 현재createdSerial을 서로 바꾸지 않는다.
std::uint32_t ShowMissionLoadingMessage(MissionLoadingMessageState& state,std::string_view formatted,
    const MissionLoadingMessageHooks& hooks) {
    auto source=BuildMissionLoadingMessageBody(formatted,hooks.cancelText);
    if (const auto existing=Need(hooks.findSerial)(state.transientSerial)) {
        Update(existing,source,hooks);state.transientSerial=Need(hooks.serial)(existing);return state.transientSerial;
    }
    const auto created=Need(hooks.create)(source,0,kRequestedWidth,kRequestedHeight);
    if (const auto current=Need(hooks.findSerial)(state.createdSerial)) Update(current,source,hooks);
    state.transientSerial=created;return created;
}
}
