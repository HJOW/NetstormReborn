// Ascend 후보판정과 최대20개 선택·삭제·최종 빈 반경 판정을 실제 원본 순서로 실행한다.
#include "o/RawVortexAscend.h"
#include <array>
#include <bit>
#include <cmath>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 Ascend에서 제외하는 표면/내용물/유닛 genus 묶음이다.
constexpr std::uint32_t kExcludedGenus=0x128b302U;
// 원본 고정 new0x50 버퍼의 DWORD 후보 수다.
constexpr std::size_t kCandidates=20;
// 반경finder 생성자가 좌표에 더하는 원본 float 편향이다. 후보의 spot 좌표에는 이 값을 더하지 않는다.
constexpr float kRadiusBias=0.99999f;
// 원본 ftol의 low DWORD를 signed로 읽는다. 실제 지도 밖 읽기는 별도 계약 위반으로 진단한다.
std::int32_t Truncate(float value) {
    const auto wide=static_cast<double>(value);
    if (!std::isfinite(wide) || wide<-9223372036854775808.0 || wide>=9223372036854775808.0)
        throw std::out_of_range("Ascend 좌표 CRT 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(wide)));
}
// signed 거리 더하기를 원본 DWORD 넘침 규칙으로 수행한다.
std::int32_t Add(std::int32_t value,std::int32_t delta) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+static_cast<std::uint32_t>(delta));
}
// 0041d7a0/CD00440a80은 x87 double 합을 절삭한 뒤 좌표float로 저장한다. 보드의 정수 범위에서 그대로 반환한다.
std::int32_t RadiusCoordinate(float value) {
    const auto biased=static_cast<double>(value)+static_cast<double>(kRadiusBias);
    if (!std::isfinite(biased) || biased<-2147483648.0 || biased>=2147483648.0)
        throw std::out_of_range("Ascend 반경 좌표 범위 오류");
    return static_cast<std::int32_t>(biased);
}
}
// 필요한 효과와 표/지도의 판본 크기를 처음 구성할 때 확인한다.
RawVortexAscend::RawVortexAscend(SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
    std::span<const RiftTypeRecord> types,std::span<const VortexAscendIsland> islands,GameRandom& random,
    VortexAscendState& state,VortexAscendHooks hooks)
    :pool_(pool),hash_(hash),spots_(spots),types_(types),islands_(islands),random_(random),state_(state),hooks_(std::move(hooks)) {
    if (spots.size()!=65536 || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        !hooks_.islandNumber || !hooks_.reserveCandidateBuffer || !hooks_.releaseCandidateBuffer || !hooks_.destroy)
        throw std::invalid_argument("Ascend 공유 지도/타입 표/하위 효과 누락");
}
// little endian 필드를 지정한 폭으로 읽는다.
std::uint32_t RawVortexAscend::Read(Sid sid,std::size_t offset,std::size_t width) const {
    const auto raw=pool_.Slot(sid);std::uint32_t value=0;
    // 지정 폭 바깥의 이웃 필드를 읽지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 현재 genus와 섬을 확인한 뒤 현재 spot/타입/owner를 원본 순서로 다시 읽는다.
bool RawVortexAscend::Candidate(Sid sid,std::int32_t island,std::uint32_t owner) const {
    const auto raw=pool_.Slot(sid);const auto type=raw[10];
    if (type>=types_.size()) throw std::out_of_range("Ascend 후보 타입 오류");
    const auto x=std::bit_cast<float>(Read(sid,14)),y=std::bit_cast<float>(Read(sid,18));
    if (types_[type].flags2&kExcludedGenus) return false;
    if (hooks_.islandNumber(x,y)!=island) return false;
    const auto cellX=Truncate(x),cellY=Truncate(y);
    if (cellX<0 || cellX>=256 || cellY<0 || cellY>=256) throw std::out_of_range("Ascend spot 지도 범위 오류");
    const auto index=static_cast<std::size_t>(cellY)*256+static_cast<std::size_t>(cellX);
    if (spots_[index]&2U) {
        const auto support=Sid{hash_.Entries(0)[index]};
        if (!support.value || pool_.Slot(support)[10]==state_.noIslandType) return false;
    } else {
        const auto currentType=pool_.Slot(sid)[10];
        if (currentType>=types_.size()) throw std::out_of_range("Ascend 후속 후보 타입 오류");
        if (!(types_[currentType].flags1&0x200000U)) return false;
    }
    return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]!=owner;
}
// finder 최초 결과를 얻은 뒤 버퍼를 확보하며 첫20개까지만 후보를 모으고 다음1회 난수로 삭제한다.
bool RawVortexAscend::Tick(Sid temple,std::uint32_t count) const {
    auto radius=std::bit_cast<std::int32_t>(count);if (radius>32) radius=64;
    const auto island=hooks_.islandNumber(std::bit_cast<float>(Read(temple,14)),std::bit_cast<float>(Read(temple,18)));
    // 반경finder 생성자는 두 좌표에0.99999f 편향을 더해 절삭한 뒤 부호 있는 반경을 더하고 뺀다.
    const auto x=RadiusCoordinate(std::bit_cast<float>(Read(temple,14))),y=RadiusCoordinate(std::bit_cast<float>(Read(temple,18)));
    const auto negativeRadius=std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(radius));
    RawSquidFinder finder(pool_,hash_,types_);auto sid=finder.Begin({Add(x,negativeRadius),Add(y,negativeRadius),Add(x,radius),Add(y,radius)});
    const bool allocated=hooks_.reserveCandidateBuffer();
    std::array<Sid,kCandidates> candidates{};std::size_t size=0;
    try {
        // 현재 후보 owner는 매번 현재 신전 raw에서 읽으며20개가 되면 Next 없이 중단한다.
        while (sid.value) {
            if (Candidate(sid,island,pool_.Slot(temple)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32])) {
                if (!allocated) throw std::bad_alloc();
                candidates[size++]=sid;if (size==candidates.size()) break;
            }
            sid=finder.Next();
        }
        if (size) hooks_.destroy(candidates[random_.Next(static_cast<std::uint32_t>(size))],0x100000U);
    } catch (...) { hooks_.releaseCandidateBuffer();throw; }
    hooks_.releaseCandidateBuffer();return size!=0 || radius!=64;
}
// 원본은 목록 길이를 현재 반복마다 다시 읽으며 존재/nonzero owner/다른 owner의 항목만 센다.
std::uint32_t RawVortexAscend::CountForeignOwnedIslands(std::uint32_t owner) const {
    std::uint32_t count=0;
    // 공유 IslandList 항목의 존재/소유자를 읽는다. 빈 번호는 개수에 더하지 않는다.
    for (const auto& island:islands_) if (island.exists && island.owner && island.owner!=owner) ++count;
    return count;
}
// 같은 풀을 사용하는 Regular 처리기에만 연결한다.
const SidPool& RawVortexAscend::Pool() const { return pool_; }
}
