// 원문 범위 진단/조기 반환과 실제 이탈 뒤 이미지 반납·CS 초기화의 부분 효과를 유지한다.
#include "client/NativeMetaParticipantRemove.h"
#include <bit>
#include <stdexcept>
#include <string>

namespace netstorm::client { //실제 World/Kernel/Chat owner는 선택된 callback의 호출 동안만 빌린다.
namespace { //현재 기록/packet의 지정 폭만 읽고 쓰는 비정렬 물리 경계다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("메타 참가자 제거의 실제 장치 미연결");return hook; } //미선택 자식은 앞에서 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("참가자 제거의 실제 읽기 물리 부족");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //같은 BYTE/WORD/DWORD를 낮은 BYTE부터 조립한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("참가자 제거의 실제 쓰기 물리 부족");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //도달한 폭만 기록하고 선행 효과를 되돌리지 않는다.
std::span<std::uint8_t> Record(std::span<std::uint8_t> cs,std::uint32_t id,bool battle) { const auto count=battle ? o::kChallengeBattleCount:o::kChallengeParticipantCount,offset=battle ? o::kChallengeBattleOffset:o::kChallengeParticipantOffset,stride=battle ? o::kChallengeBattleBytes:o::kChallengeParticipantBytes;if (cs.size()!=o::kChallengeBytes || id>=count) throw std::out_of_range("참가자 제거의 실제 캡처 CS 기록 범위");return cs.subspan(offset+id*stride,stride); } //current가 바뀌어도 같은 this CS의 실제 물리를 유지한다.
}
// 수신 관찰은 필드를 읽지 않고 송신 진단에서 복귀한 뒤 현재 개수를 다시 읽는다.
void ObserveNativeMetaParticipantRemove(std::span<std::uint8_t> packet,bool sending,const std::function<void(std::string_view,std::uint32_t)>& assertion) {
    if (!sending) return;if (Read(packet,4,1)==0xffU) Need(assertion)("numPlayers != (unsigned char)-1",0x586);Put(packet,2,Read(packet,4,1)+5U,2);
}
// 원문42b980의 미사용 추가 인자1은 수명 정책으로 재해석하지 않는다.
void DestroyNativeMetaParticipant(std::span<std::uint8_t> record,ClientChallengeGlobals& challenge,const NativeMetaParticipantRemoveHooks& h) {
    const bool downward=(Read(record,0x64,1)&0xa0U)==0;Need(h.leave)(Read(record,0xb8),downward);challenge.ClearArray(record.subspan(0xbc,16));
    for (std::size_t at=0;at<o::kChallengeParticipantBytes;at+=4) Put(record,at,0); //원문의115DWORD 전체를 같은 순서로0으로 쓰며 shapeSize도 마지막 기록 초기화에 포함한다.
}
// checking은 범위가 유효할 때만 읽고 진단 복귀 뒤 현재 active를 다시 검사한다.
void RemoveNativeMetaParticipant(std::span<std::uint8_t> cs,std::int32_t participant,ClientChallengeGlobals& challenge,const NativeMetaParticipantRemoveHooks& h) {
    if (participant<1 || participant>39) { Need(h.assertion)("0 < pId && pId < CL_NUM_CHAL_PLAYERS","\\Ns\\O\\Ch.cpp",0x499);Need(h.log)("Cannot delete illegal player number "+std::to_string(participant)+"\n");return; }
    const auto record=Record(cs,static_cast<std::uint32_t>(participant),false);if (h.checking && !(Read(record,0x64,1)&1U)) Need(h.assertion)("p->isValid()","\\Ns\\O\\Ch.cpp",0x4a1);
    if (!(Read(record,0x64,1)&1U)) { Need(h.log)("Cannot delete invalid player number "+std::to_string(participant)+"\n");return; }
    const auto battle=Read(record,0xa4);if (battle) { const auto b=Record(cs,battle,true);const auto slot=Read(record,0xa8);if (slot>=8) throw std::out_of_range("참가자 제거의 실제 좌석8칸 범위");Put(b,0x38+slot*4,0);Need(h.battle)(Read(b,4));challenge.UpdateBattleVersionBounds(cs,std::bit_cast<std::int32_t>(Read(record,0xa4)),[&h](auto expression,auto line) { Need(h.assertion)(expression,"\\Ns\\O\\Ch.cpp",line); }); }
    Need(h.chat)(Read(record,4));DestroyNativeMetaParticipant(record,challenge,h);
}
// packet 개수는 각 하위 호출 뒤 재조회하며0명인 패킷도 마지막 NSInfo를 갱신한다.
void ReceiveNativeMetaParticipantRemove(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaParticipantRemoveHooks& h) {
    const auto cs=challenge.Bytes();std::uint32_t index{};
    while (index<Read(packet,4,1)) { RemoveNativeMetaParticipant(cs,static_cast<std::int32_t>(Read(packet,5+index,1)),challenge,h);++index; } //진입CS와 현재packet 요소를 구별하고 새 active/known/송신자 관문을 넣지 않는다.
    Need(h.refreshInfo)();
}
}
