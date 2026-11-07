// CCh 준비/시작 요청의 현재 대상·명단/플래그와 실제 전투 시작·퇴장을 연결한다.
#include "root/ChallengeDetail.h"

namespace netstorm::root { //004767c0→00450530의 같은 현재 CCh 처리기다.
using namespace challenge_detail; // 실제 입력을 제자리 수정하고 원문 공유22h/전송을 사용한다.
void ChallengeRuntime::ReadyParticipant(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet) {
    const auto find=[&](std::uint32_t subscriber) -> std::optional<std::uint32_t> { //0044d3b0처럼 활성 여부와 무관한0..39 첫subscriber 일치를 찾는다.
        for (std::uint32_t candidate=0;candidate<40;++candidate) if (Read(Player(challenge,candidate),4)==subscriber) return candidate;return {};
    };
    const auto found=find(Read(packet,4));
    if (!found) {
        Need(hooks_.log)(9,"could not find player ["+std::to_string(Read(packet,4))+"]: c:\\ns\\o\\ch.cpp 3264\n");const auto current=ChallengeDirectoryView(challenges_.Bytes(challenge)).Transceiver(participant);if (!current) Need(hooks_.log)(0,"NULL transceiver <"+std::to_string(participant)+">: c:\\ns\\o\\ch.cpp 3268\n");else { transport_.Terminate(current);Need(hooks_.log)(0,"Kicking Player\n"); }return;
    }
    auto target=Player(challenge,*found);const auto battle=Read(target,0xa4);if (battle>=10) return;auto record=Battle(challenge,battle);
    if (Read(packet,8)&0x4000U) {
        Put(packet,8,Read(packet,8)&~0x4000U);Put(packet,8,Read(packet,8)&~2U);
        for (std::uint32_t slot=0;slot<8;++slot) { // 원문의 현재8명 bit2를 내리고 공유22h 작성 뒤 대상의 현재 소속을 읽어 전송한다.
            const auto member=Read(record,0x20+slot*4);if (!member) continue;auto player=Player(challenge,member);Put(player,0x64,Read(player,0x64)&~2U);auto change=PlayerChange(challenge,member);SendBattle(challenge,Read(target,0xa4),change);
        }
    }
    Put(target,0x64,Read(target,0x64)&0x1f6dU);Put(target,0x64,(Read(packet,8)&0xffffe092U)|Read(target,0x64));bool sailed{}; // 두 쓰기 사이에도 현재 입력을 읽어 입력/참가자 필드 별칭을 보존한다.
    if (Read(Player(challenge,participant),0x64)&4U) {
        if (Read(packet,8)&0x10U) {
            std::uint32_t unready{},pirates{},total{};
            for (std::uint32_t slot=0;slot<8;++slot) { // 현재 명단을 순서대로 세며 첫 미준비 참가자에서 원문처럼 즉시 멈춘다.
                const auto member=Read(record,0x20+slot*4);if (!member) continue;const auto flags=Read(Player(challenge,member),0x64);++total;if (!(flags&2U)) { ++unready;break; }if (flags&0x200U) ++pirates;
            }
            static_cast<void>(pirates);static_cast<void>(total); // 폐기 클라이언트 검증 정책에 따라 limitPirates의거부25h5/재배치 소비자는 연결하지 않는다.
            if (!unready) { StartBattle(challenge,Read(target,0xa4));return; }
        }
        if (Read(packet,8)&8U) {
            const auto current=find(Read(packet,4));if (!current) throw std::out_of_range("R 준비 퇴장의 원문 NULL 대상 읽기 거부");if (Read(target,0xa4)!=Read(Player(challenge,*current),0xa4)) return;const auto banned=Read(packet,4);Copy(record,0x10c,record.subspan(0x110,60));Put(record,0x148,banned);SailOut(challenge,*found,battle,false);sailed=true; // 요청subscriber를금지목록끝에넣고실제난수위치로퇴장시킨다.시작bit10은별도로내리지않는다.
        }
    }
    auto change=PlayerChange(challenge,*found);
    if (!sailed) { for (std::uint32_t slot=0;slot<8;++slot) { const auto member=Read(record,0x20+slot*4);if (member) Send(challenge,member,change); } } // 현재 대상 전투 명단의 비영8명에게 같은 공유22h를 전달한다.
    else Broadcast(challenge,change);
}
}
