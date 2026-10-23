// Client 수명의 현재 CoreData 포인터 슬롯·활성 DWORD·lastKnown10BYTE와 현재 Player뷰를 소유한다.
#pragma once
#include "o/RawPlayerCoreData.h"
#include "o/RawPlayerWorld.h"
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::client {
// Player 원문 기록 자체는 외부 실제 소유자에서 빌리고 현재/캐시 저장소만 장기 소유한다.
class ClientCoreDataGlobals {
public:
    ClientCoreDataGlobals()=default; // 새 Client에서만 현재NULL/active0/CRT후cached10zero를 한 번 구성한다.
    ClientCoreDataGlobals(const ClientCoreDataGlobals&)=delete; // current가 자기 cached를 가리킬 수 있으므로 주소를 복제하지 않는다.
    ClientCoreDataGlobals& operator=(const ClientCoreDataGlobals&)=delete; // 기존 alias를 대입으로 끊지 않는다.
    ClientCoreDataGlobals(ClientCoreDataGlobals&&)=delete; // 캐시와 현재 슬롯을 고정 주소에 유지한다.
    ClientCoreDataGlobals& operator=(ClientCoreDataGlobals&&)=delete; // 살아 있는 소비자 참조의 수명을 보존한다.
    std::span<std::uint8_t>& Current(); //00595350에 해당하는 실제 가변 span 슬롯이다.
    const std::span<std::uint8_t>& Current() const; // 같은 현재 포인터 역할을 읽으며 사본 버퍼를 만들지 않는다.
    std::uint32_t& Active(); //00595348의 실제 한 DWORD다.
    const std::uint32_t& Active() const; // 실제 active의 같은 주소다.
    std::span<std::uint8_t> Cached(); //005953f0 lastKnownCoreData의 같은10BYTE다.
    std::span<const std::uint8_t> Cached() const; // 실제 캐시의 읽기뷰다.
    std::span<std::uint8_t>& Players(); //0059534c의 현재 실제 Player 물리뷰 슬롯이며 할당 자체를 소유하지 않는다.
    const std::span<std::uint8_t>& Players() const; // 새 base 선택도 같은 슬롯을 통해 관측한다.
    o::PlayerCoreDataGlobals Borrow(o::PlayerWorldState& player,const std::uint32_t& metamap); //실제 두owner/known/모드를 같은 Rawhelper에 빌린다.
    std::span<std::uint8_t> Read(o::PlayerWorldState& player,const std::uint32_t& metamap,const o::PlayerCoreDataHooks& hooks); //0048f0d0 실제진단/현재재조회를 직접 실행한다.
    void Select(o::PlayerWorldState& player,const std::uint32_t& metamap,std::uint32_t owner,std::uint32_t templateOwner); //0048f3b0 실제선택/4·4·2정리를 실행한다.
private:
    std::span<std::uint8_t> current_; // 최초NULL이며 Session 생성/파괴 때 임의 reset하지 않는다.
    std::uint32_t active_{}; // 원시00595348의 최초0이다.
    std::array<std::uint8_t,10> cached_{}; //원시0에CRT004feaf0→42ce70이0쓰는같은10BYTE를Client최초한번만구성한다.
    std::span<std::uint8_t> players_; // 실제 외부Player해제 전에 원문정리/재선택을 마쳐야 하는 빌린 현재base다.
};
}
