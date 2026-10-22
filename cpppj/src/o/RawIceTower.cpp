// 원본은150에공통Damageable처리를추가하지않으며삭제전후의현재값을읽는다.
#include "o/RawIceTower.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kGrowthSeconds=40.0F; //5069bc의 원래 전체성장시간이다.
constexpr std::uint32_t kGrowthEvent=0x3a6; //150만 처리하는 예약번호다.
// 비정렬 raw의 지정바이트폭을 주변값과 섞지 않고 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) { std::uint32_t value{};for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[at+index])<<(index*8);return value; } //원본little endian이다.
// 현재DWORD 프레임만 쓰며 이 대입에서는 표시 갱신을 추가하지 않는다.
void WriteFrame(SidPool& pool,Sid sid,std::uint32_t value) { auto raw=pool.AllocatedBytes(sid);for (std::size_t index=0;index<4;++index) raw[36+index]=static_cast<std::uint8_t>(value>>(index*8)); } //네바이트 전체대입이다.
// 필요한 효과 경계가 없으면 빈성공으로 대체하지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("아이스 타워 실제 하위 몸체 미연결");return function; }
// 원본TYPE+114의 현재프레임수이며 부분코드표나 다른TYPE을 섞지 않는다.
std::int32_t FrameCount(const SidPool& pool,std::span<const RiftTypeFrames> frames,Sid sid) { const auto type=pool.Slot(sid)[10];if (type>=frames.size()) throw std::out_of_range("아이스 현재 프레임TYPE 오류");return static_cast<std::int32_t>(frames[type].Codes().size()); }
}
// 최초예약의 할당실패는 공통후처리를 막지 않으며extra/editor관문을 추가하지 않는다.
void PostPopGrowingIce(const SidPool& pool,std::span<const RiftTypeFrames> frames,Sid parent,std::uint32_t flags,
    const std::function<bool()>& reserve,const std::function<void(Sid,std::uint32_t,float)>& regular,const std::function<void(Sid,std::uint32_t)>& base) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188) throw std::invalid_argument("성장형 아이스 후처리 판본/프레임 오류");
    if ((flags&1U) && Need(reserve)()) Need(regular)(parent,kGrowthEvent,kGrowthSeconds/static_cast<float>(FrameCount(pool,frames,parent)));Need(base)(parent,flags);
}
// 소유자/공간/프레임과 전투상태를 현재 세션에 연결한다.
RawIceTower::RawIceTower(SidPool& pool,std::span<const RiftTypeFrames> frames,const IceTowerTypes& types,
    const bool& battle,const bool& boss,const std::uint32_t& loadingDepth,IceTowerHooks hooks)
    :pool_(pool),frames_(frames),types_(types),battle_(battle),boss_(boss),loadingDepth_(loadingDepth),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188) throw std::invalid_argument("아이스 타워 판본/프레임 오류");
}
// 각효과뒤의 현재 x/y를 원본 float비트로 읽는다.
std::array<float,2> RawIceTower::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 실제188프레임과같은Host예약/공통장부를성장형후처리에전달한다.
void RawIceTower::PostPop(Sid parent,std::uint32_t flags) const { PostPopGrowingIce(pool_,frames_,parent,flags,hooks_.reserveRegular,hooks_.regular,hooks_.basePop); }
// 완성형은먼저공간을비우고현재WORD/QA/owner/좌표의순서로그대로성장형에전달한다.
void RawIceTower::PostDestroy(Sid parent,std::uint32_t flags) const {
    const auto raw=pool_.Slot(parent);
    if (raw[10]==types_.ice && !(raw[40]&9U) && !(flags&0x200000U) && !loadingDepth_) {
        const auto point=Position(parent);const auto surface=Need(hooks_.grid)(point[0],point[1]);
        if (surface.value && pool_.Slot(surface)[10]==types_.noIsland && (flags&0x8000000U)) {
            const auto at=Position(parent);Need(hooks_.dropIsland)(at[0],at[1],flags,pool_.Slot(parent)[34]);Need(hooks_.damageablePost)(parent,flags);return;
        }
        if (battle_) {
            Need(hooks_.unpop)(parent,0);const auto born=Need(hooks_.create)(types_.growing,0);
            Need(hooks_.word)(born,static_cast<std::uint16_t>(Read(pool_.Slot(parent),12,2)));pool_.AllocatedBytes(born)[35]=pool_.Slot(parent)[35];
            Need(hooks_.owner)(born,pool_.Slot(parent)[34]);WriteFrame(pool_,born,0);const auto at=Position(parent);Need(hooks_.pop)(born,at[0],at[1],0);
        }
    }
    Need(hooks_.damageablePost)(parent,flags);
}
// 성장형은extra9와두삭제비트만보고현재섬의존재를조회한다.
void RawIceTower::GrowingPostDestroy(Sid parent,std::uint32_t flags) const {
    if (!(pool_.Slot(parent)[40]&9U) && (flags&0x8200000U)) {
        const auto point=Position(parent);if (Need(hooks_.genusAt)(point[0],point[1],0x1000000U).value) { const auto at=Position(parent);Need(hooks_.dropIsland)(at[0],at[1],flags,pool_.Slot(parent)[34]); }
    }
    Need(hooks_.basePost)(parent,flags);
}
// count는사용하지않고원래프레임DWORD의감김증가와signed비교만수행한다.
float RawIceTower::Handle(Sid parent,std::uint32_t event,std::uint32_t count,float payload) const {
    (void)count;if (event!=kGrowthEvent) return payload;
    if (!(pool_.Slot(parent)[40]&0x20U)) {
        const auto next=Read(pool_.Slot(parent),36)+1U;const auto last=static_cast<std::uint32_t>(FrameCount(pool_,frames_,parent))-1U;
        if (std::bit_cast<std::int32_t>(next)<std::bit_cast<std::int32_t>(last)) Need(hooks_.setFrame)(parent,std::bit_cast<std::int32_t>(next),0);
        else if (boss_) {
            const auto born=Need(hooks_.create)(types_.ice,0);Need(hooks_.owner)(born,pool_.Slot(parent)[34]);const auto point=Position(parent);
            Need(hooks_.destroy)(parent,0);Need(hooks_.pop)(born,point[0],point[1],0);return -1.0F;
        }
    }
    return static_cast<float>(static_cast<double>(kGrowthSeconds)/FrameCount(pool_,frames_,parent));
}
}
