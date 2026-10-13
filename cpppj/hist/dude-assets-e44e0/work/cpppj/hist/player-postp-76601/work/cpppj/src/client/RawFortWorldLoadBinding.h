// 기존 정상지형/저장객체/SP/Meta/Graph를 한 실제 월드 부모의 필수 경계에 연결한다.
#pragma once
#include "client/RawNormalTerrainWorldBinding.h"
#include "o/RawFortWorldLoad.h"

namespace netstorm::client {
// moneyStream/selectMeta/metaStream은매호출의현재Template와같은전역을기존소비자에선택해야한다.
// 군도/Island0 Populate/TypeNames 전신/부모getter/UI는미복원범위를명시한기존필수hooks로남는다.
// RawGraph의현재전체풀제한은보존하며Form이있는미지원월드를성공으로통과시키지않는다.
o::FortWorldLoadHooks MakeFortWorldLoadBinding(RawSurfaceWorld& world,RawNormalTerrainWorldBinding& normal,
    const o::RawFortObjectLoad& objects,const o::RawFortMoneyLoad& money,const o::RawFortMetaLoad& meta,
    std::function<o::FortMoneyCursor&(const o::FortTemplate&,std::uint32_t)> moneyStream,
    std::function<void(const o::FortTemplate&,std::uint32_t,bool)> selectMeta,
    std::function<o::FortMetaStream&(const o::FortTemplate&,bool)> metaStream,o::FortWorldLoadHooks hooks);
}
