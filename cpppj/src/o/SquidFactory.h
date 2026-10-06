// 원본 Squid.cpp의 base 생성/Take와 가상 초기화를 raw SID 풀에 연결한다.
#pragma once
#include "o/SidPool.h"

namespace netstorm::o {
class SquidFactory {
public:
    // 타입 입력은 복사하여 호출자 수명과 분리한다. 약화 mana 옵션은 원본 HP /4 분기다.
    SquidFactory(SidPool& pool,std::span<const RiftTypeRecord> types,bool weakenedMana=false);
    // 004af530 ↔ CD 004ab390의 constructor=0 경로. 파생 생성자는 할당 전에 거부한다.
    Sid Create(std::uint32_t type,std::uint32_t flags=0);
    // 004af610 ↔ CD 004ab440의 free 또는 이미 void인 base 객체 수신 경로다.
    // 원본처럼 payload·free list·freeCount를 보존한다. non-void 공간 해제는 후속이다.
    Sid Take(std::uint32_t type,Sid sid);
    // 004b0c60 ↔ CD 004ae040. 파생 클래스가 base 초기화를 호출할 때도 사용한다.
    void PostCreate(Sid sid);
    // 004ad4a0 ↔ CD 004ae120. zOrder만 갱신하며 owner·HP는 보존한다.
    void PostTake(Sid sid);
private:
    // 주소 산술 전에 타입/상태와 base fallback 지원 여부를 검증한다.
    const RiftTypeRecord& Type(std::uint32_t number,bool requireBase) const;
    std::span<std::uint8_t> LiveBytes(Sid sid);
    // 원본 32비트 vtable 값은 기록할 뿐 호스트에서 역참조하지 않는다.
    std::uint32_t BaseVtable() const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    bool weakenedMana_{};
};
}
