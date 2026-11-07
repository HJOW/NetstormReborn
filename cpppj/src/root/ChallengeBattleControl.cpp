// CCh의 실제 전투 제어 수신과 옵션/설명 바이트·통지 순서를 복원한다.
#include "root/ChallengeDetail.h"

namespace netstorm::root { //004763d0/476940/476a00/476ac0의 현재 CCh 수신 부모다.
using namespace challenge_detail; // 원문 비정렬 폭과 같은 입력/공유 송신 물리를 사용한다.
namespace { // 논리 전투 번호 검사와 다른 실제 전체 CCh 메모리 접근을 구분한다.
template<class Byte> std::span<Byte> ControlBytes(std::span<Byte> bytes,std::size_t at,std::size_t length) { // memcpy의 실제 입력/목적 구간만 확인하며 원문에 없는 논리10칸 관문을 추가하지 않는다.
    if (at>bytes.size() || length>bytes.size()-at) throw std::out_of_range("R 전투 제어의 실제 CCh/입력 물리 범위");return bytes.subspan(at,length);
}
std::string BattleViolation(std::string_view operation,std::uint32_t participant,std::string_view reason,std::span<const std::uint8_t> player) { // 원문 level9 진단의 현재 참가자 이름/부호 있는 subscriber와 줄 끝을 작성한다.
    return "HACK ATTEMPT in "+std::string(operation)+": player "+std::to_string(participant)+" "+std::string(reason)+"\nName=\""+Text(player,0x26)+"\", subId="+std::to_string(std::bit_cast<std::int32_t>(Read(player,4)))+"\n";
}
}
void ChallengeRuntime::ReceiveBattleControl(std::uint8_t type,std::uint32_t sender,std::span<std::uint8_t> packet) {
    if (state_.exitRequested) { Need(transport_.Lifecycle().exit)(0);throw std::logic_error("R 전투 제어 _exit 장치가 복귀했습니다"); }
    const auto line=type==0x10 ? 1333U:(type==0x1e ? 1409U:(type==0x1f ? 1424U:(type==0x67 ? 1455U:(type==0x68 ? 1471U:(type==0x83 ? 1349U:(type==0x87 ? 1365U:1487U))))));if (!(state_.modeFlags&kChallengeMode)) Need(transport_.Lifecycle().assertion)("smModeFlags & smCHAL","c:\\ns\\r\\r.cpp",line);
    const auto zone=std::bit_cast<std::int32_t>(sender)>>8;if (zone<=0 || zone>=state_.challengeStateCount) return;const auto current=state_.challenges.at(static_cast<std::size_t>(zone));if (!current) return;const auto participant=sender&0xffU;
    switch (type) {
    case 0x10:Put(challenges_.Bytes(current),kChallengeCounterOffset,Read(packet,4) ? 1:0);break; //004518e0은 참가자/권한·원래 요청 수치를 사용하지 않고 마지막 카운터를0/1로 쓴다.
    case 0x1e:MoveParticipant(current,participant,packet);break;
    case 0x1f:ReadyParticipant(current,participant,packet);break;
    case 0x67:BattleOptions(current,participant,packet);break;
    case 0x68:QueryBattleOptions(current,participant,packet);break;
    case 0x71:UpdateBattleDescription(current,participant,packet);break;
    case 0x83:AdminCommand(current,participant,packet);break;
    case 0x87:Command(current,participant,packet);break;
    default:throw std::logic_error("등록하지 않은 R 전투 제어 타입");
    }
}
void ChallengeRuntime::BattleOptions(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet) {
    auto player=Player(challenge,participant);const auto at=8U+Read(player,0xa4)*0x158U;auto bytes=challenges_.Bytes(challenge);Need(hooks_.log)(0,"Battle Command by "+std::to_string(std::bit_cast<std::int32_t>(Read(player,0xa4)))+" to "+std::to_string(Read(packet,4,1))+"\n");
    if (!Read(player,0xa4)) { Need(hooks_.log)(9,BattleViolation("handleChalBattleOptions",participant,"is not in a battle",player));return; }
    if (Read(player,0xa4)!=Read(packet,4,1)) { Need(hooks_.log)(9,BattleViolation("handleChalBattleOptions",participant,"is not in battle "+std::to_string(Read(packet,4,1)),player));return; }
    if (!(Read(player,0x64)&4U)) { Need(hooks_.log)(9,BattleViolation("handleChalBattleOptions",participant,"is not the master.",player));return; }
    const auto length=Read(packet,5,2);
    if (length<=160) { Copy(bytes,at+0x41U,ControlBytes(packet,7,length));Put(bytes,at+0x40U,Read(packet,5,1),1); } // 옵션은 요청 WORD 길이만 복사하고 보관 길이 필드는 낮은BYTE만 쓴다. 뒤쪽 바이트를 지우지 않는다.
    else Need(hooks_.log)(9,"ERROR in handleChalBattleOptions: option data too long\n[bufferLen == 160, optionLen == "+std::to_string(length)+"]\n"); // 원문 오류 뒤에도 아래의 상태 변경/통지를 진행한다.
    for (std::uint32_t slot=0;slot<8;++slot) { // 처음 계산한 자기 전투의 현재8명에서 비영 참가자만 bit2를 내리고 공유22h를 보낸다.
        const auto member=Read(bytes,at+0x20U+slot*4);if (!member) continue;auto record=Player(challenge,member);Put(record,0x64,Read(record,0x64)&~2U);auto change=PlayerChange(challenge,member);SendBattle(challenge,Read(packet,4,1),change); // 공유 작성 뒤 요청의 현재 전투BYTE를 읽어 입력/공유scratch 별칭도 보존한다.
    }
    SendBattle(challenge,Read(packet,4,1),packet); // 원문은 입력 옵션 패킷을 재작성하거나 요청 길이를 정상값으로 바꾸지 않는다.
}
void ChallengeRuntime::QueryBattleOptions(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> packet) {
    const auto at=8U+Read(packet,4,1)*0x158U;auto bytes=challenges_.Bytes(challenge);if (!(Read(bytes,at)&1U)) { Need(hooks_.log)(9,"ERROR in handleChalBattleQueryOptions: battle "+std::to_string(Read(packet,4,1))+" not valid.\n");return; }
    auto reply=Packet(0x67,167);Put(reply,4,Read(packet,4,1),1);Put(reply,5,160,2);Copy(reply,7,ControlBytes(bytes,at+0x41U,160));Send(challenge,participant,reply); // 현재 옵션 길이/참가자의 소속/master와 무관하게 실제160BYTE 전체를 응답한다.
}
void ChallengeRuntime::UpdateBattleDescription(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet) {
    auto player=Player(challenge,participant);const auto at=8U+Read(player,0xa4)*0x158U;auto bytes=challenges_.Bytes(challenge);
    if (!Read(player,0xa4) && player[0x75]!=1) { Need(hooks_.log)(9,BattleViolation("handleChalBattleUpdate",participant,"is not in a battle",player));return; }
    if (Read(player,0xa4)!=Read(packet,4,1) && player[0x75]!=1) { Need(hooks_.log)(9,BattleViolation("handleChalBattleUpdate",participant,"is not in battle "+std::to_string(Read(packet,4,1)),player));return; }
    if (!(Read(player,0x64)&4U) && player[0x75]!=1) { Need(hooks_.log)(9,BattleViolation("handleChalBattleUpdate",participant,"is not the master.",player));return; }
    if (player[0x74] && player[0x75]!=1) return;const auto length=Read(packet,9,1);
    if (length<=41) { Copy(bytes,at+0xe1U,ControlBytes(packet,10,length));Put(bytes,at+0xe0U+length,0,1); } // 관리자는 요청 번호와 달라도 자기 전투에 쓴다. 길이0이면 설명 직전의 마지막 옵션BYTE를0으로 쓴다.
    else Need(hooks_.log)(9,"ERROR in handleChalBattleUpdate: desc too long\n[bufferLen == 41, optionLen == "+std::to_string(length)+"]\n"); // 길이 초과는 보관만 생략하고 원문 전체 방송은 계속한다.
    Broadcast(challenge,packet);
}
}
