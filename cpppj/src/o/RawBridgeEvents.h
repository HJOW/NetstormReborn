// 다리 이벤트 처리기(다리 vtable +0x5c)와 끝 칸 변환(004215d0)을 raw SID 풀 위에서 복원한다.
// 외부 효과(이웃 탐색·삭제·생성·소유자 지정·Pop·표면 알림)는 호출자가 연결하고 순서·인자만 원본대로 호출한다.
#pragma once
#include "o/RawBridgeLifecycle.h"
#include "o/RiftType.h"
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 이벤트 0x2691: 권한이 있으면 다리 칸이 스스로 destroy한다. (0x2692 = kBridgeFallEvent는 지연 낙하다.)
inline constexpr std::uint32_t kBridgeDestroyEvent = 0x2691;
// 처리기 반환값은 Regular 이벤트 계약을 따른다: 양수 재예약, 0 종료, 음수 유지.
// 권한이 없으면 두 이벤트 모두 0(종료)을 돌려 예약을 없앤다. 권한이 있으면 -1(유지)이다.
inline constexpr float kBridgeEventEnd = 0.0f, kBridgeEventKeep = -1.0f;
// 처리기가 읽는 원본 전역이다. 패치 주소 / CD 주소를 괄호에 둔다.
struct BridgeEventState {
    bool authority{true};   // 00540bc4 / CD 00540a2c: 이벤트를 실제로 실행할 권한.
    bool debugKeep{};       // 0054db80 / CD 0052e928: 끝 칸 변환을 막는 디버그 유지 플래그.
    std::uint32_t bridgeType{}; // 005411a0 / CD 0051ca8c: 단어 쓰기(004ac220)가 표면 알림을 부르는 타입 번호.
};
// 끝 칸 변환이 부르는 외부 효과. 순서는 원본 호출 순서와 같다.
struct BridgeEventHooks {
    // 타입 번호의 프레임 코드 표. 현재 프레임의 방향 글자·글자별 첫 프레임·플래그 0x40을 읽는다.
    std::function<const RiftTypeFrames&(std::uint32_t type)> frames;
    // 004b23e0 / CD 004ebad0: 임시 프레임이 써진 채로 flag 8 표면 이웃 탐색기를 만들고 첫 이웃 번호(+0x34)를 돌려준다. 0이면 이웃 없음.
    std::function<Sid(Sid bridge)> firstNeighbor;
    // 004214a0 / CD 00448c10: 표면 알림.
    std::function<void(Sid sid)> notifySurface;
    // 004af530(type, 0) / CD 004ab390: 새 객체 생성. 타입 바이트와 가상 표가 채워진 할당 슬롯의 번호를 돌려준다.
    std::function<Sid(std::uint32_t type)> create;
    // 가상 destroy(vtable +0x10).
    std::function<void(Sid sid, std::uint32_t flags)> destroy;
    // 가상 소유자 지정(vtable +0x74). 인자는 옛 객체의 소유자 바이트다.
    std::function<void(Sid sid, std::uint8_t owner)> setOwner;
    // 가상 Pop(vtable +0x90). 좌표는 옛 객체의 위치 float을 그대로 넘긴다.
    std::function<void(Sid sid, float x, float y, std::uint32_t flags)> pop;
};
class RawBridgeEvents {
public:
    // 풀·상태·훅은 이 객체보다 오래 살아야 한다. 훅은 모두 연결돼 있어야 한다.
    RawBridgeEvents(SidPool& pool, const BridgeEventState& state, BridgeEventHooks hooks);
    // 00422740 ↔ CD 00449a20: 이벤트를 처리하고 Regular 계약의 반환값을 돌려준다.
    // 0x2691은 자기 destroy, 0x2692는 payload(좌표 포장 float)를 풀어 끝 칸 변환을 한다. 그 밖의 이벤트는 payload를 그대로 돌려준다.
    // count(두 번째 인자)는 원본이 읽지 않는다.
    float Handle(Sid bridge, std::uint32_t event, std::uint32_t count, float payload) const;
    // 004215d0(패치 단독 함수, CD는 처리기에 인라인): 다리 칸을 방향에 맞는 끝 칸(L·M·N·O)으로 바꾼다.
    // (x, y)는 지연 낙하가 가리키는 칸 좌표다. 권한이 없거나 디버그 유지가 켜져 있으면 아무것도 하지 않는다.
    void ConvertEnd(Sid bridge, float x, float y) const;
private:
    // 판본별 프레임 번호 필드를 읽고 쓴다. 패치는 +0x24 DWORD, CD는 +0x22 바이트다.
    std::int32_t Frame(Sid sid) const;
    void SetFrame(Sid sid, std::uint32_t frame) const;
    // 004ac220 ↔ CD 004abb00: +0xc 단어를 쓰고 다리 타입이면 표면 알림을 부른다.
    void SetWord(Sid sid, std::uint16_t word) const;
    SidPool& pool_;
    const BridgeEventState& state_;
    BridgeEventHooks hooks_;
};
}
