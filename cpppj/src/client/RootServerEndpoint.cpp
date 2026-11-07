// 전체 정규식 엔진 대신 원문에서 사용하는 두 패턴의 첫 일치/탐욕 그룹을 그대로 계산한다.
#include "client/RootServerEndpoint.h"
#include "o/ConfigInterface.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>

namespace netstorm::client { //동일한 원문 좁은 문자 C문자열만 읽고 DNS를 호출하지 않는다.
namespace { //원문의 C문자열 끝과 길이 제한 복사만 공유한다.
// 실제 첫NUL 뒤의 저장소는 원문 패턴/CRT 입력에 포함하지 않는다.
std::string_view Text(std::string_view text) { return text.substr(0,text.find('\0')); }
// GetMatch의 최대 count 복사와 마지막NUL을 실제 제공한 목적 span에 보존한다.
void Copy(std::span<char> target,std::string_view source) { if (target.empty()) return;const auto count=std::min(target.size()-1,source.size());if (count) std::copy_n(source.data(),count,target.data());target[count]=0; }
}
// 유효 행 parser에 진입한 출력만0으로 만들며 호출자의 무효 인덱스 관문은 바깥에 남긴다.
void ReadRootServerRecord(std::string_view record,std::span<char> endpoint,std::span<char> name,std::int32_t* subscriber) {
    std::fill(endpoint.begin(),endpoint.end(),char{});std::fill(name.begin(),name.end(),char{});if (subscriber) *subscriber=0;
    record=Text(record);const auto first=record.find(';');if (first==std::string_view::npos) return;
    const auto second=record.find(';',first+1);Copy(endpoint,record.substr(0,first));Copy(name,record.substr(first+1,second==std::string_view::npos ? second : second-first-1));
    if (subscriber) { const auto suffix=second==std::string_view::npos ? std::string_view{} : record.substr(second+1);const auto group=suffix.substr(0,suffix.find('\n'));std::array<char,80> temporary{};Copy(temporary,group);*subscriber=o::ConfigParseLong(temporary.data()); }
}
// 콜론 뒤 첫 숫자 그룹을 찾을 때 앞서 실패한 콜론을 가로질러 host를 합치지 않는다.
RootServerEndpoint ResolveRootServerEndpoint(std::string_view text,std::uint32_t port,const std::function<std::int32_t()>& configuredPort) {
    text=Text(text);if (text.size()>=80) throw std::out_of_range("원문 root endpoint 지역80BYTE 물리 부족");RootServerEndpoint result{std::string(text),0};std::size_t begin{};bool matched{};
    for (auto colon=text.find(':');colon!=std::string_view::npos;colon=text.find(':',begin)) { //왼쪽부터 패턴을 시도하며 첫 성공 뒤의 문자는 읽지 않는다.
        if (colon+1<text.size() && text[colon+1]>='0' && text[colon+1]<='9') {
            auto end=colon+1;while (end<text.size() && text[end]>='0' && text[end]<='9') ++end; //연속된 십진 숫자 그룹만 탐욕적으로 소비한다.
            result.host=std::string(text.substr(begin,colon-begin));result.port=std::bit_cast<std::uint32_t>(o::ConfigParseLong(text.substr(colon+1,end-colon-1)));matched=true;break;
        }begin=colon+1;
    }
    if (!matched) { if (!configuredPort) throw std::logic_error("현재 rootServerBasePort 조회 미연결");result.port=std::bit_cast<std::uint32_t>(configuredPort()); }
    if (port) result.port=port;return result;
}
}
