// 원본TYPE+a0의mana속성/실제후처리를기존TYPE와Definition에서조회한다.
#pragma once
#include "o/RiftType.h"

namespace netstorm::o {
// PATCH0049c3b0의mana문자열과0049b0d0의기본보충/소문자화를복원한다.
// definition=nullptr은실제내부/빈TYPE의zero초기화된mana입력을뜻하며포인터NULL반환이아니다.
std::string TypeRequiredEnergy(OriginalEdition edition,const RiftTypeRecord& type,const RiftTypeDefinition* definition);
}
