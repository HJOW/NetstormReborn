// Early1의 로컬 간헐천 생성 부모·100회 후보·최근5개·자원 pool을 실제 공간에 연결한다.
#pragma once
#include "o/RawConstructionConfirm.h"
#include "o/RawFortFinishOwner.h"
#include "o/RawSquidFinder.h"

namespace netstorm::o {
// 로컬 후보의 float 원본 좌표다. 실패한 시도도 마지막 반올림 좌표를 출력한다.
struct LocalGeyserPoint { float x{},y{}; }; // x/y는 간헐천 Pop 기준점이며 noIsland 확정 좌표보다 각각2 크다.
// 원본 전역은 매 효과 뒤 다시 읽는다. 최근5목록은 전체 간헐천 목록과 별개다.
struct LocalGeyserState {
    bool boss{true},fort{}; // 00540bc4/00594fb8의 현재 생성 권한과 요새 모드다.
    std::int32_t firstPlayerQuantity{3},lastPlayerQuantity{4}; // 00540bd4/d8의 현재1~4/5~8 생성 요청 수다.
    std::int32_t resourcePool{}; // 00595088의 현재 전장 간헐천 자원 pool이다.
    std::uint32_t emptyGeyserType{84},geyserType{122},noIslandType{157},isleType{97}; // 현재 삭제/생성/건설확정/프레임문자수집 타입 전역이다.
    const std::uint32_t* sharedGeyserType{}; // 현재 생성/비용 TYPE이며 값형 입력은 보존한다.
    std::uint32_t GeyserType() const { return sharedGeyserType ? *sharedGeyserType : geyserType; } // 생성/Pop 효과 뒤에도 현재 TYPE를 읽는다.
    const bool* sharedBoss{}; // 실제 Client의 생성 권한이며 독립 입력 bool은 유지한다.
    const bool& Boss() const { return sharedBoss ? *sharedBoss : boss; } // 각 Populate 진입의 원문 권한 관문에서 현재값을 읽는다.
};
// 원본 생성/공간/설정의 필수 하위다. 생성 성공을 임시 좌표나 빈 콜백으로 대신하지 않는다.
struct LocalGeyserHooks {
    std::function<std::int32_t(std::string_view,std::int32_t)> configInt; // 00441470(predictableGeysers,1)의 현재 설정값이다.
    std::function<double()> wallSeconds; // 비결정적 옵션일 때만 기존 TimeSeed에 전달하는 현재 벽시각이다.
    std::function<FortRandomRectangle(std::uint32_t)> playerBounds; // 0048f5c0의 현재 Player별 확장 사각형이다.
    std::function<void()> negativeQuantity; // 00486130의 reps>=0 진단이다. 진단 뒤 수량0으로 계속한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 현재 emptyGeyser의 실제 파생 삭제(flags0x400000)다.
    std::function<void(const ConstructionConfirmRequest&)> confirm; // 동일 풀의 실제00444590 noIsland 건설 확정이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 실제004af530(현재geyserType,0) factory다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 생성한 간헐천의 실제 가상+90(x,y,0)이다.
};
// PATCH의 args(0,0,1)와 부모의empty타입/프레임무늬 추가설정만 제공한다. 다른 exclusion 모드는 공개하지 않는다.
class RawLocalGeyser {
public:
    // players는 같은9×0xb4 Player 기록이며 spots/hash/pool/types는 동일 월드여야 한다.
    RawLocalGeyser(const SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> players,SquidPostPopList& recent,
        LocalGeyserState& state,LocalGeyserHooks hooks);
    // 00486360: seed0xfed 또는 현재 TimeSeed, eligible Player1~8 순서에서 같은 local 난수를 공유한다.
    void Generate() const;
    // 00486130의 로컬 요청: 권한/요새 guard→최근목록0→각 생성100회 시도→실제 생성/자원 순서다.
    void Populate(std::int32_t quantity,GameRandom& random,FortRandomRectangle rectangle) const;
    // 00486030/00485890의 로컬 후보: 원본 rectangle 난수/반올림/포함 경계/3×3 spot 순서를 실행한다.
    bool TryCandidate(GameRandom& random,FortRandomRectangle rectangle,LocalGeyserPoint& point) const;
    // 실제 Session은 원본00595088과 같은 SquidRewardState.geyserPool 주소를 생성/삭제 전에 연결한다.
    // 독립 fixture는 기존 state.resourcePool을 쓰며, 연결 시 값을 복사하거나 양쪽을 동기화하지 않는다.
    void BindResourcePool(std::int32_t& resourcePool);
    // 현재 실제 단일 자원 저장소다. 삭제/Pop 효과 뒤에도 이 주소의 현재 값을 다시 읽는다.
    std::int32_t& ResourcePool() const;
    // 상위가 전체 간헐천 목록이나 다른 풀을 잘못 연결하지 않도록 같은 자료 identity를 제공한다.
    const SidPool& Pool() const;
    SquidPostPopList& Recent() const;
private:
    // 실제4e49c0의 signed64 절삭 뒤 저DWORD 비트와 불확정 하위0을 보존한다.
    static std::int32_t Truncate(double value);
    // 원본004bca20은 limit<=0이면 local seed를 소비하지 않고0을 반환한다.
    static std::uint32_t Next(GameRandom& random,std::int32_t limit);
    // 부모가초기에보존한empty타입을각후보에서사용하며3×3프레임문자무늬도검사한다.
    bool Candidate(GameRandom& random,FortRandomRectangle rectangle,LocalGeyserPoint& point,std::uint32_t reuseType) const;
    // 실제typedfinder의첫exact일치와현재타입표의surface생략규칙을사용한다.
    Sid FindType(float x,float y,std::uint32_t type) const;
    // current emptyGeyser를 기존 일반 finder의 exact type/현재 공간 순서로 조회한다.
    Sid FindEmpty(float x,float y) const;
    // 원본이 직접 산술한 linear spot 인덱스를 읽는다. 물리 배열 밖은 명시 오류다.
    std::uint8_t Spot(std::int32_t x,std::int32_t y) const;
    const SidPool& pool_; // 실제 생성/삭제 SID와 현재 raw 타입을 읽는 유일한 풀이다.
    const SquidHash& hash_; // 기존 일반 typed finder가 읽는 같은 공간 해시다.
    std::span<const std::uint8_t> spots_; // 후보/confirm 사이 효과가 바꾼 현재65536BYTE spot 표다.
    std::span<const RiftTypeRecord> types_; // 현재 emptyGeyser genus와 geyser cost를 읽는 실제 TYPE 표다.
    std::span<const RiftTypeFrames> frames_; // hash0의현재isle 프레임문자를읽는같은프레임표다.
    std::span<const std::uint8_t> players_; // eligible flags/region을 매 Player 차례에서 직접 읽는 원본9기록이다.
    SquidPostPopList& recent_; // 00595094/98의 최근 최대5생성 목록이며 전체 장부를 대신하지 않는다.
    LocalGeyserState& state_; // 권한/수량/자원pool/현재타입 전역의 단일 저장소다.
    LocalGeyserHooks hooks_; // 실제 설정/region 사각형/건설확정/파생수명 경계다.
    std::int32_t* resourcePool_{}; // 후설치되면 실제 보상과 같은00595088 주소이며state의 독립검사용값은 사용하지 않는다.
};
}
