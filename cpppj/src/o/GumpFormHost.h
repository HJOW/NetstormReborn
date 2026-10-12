// Squid.cpp의 GumpForm을 기존 SID 풀/종속 체인/공통 삭제에 연결한다. UI 등록 표는 client 쪽 알림이 조회한다.
#pragma once
#include "o/SquidFactory.h"
#include "o/RawSquidDestroy.h"

namespace netstorm::o {
// 두 판본의 내장 GumpForm 번호다. ProcessForm(2)/ContentForm(6)과 별도 타입이다.
inline constexpr std::uint32_t kGumpFormType=3;
// 현재 Form의 +0x12 DWORD 소유 ID와 +0x10 WORD 부모를 전달한다. 콜백은 같은 풀을 중첩 삭제할 수 있다.
using GumpFormNotice=std::function<void(std::uint32_t owner,Sid object)>;

// 자산/프로세스와 같은 실제 SID 풀에서 GumpForm의 생성·소유·부착·알림·삭제를 처리한다.
class GumpFormHost {
public:
    // 모든 참조는 host/반환 훅보다 오래 살아야 한다. fallback은 ProcessForm/자산의 기존 가상 삭제 처리다.
    // notice는 소유 ID가 비영일 때만 필요하다. 서로 다른 풀/판본 타입 표는 생성 단계에서 거부한다.
    GumpFormHost(SidPool& pool,SquidFactory& factory,std::span<const RiftTypeRecord> types,
        RawSquidDestroy& destroy,GumpFormNotice notice={},SquidDestroyHooks fallback={});
    // 메뉴의 Form(type,2)을 실제 factory에 연결한다. 생성 자체는 부모/소유 ID를 쓰지 않는다.
    Sid Create(std::uint32_t flags=2) const;
    // 기존 처리기와 같은 풀/판본인지 연결 단계에서 확인할 수 있다.
    const SidPool& Pool() const;
    // 반납된 타입 기록도 포함해 번호가 GumpForm인지 판별한다. 이미 dead인 중첩 삭제를 원본처럼 건너뛰기 위해서다.
    bool IsGumpForm(Sid form) const;
    // 현재 raw DWORD 소유 ID와 WORD 부모를 읽는다. free 슬롯은 읽기 계약 밖이다.
    std::uint32_t Owner(Sid form) const;
    Sid Parent(Sid form) const;
    // 004644e0/CD 00492f80이 +0x12에 쓰는 Gump 등록 ID다. WORD로 절단하거나 호스트 포인터로 해석하지 않는다.
    void SetOwner(Sid form,std::uint32_t owner) const;
    // 가상 +0x74→공통 contained 부착이다. 패치 parent=0은 무동작, CD의 assert 뒤 계속 경로는 거부한다.
    // 비영 부모는 종속 머리 앞에 넣고 void를 끈다.
    // client SID의 메뉴 경로와 전송 억제 flags를 지원한다. 서버 Form의 실제 Pop 직렬화/전송은 후속이다.
    void Attach(Sid form,Sid parent,std::uint32_t flags=0) const;
    // 004ae140/CD 004af270: void를 켜고 살아 있는 부모의 체인에서 뺀다. 부모 free/dead이면 링크/contained를 보존한다.
    void Unpop(Sid form) const;
    // 가상 +0x1c와 +0x78은 같은 알림이다. 호출 인자로 받은 삭제 부모 대신 Form의 현재 부모를 보낸다.
    void Notify(Sid form) const;
    // 가상 +0x10을 공통 삭제→실제 Unpop→SID 반납으로 연결한다. Form 자체의 삭제에는 소유 UI 알림이 없다.
    void Destroy(Sid form,std::uint32_t flags=0) const;
    // GumpForm pre/post 깊이·부모 삭제 알림·종속 삭제·Unpop을 처리하고 다른 타입은 fallback으로 전달한다.
    // 전체 월드 삭제도 이 훅을 사용해야 자산의 종속 GumpForm에 실제 UI 알림이 도달한다.
    SquidDestroyHooks Hooks() const;
private:
    // 원본 필드를 읽되 범위/타입/free 상태를 검사한다. dead는 공통 삭제 중에 필요하므로 허용한다.
    std::span<const std::uint8_t> Form(Sid form) const;
    std::uint16_t Word(Sid sid,std::size_t offset) const;
    // raw little endian 쓰기는 할당된 슬롯에만 적용한다.
    void Put(Sid sid,std::size_t offset,std::uint32_t value,std::size_t width) const;
    // 가상 setPrev의 Form +0xe/contained 자산 +0x10 위치를 구분한다.
    void SetPrevious(Sid sid,std::uint16_t previous) const;
    // 원본 Pop의 서버/client flags를 계산하고 미복원 전송을 부착 쓰기 전에 거부한다.
    void ValidatePop(Sid form,std::uint32_t flags) const;
    SidPool& pool_;
    SquidFactory& factory_;
    std::vector<RiftTypeRecord> types_;
    RawSquidDestroy& destroy_;
    GumpFormNotice notice_;
    SquidDestroyHooks fallback_;
};
}
