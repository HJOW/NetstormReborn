// 원문 NULL/기존 CS 분기와 할당 성공 뒤 포인터 공개 순서를 같은 장기 owner에 연결한다.
#include "client/ClientChallengeGlobals.h"
#include <new>
#include <stdexcept>

namespace netstorm::client { // 실제 같은 주소의 전역/할당만 제공하며 메타 상태 성공을 합성하지 않는다.
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
