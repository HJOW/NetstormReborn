// 원본004904b0의Editor이름/Player필드/직접기술생성순서를복원한다.
#pragma once
#include "o/RiftType.h"
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 이름/현재저장소/실제생성만명시경계로연결하며본문이필드쓰기·타입필터·순서를소유한다.
struct EditorPlayersHooks {
    std::function<std::span<const std::uint8_t>(std::string_view)> translate; //004de9a0의Human Player/Player/Computer #%d NUL포함원문이다.
    std::function<std::span<std::uint8_t>()> players; //0059534c의현재9×180BYTE Player표다.본문이요청하는시점에같은현재주소를빌린다.
    std::function<std::span<std::uint8_t>()> handle; //00540ca8의현재Client NUL쓰기버퍼다.Player번역뒤조회한다.
    std::function<Sid(std::uint32_t)> technology; //00490250(owner)의실제새Player기술부모SID다.Find/Ensure/CurrentTechnology로대체하지않는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t,std::span<const std::uint8_t>,std::int32_t)> format; //004b9390(capacity30,번역원문,owner-1)의NUL포함결과다.
    std::function<std::int32_t()> typeCount; //00541348의현재signed개수다.각owner시작/매반복끝에다시읽는다.
    std::function<const RiftTypeRecord&(std::int32_t)> typeAt; //0049a840의현재기록이며반환뒤group/theme를직접읽는다.타입0도포함한다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> createContained; //004afa70(parent,type,0,4,0x10,0)의직접생성이다.반환은무시하며현재Pool주소결합은장치가맡는다.
    std::function<void()> finish; //마지막tail JMP0046e980의실제후처리다.미연결을빈성공으로채우지않는다.
};
//Human Player/현재handle이름→owner2..8 Player/이름/flags/모든eligible타입직접생성→0046e980순서다.config/title/창종료효과는추가하지않는다.
void InitializeEditorPlayers(const EditorPlayersHooks& hooks);
}
