// 서버의 건설 확정(Construction.cpp 00444590 / CD 004d0f10)을 복원한다.
// 배치 가능 판정을 통과한 요청에 대해 조각마다 서버 SID를 확보하고, 배치 통지(메시지 0x3f)를 만들어 보낸 뒤
// 같은 통지를 자기 쪽 처리기에 직접 넘긴다. 통지를 받은 쪽이 RawConstructionPlace로 조각을 놓는다.
#pragma once
#include "o/CanonTypeDecoder.h"
#include "o/RawPriestPlacementGeometry.h"
#include "o/SquidFactory.h"
#include <vector>

namespace netstorm::o {
// 확정이 읽는 전역이다. 주소는 패치 / CD 순서다.
struct ConstructionConfirmState {
    bool skipBuilderCheck{}; // 0054db14 / CD 0052e8a8: 켜져 있으면 지을 사제를 찾지 않는다.
    bool recorderEnabled{};  // 0059a7e4 / CD 00537214: 켜져 있으면 기록기에 활성 여부를 묻는다.
    // CanonDecoder가 패턴으로 분기하는 여덟 타입 전역의 현재 번호다(puzzlePiece, bridge, island, noIsland, 전투 타입 넷).
    std::array<std::uint32_t,8> patternTypes{107,82,94,157,131,129,140,142};
};
// 확정 한 번의 입력이다. 원본 열 개 인자와 같은 순서다.
struct ConstructionConfirmRequest {
    std::uint32_t type{};      // 배치할 타입 번호.
    float x{},y{};             // 첫 칸의 좌표. 통지에는 비트 그대로 실린다.
    std::uint32_t player{};    // 소유자가 될 플레이어. 통지에는 하위 바이트만 실린다.
    std::uint32_t argument{};  // decoder 인자(패턴 번호 또는 프레임 인자). 통지에는 하위 바이트만 실린다.
    std::uint32_t direction{}; // 방향 값. 통지에는 하위 바이트만 실린다.
    std::uint32_t flags{};     // bit 0이 켜지면 사제 조회를 건너뛴다. 통지에는 하위 바이트가 실린다.
    std::uint32_t timeLow{},timeHigh{}; // 통지 +0x12의 double을 이루는 두 DWORD(사제 도착 시각 인자).
    std::uint32_t quality{};   // 다리 품질. 통지에는 하위 바이트만 실린다.
};
// 메시지 0x3f의 본문: 배치 통지다. 원본 위치는 +4 타입, +5 x, +9 y, +0xd 인자, +0xe 방향, +0xf 플레이어,
// +0x10 플래그, +0x11 품질, +0x12 시각, +0x1a SID 개수, +0x1b부터 SID 목록이다.
struct ConstructionNotice {
    std::uint8_t type{};
    float x{},y{};
    std::uint8_t argument{},direction{},player{},flags{},quality{};
    std::uint32_t timeLow{},timeHigh{};
    std::vector<std::uint16_t> sids; // 조각 순서의 SID. 원본 상한은 19개다.
};
// 이 모듈 밖의 효과다. 모두 필수다.
struct ConstructionConfirmHooks {
    // 0048fdb0 / CD 00406f80: (플레이어, 타입, x, y)로 지을 사제를 찾는다. 0이면 확정하지 않는다.
    std::function<Sid(std::uint32_t,std::uint32_t,float,float)> findBuilder;
    // 004af530 / CD 004ab390: (타입, 0)으로 서버 SID의 void 객체를 만든다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create;
    // 004e2e30 / CD 004c0420: 통지를 보낸다.
    std::function<void(const ConstructionNotice&)> broadcast;
    // 004441b0 / CD 004d1d00: 서버 번호 9가 보낸 것처럼 같은 통지를 직접 처리한다.
    std::function<void(const ConstructionNotice&)> handle;
    // 00495710 / CD 0049f2b0: 기록기가 활성인지 묻는다.
    std::function<bool()> recorderActive;
    // 00495730 / CD 0049f330: 첫 조각의 SID를 기록기에 넘긴다.
    std::function<void(Sid)> record;
};
// 건설 확정의 실행 계층이다. 사용 순서: 배치 가능 판정을 통과한 요청마다 Confirm을 한 번 부른다.
class RawConstructionConfirm {
public:
    // 풀·타입 표·프레임 표·상태는 이 객체보다 오래 살아야 한다. 경계가 하나라도 비어 있으면 생성에서 거부한다.
    RawConstructionConfirm(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const PriestPlainCanonType> frames,
        const ConstructionConfirmState& state,ConstructionConfirmHooks hooks);
    // 00444590 / CD 004d0f10: 사제가 지어야 하는 타입(flags1 0x8000)이면 먼저 사제를 찾고, 없으면 아무 효과 없이 false다.
    // 그 밖에는 조각마다 SID를 만들어 통지에 담고 전송 → 직접 처리 → (기록기) 순서로 부른 뒤 true를 돌려준다.
    bool Confirm(const ConstructionConfirmRequest& request) const;
    // 어댑터가 같은 실제 풀을 연결하는지 확인하는 데 쓴다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    const ConstructionConfirmState& state_;
    ConstructionConfirmHooks hooks_;
};
// SID 생성만 같은 풀의 실제 factory로 바꾼다. 사제 조회·통지·기록기 경계는 받은 그대로 둔다.
ConstructionConfirmHooks MakeConstructionConfirmHooks(const SidPool& pool,SquidFactory& factory,ConstructionConfirmHooks hooks);
}
