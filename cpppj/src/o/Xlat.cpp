// 원본: 004de260(A) ↔ CD 004bc7a0, 004de1e0(A) ↔ CD 004bc740.
// 004de9a0의 언어 0·1 우회와 미등록 원문 폴백도 함께 옮겼다.
#include "o/Xlat.h"
#include <algorithm>

namespace netstorm::o {
// 파일 전체 CR 제거 후 줄 첫 글자만 검사한다. 미완성 마지막 블록은 저장하지 않는다.
XlatTable::XlatTable(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    int state = 0;
    std::size_t originalStart = 0;
    std::size_t translatedStart = 0;
    std::string original;
    // 줄 시작에서만 구분 문자를 판단한다. 마지막 LF가 없어도 완성 구분 줄은 처리한다.
    for (std::size_t pos = 0; pos < text.size();) {
        const auto newline = text.find('\n', pos);
        const auto next = newline == std::string::npos ? text.size() : newline + 1;
        if (state == 0 && text[pos] == '*') { originalStart = next; state = 1; }
        else if (state == 1 && text[pos] == '-') {
            auto end = pos;
            if (end > originalStart && text[end-1] == '\n') --end;
            original = text.substr(originalStart, end - originalStart);
            translatedStart = next;
            state = 2;
        } else if (state == 2 && text[pos] == '=') {
            auto end = pos;
            if (end > translatedStart && text[end-1] == '\n') --end;
            translations_[original] = text.substr(translatedStart, end - translatedStart);
            state = 0;
        }
        pos = next;
    }
}
// 번역 키는 원본처럼 대소문자를 구분한다.
std::string XlatTable::Translate(std::string_view original, int languageNumber) const {
    if (languageNumber == 0 || languageNumber == 1) return std::string(original);
    const auto found = translations_.find(std::string(original));
    return found == translations_.end() ? std::string(original) : found->second;
}
// 원본 중복 블록은 같은 키를 덮어쓰므로 최종 키 수만 센다.
std::size_t XlatTable::Size() const { return translations_.size(); }
}
