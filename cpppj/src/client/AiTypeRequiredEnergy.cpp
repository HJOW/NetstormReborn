// 자산70..185만 실제 Definition에 연결하고 내부/186·187 빈 TYPE는 원본 zero 초기 mana에서 후처리한다.
#include "client/AiTypeRequiredEnergy.h"
#include "o/RawTypeRequiredEnergy.h"
#include <stdexcept>

namespace netstorm::client {
// 같은 현재 TYPE/실제 보호 AST이며 기존 이름처럼 requiredEnergy를 새로 복제하지 않는다.
std::function<std::string(std::uint32_t)> BindAiTypeRequiredEnergy(const GameAssets& assets,std::span<const o::RiftTypeRecord> currentTypes) {
    if (assets.Edition()!=o::OriginalEdition::Patch1078 || currentTypes.size()!=assets.TypeTable().Types().size() || currentTypes.size()!=188 || assets.Types().size()!=116)
        throw std::invalid_argument("AI 에너지 실제 PATCH188TYPE/116자산 계약 오류");
    // 같은 현재 TYPE와 원본 AST만 참조하며 저장소나 임의 기본 문자열을 만들지 않는다.
    return [&assets,currentTypes](std::uint32_t number) {
        if (number>=currentTypes.size()) throw std::out_of_range("AI 에너지 현재 TYPE 범위 오류");
        const o::RiftTypeDefinition* definition=nullptr; //내부/빈 타입은 실제 zero 초기 입력이다.
        if (number>=70 && number<70+assets.Types().size()) definition=&assets.Types()[number-70].definition;
        return o::TypeRequiredEnergy(assets.Edition(),currentTypes[number],definition);
    };
}
}
