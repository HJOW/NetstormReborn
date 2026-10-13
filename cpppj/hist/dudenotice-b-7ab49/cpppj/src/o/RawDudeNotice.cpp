// 실제task/선택순서와float저장·double시각 및 대피 속도 원복의원본절삭을보존한다.
#include "o/RawDudeNotice.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 Dude task WORD와 가까운거주지 최초거리 및스냅float 바이어스다.
constexpr std::uint16_t kNoticeTask=0x5107,kFallTask=0x5105;
constexpr float kFarDistance=9999.0f,kSnapBias=0.9999f;
// 원본비정렬little endian DWORD/WORD/BYTE를 읽는다.
std::uint32_t ReadDude(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정된쓰기폭과같은 낮은바이트만조합한다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 현재좌표float의원본비트를 읽는다.
float DudeCoordinate(std::span<const std::uint8_t> raw,std::size_t offset) { return std::bit_cast<float>(ReadDude(raw,offset)); }
}
// 현재실제타입/프레임/속도배열의판본별전체크기와거주지count를검사한다.
RawDudeNotice::RawDudeNotice(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,std::span<DudeNoticeTypeData> speeds,const SquidPostPopList& residences,DudeNoticeState state,DudeNoticeHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),speeds_(speeds),residences_(residences),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || frames.size()!=count || speeds.size()!=count) throw std::invalid_argument("Dude notice 타입/판본배열 오류");
    static_cast<void>(residences.Items());
}
// free/contained는정상Notice의자산계약밖이며dead는반응없이반환하기위해조회한다.
std::span<const std::uint8_t> RawDudeNotice::Slot(Sid sid) const {
    if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("Dude notice SID 오류");const auto raw=pool_.Slot(sid);
    if ((raw[11]&9) || raw[10]>=types_.size()) throw std::logic_error("Dude notice 현재자산 상태/타입 오류");return raw;
}
// 최신contained Form부터 실제type/dead를검사하며원본next 순서로 찾는다.
Sid RawDudeNotice::Process(Sid sid,std::uint32_t type) const {
    auto current=static_cast<std::uint16_t>(ReadDude(Slot(sid),6,2));std::size_t visited=0;
    // C++에서는손상된체인에대한무한루프를거부하고정상원본순서는유지한다.
    while (current) {
        if (current>=pool_.Capacity() || ++visited>pool_.Capacity()) throw std::logic_error("Dude notice 종속체인 오류");const auto raw=pool_.Slot(Sid{current});
        if (raw[10]==type && !(raw[11]&2)) return Sid{current};current=static_cast<std::uint16_t>(ReadDude(raw,4,2));
    }
    return {};
}
// 거의올림은float입력+float0.9999를double정밀도로절삭한후원본float에저장한다.
std::array<float,2> RawDudeNotice::Snap(std::array<float,2> point) {
    // 두현재좌표를원본CRT 정상int32범위에서만절삭한다.
    for (auto& value:point) { const double wide=static_cast<double>(value)+kSnapBias;if (!std::isfinite(wide) || wide<std::numeric_limits<std::int32_t>::min() || wide>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("Dude notice 스냅좌표 오류");value=static_cast<float>(static_cast<std::int32_t>(wide)); }
    return point;
}
// 실제영역탐색기와타입genus를사용하고buried/범위필터를수동대체하지않는다.
void RawDudeNotice::Around(float x,float y) const {
    const auto point=Snap({x-1.0f,y-1.0f});const int cx=static_cast<int>(point[0]),cy=static_cast<int>(point[1]);RawSquidFinder finder(pool_,hash_,types_);
    // 후보next는각Notice가바꾼raw상태이후에도원본finder의현재커서로전진한다.
    for (auto sid=finder.Begin({cx-4,cy-4,cx+4,cy+4});sid.value;sid=finder.Next()) {
        const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("Dude notice 후보타입 오류");if (types_[raw[10]].flags2&0x400) Notice(sid);
    }
}
// 현재owner/좌표를읽고각후보후best float저장을유지한다. 같은거리에서는앞선후보가남는다.
Sid RawDudeNotice::NearestResidence(Sid sid) const {
    const auto raw=Slot(sid);const auto owner=raw[pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U];const double x=DudeCoordinate(raw,14),y=DudeCoordinate(raw,18);float best=kFarDistance;Sid result;
    // 실제거주지active DWORD 목록을현재순서대로만조회한다.
    for (const auto number:residences_.Items()) {
        if (!number || number>=pool_.Capacity()) throw std::out_of_range("Dude notice 거주지 SID 오류");const auto candidate=pool_.Slot(Sid{static_cast<std::uint16_t>(number)});
        if (candidate[pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U]!=owner) continue;const double dx=x-DudeCoordinate(candidate,14),dy=y-DudeCoordinate(candidate,18);const double distance=std::sqrt(dx*dx+dy*dy);
        if (distance<best) { best=static_cast<float>(distance);result={static_cast<std::uint16_t>(number)}; }
    }
    return result;
}
// 일반Gun center와달리x 정수+0.5 및타입높이/현재genus로y를보정한다.
std::array<float,2> RawDudeNotice::ResidencePoint(Sid sid) const {
    const auto raw=Slot(sid);const auto& type=types_[raw[10]];const float x=DudeCoordinate(raw,14);if (!std::isfinite(x) || x<0 || x>256) throw std::out_of_range("Dude notice 거주지좌표 오류");float y=DudeCoordinate(raw,18)-static_cast<float>(type.footY);
    if (!(type.flags2&state_.raisedGenusMask)) y-=0.5f;else if (type.flags2&0x10040000) y+=1.0f;return {static_cast<float>(static_cast<int>(x))+0.5f,y};
}
// 원본은기존목적지를비교한후speed정수백업을만들고새경로요청앞에서만2배로쓴다.
bool RawDudeNotice::Move(Sid sid,std::array<float,2> destination,bool quickly) const {
    Slot(sid);
    if (Process(sid,state_.pathProcessType).value) {
        if (!hooks_.pathEndpoint) throw std::logic_error("Dude notice 실제기존경로 endpoint 미연결");const auto endpoint=hooks_.pathEndpoint(sid);
        if (!endpoint) throw std::logic_error("Dude notice 현재경로 객체조회 오류");if (*endpoint==Snap(destination)) return true;
    }
    if (!hooks_.startPath) throw std::logic_error("Dude notice 실제DudePath24 생성 미복원");const auto type=Slot(sid)[10];const float speed=speeds_[type].speed;
    if (!std::isfinite(speed) || static_cast<double>(speed)<std::numeric_limits<std::int32_t>::min() || static_cast<double>(speed)>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("Dude notice 속도CRT 정상범위 오류");const auto saved=static_cast<std::int32_t>(speed);if (quickly) speeds_[type].speed=speed+speed;
    const bool made=hooks_.startPath(sid,destination[0],destination[1]);speeds_[type].speed=static_cast<float>(saved);
    if (made) {
        const auto raw=Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto frame=ReadDude(raw,patch ? 36U : 34U,patch ? 4U : 1U);const auto codes=frames_[raw[10]].Codes();if (frame>=codes.size()) throw std::out_of_range("Dude notice 현재단계frame 오류");
        if (codes[frame].side=='I') { if (!hooks_.setFrame) throw std::logic_error("Dude notice 실제A프레임쓰기 미연결");hooks_.setFrame(sid,frames_[Slot(sid)[10]].Run('A').first,0); }
    }
    return made;
}
// 상태/Regular→Burning→최근같은owner거주지→원본대피 요청 순서를유지한다.
void RawDudeNotice::Notice(Sid sid) const {
    const auto raw=Slot(sid);const auto task=ReadDude(raw,12,2);if ((raw[11]&2) || task==kNoticeTask || task==kFallTask) return;
    if (!hooks_.taskRegular) throw std::logic_error("Dude notice 현재Regular조회 미연결");auto* regular=hooks_.taskRegular(sid);
    if (!regular) { if (!hooks_.addTaskRegular) throw std::logic_error("Dude notice 실제Regular생성 미연결");regular=hooks_.addTaskRegular(sid);if (!regular) throw std::logic_error("Dude notice 실제Regular생성 실패"); }
    regular->ResetTask(kNoticeTask,state_.clock.now);auto bytes=pool_.AllocatedBytes(sid);bytes[12]=static_cast<std::uint8_t>(kNoticeTask);bytes[13]=static_cast<std::uint8_t>(kNoticeTask>>8);
    if (pool_.Slot(sid)[10]==state_.bridgeType) { if (!hooks_.notifySurface) throw std::logic_error("Dude task 실제표면통지 미연결");hooks_.notifySurface(sid); }
    if (!Process(sid,state_.burningProcessType).value) { if (!hooks_.createBurning) throw std::logic_error("Dude notice 실제Burning14 생성 미복원");hooks_.createBurning(sid); }
    const auto destination=NearestResidence(sid);if (!destination.value) return;
    const auto target=pool_.Slot(destination);if (target[11]&3) { if (pool_.Edition()==OriginalEdition::Patch1078) throw std::logic_error("Dude notice 대피표적 alive 원본 assert");return; }
    static_cast<void>(Move(sid,ResidencePoint(destination),true));
}
// 실제어댑터를다른월드풀과합성하지않도록현재풀을제공한다.
const SidPool& RawDudeNotice::Pool() const { return pool_; }
// 현재Regular는기존Host객체그대로이며ResetTask는공유좁은API를사용한다.
DudeNoticeHooks MakeDudeNoticeHostHooks(const SidPool& pool,SquidProcessHost& host,DudeNoticeHooks hooks) {
    if (&pool!=&host.Pool()) throw std::invalid_argument("Dude notice/Host SID풀 불일치");hooks.taskRegular=[&host](Sid sid) { return host.FindEventMasked(sid,0xfff0,0x5100); };hooks.addTaskRegular=[&host](Sid sid) { return host.AddRegular(sid,0x5107,5.0f,0x50); };return hooks;
}
}
