// 빈 공간의 검사 생략·검사 전 spot 보존·고정 WORD/소유자·나선 길이를 유지한다.
#include "o/RawPriestSpawn.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 raw 타입/WORD 필드와 Carrier genus 마스크다.
constexpr std::size_t kType=10,kWord=12;
constexpr std::uint32_t kCarrier=0x30000,kOwnerMask=0x7f;
// 256×256 지도에서 후보는 가장자리 0과 255를 제외한다. 스냅 상수는 공간 조회 bias와 다르다.
constexpr std::size_t kSide=256,kCells=kSide*kSide;
constexpr float kSnapBias=0.99999f;
// 원본 8방향 표의 0/2/4/6번, 북→동→남→서 순서다.
constexpr std::array<int,4> kDx{0,1,0,-1},kDy{-1,0,1,0};
// stage 2~23에서 길이 stage/2로 진행하여 총 132개 후보를 확인한다.
constexpr int kFirstStage=2,kStageEnd=24;
// x87의 넓은 덧셈 뒤 CRT 절삭을 보존하고 비유한/정수 범위 밖 입력을 거부한다.
std::int64_t Snap(float coordinate) {
    const double value=static_cast<double>(coordinate)+static_cast<double>(kSnapBias);
    if (!std::isfinite(value) || value<std::numeric_limits<std::int32_t>::min() || value>std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("사제 생성 스냅 좌표 범위 오류");
    return static_cast<std::int32_t>(value);
}
}
// 판본별 타입 표 길이·공간 크기·필수 경계를 확인한다.
RawPriestSpawn::RawPriestSpawn(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
    const PriestSpawnState& state,PriestSpawnHooks hooks):pool_(pool),types_(types),spots_(spots),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || spots.size()!=kCells || !hooks_.mayPlace || !hooks_.create || !hooks_.setOwner || !hooks_.pop || !hooks_.notifySurface || !hooks_.checkCarrier)
        throw std::invalid_argument("사제 생성 자료/하위 효과 누락");
}
// 실패해도 CD는 조용히 반환하고 패치는 현재 debug 전역에 따라 진단한다.
void RawPriestSpawn::Spawn(float x,float y,std::uint32_t type,std::uint16_t word) const {
    if (type<kFirstAssetTypeNumber || type>=types_.size()) throw std::out_of_range("사제 생성 타입 범위 오류");
    std::int64_t cellX=Snap(x),cellY=Snap(y);const auto owner=word&kOwnerMask;std::size_t direction=0;
    // 나선의 한 변 길이는 1,1,2,2,...,11,11이다.
    for (int stage=kFirstStage;stage<kStageEnd;++stage) {
        // 범위 밖 후보도 같은 방향으로 한 칸 이동하며 탐색 예산을 소비한다.
        for (int step=0;step<stage/2;++step) {
            if (cellX>0 && cellY>0 && cellX<kSide-1 && cellY<kSide-1) {
                const auto spot=spots_[static_cast<std::size_t>(cellY)*kSide+static_cast<std::size_t>(cellX)];
                const float candidateX=static_cast<float>(cellX),candidateY=static_cast<float>(cellY);
                if (!spot || (hooks_.mayPlace({type,candidateX,candidateY,0,owner,0}) && !(spot&0x10))) {
                    const Sid created=hooks_.create(type,0);auto raw=pool_.AllocatedBytes(created);
                    raw[kWord]=static_cast<std::uint8_t>(word);raw[kWord+1]=static_cast<std::uint8_t>(word>>8);
                    if (raw[kType]==state_.bridgeType) hooks_.notifySurface(created);
                    hooks_.setOwner(created,owner);hooks_.pop(created,candidateX,candidateY,0);CheckCarrier(created);return;
                }
            }
            cellX+=kDx[direction];cellY+=kDy[direction];
        }
        direction=(direction+1)%kDx.size();
    }
    if (pool_.Edition()==OriginalEdition::Patch1078 && state_.checkingPlacement) throw std::runtime_error("사제 생성 자리 탐색 실패: 원본 debug assert");
}
// Carrier 검사 래퍼의 genus 분기는 실제 구현이며 가상 +0xcc 몸체만 외부에 남긴다.
void RawPriestSpawn::CheckCarrier(Sid sid) const {
    const auto type=pool_.AllocatedBytes(sid)[kType];
    if (type>=types_.size()) throw std::out_of_range("사제 Carrier 검사 타입 범위 오류");
    if (types_[type].flags2&kCarrier) hooks_.checkCarrier(sid);
    else if (pool_.Edition()==OriginalEdition::Patch1078) throw std::invalid_argument("사제 Carrier 검사 genus 누락: 원본 assert");
}
// 종속 해방과 같은 풀인지 확인한다.
const SidPool& RawPriestSpawn::Pool() const { return pool_; }
// 참조 수명은 호출자가 보장하며 다른 일반 자산 해방 효과는 유지한다.
DamageableReleaseHooks MakePriestSpawnHooks(const SidPool& pool,const RawPriestSpawn& spawn,DamageableReleaseHooks hooks) {
    if (&pool!=&spawn.Pool()) throw std::invalid_argument("사제 생성/종속 해방의 SID 풀이 다릅니다");
    hooks.spawnPriest=[&spawn](float x,float y,std::uint32_t type,std::uint16_t word) { spawn.Spawn(x,y,type,word); };return hooks;
}
}
