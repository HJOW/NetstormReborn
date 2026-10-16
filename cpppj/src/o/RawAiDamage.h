// PATCH AI 피해 반응의 현재 목록과 조건부 추가·로그 순서를 복원한다.
#pragma once
#include "o/RawAiBrain.h"

namespace netstorm::o {
// 실제004105b0 호출의 서식 인수다. 로그 출력 여부는 현재 brain+b00을 읽는 소비자가 결정한다.
struct AiDamageLog {
    bool more{}; // false는 I hate, true는 I hate MORE 서식이다.
    Sid attacker{}; // 이번 로그의 실제 공격자 또는 홈 SID다.
    std::uint32_t type{}; // 좌표 절삭 뒤 현재 공격자의 TYPE 번호다.
    std::int32_t x{},y{}; // 원본 CRT가0방향으로 절삭한 좌표다.
};
// 실제 큰 하위 둘만 외부 경계로 받는다. 목록 판단은 본체가 수행한다.
struct AiDamageHooks {
    std::function<Sid(Sid)> home; //0045c100의 같은 Host/FlyerProcessFields::Home 조회다.
    std::function<void(std::uint32_t,const AiDamageLog&)> log; //004105b0(ai,29,format,typeName,x,y)의 실제 소비자다.
};
// 같은 실제 AI 소유 블록/Pool/현재 TYPE에00415460을 적용한다.
class RawAiDamage {
public:
    // metadata와 memory/pool은 처리기보다 오래 유지해야 한다. PATCH188만 받는다.
    RawAiDamage(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,AiDamageHooks hooks);
    // 실제 Player+0xb0의 opaque32 token으로 같은 brain과 목록을 변경한다.
    void Notice(std::uint32_t token,Sid victim,Sid attacker) const;
    // 같은 실제 월드에 연결하는지 검사할 수 있는 풀 참조다.
    const SidPool& Pool() const;
private:
    // 현재 원문 크기를 확인하며 padding을 초기화하지 않는다.
    std::span<std::uint8_t> Brain(std::uint32_t token) const;
    // 실제 list{pointer,capacity,count}의 현재 내용을 검색한다.
    bool Contains(std::uint32_t token,std::size_t offset,Sid sid) const;
    //00414450의 capacity 우선/중복 방지 추가다.
    void Append(std::uint32_t token,std::size_t offset,Sid sid) const;
    //00411200은 full이어도 로그를 호출하며0/중복에서만 반환한다.
    void Hate(std::uint32_t token,Sid sid) const;
    // 실제 로그 인수를 현재 raw에서 읽는다. 호출 뒤에는 저장된 포인터를 재사용하지 않는다.
    void Log(std::uint32_t token,Sid sid,bool more) const;
    // 현재 TYPE의 genus를 읽는다. free/dead는 호출 지점의 별도 조건이다.
    std::uint32_t Genus(Sid sid) const;
    // 홈의 범위/free/group3 조건이며 dead 비트는 거부하지 않는다.
    bool ValidHome(Sid sid) const;
    RawAiBrainMemory& memory_; // 실제 본체와 네 목록 블록의 유일한 소유자다.
    const SidPool& pool_; // 실제 피해/공격자/홈 raw 슬롯이다.
    std::span<const RiftTypeRecord> types_; // 같은 현재188개 TYPE다.
    AiDamageHooks hooks_; // 실제 홈/로그 하위다.
};
}
