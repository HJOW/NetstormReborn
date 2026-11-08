// 원문 NULL/기존 CS 분기와 할당 성공 뒤 포인터 공개 순서를 같은 장기 owner에 연결한다.
#include "client/ClientChallengeGlobals.h"
#include <new>
#include <stdexcept>
#include <bit>

namespace netstorm::client { // 실제 같은 주소의 전역/할당만 제공하며 메타 상태 성공을 합성하지 않는다.
namespace { //CS조회/버전집계의실제필드폭과선택된진단만처리한다.
constexpr std::array<std::uint8_t,1> kEmptyParticipantName{0}; //00503c8b의빈C문자열에대응하는실제NUL수명이다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("CS 조회의 실제 필드 읽기 범위");std::uint32_t result{};for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[at+byte])<<(8*byte);return result; } //원문1/2/4BYTE를낮은BYTE부터조립한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("CS 버전의 실제 DWORD 쓰기 범위");for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //선택한현재DWORD의네BYTE만순서대로쓴다.
void Assert(const std::function<void(std::string_view,std::uint32_t)>& assertion,std::string_view expression,std::uint32_t line) { if (!assertion) throw std::logic_error("CS 원문 Ch.cpp 진단 장치 미연결");assertion(expression,line); } //진단이복귀하면원문처럼뒤의현재물리접근을계속한다.
}
std::uint32_t& ClientChallengeGlobals::Current() { return current_; } // 생성/옵션/Session 교체에서 다시0으로 쓰지 않는다.
const std::uint32_t& ClientChallengeGlobals::Current() const { return current_; } // 판정은 현재 같은 DWORD를 읽는다.
o::ChallengeGeometry& ClientChallengeGlobals::Geometry() { return geometry_; } // 별도 캐시 사본을 만들지 않는다.
const o::ChallengeGeometry& ClientChallengeGlobals::Geometry() const { return geometry_; } // 현재 같은 표를 읽는 const 소비자다.
MetaProjectionState& ClientChallengeGlobals::Projection() { return projection_; } //메타 표시/카메라 부모가 같은 물리를 변경한다.
const MetaProjectionState& ClientChallengeGlobals::Projection() const { return projection_; } //보고/표시는 보정이나 상태 변경 없이 현재값을 읽는다.
std::uint32_t& ClientChallengeGlobals::GeometryMode() { return geometryMode_; } //실제메타카메라/배치writer가같은DWORD를변경한다.
const std::uint32_t& ClientChallengeGlobals::GeometryMode() const { return geometryMode_; } //값의조회로공통좌표나CS기록을재생성하지않는다.
std::uint32_t& ClientChallengeGlobals::ZoneGlyphType() { return zoneGlyphType_; } //현재TYPE번호를그림/생성/기본프레임경계가공유한다.
const std::uint32_t& ClientChallengeGlobals::ZoneGlyphType() const { return zoneGlyphType_; } //조회시고정TYPE161로덮지않고현재값을보존한다.
std::uint32_t& ClientChallengeGlobals::ParticipantType() { return participantType_; } //현재참가자생성/메타카메라검색에같은DWORD를제공한다.
const std::uint32_t& ClientChallengeGlobals::ParticipantType() const { return participantType_; } //조회만으로현재TYPE를다시초기화하지않는다.
std::uint32_t& ClientChallengeGlobals::BattleType() { return battleType_; } //현재전투섬공장/프레임크기가같은TYPE를사용한다.
const std::uint32_t& ClientChallengeGlobals::BattleType() const { return battleType_; } //CS Reset/Session 교체후에도현재DWORD를그대로읽는다.
bool ClientChallengeGlobals::Prepare(const std::function<void()>& uninitialized) { // 객체 포인터 존재와 좌표 초기화 플래그를 별개로 확인한다.
    if (current_) {
        if (!geometry_.initialized) { if (!uninitialized) throw std::logic_error("기존 CS listsInitialized의 실제 진단 장치 미연결");const auto assertion=uninitialized;assertion(); }
        o::ResetChallengeState(Bytes());return true;
    }
    if (!nextToken_) throw std::overflow_error("CS 실제 할당 토큰 소진");
    std::unique_ptr<std::uint8_t[]> memory(new(std::nothrow) std::uint8_t[o::kChallengeBytes]); // 원문21328BYTE를 확보하고 성공한 뒤에만 생성자를 실행한다.
    if (!memory) { current_=0;return false; }
    o::ConstructChallengeState({memory.get(),o::kChallengeBytes},geometry_);const auto token=nextToken_;
    allocations_.emplace(token,std::move(memory));++nextToken_;current_=token;return true;
}
std::span<std::uint8_t> ClientChallengeGlobals::Bytes() { // 현재 포인터를 매 소비 위치에서 재조회한다.
    const auto found=allocations_.find(current_);if (!current_ || found==allocations_.end()) throw std::logic_error("현재 CS의 실제 할당 물리 미등록");return {found->second.get(),o::kChallengeBytes};
}
std::span<const std::uint8_t> ClientChallengeGlobals::Bytes() const { // mutable 부모와 같은 실제 할당을 읽는다.
    return const_cast<ClientChallengeGlobals*>(this)->Bytes();
}
std::span<std::uint8_t> ClientChallengeGlobals::Battle(std::uint32_t index) { // 유효/참가 조건은 원문 caller가 검사하며 물리10개 상한만 확인한다.
    if (index>=o::kChallengeBattleCount) throw std::out_of_range("CS 전투10기록 물리 범위");return Bytes().subspan(o::kChallengeBattleOffset+index*o::kChallengeBattleBytes,o::kChallengeBattleBytes);
}
std::span<std::uint8_t> ClientChallengeGlobals::Participant(std::uint32_t index) { // 물리40개와 각 기록의 실제 폭만 제한한다.
    if (index>=o::kChallengeParticipantCount) throw std::out_of_range("CS 참가자40기록 물리 범위");return Bytes().subspan(o::kChallengeParticipantOffset+index*o::kChallengeParticipantBytes,o::kChallengeParticipantBytes);
}
// 기존가변getter와동일한현재할당/범위만빌린다.
std::span<const std::uint8_t> ClientChallengeGlobals::Battle(std::uint32_t index) const { return const_cast<ClientChallengeGlobals*>(this)->Battle(index); }
// const조회도같은CS포인터를다시읽으며별도스냅샷을만들지않는다.
std::span<const std::uint8_t> ClientChallengeGlobals::Participant(std::uint32_t index) const { return const_cast<ClientChallengeGlobals*>(this)->Participant(index); }
// 유효성비트나subscriber0은검색조건이아니며처음일치한실제0번도반환한다.
std::optional<std::uint32_t> ClientChallengeGlobals::FindParticipantBySubscriber(std::uint32_t subscriber) const {
    for (std::uint32_t index=0;index<o::kChallengeParticipantCount;++index) if (Read(Participant(index),4)==subscriber) return index; //원문40개의현재subscriberDWORD를배열순서로비교한다.
    return std::nullopt;
}
// 원문은이름내용을읽지않고포인터만돌려준다.호스트도현재고정이름물리뷰만제공한다.
std::span<const std::uint8_t> ClientChallengeGlobals::ParticipantNameBytes(std::int32_t participant,const std::function<void(std::string_view,std::uint32_t)>& assertion) const {
    if (participant<1 || participant>=40) Assert(assertion,"pId > 0 && pId < CL_NUM_CHAL_PLAYERS",0x48c);const auto record=Participant(std::bit_cast<std::uint32_t>(participant));return (Read(record,0x64,1)&1U) ? record.subspan(0x26,40):std::span<const std::uint8_t>(kEmptyParticipantName);
}
// 무효/비활성전투의추가관문없이같은현재여덟슬롯을센다.
std::uint32_t ClientChallengeGlobals::FreeBattleSlots(std::uint32_t battle) const {
    const auto record=Battle(battle);std::uint32_t count{};for (std::uint32_t slot=0;slot<8;++slot) if (!Read(record,0x38U+slot*4U)) ++count;return count; //각playerId DWORD가정확히0일때만증가한다.
}
// Native루프의slot0..7순서/참가자active필터없음·WORD상한후하한조회와최종두쓰기순서를유지한다.
void ClientChallengeGlobals::UpdateBattleVersionBounds(std::int32_t battle,const std::function<void(std::string_view,std::uint32_t)>& assertion) {
    if (battle<0 || battle>=10) { Assert(assertion,"bId >= 0 && bId < CL_NUM_CHAL_BATTLES",0x45a);throw std::out_of_range("CS 버전 집계의 실제 전투10칸 범위"); } //원문 진단은 currentCS 물리 조회보다 앞이며 복귀해도 잘못된 배열 주소를 사용하지 않는다.
    UpdateBattleVersionBounds(Bytes(),battle,assertion);
}
// 원문의 this 기록은 진단/표시 callback이 current를 바꿔도 같은 물리를 유지한다.
void ClientChallengeGlobals::UpdateBattleVersionBounds(std::span<std::uint8_t> cs,std::int32_t battle,const std::function<void(std::string_view,std::uint32_t)>& assertion) {
    if (battle<0 || battle>=10) Assert(assertion,"bId >= 0 && bId < CL_NUM_CHAL_BATTLES",0x45a);if (cs.size()!=o::kChallengeBytes || battle<0 || battle>=10) throw std::out_of_range("CS 버전 집계의 실제 캡처 기록 범위");const auto record=cs.subspan(o::kChallengeBattleOffset+std::size_t(battle)*o::kChallengeBattleBytes,o::kChallengeBattleBytes);if (!(Read(record,0,1)&1U)) Assert(assertion,"b->isValid()",0x45d);std::uint32_t maximum=0xffffffffU,minimum=0x100U;
    for (std::uint32_t slot=0;slot<8;++slot) { //원문의두묶음네슬롯을동일한현재순서로순회한다.
        const auto participant=Read(record,0x38U+slot*4U);if (!participant) continue;if (participant>=o::kChallengeParticipantCount) throw std::out_of_range("CS 버전 집계의 실제 참가자40칸 범위");const auto peer=cs.subspan(o::kChallengeParticipantOffset+participant*o::kChallengeParticipantBytes,o::kChallengeParticipantBytes);const auto upper=Read(peer,0xb4,2);if (std::bit_cast<std::int32_t>(maximum)<=static_cast<std::int32_t>(upper)) maximum=upper;const auto lower=Read(peer,0xb2,2);if (lower<=minimum) minimum=lower;
    }
    Put(record,0x1c,minimum);Put(record,0x18,maximum);
}
// 실제 할당 성공 뒤만 토큰을 발행하고 아직 초기화하지 않은 원소는 caller의 native 쓰기에 맡긴다.
std::uint32_t ClientChallengeGlobals::AllocateArray(std::size_t bytes) {
    if (!nextArrayToken_) throw std::overflow_error("CS 가변 배열 토큰 소진");std::unique_ptr<std::uint8_t[]> value(new(std::nothrow) std::uint8_t[bytes]);if (!value) return 0;
    const auto token=nextArrayToken_;arrays_.emplace(token,ArrayAllocation{std::move(value),bytes});++nextArrayToken_;return token;
}
std::span<std::uint8_t> ClientChallengeGlobals::ArrayBytes(std::uint32_t token) { // 같은 원문 포인터를 조회하며없는 할당을 빈 배열로 합성하지 않는다.
    const auto found=arrays_.find(token);if (!token || found==arrays_.end()) throw std::logic_error("CS 가변 배열의 실제 할당 미등록");return {found->second.bytes.get(),found->second.size};
}
void ClientChallengeGlobals::ClearArray(std::span<std::uint8_t> header) { // 원문 free 실패 경계에서 아직 쓰지 않은 헤더/둘째 포인터를 보존한다.
    if (header.size()<16) throw std::out_of_range("CS 가변 배열의 실제16BYTE 헤더 부족");
    // 두 포인터는 순서대로 다시 읽으며 NULL일 때만 free를 생략한다.
    for (const auto at:{0U,4U}) {
        std::uint32_t token{};
        // 비정렬 DWORD를 낮은 BYTE부터 읽는다.
        for (std::size_t byte=0;byte<4;++byte) token|=std::uint32_t(header[at+byte])<<(byte*8);
        if (token && arrays_.erase(token)!=1) throw std::logic_error("CS 가변 배열 free의 실제 할당 미등록");
    }
    // 원문은 두 free가 끝난 뒤에만 네 DWORD를 이 순서로 쓴다.
    for (const auto at:{0U,4U,12U,8U}) {
        // 선택된 DWORD의 네 BYTE만 지워 인접 헤더를 보존한다.
        for (std::size_t byte=0;byte<4;++byte) header[at+byte]=0;
    }
}
}
