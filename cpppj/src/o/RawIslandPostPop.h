// noIsland postPop(004423b0 / CD 004d2790)과 3×3 받침 소유자 전파(0044bd20 / CD 00461f00)를 복원한다.
// 근거: docs/exe/cpp-islandpostpop-reconstruction.md. 생성·소유자·Pop·연결·표시 효과는 기존 raw 모듈로 연결한다.
#pragma once
#include "o/RawBridgeConnect.h"

namespace netstorm::o {
// noIsland의 실제 가상 표 주소 기록값. 호스트에서 역참조하지 않는다.
inline constexpr std::uint32_t kPatchNoIslandVtable = 0x00506f08, kCdNoIslandVtable = 0x00506ad0;
// 섬 후처리 Pop 처리 중 조회·갱신하는 상태 묶음이다.
struct IslandPostPopState {
    std::uint32_t loadingDepth{}; // 005c89b8 / CD 005178d4: 요새 읽기 중이면 서/북 칸으로 프레임 글자를 선택한다.
    bool ownerPropagationSuppressed{}; // 005c85d4 / CD 0051894c: H 칸에서의 받침 소유자 전파를 억제한다.
    bool* sharedSavedGame{}; // 실제 Client의005c85d4를 빌리며 생략한 독립 입력은 기존 bool을 사용한다.
    bool& SavedGame() { return sharedSavedGame ? *sharedSavedGame : ownerPropagationSuppressed; } // 현재 저장 게임 관문의 실제 주소를 반환하며 사본을 동기화하지 않는다.
    const bool& SavedGame() const { return sharedSavedGame ? *sharedSavedGame : ownerPropagationSuppressed; } // H 칸 처리 때마다 같은 소유자의 현재 값을 직접 읽는다.
};
// 섬 후처리 Pop 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct IslandPostPopHooks {
    BridgeConnectHooks objects; // 기존 생성·가상 소유자·가상 Pop 효과를 공유한다. setFrame/notifySurface는 이 모듈에서 쓰지 않는다.
    std::function<void(Sid)> connect; // 004213b0 / CD 00448b00: 표면 칸의 연결 순회.
    std::function<Sid(float,float,std::uint32_t)> findTypeAt; // 004b1fa0 / CD 004eb4b0: 받침·종유석 조회.
    std::function<void(Sid)> updateDisplay; // 가상 표 +0x88: 소유자를 지정한 직후 표시 갱신.
    std::function<void(bool,bool,bool)> incompleteSupport; // 표면 칸/받침/종유석 누락 시 원본의 진단·assert 경계. 이미 수행한 소유자 효과는 유지한다.
};
// noIsland postPop(004423b0 / CD 004d2790)과 3×3 받침 소유자 전파(0044bd20 / CD 00461f00)를 복원한다.
class RawIslandPostPop {
public:
    // 지도는 256×256 SID 머리 배열이다. 고정 지도(005c84bc / CD 0052d590)는 현재 지도(00542514 / CD 005670cc)와 달라질 수 있어 별도로 받는다.
    // 모든 참조/지도는 이 객체보다 오래 살아야 한다. 생략한 소유자 지도는 현재 지도를 쓴다.
    RawIslandPostPop(SidPool& pool,std::span<const std::uint16_t> surfaceMap,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,const BridgeConnectState& typeState,const IslandPostPopState& state,
        IslandPostPopHooks hooks,std::span<const std::uint16_t> ownerMap = {});
    // 00442350 / CD 004d03a0: 범위 안의 한 칸이 noIsland이면 프레임 방향 글자, 그 밖에는 0이다.
    std::uint8_t SurfaceLetter(int x,int y) const;
    // 최초 등록(flags & 1)에서만 프레임 선택·F 받침 생성·H 소유자 전파·연결 순회를 수행한다.
    // 뒤따르는 공통 postPop은 SquidPostPop::Activate가 같은 flags로 호출한다.
    void Prefix(Sid surface,std::uint32_t flags) const;
    // (x,y)가 noIsland인 경우 x/y를 절삭한 오른쪽 아래 기준점의 3×3 칸을 x 먼저, y 다음 순서로 갱신한다.
    // 이어 받침과 종유석을 갱신하며 누락 진단은 모든 정상 칸의 효과 뒤에 호출한다.
    void SetSupportOwner(float x,float y,std::uint32_t owner) const;
private:
    // 현재 프레임 코드의 방향 글자를 검증해 읽는다.
    std::uint8_t Letter(Sid sid) const;
    // 지도 밖은 0, 지도 안은 머리 SID 그대로다(Next 체인을 순회하지 않는다).
    Sid SurfaceAt(int x,int y,std::span<const std::uint16_t> map) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // surfaceMap_: 표면 지도.
    // ownerMap_: 소유자 지도.
    std::span<const std::uint16_t> surfaceMap_,ownerMap_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::span<const RiftTypeFrames> frames_;
    // typeState_: 연결 함수들이 읽는 원본 전역이다. 패치 주소 / CD 주소를 주석에 둔다 (BridgeConnectState).
    const BridgeConnectState& typeState_;
    // state_: 처리 중 조회·갱신하는 상태 (const IslandPostPopState).
    const IslandPostPopState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (IslandPostPopHooks).
    IslandPostPopHooks hooks_;
};
}
