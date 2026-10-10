// 건설 배치 실행(Construction.cpp 00442c80 / CD 004d0ca0)과 패치판의 비용 부족 처리·환불/취소 통지·SP 조회를 복원한다.
// 로컬 배치(004433b0)와 서버 확정(00444590)이 미리 확보한 SID 목록을 받아, 타입의 조각마다 abstract 비트·HP·프레임·단어·소유자를
// 정하고 Pop을 요청한다. SID 확보·배치 가능 판정·메시지 수신 처리는 호출자 쪽이며 이 모듈에 없다.
#pragma once
#include "o/CanonTypeDecoder.h"
#include "o/RawPriestPlacementGeometry.h"
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include "o/SquidReward.h"
#include <string>
#include <string_view>

namespace netstorm::o {
// 건설 배치가 읽고 쓰는 전역과 Player 필드다. 주소는 패치 / CD 순서이며 하나만 있으면 패치판 전용이다.
struct ConstructionPlaceState {
    std::uint32_t localPlayer{}; // 00540c70: 로컬 플레이어 번호. 배치의 플레이어 인자와 DWORD로 비교한다.
    bool server{true};           // 00540bc0 / CD 00540a28: 꺼져 있으면 번호 5 미만·서버 영역 SID를 수신(Take)해서 쓴다.
    bool authority{true};        // 00540bc4: 비용 부족 처리가 권한 쪽 안내/환불과 비권한 쪽 강제 차감 가운데 하나를 고른다.
    bool chargeBlocked{};        // 00594fa4: 켜져 있으면 배치가 비용을 판정하지 않는다.
    bool battleShown{};          // 00595344: 로컬 플레이어의 환불 가산을 건너뛰고 SP 조회의 번호 범위 검사를 건너뛴다.
    std::uint32_t noIslandType{157}; // 005412cc / CD 0051cbb8: 시작 좌표의 칸에 단어 0x5d4를 쓰는 타입이다.
    std::uint32_t bridgeType{82};    // 005411a0 / CD 0051ca8c: 품질 프레임과 수명 비트를 정하는 타입이다.
    std::uint32_t nuggetType{154};   // 005412c0: 비용 판정과 환불에서 빠지는 타입이다.
    std::int32_t bridgeLife{5};      // 0052f960 / CD 0051f05c: 다리 수명 상수. 금 간 품질과 끝 칸은 여기서 1을 뺀 값으로 시작한다.
    // CanonDecoder가 패턴으로 분기하는 여덟 타입 전역의 현재 번호다(puzzlePiece, bridge, island, noIsland, 전투 타입 넷).
    std::array<std::uint32_t,8> patternTypes{107,82,94,157,131,129,140,142};
    std::array<std::uint32_t,kPlayerCount+1> playerFlags{}; // Player +0x54. 0x400은 비용 부족이 한 번 보고됐다는 표시다.
    std::array<std::string,kPlayerCount+1> playerNames;     // Player +0xc. 비용 부족 안내 문구의 이름 인자다.
};
// 배치 한 번의 입력이다. 원본 열 개 인자 가운데 SID 개수는 sids의 길이로 받는다.
struct ConstructionPlaceRequest {
    std::uint32_t type{};      // 배치할 타입 번호.
    float x{},y{};             // 첫 칸의 좌표. CanonDecoder의 시작 좌표다.
    std::uint32_t argument{};  // 패턴 번호 또는 프레임 인자. 하위 단어는 조각의 +0xc 단어가 된다.
    std::uint32_t direction{}; // 방향 값. 회전은 direction / 2다.
    std::uint32_t player{};    // 조각의 소유자가 될 플레이어 번호. 서버 번호 9는 원본 assert다.
    std::span<const std::uint16_t> sids; // 조각 순서의 SID. 길이가 조각 수와 같아야 한다.
    std::uint32_t abstract{};  // 원본 아홉째 인자. bit 0이 abstract 비트가 되고, 0이 아니면 HP 0·Pop 플래그 2로 놓는다.
    std::uint32_t quality{};   // 원본 열째 인자. 다리 품질이며 1은 금 간 프레임(0x20)·수명 -1, 3은 단단한 프레임(0x40)이다.
};
// 메시지 0x6c(11바이트)의 본문: 플레이어에게 보내는 SP 통지다. 원본은 번호 바이트 둘과 1, 정수 금액을 싣는다.
struct ConstructionMoneyNotice {
    std::uint8_t player{},source{},kind{1};
    std::int32_t amount{};
};
// 메시지 0x2f(15바이트)의 본문: 배치 취소 통지다. 좌표 비트와 타입·인자·방향의 하위 바이트를 싣는다.
struct ConstructionRejectNotice {
    float x{},y{};
    std::uint8_t type{},argument{},direction{};
};
// 이 모듈 밖의 효과다. 아래 넷(take~pop)과 sendMoney는 두 판본 공통이고 나머지는 패치판에서만 불린다.
struct ConstructionPlaceHooks {
    // 004af610 / CD 004ab440: 서버가 아닐 때 번호 5 미만·서버 영역 SID의 객체를 받아 온다. 돌려준 SID를 조각으로 쓴다.
    std::function<Sid(std::uint32_t,Sid)> take;
    // 004214a0 / CD 00448c10: 다리 타입 조각의 단어를 쓴 뒤와 품질을 정한 뒤에 부르는 표면 알림이다.
    std::function<void(Sid)> notifySurface;
    // 가상 +0x74: 조각의 소유자 지정이다.
    std::function<void(Sid,std::uint32_t)> setOwner;
    // 가상 +0x90: 조각의 Pop(x, y, 플래그)이다. 공간 등록과 표시는 이 경계 뒤에서 일어난다.
    std::function<void(Sid,float,float,std::uint32_t)> pop;
    // 004e2dc0 / CD 004c0380에 실리는 SP 통지다. 첫 인자는 받는 플레이어다.
    std::function<void(std::uint32_t,const ConstructionMoneyNotice&)> sendMoney;
    // 0040ec50: SP 저장소에서 그 플레이어의 잔액을 읽는다.
    std::function<float(std::uint32_t)> storedSp;
    // 0040eec0: 로컬 플레이어의 SP에 더한다(음수는 차감).
    std::function<void(float)> addLocalSp;
    // 0041df80: 지정한 플레이어의 SP에 더한다(음수는 차감).
    std::function<void(std::uint32_t,float)> addPlayerSp;
    // 004a89c0: 서식 key와 이름 인자로 안내 문구를 만든다.
    std::function<std::string(std::string_view,std::string_view)> format;
    // 004c9200: 그 플레이어를 뺀 모두에게 문구를 보낸다.
    std::function<void(std::uint32_t,std::string_view)> tellOthers;
    // 004c9150: 그 플레이어에게 문구를 보낸다.
    std::function<void(std::uint32_t,std::string_view)> tellPlayer;
    // 004cf960: 로컬 안내 창에 문구를 띄운다.
    std::function<void(std::string_view)> tell;
    // 004e2dc0에 실리는 배치 취소 통지다. 첫 인자는 받는 플레이어다.
    std::function<void(std::uint32_t,const ConstructionRejectNotice&)> sendReject;
};
// 건설 배치의 실행 계층이다. 사용 순서: 호출자가 SID를 확보하고 Place를 한 번 부른다.
class RawConstructionPlace {
public:
    // 풀·타입 표·프레임 표·상태·보상 모듈은 이 객체보다 오래 살아야 한다. 보상 모듈은 같은 풀을 써야 하며 HP 유무와 최대 HP를 공급한다.
    // 공통 경계가 비어 있거나 패치판인데 패치 전용 경계가 비어 있으면 생성에서 거부한다.
    RawConstructionPlace(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const PriestPlainCanonType> frames,
        ConstructionPlaceState& state,const SquidReward& reward,ConstructionPlaceHooks hooks);
    // 00442c80 / CD 004d0ca0: 패치판은 먼저 비용을 판정하고, 이어 조각마다 abstract 비트·HP·프레임·단어(다리는 품질·수명)를 쓰고
    // 소유자 지정과 Pop을 요청한다. SID 개수가 조각 수와 다르면 아무 효과 없이 거부한다(원본은 끝에서 assert).
    void Place(const ConstructionPlaceRequest& request);
    // 00442b50(패치): 잔액이 모자란 배치의 처리다. 표시 비트를 켜고, 권한이 있으면 안내와 환불/취소 통지를, 없으면 강제 차감과 로컬 안내를 한다.
    void ChargeShortfall(std::uint32_t type,std::uint32_t player,float x,float y,std::uint32_t argument,std::uint32_t direction);
    // 00442a40 / CD 004d1bc0: 타입 비용을 0방향 절삭한 정수만큼 돌려주고(패치) 금액이 양수이면 SP 통지를 보낸다.
    void Refund(std::uint32_t type,std::uint32_t player);
    // 00442af0(패치): 환불한 뒤 배치 취소 통지를 보낸다.
    void Reject(std::uint32_t type,std::uint32_t player,float x,float y,std::uint32_t argument,std::uint32_t direction);
    // 0040ef30(패치): 플레이어의 잔액이다. 번호가 1~8 밖이면 저장소를 읽지 않고 0을 돌려준다(전투 표시 중의 로컬 플레이어는 예외).
    float Money(std::uint32_t player) const;
    // 0041dff0(패치): 로컬 플레이어이면 로컬 가산, 아니면 플레이어 가산을 부른다.
    void AddMoney(std::uint32_t player,float delta);
    // 어댑터가 같은 실제 풀을 연결하는지 확인하는 데 쓴다.
    const SidPool& Pool() const;
private:
    // 패치판에만 있는 몸체를 CD판 풀에서 부르면 거부한다.
    void RequirePatch() const;
    // 타입 번호가 타입 표·프레임 표·타입 바이트 범위 안인지 검사하고 레코드를 돌려준다.
    const RiftTypeRecord& Type(std::uint32_t type) const;
    // 00442c80의 비용 판정 구간(패치): 다른 플레이어의 배치만 판정하며 모자라면 ChargeShortfall, 아니면 차감이다.
    void ChargeForPlacement(const ConstructionPlaceRequest& request);
    // 004ac220 / CD 004abb00: +0xc 단어를 쓰고 객체 타입이 다리 타입이면 표면 알림을 부른다.
    void SetWord(Sid sid,std::uint16_t word);
    SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    ConstructionPlaceState& state_;
    const SquidReward& reward_;
    ConstructionPlaceHooks hooks_;
};
// 수신·소유자 지정·SP 읽기/가산을 같은 풀의 실제 모듈로 바꾼다. Pop·표면 알림·문구·메시지 경계는 받은 그대로 둔다.
// store는 패치판에서만 필요하며 CD판에서는 nullptr을 준다.
ConstructionPlaceHooks MakeConstructionPlaceHooks(const SidPool& pool,SquidFactory& factory,SquidOwner& owner,SquidReward& reward,
    ScrambledSpStore* store,const ConstructionPlaceState& state,ConstructionPlaceHooks hooks);
}
