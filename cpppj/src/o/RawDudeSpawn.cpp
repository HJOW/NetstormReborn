// 실제 Dude 생성/최초 Pop의 판본별 frame 폭과 population 등록을 복원한다.
#include "o/RawDudeSpawn.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 Dude 생성자가 기록하는 두 가상 표다. 패치508cd0은 표 시작에서8바이트 뒤다.
constexpr std::uint32_t kPatchDudeVtable=0x00508cc8,kCdDudeVtable=0x005072f0;
// 최초 등록 때 raw WORD만 쓰는 idle 작업 번호다. Regular 생성과 구분한다.
constexpr std::uint32_t kIdleTask=0x5101;
template<class Function> Function DudeNeed(const Function& function) { if (!function) throw std::logic_error("Dude의 선택된 실제 하위 미연결");return function; } // 도달한 장치만 요구하고 현재 callable의 호출 수명을 유지한다.
std::span<std::uint8_t> DudeSlot(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 실제 free/contained와 무관한 물리 슬롯을 읽고 쓴다.
std::uint32_t DudeCapacity(const SquidPostPopList& population) { const auto capacity=population.Capacity();if (capacity>0xffffffffU) throw std::out_of_range("Dude population DWORD 용량 물리 범위");return static_cast<std::uint32_t>(capacity); } // 현재 용량을 원본 signed 비교에 사용할 DWORD로 읽는다.
// raw 가상 표 DWORD를 정렬/호스트 포인터 해석 없이 읽는다.
std::uint32_t DudeVtable(std::span<const std::uint8_t> raw) {
    std::uint32_t value=0;
    // 원본 DWORD의 낮은 바이트부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[index])<<(index*8);
    return value;
}
// 원본 signed count/capacity의 정상 비음수 목록 계약을 확인한다.
void ValidatePopulation(const SquidPostPopList& population) {
    if (population.entries.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::out_of_range("Dude population signed capacity 오류");
    static_cast<void>(population.Items());
}
}
// 실제 factory/owner의 풀을 확인하며 Pop 미연결은 원본 호출 시점에 거부한다.
RawDudeSpawn::RawDudeSpawn(SidPool& pool,SquidFactory& factory,SquidOwner& owner,DudeSpawnState& state,DudeSpawnHooks hooks)
    :pool_(pool),factory_(factory),owner_(owner),state_(state),hooks_(std::move(hooks)) {
    if (&factory_.Pool()!=&pool_ || &owner_.Pool()!=&pool_) throw std::invalid_argument("Dude spawn/factory/owner SID 풀 불일치");
}
// 실제 PE 인자는 x/y/owner의3개이며 잘못된 unaff_retaddr 디컴파일 표현을 사용하지 않는다.
Sid RawDudeSpawn::Spawn(float x,float y,std::uint32_t owner) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (patch) { const auto used=state_.population.Count();const auto capacity=DudeCapacity(state_.population);if (std::bit_cast<std::int32_t>(used)>=std::bit_cast<std::int32_t>(capacity) || state_.blocked) return {}; }
    else { ValidatePopulation(state_.population);if (state_.population.count>=state_.population.entries.size() || state_.blocked) return {};if (!std::isfinite(x) || !std::isfinite(y)) throw std::invalid_argument("Dude spawn 유한 좌표 오류"); }
    const auto sid=factory_.Create(state_.dudeType,2);
    auto raw=patch ? DudeSlot(pool_,sid) : pool_.AllocatedBytes(sid);
    if (!patch && (raw[10]!=state_.dudeType || DudeVtable(raw)!=kCdDudeVtable)) throw std::logic_error("Dude spawn 실제 생성자 타입/가상 표 오류");
    // 패치는 frame DWORD 전체, CD는 frame BYTE만0으로 써 이웃 payload를 보존한다.
    if (patch) { raw[36]=0;raw[37]=0;raw[38]=0;raw[39]=0; } else raw[34]=0;
    owner_.Set(sid,owner);
    DudeNeed(hooks_.pop)(sid,x,y,0);return sid;
}
// 같은 최초 Pop/list 연결인지 확인하기 위한 읽기 전용 참조다.
const SidPool& RawDudeSpawn::Pool() const { return pool_; }
// 같은 현재 Run/population을 참조하며 CD의 기존 구성 사전 검사를 유지한다.
RawDudePostPop::RawDudePostPop(SidPool& pool,std::span<const RiftTypeFrames> frames,DudeSpawnState& state,DudePostPopHooks hooks,const std::uint32_t& bridgeType)
    :pool_(pool),frames_(frames),state_(state),bridgeType_(bridgeType),hooks_(std::move(hooks)) {
    if (pool.Edition()==OriginalEdition::Cd1072 && (!hooks_.validateBase || !hooks_.base)) throw std::invalid_argument("Dude postPop 공통 몸체 미연결");
}
// PATCH 실제 가상 표 진입에 현재 TYPE나 할당 상태를 추가 관문으로 만들지 않는다.
bool RawDudePostPop::Handles(Sid sid) const {
    const auto raw=pool_.Slot(sid);return (pool_.Edition()==OriginalEdition::Patch1078 || raw[10]==state_.dudeType) && DudeVtable(raw)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchDudeVtable : kCdDudeVtable);
}
// 최초 population/frame 쓰기 뒤 필요한 공통 효과를 공간 변경 전에 검증한다.
void RawDudePostPop::Validate(Sid sid,std::uint32_t flags) const {
    static_cast<void>(pool_.AllocatedBytes(sid));
    if (!Handles(sid) || pool_.Slot(sid)[10]!=state_.dudeType) throw std::invalid_argument("Dude postPop 실제 타입/가상 표 오류");
    if (flags&1) {
        ValidatePopulation(state_.population);
        const auto type=pool_.Slot(sid)[10];if (type>=frames_.size()) throw std::out_of_range("Dude postPop 현재 Run 표 오류");
        const auto first=frames_[type].Run('I').first;
        if (first<0 || static_cast<std::size_t>(first)>=frames_[type].Codes().size()) throw std::out_of_range("Dude postPop 실제 I 프레임 미연결");
        if (!hooks_.setFrame) throw std::logic_error("Dude postPop 실제 프레임 쓰기 미연결");
    }
    DudeNeed(hooks_.validateBase)(sid,flags);
}
// 처음만 idle/등록/I 프레임을 쓰며 가득 찬 population에도 프레임/공통 호출은 유지한다.
void RawDudePostPop::PostPop(Sid sid,std::uint32_t flags) const {
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        if (flags&1U) {
            auto raw=DudeSlot(pool_,sid);raw[12]=static_cast<std::uint8_t>(kIdleTask);raw[13]=static_cast<std::uint8_t>(kIdleTask>>8);
            if (pool_.Slot(sid)[10]==bridgeType_) DudeNeed(hooks_.notifySurface)(sid);
            const auto used=state_.population.Count();const auto capacity=DudeCapacity(state_.population); // 원문의 cmp에 앞선 index는 보존하고 배열/개수는 도달 위치에서 다시 읽는다.
            if (std::bit_cast<std::int32_t>(used)<std::bit_cast<std::int32_t>(capacity)) {
                if (std::bit_cast<std::int32_t>(used)<0) throw std::out_of_range("Dude population 음수 저장 위치");const auto storage=state_.population.Storage();if (used>=storage.size()) throw std::out_of_range("Dude population 실제 배열 저장 범위");storage[used]=sid.value;state_.population.SetCount(state_.population.Count()+1U);
            }
            const auto type=pool_.Slot(sid)[10];if (type>=frames_.size()) throw std::out_of_range("Dude postPop 현재 Run 표 물리 범위");const auto first=frames_[type].Run('I').first;DudeNeed(hooks_.setFrame)(sid,first,0); // TYPE+150의 현재 값은 코드 개수로 사전 거부하지 않고 실제 SetFrame에 전달한다.
        }
        DudeNeed(hooks_.base)(sid,flags);return;
    }
    Validate(sid,flags);
    if (flags&1) {
        auto raw=pool_.AllocatedBytes(sid);raw[12]=static_cast<std::uint8_t>(kIdleTask);raw[13]=static_cast<std::uint8_t>(kIdleTask>>8);
        if (state_.population.count<state_.population.entries.size()) state_.population.entries[state_.population.count++]=sid.value;
        hooks_.setFrame(sid,frames_[pool_.Slot(sid)[10]].Run('I').first,0);
    }
    hooks_.base(sid,flags);
}
// 공유 Pop의 가상 분배와 직접 소비자의 풀 검증에 사용한다.
const SidPool& RawDudePostPop::Pool() const { return pool_; }
// 기존 실제 프레임 쓰기와 비가상 공통 후처리의 전체 몸체를 재사용한다.
DudePostPopHooks MakeDudePostPopHooks(const SidPool& pool,SquidFrame& frame,SquidPostPop& base) {
    if (&frame.Pool()!=&pool || &base.Pool()!=&pool) throw std::invalid_argument("Dude postPop/Frame/Base SID 풀 불일치");
    return {
        [&frame](Sid sid,std::int32_t value,std::uint32_t flags) { frame.Set(sid,value,flags); },
        [&base](Sid sid,std::uint32_t flags) { base.ValidateBase(sid,flags); },
        [&base](Sid sid,std::uint32_t flags) { base.PostPopBase(sid,flags); }
    };
}
}
