// 원본 TYPE+a0의 mana 속성/후처리를 기존 TYPE와 Definition에서 조회한다.
#include "o/RawTypeRequiredEnergy.h"
#include "o/OriginalText.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::string_view kEnergyLetters="wrtsWRTS.?>"; //00540ad8→0050c340 원문이며 바로 뒤 NUL도 표의 일부다.
// 실제 원소 문자표를 읽으며 표 밖의 원본 메모리 접근은 명시 경계로 거부한다.
char Letter(std::int32_t theme) {
    if (theme<0 || static_cast<std::size_t>(theme)>kEnergyLetters.size())
        throw std::out_of_range("TYPE 요구 에너지 실제 원소 문자표 범위 오류");
    return kEnergyLetters.data()[theme];
}
}
// 같은 현재 TYPE와 실제 AST의 mana만 사용한다. requiredEnergy라는 가상 키를 조회하지 않는다.
std::string TypeRequiredEnergy(OriginalEdition edition,const RiftTypeRecord& type,const RiftTypeDefinition* definition) {
    if (edition!=OriginalEdition::Patch1078)
        throw std::invalid_argument("TYPE 요구 에너지는 PATCH 실제 후처리 전용입니다");
    std::string value; //원본 TYPE+a0의 zero 초기 입력과 mana 대입 결과다.
    if (definition) {
        if (const auto* property=definition->Property("mana")) {
            const auto* text=std::get_if<std::string>(&property->value); //BT_STRING_CONSTANT에 해당한다.
            if (!text) throw std::invalid_argument("원본 TYPE mana는 문자열 AST여야 합니다");
            const auto encoded=EncodeOriginalText(*text); //원본 strcpy에 전달할 Windows-1252 byte다.
            value.assign(encoded.begin(),encoded.end());
            value.resize(value.find('\0')==std::string::npos ? value.size() : value.find('\0'));
            if (value.size()>9)
                throw std::out_of_range("원본 TYPE mana는 9byte 이하이며 그 밖은 assert 후 손상 경계입니다");
        }
    }
    if (type.group!=10 && !(type.flags2&0x200000U) && value.empty()) {
        if (type.level>=0) value+=Letter(type.theme);
        if (type.level==1) value+=kEnergyLetters[3];
        if (type.level==2) { value+=Letter(type.theme);value+=kEnergyLetters[3]; }
    }
    value.resize(value.find('\0')==std::string::npos ? value.size() : value.find('\0'));
    // 현재 실제 원본 mana는 ASCII 원소 문자열이다. 임의 비ASCII CRT locale 동일성을 합성하지 않는다.
    for (const auto byte:value)
        if (static_cast<unsigned char>(byte)>=128)
            throw std::invalid_argument("TYPE 비ASCII mana는 원본 CRT tolower 경계 미복원");
    return AsciiLower(value);
}
}
