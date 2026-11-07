// CCh의 실제41h 채팅 수신과 제자리 수정·참가자/감시자 전송 순서를 복원한다.
#include "root/ChallengeDetail.h"
#include <ctime>

namespace netstorm::root { //00476880→004512b0의 현재 CCh/옵션과 실제 송신을 공유한다.
using namespace challenge_detail; // 실제 비정렬 폭/첫NUL·현재 패킷 allocator를 사용한다.
namespace { // PE의 원문 바이트와41h의 서로 다른 signed 필드 해석을 보관한다.
constexpr std::string_view kLineWarning="~3~E~rChat Blocked!<br>~.~E~wYour message has exceeded this server\x92s multiple line regulation and was blocked. This regulation was added to prevent Spamming and should you have any questions concerning it talk to a local administrator."; //004be518의 CP1252 아포스트로피를 포함한 실제 wire 바이트다.
constexpr std::string_view kRateWarning="~3~E~rChat Blocked!<br>~.~E~wYou have sent to many lines of chat and must wait a few seconds before you can send another message!\n"; //004be470의 원문 문구/줄 끝이다.
std::size_t ChatBody(std::span<const std::uint8_t> packet) { //446280의 signed nameLen을 물리 시작 위치로 바꾼다.
    const auto offset=15+static_cast<std::int32_t>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(packet,10,2))));if (offset<0 || static_cast<std::size_t>(offset)>=packet.size()) throw std::out_of_range("R41h 본문의 실제 시작 범위");return static_cast<std::size_t>(offset);
}
std::size_t ChatRecipients(std::span<const std::uint8_t> packet) { return ChatBody(packet)+Read(packet,12,2); } //4462b0은 nameLen만 signed이며 bufferLen은 unsigned WORD다. 실제 DWORD 접근은 Read/Put이 확인한다.
}
void ChallengeRuntime::ReceiveChat(std::uint32_t sender,std::span<std::uint8_t> packet) {
    if (state_.exitRequested) { Need(transport_.Lifecycle().exit)(0);throw std::logic_error("R41h _exit 장치가 복귀했습니다"); }
    if (!(state_.modeFlags&kChallengeMode)) Need(transport_.Lifecycle().assertion)("smModeFlags & smCHAL","c:\\ns\\r\\r.cpp",1439);
    const auto zone=std::bit_cast<std::int32_t>(sender)>>8;if (zone<=0 || zone>=state_.challengeStateCount) return;const auto current=state_.challenges.at(static_cast<std::size_t>(zone));if (current) Chat(current,sender&0xffU,packet); // 별도 local/active 비트 검사를 더하지 않는 원문 관문이다.
}
bool ChallengeRuntime::ChatCommand(std::uint32_t challenge,std::uint32_t participant,std::string_view text) {
    for (const auto command:std::array<std::string_view,2>{".test",".test1"}) { //0044b5c0으로 나눈 두 명령을 원문 순서의 부분 검색으로 찾는다.
        if (text.find(command)==std::string_view::npos) continue;constexpr std::string_view reply="A COMMAND WAS FOUND WOOO\n\r"; //004be360/50ff0의 실제 응답이며 뒤의 NUL은 bufferLen에 포함하지 않는다.
        auto packet=Packet(0x41,reply.size()+20);Put(packet,0,3,1);Put(packet,5,1,1);Put(packet,6,0);Put(packet,10,1,2);Put(packet,12,static_cast<std::uint32_t>(reply.size()),2);Put(packet,14,1,1);Put(packet,15,0,1);Copy(packet,16,{reinterpret_cast<const std::uint8_t*>(reply.data()),reply.size()});Put(packet,16+reply.size(),0);Send(challenge,participant,packet);return true; // recipient0의 첫0바이트가 문자열 종결도 맡는다. 입력 명령 패킷은 변경하지 않는다.
    }
    return false;
}
void ChallengeRuntime::Chat(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet) {
    auto player=Player(challenge,participant);const auto original=Text(packet,ChatBody(packet));if (ChatCommand(challenge,participant,original)) return;const auto lines=static_cast<std::int32_t>(std::count(original.begin(),original.end(),'\n'));const std::array<std::uint8_t,1> emptyName{};
    if (options_.multiLineLimit<lines && player[0x75]!=1) { Message(challenge,participant,kLineWarning,emptyName,true);return; }
    std::size_t cursor{};
    while ((cursor=original.find("~4",cursor))!=std::string::npos) { // 원문에서 찾는 모든 ~4 쌍을 같은 입력 본문의 두 공백으로 바꾼다.
        const auto body=ChatBody(packet);Put(packet,body+cursor,' ',1);Put(packet,body+cursor+1,' ',1);++cursor;
    }
    Put(player,0x68,static_cast<std::uint32_t>(std::clock()));const auto elapsed=static_cast<float>(std::bit_cast<std::int32_t>(Read(player,0x68)-Read(player,0x6c))/1000);const auto penalty=std::max(std::bit_cast<std::int32_t>(Read(player,0x70)-3U),0);const auto adjusted=static_cast<double>(elapsed)-static_cast<double>(penalty); //451890은 DWORD 감산→signed idiv1000→float 저장이다. 초 소수부를 만들지 않는다.
    if (!(static_cast<double>(options_.spamTime)<=adjusted)) { //451489 fcompp/test41/jp는 threshold가 더 크거나 unordered일 때만 누적한다. 같은 값이면 타이머를 다시 시작한다.
        Put(player,0x70,Read(player,0x70)+1U);if (options_.spamCount<=std::bit_cast<std::int32_t>(Read(player,0x70)) && player[0x75]!=1) { Message(challenge,participant,kRateWarning,emptyName,true);return; }
    } else { Put(player,0x6c,static_cast<std::uint32_t>(std::clock()));Put(player,0x70,0); } // 원문 reset은 앞의 clock 값을 재사용하지 않고 새 CRT clock을 읽는다.
    if (Read(packet,14,1)==1 && !Read(packet,ChatRecipients(packet))) { Need(hooks_.log)(29,"Broadcasting chat to all\n");Put(packet,14,0,1);Broadcast(challenge,packet);return; }
    const auto recipients=Read(packet,14,1);Put(packet,14,1,1);const auto subscriber=std::bit_cast<std::int32_t>(Read(packet,6));const auto nameLength=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(packet,10,2)));
    Need(hooks_.log)(29,nameLength<1 ? "Sending chat from (empty name) ["+std::to_string(subscriber)+"]\n":"Sending chat from \""+Text(packet,15)+"\" ["+std::to_string(subscriber)+"]\n"); // 원문 empty-name 분기의 추가 printf 인자는 사용되지 않는다.
    for (std::uint32_t recipient=0;recipient<recipients;++recipient) { // 원래 수신자 수를 보존하되 현재 첫 DWORD를 매회 덮는다. 목록 사본이나 패킷 복사본을 만들지 않는다.
        const auto target=Read(packet,ChatRecipients(packet)+recipient*4);Put(packet,ChatRecipients(packet),target);const auto id=std::bit_cast<std::int32_t>(target);Need(hooks_.log)(29,"Sending to subId "+std::to_string(id)+"\n");std::optional<std::uint32_t> found;
        for (std::uint32_t candidate=0;candidate<40;++candidate) { //0044d3b0의 첫 subscriber 일치만 찾고 활성 비트로 거르지 않는다.
            if (Read(Player(challenge,candidate),4)==target) { found=candidate;break; }
        }
        if (!found) { Need(hooks_.log)(29,"No player with subId == "+std::to_string(id)+"\n");continue; }const auto destination=Player(challenge,*found);const auto destinationId=Read(destination,0);const auto connection=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(destinationId);
        if (!connection) { Need(hooks_.log)(29,"No transceiver for subId == "+std::to_string(id)+"\n");continue; }if (!destination[0x75] && (!Player(challenge,participant)[0x74] || destinationId==participant)) transport_.Send(connection,packet); // 관리자는 별도 감시자 경로로만 보내며 muted 발신자의 자기 전달만 허용한다.
    }
    for (std::uint32_t observer=0;observer<40;++observer) { //0044e380은 현재 연결/비영 +75만 보며 마지막 수신자 DWORD와 addrLen1을 그대로 전달한다.
        const auto connection=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(observer);if (connection && Player(challenge,observer)[0x75]) transport_.Send(connection,packet);
    }
}
}
