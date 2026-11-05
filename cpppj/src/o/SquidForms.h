// Squid.cpp의 내장 Form 생성·지정 Take와 공통 종속 해제를 같은 raw SID 풀에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/RawFormAttach.h" // 실제 공통 부착 부모의 선택된 장치를 연결한다.
#include "o/RawFormPop.h" // 공통 Pop의 현재 진단/공유 깊이를 연결한다.
#include <functional>
#include <string_view>

namespace netstorm::o {
// ContentForm이 살아 있는 부모에서 빠진 뒤 가상 +0x24(form,2)를 요청하는 경계다.
using FormRemovalNotice=std::function<void(Sid parent,Sid form)>;
// Form의 void 해제 뒤 서버 Pop을 요청하는 경계다. 전송기가 같은 raw 상태를 읽도록 flags를 전달한다.
using FormPopTransmit=std::function<void(Sid form,std::uint32_t flags)>;
struct FormUnpopHooks { // 공통 Form 해제의 현재 가상 getter·종속 분리·진단 정상 복귀를 연결한다.
    std::function<Sid(Sid,std::uint32_t)> parent; // 진단 뒤 선택한 현재 VT+0의 부모 getter다.
    std::function<void(Sid)> detach; // void 설정/부모 free 관문 뒤 실제 공통 종속 분리만 실행한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 식/파일/행을 전달하며 돌아오면 현재 상태로 계속한다.
};
// 네 내장 Form의 생성자와 Form 공통 Unpop을 처리한다. 원본 가상 표는 기록값으로만 사용한다.
class SquidForms {
public:
    // 풀은 이 객체보다 오래 살아야 한다. ContentForm의 live 부모 제거에는 notice가 필요하다.
    explicit SquidForms(SidPool& pool,FormRemovalNotice notice={},FormPopTransmit transmit={},FormUnpopHooks unpop={});
    void BindPop(FormPopHooks hooks); // 현재 World의 같은 깊이/진단과 선택된 실제 +20을 대여한다.
    void BindAttach(FormAttachHooks hooks); // 실제 현재 TYPE/번호/진단을 빌리며 setter/Pop 미지정은 같은 풀 기본 장치를 사용한다.
    void BindUnpop(FormUnpopHooks hooks); // 실제 World의 현재 종속 분리/진단을 같은 수명 안에서 빌린다.
    // 판본별1/2/3/6의 실제 cdecl 생성자는 지정 물리 SID의 가상 표DWORD만 쓴다. 할당/타입BYTE/다른 바이트는 쓰지 않는다.
    void ConstructVtableAt(std::uint32_t type,Sid sid) const; // Take/Create를 실행하지 않으며 SID0/free/dead도 원문 물리 범위 안이면 그대로 쓴다.
    // 004af530/CD 004ab390의 타입 1/2/3/6 경로다. Allocate 뒤 가상 표→타입만 쓰며 postCreate는 비어 있다.
    Sid Create(std::uint32_t type,std::uint32_t flags=0) const;
    // 004af610/CD 004ab440: 지정 서버/예측 슬롯의 payload·풀 목록/카운터를 유지한다.
    // live 슬롯은 같은 타입의 실제 Form Unpop을 먼저 수행한다. 선택된 진단이 정상 복귀하면 현재 상태로 계속한다.
    Sid Take(std::uint32_t type,Sid sid) const;
    // 공통 Form 부착 부모다. 현재 setter/링크 뒤 직접 Pop을 호출하며 별도 Content 정렬 API와 구별한다.
    void Attach(Sid form,Sid parent,std::uint32_t flags=0) const;
    // 이미 종속 체인을 만든 Form의 void 해제/서버 전송을 수행한다. 기본 postPop은 외부 효과 없이 깊이를 반납한다.
    void Pop(Sid form,std::uint32_t flags) const;
    // 정렬 Content 등 기존 호출자의 명시 선행 검증이다. 공통 Pop 부모는 이를 사용하지 않으며 해당 호출자 guard는 별도 잔여다.
    std::uint32_t ValidatePop(Sid form,std::uint32_t flags) const;
    // 004ae140/CD 004af270: 세 진단 복귀→void 설정→현재 부모 free 관문→공통 종속 분리 순서다.
    // 타입으로 VT를 추정하지 않으며 현재 번호의 ContentForm 부모 알림은 분리 종단이 선택한다.
    void Unpop(Sid form) const;
    // Form의 +0x10 부모 WORD와 +0xe 이전 WORD를 읽는다. free 또는 미복원 가상 표는 거부한다.
    Sid Parent(Sid form) const;
    Sid Previous(Sid form) const;
    // 다른 모듈과 같은 풀/판본을 공유하는지 확인할 때 사용한다.
    const SidPool& Pool() const;
    // 지원 내장 타입의 생성자가 쓰는 판본별 실제 가상 표 기록값이다.
    std::uint32_t Vtable(std::uint32_t type) const;
private:
    // 기존 Form의 현재 가상 표 클래스를 확인한다. 변경 가능한 TYPE 번호로 VT를 추정하지 않으며 dead는 공통 삭제 중에도 허용한다.
    std::span<const std::uint8_t> Form(Sid sid) const;
    // Take는 free/예약 예측 머리도 쓰므로 AllocatedBytes와 분리하여 범위만 확인한다.
    std::span<std::uint8_t> Raw(Sid sid) const;
    // 원본의 정렬되지 않은 WORD/DWORD를 little endian으로 조립한다.
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width) const;
    // 원본 store 폭만 바꿔 주변 payload를 보존한다.
    void Put(Sid sid,std::size_t offset,std::uint32_t value,std::size_t width) const;
    // 이웃의 Form +0xe와 contained 자산 +0x10 이전 링크를 구분한다.
    void SetPrevious(Sid sid,std::uint16_t previous) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // notice_: 통지.
    FormRemovalNotice notice_;
    // transmit_: Form 데이터 작성과 송신을 연결하는 전송 경계.
    FormPopTransmit transmit_;
    FormPopHooks pop_; // 독립 입력은 아래 지역 깊이와 같은 실제 Form 빈 +20을 사용한다.
    mutable std::uint32_t localPostPopDepth_{}; // 장기 공유 깊이를 빌리지 않는 독립 편의 객체의 현재 DWORD다.
    FormAttachHooks attach_; // 독립 입력은 명시 genus0/content6을 사용하고 실제 공장은 현재 TYPE/전역을 공급한다.
    FormUnpopHooks unpop_; // 미연결 독립 입력은 같은 공통 분리와 명시 ContentForm 알림을 사용한다.
};
}
