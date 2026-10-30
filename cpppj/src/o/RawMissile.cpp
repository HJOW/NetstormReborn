// 원본최초썬캐논및SunArcher의필드산술/표시생성/공간이동/피해/삭제순서를보존한다.
#include "o/RawMissile.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 Baseprocess와 판본 공통 발사/표적/코너 필드의 위치다.
constexpr std::size_t kForm=4,kParent=8,kRicochet=12,kPassed=16,kFiring=20,kTarget=24,kCorner=28;
// 판본별 실제 Missile 필드 위치다.
struct Layout {
    // speed/allowance/origin/position/endpoint/box/end/start/previous: 원본 raw byte 위치다.
    std::size_t speed,allowance,origin,position,endpoint,box,end,start,previous;
};
// 패치에는 incoming 이전 좌표/빗나감 필드가 추가되지만 CD 레이아웃을 합성하지 않는다.
constexpr Layout kPatch{48,52,56,64,72,88,104,112,120},kCd{32,36,40,48,56,64,80,88,96};
// 비정렬 little endian 필드를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 현재 원본 필드 폭만 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// 주변 raw padding을 보존하며 원본 폭으로 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 낮은 바이트부터 지정한 필드만 쓴다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 원본 float32 좌표와 double 시각의 비트를 그대로 읽는다.
float Float(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(Read(bytes,offset)); }
double Double(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<double>(static_cast<std::uint64_t>(Read(bytes,offset))|(static_cast<std::uint64_t>(Read(bytes,offset+4))<<32)); }
// 원본 float/double 저장 폭을 명시한다.
void PutFloat(std::span<std::uint8_t> bytes,std::size_t offset,float value) { Put(bytes,offset,std::bit_cast<std::uint32_t>(value)); }
void PutDouble(std::span<std::uint8_t> bytes,std::size_t offset,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Put(bytes,offset,static_cast<std::uint32_t>(bits));Put(bytes,offset+4,static_cast<std::uint32_t>(bits>>32)); }
// 정상 BoardCoord의 유효 범위와 clamp을 원본 경계로 적용한다.
bool Valid(std::array<float,2> point) { return point[0]>0 && point[1]>0 && point[0]<256 && point[1]<256; }
std::array<float,2> Clamp(std::array<float,2> point) { return {std::clamp(point[0],1.0f,255.0f),std::clamp(point[1],1.0f,255.0f)}; }
// 원본 Distance의 정상 유한 좌표 Euclidean 길이다.
double Distance(std::array<float,2> first,std::array<float,2> second) { const double dx=static_cast<double>(first[0])-second[0],dy=static_cast<double>(first[1])-second[1];return std::sqrt(dx*dx+dy*dy); }
// 현재 raw 좌표 쌍을 조회/저장한다.
std::array<float,2> Point(std::span<const std::uint8_t> bytes,std::size_t offset) { return {Float(bytes,offset),Float(bytes,offset+4)}; }
void PutPoint(std::span<std::uint8_t> bytes,std::size_t offset,std::array<float,2> point) { PutFloat(bytes,offset,point[0]);PutFloat(bytes,offset+4,point[1]); }
// 실제 __ftol의64비트 절삭 뒤 하위 DWORD를 보존한다. 비유한/넘침은 원본 정수 불확정값의 하위0이다.
std::int32_t DamageInteger(float value) {
    const auto exact=static_cast<double>(value);if (!std::isfinite(exact) || exact>=9223372036854775808.0 || exact< -9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(exact)));
}
}
// 원본 공통 SID 필드를 직접 읽는다.
Sid MissileFields::Form() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kForm))}; }
Sid MissileFields::Parent() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kParent))}; }
Sid MissileFields::Firing() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kFiring))}; }
Sid MissileFields::Target() const { return Sid{static_cast<std::uint16_t>(Read(bytes,kTarget))}; }
// 같은 원본 판본 타입 표와 사운드/방향 보충 자료만 받는다.
RawMissile::RawMissile(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const MissileTypeData> missileTypes,MissileState& state,MissileHooks hooks)
    :pool_(pool),types_(types),missileTypes_(missileTypes),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || missileTypes.size()!=types.size()) throw std::invalid_argument("Missile 타입 판본/보충 자료 크기 오류");
    if (!std::isfinite(state.scaleX) || !std::isfinite(state.scaleY) || state.scaleX<=0 || state.scaleY<=0 || state.cornerImpactType>=types.size()) throw std::invalid_argument("Missile 픽셀 배율/충돌 타입 오류");
}
// 원본 SID/타입 범위를 검증하되 free/dead는 각 실제 body 분기가 판단한다.
std::span<const std::uint8_t> RawMissile::Slot(Sid sid,bool allowZero) const {
    if ((!sid.value && !allowZero) || sid.value>=pool_.Capacity()) throw std::out_of_range("Missile SID 범위 오류");
    const auto raw=pool_.Slot(sid);if (raw[10]>=types_.size()) throw std::out_of_range("Missile 타입 범위 오류");return raw;
}
// 부모 표시 자산의 현재 frame 폭으로만 쓴다.
void RawMissile::FrameNumber(Sid sid,std::uint32_t frame) const { const bool patch=pool_.Edition()==OriginalEdition::Patch1078;Put(pool_.AllocatedBytes(sid),patch ? 36U : 34U,frame,patch ? 4U : 1U); }
// 실제 ctor 순서에 따라 숫자 준비와 표시 자산 생성/Pop/비행효과를 수행한다.
Sid RawMissile::Construct(MissileFields& fields,const GunProjectileRequest& request) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const bool reflected=patch && request.ricochet!=0;const auto layout=patch ? kPatch : kCd;const auto firing=Slot(request.firing),target=Slot(request.target,reflected);
    const bool archer=firing[10]==state_.sunArcherType;
    // group0 탄도와 썬 디스크 발사기71 전용 포구 보정 생략을 독립 판정한다.
    const bool groupZero=archer || (patch && firing[10]==state_.crossbowType);
    const bool cannon=firing[10]==state_.sunCannonType || (patch && (firing[10]==state_.thunderCannonType || firing[10]==state_.rainCannonType)); // 같은 group1의 초기 아이스129도 원문 수치/표시 생성 순서를 사용한다.
    if ((reflected ? firing[10]!=state_.iceMissileType : (request.ricochet || (!groupZero && !cannon) || Valid(request.firingPosition)))) throw std::logic_error("Missile 미복원 반사/부모/포구 생성 경로 TYPE"+std::to_string(firing[10]));
    if (!reflected && types_[firing[10]].group!=(groupZero ? 0 : 1)) throw std::logic_error("Missile 실제부모 group 계약 오류");
    if (!std::isfinite(request.speed) || request.speed<=0 || !Valid(request.targetPosition)) throw std::invalid_argument("Missile 생성 좌표/속도 오류");
    if ((!Valid(request.firingPosition) && !hooks_.muzzle) || (!reflected && !archer && !hooks_.hotspot) || (!reflected && !groupZero && !hooks_.rayEnd) || (target[10]>=10 && !hooks_.targetDimensions) || !hooks_.create || !hooks_.owner || !hooks_.pop || (request.effectType && !hooks_.flightEffect)) throw std::logic_error("Missile 실제 생성/Pop/비행 효과 경계 누락 TYPE"+std::to_string(firing[10])+" effect"+std::to_string(request.effectType));
    if (request.displayType>=types_.size()) throw std::out_of_range("Missile 표시 타입 오류");
    Put(fields.bytes,0,patch ? 0x50d630U : 0x501ca0U);Put(fields.bytes,kForm,0);Put(fields.bytes,kParent,0);
    // 원본 초기 사각형과 직전 위치는 invalid 좌표 -10000이다.
    for (std::size_t offset=layout.box;offset<layout.box+16;offset+=4) PutFloat(fields.bytes,offset,-10000.0f);
    PutDouble(fields.bytes,layout.end,0);PutDouble(fields.bytes,layout.start,0);Put(fields.bytes,kFiring,request.firing.value);Put(fields.bytes,kTarget,request.target.value);PutFloat(fields.bytes,layout.speed,request.speed);
    Put(fields.bytes,kRicochet,request.ricochet);Put(fields.bytes,kPassed,0);PutPoint(fields.bytes,layout.previous,{-10000,-10000});
    const bool corner=!reflected && Valid(request.targetCenter);
    // 패치의 +32/+36 초기화는 group1 코너 분기에만 있으며 group0의 기존 바이트는 보존한다.
    if (patch) { if (corner && !groupZero) { Put(fields.bytes,32,0);Put(fields.bytes,36,0); }Put(fields.bytes,40,0);Put(fields.bytes,44,0);PutPoint(fields.bytes,80,request.targetCenter); }
    auto origin=Valid(request.firingPosition) ? request.firingPosition : hooks_.muzzle(request.firing);const auto& type=types_[firing[10]];
    // SunArcher71은원본처럼SHP/type hotspot보정을생략한다.기존128의두float저장은그대로다.
    if (!reflected && !archer) {
        const auto hot=hooks_.hotspot(request.firing);
        origin[0]=static_cast<float>(static_cast<double>(origin[0])+static_cast<double>(hot[0])/state_.scaleX);origin[1]=static_cast<float>(static_cast<double>(origin[1])+static_cast<double>(hot[1])/state_.scaleY);
        origin[0]=static_cast<float>(static_cast<double>(origin[0])-static_cast<double>(type.hotspotX)/state_.scaleX);origin[1]=static_cast<float>(static_cast<double>(origin[1])-static_cast<double>(type.hotspotY)/state_.scaleY);
    }
    const auto dimensions=target[10]<10 ? std::array<std::int32_t,2>{3,3} : hooks_.targetDimensions(request.target);const std::array<float,2> targetCenter{static_cast<float>(static_cast<double>(request.targetPosition[0])-dimensions[0]*0.5),static_cast<float>(static_cast<double>(request.targetPosition[1])-dimensions[1]*0.5)};PutPoint(fields.bytes,layout.endpoint,targetCenter); // 원문 target0은 기본3×3 치수로 보정한다.
    PutPoint(fields.bytes,layout.position,corner ? request.targetCenter : request.targetPosition);Put(fields.bytes,kCorner,corner ? 1U : 0U);
    // group0 코너의 종점은 패치에서 코너이며 CD10.72에서는 표적 치수로 보정한 중심이다.
    auto endpoint=Clamp(reflected ? targetCenter : (groupZero ? (patch && corner ? request.targetCenter : targetCenter) : hooks_.rayEnd(origin,corner ? request.targetCenter : targetCenter,request.firing)));origin=Clamp(origin);PutPoint(fields.bytes,layout.endpoint,endpoint);PutPoint(fields.bytes,layout.origin,origin);
    if (!Valid(origin) || !Valid(endpoint) || ((firing[11]&1) && (target[11]&1))) throw std::logic_error("Missile 실제 생성 좌표/자산 원본 assert");
    PutFloat(fields.bytes,layout.allowance,0);const float distance=static_cast<float>(Distance(origin,endpoint));if (distance<0 || distance>=256) throw std::out_of_range("Missile 생성 거리 원본 assert");
    PutDouble(fields.bytes,layout.start,state_.now);PutDouble(fields.bytes,layout.end,static_cast<double>(distance)/request.speed+state_.now);
    const auto asset=hooks_.create(request.displayType,2);Slot(asset);const auto& display=missileTypes_[request.displayType];
    if (display.frameCount>0) {
        if (request.displayType==state_.sunDiscType) FrameNumber(asset,1);
        else { if (!hooks_.directionFrame) throw std::logic_error("Missile 방향 프레임 경계 누락");FrameNumber(asset,static_cast<std::uint32_t>(hooks_.directionFrame(origin,endpoint,request.displayType))); }
    }
    hooks_.owner(asset,firing[patch ? 34U : 32U]);const auto sound=missileTypes_[firing[10]].firingSound ? missileTypes_[firing[10]].firingSound : display.firingSound;
    if (sound) { if (!hooks_.firingSound) throw std::logic_error("Missile 실제 발사 사운드 경계 누락");hooks_.firingSound(origin[0],origin[1],sound); }
    hooks_.pop(asset,origin[0],origin[1],0);if (request.effectType) hooks_.flightEffect(request.effectType,asset);return asset;
}
// 최초 썬 캐논 incomingCorner 경로의 실제 보간·swept 충돌·피해/종료를 처리한다.
void RawMissile::Frame(MissileFields& fields) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const bool reflected=patch && Read(fields.bytes,kRicochet)!=0;const auto layout=patch ? kPatch : kCd;const auto firing=Slot(fields.Firing()),target=Slot(fields.Target(),reflected),asset=Slot(fields.Parent());
    const bool archer=firing[10]==state_.sunArcherType;
    // 원본 group0 사거리/코너 종료는 크로스보우에도 같고71 전용 포구와 무관하다.
    const bool groupZero=archer || (patch && firing[10]==state_.crossbowType);
    const bool cannon=firing[10]==state_.sunCannonType || (patch && (firing[10]==state_.thunderCannonType || firing[10]==state_.rainCannonType)); // 초기 아이스도 같은 group1 이동/만료를 사용하고 충돌의 파생 효과는 아래에서 구별한다.
    if (!reflected && ((!groupZero && !cannon) || Read(fields.bytes,kRicochet))) throw std::logic_error("Missile 미복원 frame 파생/반사 경로"); // 반사 생성 뒤 발사 슬롯이 반납/재사용됐으면 원문처럼 아래에서 현재 TYPE/group을 읽는다.
    if (patch && Read(fields.bytes,40)) { Put(fields.bytes,40,0);Put(fields.bytes,44,1); } // 원문 빗나감 요청은 첫 프레임에 소비하고 종료 음향 표시를 유지한다.
    const float margin=static_cast<float>(patch && asset[10]==state_.thunderMissileType ? state_.thunderRangeMargin : (reflected ? 0 : state_.rangeMargin)); // 반사는 일반4 여유를 더하지 않는다.
    if (!hooks_.unpop || !hooks_.pop || !hooks_.destroy || (!reflected && !hooks_.center)) throw std::logic_error("Missile 실제 이동/삭제 경계 누락");
    const auto origin=Point(fields.bytes,layout.origin),endpoint=Point(fields.bytes,layout.endpoint);const auto end=Double(fields.bytes,layout.end),start=Double(fields.bytes,layout.start);
    const double remaining=patch ? end-state_.now : static_cast<float>(end-state_.now);const double duration=patch ? end-start : static_cast<float>(end-start);
    if (!patch && !(duration>0)) throw std::logic_error("CD Missile 0거리 프레임 별도 복원 필요"); // 패치는 원문 나눗셈의 NaN/invalid 좌표를 계산한 뒤 실제 삭제 분기로 간다.
    const float dx=static_cast<float>(static_cast<double>(endpoint[0])-origin[0]),dy=static_cast<float>(static_cast<double>(endpoint[1])-origin[1]);
    const double factor=patch ? 1.0/duration : static_cast<double>(dx)/duration;
    const std::array<float,2> current{static_cast<float>(static_cast<double>(endpoint[0])-(patch ? static_cast<double>(dx)*factor : factor)*remaining),static_cast<float>(static_cast<double>(endpoint[1])-(patch ? factor*static_cast<double>(dy) : static_cast<double>(dy)/duration)*remaining)};
    if (!Valid(current)) { if (patch && Read(fields.bytes,44)) { if (!hooks_.impactSound) throw std::logic_error("Missile 빗나감 종료 음향 미연결");hooks_.impactSound(current[0],current[1],missileTypes_[state_.cornerImpactType].impactSound); }hooks_.destroy(fields.Parent(),0);return; }
    hooks_.unpop(fields.Parent(),0);hooks_.pop(fields.Parent(),current[0],current[1],0);
    const bool corner=Read(fields.bytes,kCorner)!=0;
    if (fields.Target().value && !(target[11]&3)) {
        if (corner) { const auto point=Point(fields.bytes,layout.endpoint);PutFloat(fields.bytes,layout.box,point[0]-1.0f);PutFloat(fields.bytes,layout.box+4,point[1]-1.0f);PutFloat(fields.bytes,layout.box+8,point[0]);PutFloat(fields.bytes,layout.box+12,point[1]); }
        else { if (!hooks_.targetBounds) throw std::logic_error("Missile 실제 표적 발자국 경계 누락");const auto bounds=hooks_.targetBounds(fields.Target(),Point(fields.bytes,layout.position));const float allowance=Float(fields.bytes,layout.allowance);PutFloat(fields.bytes,layout.box,bounds[0]-allowance);PutFloat(fields.bytes,layout.box+4,bounds[1]-allowance);PutFloat(fields.bytes,layout.box+8,bounds[2]+allowance);PutFloat(fields.bytes,layout.box+12,bounds[3]+allowance); }
    }
    const auto previous=Point(fields.bytes,layout.previous);bool hit=false;
    if (Valid(previous) && !Read(fields.bytes,kPassed) && !(target[patch ? 40U : 35U]&0x10)) {
        const auto low=std::array<float,2>{std::min(previous[0],current[0]),std::min(previous[1],current[1])},high=std::array<float,2>{std::max(previous[0],current[0]),std::max(previous[1],current[1])};
        const auto boxLow=Point(fields.bytes,layout.box),boxHigh=Point(fields.bytes,layout.box+8);hit=low[0]<=boxHigh[0] && boxLow[0]<=high[0] && low[1]<=boxHigh[1] && boxLow[1]<=high[1];
    }
    const auto currentCenter=reflected ? origin : hooks_.center(fields.Firing());const float currentDistance=static_cast<float>(Distance(current,currentCenter));
    bool exhausted;
    if (reflected) exhausted=missileTypes_[asset[10]].range+margin<currentDistance; // 현재 발사 자산 위치 대신 저장 원점과 현재 표시 TYPE의 사거리를 쓴다.
    else if (corner) { const auto endCenter=hooks_.center(fields.Firing());exhausted=patch && !groupZero ? Distance(endpoint,endCenter)<currentDistance : static_cast<float>(Distance(endpoint,currentCenter))+margin<Distance(current,endCenter); }
    else {
        const float range=missileTypes_[firing[10]].range;
        exhausted=patch ? range+margin<currentDistance : range+margin<Distance(current,currentCenter);
    }
    auto last=current;
    if (hit && patch && target[10]==state_.floaterType) {
        if (!hooks_.randomFloat) throw std::logic_error("Missile 클라우드 플로터의 실제 공유 난수 미연결");
        const auto roll=hooks_.randomFloat(0.0F,1000.0F); //원문은 현재 cutoff를 난수 반환 뒤 FILD로 읽으며 float로 줄이지 않는다.
        if (roll<state_.avoidanceCutoff) { hit=false;Put(fields.bytes,kPassed,1); } //회피는 이후 충돌 검사를 막되 현재 위치/사거리 종료는 계속 처리한다.
    }
    if (hit) {
        last={static_cast<float>((static_cast<double>(current[0])+previous[0])*0.5),static_cast<float>((static_cast<double>(current[1])+previous[1])*0.5)};
        if (fields.Target().value && !(target[11]&1)) {
            if (corner) { if (!hooks_.impact) throw std::logic_error("Missile 실제 배리어 충돌 효과 경계 누락");hooks_.impact(last[0],last[1],2,0.02f);const auto sound=missileTypes_[state_.cornerImpactType].impactSound;if (!hooks_.impactSound) throw std::logic_error("Missile 실제 충돌 사운드 경계 누락");hooks_.impactSound(last[0],last[1],sound); }
            // CD10.72는 사거리 종료와 충돌이 겹쳐도 nonzero 표적 충돌 사운드를 요청한다.
            else {
                if (patch && asset[10]==state_.iceMissileType) {
                    if (!hooks_.impact) throw std::logic_error("아이스 투사체의 실제 충돌 표시 미연결");hooks_.impact(last[0],last[1],2,.02F); // 아이스는 일반 HitEffect 대신 코너와 같은 실제2번 표시를 생성한다.
                    if (missileTypes_[asset[10]].impactSound) {
                        if (!hooks_.randomInteger || !hooks_.impactName) throw std::logic_error("아이스 투사체의 실제 난수/파일명 음향 미연결");const auto number=hooks_.randomInteger(1000,5000);hooks_.impactName(last[0],last[1],"iceCannonImpact"+std::to_string(number/1000)+".wav"); // 사거리 종료와 겹쳐도 전용 음향을 요청한다.
                    }
                } else { if (!hooks_.hitEffect) throw std::logic_error("Missile 실제 표적 충돌 효과 경계 누락");hooks_.hitEffect(last[0],last[1]);if (missileTypes_[asset[10]].impactSound && (!patch || !exhausted)) { if (!hooks_.impactSound) throw std::logic_error("Missile 실제 표적 충돌 사운드 경계 누락");hooks_.impactSound(last[0],last[1],missileTypes_[asset[10]].impactSound); } }
            }
        }
        if (patch && asset[10]==state_.iceMissileType && !corner) { if (!hooks_.ricochet) throw std::logic_error("아이스 투사체의 실제 반사00482190 미연결");hooks_.ricochet(fields); } // 최초 표적의 free 비트와 별개로 효과 뒤의 현재 표시 TYPE를 읽는다.
        // 원본10.78은 표적의 free 비트나 코너와 별개로 충돌 확인 뒤 이 타입 조합의 사건을 보낸다.
        // 이전 좌표 기록·직접 피해·삭제보다 먼저 같은 클라이언트 입력 큐에 세 DWORD를 전달한다.
        if (patch && asset[10]==state_.thunderMissileType && target[10]==state_.stoneTowerType) {
            if (!hooks_.queueCommand) throw std::logic_error("Missile 실제 충돌 입력 큐 미연결");
            hooks_.queueCommand(state_.hitCommand,state_.thunderMissileType,state_.stoneTowerType);
        }
    }
    PutPoint(fields.bytes,layout.previous,last);if (!hit && !exhausted) return;
    const bool liveFiring=(firing[11]&1)==0;const auto firingGroup=types_[firing[10]].group;
    if (liveFiring && (firingGroup==0 || firingGroup==1)) { if (hit && !corner) { if (!hooks_.hit) throw std::logic_error("Missile 실제 직접 총 피해 경계 누락");hooks_.hit(fields.Firing(),fields.Target()); }else { if (!hooks_.refresh) throw std::logic_error("Missile 실제 총 사거리 갱신 경계 누락");hooks_.refresh(fields.Firing()); } }
    else if (patch && (liveFiring || (hit && !corner))) {
        const auto& damageType=missileTypes_[liveFiring ? firing[10] : state_.rainCannonType];float damage=static_cast<float>(static_cast<double>(damageType.damagePerSecond)*0.6F);
        if (missileTypes_[firing[10]].specialFlyerDamage && (types_[target[10]].flags2&0x120000U)) damage=missileTypes_[firing[10]].flyerDamagePerSecond;
        // 원문의 살아 있는 비총 발사자는 dead2 표적, 반납된 발사자는 살아 있는 표적에만 가상98을 호출한다.
        const bool eligible=fields.Target().value && (liveFiring ? ((target[11]&2)!=0 && !(target[11]&1)) : !(target[11]&3));
        if (eligible) { if (!hooks_.damage) throw std::logic_error("반사 Missile 실제 가상98 피해 미연결");hooks_.damage(fields.Target(),DamageInteger(damage),asset[34],Sid{}); }
    }
    if (patch && !hit && asset[10]==state_.iceMissileType) { if (!hooks_.impact) throw std::logic_error("아이스 투사체의 실제 사거리 종료 표시 미연결");hooks_.impact(last[0],last[1],2,.02F); } //00482b00은 비충돌 종료의 사거리 갱신 뒤에도2번 표시를 생성한다.
    if (patch && Read(fields.bytes,44)) { if (!hooks_.impactSound) throw std::logic_error("Missile 빗나감 종료 음향 미연결");hooks_.impactSound(last[0],last[1],missileTypes_[state_.cornerImpactType].impactSound); }
    hooks_.destroy(fields.Parent(),0);static_cast<void>(asset);
}
// 같은 월드/프로세스 타입 번호를 factory에 제공한다.
const SidPool& RawMissile::Pool() const { return pool_; }
std::uint32_t RawMissile::ProcessType() const { return state_.processType; }
// 실제 Kernel 객체가 원본 필드와 같은 수명을 가진다.
Missile::Missile(const RawMissile& logic,MissileFields fields):logic_(logic),fields_(std::move(fields)) {}
void Missile::RunFrame() { logic_.Frame(fields_); }
void Missile::OnAttached() { Put(fields_.bytes,kForm,Form().value);Put(fields_.bytes,kParent,Parent().value); }
const MissileFields& Missile::Fields() const { return fields_; }
// 실제 Create 몸체만 연결하고 월드 Pop/owner/effect를 임의 noop로 대체하지 않는다.
MissileHooks MakeMissileFactoryHooks(const SidPool& pool,SquidFactory& factory,MissileHooks hooks) {
    if (&pool!=&factory.Pool()) throw std::invalid_argument("Missile/Factory SID 풀 불일치");hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };return hooks;
}
// 실제 ctor의 자산 생성·Pop·flight effect 뒤 원본 flags0x10 부착으로 완료한다.
Missile* CreateMissile(SquidProcessHost& host,const RawMissile& logic,const GunProjectileRequest& request) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("Missile/Host SID 풀 불일치");MissileFields fields;const auto asset=logic.Construct(fields,request);
    return dynamic_cast<Missile*>(host.Attach(std::make_unique<Missile>(logic,std::move(fields)),logic.ProcessType(),asset,0x10));
}
// 실제 GunAnimation launch에서 같은 Host/Kernel의 Missile factory를 호출한다.
GunAnimationHooks MakeGunMissileHooks(SquidProcessHost& host,const RawMissile& logic,GunAnimationHooks hooks) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("GunAnimation/Missile SID 풀 불일치");hooks.createProjectile=[&host,&logic](const GunProjectileRequest& request) { if (!CreateMissile(host,logic,request)) throw std::logic_error("Missile 생성 자산 삭제 중 부착 실패"); };return hooks;
}
}
