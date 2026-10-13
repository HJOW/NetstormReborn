// 로컬 건설 배치(004433b0 / CD 004d14a0): 커서 미리보기를 없애고 abstract 조각을 놓은 뒤 서버에 요청한다.
#pragma once
#include "o/RawConstructionPlace.h"

namespace netstorm::o {
// 로컬 배치가 읽는 플레이어와 CanonDecoder 타입 전역이다. 비용 차감은 상위 커서 내려놓기의 책임이다.
struct ConstructionLocalState {
    std::uint32_t localPlayer{}; // 00540c70 / CD 0050f6c8: 예측 조각 소유자와 사제 이동의 플레이어.
    std::array<std::uint32_t,8> patternTypes{107,82,94,157,131,129,140,142}; // puzzlePiece, bridge, island, noIsland, 전투 타입 넷.
};
// 미리보기 SID와 원본 다섯 인자다. 타입·조각 반환 플래그·품질은 미리보기의 raw 필드에서 읽는다.
struct ConstructionLocalRequest {
    // preview: 실제 배치 전의 미리보기 판정·출력 처리기 또는 상태.
    Sid preview;
    // x: X 좌표.
    // y: Y 좌표.
    float x{},y{};
    // argument: 타입·패턴·명령에 별도로 전달하는 첫 인자.
    // direction: 방향이나 회전을 선택하는 값.
    std::uint32_t argument{},direction{};
};
// 서버 건설 요청 메시지의 논리 본문이다. 양 판본 모두 타입 BYTE, 좌표 float 둘, 인자 WORD, 방향/품질 BYTE다.
// wire 순서는 필드별로 직렬화해야 하며 호스트 구조체의 패딩을 전송하지 않는다.
struct ConstructionLocalMessage {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    std::uint8_t type{};
    // x: X 좌표.
    // y: Y 좌표.
    float x{},y{};
    // argument: 타입·패턴·명령에 별도로 전달하는 첫 인자.
    std::uint16_t argument{};
    // direction: 방향이나 회전을 선택하는 값.
    // quality: 품질.
    std::uint8_t direction{},quality{};
    std::uint32_t timeLow{},timeHigh{}; // 메시지 생성자가 0으로 초기화하는 시각 자리. 로컬 배치는 덮어쓰지 않는다.
};
// 미복원 가상 Form/조각 반환·사제 이동·메시지 계층은 명시 효과 경계로 둔다.
struct ConstructionLocalHooks {
    std::function<std::uint32_t(Sid)> previewSource; // 가상 +0: 미리보기와 연관된 객체 번호(원본 DWORD 반환).
    std::function<void(Sid,std::uint32_t)> destroyPreview; // 가상 +0x10: 미리보기를 플래그 0으로 삭제.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> returnPiece; // 004468b0 / CD 0043e930: 플래그·연관 번호·품질로 생산 칸 조각 반환.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 004af530 / CD 004ab390: 타입과 플래그 2로 클라이언트 SID 생성.
    std::function<void(const ConstructionPlaceRequest&)> place; // 00442c80 / CD 004d0ca0: abstract=1인 예측 배치.
    std::function<void(std::uint32_t,Sid,std::uint32_t,std::uint32_t)> moveBuilder; // 00443000 / CD 004d1120: 첫 조각으로 사제 이동, 시각 비트는 0/0.
    std::function<void(std::uint32_t,const ConstructionLocalMessage&)> send; // 004e2dc0 / CD 004c0380: 서버 번호 9로 요청 전송.
};
// 기존 SID 풀·타입/프레임 표와 spot bit 2 지도를 이용하는 로컬 배치 계층이다.
class RawConstructionLocal {
public:
    // 모든 참조는 객체보다 오래 살아야 한다. spot은 최소 256×256바이트이며 효과 경계는 모두 필수다.
    RawConstructionLocal(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const PriestPlainCanonType> frames,
        std::span<const std::uint8_t> spots,const ConstructionLocalState& state,ConstructionLocalHooks hooks);
    // 원본 스택 인자처럼 요청을 복사한다. 미리보기 조회/삭제 → 반환 → 본 SID 생성 → 받침/본 배치 → 사제 이동 → 서버 요청 순서다.
    void Place(ConstructionLocalRequest request) const;
    // 실제 factory/배치 어댑터가 같은 풀인지 확인할 때 쓴다.
    const SidPool& Pool() const;
private:
    // 타입/인자/방향/좌표로 decoder를 만들어 유효 칸마다 클라이언트 SID를 확보한다.
    std::vector<std::uint16_t> Allocate(std::uint32_t type,float x,float y,std::uint32_t argument,std::uint32_t direction) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::span<const PriestPlainCanonType> frames_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // state_: 로컬 배치가 읽는 플레이어와 CanonDecoder 타입 전역이다. 비용 차감은 상위 커서 내려놓기의 책임이다 (ConstructionLocalState).
    const ConstructionLocalState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ConstructionLocalHooks).
    ConstructionLocalHooks hooks_;
};
// 같은 풀의 실제 factory와 건설 배치를 연결한다. 나머지 Form/이동/전송 경계는 받은 그대로 유지한다.
ConstructionLocalHooks MakeConstructionLocalHooks(const SidPool& pool,SquidFactory& factory,RawConstructionPlace& place,ConstructionLocalHooks hooks);
}
