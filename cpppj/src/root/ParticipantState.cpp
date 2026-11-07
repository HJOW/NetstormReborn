// 실제 참가자 생성자의 지정 폭 쓰기/문자 제거·시계와 미작성 필드 보존을 복원한다.
#include "root/ParticipantState.h"
#include "root/ChallengeDetail.h"
#include <ctime>

namespace netstorm::root { // 로컬 full participant와 Root mirror의 생성자 의미를 합친다.
using namespace challenge_detail; // 같은 비정렬 필드/첫NUL 복사 경계를 사용한다.
void InitializeParticipant(std::span<std::uint8_t> player,std::uint32_t id,std::uint32_t subscriber,std::optional<std::string_view> name,std::span<const std::uint8_t,6> identity,std::span<const std::uint8_t,10> core,std::span<const std::uint8_t,8> position,std::uint32_t flags) {
    if (player.size()!=kChallengeParticipantBytes) throw std::out_of_range("R 참가자 생성자의 실제188BYTE 필요");Put(player,0,id);Put(player,4,subscriber);
    if (!name) player[0x26]=0;
    else {
        const auto text=name->substr(0,name->find('\0'));if (text.size()+1>40) throw std::out_of_range("R 참가자 이름의 실제40BYTE 범위");PutText(player,0x26,text);
        constexpr std::string_view remove=".?=()[]~\"`;: "; // PE004cd528의 실제 제거 문자 집합이다.
        std::size_t at=0x26;
        while (player[at]) { //0044c8e0처럼 현재 이름에서 겹침 이동하며 뒤쪽 기존 바이트는 보존한다.
            if (remove.find(static_cast<char>(player[at]))==std::string_view::npos) { ++at;continue; }const auto tail=Text(player,at);std::memmove(player.data()+at,player.data()+at+1,tail.size());
        }
        player[at]=0;
    }
    Put(player,0x6c,static_cast<std::uint32_t>(std::clock()));Put(player,0x70,0);Put(player,0x74,0,1);Copy(player,0x4e,identity);Copy(player,0x54,core);Put(player,0x64,((flags&0x1b00U)|1U)&~0x200U);Copy(player,0x9c,position);Put(player,0xa4,0);Put(player,0xa8,0);
}
}
