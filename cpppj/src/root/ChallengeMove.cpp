// 실제 CCh 이동/전투 입장 요청의 검증과 같은 현재 명단·전송을 복원한다.
#include "root/ChallengeDetail.h"

namespace netstorm::root { //00476700→0044fad0과 현재 전투의 보조 조회를 공유한다.
using namespace challenge_detail; // 비정렬 원문 폭/같은 현재 패킷을 사용한다.
namespace { // 원문8자리의 현재 빈칸 수만 읽으며 첫빈칸으로 요청을 바꾸지 않는다.
std::uint32_t FreeSlots(std::span<const std::uint8_t> battle) { //0044c270은 매번8자리 전체를 세어0..8을 반환한다.
    std::uint32_t count{};for (std::size_t slot=0;slot<8;++slot) if (!Read(battle,0x20+slot*4)) ++count;return count; // 현재0만 빈자리이며 참가자valid 여부를 따로 조회하지 않는다.
}
}
void ChallengeRuntime::SailResult(std::uint32_t challenge,std::uint32_t participant,std::uint16_t status) {
    auto packet=Packet(0x25,8,8);AllocationInput(packet);Header(packet,0x25,8);Put(packet,6,status,2);Send(challenge,participant,packet); // 원문 지역 생성자는 헤더/상태만 쓰므로4/5BYTE를ff나0으로 합성하지 않는다.
}
std::int32_t ChallengeRuntime::CountBattlePirates(std::uint32_t challenge,std::uint32_t battle) {
    if (battle<1 || battle>9) Assert("bId > 0 && bId < CL_NUM_CHAL_BATTLES",2178);const auto at=8U+battle*0x158U;auto bytes=challenges_.Bytes(challenge);std::int32_t count{};
    if (!(Read(bytes,at)&1U)) { Need(hooks_.log)(9,"ERROR in countPirates: battle "+std::to_string(std::bit_cast<std::int32_t>(battle))+" is not valid!\n");return count; }
    for (std::uint32_t slot=0;slot<8;++slot) { // 현재 명단의 비영 참가자만 검사하며 invalid 로그 뒤에도 다음 자리를 본다.
        const auto member=Read(bytes,at+0x20U+slot*4);if (!member) continue;const auto flags=Read(Player(challenge,member),0x64);if (!(flags&1U)) Need(hooks_.log)(9,"ERROR in countPirates: player "+std::to_string(std::bit_cast<std::int32_t>(member))+" is not valid!\n");else if (flags&0x200U) ++count;
    }
    return count;
}
bool ChallengeRuntime::HasBattleMaster(std::uint32_t challenge,std::uint32_t battle) {
    const auto at=8U+battle*0x158U;auto bytes=challenges_.Bytes(challenge);
    for (std::uint32_t slot=0;slot<8;++slot) { // 현재8명 중 첫 master에서 즉시 반환하며 원문 valid 진단 뒤에도 bit4를 조회한다.
        const auto member=Read(bytes,at+0x20U+slot*4);if (!member) continue;const auto player=Player(challenge,member);if (!(Read(player,0x64)&1U)) Assert("p->isValid()",5537);if (Read(player,0x64)&4U) return true;
    }
    return false;
}
void ChallengeRuntime::MoveParticipant(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> request) {
    auto player=Player(challenge,participant);std::optional<std::uint32_t> destination;
    if (Read(request,12,1)) {
        const auto validate=[&]() -> bool { // 실패 때 원문 오류25h/기존22h를 보내며 현재 소속을 먼저 제거하지 않는다.
            const auto battle=Read(request,12,1);
            if (battle>=10) { Need(hooks_.log)(9,"Invalid battle id in RequestSailTo. pId == "+std::to_string(participant)+", battleId == "+std::to_string(battle)+"\n");SailResult(challenge,participant,2);return false; }
            const auto slot=Read(request,13,1);
            if (slot>=8) { Need(hooks_.log)(9,"Invalid battle slot in RequestSailTo. pId == "+std::to_string(participant)+", slot == "+std::to_string(slot)+"\n");SailResult(challenge,participant,3);return false; }
            destination=battle;const auto record=Battle(challenge,battle);if (!(Read(record,0)&1U)) { SailResult(challenge,participant,4);return false; }
            for (std::uint32_t remaining=16;remaining;--remaining) { //0044c1d0처럼 금지 subscriber16개를 끝에서 앞으로 조회하며0도 비교한다.
                if (Read(record,0x10c+(remaining-1U)*4)==Read(player,4)) { SailResult(challenge,participant,1);return false; }
            }
            const auto free=FreeSlots(record);static_cast<void>(CountBattlePirates(challenge,battle));if (!free || Read(record,0x20+slot*4)) return false; // 원문은 기존flag200 집계 반환값을 사용하지 않지만 진단 부작용은 보존한다.
            if (Read(player,0x64)&0x400U) { const std::array<std::uint8_t,1> emptyName{};const auto extended=!(Read(player,0xb0,2)<7 && Read(player,0xb2,2)<2);Message(challenge,participant,options_.badVersionNoBattle.value_or("~w~3~EYou cannot join until you install the current Netstorm 11.01 build.\n"),emptyName,extended);return false; } // 폐기 공식HQ 링크의 기본 안내만 현재 로컬 빌드로 바꾼다. 옵션/기존 버전 플래그는 그대로 읽는다.
            if (free!=8 && (static_cast<std::int32_t>(Read(player,0xb2,2))<std::bit_cast<std::int32_t>(Read(record,0x18)) || std::bit_cast<std::int32_t>(Read(record,0x1c))<static_cast<std::int32_t>(Read(player,0xb4,2)))) { SailResult(challenge,participant,6);return false; }
            return true;
        };
        if (!validate()) { Send(challenge,participant,PlayerChange(challenge,participant));return; }
    }
    if (Read(player,0xa4)) {
        const auto old=Read(player,0xa4),slot=Read(player,0xa8);auto bytes=challenges_.Bytes(challenge);Put(bytes,8U+old*0x158U+0x20U+slot*4,0);RecalculateBattleVersions(challenge,Read(player,0xa4));UpdateBattleMaster(challenge,Read(player,0xa4));Put(player,0xa4,0);Put(player,0xa8,0);Put(player,0x64,Read(player,0x64)&0xffff5ff9U); // 제거→버전→master 갱신 뒤 자기 소속/관련 플래그만 내리는 원문 순서다.
    }
    if (!Read(request,12,1)) { Copy(player,0x9c,request.subspan(4,8));Broadcast(challenge,PlayerChange(challenge,participant));return; } // 전투0은 원문 좌표 두DWORD를 그대로 적용하며 목적 자리 검사는 없다.
    if (!destination) { Assert("b",3121);throw std::out_of_range("R 이동의 원문 NULL 목적 전투 읽기 거부"); }const auto free=FreeSlots(Battle(challenge,*destination));auto bytes=challenges_.Bytes(challenge);const auto at=8U+*destination*0x158U+0x20U+Read(request,13,1)*4;
    if (Read(bytes,at)) Assert("!playerIds[slot]",446);Put(bytes,at,participant);RecalculateBattleVersions(challenge,Read(request,12,1)); // 진단이 복귀하면 요청 자리를 덮으며 공유scratch가 입력을 바꾼 경우 현재BYTE를 다시 읽는다.
    if (!(Read(player,0x64)&0x800U) && (free==8 || !HasBattleMaster(challenge,Read(request,12,1)))) Put(player,0x64,Read(player,0x64)|4U);
    Put(player,0xa4,Read(request,12,1));Put(player,0xa8,Read(request,13,1));Copy(player,0x9c,request.subspan(4,8));Broadcast(challenge,PlayerChange(challenge,participant));
    const auto battle=Read(player,0xa4);const auto record=Battle(challenge,battle);
    for (std::uint32_t slot=0;slot<8;++slot) { // 현재 전투의 자기 자신을 포함한 비영8명의 공유22h를 새 참가자에게 순서대로 보낸다.
        const auto member=Read(record,0x20+slot*4);if (member) Send(challenge,participant,PlayerChange(challenge,member));
    }
    const auto length=Read(record,0x40,1);auto options=Packet(0x67,length+7U);Put(options,4,Read(player,0xa4,1),1);Put(options,5,length,2);Copy(options,7,record.subspan(0x41,length));Send(challenge,participant,options); // 조회68h의 고정160과 달리 이동 완료는 현재 보관BYTE 길이만 전송한다.
}
}
