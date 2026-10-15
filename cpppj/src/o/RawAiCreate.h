// PATCH00417e50의 실제 AI 생성/기존 소유자/NoTemple 순서를 엄격한 생성자 경계로 복원한다.
#pragma once
#include "o/RawMissionAiSettings.h"
#include "o/RawPlayerParticipation.h"

namespace netstorm::o {
// 실제 색/섬/조각 소유자와 아직 큰 생성자·초기기술 갱신의 명시 경계다.
struct AiCreateHooks {
    std::function<void(std::uint32_t,std::uint32_t)> changeColor; // 0043ca70이며 기존 brain 검사보다 먼저다.
    std::function<std::uint32_t(std::uint32_t)> firstRegion; // 현재 Player+78의 실제 DWORD다.
    std::function<std::uint32_t(std::uint32_t)> islandTemple; // 0046f260의 현재 지역 신전 SID다.
    std::function<void(std::uint32_t,std::uint32_t)> destroyTemple; // 실제 신전가상삭제(SID,0x400000)다.
    std::function<std::uint32_t(std::uint32_t,std::span<const std::uint8_t>,std::size_t)> construct; // 실제 new(0xb08)→00416860이며 반환0은 안전한 호출 계약에서 거부한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> initialUpdate; // 00414710의 실제 기술/분류 초기화 전체다.
};
// 이 wrapper는 brain을 합성하지 않는다. 생성자와 initialUpdate가 없으면 flags20을 켜지 않는다.
class RawAiCreate {
public:
    // 같은 현재 Player 기록의 참여/수명 처리기와 실제 효과 공급자를 빌린다.
    RawAiCreate(OriginalEdition edition,RawPlayerParticipation& players,AiCreateHooks hooks);
    // Color→기존brain→NoTemple→실제생성→부착→초기갱신 순이다. 기존brain에는 갱신을 재호출하지 않는다.
    std::uint32_t Create(std::uint32_t owner,std::span<const std::uint8_t> settings) const;
private:
    RawPlayerParticipation& players_; // 실제Player+b0/flags20과 수명 계약을 공유한다.
    AiCreateHooks hooks_; // 실제 생성자가 미복원인 현재 큰 몸체는 반드시 명시 공급해야 한다.
};
}
