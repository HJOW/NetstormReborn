// 10.78 세계 간헐천 생성 부모의 장식·난수·예산·분할/전체 요청 순서를 복원한다.
#pragma once
#include "o/RawLocalGeyser.h"
#include "o/RawWorldGeyserBounds.h"

namespace netstorm::o { // 같은 실제 Session의 옵션과 건설 몸체를 조립하는 생성 부모다.
// Local 생성 뒤 현재 battle/옵션/타입을 읽으며 별도 설정 사본을 보유하지 않는다.
struct WorldGeyserState {
    const bool& battle; //00594fbc의 현재 전투 관문이다.
    std::span<const std::uint8_t> options; //0052f7e0의 같은87BYTE이며9/18을 반복 중 재조회한다.
    std::span<const std::uint32_t,4> decorationTypes; //현재treeTwo/RUIN/treeThree/Monument의 네 DWORD다.
};
// 실제 큰 세계 하위만 경계로 두며 선택되지 않은 분기의 기본 성공을 만들지 않는다.
struct WorldGeyserHooks {
    std::function<void(std::string_view)> log; //004a68e0의 원래 생성 로그다.
    std::function<void()> local; //00486360: 같은 원본 로컬 생성 부모를 먼저 실행한다.
    std::function<std::int32_t(std::string_view,std::int32_t)> configInt; //predictableGeysers의 현재 기본1 조회다.
    std::function<double()> wallSeconds; //각 TimeSeed가 요청할 때 현재 벽시각을 읽는다.
    std::function<std::int32_t()> budget,density,spacing; //실제0041c8e0/0041ca40/0041ca10의 현재 getter다.
    std::function<std::uint32_t()> demo; //00463b50: 현재 Guide+68의 비영을0/1로 반환한다.
    std::function<FortRandomRectangle()> whole; //00485bc0의 실제 전체 범위다.
    std::function<FortRandomRectangle(std::int32_t,std::int32_t)> partition; //00485cc0의 실제 분할 범위다.
    std::function<void(GameRandom&,FortRandomRectangle,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> decorate; //0040ded0의 같은 현재 세계 배치다.
    std::function<void(std::int32_t,GameRandom&,FortRandomRectangle,GeyserPlacementOptions)> populate; //00486130의 같은 Pool/Player/자원/최근 목록이다.
};
//0041ca40의 옵션0~3을 실제 밀도-1/0/1/2로 읽고 표 밖은 명시 오류다.
std::int32_t WorldGeyserDensity(std::uint8_t selection);
//전체 생성 부모00486440이며 로컬 생성 하나로 대신하지 않는다.
class RawWorldGeyser {
public:
    // 현재87BYTE/타입4개와 실제 하위의 수명은 이 객체보다 길어야 한다.
    RawWorldGeyser(OriginalEdition edition,WorldGeyserState state,WorldGeyserHooks hooks);
    // 로그→로컬→장식→현재 seed/예산→분할 및 선택된 두 전체 배치를 실행한다.
    void Generate() const;
private:
    WorldGeyserState state_; //실제 현재 전역을 빌리며 생성 시작의 값으로 고정하지 않는다.
    WorldGeyserHooks hooks_; //실제 도달한 원문 하위만 호출하는 같은 Session 장치다.
};
}
