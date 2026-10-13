// Squid.cpp의 내장 Form 생성·지정 Take와 공통 종속 해제를 같은 raw SID 풀에 연결한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// ContentForm이 살아 있는 부모에서 빠진 뒤 가상 +0x24(form,2)를 요청하는 경계다.
using FormRemovalNotice=std::function<void(Sid parent,Sid form)>;
// Form의 void 해제 뒤 서버 Pop을 요청하는 경계다. 전송기가 같은 raw 상태를 읽도록 flags를 전달한다.
using FormPopTransmit=std::function<void(Sid form,std::uint32_t flags)>;
// 네 내장 Form의 생성자와 Form 공통 Unpop을 처리한다. 원본 가상 표는 기록값으로만 사용한다.
class SquidForms {
public:
    // 풀은 이 객체보다 오래 살아야 한다. ContentForm의 live 부모 제거에는 notice가 필요하다.
    explicit SquidForms(SidPool& pool,FormRemovalNotice notice={},FormPopTransmit transmit={});
    // 004af530/CD 004ab390의 타입 1/2/3/6 경로다. Allocate 뒤 가상 표→타입만 쓰며 postCreate는 비어 있다.
    Sid Create(std::uint32_t type,std::uint32_t flags=0) const;
    // 004af610/CD 004ab440: 지정 서버/예측 슬롯의 payload·풀 목록/카운터를 유지한다.
    // live 슬롯은 같은 타입의 실제 Form Unpop을 먼저 수행한다. void live 슬롯의 assert 이후 실행은 지원하지 않는다.
    Sid Take(std::uint32_t type,Sid sid) const;
    // 기본 Form 부착: 부모 머리에 넣고 contained를 켠 뒤 공통 Pop을 호출한다. ContentForm은 정렬 부착기를 사용한다.
    void Attach(Sid form,Sid parent,std::uint32_t flags=0) const;
    // 이미 종속 체인을 만든 Form의 void 해제/서버 전송을 수행한다. 기본 postPop은 외부 효과 없이 깊이를 반납한다.
    void Pop(Sid form,std::uint32_t flags) const;
    // 부착/Pop 전에 현재 상태와 전송 경계가 연결되어 있는지 확인한다. flags 정규화 결과를 돌려준다.
    std::uint32_t ValidatePop(Sid form,std::uint32_t flags) const;
    // 004ae140/CD 004af270: void 먼저 설정→부모 free/dead 분기→종속 링크 정리→contained 해제다.
    // ProcessForm의 실제 프로세스 타입도 같은 가상 표면 지원한다. ContentForm은 끝에 부모 제거 알림을 보낸다.
    void Unpop(Sid form) const;
    // Form의 +0x10 부모 WORD와 +0xe 이전 WORD를 읽는다. free 또는 미복원 가상 표는 거부한다.
    Sid Parent(Sid form) const;
    Sid Previous(Sid form) const;
    // 다른 모듈과 같은 풀/판본을 공유하는지 확인할 때 사용한다.
    const SidPool& Pool() const;
private:
    // 지원 내장 타입의 생성자가 쓰는 판본별 실제 가상 표 기록값이다.
    std::uint32_t Vtable(std::uint32_t type) const;
    // 기존 Form의 현재 타입·가상 표를 확인한다. dead는 공통 삭제 중에도 허용한다.
    std::span<const std::uint8_t> Form(Sid sid) const;
    // Take는 free/예약 예측 머리도 쓰므로 AllocatedBytes와 분리하여 범위만 확인한다.
    std::span<std::uint8_t> Raw(Sid sid) const;
    // 원본의 정렬되지 않은 WORD/DWORD를 little endian으로 조립한다.
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width) const;
    // 원본 store 폭만 바꿔 주변 payload를 보존한다.
    void Put(Sid sid,std::size_t offset,std::uint32_t value,std::size_t width) const;
    // 이웃의 Form +0xe와 contained 자산 +0x10 이전 링크를 구분한다.
    void SetPrevious(Sid sid,std::uint16_t previous) const;
    SidPool& pool_;
    FormRemovalNotice notice_;
    FormPopTransmit transmit_;
};
}
