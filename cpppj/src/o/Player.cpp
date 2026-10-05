#include "o/Player.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <cstdlib>

namespace netstorm::o {
namespace {
// 00542660이 가리키는 원본 색 이름의 번호 순서.
constexpr std::array<std::string_view,9> kColorNames{"none","blue","red","white","green","purple","yellow","lightblue","orange"};
// 숫자 머리 값은 원본 atof 계열처럼 앞의 숫자를 읽는다.
double Number(const std::optional<std::string>& value,double fallback) { return value ? std::strtod(value->c_str(),nullptr) : fallback; }
// 세미콜론 목록의 타입 이름을 현재 타입 번호로 바꾼다. all은 생산 그룹의 자산 타입이다.
std::vector<int> Knowledge(std::string_view text,const RiftTypeTable& types) {
    std::vector<int> result;
    // 원본 목록 순서를 보존하며 중복을 제거한다.
    for (int i=0;;++i) {
        const auto token=ConfigListItem(text,i); if (token.empty()) break;
        if (AsciiLower(token)=="all") {
            // 004914e0은 NO_GROUP을 제외한 생산 그룹의 타입만 전체 지식에 넣는다.
            for (std::size_t t=kFirstAssetTypeNumber;t<types.Types().size();++t) if (types.Types()[t].fromAsset && types.Types()[t].group<9) result.push_back(static_cast<int>(t));
        } else { const int t=types.Find(token); if (t>0) result.push_back(t); }
    }
    std::sort(result.begin(),result.end()); result.erase(std::unique(result.begin(),result.end()),result.end()); return result;
}
}
// 기술 허용 표의 deny/allow/all 순서와 시작 SP 덮어쓰기를 적용한다.
MissionPlayers MissionPlayers::Load(const std::function<std::optional<std::string>(std::string_view)>& get,
    const FortTemplate& fort,const RiftTypeTable& types,double fallbackMoney) {
    MissionPlayers result; result.techAllowed.assign(types.Types().size(),true);
    result.denySalvage=Number(get("denySalvage"),0)!=0; result.denyAscend=Number(get("denyAscend"),0)!=0; result.aiOff=Number(get("aiOff"),0)!=0;
    const auto permissions=get("techAllowed").value_or(""); bool allow=true;
    // 00482eb0의 상태 변경 토큰을 앞에서부터 적용한다.
    for (int i=0;;++i) {
        const auto token=ConfigListItem(permissions,i); if (token.empty()) break; const auto lower=AsciiLower(token);
        if (lower=="allow") allow=true; else if (lower=="deny") allow=false;
        else if (lower=="all") std::fill(result.techAllowed.begin(),result.techAllowed.end(),allow);
        else { const int type=types.Find(token); if (type>0) result.techAllowed[static_cast<std::size_t>(type)]=allow; }
    }
    // 소유 번호를 색 번호의 기본값으로 사용한다.
    for (int n=1;n<=kPlayerCount;++n) {
        auto& player=result.players[static_cast<std::size_t>(n)]; player.number=n; player.color=n; player.computer=n!=1; player.allies=static_cast<std::uint16_t>(1u<<n);
        const std::string prefix="ai"+std::to_string(n);
        // 번호가 없는 구식 ai 설정은 플레이어 2의 기본 입력이다.
        const auto aiGet=[&](std::string_view suffix) { const auto value=get(prefix+std::string(suffix)); return value ? value : n==2 ? get("ai"+std::string(suffix)) : std::nullopt; };
        player.name=n==1 ? "Player" : aiGet("Name").value_or(""); player.active=n==1 || !player.name.empty();
        player.stormPower=n==1 ? Number(get("myStartMoney"),fallbackMoney) : Number(aiGet("StartMoney"),0);
        player.startingTech=n==1 ? get("myTech").value_or("") : aiGet("Tech").value_or(""); player.knowledge=Knowledge(player.startingTech,types);
        player.ability=aiGet("Ability").value_or("");
        if (const auto color=aiGet("Color")) {
            const auto found=std::find(kColorNames.begin(),kColorNames.end(),AsciiLower(*color));
            if (found!=kColorNames.end() && found!=kColorNames.begin()) player.color=static_cast<int>(found-kColorNames.begin());
        }
        const auto allyText=n==1 ? get("myAllyList").value_or("") : aiGet("AllyList").value_or("");
        // 자기 자신은 항상 동맹이며 유효한 목록 번호만 추가한다.
        for (int i=0;;++i) { const auto token=ConfigListItem(allyText,i); if (token.empty()) break; const int ally=ConfigParseLong(token); if (ally>0 && ally<=kPlayerCount) player.allies |= static_cast<std::uint16_t>(1u<<ally); }
    }
    // 저장된 기술/덱은 시작 지식과 별도로 보존한다. 생산 덱의 재구성은 다음 단계다.
    result.players[1].storedTechnology=fort.technology; result.players[1].storedDeck=fort.deck;
    // 경로 조회에서도 동맹을 대칭으로 사용한다.
    for (int first=1;first<=kPlayerCount;++first)
        // 두 플레이어 중 한쪽의 지정으로 양쪽 비트를 켠다.
        for (int second=first+1;second<=kPlayerCount;++second) if (result.Allied(first,second)) {
            result.players[static_cast<std::size_t>(first)].allies |= static_cast<std::uint16_t>(1u<<second);
            result.players[static_cast<std::size_t>(second)].allies |= static_cast<std::uint16_t>(1u<<first);
        }
    return result;
}
// 중립·범위 밖 번호는 어느 플레이어와도 동맹이 아니다.
bool MissionPlayers::Allied(int first,int second) const {
    if (first<=0 || second<=0 || first>kPlayerCount || second>kPlayerCount) return false;
    return (players[static_cast<std::size_t>(first)].allies & (1u<<second))!=0 || (players[static_cast<std::size_t>(second)].allies & (1u<<first))!=0;
}
}
