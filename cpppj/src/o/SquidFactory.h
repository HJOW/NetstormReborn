// 원본 Squid.cpp의 자산 생성/void Take와 공통 가상 초기화를 raw SID 풀에 연결한다.
#pragma once
#include "o/SidPool.h"

namespace netstorm::o {
class SquidUnpop;
class SquidFactory {
public:
    // 타입 입력은 복사하여 호출자 수명과 분리한다. 약화 mana 옵션은 원본 HP /4 분기다.
    SquidFactory(SidPool& pool,std::span<const RiftTypeRecord> types,bool weakenedMana=false,SquidUnpop* unpop=nullptr);
    // 004af530 ↔ CD 004ab390. 복원된 자산 생성자 또는 constructor=0 fallback을 호출한다.
    Sid Create(std::uint32_t type,std::uint32_t flags=0);
    // 이미 확보된 슬롯에 생성자 쓰기만 적용한다. type/상태/풀 목록은 덮지 않는다.
    Sid Construct(std::uint32_t type,Sid sid);
    // 004af610 ↔ CD 004ab440의 지원 자산 수신 경로다. free list/freeCount는 변경하지 않는다.
    // free/void와 선택적인 SquidUnpop 연결의 non-void 공간 경로를 처리한다.
    Sid Take(std::uint32_t type,Sid sid);
    // 004ad470 ↔ CD 004ae0f0. abstract/buried 비트에 따른 공통 최초 Pop 플래그를 읽는다.
    std::uint32_t FirstPopFlags(Sid sid);
    // 004b0c60 ↔ CD 004ae040. 파생 클래스가 base 초기화를 호출할 때도 사용한다.
    void PostCreate(Sid sid);
    // 004ad4a0 ↔ CD 004ae120. zOrder만 갱신하며 owner·HP는 보존한다.
    void PostTake(Sid sid);
private:
    // 주소 산술 전에 타입 범위와 원본 생성자 지원 여부를 검증한다.
    const RiftTypeRecord& Type(std::uint32_t number,bool requireSupported) const;
    std::span<std::uint8_t> LiveBytes(Sid sid);
    // 원본 생성자의 쓰기 순서/폭/OR 비트를 보존한다. 호출 전에 지원 여부를 검사한다.
    void ApplyConstructor(std::span<std::uint8_t> bytes,const RiftTypeRecord& type) const;
    // 복원된 자산은 공통 Unpop을 사용한다. 최종 vtable 기록값으로 기존 객체를 확인한다.
    std::uint32_t ConstructorVtable(const RiftTypeRecord& type) const;
    // 원본 32비트 vtable 값은 기록할 뿐 호스트에서 역참조하지 않는다.
    std::uint32_t BaseVtable() const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    bool weakenedMana_{};
    SquidUnpop* unpop_{};
};
}
