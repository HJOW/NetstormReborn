//10.78 메타 항해의 원문 읽기 전용 표·x86 DWORD·현재 CS/패킷 부분 효과를 보존한다.
#include "client/NativeMetaSail.h"
#include "client/NativeMetaSpriteDraw.h"
#include <bit>
#include <stdexcept>
#include <string>

namespace netstorm::client { //장치는 동기 호출 동안만 빌리며 새 Session이나 통신 owner를 만들지 않는다.
namespace { //선택한 물리 필드와 하위만 확인한다.
constexpr float kDiagonal=std::bit_cast<float>(std::uint32_t{0x3f3504f8}); //5316f8의 실제 단정도 양수 대각선 성분이다.
constexpr std::array<std::array<float,2>,8> kOffset{{{0,1},{0,-1},{-1,0},{1,0},{-kDiagonal,-kDiagonal},{kDiagonal,kDiagonal},{-kDiagonal,kDiagonal},{kDiagonal,-kDiagonal}}}; //5316d0의8쌍이며 좌석 순서를 바꾸지 않는다.
constexpr std::array<std::array<float,2>,8> kScale{{{0.5F,1},{0.5F,0},{0,0.5F},{1,0.5F},{0,0},{1,1},{0,1},{1,0}}}; //531710의8쌍 크기 배율이다. 네 대각선도 같은 표를 사용한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("메타 항해의 선택된 실제 장치 미연결");return value; } //조기 반환에서 미사용 장치를 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("메타 항해의 실제 DWORD 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //선택한 DWORD만 낮은 BYTE부터 조립한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 항해의 실제 필드 쓰기 범위");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //주변 바이트는 덮지 않고 지정 폭만 기록한다.
void Packet(std::span<std::uint8_t> packet) { Put(packet,0,1,1);Put(packet,1,0x1e,1);Put(packet,2,14,2); } //40a2c0/52ed86의 고정 헤더이며 payload는 caller가 모두 작성한다.
std::span<std::uint8_t> Own(ClientChallengeGlobals& challenge,const NativeMetaSailHooks& hooks) { const auto owner=challenge.FindParticipantBySubscriber(Need(hooks.subscriber)());if (!owner) throw std::logic_error("메타 항해의 Native NULL 참가자 물리 접근");return challenge.Participant(*owner); } //40칸 첫 subscriber 일치만 사용하고 active/owner0 거부를 추가하지 않는다.
}
//크기 보정·포구 환산·전투 원점 덧셈의 저장된 float32를 다음 단계가 다시 읽는다.
std::array<float,2> NativeMetaSeatPosition(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> participant,std::uint32_t battle,std::uint32_t seat,const RawCameraViewState& camera,const NativeMetaSeatHooks& hooks) {
    if (seat>=kOffset.size()) throw std::out_of_range("메타 좌석의 읽기 전용8쌍 물리 범위");
    const auto x=static_cast<float>((static_cast<long double>(std::bit_cast<std::int32_t>(Read(participant,0xc4)))/std::bit_cast<float>(camera.cellFloatBits[0]))*kScale[seat][0]+kOffset[seat][0]);
    const auto y=static_cast<float>((static_cast<long double>(std::bit_cast<std::int32_t>(Read(participant,0xc8)))/std::bit_cast<float>(camera.cellFloatBits[1]))*kScale[seat][1]+kOffset[seat][1]);
    const auto type=challenge.BattleType();const auto frame=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(Need(hooks.firstFrame)(type))+std::bit_cast<std::uint32_t>(kMetaBattleSeatOrder[seat]));const auto muzzle=Need(hooks.muzzle)(type,frame);
    const auto movedX=static_cast<float>(static_cast<long double>(muzzle[0])/std::bit_cast<float>(camera.baseCellFloatBits[0])+x),movedY=static_cast<float>(static_cast<long double>(muzzle[1])/std::bit_cast<float>(camera.baseCellFloatBits[1])+y);
    const auto record=challenge.Battle(battle);return {static_cast<float>(static_cast<long double>(std::bit_cast<float>(Read(record,0x10)))+movedX),static_cast<float>(static_cast<long double>(std::bit_cast<float>(Read(record,0x14)))+movedY)};
}
//원문은 자기 기록을 먼저 변경하므로 전송 실패여도 좌표와 후행 Rig 호출을 되돌리지 않는다.
void JoinNativeMetaBattle(ClientChallengeGlobals& challenge,std::uint32_t parameter,const NativeMetaSailHooks& hooks) {
    std::array<std::uint8_t,14> packet;Packet(packet);const auto seat=parameter&15U,battle=(parameter>>8)&255U;Put(packet,12,battle,1);Put(packet,13,seat,1);const auto own=Own(challenge,hooks);const auto point=Need(hooks.seat)(own,battle,seat);
    Put(own,0x9c,std::bit_cast<std::uint32_t>(point[0]));Put(own,0xa0,std::bit_cast<std::uint32_t>(point[1]));Put(packet,4,Read(own,0x9c));Put(packet,8,Read(own,0xa0));Need(hooks.send)(packet);Need(hooks.animate)(own);
}
//임의 위치 항해는 송신의 부분 효과 뒤에 현재 subscriber/CS를 다시 조회한다.
void SailNativeMetaParticipant(ClientChallengeGlobals& challenge,std::array<std::uint32_t,2> position,const NativeMetaSailHooks& hooks) {
    std::array<std::uint8_t,14> packet;Packet(packet);Put(packet,12,0,1);Put(packet,13,0,1);Put(packet,4,position[0]);Put(packet,8,position[1]);Need(hooks.send)(packet);const auto own=Own(challenge,hooks);Put(own,0x9c,position[0]);Put(own,0xa0,position[1]);Need(hooks.animate)(own);
}
bool IsNativeMetaSailCommand(std::uint32_t code) { return code==0x0200006bU || code==0x0200006cU || code==0x020000e3U || code==0x020000feU; } //메뉴가 큐에 넣는 원문 명령과 실행 소비자가 같은 네 값을 사용한다.
//경고 조건은 short circuit 순서로 읽고 일반/직접/억제 확인 입력을 구별한다.
bool DispatchNativeMetaSailInput(InputEvent event,const NativeMetaSailInputHooks& hooks) {
    const auto code=event.code&0x03ffffffU;if (!IsNativeMetaSailCommand(code)) return false;if (event.code&InputCode::kRelease) return true;
    const auto parameter=std::bit_cast<std::uint32_t>(event.y);
    if (code==0x0200006cU) { Need(hooks.sail)({std::bit_cast<std::uint32_t>(static_cast<float>(event.y>>16)),std::bit_cast<std::uint32_t>(static_cast<float>(parameter&65535U))}); }
    else if (code==0x0200006bU) { const bool warn=Need(hooks.passedDiags)()==0 && Need(hooks.noShowWarn)()==0 && Need(hooks.serverSelection)()!=-1;const auto battle=std::bit_cast<std::uint32_t>(event.y>>8);if (Need(hooks.freeSlots)(battle)==8 && warn) Need(hooks.tell)("WarnFailedDiags("+std::to_string(event.y)+")");else Need(hooks.join)(parameter); }
    else { if (code==0x020000feU) Need(hooks.suppress)();Need(hooks.join)(parameter); }return true;
}
}
