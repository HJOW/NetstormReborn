// 다리/섬 연결(004213b0), 연결 객체 생성(004210f0), 소유자 전파(00421240)를 raw SID 풀 위에서 복원한다.
// 외부 효과(새 객체 생성·가상 소유자 지정·가상 Pop·프레임 지정·표면 알림)는 호출자가 연결하고 순서·인자만 원본대로 호출한다.
// 근거와 검증 범위: docs/exe/cpp-bridgeconnect-reconstruction.md, 세 실제 PE의 제한 x86 기대값 bridgeconnect-x86.tsv.
#pragma once
#include "o/RawSquidNeighbors.h"
#include <array>
#include <functional>

namespace netstorm::o {
// 소유자 색이 없을 때의 섬 프레임 번호(중립)다. 원본 004421a0 ↔ CD 004d0180의 기본 반환값이다.
inline constexpr std::int32_t kNeutralOwnerFrame = 8;
// 연결 객체와 종유석을 만들 때 생성 함수에 넘기는 flags다(원본 `004af530(type, 2)` ↔ CD `004ab390(type, 2)`).
inline constexpr std::uint32_t kConnectCreateFlags = 2;
// 다리가 연결을 만드는 상대 표면의 genus 마스크다: island(2)와 섬 받침 계열(0x1000000). 원본 004213b0의 `0x1000002`.
inline constexpr std::uint32_t kIslandSurfaceMask = 0x01000002;
// 섬 받침·종유석 클래스의 가상 표 주소 기록값(패치 10.78 / CD·10.37). raw 슬롯 +0의 DWORD와 비교만 하고 호스트에서 역참조하지 않는다.
inline constexpr std::uint32_t kPatchIslandVtable = 0x00506d98, kCdIslandVtable = 0x00506960;
inline constexpr std::uint32_t kPatchStalagVtable = 0x00506e50, kCdStalagVtable = 0x00506a18;
// 소유자 색 표(00531b08 ↔ CD 005365a0)의 칸 수다. 0~8이 플레이어 번호다.
// 그 뒤의 메모리는 판본마다 다른 값(패치 1, CD 0)이라 표의 일부로 보지 않는다.
inline constexpr std::size_t kOwnerColorCount = 9;
// 연결 함수들이 읽는 원본 전역이다. 패치 주소 / CD 주소를 주석에 둔다.
struct BridgeConnectState {
    std::uint32_t connectorType{}; // 005412c4 / CD 0051cbb0: bridgeConnector 타입 번호(연결 객체).
    std::uint32_t islandType{};    // 005411d0 / CD 0051cabc: island 타입 번호(섬 받침).
    std::uint32_t stalagType{};    // 005411d4 / CD 0051cac0: islandStalag 타입 번호(섬 아래 종유석).
    std::uint32_t noIslandType{};  // 005412cc / CD 0051cbb8: noIsland 타입 번호(섬 받침 위의 표면 칸).
    std::uint32_t bridgeType{};    // 005411a0 / CD 0051ca8c: 단어 쓰기(004ac220)가 표면 알림을 부르는 타입 번호.
    bool battle{};                 // 00594fbc / CD 00540a20: 전투 모드(inBattleMode). 꺼져 있으면 색 프레임은 항상 중립이다.
    bool mission{};                // 00594fa0 / CD 0052e16c: 미션 중(global.inMission). 중립 프레임의 섬은 소유자가 바뀌어도 프레임을 그대로 둔다.
    std::array<std::int32_t, kOwnerColorCount> ownerColors{0, 1, 2, 3, 4, 5, 6, 7, 8}; // 00531b08 / CD 005365a0의 초기값.
};
// 연결 함수들이 부르는 외부 효과다. 호출 순서는 원본과 같다.
struct BridgeConnectHooks {
    // 004af530(type, flags) / CD 004ab390: 새 객체 생성. 타입 바이트와 가상 표가 채워진 할당 슬롯의 번호를 돌려준다.
    std::function<Sid(std::uint32_t type, std::uint32_t flags)> create;
    // 가상 소유자 지정(vtable +0x74). 섬 받침·종유석은 재정의(MakeOwnerDispatch)로 분배해야 한다.
    std::function<void(Sid sid, std::uint32_t owner)> setOwner;
    // 가상 Pop(vtable +0x90).
    std::function<void(Sid sid, float x, float y, std::uint32_t flags)> pop;
    // 004acee0(frame, flags) / CD 004acbc0: 프레임 지정. 표시 갱신 또는 해시 단계 재등록과 함께 프레임 필드를 바꾼다.
    // 소유자 전파는 이 호출 뒤의 프레임 필드를 다시 읽어 종유석에 넘긴다.
    std::function<void(Sid sid, std::int32_t frame, std::uint32_t flags)> setFrame;
    // 004214a0 / CD 00448c10: 연결 객체의 타입이 다리 타입 전역과 같을 때만 불린다. 상태를 바꾸지 않는 검사다.
    std::function<void(Sid sid)> notifySurface;
};
// 004421a0 ↔ CD 004d0180: 소유자의 색 프레임이다. 전투 모드이고 번호가 양수이면 색 표 값 - 1, 아니면 중립(8)이다.
// 색 표 밖의 번호는 원본이 표 뒤의 메모리를 읽으므로 여기서는 거부한다.
std::int32_t OwnerColorFrame(const BridgeConnectState& state, std::int32_t owner);
// 00442310 ↔ CD 004d01a0(섬 받침)·004d0350(종유석): base 소유자 지정을 부른 뒤 프레임 필드를 소유자 색으로 직접 쓴다(표시 갱신 없음).
// 미션 중이고 현재 프레임이 중립이면 프레임을 그대로 둔다. 패치는 DWORD, CD는 하위 바이트를 쓴다.
void SetIslandOwner(SidPool& pool, const BridgeConnectState& state, Sid sid, std::uint32_t owner,
    const std::function<void(Sid, std::uint32_t)>& base);
// 가상 소유자 지정(+0x74)을 객체의 가상 표 기록값으로 분배한다. 섬 받침·종유석이면 재정의를, 아니면 base를 부른다.
// pool·state는 반환한 함수보다 오래 살아야 한다.
std::function<void(Sid, std::uint32_t)> MakeOwnerDispatch(SidPool& pool, const BridgeConnectState& state,
    std::function<void(Sid, std::uint32_t)> base);
class RawBridgeConnect {
public:
    // 풀·이웃 탐색기·상태는 이 객체보다 오래 살아야 한다. neighbors는 같은 풀을 읽어야 하고 훅은 모두 연결돼 있어야 한다.
    RawBridgeConnect(SidPool& pool, const RawSquidNeighbors& neighbors, const BridgeConnectState& state, BridgeConnectHooks hooks);
    // 004213b0 ↔ CD 00448b00: flag 4(abstract 제외) 연결 이웃을 순회하며 다리↔섬 표면 쌍마다 연결 객체를 만들고 소유자를 전파한다.
    // 자기가 다리이면 genus가 kIslandSurfaceMask에 걸리는 이웃이, 아니면 다리인 이웃이 대상이다. 다리 쪽이 항상 첫 인자다.
    // preview가 있으면 연결 객체를 등록하지 않고 위치만 써서 목록에 넣는다(소유자 전파는 그대로 한다).
    void Connect(Sid sid, std::vector<Sid>* preview = nullptr) const;
    // 004210f0 ↔ CD 004487c0: first의 위치에서 second의 (올림한) 중심 쪽으로 한 칸 옮긴 곳에 연결 객체를 만든다.
    // 프레임은 방향/2, +0xc 단어는 first, +8 단어는 second다. 원본은 두 객체의 타입을 검사하지 않는다.
    Sid Link(Sid first, Sid second, std::uint32_t owner, std::vector<Sid>* preview = nullptr) const;
    // 00421240 ↔ CD 00448910: second 위치의 섬 받침이 주인이 없으면 first의 소유자를 준다.
    // 섬 받침의 소유자·색 프레임 → 같은 위치 종유석의 프레임 → 받침 발자국 안의 noIsland 소유자 순서다.
    void PropagateOwner(Sid first, Sid second) const;
    // 004b1fa0 ↔ CD 004eb4b0: 좌표를 자른 한 칸에서 일반 탐색 순서로 지정 타입의 첫 객체를 찾는다. 없으면 0이다.
    // 찾는 타입이 island·bridge genus가 아니면 해시 0단계를 건너뛴다.
    Sid FindTypeAt(float x, float y, std::uint32_t type) const;
    // 004421c0 ↔ CD 004d01d0의 앞부분(섬 받침의 postPop 재정의): 최초 등록(flags 비트 1)이면 같은 위치에 종유석을 만들어
    // 소유자와 Pop을 준 뒤 연결 순회를 한다. 뒤따르는 공통 postPop은 호출자가 부른다.
    void IslandPostPopPrefix(Sid island, std::uint32_t flags) const;
    // 004421c0 ↔ CD 004d01d0 전체: 접두 뒤에 같은 flags로 공통 postPop(base, 004b0d30 ↔ CD 004ae180)을 부른다.
    void IslandPostPop(Sid island, std::uint32_t flags, const std::function<void(Sid, std::uint32_t)>& base) const;
private:
    // 판본별 프레임 번호 필드를 읽고 쓴다. 패치는 +0x24 DWORD, CD는 +0x22 바이트다.
    std::int32_t Frame(Sid sid) const;
    void SetFrameField(Sid sid, std::int32_t frame) const;
    // 004ac200 ↔ CD 004abae0: 객체 타입의 genus(flags2)다.
    std::uint32_t Genus(Sid sid) const;
    SidPool& pool_;
    const RawSquidNeighbors& neighbors_;
    const BridgeConnectState& state_;
    BridgeConnectHooks hooks_;
};
}
