// 내용물 삭제 시 커서 취소·너겟 수입/알림을 공통 삭제·SP 저장소와 같은 수명으로 연결한다.
#pragma once
#include "o/ContentFormHost.h"
#include "o/RawSquidDestroy.h"
#include "o/SquidReward.h"

namespace netstorm::o {
// 삭제 중 효과가 갱신할 수 있는 원본 전역이다. 매 분기/요청에서 최신 값을 읽는다.
struct ContentLifetimeState {
    Sid cursor{};                         // 패치 0055a2a8 / CD 0051b658: 잡고 있는 내용물 SID다.
    std::uint32_t nuggetType{154};         // 패치 005412c0: 너겟 kind WORD와 비교하는 현재 타입 DWORD다.
    std::uint32_t localOwner{1},notice{};  // 패치 00540c70 / 005429c4: 로컬 소유자와 수입 알림 명령이다.
    bool boss{true};                      // 패치 00540bc4: 다른 소유자에게 너겟 알림을 송신하는 권한이다.
    std::function<Sid()> currentCursor;    // 실제 세션의 현재 커서를 매 삭제 시 읽으며 미연결 fixture는 cursor 값형 입력을 유지한다.
    const bool* sharedBoss{}; // 실제 커서 취소와 같은 Client의 boss 주소다.
    const bool& Boss() const { return sharedBoss ? *sharedBoss : boss; } // 중첩 커서 취소 뒤 원격 수입 알림 위치에서 현재값을 읽는다.
};
// 실제 UI/패킷·소리 경계다. wallet/SP는 SquidReward를 공급하면 실제 저장소로 연결한다.
struct ContentLifetimeHooks {
    std::function<void()> cancelCursor; // 원본 커서 취소 몸체를 요청한다. 내부 중첩 삭제는 같은 월드 분배기로 돌아와야 한다.
    std::function<void(std::uint32_t)> localNotice; // 00452fe0(command,0,0)의 같은 알림 큐다.
    std::function<void(std::uint32_t)> remoteNotice; // 0040abe0의 빈 수입 패킷을 004e2dc0(owner,packet)으로 보내는 경계다.
    std::function<void(std::uint32_t,std::int32_t)> wallet; // 00413bb0(owner,절삭(cost)): SP 가산에 앞선 별도 AI 지갑 가산이다.
    std::function<void(std::uint32_t,float,bool)> addSp; // local이면 0040eec0, 다른 소유자면 0041df80이다.
    std::function<void(float,float)> sound; // 004a9d70(center.x,center.y,"guy.wav",0)의 위치 소리다.
};
// 타입6의 가상 pre/postDestroy·빈 부모 삭제 통지와 실제 Unpop/SID 반납을 복원한다.
class ContentFormLifetime {
public:
    // 참조들은 이 객체보다 오래 살아야 한다. 타입 span은 콜백이 바꾼 현재 비용/발자국을 읽기 위해 복사하지 않는다.
    ContentFormLifetime(SidPool& pool,std::span<const RiftTypeRecord> types,ContentFormHost& forms,RawSquidDestroy& destroy,
        const ContentLifetimeState& state,ContentLifetimeHooks hooks,SquidDestroyHooks fallback={},SquidReward* reward=nullptr);
    // 004aeeb0/CD 004b0300: 커서 취소 검사→패치 너겟 효과→같은 공통 pre 깊이 반납이다.
    // 이 직접 진입은 이미 공통 삭제기가 pre 깊이를 올린 상태에서 호출한다.
    void PreDestroy(Sid form) const;
    // 가상 +0x10의 공통 삭제→종속 재진입→실제 Unpop→반납을 공유한다. dead이면 공통 몸체가 바로 반환한다.
    void Destroy(Sid form,std::uint32_t flags=0) const;
    // Content만 분배하고 다른 타입의 기존 효과를 유지한다. 전체 월드 분배기와 내부 삭제도 같은 훅을 사용해야 한다.
    SquidDestroyHooks Hooks() const;
    // 반납된 타입 기록도 판정하되 필드 접근은 별도로 free를 검사한다.
    bool IsContentForm(Sid form) const;
    const SidPool& Pool() const;
private:
    // 정렬되지 않은 WORD와 현재 부모 소유 BYTE를 읽는다.
    std::uint16_t Word(Sid sid,std::size_t offset) const;
    std::uint32_t Owner(Sid parent) const;
    // 실제 타입 비용은 float이고 별도 지갑 지급액만 CRT처럼 0방향 절삭/하위 DWORD로 바뀐다.
    float Cost() const;
    // 타입의 footX/footY 절반을 현재 좌표에서 빼서 원본 중심 float 저장 순서를 보존한다.
    std::array<float,2> Center(Sid parent) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // forms_: 객체에 부착된 Form의 생성·통지·수명 처리기.
    ContentFormHost& forms_;
    // destroy_: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    RawSquidDestroy& destroy_;
    // state_: 삭제 중 효과가 갱신할 수 있는 원본 전역이다. 매 분기/요청에서 최신 값을 읽는다 (ContentLifetimeState).
    const ContentLifetimeState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ContentLifetimeHooks).
    ContentLifetimeHooks hooks_;
    // fallback_: emit은 같은 풀을 바꿀 수 있다. 종속 next는 ReleaseDependent 전에 저장한다 (SquidDestroyHooks).
    SquidDestroyHooks fallback_;
    // reward_: HP·SP·지갑 보상을 실제 장부에 적용하는 처리기.
    SquidReward* reward_{};
};
}
