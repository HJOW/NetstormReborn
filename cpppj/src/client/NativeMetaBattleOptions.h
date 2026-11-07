// Ch.cpp의67h 전투 옵션 수신을 같은 현재CS/자기참가자·실제음향/NSInfo에 연결한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // 원문의sender/phase/known추가관문없이실제선택필드만읽는다.
struct NativeMetaBattleOptionsHooks {
    std::function<std::uint32_t()> owner; //0040da40의같은현재540c70이며길이관문뒤에만읽는다.
    std::function<void(std::string_view)> log; //원문수준9의진단문자열이며번역을추가하지않는다.
    std::function<void()> guySound,refreshInfo; //실제guy.wav(0,0,0,0,0)/0047cab0의현재NSInfo Push/Attach다.
};
void ReceiveNativeMetaBattleOptions(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleOptionsHooks& hooks); //0042c920 전체:active/160포함상한·자기master관문·음향후현재길이복사/현재BYTE길이저장·조건부NSInfo다.
}
