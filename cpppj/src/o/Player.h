// Player.cpp/Totalmade.cpp의 미션 시작 상태 중 자원·기술·동맹·색만 연결한다.
#pragma once
#include "o/Template.h"
#include <functional>

namespace netstorm::o {
// 원본 소유자 번호는 1~8이며 0은 중립이다.
inline constexpr int kPlayerCount = 8;
// 플레이어 번호·색·SP·지식·기술·동맹의 상태이다.
struct Player {
    // number: 번호.
    // color: 그림·글자 출력에 사용할 팔레트 색 인덱스 또는 색 상태.
    int number{}, color{};
    // active: 현재 입력·실행 대상이 활성 상태인지.
    // computer: 컴퓨터 여부.
    bool active{}, computer{};
    // stormPower: 스톰 파워.
    double stormPower{};
    // name: 이름으로 조회·표시할 때 사용하는 문자열.
    // startingTech: 시작 기술.
    // ability: 능력.
    std::string name, startingTech, ability;
    // allies: 동맹.
    std::uint16_t allies{};
    // knowledge: 지식.
    std::vector<int> knowledge;
    // storedTechnology: 내용물 목록 전체. complete가 거짓이면 변환한 타입이 0인 항목에서 읽기가 끊긴 것이다(원본 반환값 0) (FortContentList).
    FortContentList storedTechnology;
    // storedDeck: 저장된 덱.
    std::vector<FortDeckEntry> storedDeck;
};
// 미션 플레이어 목록과 기술·회수·승급·AI 허용 상태이다.
struct MissionPlayers {
    // players: 플레이어.
    std::array<Player,kPlayerCount+1> players{};
    // techAllowed: 기술 허용된.
    std::vector<bool> techAllowed;
    // denySalvage: 금지 회수 여부.
    // denyAscend: 금지 승급 여부.
    // aiOff: AI 끄기 여부.
    bool denySalvage{}, denyAscend{}, aiOff{};
    // 사람/AI 머리 값과 요새 저장 상태를 구분하여 초기화한다.
    static MissionPlayers Load(const std::function<std::optional<std::string>(std::string_view)>& get,
        const FortTemplate& fort, const RiftTypeTable& types, double fallbackMoney);
    // 목록 한쪽에만 지정된 동맹도 양방향으로 조회한다.
    bool Allied(int first,int second) const;
};
}
