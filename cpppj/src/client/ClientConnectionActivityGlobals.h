// Client 수명의 두 활동 표와 그 사이의 실제 Zone DWORD를 별개 필드로 보관한다.
#pragma once
#include "o/RawConnectionActivity.h"
#include <array>

namespace netstorm::client { // Session/연결 교체보다 오래 사는 실제 현재 주소의 소유 계층이다.
// 같은 실제22프레임/22벽시각 표와 그 사이 Zone 두DWORD의 주소를 고정한다.
class ClientConnectionActivityGlobals {
public:
    ClientConnectionActivityGlobals()=default; // 새 Client의 최초 BSS0만 만들며 Session 대여/반납 때 다시 초기화하지 않는다.
    ClientConnectionActivityGlobals(const ClientConnectionActivityGlobals&)=delete; // 현재 표와 그 사이 Zone 참조를 복제하지 않는다.
    ClientConnectionActivityGlobals& operator=(const ClientConnectionActivityGlobals&)=delete; // 살아 있는 같은 Native 주소를 다른 값형 owner로 교체하지 않는다.
    ClientConnectionActivityGlobals(ClientConnectionActivityGlobals&&)=delete; // 소비자가 빌린 현재 표 주소를 이동하지 않는다.
    ClientConnectionActivityGlobals& operator=(ClientConnectionActivityGlobals&&)=delete; // 연결된 모듈의 주소 수명을 유지한다.
    o::ConnectionActivityTables Tables() { return {frames_,wallSeconds_}; } // 간극을 포함하지 않는 두 고정 span을 실제 수신 몸체에 빌린다.
    std::array<std::uint32_t,o::kConnectionActivityEntries>& Frames() { return frames_; } //005c84e8..005c853c이며 cleanup은 이22DWORD만 지운다.
    const std::array<std::uint32_t,o::kConnectionActivityEntries>& Frames() const { return frames_; } // 실제 마지막 수신 프레임의 현재값을 관측한다.
    std::array<std::uint32_t,o::kConnectionActivityEntries>& WallSeconds() { return wallSeconds_; } //005c8548..005c859c이며 이후 reader는 각 DWORD를 signed32로 해석한다.
    const std::array<std::uint32_t,o::kConnectionActivityEntries>& WallSeconds() const { return wallSeconds_; } // frame cleanup이나 Session 반납으로 암묵0으로 만들지 않는다.
    std::uint32_t& ActiveStamp() { return activeStamp_; } //005c8540의 Zone 설치 stamp이며 frame표의23번째 항목이 아니다.
    const std::uint32_t& ActiveStamp() const { return activeStamp_; } //005c85e4의 pendingStamp 원천과도 별개다.
    std::uint32_t& ActiveZoneWord() { return activeZoneWord_; } //005c8544의 Zone+10 복사 출력이며 wall표의음수항목이 아니다.
    const std::uint32_t& ActiveZoneWord() const { return activeZoneWord_; } // 같은 실제 Zone 소비자가 빌릴 현재 DWORD다.
private:
    std::array<std::uint32_t,o::kConnectionActivityEntries> frames_{}; //005c84e8부터88BYTE이며 최초 BSS0이다.
    std::uint32_t activeStamp_{},activeZoneWord_{}; //005c8540/544의 별도 두 DWORD이며 표 clear에 포함하지 않는다.
    std::array<std::uint32_t,o::kConnectionActivityEntries> wallSeconds_{}; //005c8548부터88BYTE이며 바로 뒤005c85a0 CentralCreated를 포함하지 않는다.
};
}
