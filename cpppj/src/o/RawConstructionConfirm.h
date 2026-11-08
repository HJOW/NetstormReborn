// 서버의 건설 확정(Construction.cpp 00444590 / CD 004d0f10)을 복원한다.
// 배치 가능 판정을 통과한 요청에 대해 조각마다 서버 SID를 확보하고, 배치 통지(메시지 0x3f)를 만들어 보낸 뒤
// 같은 통지를 자기 쪽 처리기에 직접 넘긴다. 통지를 받은 쪽이 RawConstructionPlace로 조각을 놓는다.
#pragma once
#include "o/CanonTypeDecoder.h"
#include "o/RawPriestPlacementGeometry.h"
#include "o/SquidFactory.h"
#include <string_view> //실제 SID 개수 진단의 식/파일 문자열을 직접 선언한다.
#include <vector>

namespace netstorm::o {
struct ConstructionPacketHeader; // 실제 지역 패킷의 현재 헤더만 빌리며 typed 통지와 순환 include하지 않는다.
// 확정이 읽는 전역이다. 주소는 패치 / CD 순서다.
struct ConstructionConfirmState {
    bool skipBuilderCheck{}; // 0054db14 / CD 0052e8a8의 독립 raw용 fallback이며 공유 시 현재 DWORD로 작업장/그래프 기준점 검사를 생략한다.
    bool recorderEnabled{};  // 0059a7e4 / CD 00537214: 켜져 있으면 기록기에 활성 여부를 묻는다.
    // CanonDecoder가 패턴으로 분기하는 여덟 타입 전역의 현재 번호다(puzzlePiece, bridge, island, noIsland, 전투 타입 넷).
    std::array<std::uint32_t,8> patternTypes{107,82,94,157,131,129,140,142};
    const std::uint32_t* sharedFullDeck{}; // 실제0054db14 owner를 선택적으로 빌리며 기존 독립 aggregate의 순서를 유지한다.
    bool SkipBuilderCheck() const { return sharedFullDeck ? *sharedFullDeck!=0 : skipBuilderCheck; } // 각 Confirm의 원문 분기에서 현재 비영 여부를 다시 읽는다.
    const CanonDecoderDiagnostics* decoderDiagnostics{}; // 반복자보다 오래 사는 같은 Client의 현재 checking·진단 장치를 빌린다.
    const std::uint32_t* sharedRecorderEnabled{}; // 실제59a7e4를 broadcast/자기 수신 뒤 현재 주소에서 읽는다. 독립 입력의 bool fallback은 유지한다.
    bool RecorderEnabled() const { return sharedRecorderEnabled ? *sharedRecorderEnabled!=0 : recorderEnabled; } // 현재 재생 플래그이며 Prepare 시점 사본으로 바꾸지 않는다.
    std::function<std::array<std::uint32_t,8>()> patternTypesRead; // ActualConfirm은 FindBuilder 정상 복귀 뒤 같은 현재8 TYPE owner를 조회한다. 독립 fixture는 기존 배열을 사용한다.
    std::array<std::uint32_t,8> CurrentPatternTypes() const { return patternTypesRead ? patternTypesRead() : patternTypes; } // 상위 결합 검증과 실제 decoder가 같은 현재 owner를 조회한다.
};
// 확정 한 번의 입력이다. 원본 열 개 인자와 같은 순서다.
struct ConstructionConfirmRequest {
    std::uint32_t type{};      // 배치할 타입 번호.
    float x{},y{};             // 첫 칸의 좌표. 통지에는 비트 그대로 실린다.
    std::uint32_t player{};    // 소유자가 될 플레이어. 통지에는 하위 바이트만 실린다.
    std::uint32_t argument{};  // decoder 인자(패턴 번호 또는 프레임 인자). 통지에는 하위 바이트만 실린다.
    std::uint32_t direction{}; // 방향 값. 통지에는 하위 바이트만 실린다.
    std::uint32_t flags{};     // bit 0이 켜지면 기준점 조회를 건너뛴다. 통지에는 하위 바이트가 실린다.
    std::uint32_t timeLow{},timeHigh{}; // 통지 +0x12의 double을 이루는 두 DWORD(사제 도착 시각 인자).
    std::uint32_t quality{};   // 다리 품질. 통지에는 하위 바이트만 실린다.
};
// 메시지 0x3f의 본문: 배치 통지다. 원본 위치는 +4 타입, +5 x, +9 y, +0xd 인자, +0xe 방향, +0xf 플레이어,
// +0x10 플래그, +0x11 품질, +0x12 시각, +0x1a SID 개수, +0x1b부터 SID 목록이다.
struct ConstructionNotice {
    // type: 타입 표에서 객체 종류를 찾는 번호.
    std::uint8_t type{};
    // x: X 좌표.
    // y: Y 좌표.
    float x{},y{};
    // argument: 타입·패턴·명령에 별도로 전달하는 첫 인자.
    // direction: 방향이나 회전을 선택하는 값.
    // player: 플레이어.
    // flags: 동작을 선택하는 플래그 비트.
    // quality: 품질.
    std::uint8_t argument{},direction{},player{},flags{},quality{};
    // timeLow: 전송할 시각 비트 표현의 하위 DWORD.
    // timeHigh: 전송할 시각 비트 표현의 상위 DWORD.
    std::uint32_t timeLow{},timeHigh{};
    std::vector<std::uint16_t> sids; // 조각 순서의 SID. 원본 상한은 19개다.
};
// 이 모듈 밖의 효과다. typed Confirm은 기존 여섯 효과를, ActualConfirm은 실제 mutable 송신/직접처리와 진단도 요구한다.
struct ConstructionConfirmHooks {
    // 0048fdb0 / CD 00406f80: (플레이어, 타입, x, y)로 작업장/그래프 기준점을 찾는다. 0이면 확정하지 않는다.
    // 이름은 기존 API를 유지한다. 소유 사제 조회 004921f0은 통지 처리기의 별도 경계다.
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
    std::function<void(std::span<std::uint8_t>)> transmitAndHandle; // ActualConfirm의 호출별 지역512를 방송하고 같은 현재 BYTE로 직접 처리한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 20번째 SID store 뒤 원문 진단이며 정상 복귀하면 Advance를 계속한다.
};
// 건설 확정의 실행 계층이다. 사용 순서: 배치 가능 판정을 통과한 요청마다 Confirm을 한 번 부른다.
class RawConstructionConfirm {
public:
    // 풀·타입 표·프레임 표·상태는 이 객체보다 오래 살아야 한다. 경계가 하나라도 비어 있으면 생성에서 거부한다.
    RawConstructionConfirm(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const PriestPlainCanonType> frames,
        const ConstructionConfirmState& state,ConstructionConfirmHooks hooks);
    // 00444590 / CD 004d0f10: 사제가 지어야 하는 타입(flags1 0x8000)이면 먼저 그래프 기준점을 찾고, 없으면 아무 효과 없이 false다.
    // 그 밖에는 조각마다 SID를 만들어 통지에 담고 전송 → 직접 처리 → (기록기) 순서로 부른 뒤 true를 돌려준다.
    bool Confirm(const ConstructionConfirmRequest& request) const;
    bool ActualConfirm(ConstructionConfirmRequest request,const ConstructionPacketHeader& header) const; // 실제 경로는 지역512 초기화→Create/store/후행 진단→방송/같은 wire 직접처리→기록 순서다.
    // 어댑터가 같은 실제 풀을 연결하는지 확인하는 데 쓴다.
    const SidPool& Pool() const;
private:
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const RiftTypeRecord> types_;
    // frames_: 타입별 프레임 자료.
    std::span<const PriestPlainCanonType> frames_;
    // state_: 확정이 읽는 전역이다. 주소는 패치 / CD 순서다 (ConstructionConfirmState).
    const ConstructionConfirmState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ConstructionConfirmHooks).
    ConstructionConfirmHooks hooks_;
};
// SID 생성만 같은 풀의 실제 factory로 바꾼다. 기준점 조회·통지·기록기 경계는 받은 그대로 둔다.
ConstructionConfirmHooks MakeConstructionConfirmHooks(const SidPool& pool,SquidFactory& factory,ConstructionConfirmHooks hooks);
}
