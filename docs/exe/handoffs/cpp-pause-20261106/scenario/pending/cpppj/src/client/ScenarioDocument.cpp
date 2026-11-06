// Config의 첫 키/주석/이스케이프 경계를 재사용하고 선택한 값 이외의 대본은 보존한다.
#include "client/ScenarioDocument.h"
#include "o/BaseFile.h"
#include "o/Config.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 편집 텍스트는 월드/전역 Config 등록부와 수명을 공유하지 않는다.
namespace { // 값 구간과 새 문서 서식만 담당하는 내부 도구다.
constexpr std::string_view kBom="\xef\xbb\xbf"; // 새 UTF-8 파일을 Config가 CP1252로 오인하지 않게 하는 BOM이다.
// 키/값 주위에서 실제 Config가 건너뛰는 두 공백이다.
bool Blank(char value) { return value==' ' || value=='\t'; }
// 유효한 단일 설정 키만 받아 줄이나 섹션을 삽입하는 입력을 거부한다.
void CheckKey(std::string_view key) {
    if (key.empty()) throw std::invalid_argument("시나리오 설정 키가 비었습니다");
    for (const char value:key) { // 지원 키의 ASCII 문자/숫자만 허용하며 대소문자는 보존한다.
        if (!((value>='a' && value<='z') || (value>='A' && value<='Z') || (value>='0' && value<='9') || value=='_'))
            throw std::invalid_argument("시나리오 설정 키에 지원하지 않는 문자가 있습니다");
    }
}
// 사용자가 입력한 글자는 Config 치환 뒤에도 같은 글자가 되도록 보호한다.
std::string EscapeLiteral(std::string_view value) {
    ScenarioDocument::ValidateUtf8(value);std::string result;
    for (const char byte:value) { // 백틱 자체와 따옴표·열린 중괄호만 원문 escape로 보호한다.
        if (byte=='\r' || byte=='\n') throw std::invalid_argument("시나리오 필드에는 줄바꿈을 넣을 수 없습니다");
        if (byte=='`' || byte=='"' || byte=='{') result+='`';result+=byte;
    }
    return result;
}
// 끝의 백틱은 닫는 따옴표 밖에 둔다. GetRaw의 직전 글자 규칙에도 원시값이 동일하다.
std::string QuoteEscaped(std::string_view value) {
    auto end=value.size();while (end && value[end-1]=='`') --end; // 마지막 quote 앞에서 escape로 해석될 바이트만 밖으로 옮긴다.
    return '"'+std::string(value.substr(0,end))+'"'+std::string(value.substr(end));
}
// 현재 문서의 첫 LF에서 사용한 줄바꿈만 새 키에 사용한다. 기존 줄바꿈은 바꾸지 않는다.
std::string_view Newline(std::string_view text) {
    const auto lf=text.find('\n');return lf!=std::string_view::npos && lf && text[lf-1]=='\r' ? "\r\n" : "\n";
}
// 원문 Header 끝 또는 첫 섹션 앞에 없는 키를 추가한다. 기존 본문 순서는 바꾸지 않는다.
std::size_t InsertionPoint(std::string_view text) {
    std::size_t line=0;std::optional<std::size_t> headerEnd;
    for (;;) { // LF 단위로만 이동하여 CR 단독을 새 줄로 간주하지 않는다.
        auto start=line;while (start<text.size() && Blank(text[start])) ++start; // 섹션 앞 들여쓰기만 건너뛴다.
        auto end=text.find('\n',start);if (end==std::string_view::npos) end=text.size();
        auto section=text.substr(start,end-start);while (!section.empty() && (section.back()=='\r' || Blank(section.back()))) section.remove_suffix(1); // 섹션 판정에 쓰는 오른쪽 공백만 제외한다.
        if (!section.empty() && section.front()=='[') {
            if (headerEnd) return line;
            if (o::AsciiLower(section)=="[header]") headerEnd=end<text.size() ? end+1 : end;
            else return line;
        }
        if (end==text.size()) return text.size();line=end+1;
    }
}
// 브리핑 제목에 넣는 사용자 맵 이름의 HTML 제어문자만 보호한다.
std::string EscapeHtml(std::string_view value) {
    std::string result;
    for (const char byte:value) { // UTF-8 비ASCII 바이트는 그대로 보존한다.
        if (byte=='&') result+="&amp;";else if (byte=='<') result+="&lt;";else if (byte=='>') result+="&gt;";else if (byte=='"') result+="&quot;";else result+=byte;
    }
    return result;
}
}

// 입력 바이트와 디코딩 본문의 최초 사본을 함께 소유한다.
ScenarioDocument::ScenarioDocument(std::vector<std::uint8_t> bytes,std::string text,ScenarioDocumentEncoding encoding)
    :originalBytes_(std::move(bytes)),originalText_(text),text_(std::move(text)),encoding_(encoding) {}
