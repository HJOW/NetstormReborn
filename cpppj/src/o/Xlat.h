// 원본 Xlat.cpp의 번역 블록 읽기·동일 원문 덮어쓰기·영어 폴백.
#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace netstorm::o {
class XlatTable {
public:
    // UTF-8 텍스트에서 첫 글자가 '*', '-', '='인 구분 줄을 파싱한다.
    explicit XlatTable(std::string text);
    // 원본 언어 번호 0·1은 원문, 나머지는 번역 실패 시 원문을 돌려준다.
    std::string Translate(std::string_view original, int languageNumber) const;
    // 중복 원문을 제외한 실제 번역 키 개수.
    std::size_t Size() const;
private:
    std::unordered_map<std::string, std::string> translations_;
};
}
