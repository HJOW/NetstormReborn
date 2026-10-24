// Client의 실제 배열을 Session보다 오래 보관하고 같은 물리를 두 월드에 중복 설치하지 않는다.
#include "client/RawSurfaceWorld.h"
#include "client/GameAssets.h"
#include "client/SquidRenderer.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kCapacity=32768; // 기존 실제 월드의 SID 풀 크기이며 다음 대여에서 재확보하지 않는다.
constexpr std::size_t kCells=65536; // 원본256×256 spot 배열의 실제 바이트 수다.
}
// 참조 배열을 원본 자산에서 최초 한 번만 복사해 이후 주소를 고정한다.
RawSurfaceStorage::RawSurfaceStorage(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> inputTypes,std::span<const o::RiftTypeFrames> inputFrames,std::span<const o::SquidDisplayShape> inputShapes,const GameAssets* source)
    :types(inputTypes.begin(),inputTypes.end()),frames(inputFrames.begin(),inputFrames.end()),shapes(inputShapes.begin(),inputShapes.end()),codes(types.size()),pool(edition,kCapacity,true),spots(kCells),source_(source) {
    if (frames.size()!=types.size() || shapes.size()!=types.size()) throw std::invalid_argument("실제 월드 저장소의 자산 배열 크기 불일치");
    for (std::size_t type=0;type<frames.size();++type) codes[type].assign(frames[type].Codes().begin(),frames[type].Codes().end()); // 같은 타입의 실제 프레임 코드만 최초 보관한다.
}
// 동시 설치를 거부한 뒤에만 이전 물리를 초기화한다. TYPE/삭제 기록과 배열 주소는 유지한다.
std::unique_ptr<RawSurfaceStorage::Use> RawSurfaceStorage::Acquire() {
    if (borrowed_) throw std::logic_error("실제 월드 저장소 중복 대여");
    auto use=std::unique_ptr<Use>(new Use(*this));
    if (generation_) { pool.Reset(0);hash.Reset();std::fill(spots.begin(),spots.end(),std::uint8_t{}); }
    borrowed_=true;++generation_;return use;
}
// 대여 획득 전 생성한 반납 경계에는 별도 게임 효과가 없다.
RawSurfaceStorage::Use::Use(RawSurfaceStorage& storage):storage_(storage) {}
// 월드 모듈이 사라진 뒤에만 다음 월드가 같은 물리를 다시 사용할 수 있다.
RawSurfaceStorage::Use::~Use() { storage_.borrowed_=false; }
// 현재 대여 여부만 읽고 초기화나 할당을 추가하지 않는다.
bool RawSurfaceStorage::Borrowed() const { return borrowed_; }
// 호스트 검증용이며 게임 규칙의 프레임/세대로 사용하지 않는다.
std::uint64_t RawSurfaceStorage::Generation() const { return generation_; }
// 다른 자산의 뷰를 같은 실제 Client 저장소로 잘못 연결하지 않도록 한다.
const GameAssets* RawSurfaceStorage::Source() const { return source_; }
// 실제 TYPE70부터 파일 프레임을 같은 배열에 놓고 기존 SquidRenderer의 SHP 뷰를 사용한다.
std::shared_ptr<RawSurfaceStorage> MakeRawSurfaceStorage(const GameAssets& assets) {
    const auto& table=assets.TypeTable().Types();std::vector<o::RiftTypeFrames> frames(table.size(),o::RiftTypeFrames({})); // 누락된 내장 타입은 기존과 같은 빈 프레임 표다.
    for (const auto& type:assets.Types()) frames.at(o::kFirstAssetTypeNumber+type.block)=type.definition.FrameTable(); // 실제 archive 블록의 프레임 표를 보관한다.
    return std::make_shared<RawSurfaceStorage>(assets.Edition(),table,frames,SquidRenderer::Shapes(assets,assets.Edition()),&assets);
}
}
