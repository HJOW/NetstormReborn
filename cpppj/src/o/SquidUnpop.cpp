// 두 판본의 현재 진단 복귀/부분 공간 변경을 같은 실제 해제 부모로 연결한다.
#include "o/SquidUnpop.h"
#include "o/RawMetadisplayDirty.h"
#include "o/Squid.h"
#include "o/SquidDisplay.h"
#include "o/RawGraph.h"
#include "o/RawBuildingSpace.h"
#include <stdexcept>

namespace netstorm::o {
// 새 미니맵이 이미 빌린 관측자는 이전 처리기 반납 뒤에도 같은 dirty 큐에 연결돼 있어야 한다.
bool SquidUnpop::ReleaseMetadisplayDirty(const RawMetadisplayDirty* expected) { if (!expected || metadisplay_!=expected) return false;metadisplay_=nullptr;return true; }
// 삭제/프레임 갱신도 같은 월드의 미니맵 큐만 사용할 수 있다.
void SquidUnpop::SetMetadisplayDirty(const RawMetadisplayDirty* dirty) {
    if (dirty && &dirty->Pool()!=&pool_) throw std::invalid_argument("Unpop/미니맵 큐 SID 풀 불일치");
    metadisplay_=dirty;
}
namespace {
// 선택한 가상 경로의 지원 비트. update8c가 update88로 꼬리 호출하는 것까지 확인했다.
struct DisplayVtable { /* vtable: 원본 객체의 가상 함수 표 주소. */ std::uint32_t vtable; /* mask: 대상 비트 또는 투명 영역을 선택하는 마스크. */ std::uint8_t mask; };
#include "UnpopVtables.inc"
// 정렬되지 않은 little endian raw 값을 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 필요한 폭만 조합하여 CD 이웃 필드를 읽지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
}
// 범위가 정확한 공유 지도만 받아 부분 지도 밖 쓰기를 막는다.
SquidUnpop::SquidUnpop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,SquidDisplay* display)
    :pool_(pool),hash_(hash),spots_(spots),display_(display) {
    if (spots_.size()!=kWorldCells*kWorldCells) throw std::invalid_argument("Unpop spot 지도 크기 오류");
}
// 잘못된 SID는 풀 밖 접근 전에 거부한다. 예측 머리도 원본 Take/Unpop 범위에서는 허용된다.
std::span<std::uint8_t> SquidUnpop::Bytes(Sid sid) {
    return std::span(pool_.bytes_).subspan(pool_.Offset(sid),pool_.layout_.stride);
}
// 코드 주소는 비교용 기록값이다. 실제 공통 표시 연결도 이 지원 표를 따른다.
bool SquidUnpop::SupportsDisplay(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags) {
    if (edition==OriginalEdition::Patch1078 && vtable==0x50c9f0U) return true; //보호 PE의88=4787c0/8c=4ad670(JMP현재88)을 실제 메타 장치로 분배한다.
    // 새 독립 PE 관측으로 복원한 마나88/8c다. 과거 공통 가상 경로 metadata는 그대로 보존한다.
    if (edition==OriginalEdition::Patch1078 && (vtable==0x50c3b0 || vtable==0x509810)) return true; // 105의45ac90 부분 갱신도 별도 몸체로 복원했다.
    const auto values=edition==OriginalEdition::Patch1078 ? std::span<const DisplayVtable>(kPatchDisplayVtables) :
        std::span<const DisplayVtable>(kCdDisplayVtables);
    const auto mask=(flags&0x2000)!=0 ? 2U : 1U;
    // 확인한 가상 함수가 아니면 미복원 override를 적용하지 않는다.
    for (const auto& value:values) if (value.vtable==vtable) return (value.mask&mask)!=0;
    return false;
}
// raw 풀 소유자를 읽기 전용으로 확인한다.
const SidPool& SquidUnpop::Pool() const { return pool_; }
// 표시를 생략한 Unpop으로 옛 영역을 남기는 연결을 구성 단계에서 거부한다.
void SquidUnpop::ValidateFrameBinding(const SidPool& pool,const SquidDisplay& display,const SquidHash& hash,
    std::span<const std::uint8_t> spots) const {
    if (&pool!=&pool_ || &display!=display_ || &hash!=&hash_ || spots.data()!=spots_.data() || spots.size()!=spots_.size())
        throw std::invalid_argument("프레임 Unpop 풀/표시/공간 연결 불일치");
}
// 크기뿐 아니라 동일한 공간 인스턴스의 연결을 요구한다.
void SquidUnpop::ValidateGraph(const RawGraph& graph) const { graph.ValidateSpace(hash_,spots_); }
// 원본 표면 지도와 같은 해시/풀의 몸체만 등록한다.
void SquidUnpop::SetBuildingSpace(const RawBuildingSpace* space) {
    if (space) space->ValidateSpace(pool_,hash_);
    building_=space;
}
// 일반 공간 경로의 상태·spot·체인 쓰기 뒤 선택 연결한 공통 표시를 원본 순서로 적용한다.
void SquidUnpop::Unpop(Sid sid,const RiftTypeRecord& type,std::uint32_t flags) {
    SquidUnpopParentHooks effects;effects.assertion=diagnostics_.assertion;
    effects.type=[this,&type](std::uint8_t number) -> const RiftTypeRecord& { if (diagnostics_.types.empty()) return type;if (number>=diagnostics_.types.size()) throw std::out_of_range("Unpop 현재 TYPE 범위");return diagnostics_.types[number]; }; // 독립 입력은 기존 타입 참조, 실제 World는 같은 현재 전체 TYPE 표를 사용한다.
    effects.miniMap=[this](Sid object,std::uint32_t) { if (metadisplay_) metadisplay_->Enqueue(object); }; // 원문 활성 큐가 없으면 helper도 자연 반환한다.
    effects.side=[this](int x,int y) { if (!building_) throw std::logic_error("Unpop 실제 건물 옆면 미연결");building_->UnpopSide(x,y); }; // 선택된 옆면 종단의 부재는 앞선 void/spot 쓰기를 되돌리지 않는다.
    effects.contained=containedUnpop_;
    effects.update=[this](Sid object,std::uint32_t updateFlags) { const auto raw=Bytes(object);if (!SupportsDisplay(pool_.Edition(),Read(raw,0,4),updateFlags)) throw std::logic_error("Unpop의 현재 파생 표시 갱신 미복원");if (display_) display_->Update(raw,updateFlags); }; // 해시/지도 쓰기 뒤 현재 VT의 실제 표시 경로를 선택한다.
    RunSquidUnpopParent(pool_,hash_,spots_,sid,flags,diagnostics_.checking,effects);
}
}
