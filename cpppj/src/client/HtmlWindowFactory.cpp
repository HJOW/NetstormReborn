// 실제 alnum 관문과 원문의 기본 버튼/줄바꿈 변환·빈 본문xxx·현재serial을 유지한다.
#include "client/HtmlWindowFactory.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::client { // 실제 전체 HTML 창 생성 구현이다.
namespace { // 고정 원문 버퍼와 선택 경계 확인만 모은다.
constexpr std::size_t kHtmlFactoryCapacity=8192; // 원문 각 지역 출력과 부모 링크 저장소 크기다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("HTML 전체 생성 실제 장치 미연결");return hook; } // 해당 원문 분기에서만 장치를 확인한다.
std::string_view CString(std::string_view text) { return text.substr(0,text.find('\0')); } // 원문 첫NUL 뒤 바이트는 처리하지 않는다.
void Copy(std::span<char> target,std::string_view text) { text=CString(text);if (text.size()>=target.size()) throw std::length_error("HTML 전체 생성 지역 NUL 범위 초과");std::copy(text.begin(),text.end(),target.begin());target[text.size()]=0; } // 실제 물리 버퍼 밖 원문 입력만 거부한다.
bool HasContent(std::string_view text) { for (const unsigned char byte:CString(text)) if ((byte>='a' && byte<='z') || (byte>='A' && byte<='Z') || (byte>='0' && byte<='9')) return true;return false; } //4c8d50/C로케일의 실제 바이트 순회다.
}
HtmlWindowFactory::HtmlWindowFactory(HtmlWindowGumpState& state,HtmlWindowFactoryHooks hooks):state_(state),hooks_(std::move(hooks)) {} // 상태를 복제하거나 미리 선택 장치를 실행하지 않는다.
HtmlWindowGumpNode& HtmlWindowFactory::Node(std::uint32_t token) const { auto* node=token ? Need(hooks_.node)(token) : nullptr;if (!node) throw std::logic_error("HTML 전체 생성 현재 부모 없음");return *node; } // 생성/콜백 뒤에도 같은 실제 저장소를 다시 읽는다.
std::uint32_t HtmlWindowFactory::Show(std::string_view source,std::uint32_t suppress,std::uint32_t width,std::uint32_t height) const {
    const auto root=suppress ? 0U : Need(hooks_.create)(width,height);state_.createdSerial=root ? Node(root).serial : 0U;
    const auto substituted=Need(hooks_.substitute)(CString(source),static_cast<std::uint32_t>(kHtmlFactoryCapacity));if (CString(substituted).size()>=kHtmlFactoryCapacity) throw std::length_error("HTML 치환 지역8192BYTE 범위 초과");
    HtmlTextParserState parser;std::array<char,kHtmlFactoryCapacity> first{};Copy(first,Need(hooks_.parse)(parser,root,CString(substituted),static_cast<std::uint32_t>(kHtmlFactoryCapacity)));if (!root) return 0;
    if (!HasContent(first.data())) { state_.createdSerial=0;Need(hooks_.body)(root,"xxx");Need(hooks_.attach)(root,0,0,Need(hooks_.read)(0x563e38),0);Need(hooks_.kill)(root);return 0; }
    state_.lastCloseTime=Need(hooks_.wallSeconds)();if (!Need(hooks_.metadata)(root,first)) { std::array<char,kHtmlFactoryCapacity> fallback{};Copy(fallback,Need(hooks_.fallback)());Need(hooks_.metadata)(root,fallback); }
    std::string secondSource;for (const char byte:CString(first.data())) { if (byte=='\n') secondSource+="<br>";else secondSource+=byte; } // 실제String.Replace의모든LF를BR로바꾼다. 첫 메타가 바꾼 현재 버퍼를 읽는다.
    auto second=Need(hooks_.parse)(parser,root,secondSource,static_cast<std::uint32_t>(kHtmlFactoryCapacity));second.resize(CString(second).size());if (second.size()+1>=kHtmlFactoryCapacity) throw std::length_error("HTML 마지막 LF/NUL 지역 범위 초과");second+='\n';Need(hooks_.body)(root,second);
    const auto screenHeight=std::bit_cast<std::int32_t>(Need(hooks_.read)(0x5c78f4));const auto parent=Need(hooks_.read)(0x563e38);const auto y=static_cast<float>((static_cast<double>(screenHeight)-Node(root).baseHeight)*0.5);const auto screenWidth=std::bit_cast<std::int32_t>(Need(hooks_.read)(0x5c78f0));const auto x=static_cast<float>((static_cast<double>(screenWidth)-Node(root).baseWidth)*0.5);
    Need(hooks_.attach)(root,x,y,parent,0);Need(hooks_.modal)(root,true);return Node(root).serial; // Modal효과 뒤의 현재serial을 읽으며 createdSerial을 반환값으로 재사용하지 않는다.
}
}
