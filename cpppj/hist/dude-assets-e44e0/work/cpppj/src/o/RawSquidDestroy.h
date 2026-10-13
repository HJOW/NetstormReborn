// 공통 Squid destroy의 dead·삭제 훅·종속 체인·공간 해제·SID 반납 순서를 복원한다.
#pragma once
#include "o/SquidUnpop.h"
#include <functional>

namespace netstorm::o {
// 가상 삭제 훅과 외부 효과는 호스트 콜백이다. 원본 vtable 주소를 실행하지 않는다.
enum class SquidDestroyEffect { PreDestroy,ReleaseDependent,DestroyDependent,Transmit,ClearSelection,PostDestroy };
// 객체 삭제 발생한 동작의 대상과 인자를 기록하는 이벤트이다.
struct SquidDestroyEvent {
    // effect: 효과.
    SquidDestroyEffect effect{};
    // sid: 원본 풀에서 객체를 찾는 SID 번호.
    // parent: 부착·통지 대상인 부모 객체 또는 부모 SID.
    Sid sid{},parent{};
    // flags: 동작을 선택하는 플래그 비트.
    std::uint32_t flags{};
};
// emit은 같은 풀을 바꿀 수 있다. 종속 next는 ReleaseDependent 전에 저장한다.
// Transmit은 원본 수신자 -1·명령 -2·payload 0의 삭제 전파다. 실제 네트워크 구현은 별도다.
struct SquidDestroyHooks {
    // emit: 관찰한 이벤트를 외부 처리기에 전달하는 콜백.
    std::function<void(const SquidDestroyEvent&)> emit;
    // selected: 현재 선택한 객체의 SID를 조회하는 콜백.
    std::function<Sid()> selected;
    // form(타입 70 미만) 루트의 가상 Unpop(vtable +0x48)이다. 있으면 contained form 루트를 삭제할 수 있다.
    std::function<void(Sid)> unpopForm;
};
// 공통 Squid destroy의 dead·삭제 훅·종속 체인·공간 해제·SID 반납 순서를 복원한다.
class RawSquidDestroy {
public:
    // 풀/공간 해제/타입 입력의 수명은 어댑터보다 길어야 한다. 서로 다른 풀 연결은 거부한다.
    RawSquidDestroy(SidPool& pool,SquidUnpop& unpop,std::span<const RiftTypeRecord> types);
    // 004af780 ↔ CD 004ab7e0. 지원 자산의 실제 Unpop/반납을 연결하며 이미 dead이면 반환한다.
    // 이 함수는 공통 몸체다. vtable +0x10 호출은 RawSquidDestroyDispatch를 통해 다리 재정의를 거친다.
    // pre/post 훅은 각각 CompletePreDestroy/CompletePostDestroy를 정확히 한 번 호출해야 한다.
    // 종속 form/contained의 가상 release/destroy와 파생 훅 내부 구현은 emit 호출자 계약이다.
    // form 루트는 unpopForm 훅이 있을 때만 받으며 자산 공간 해제 대신 그 훅을 부른다.
    void Destroy(Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks);
    // 공통 pre/postDestroy의 마지막 깊이 감소를 연결하는 지점이다. 그 외 효과까지 수행하지 않는다.
    void CompletePreDestroy();
    void CompletePostDestroy();
    // 원본의 curDestroy/curPostDestroy 카운터를 검증/후속 연결에 공개한다.
    std::uint32_t PreDepth() const;
    std::uint32_t PostDepth() const;
    // 공통 훅이 같은 raw 풀을 처리하는지 생성 단계에서 확인한다.
    const SidPool& Pool() const;
    // 공통 훅의 Graph와 실제 Unpop이 서로 다른 공간을 갱신하는 연결을 막는다.
    void ValidateGraph(const RawGraph& graph) const;
private:
    // raw +4/+6 WORD를 호스트 정렬과 무관하게 읽는다.
    std::uint16_t Word(Sid sid,std::size_t offset) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // unpop_: 객체의 공간 등록을 해제하는 Unpop 처리기.
    SquidUnpop& unpop_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // preDepth_: 삭제 전처리의 중첩 깊이.
    // postDepth_: 삭제 후처리의 중첩 깊이.
    std::uint32_t preDepth_{},postDepth_{};
};
}
