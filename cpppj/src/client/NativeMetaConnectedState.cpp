// 원본 PE의 상태별 실제 인자와 캐시/닫힘/주소 재조회의 순서를 복원한다.
#include "client/NativeMetaConnectedState.h"
#include "client/ClientChallengeGlobals.h"
#include <array>
#include <bit>
#include <cstdio>
#include <stdexcept>

namespace netstorm::client { //메타 표시/패킷 부모는 현재 실제 장치에 연결하는 필수 hook 경계다.
namespace { //현재 CS 바이트와 실제 callback만 접근한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("메타 State28~33의 실제 장치 미연결");return function; } //선택되지 않은 하위는 미리 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { //현재 비정렬 DWORD/BYTE의 물리만 확인한다.
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 상태의 실제 CS 필드 부족");
    std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; //낮은 BYTE부터 x86 정수를 조립한다.
}
std::span<const std::uint8_t> Participant(std::span<const std::uint8_t> bytes,std::uint32_t index) { //캡처한 기존 CS 또는 현재 CS의 실제40기록 상한이다.
    if (index>=o::kChallengeParticipantCount) throw std::out_of_range("메타 상태의 실제 참가자40칸 범위");const auto at=o::kChallengeParticipantOffset+index*o::kChallengeParticipantBytes;
    if (at>bytes.size() || o::kChallengeParticipantBytes>bytes.size()-at) throw std::out_of_range("메타 상태의 실제 참가자 물리 부족");return bytes.subspan(at,o::kChallengeParticipantBytes);
}
void Invalidate(NativeMetaConnectedGlobals g) { g.cache.index=0xffffffffU;g.cache.value=0xffffffffU;g.cache.battle=0xffffffffU;g.cache.slot=0xffffffffU;g.notified=0; } //원문 네 DWORD 순서와 뒤쪽 통지0쓰기를 보존한다.
void CloseGamePeers(const NativeMetaConnectedHooks& h) { Need(h.closePending)();for (std::uint32_t id=1;id<=9;++id) Need(h.closeConnection)(id); } //현재 pending 정리 뒤1..9를 모두 닫으며10/11은 남긴다.
void ConnectMaster(NativeMetaConnectedGlobals g,const NativeMetaConnectedHooks& h,std::int32_t master) { //닫힘 callback 이후의 현재 CS와 포트를 다시 읽는다.
    const auto port=g.gamePort;const auto record=g.challenge.Participant(std::bit_cast<std::uint32_t>(master));const auto address=record.subspan(0x1a0);
    Need(h.connect)(9,{reinterpret_cast<const char*>(address.data()),address.size()},port); //같은44BYTE 경계만 넘긴다. 기존9 준비 비영의 조기 재사용 뒤에만 실제 TCP가 NUL/주소를 읽는다.
}
}
std::int32_t FindMetaBattleMaster(ClientChallengeGlobals& challenge,std::uint32_t owner) {
    const auto battle=Read(challenge.Participant(owner),0xa4);if (!battle) return -1;const auto current=challenge.Battle(battle);
    for (std::uint32_t slot=0;slot<8;++slot) { //참가자0을 건너뛴 뒤 현재 flags의bit4만 확인하며 별도 active 조건을 더하지 않는다.
        const auto candidate=Read(current,0x38+slot*4U);if (!candidate) continue;if (Read(challenge.Participant(candidate),0x64,1)&4U) return std::bit_cast<std::int32_t>(candidate);
    }return -1;
}
std::string MetaLoginReason(std::int32_t code,const std::function<std::string(std::string_view)>& translate) {
    auto reason=Need(translate)("Unknown error");
    constexpr std::array<std::string_view,6> reasons{{"Unspecified reason","Login successful","Initial info sent","Player already logged in","Name length is bad","The server is full"}}; //PE005316b0의 여섯 포인터 뒤NULL로 끝나는 원문 표다.
    if (code>=0 && static_cast<std::size_t>(code)<reasons.size()) reason=reasons[static_cast<std::size_t>(code)];return Need(translate)(reason); //범위 밖도 선번역된 Unknown을 다시 번역하는 원문 순서다.
}
o::LocalMissionParentResult InstallNativeMetaConnectedState(std::uint32_t selected,NativeMetaConnectedGlobals g,const NativeMetaConnectedHooks& h) {
    if (selected<28 || selected>33 || g.state.current[0]!=selected) throw std::logic_error("메타 State28~33의 실제 current 계약");
    switch (selected) {
    case 28:
        if (g.state.current[0x38/4]==1) { o::ResetChallengeState(g.challenge.Bytes());Need(h.request)(29);g.zone=g.state.phase[0x34/4];Need(h.resetCamera)(); }
        else {
            const auto format=Need(h.translate)("Challenge server login denied:%s");const auto reason=MetaLoginReason(std::bit_cast<std::int32_t>(g.state.current[0x38/4]),h.translate);std::array<char,256> text;
            const auto length=std::snprintf(text.data(),text.size(),format.c_str(),reason.c_str());if (length<0 || static_cast<std::size_t>(length)>=text.size()) throw std::out_of_range("메타 로그인 거부 문구의 원문256BYTE 물리 초과");Need(h.progress)({text.data(),static_cast<std::size_t>(length)}); //원문 번역 format/문자열 한 인자의 printf와 실제 지역 물리 경계를 사용한다.
            auto section=std::string_view("CSLGenericFail");switch (g.state.current[0x38/4]) { case 3:section="CSLAlreadyHere";break;case 4:section="CSLBadNameLen";break;case 5:section="CSLServerFull";break;case 6:section="CSLShutdown";break;default:break; }
            g.state.immediate[0x3c/4]=Need(h.tell)(section);Need(h.request)(32);Need(h.resetCamera)();
        }break;
    case 29: { Need(h.closeProgress)();g.metadataMode=1;g.connected=1;Need(h.display)();Need(h.rootLogin)(11);const auto port=g.gamePort;Invalidate(g);Need(h.listen)(port);g.state.next[0]=33;break; } //원문841c는 현재 포트를 네 캐시/통지 초기화 전에 캡처한다.
    case 30:Need(h.progress)(Need(h.translate)("Communicating with Challenge Server."));g.connected=0;break;
    case 31:g.connected=1;break;
    case 32: { const auto serial=Need(h.modalSerial)();if (!serial || *serial!=g.state.current[0x3c/4]) Need(h.request)(20);else g.state.next[0]=32;break; }
    case 33: {
        if (!Need(h.open)(11)) { Need(h.progress)(Need(h.translate)("Challenge Server connection lost."));g.state.next[0]=3;break; }
        if (!g.player.playerKnown) break;const auto bytes=g.challenge.Bytes();const auto own=Participant(bytes,g.player.battleOwner);const auto battle=Read(own,0xa4);
        if (!battle) { CloseGamePeers(h);Invalidate(g); }
        else {
            const auto master=FindMetaBattleMaster(g.challenge,g.player.battleOwner);const auto value=master<0 ? 0xffffffffU:Read(Participant(bytes,std::bit_cast<std::uint32_t>(master)),0xa8);
            if (g.cache.index!=std::bit_cast<std::uint32_t>(master) || g.cache.value!=value || g.cache.battle!=battle || g.cache.slot!=Read(own,0xa8)) {
                g.cache.index=std::bit_cast<std::uint32_t>(master);g.cache.value=value;g.cache.battle=Read(own,0xa4);g.cache.slot=Read(own,0xa8);g.notified=0;CloseGamePeers(h);
                if (std::bit_cast<std::uint32_t>(master)==g.player.battleOwner) Need(h.notifyMaster)();else ConnectMaster(g,h,master); //원문은 변경 분기의master=-1도 먼저 주소 접근하므로 임의 무시하지 않는다.
            }
            if (std::bit_cast<std::uint32_t>(master)!=g.player.battleOwner && master!=-1) {
                if (!Need(h.open)(9)) ConnectMaster(g,h,master);
                if (Need(h.status)(9)!=0 && !g.notified) { g.notified=1;Need(h.sendLogin)();g.state.next[0]=33;return o::LocalMissionParentResult::Commit; } //음수 상태도 비영이면 같은 통지 경로를 선택한다.
            }
        }g.state.next[0]=33;break;
    }
    default:break;
    }return o::LocalMissionParentResult::Commit;
}
}
