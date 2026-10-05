// 원본 조건은 산술식이 아닌 정수 접두어·문자열/정수 비교다. 모든 치환은 호출자가 먼저 한다.
#include "client/DialogScript.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <cctype>

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
// 명령 줄을 본문에서 빼고 버튼·메뉴·시간 제한을 보존한다.
DialogPage ParseDialogPage(std::string_view source) {
    const auto filtered = FilterDialogConditions(source);
    DialogPage page; std::string body;
    // 줄 가운데 $명령도 읽는다. 다른 $명령 또는 줄바꿈에서 인자가 끝난다.
    for (std::size_t i = 0; i < filtered.size();) {
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
    page.paragraphs = Paragraphs(body, page.background);
    return page;
}
}
