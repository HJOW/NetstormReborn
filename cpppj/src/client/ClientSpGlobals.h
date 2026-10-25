// 원문 공통 시작의 SP/추적 DWORD를 메뉴·Session·사제보다 오래 같은 주소에 소유한다.
#pragma once
#include "client/ClientRandomGlobals.h"
#include "o/RawGameRandomSeed.h"
#include <memory>

namespace netstorm::client { // 동일 Client의 시간 입력/게임·CRT 난수와 실제 SP를 조립한다.
std::int32_t ClientCurrentUnixSeconds(); // 실제 time32 지원 범위를 검사하고 현재 시스템 초를 한 번 읽는다.
class ClientSpGlobals {
public:
    ClientSpGlobals(std::shared_ptr<ClientRandomGlobals> random,std::function<std::int32_t()> unixSeconds,std::function<void(std::span<std::uint8_t>)> allocation={}); // 생성자는 주소만 연결한다. 원문004391ed에서 명시 Initialize해야 한다.
    ClientSpGlobals(const ClientSpGlobals&)=delete;ClientSpGlobals& operator=(const ClientSpGlobals&)=delete; // 같은 SP/추적 전역과 callback의 this 주소를 복제하지 않는다.
    ClientSpGlobals(ClientSpGlobals&&)=delete;ClientSpGlobals& operator=(ClientSpGlobals&&)=delete; // 모든 대여 소비자가 반납할 때까지 고정 주소를 유지한다.
    o::ScrambledSpStore& Store();const o::ScrambledSpStore& Store() const; // 같은32×9DWORD/offset/순열/초기화 상태의 유일한 저장소다.
    const std::shared_ptr<ClientRandomGlobals>& RandomGlobals() const; // Session이 다른 난수와 SP를 섞지 않도록 실제 소유자 정체성을 읽는다.
private:
    std::shared_ptr<ClientRandomGlobals> random_; // seed/SP보다 먼저 만들고 뒤에 반납하는 같은 장기 게임·CRT 상태다.
    std::int32_t tracking_{}; //0054d4e0의 최초 BSS0이며 Session 생성/소멸에서 초기화하지 않는다.
    o::RawGameRandomSeed seed_; // 실제 같은 time32/CRT/게임 난수/모드에 재시드한다.
    o::ScrambledSpStore store_; // 초기화 전 NULL·32실제 행·offset·순열을 같은 Client 수명에 유지한다.
};
}
