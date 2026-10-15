// local 난수 소비·BYTE spot·현재 타입/자원 재조회와 생성 뒤 순서를 보존한다.
#include "o/RawLocalGeyser.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kPlayerStride=0xb4,kPlayerFlags=0x50,kPlayerRegion=0x78; // 실제 PATCH Player 필드 위치다.
constexpr std::uint32_t kPredictableSeed=0xfed,kAttemptLimit=100,kRecentLimit=5; // 원본 local시드/생성별 최대시도/최근목록 상한이다.
constexpr float kRadius=2.0F,kHalf=0.5F,kSpotBias=0.99989998340606689453125F; // ROM00501444/500ee4/500edc의 실제float다.
// 정렬되지 않은 현재 Player DWORD를 정확 네 BYTE로 읽는다.
std::uint32_t PlayerWord(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value=0;
    // 원본DWORD의 낮은BYTE부터 조합하며 주변 stale필드를 읽지 않는다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(bytes[offset+i])<<(8*i);return value;
}
}
// 생성 효과 전 판본/공유 물리 저장소/필수 하위를 확인한다. 실제 최근 목록의 기존 값은 유지한다.
RawLocalGeyser::RawLocalGeyser(const SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
    std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> players,SquidPostPopList& recent,
    LocalGeyserState& state,LocalGeyserHooks hooks)
    :pool_(pool),hash_(hash),spots_(spots),types_(types),frames_(frames),players_(players),recent_(recent),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=types.size() || spots.size()!=65536 || players.size()!=9*kPlayerStride ||
        recent.entries.size()<kRecentLimit || !hooks_.configInt || !hooks_.wallSeconds || !hooks_.playerBounds ||
        !hooks_.negativeQuantity || !hooks_.destroy || !hooks_.confirm || !hooks_.create || !hooks_.pop)
        throw std::invalid_argument("로컬 간헐천의 현재 원본 자료/실제 생성 경계 누락");
}
// _ftol의 범위 밖/NaN signed64 불확정값은 저DWORD0이다. 정상64비트 범위는0방향 절삭한다.
std::int32_t RawLocalGeyser::Truncate(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 같은 recurrence를 이미복원한 GameRandom으로 실행하며 signed limit의 자연0분기를 유지한다.
std::uint32_t RawLocalGeyser::Next(GameRandom& random,std::int32_t limit) { return limit>0 ? random.Next(static_cast<std::uint32_t>(limit)) : 0U; }
// 좌표를 먼저float로줄이지않고 원본선형주소의 물리범위만 검사한다.
std::uint8_t RawLocalGeyser::Spot(std::int32_t x,std::int32_t y) const {
    const auto index=static_cast<std::int64_t>(y)*256+x; // 실제 spot의 x+256*y 주소 계산이다.
    if (index<0 || index>=static_cast<std::int64_t>(spots_.size())) throw std::out_of_range("로컬 간헐천 spot 물리 범위");
    return spots_[static_cast<std::size_t>(index)];
}
// rectangle의 값형 snapshot은 현재 Player 처리 중 외부 사각형 변경과 분리한다.
bool RawLocalGeyser::TryCandidate(GameRandom& random,FortRandomRectangle rectangle,LocalGeyserPoint& point) const {
    return Candidate(random,rectangle,point,state_.emptyGeyserType);
}
// 부모생성자가ctor뒤덮어쓰는+ac=emptyType/+b0=1을포함하여실제로컬후보전체를실행한다.
bool RawLocalGeyser::Candidate(GameRandom& random,FortRandomRectangle rectangle,LocalGeyserPoint& point,std::uint32_t reuseType) const {
    const auto width=Truncate(static_cast<double>(rectangle.right)-rectangle.left),height=Truncate(static_cast<double>(rectangle.bottom)-rectangle.top); // 0041daf0의 현재 정수화한 폭/높이다.
    if (width!=0 && height!=0) {
        const auto dy=Next(random,height),dx=Next(random,width); // 원본은 높이 난수 다음 폭 난수를 소비한다.
        point.y=static_cast<float>(static_cast<double>(dy)+rectangle.top);point.x=static_cast<float>(static_cast<double>(dx)+rectangle.left);
    } else { point.x=rectangle.left;point.y=rectangle.top; }
    point.x=static_cast<float>(std::floor(static_cast<double>(point.x)+kHalf));point.y=static_cast<float>(std::floor(static_cast<double>(point.y)+kHalf));
    const float left=static_cast<float>(static_cast<double>(point.x)-kRadius),top=static_cast<float>(static_cast<double>(point.y)-kRadius); // 원본이한번저장한3×3첫칸이다.
    if (!(rectangle.left<=left && left<=rectangle.right && rectangle.top<=top && top<=rectangle.bottom)) return false;
    const auto firstX=Truncate(left),firstY=Truncate(top); // 원본조건통과뒤같은첫칸을ftol로읽는다.
    std::string pattern; // 원본32BYTE버퍼에순서대로모으는현재isle의프레임방향문자다.
    // 원본은x바깥/y안쪽이며끝점도포함하여최대3×3의현재spot을읽는다.
    for (std::int64_t x=firstX;static_cast<double>(static_cast<float>(x))<=point.x;++x) {
        // 첫거부칸에서즉시return하여다음칸이나난수를추가소비하지않는다.
        for (std::int64_t y=firstY;static_cast<double>(static_cast<float>(y))<=point.y;++y) {
            if (x<std::numeric_limits<std::int32_t>::min() || x>std::numeric_limits<std::int32_t>::max() ||
                y<std::numeric_limits<std::int32_t>::min() || y>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("로컬 간헐천 후보 정수 범위");
            const auto sx=static_cast<std::int32_t>(x),sy=static_cast<std::int32_t>(y); // 현재loop의원본signed좌표다.
            const auto spot=Spot(sx,sy); // 원본은typedFind호출전에이칸의BYTE를보존한다.
            if (reuseType) if (const auto existing=FindType(static_cast<float>(sx),static_cast<float>(sy),reuseType);existing.value) {
                const auto raw=pool_.Slot(existing);point.x=std::bit_cast<float>(PlayerWord(raw,14));point.y=std::bit_cast<float>(PlayerWord(raw,18));return true;
            }
            if (spot&0x3fU) return false;
            const auto surface=sx>=0 && sy>=0 && sx<256 && sy<256 ? hash_.Entries(0)[static_cast<std::size_t>(sy*256+sx)] : std::uint16_t{}; // 원본WORDgetter는범위밖0을읽는다.
            const auto raw=pool_.Slot(Sid{surface}); // SID0도원본처럼type0을조회한다.
            if (raw[10]==state_.isleType) {
                const auto frame=std::bit_cast<std::int32_t>(PlayerWord(raw,36)); // PATCH현재frameDWORD다.
                if (raw[10]>=frames_.size() || frame<0 || static_cast<std::size_t>(frame)>=frames_[raw[10]].Codes().size()) throw std::out_of_range("로컬 간헐천 isle프레임 물리 범위");
                auto side=frames_[raw[10]].Codes()[static_cast<std::size_t>(frame)].side; // 실제004ad680의현재첫코드BYTE다.
                if (side>='a' && side<='z') side=static_cast<std::uint8_t>(side-'a'+'A');pattern+=static_cast<char>(side);
            }
        }
    }
    const auto zero=pattern.find('\0');if (zero!=std::string::npos) pattern.resize(zero); // 원본stricmp의첫NUL종료를보존한다.
    return pattern!="FBICANGDH";
}
// emptyGeyser의 부류가 surface/bridge가 아니면 원본처럼hash0을 건너뛴다.
Sid RawLocalGeyser::FindEmpty(float x,float y) const {
    return FindType(x,y,state_.emptyGeyserType);
}
// 부모가보존한reuse타입과성공뒤의현재empty전역모두같은실제typedfinder를사용한다.
Sid RawLocalGeyser::FindType(float x,float y,std::uint32_t number) const {
    if (number>=types_.size()) throw std::out_of_range("로컬 간헐천 현재 empty타입 범위");
    RawSquidFinder finder(pool_,hash_,types_);const auto ix=Truncate(x),iy=Truncate(y);const auto flags=(types_[number].flags2&6U) ? 0U : 1U;
    // 같은raw타입을매후보마다읽고첫exact일치만반환한다.
    for (auto sid=finder.Begin({ix,iy,ix,iy},flags);sid.value;sid=finder.Next()) if (pool_.Slot(sid)[10]==number) return sid;
    return {};
}
// 각생성의100회실패는이요청전체를끝내고상위Player순회는원본처럼계속된다.
void RawLocalGeyser::Populate(std::int32_t quantity,GameRandom& random,FortRandomRectangle rectangle) const {
    if (!state_.boss || state_.fort) return;
    if (quantity<0) { hooks_.negativeQuantity();quantity=0; }
    recent_.count=0;
    const auto reuseType=state_.emptyGeyserType; // 원본ctor뒤finder+ac는이시점타입을한번보존한다.
    // 요청수량은값형인자이며실제생성성공마다한번감소한다.
    while (quantity!=0) {
        LocalGeyserPoint point;bool found=false;
        // local난수와현재spot을같은후보몸체에전달한다.
        for (std::uint32_t attempt=0;attempt<kAttemptLimit;++attempt) if (Candidate(random,rectangle,point,reuseType)) { found=true;break; }
        if (!found) return;
        if (const auto empty=FindEmpty(point.x,point.y);empty.value) hooks_.destroy(empty,0x400000);
        --quantity;
        const float x=static_cast<float>(static_cast<double>(point.x)-kRadius),y=static_cast<float>(static_cast<double>(point.y)-kRadius); // 원본confirm의별도단정도저장좌표다.
        if (!(Spot(Truncate(static_cast<double>(point.x)+kSpotBias),Truncate(static_cast<double>(point.y)+kSpotBias))&2U))
            hooks_.confirm({state_.noIslandType,x,y,0,0,0,0,0,0,0});
        const auto created=hooks_.create(state_.geyserType,0); // 실제factory실패를존재하는간헐천으로계산하지않는다.
        if (!created.value) throw std::logic_error("로컬 간헐천 실제 factory 생성 실패");
        hooks_.pop(created,point.x,point.y,0);
        if (std::bit_cast<std::int32_t>(recent_.count)<static_cast<std::int32_t>(kRecentLimit)) {
            if (recent_.count>=recent_.entries.size()) throw std::out_of_range("로컬 간헐천 최근목록 물리 범위");
            recent_.entries[recent_.count]=created.value;++recent_.count;
        }
        const float pool=static_cast<float>(state_.resourcePool); // 원본은TYPE조회전에현재int pool을float로한번저장한다.
        if (state_.geyserType>=types_.size()) throw std::out_of_range("로컬 간헐천 현재 cost타입 범위");
        state_.resourcePool=std::max(0,Truncate(static_cast<double>(pool)-types_[state_.geyserType].cost));
    }
}
// 원본은부모진입에서boss/fort로earlyreturn하지않으며각Populate가현재값을검사한다.
void RawLocalGeyser::Generate() const {
    GameRandom random(kPredictableSeed);
    if (hooks_.configInt("predictableGeysers",1)==0) random.SetState(RawFortFinishOwner::TimeSeed(hooks_.wallSeconds()));
    // 플레이어번호순으로현재flags/지역을읽으며앞선생성효과가바꾼뒤기록도반영한다.
    for (std::uint32_t owner=1;owner<=8;++owner) {
        const auto offset=owner*kPlayerStride; // 현재Player의원문기록시작이다.
        if (!(players_[offset+kPlayerFlags]&0x22U) || PlayerWord(players_,offset+kPlayerRegion)==127U) continue;
        const auto rectangle=hooks_.playerBounds(owner); // 원본은수량전역을읽기전에현재Player사각형을구한다.
        Populate(owner<=4 ? state_.firstPlayerQuantity : state_.lastPlayerQuantity,random,rectangle);
    }
}
// 실제같은raw풀/최근목록을상위바인딩에노출한다.
const SidPool& RawLocalGeyser::Pool() const { return pool_; }
SquidPostPopList& RawLocalGeyser::Recent() const { return recent_; }
}