// BOM 없는 원본은 CP1252다. UTF-8을 추측해 원문 바이트의 의미를 바꾸지 않는다.
ScenarioDocument ScenarioDocument::FromBytes(std::span<const std::uint8_t> bytes) {
    std::vector<std::uint8_t> plain(bytes.begin(),bytes.end());const bool encoded=plain.size()>=3 && plain[0]==0 && plain[2]==0;
    if (encoded) {
        o::ApplyXor(plain);
        if (plain.size()<o::kConfigSignature.size() || !std::equal(o::kConfigSignature.begin(),o::kConfigSignature.end(),plain.begin()))
            throw std::invalid_argument("시나리오 암호 Config 서명이 손상되었습니다");
    }
    if (std::find(plain.begin(),plain.end(),std::uint8_t{})!=plain.end()) throw std::invalid_argument("시나리오 문서에 NUL 바이트가 있습니다");
    const bool signature=plain.size()>=o::kConfigSignature.size() && std::equal(o::kConfigSignature.begin(),o::kConfigSignature.end(),plain.begin());
    const auto skip=signature ? o::kConfigSignature.size() : std::size_t{};
    const auto body=std::span<const std::uint8_t>(plain).subspan(skip);
    const bool bom=body.size()>=3 && body[0]==0xef && body[1]==0xbb && body[2]==0xbf;
    auto text=std::string(signature ? o::kConfigSignature : std::string_view{})+o::DecodeOriginalText(body);ValidateUtf8(text);
    const auto encoding=encoded ? (bom ? ScenarioDocumentEncoding::EncodedUtf8Bom : ScenarioDocumentEncoding::EncodedWindows1252)
        : (bom ? ScenarioDocumentEncoding::Utf8Bom : ScenarioDocumentEncoding::Windows1252);
    return ScenarioDocument({bytes.begin(),bytes.end()},std::move(text),encoding);
}
// 새로 생성하는 문서는 BOM이 있는 UTF-8 바이트를 최초 identity로 삼는다.
ScenarioDocument ScenarioDocument::FromUtf8(std::string text) {
    ValidateUtf8(text);std::vector<std::uint8_t> bytes(kBom.begin(),kBom.end());bytes.insert(bytes.end(),text.begin(),text.end());
    return ScenarioDocument(std::move(bytes),std::move(text),ScenarioDocumentEncoding::Utf8Bom);
}
// 원문 조회 함수를 공유하므로 Header 밖/중복 키의 우선순위도 같다.
std::optional<std::string> ScenarioDocument::Raw(std::string_view key) const { return o::ConfigGetRaw(text_,key); }
// 값의 실제 시작부터 주석/줄끝 앞까지 바꾸고 키 철자·공백·주석·다음 줄은 보존한다.
bool ScenarioDocument::SetLiteral(std::string_view key,std::string_view value) {
    CheckKey(key);const auto escaped=EscapeLiteral(value);const auto found=o::ConfigFindKey(text_,key);
    if (found) {
        auto first=text_.find('=',*found);while (first<text_.size() && (text_[first]=='=' || Blank(text_[first]))) ++first; // GetRaw처럼 연속 '='와 값 앞 공백을 보존 영역으로 둔다.
        auto end=first;bool quoted=false;char previous{};
        for (;end<text_.size();++end) { // ConfigGetRaw와 같은 quote/직전 백틱/주석 판정으로 범위를 찾는다.
            const char byte=text_[end];if (byte=='\r' || byte=='\n' || (!quoted && byte=='/' && end+1<text_.size() && text_[end+1]=='/')) break;
            if (byte=='"' && previous!='`') quoted=!quoted;previous=byte;
        }
        if (quoted) throw std::invalid_argument("수정할 시나리오 필드의 따옴표가 닫히지 않았습니다");
        while (end>first && Blank(text_[end-1])) --end; // 주석 앞 원래 공백은 치환 범위에서 제외한다.
        auto old=Raw(key).value_or("");while (!old.empty() && Blank(old.back())) old.pop_back(); // Config 치환이 제거하는 뒤 공백만 동일값 판정에서 제외한다.
        if (old==escaped) return false;
        text_.replace(first,end-first,QuoteEscaped(escaped));return true;
    }
    const auto insertion=InsertionPoint(text_);const auto newline=Newline(text_);std::string addition;
    if (insertion && text_[insertion-1]!='\n') addition+=newline;
    addition+=std::string(key)+'='+QuoteEscaped(escaped)+std::string(newline);text_.insert(insertion,addition);return true;
}
// 문서 참조만 빌려 주며 현재 게임의 설정에는 등록하지 않는다.
const std::string& ScenarioDocument::Text() const { return text_; }
// 원문 ConfigFindSection의 첫 일치 본문 범위를 사용한다.
std::optional<std::string> ScenarioDocument::Section(std::string_view name) const {
    o::ConfigRegistry registry;o::Config config(registry,"scenario");config.Assign(text_);if (const auto section=config.Section(name)) return std::string(*section);return std::nullopt;
}
// 첫 A. 본문 편집은 명시 동작이다. 다른 섹션을 넣는 줄은 이 API에서 거부한다.
bool ScenarioDocument::SetSection(std::string_view name,std::string_view body) {
    ValidateUtf8(body);if (name.empty() || name.find_first_of("[]\r\n")!=name.npos) throw std::invalid_argument("시나리오 섹션 이름 범위");
    for (std::size_t at=0;at<body.size();++at) if (body[at]=='[' && (!at || body[at-1]=='\n' || body[at-1]=='\r')) throw std::invalid_argument("브리핑 본문에 새 섹션 행을 넣을 수 없습니다"); // 사용자에게 전체 대본 편집이라고 가장하지 않는다.
    const auto header="["+std::string(name)+"]";const auto start=o::ConfigFindSection(text_,0,header);const auto newline=Newline(text_);std::string replacement(body);
    if (!replacement.empty() && replacement.back()!='\n') replacement+=newline;
    if (start) { auto end=*start;while (end<text_.size() && !(text_[end]=='[' && end && (text_[end-1]=='\n' || text_[end-1]=='\r'))) ++end;const auto old=text_.substr(*start,end-*start);if (old==replacement) return false;text_.replace(*start,end-*start,replacement);return true; } // 기존 첫 섹션의 바깥 문자는 이동하지 않는다.
    const auto lower=o::AsciiLower(text_);auto insertion=text_.size(),at=lower.find("[end]");
    while (at!=text_.npos) { if (!at || text_[at-1]=='\n' || text_[at-1]=='\r') { insertion=at;break; }at=lower.find("[end]",at+1); } // 빈 마지막 END도 본문 존재 검사 없이 찾아 그 앞에 새 섹션을 둔다.
    const auto prefix=insertion && text_[insertion-1]!='\n' ? std::string(newline) : std::string{};text_.insert(insertion,prefix+header+std::string(newline)+replacement);return true;
}
// 변경된 본문만 UTF-8 BOM 평문으로 만들고 미변경 암호/줄바꿈/바이트는 그대로 반환한다.
std::vector<std::uint8_t> ScenarioDocument::Serialize() const {
    if (!Changed()) return originalBytes_;std::vector<std::uint8_t> bytes(kBom.begin(),kBom.end());bytes.insert(bytes.end(),text_.begin(),text_.end());return bytes;
}
// 저장 성공 여부가 아닌 최초 문서와의 차이를 반환한다.
bool ScenarioDocument::Changed() const { return text_!=originalText_; }
// 입력 형식 표시용이며 미변경 문서를 다시 인코딩하지 않는다.
ScenarioDocumentEncoding ScenarioDocument::Encoding() const { return encoding_; }
// 기존 DecodeOriginalText가 검사하지 않는 BOM UTF-8 손상을 편집층에서 명시한다.
void ScenarioDocument::ValidateUtf8(std::string_view text) {
    for (std::size_t position=0;position<text.size();) { // Unicode 스칼라 하나씩 읽으며 최단 표현을 확인한다.
        const auto first=static_cast<unsigned char>(text[position++]);if (!first) throw std::invalid_argument("시나리오 텍스트에 NUL이 있습니다");if (first<0x80) continue;
        const int count=first>=0xc2 && first<=0xdf ? 1 : first>=0xe0 && first<=0xef ? 2 : first>=0xf0 && first<=0xf4 ? 3 : -1;
        if (count<0 || position+static_cast<std::size_t>(count)>text.size()) throw std::invalid_argument("시나리오 UTF-8이 손상되었습니다");
        std::uint32_t point=first&static_cast<unsigned char>(count==1 ? 0x1f : count==2 ? 0x0f : 7);
        for (int index=0;index<count;++index) { // 후속 바이트는 10xxxxxx여야 한다.
            const auto next=static_cast<unsigned char>(text[position++]);if ((next&0xc0)!=0x80) throw std::invalid_argument("시나리오 UTF-8 후속 바이트가 손상되었습니다");point=(point<<6)|(next&0x3f);
        }
        if ((count==1 && point<0x80) || (count==2 && point<0x800) || (count==3 && point<0x10000) || (point>=0xd800 && point<=0xdfff) || point>0x10ffff)
            throw std::invalid_argument("시나리오 UTF-8 코드포인트가 손상되었습니다");
    }
}
// 실제 Tutorial/Go 버튼 문법으로 새 맵의 설명만 생성한다.
ScenarioDocument CreateDefaultEditorScenario(std::string_view mapName) {
    ScenarioDocument::ValidateUtf8(mapName);const auto title=EscapeLiteral(mapName);
    return ScenarioDocument::FromUtf8("[Header]\r\nmissionType=\"Tutorial\"\r\nmissionNumber=1\r\ntutorialNumber=0\r\ntitle="+QuoteEscaped(title)+
        "\r\nmyStartMoney=5000\r\nmyTech=\"all\"\r\nmoreGeysers=0\r\n\r\n[Options]\r\n\r\n[A.]\r\n<h2>"+EscapeLiteral(EscapeHtml(mapName))+"</h2>\r\n$Button=Go!,DoNothing,0\r\n\r\n[END]\r\n");
}
}
