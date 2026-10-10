// 배치 통지 전의 로컬 예측 조각 정리(Construction.cpp 00441fd0 / CD 004cff80)를 복원한다.
// 배치 통지 처리기가 조각을 놓기 직전에 부른다. 통지가 놓을 칸마다 그 자리에 먼저 놓여 있던 클라이언트 영역의
// abstract 조각(로컬 배치가 예측으로 놓은 것)을 하나씩 지워 통지의 조각이 들어갈 자리를 비운다.
#pragma once
#include "o/CanonTypeDecoder.h"
#include "o/RawPriestPlacementGeometry.h"
#include "o/RawSquidFinder.h"

namespace netstorm::o {
// 정리가 읽는 전역이다. 주소는 패치 / CD 순서다.
struct ConstructionClearState {
    bool authority{true};         // 00540bc4 / CD 00540a2c: 꺼져 있으면 타입이 다른 abstract 조각도 후보가 된다.
    std::uint32_t bridgeType{82}; // 005411a0 / CD 0051ca8c: 프레임이 달라도 방향 글자가 같으면 같은 조각으로 보는 타입이다.
    // CanonDecoder가 패턴으로 분기하는 여덟 타입 전역의 현재 번호다(puzzlePiece, bridge, island, noIsland, 전투 타입 넷).
    std::array<std::uint32_t,8> patternTypes{107,82,94,157,131,129,140,142};
};
// 정리 한 번의 입력이다. 통지의 타입·좌표·decoder 인자·방향·플레이어를 그대로 받는다.
struct ConstructionClearRequest {
    std::uint32_t type{};      // 통지가 놓을 타입 번호.
    float x{},y{};             // 첫 칸의 좌표.
    std::uint32_t argument{};  // decoder 인자(패턴 번호 또는 프레임 인자).
    std::uint32_t direction{}; // 방향 값.
    std::uint32_t player{};    // 통지의 플레이어. 후보의 소유자 바이트와 DWORD로 비교한다.
};
// 이 모듈 밖의 효과다.
struct ConstructionClearHooks {
    // 가상 +0x10: 후보 조각을 지운다. 둘째 인자는 삭제 플래그이며 통지와 같은 조각이면 0x2000000, 아니면 0이다. 필수다.
    std::function<void(Sid,std::uint32_t)> destroy;
    // 칸마다 탐색을 시작하기 전에 그 칸의 탐색 범위를 알린다. 원본에 없는 관찰 지점이며 검사와 진단에만 쓴다. 비워 둘 수 있다.
    std::function<void(const SquidSearchArea&)> scan;
};
// 로컬 예측 조각 정리의 실행 계층이다. 사용 순서: 배치 통지를 받으면 조각을 놓기 전에 Clear를 한 번 부른다.
class RawConstructionClear {
public:
    // 통지와 같은 조각을 지울 때 넘기는 삭제 플래그다.
    static constexpr std::uint32_t kSamePieceFlag=0x2000000;
    // 풀·해시·타입 표·프레임 표·상태는 이 객체보다 오래 살아야 한다. 삭제 경계가 비어 있으면 생성에서 거부한다.
    RawConstructionClear(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const PriestPlainCanonType> frames,const ConstructionClearState& state,ConstructionClearHooks hooks);
    // 00441fd0 / CD 004cff80: decoder의 유효 칸마다 칸 범위의 객체를 일반 탐색기로 훑어, abstract이면서 번호가 클라이언트 영역이고
    // 타입이 같은(권한이 없으면 타입 무관) 첫 후보 하나를 지운다. 한 칸에서 둘 이상 지우지 않는다.
    void Clear(const ConstructionClearRequest& request) const;
    // 00425af0 → 0041d770 / 0041d7a0 (CD 00420140 → 00440a50 / 00440a80): 칸 좌표와 타입 발자국으로 탐색 범위를 구한다.
    // 범위는 (x - (폭 - 1), y - (높이 - 1))을 1~255로 자른 점과 칸 좌표를 잇는 사각형이며, 칸 좌표가 지도 밖(0 이하 또는 256 이상)이면
    // 자른 점 하나로 줄어든다. 작은 쪽은 0방향 절삭, 큰 쪽은 0.99999를 더해 절삭한다.
    static SquidSearchArea CellArea(float x,float y,int footX,int footY);
    // 호출자가 같은 풀을 쓰는지 확인하는 데 쓴다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_;
    const SquidHash& hash_;
    std::span<const RiftTypeRecord> types_;
    std::span<const PriestPlainCanonType> frames_;
    const ConstructionClearState& state_;
    ConstructionClearHooks hooks_;
};
}
