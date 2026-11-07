// 실제83h 관리 명령의 현재 비밀번호·소켓/전투/예약·Root 전체 안내를 연결한다.
#include "root/ChallengeDetail.h"

namespace netstorm::root { //00476490→00456b30의 같은 CCh/현재 옵션·상위 장치다.
using namespace challenge_detail; // 실제 입력 바이트와 원문 미작성 지역 바이트 경계를 공유한다.
namespace { // 전처리는 패킷 암호를 바꾸지 않는 원문 지역 계산만 보존한다.
void PrepareAdmin() { //00456df0은9BYTE 지역키를사용하지않고30회지역위상만계산한다.원문입력자체는읽거나쓰지않는다.
    std::uint32_t phase{};for (std::uint32_t index=0;index<30;++index) phase=phase==9 ? 0:phase+1;static_cast<void>(phase); // 원문의0..9 고리30회이며 외부부작용은없다.
}
}
void ChallengeRuntime::AdminCommand(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> packet) {
    PrepareAdmin();const auto expected=std::string_view(options_.clientAdmin).substr(0,options_.clientAdmin.find('\0'));if (Text(packet,16)!=expected) return;
    switch (Read(packet,4)) {
    case 0:
        for (std::uint32_t candidate=0;candidate<40;++candidate) { // 활성여부와무관한첫subscriber일치의실제레코드id로하위소켓만닫는다.
            const auto player=Player(challenge,candidate);if (Read(player,4)!=Read(packet,8)) continue;const auto current=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(Read(player,0));if (current) transport_.CloseSocket(current);break;
        }break;
    case 3:Need(hooks_.schedule)(0,static_cast<double>(std::bit_cast<std::int32_t>(Read(packet,8))));break; // 원문 fild의 signed초 값을 넘기며분단위로바꾸지않는다.
    case 4:RepositionBattle(challenge,Read(packet,12),Read(packet,8));break;
    case 5:RepositionBattle(challenge,Read(packet,8));break;
    case 6: {
        auto request=Packet(0x1e,14,14);Put(request,12,Read(packet,174,1),1);Put(request,13,Read(packet,175,1),1);Put(request,4,Read(packet,166));Put(request,8,Read(packet,170));MoveParticipant(challenge,Read(packet,8),request);break; // 목적BYTE/자리→x/y 두DWORD를 원문 위치와 순서로 읽고 같은 이동 부모를 호출한다.
    }
    case 7:case 8: {
        std::array<std::uint8_t,124> message;AllocationInput(message);const auto text=Text(packet,46);PutText(message,0,text);PutText(message,text.size(),"\n");Need(hooks_.broadcastRoot)(std::span<const std::uint8_t>(message).first(122),Read(packet,4)==8);break; // 원문124BYTE지역의strcpy/strcat뒤정확히122BYTE를전달하며NUL뒤미작성바이트도포함한다.
    }
    case 9:Put(Player(challenge,participant),0x75,1,1);break;
    case 10:StartBattle(challenge,Read(packet,8));break;
    default:break; // 원문1/2와그외명령은비밀번호가맞아도부작용없이반환한다.
    }
}
}
