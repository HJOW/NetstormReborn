// 실제Tell은전체치환뒤1차HTML의현재조건/명령효과와2차HTML입력을연결한다. 기존원문호환검사는별도다.
#include "client/DialogScript.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <cctype>
#include <array>
#include <regex>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본 인자의 앞뒤 공백과 바깥 따옴표를 정리한다.
std::string Trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n"); if (first == std::string_view::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n"); text = text.substr(first, last - first + 1);
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') text = text.substr(1, text.size() - 2);
    return std::string(text);
}
// 따옴표 안의 쉼표는 인자 구분자로 처리하지 않는다.
std::vector<std::string> Fields(std::string_view text) {
    std::vector<std::string> result; std::size_t start = 0; bool quoted = false;
    // 각 쉼표 경계에서 현재 인자를 추가한다.
    for (std::size_t i = 0; i <= text.size(); ++i) {
        if (i < text.size() && text[i] == '"') quoted = !quoted;
        if (i == text.size() || (text[i] == ',' && !quoted)) { result.push_back(Trim(text.substr(start, i - start))); start = i + 1; }
    }
    return result;
}
// 비교의 양쪽에서 원본처럼 앞 공백과 따옴표를 제거한다.
bool Condition(std::string expression) {
    bool negate = false;
    if (!expression.empty() && expression[0] == '!') { negate = true; expression.erase(0, 1); }
    bool value = false;
    char compare = 0; bool equal = false;
    if (!expression.empty() && expression[0] == '?') { compare = '?'; expression.erase(0,1); }
    if (!expression.empty() && (expression[0] == 'g' || expression[0] == 'G')) {
        compare = 'g'; expression.erase(0,1);
        if (!expression.empty() && (expression[0] == 'e' || expression[0] == 'E')) { equal = true; expression.erase(0,1); }
    }
    if (!expression.empty() && (expression[0] == 'l' || expression[0] == 'L')) {
        compare = 'l'; equal = false; expression.erase(0,1);
        if (!expression.empty() && (expression[0] == 'e' || expression[0] == 'E')) { equal = true; expression.erase(0,1); }
    }
    if (!expression.empty() && expression[0] == '!') { negate = true; expression.erase(0,1); }
    if (compare != 0) {
        bool quote = false; std::size_t split = std::string::npos;
        // 따옴표 밖 첫 등호만 비교 구분자로 사용한다.
        for (std::size_t i = 0; i < expression.size(); ++i) {
            if (expression[i] == '"') quote = !quote;
            if (expression[i] == '=' && !quote) { split = i; break; }
        }
        const auto left = Trim(expression.substr(0, split));
        const auto right = split == std::string::npos ? std::string() : Trim(expression.substr(split + 1));
        if (compare == '?') value = left == right;
        else {
            const auto a = o::ConfigParseLong(left), b = o::ConfigParseLong(right);
            value = compare == 'g' ? (equal ? a >= b : a > b) : (equal ? a <= b : a < b);
        }
    } else value = o::ConfigParseLong(Trim(expression)) != 0;
    return negate ? !value : value;
}
// HTML 줄바꿈·스타일을 문단 단위로 보존한다. 원본 StyleText의 본문 슬롯은 출력 단계에서 적용한다.
std::vector<DialogParagraph> Paragraphs(std::string_view source, std::string& background) {
    std::vector<DialogParagraph> result; DialogParagraph current;
    // 공백을 정리해 현재 문단을 끝낸다.
    const auto flush = [&]() { current.text = Trim(current.text); if (!current.text.empty()) result.push_back(current); current.text.clear(); };
    // HTML 태그와 일반 문자를 순서대로 읽는다.
    for (std::size_t i = 0; i < source.size();) {
        if (source[i] == '<') {
            const auto end = source.find('>', i); if (end == std::string_view::npos) break;
            const auto tag = o::AsciiLower(source.substr(i + 1, end - i - 1));
            if (tag == "p" || tag == "br" || tag == "br/" || tag == "hr") flush();
            else if (tag.size() == 2 && tag[0] == 'h' && tag[1] >= '1' && tag[1] <= '4') { flush(); current.heading = tag[1] - '0'; }
            else if (tag.size() == 3 && tag[0] == '/' && tag[1] == 'h') { flush(); current.heading = 0; }
            else if (tag == "i" || tag == "/i") { flush(); current.italic = tag == "i"; }
            else if (tag == "b" || tag == "/b") { flush(); current.bold = tag == "b"; }
            else if (tag.starts_with("background=")) background = Trim(source.substr(i + 12, end - i - 12));
            i = end + 1; continue;
        }
        if (source[i] == '~' && i + 1 < source.size()) {
            // ~문자 색/슬롯 제어는 화면 문맥의 기본 색으로 표시한다. 이미지/링크 등 전체 StyleText는 후속이다.
            if (source[i + 1] == '~') current.text.push_back('~');
            i += 2; continue;
        }
        if (std::isspace(static_cast<unsigned char>(source[i]))) {
            if (!current.text.empty() && current.text.back() != ' ') current.text.push_back(' ');
        } else current.text.push_back(source[i]);
        ++i;
    }
    flush(); return result;
}
}
// 원본의 한 개 표시 상태를 유지한다. 일반적인 중첩 스택으로 바꾸지 않는다.
std::string FilterDialogConditions(std::string_view source) {
    std::string result; bool shown = true;
    // 여는/닫는 조건은 출력에서 제거한다.
    for (std::size_t i = 0; i < source.size();) {
        if (source.substr(i, 4) == "</?>") { shown = true; i += 4; continue; }
        if (source.substr(i, 2) == "<?") {
            const auto end = source.find('>', i + 2); if (end == std::string_view::npos) break;
            if (shown) shown = Condition(std::string(source.substr(i + 2, end - i - 2)));
            i = end + 1; continue;
        }
        if (shown) result.push_back(source[i]); ++i;
    }
    return result;
}
namespace {
// 기존메타추출과검사호환inline추출을분리한다. 실제경로의inline은이미1차HTML에서실행됐다.
DialogPage ExtractDialogPage(std::string_view source,bool inlineCommands) {
    const std::string filtered(source);
    DialogPage page; std::string body;
    // 줄 가운데 $명령도 읽는다. 다른 $명령 또는 줄바꿈에서 인자가 끝난다.
    for (std::size_t i = 0; i < filtered.size();) {
        if (inlineCommands && std::string_view(filtered).substr(i,2)=="<$") {
            bool quoted=false;std::size_t end=i+2;
            // 따옴표 안의 >와 쉼표는 Warning 문자열의 일부다.
            for (;end<filtered.size();++end) {
                if (filtered[end]=='"') quoted=!quoted;
                if (filtered[end]=='>' && !quoted) break;
            }
            if (end<filtered.size()) {
                const auto raw=std::string_view(filtered).substr(i+2,end-i-2); // '$' 뒤 원문은 실제004c9090의 토큰 helper가 처리한다.
                page.commands.emplace_back(raw);const auto arguments=Fields(raw);
                if (!arguments.empty() && o::AsciiLower(arguments[0])=="warning") {
                    page.warnings.push_back(arguments.size()>1 ? arguments[1] : std::string());
                }
                i=end+1;continue;
            }
        }
        if (filtered[i] != '$') { body.push_back(filtered[i++]); continue; }
        const auto equal = filtered.find('=', i + 1), boundary = filtered.find_first_of("\r\n$", i + 1);
        if (equal == std::string::npos || (boundary != std::string::npos && equal > boundary)) { ++i; continue; }
        const auto kind = o::AsciiLower(Trim(std::string_view(filtered).substr(i + 1, equal - i - 1)));
        const auto end = filtered.find_first_of("\r\n$", equal + 1);
        const auto fields = Fields(std::string_view(filtered).substr(equal + 1, end == std::string::npos ? filtered.size() - equal - 1 : end - equal - 1));
        // 없는 선택 인자는 빈 문자열로 읽는다.
        const auto field = [&](std::size_t n) { return n < fields.size() ? fields[n] : std::string(); };
        DialogAction action{field(1), field(2)};
        if (kind == "button" || kind == "buttoninstant" || kind == "menu" || kind == "checked") {
            page.items.push_back({field(0), action, kind == "menu" || kind == "checked", kind == "checked" && o::ConfigParseLong(field(3)) != 0,
                kind != "checked" || o::ConfigParseLong(field(4)) != 0, kind == "buttoninstant"});
        } else if (kind == "timeout") { page.timeoutSeconds = o::ConfigParseLong(field(0)); page.timeout = action; }
        else if (kind == "onexit") page.onExit = action;
        i = end == std::string::npos ? filtered.size() : end;
    }
    page.htmlBody=body;page.paragraphs = Paragraphs(body, page.background);
    return page;
}
// 기존commands/warnings관찰은태그원문을보관하되실제큐hook에는원본처럼NUL까지의전체remainder를준다.
std::string CommandText(std::string_view raw) {
    bool quoted=false;std::size_t end=0;
    // 따옴표안의>는명령보고문자열에남긴다. 실제HTMLcursor를바꾸는동작은하지않는다.
    for (;end<raw.size();++end) {
        if (raw[end]=='"') quoted=!quoted;
        if (!raw[end] || (raw[end]=='>' && !quoted)) break;
    }
    return std::string(raw.substr(0,end));
}
// 원본Tell의C로케일alnum존재관문이다. 영문/숫자가없는줄바꿈·기호만으로새창을만들지않는다.
bool HasContent(std::string_view text) {
    // StyleText제어문자의영문도원본처럼현재출력바이트에포함하여검사한다.
    for (const unsigned char byte:text) if ((byte>='A' && byte<='Z') || (byte>='a' && byte<='z') || (byte>='0' && byte<='9')) return true;
    return false;
}
// 1차HTML이원문LF를공백으로바꾸므로원본메타regex의마지막인자경계와종류우선순위를사용한다.
// Key/Input/Zsend의실제입력컨트롤은아직공급되지않아선택시명시적으로거부한다.
DialogPage ExtractRuntimeMetadata(std::string text,const DialogMetadataHooks& hooks) {
    static const std::array<std::regex,9> patterns{ // 원본004c83b0의같은대소문자/캡처/검색우선순위다.
        std::regex(R"(\$Button[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),
        std::regex(R"(\$ButtonInstant[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),
        std::regex(R"(\$Menu[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),
        std::regex(R"(\$Checked[ \t]*=[ \t]*([^,]+),([^,]+),([^,]+),([^,]+),([^$ ]+))"),
        std::regex(R"(\$Timeout[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),
        std::regex(R"(\$OnExit[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),
        std::regex(R"(\$Key[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),
        std::regex(R"(\$Input[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),
        std::regex(R"(\$Zsend[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))")};
    DialogPage page;
    // 매제거뒤다시Button부터검색한다. 원문등장순으로모든메타를정렬하지않는다.
    for (;;) {
        std::smatch match;std::size_t kind=0;
        // 앞종류가없을때만다음종류의첫일치를찾는다.
        for (;kind<patterns.size();++kind) if (std::regex_search(text,match,patterns[kind])) break;
        if (kind==patterns.size()) break;
        if (kind>=6) throw std::logic_error("Tell 메타의실제Key/Input/Zsend컨트롤미연결");
        const auto rawCommand=match[2].str(),rawArgument=match[3].str(); // 원문OnExit의숫자/고리판정에공백·인용바이트를남긴다.
        const auto label=Trim(match[1].str());DialogAction action{Trim(rawCommand),Trim(rawArgument)};
        if (hooks.resolveAction) hooks.resolveAction(action); // sentinel도 원래 placeholder의 조회/인자 효과를 먼저 실행한다.
        if (kind!=5 && hooks.resolveRuntimeAction) hooks.resolveRuntimeAction(action,rawCommand,rawArgument); // OnExit의 기존 별도 해석과 중복해 고리를 진행하지 않는다.
        if (kind<4) {
            DialogItem item{label,action,kind==2 || kind==3,kind==3 && o::ConfigParseLong(match[4].str())!=0,
                kind!=3 || o::ConfigParseLong(match[5].str())!=0,kind==1};
            item.checkedDirective=kind==3; // $Checked의 거짓 체크값도 $Menu 이름 판별로 보내지 않는다.
            if (item.menu && hooks.appendMenu) hooks.appendMenu(page,std::move(item));else page.items.push_back(std::move(item));
        }
        else if (kind==4) { page.timeoutSeconds=o::ConfigParseLong(label);page.timeout=action; }
        else {
            if (hooks.resolveOnExit) hooks.resolveOnExit(rawCommand,rawArgument); // 기존Trim콜백뒤같은메타시점에현재HTML효과가끝난원문을전달한다.
            page.onExit=action;
        }
        const auto start=static_cast<std::size_t>(match.position());auto end=text.find_first_of("\n$",start+1);
        if (end==std::string::npos) end=text.size();else if (text[end]=='\n') ++end;
        text.erase(start,end-start); // 원본004c8bbd는캡처끝이아니라다음LF(포함)/$(제외)/NUL까지실제로제거한다.
    }
    page.htmlBody=std::move(text);page.paragraphs=Paragraphs(page.htmlBody,page.background);return page;
}
}
// 기존검사의원문호환계약은유지하고실제Tell에서는이overload를선택하지않는다.
DialogPage ParseDialogPage(std::string_view source) { return ExtractDialogPage(FilterDialogConditions(source),true); }
// 1차HTML의효과·표시상태를메타추출보다먼저실행하고같은상태의2차HTML입력을반환한다.
DialogPage ParseDialogPage(std::string_view source,HtmlTextParserState& state,HtmlTextParserGlobals& globals,HtmlTextParserHooks hooks,o::OriginalEdition edition,DialogMetadataHooks metadata) {
    std::vector<std::string> commands,warnings;std::string background;
    const auto command=hooks.command;const auto setBackground=hooks.background;
    hooks.command=[&](HtmlTextParserState& current,std::string_view raw) {
        const auto text=CommandText(raw);commands.push_back(text);const auto arguments=Fields(text);
        if (!arguments.empty() && o::AsciiLower(arguments[0])=="warning") warnings.push_back(arguments.size()>1 ? arguments[1] : std::string());
        if (!command) throw std::logic_error("Tell 1차HTML의실제명령큐효과미연결");command(current,raw);
    }; // 선택된명령은메타처리를기다리지않고현재실제큐에예약한다.
    hooks.background=[&](HtmlTextParserState& current,std::string_view name) {
        background=name;if (setBackground) setBackground(current,name);
    }; // 실제배경이름은현재페이지의소유필드에도보관한다.
    const auto first=HtmlTextParser(state,globals,std::move(hooks),edition).Parse(source);
    auto page=ExtractRuntimeMetadata(first,metadata);page.commands=std::move(commands);page.warnings=std::move(warnings);page.background=std::move(background);page.hasContent=HasContent(first);
    std::string second;second.reserve(page.htmlBody.size());
    // 원본00504b0c→00515eb8과같이1차출력의모든LF를실제BR태그로되돌린다.
    for (const char byte:page.htmlBody) { if (byte=='\n') second+="<br>";else second+=byte; }
    page.htmlBody=std::move(second);return page;
}
}
