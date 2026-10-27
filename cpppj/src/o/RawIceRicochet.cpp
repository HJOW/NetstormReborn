// 원문의 float 저장 위치·현재 필드 재조회·후보 순서와 직접 FCOS/FSIN 각 입력을 보존한다.
#include "o/RawIceRicochet.h"
#include "o/RawArcherAnimation.h"
#include "o/SquidReward.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kSnapBias=0.99999F,kInverseTurn=1.0F/360.0F; // 실제00502310과0050b534의 binary32 상수다.
constexpr std::uint32_t kAirborne=0x120000; // 원문 후보의 두 비행 플래그다.
// 현재 비정렬 little endian DWORD/float 좌표를 읽는다.
std::uint32_t ReadIce(std::span<const std::uint8_t> raw,std::size_t at) {
    std::uint32_t value=0;
    // 필드의 네 바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[at+i])<<(i*8);
    return value;
}
// 원본 float 저장값과 좌표 쌍을 비트 그대로 읽는다.
float IceFloat(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(ReadIce(raw,at)); }
std::array<float,2> IcePoint(std::span<const std::uint8_t> raw,std::size_t at) { return {IceFloat(raw,at),IceFloat(raw,at+4)}; }
//00468e70은 정확히360인 값을 유지하며 음수 보정에서 float 역수를 사용한다.
float NormalizeIce(float angle) {
    if (angle<0) angle=static_cast<float>(static_cast<double>(angle)+(1-static_cast<int>(static_cast<double>(angle)*kInverseTurn))*360.0);
    return angle>360 ? static_cast<float>(std::fmod(static_cast<double>(angle),360.0)) : angle;
}
//00422230은 보정/절삭 후 float를 거쳐 다시 절삭한다. 유효 지도 점만 호출한다.
int IceCell(float value) { return static_cast<int>(static_cast<float>(static_cast<int>(static_cast<double>(value)+kSnapBias))); }
// 원본 BoardCoord 유효 범위이며 NaN 또한 거부한다.
bool ValidIce(std::array<float,2> point) { return point[0]>0 && point[0]<256 && point[1]>0 && point[1]<256; }
}
// 같은 패치판 자료와 공유 난수만 허용한다.
RawIceRicochet::RawIceRicochet(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const MissileTypeData> missileTypes,GameRandom& random,IceRicochetHooks hooks)
    :pool_(pool),types_(types),missileTypes_(missileTypes),random_(random),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || missileTypes.size()!=types.size()) throw std::invalid_argument("아이스 반사 PATCH/타입 자료 오류");
}
// 원문이 허용하는 저번호 서버 슬롯도 물리 저장소 안에서 읽는다.
std::span<const std::uint8_t> RawIceRicochet::Slot(Sid sid) const {
    if (sid.value>=pool_.Capacity()) throw std::out_of_range("아이스 반사 SID 범위");
    const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("아이스 반사 TYPE 범위");return raw;
}
// 실제 float 방향/두 난수/정규화 뒤 원본처럼 각을 변환하지 않고 삼각함수에 넣는다.
bool RawIceRicochet::Point(const MissileFields& fields,std::array<float,2>& output) const {
    const auto raw=Slot(fields.Parent());const auto current=IcePoint(raw,14),origin=IcePoint(fields.bytes,56);
    if (current==origin) return false;
    const auto direction=RawArcherAnimation::DirectionAngle(origin,current,pool_.Edition());
    const float sign=random_.Next(100)>49 ? -1.0F : 1.0F;
    const double offset=static_cast<double>(random_.Next(65536))*(1.0/65536.0)*120.0+30.0;
    const float angle=NormalizeIce(static_cast<float>(offset*sign+direction));
    const auto range=missileTypes_[raw[10]].range;
    output={std::clamp(static_cast<float>(std::cos(static_cast<double>(angle))*range+current[0]),1.0F,255.0F),std::clamp(static_cast<float>(std::sin(static_cast<double>(angle))*range+current[1]),1.0F,255.0F)};
    return true;
}
// 원본은 표적이 없어도 target0의 반사 Missile을 생성한다.
void RawIceRicochet::Generate(const MissileFields& fields) const {
    if (random_.Next(99)<=19) return;
    const auto display=Slot(fields.Parent());std::array<float,2> point;
    // 원문의 미초기화 지역점에 임의 좌표를 넣어 성공시키지 않는다. 명시 Point 호출은 출력 보존을 지원한다.
    if (!Point(fields,point)) throw std::logic_error("아이스 반사 변위0의 원본 미초기화 지역점 입력 미연결");
    if (!ValidIce(point)) return;
    if (!hooks_.begin || !hooks_.next || !hooks_.hasHitPoints) throw std::logic_error("아이스 반사 실제 Finder/HP 미연결");
    const int x=IceCell(point[0]),y=IceCell(point[1]);float bestDistance=10000.0F;Sid best;
    // 현재 flags2를 먼저 캡처하고 HP 호출 뒤 나머지 raw/표적을 다시 읽는다. 동률은 최초 후보를 유지한다.
    for (auto sid=hooks_.begin({x-1,y-1,x+1,y+1},0);sid.value;sid=hooks_.next()) {
        const auto raw=Slot(sid);const auto flags2=types_[raw[10]].flags2;
        if (!hooks_.hasHitPoints(sid) || (sid.value>=5 && sid.value<=14999) || (raw[40]&1) || (flags2&kAirborne) || sid==fields.Target()) continue;
        const auto candidate=IcePoint(raw,14);const double dx=static_cast<double>(candidate[0])-point[0],dy=static_cast<double>(candidate[1])-point[1];const double distance=std::sqrt(dx*dx+dy*dy);
        if (distance<bestDistance) { best=sid;bestDistance=static_cast<float>(distance); }
    }
    if (!hooks_.createProjectile) throw std::logic_error("아이스 반사 실제 후속 Missile 생성 미연결");
    GunProjectileRequest request;request.firing=fields.Parent();request.target=best;request.speed=IceFloat(fields.bytes,48);request.displayType=display[10];request.effectType=0;request.targetPosition=point;request.targetCenter={-10000,-10000};request.ricochet=1;request.firingPosition=IcePoint(display,14);hooks_.createProjectile(request);
}
// 각 시작은 원본처럼 전체 Finder 상태를 초기화하며 flags0의 일반 공간 체인을 사용한다.
IceRicochetHooks MakeIceRicochetWorldHooks(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,const SquidReward& reward,IceRicochetHooks hooks) {
    if (&pool!=&reward.Pool()) throw std::invalid_argument("아이스 반사 Finder/HP 풀 불일치");
    const auto finder=std::make_shared<RawSquidFinder>(pool,hash,types);
    hooks.begin=[finder](SquidSearchArea area,std::uint32_t flags) { return finder->Begin(area,flags); };
    hooks.next=[finder] { return finder->Next(); };
    hooks.hasHitPoints=[&reward](Sid sid) { return reward.HasHitPoints(sid); };
    return hooks;
}
}
