// 로컬 예측 조각의 할당/배치 순서와 서버 요청 필드 폭을 원본대로 보존한다.
#include "o/RawConstructionLocal.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 미리보기 Form 안의 타입 WORD·반환 플래그 WORD와 판본별 품질 BYTE 위치다.
constexpr std::size_t kPreviewType=18,kPreviewFlags=20,kPatchQuality=30,kCdQuality=26;
// spot의 좌표 절삭 전 보정값이다. float 덧셈으로 먼저 반올림하지 않고 x87처럼 넓혀 더한다.
constexpr double kSpotBias=0.9998999834060669;
// 타입의 받침/사제 비트, 생산 칸 반환 비트와 클라이언트 할당 플래그다.
constexpr std::uint32_t kNeedsBase=0x400,kNeedsBuilder=0x8000,kReturnPiece=8,kClient=2;
// 지도 한 변과 로컬 배열의 SID 상한이다(원본은 WORD 배열 18개를 0으로 초기화한다).
constexpr std::size_t kMapSide=256,kMaxSids=18;
// raw Form의 두 바이트를 리틀엔디언 WORD로 읽는다. 정렬과 호스트 표현에 의존하지 않는다.
std::uint16_t Word(std::span<const std::uint8_t> raw,std::size_t offset) {
    return static_cast<std::uint16_t>(raw[offset]|(static_cast<std::uint16_t>(raw[offset+1])<<8));
}
// 원본의 32비트 (1-foot) 계산을 보존해 좌표 오프셋으로 넓힌다.
double BaseOffset(std::int32_t foot) { return static_cast<double>(std::bit_cast<std::int32_t>(1U-std::bit_cast<std::uint32_t>(foot))); }
}
// 첫 호출 전에 지도 크기와 필수 경계를 검사한다.
RawConstructionLocal::RawConstructionLocal(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,std::span<const std::uint8_t> spots,const ConstructionLocalState& state,ConstructionLocalHooks hooks)
    :pool_(pool),types_(types),frames_(frames),spots_(spots),state_(state),hooks_(std::move(hooks)) {
    if (spots_.size()<kMapSide*kMapSide || !hooks_.previewSource || !hooks_.destroyPreview || !hooks_.returnPiece ||
        !hooks_.create || !hooks_.place || !hooks_.moveBuilder || !hooks_.send) throw std::invalid_argument("로컬 건설 지도/필수 효과 연결 오류");
}
// 모든 유효 칸에 타입 자체를 사용해 SID를 만든다. decoder가 골라 준 프레임은 뒤의 실제 Place가 사용한다.
std::vector<std::uint16_t> RawConstructionLocal::Allocate(std::uint32_t type,float x,float y,std::uint32_t argument,std::uint32_t direction) const {
    const auto& meta=frames_[type];auto decoder=DecodeCanonType(meta.frames,meta.defaultFrame,state_.patternTypes,
        {type,std::bit_cast<int>(argument),std::bit_cast<int>(direction),x,y,false},pool_.Edition());
    std::vector<std::uint16_t> sids;std::size_t pieces=0;
    // 원본 로컬 WORD 배열 안에 들어가는 칸 수인지 SID 생성 전에 검사한다.
    for (auto probe=decoder;probe.Valid();probe.Advance()) if (++pieces>kMaxSids) throw std::length_error("로컬 건설 조각 수가 지역 SID 배열을 넘는다");
    // decoder 순서 그대로 클라이언트 슬롯을 확보한다.
    for (;decoder.Valid();decoder.Advance()) {
        if (sids.size()>=kMaxSids) throw std::length_error("로컬 건설 조각 수가 지역 SID 배열을 넘는다");
        const Sid sid=hooks_.create(type,kClient);
        if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::invalid_argument("로컬 건설 SID 생성 번호 오류");
        sids.push_back(sid.value);
    }
    return sids;
}
// 좌표·타입 오류는 미리보기 삭제 전에 거부한다. 실제 호출 순서는 원본 로컬 배치 몸체와 같다.
void RawConstructionLocal::Place(ConstructionLocalRequest request) const {
    if (request.preview.value<5 || request.preview.value>=pool_.Capacity() || !std::isfinite(request.x) || !std::isfinite(request.y))
        throw std::out_of_range("로컬 건설 미리보기/좌표 오류");
    const auto raw=pool_.Slot(request.preview);const std::uint32_t type=Word(raw,kPreviewType);
    const auto island=state_.patternTypes[2],noIsland=state_.patternTypes[3];
    if (type>=types_.size() || type>=frames_.size() || type>0xff || island>=types_.size() || noIsland>0xff || noIsland>=types_.size() || noIsland>=frames_.size())
        throw std::out_of_range("로컬 건설 타입 번호 오류");
    const double spotX=std::trunc(static_cast<double>(request.x)+kSpotBias),spotY=std::trunc(static_cast<double>(request.y)+kSpotBias);
    const bool inSpotRange=spotX>=0 && spotX<kMapSide && spotY>=0 && spotY<kMapSide;
    if ((types_[type].flags1&kNeedsBase) && !inSpotRange)
        throw std::out_of_range("로컬 건설 spot 좌표가 지도 밖입니다");
    const auto source=hooks_.previewSource(request.preview);
    const std::uint32_t flags=Word(raw,kPreviewFlags),quality=raw[pool_.Edition()==OriginalEdition::Patch1078 ? kPatchQuality : kCdQuality];
    hooks_.destroyPreview(request.preview,0);
    if (flags&kReturnPiece) hooks_.returnPiece(flags,source,quality);
    const auto main=Allocate(type,request.x,request.y,request.argument,request.direction);
    // 받침 여부는 시작 칸 하나의 spot bit 2만 읽는다. 서버 요청의 전체 발자국 검사와 다르다.
    if (types_[type].flags1&kNeedsBase) {
        // 효과 경계가 받침 비트를 켠 경우에도 잘못된 지도 인덱스는 조회하지 않는다.
        if (!inSpotRange) throw std::out_of_range("로컬 건설 spot 좌표가 지도 밖입니다");
        if (!(spots_[static_cast<std::size_t>(spotY)*kMapSide+static_cast<std::size_t>(spotX)]&2)) {
            const float baseX=static_cast<float>(static_cast<double>(request.x)+BaseOffset(types_[island].footX));
            const float baseY=static_cast<float>(static_cast<double>(request.y)+BaseOffset(types_[island].footY));
            const auto base=Allocate(noIsland,baseX,baseY,0,0);
            hooks_.place({noIsland,baseX,baseY,0,0,state_.localPlayer,base,1,0});
        }
    }
    hooks_.place({type,request.x,request.y,request.argument,request.direction,state_.localPlayer,main,1,quality});
    // 배치 경계가 타입 플래그를 바꿀 수 있으므로 현재 값으로 사제 이동을 판정한다. 빈 decoder의 첫 SID는 초기값 0이다.
    if (types_[type].flags1&kNeedsBuilder) hooks_.moveBuilder(state_.localPlayer,Sid{main.empty() ? std::uint16_t{} : main.front()},0,0);
    hooks_.send(9,{static_cast<std::uint8_t>(type),request.x,request.y,static_cast<std::uint16_t>(request.argument),
        static_cast<std::uint8_t>(request.direction),static_cast<std::uint8_t>(quality)});
}
// 어댑터가 같은 풀인지 확인할 때 실제 참조를 돌려준다.
const SidPool& RawConstructionLocal::Pool() const { return pool_; }
// factory와 배치가 같은 풀을 공유하는지 검사하고 두 실제 효과를 연결한다.
ConstructionLocalHooks MakeConstructionLocalHooks(const SidPool& pool,SquidFactory& factory,RawConstructionPlace& place,ConstructionLocalHooks hooks) {
    if (&pool!=&factory.Pool() || &pool!=&place.Pool()) throw std::invalid_argument("로컬 건설/factory/배치 SID 풀이 다릅니다");
    // 생성은 클라이언트 플래그를 그대로 전달하고 배치는 SID 목록을 호출 중에만 빌려 쓴다.
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
    hooks.place=[&place](const ConstructionPlaceRequest& request) { place.Place(request); };
    return hooks;
}
}
