// 명령을 페이지 전환 예약과 섞지 않고 기존40칸 입력 큐로 보낸다.
#include "client/ScriptCommand.h"
#include "o/RawTextToken.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
// NUL 뒤 원문은 원본 strlen/strcpy에서 보이지 않는다.
std::uint32_t ScriptStringRing::Store(std::string_view text) {
    index_=(index_+1)%slots_.size();auto& slot=slots_[index_];slot={};
    if (serial_==0xffffffffU) throw std::overflow_error("스크립트 문자열 수명 토큰 소진");
    slot.text=text.substr(0,text.find('\0'));slot.token=++serial_;return slot.token;
}
// 고리가 덮어쓴 이전 수명을 새 문자열로 잘못 읽지 않는다.
std::string_view ScriptStringRing::Resolve(std::uint32_t token) const {
    // 실제 현재80개 항목만 번호로 조회하며 문자열 값의 중복은 합치지 않는다.
    for (const auto& slot:slots_) if (slot.token && slot.token==token) return slot.text;
    throw std::out_of_range("스크립트 문자열의 해제되었거나 잘못된 토큰");
}
// 원본 전역 인덱스의 현재 값을 제공한다.
std::size_t ScriptStringRing::Index() const { return index_; }
// 실제 lookup0의 진단과 lookup-1의 지연 거부는 다른 분기다.
void QueueScriptCommand(std::string_view source,InputQueue& queue,ScriptStringRing& strings,const ScriptCommandHooks& hooks) {
    if (!hooks.lookup) throw std::invalid_argument("스크립트 명령 표 조회 미연결");
    const auto command=o::ReadRawTextToken(source,0,"#,;>"); // 첫 필드는 원본 네 구분자를 사용한다.
    const auto argument=o::ReadRawTextToken(source,command.next,";>"); // 두 번째 필드의 쉼표는 보존한다.
    const auto code=hooks.lookup(command.text); // 인자 변환 전에 실제 표를 조회한다.
    const auto diagnose=[&] { if (!hooks.unknown) throw std::logic_error("스크립트 미등록 명령 진단 미연결");hooks.unknown(command.text); };
    if (!code) diagnose();
    const auto parameter=!argument.text.empty() && argument.text.front()>='0' && argument.text.front()<='9' ?
        std::bit_cast<std::uint32_t>(o::ConfigParseLong(argument.text)) : strings.Store(argument.text); // 부호로 시작하면 문자열이다.
    if (code==0xffffffffU) { diagnose();return; }
    queue.PushCommand(code,std::bit_cast<std::int32_t>(parameter),std::bit_cast<std::int32_t>(parameter));
}
// 실제 Set은 기존 값을 덮어쓰며 append처럼 첫 정의를 보존하지 않는다.
std::int32_t ApplyScriptConfig(std::string_view source,o::ConfigInterface& config,const std::function<void()>& noKeyword) {
    return ApplyScriptConfig(source,[&](std::string_view key,std::string_view value) { return config.Set(key,value); },noKeyword);
}
// 인용공백 키도 원본이 받은 그대로 실제 Set에 전달하며 조회 문법으로 재해석하지 않는다.
std::int32_t ApplyScriptConfig(std::string_view source,const std::function<bool(std::string_view,std::string_view)>& set,const std::function<void()>& noKeyword) {
    const auto keyword=o::ReadRawTextToken(source,0,"="); // 키는 따옴표/78입력 제한까지 공통 helper와 같다.
    const auto value=o::ReadRawTextToken(source,keyword.next,"\n"); // 값 안의 '='와';'는 자르지 않는다.
    if (keyword.text.empty()) { if (noKeyword) noKeyword();return 0; }
    if (!set) throw std::logic_error("스크립트 Config의 실제 설정 쓰기 미연결");set(keyword.text,value.text);return 0;
}
}
