// Player.cpp/Totalmade.cpp의 미션 시작 상태 중 자원·기술·동맹·색만 연결한다.
#pragma once
#include "o/Template.h"
#include <functional>

namespace netstorm::o {
// 원본 소유자 번호는 1~8이며 0은 중립이다.
inline constexpr int kPlayerCount = 8;
struct Player {
    int number{}, color{};
    bool active{}, computer{};
    double stormPower{};
    std::string name, startingTech, ability;
    std::uint16_t allies{};
    std::vector<int> knowledge;
    FortContentList storedTechnology;
    std::vector<FortDeckEntry> storedDeck;
};
struct MissionPlayers {
    std::array<Player,kPlayerCount+1> players{};
    std::vector<bool> techAllowed;
    bool denySalvage{}, denyAscend{}, aiOff{};
    // 사람/AI 머리 값과 요새 저장 상태를 구분하여 초기화한다.
    static MissionPlayers Load(const std::function<std::optional<std::string>(std::string_view)>& get,
        const FortTemplate& fort, const RiftTypeTable& types, double fallbackMoney);
    // 목록 한쪽에만 지정된 동맹도 양방향으로 조회한다.
    bool Allied(int first,int second) const;
};
}
