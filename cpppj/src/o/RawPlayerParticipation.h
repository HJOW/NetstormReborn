// PATCH의 실제 로그인·방향별 동맹·AI 부착/해제에서 Player 기록의 쓰기와 효과 순서를 복원한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <span>

namespace netstorm::o {
// 원본의 진단 후 계속 진행하는 두 assertion 경계다.
enum class PlayerParticipationAssertion {
    BrainAlreadyAssigned, // 기존brain이0이어야하는004911a0의진단이다.
    MetamapMode // Player번호getter/Chat방문에서metamap아님을확인하는진단이다.
};
// 각 함수가 실제 도달 시 읽는 mutable 전역이다. Ready와 사람 연결/AI 참여를 같은 bool로 합치지 않는다.
struct PlayerParticipationState {
    std::uint32_t localOwner{1},playerKnown{}; // 00540c70/00595344의 현재 DWORD다.
    bool battle{},metamap{},localTransport{}; // 00594fbc/00594fc8/0054db50의 서로 다른 모드다.
};
// 실제 세션의 다른 모듈이 소유한 현재 전역을 각 원본 읽기 위치에 연결한다.
struct PlayerParticipationReads {
    std::function<std::uint32_t()> localOwner,playerKnown; // 같은 PlayerWorld의 현재 owner/known DWORD다.
    std::function<bool()> battle,metamap,localTransport; // 같은 월드와 전송 계층의 현재 서로 다른 모드다.
};
// 실제 전송/Chat UI와 실제 AI 소유자의 파괴·반납 경계다. 가짜 brain 생성/등록은 제공하지 않는다.
struct PlayerParticipationHooks {
    std::function<void(PlayerParticipationAssertion)> assertion; // 원본 assert 보고 후 계속 수행하는 진단이다.
    std::function<void(std::int32_t,std::int32_t)> assignConnection; // 004e27b0(pending,owner)의 실제 연결 번호 재배치다.
    std::function<void(std::int32_t)> rejectLogin,disconnect; // 004a6920(pending,0)와004e2940(pending)의 순서다.
    std::function<void(std::uint32_t)> clearChat; // 00445080(0x40/0x80)의 실제 채널 초기화다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t)> addChat; // 00444ee0(sub,name,0,0,-1,channel,-1,0)의 고정 나머지 인자를 갖는 실제 등록이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> sendAlliance; // 004e2dc0(9,playerBYTE+maskDWORD)의 논리 packet이다.
    std::function<void(std::uint32_t)> destructBrain,freeBrain; // 00410100(brain) 뒤 CRT free(brain)이며 같은 실제 소유자를 사용한다.
};
// 9×0xb4 Player와 원본9×9 DWORD 표를 참조한다. 배열 사본/대칭 동맹/추정 active 입력을 만들지 않는다.
class RawPlayerParticipation {
public:
    // 이 첫 구현은 PATCH전용이다. 실제 저장소/모드/효과 대상은 객체보다 오래 살아야 한다.
    RawPlayerParticipation(OriginalEdition edition,std::span<std::uint8_t> players,std::span<std::uint32_t> allies,
        std::span<const std::uint8_t> hidden,PlayerParticipationState& state,PlayerParticipationHooks hooks);
    // 다섯 현재 조회를 모두 공급한다. 독립 원본 대조의 기존 값형 state는 미연결일 때만 사용한다.
    void BindCurrentInputs(PlayerParticipationReads reads);
    // 004a6dc0: 유효 owner1..8의 bit1 부재에서만 flags=(flags&~100)|3 뒤 연결을 수락한다.
    // 이미 로그인/무효 owner는 거부 응답→연결 해제를 수행한다. Ready40이나 AI20은 이 분기와 별개다.
    bool CheckLogin(std::int32_t pending,std::int32_t owner) const;
    // 00490ea0: 자기 bit를 포함한+54 mask/현재 한행9칸만 쓰고, playerKnown이면 양방향 Chat 표시를 재구성한다.
    void SetAlliance(std::uint32_t owner,std::uint32_t mask) const;
    // 0048f6a0: 지정 방향의 한 bit/한 cell만 켜며 battle이면 현재 mask를 서버9에 요청한다.
    void AddAlly(std::uint32_t owner,std::uint32_t target) const;
    // 004911a0: 실제 생성 소유자가 전달한 opaque32 brain을 +b0에 기록하고 flags20을 켠다.
    // 기존 brain은 assert만 보고 덮어쓰며 임의로 해제하지 않는다. 0 입력도 원본 필드쓰기와 동일하다.
    void AttachBrain(std::uint32_t owner,std::uint32_t brain) const;
    // 00413ae0/00413b20: 실제 파괴→반납→+b0=0이다. 원본대로 flags20은 지우지 않는다.
    void RemoveBrain(std::uint32_t owner) const;
    void RemoveAllBrains() const;
    // 다른 실제 AI 소비자가 같은 현재 토큰을 조회한다. 이 클래스는 brain 메모리를 소유하지 않는다.
    std::uint32_t Brain(std::uint32_t owner) const;
private:
    // 실제 0..8 Player와 little endian DWORD를 조회/기록한다.
    std::span<std::uint8_t> Player(std::uint32_t owner) const;
    std::uint32_t Read(std::uint32_t owner,std::size_t offset) const;
    void Put(std::uint32_t owner,std::size_t offset,std::uint32_t value) const;
    // 원본 Player+0xc NUL 문자열을 현재 채널 등록 직전에 읽는다.
    std::string_view Name(std::uint32_t owner) const;
    // 효과 callback 뒤에도 같은 원본 전역의 현재 값을 다시 읽는다.
    std::uint32_t LocalOwner() const;
    std::uint32_t PlayerKnown() const;
    bool Battle() const;
    bool Metamap() const;
    bool LocalTransport() const;
    std::span<std::uint8_t> players_; // 같은 실제9 Player 기록이다.
    std::span<std::uint32_t> allies_; // 한쪽 방향만 바뀔 수 있는 원본81 DWORD다.
    std::span<const std::uint8_t> hidden_; // 0052f81e의 현재9개 숨김 표시다.
    PlayerParticipationState& state_; // 효과 callback 이후에도 다시 읽는 현재 전역이다.
    PlayerParticipationHooks hooks_; // 실제 전송·Chat·생성 소유자 수명 효과다.
    PlayerParticipationReads reads_; // 실제 세션이 바인딩한 현재 전역 getter이며 값형 fixture와 구분한다.
};
}
