// PATCH00414710의실제이름/기술/빌더/원소분류/최대비용을같은AI소유메모리에적용한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawPlayerTechnology.h"

namespace netstorm::o {
// 원본의현재mission표식/분류제외타입/워크샾DWORD를참조한다.
struct AiBrainInitialUpdateGlobals {
    const std::uint32_t& missionMode; //00594fa4의현재비제로표식이다.
    const std::uint32_t& excludedType; //0054126c의현재분류제외타입133이다.
    std::span<const std::uint32_t> workshops; //theme0/1/2/3의현재b0/b4/b8/ac워크샾타입이다.
};
// 실제언어설정과TYPE+a0metadata조회경계다.
struct AiBrainInitialUpdateHooks {
    std::function<std::string()> defaultName; //004410f0("defaultAiName",0,0,0)의실제언어조회다.
    std::function<std::string(std::uint32_t)> requiredEnergy; //TYPE+a0의실제현재문자열이며최대길이분기에서는다시조회한다.
};
// 실제큰초기update를빈성공으로대체하지않고원본원문/목록과실제기술사슬을재사용한다.
class RawAiBrainInitialUpdate {
public:
    // 같은메모리/Player9/실제기술/contained와현재TYPE의수명은객체보다길어야한다.
    RawAiBrainInitialUpdate(RawAiBrainMemory& memory,RawPlayerWorld& player,RawPlayerTechnology& technology,
        const ContainedFinderState& finder,std::span<const RiftTypeRecord> types,
        AiBrainInitialUpdateGlobals globals,AiBrainInitialUpdateHooks hooks);
    //600byte재복사→선택이름→실제기술부모ensure/목록→실제분류를실행한다. 지갑2d8/SP는바꾸지않는다.
    void Update(std::uint32_t token,std::span<const std::uint8_t> settings) const;
private:
    // 현재TYPE를그대로읽으며host표밖의원본메모리손상만거부한다.
    const RiftTypeRecord& Type(std::uint32_t number) const;
    // 실제workshop스위치004709d0의0..3반환/4..5의0반환이다.
    std::uint32_t Workshop(std::int32_t theme) const;
    RawAiBrainMemory& memory_; //생성자와같은0xb08/목록소유자다.
    RawPlayerWorld& player_; //현재실제9Player와기술부모생성몸체다.
    RawPlayerTechnology& technology_; //실제문자열→40개목록→ensure/contained생성사슬이다.
    const ContainedFinderState& finder_; //현재타입6/권한/죽음진단전역이다.
    std::span<const RiftTypeRecord> types_; //현재비용/flags2/theme의같은TYPE표다.
    AiBrainInitialUpdateGlobals globals_; //사본없는현재mission/제외타입/워크샾참조다.
    AiBrainInitialUpdateHooks hooks_; //실제이름/요구에너지문자열조회다.
};
}
