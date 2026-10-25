// 사정거리 표시의 실제 필드 폭·현재 값·가상 호출 순서와 원본 근사 상수를 보존한다.
#include "client/RawCursorRange.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>
namespace netstorm::client {
namespace {
constexpr std::uint32_t kRangeVtable=0x50c498,kBaseVtable=0x5019ec; // 비교용 원본 주소이며 호스트 함수 포인터가 아니다.
// 선택된 실제 효과만 요구하며 빈 경계로 동작을 생략하지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("실제 커서 사정거리 경계 미연결");return function; }
// 비정렬 원본 DWORD를 호스트 정렬과 무관하게 읽는다.
std::uint32_t U(const CursorRangeFields& fields,std::size_t offset) { std::uint32_t result{};std::memcpy(&result,fields.bytes.data()+offset,4);return result; }
// 원본 signed 필드의 비트 패턴을 유지한다.
std::int32_t I(const CursorRangeFields& fields,std::size_t offset) { return std::bit_cast<std::int32_t>(U(fields,offset)); }
// 원본 float 필드의 비트 패턴을 유지한다.
float F(const CursorRangeFields& fields,std::size_t offset) { return std::bit_cast<float>(U(fields,offset)); }
// 원본 double 시각의 전체64비트를 읽는다.
double D(const CursorRangeFields& fields,std::size_t offset) { double result{};std::memcpy(&result,fields.bytes.data()+offset,8);return result; }
// DWORD 필드의 현재 값만 바꾼다.
void Put(CursorRangeFields& fields,std::size_t offset,std::uint32_t value) { std::memcpy(fields.bytes.data()+offset,&value,4); }
// 원본 fstp DWORD의 단정도 저장 경계를 보존한다.
void PutF(CursorRangeFields& fields,std::size_t offset,float value) { Put(fields,offset,std::bit_cast<std::uint32_t>(value)); }
// 시각을 중간 float 변환 없이 저장한다.
void PutD(CursorRangeFields& fields,std::size_t offset,double value) { std::memcpy(fields.bytes.data()+offset,&value,8); }
// raw 객체의 단정도 좌표를 비정렬 읽기한다.
float RawFloat(std::span<const std::uint8_t> raw,std::size_t offset) { float result{};std::memcpy(&result,raw.data()+offset,4);return result; }
// 실제 _ftol의64비트 결과 중 EAX만 사용하는 원본 시간 절삭이다.
std::int32_t TimeInteger(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// x87 fcos/fsin은 범위 밖 유한 입력을 그대로 남긴다. 게임 범위에서는 실제 삼각 함수를 계산한다.
double Trig(double angle,bool cosine) { if (std::isfinite(angle) && std::fabs(angle)>=9223372036854775808.0) return angle;return cosine ? std::cos(angle) : std::sin(angle); }
// 실제468e70은360을 그대로 허용하며 음수에서 한 바퀴를 더한 뒤 나머지를 구한다.
float Normalize(float value) {
    if (value<0) value=static_cast<float>(static_cast<double>(std::bit_cast<std::int32_t>(1U-std::bit_cast<std::uint32_t>(TimeInteger(static_cast<double>(value)/360.0))))*360.0+value);
    if (value>360) value=static_cast<float>(std::fmod(static_cast<double>(value),360.0));return value;
}
// 원본 Angle::contains의 원형 최소 구간은 양 끝점을 포함한다.
bool Contains(float value,float first,float last) { if (first>last) std::swap(first,last);return static_cast<double>(last)-first<=180 ? first<=value && value<=last : value<=first || value>=last; }
// 각도의 짧은 쪽 거리를 원본의 중간 float 저장 없이 계산한다.
double AngleDistance(float value,float endpoint) { const auto distance=std::fabs(static_cast<double>(value)-endpoint);return distance>180 ? 360.0-distance : distance; }
}
// 다른 판본의 raw 오프셋이나 TYPE 번호 표를 혼용하지 않는다.
RawCursorRange::RawCursorRange(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,o::SquidProcessState& clock,
    CursorRangeGlobals& globals,CursorDropState& cursor,CursorRangeHooks hooks):pool_(pool),types_(types),clock_(clock),globals_(globals),cursor_(cursor),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("사정거리10.78 Pool/TYPE 계약 오류");
}
// 도달한 raw 슬롯만 호스트 범위 안에서 읽는다.
std::span<const std::uint8_t> RawCursorRange::Raw(o::Sid sid) const { if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("사정거리 SID 범위 오류");return pool_.Slot(sid); }
// Baseprocess는 현재 부모의 종속 머리가 살아 있음을 진단한다.
o::Sid RawCursorRange::Parent(const CursorRangeFields& fields) const {
    const auto value=U(fields,8);if (value>=pool_.Capacity()) throw std::out_of_range("사정거리 부모 SID 범위 오류");const o::Sid sid{static_cast<std::uint16_t>(value)};const auto raw=Raw(sid);
    if (!(raw[6] || raw[7])) throw std::logic_error("parent->getHead() != 0 원본 사정거리 진단");return sid;
}
// 원본 목록의DWORD SID를16비트로 조용히 절단하지 않는다.
o::Sid RawCursorRange::Marker(const CursorRangeFields& fields,std::int32_t index) const {
    if (index<0 || index>=100) throw std::out_of_range("사정거리100칸 목록 범위 오류");const auto value=U(fields,20+4*index);if (value>=pool_.Capacity()) throw std::out_of_range("사정거리 목록 SID 범위 오류");return o::Sid{static_cast<std::uint16_t>(value)};
}
// NaN과 맵 끝점도 원본의 엄격한 부등식을 통과하지 못한다.
std::array<float,2> RawCursorRange::Clamp(std::array<float,2> point) const { const auto limit=static_cast<float>(globals_.coordinates.mapLimit);if (!(point[0]>0 && point[0]<limit && point[1]>0 && point[1]<limit)) point={globals_.coordinates.invalidPoint,globals_.coordinates.invalidPoint};return point; }
// 방향0/2/4/6의 현재8DWORD 문맥과 생성 순서를 유지한다.
void RawCursorRange::CreateRays(CursorRangeFields& fields,o::Sid parent) const {
    static_cast<void>(Need(hooks_.footprint)(parent));
    // 펜스(group6)는 같은 문맥을 읽지만 Cannon 방향 제한을 적용하지 않는다.
    for (std::int32_t direction=0;direction<8;direction+=2) {
        auto raw=Raw(parent);o::GunTargetQuery query;query.x=RawFloat(raw,14);query.y=RawFloat(raw,18);query.type=raw[10];query.direction=static_cast<std::int32_t>(Need(hooks_.mode)(parent));
        raw=Raw(parent);query.owner=raw[34];std::memcpy(&query.frame,raw.data()+36,4);const auto center=Need(hooks_.world.center)(parent);query.centerX=center[0];query.centerY=center[1];
        Put(fields,0x1f0+2*direction,U(fields,0x10) || Need(hooks_.cannonDirection)(query,direction) ? 1U : 0U);
    }
    Put(fields,0x1a4,0);Put(fields,0x1ac,0);
    // 목록은 방향별 현재 변 길이로 색인하고 원본처럼1a4는0인 채 생성 수만1ac에 더한다.
    for (std::int32_t side=0;side<4;++side) {
        // 생성/위치 콜백 뒤 다음 조건도 현재 개수를 읽는다.
        for (std::int32_t item=0;item<I(fields,0x1a8);++item) if (U(fields,0x1f0+4*side)) {
            const auto index=I(fields,0x1a8)*side+item;if (index<0 || index>=100) throw std::out_of_range("사정거리 직선 생성 목록 범위 오류");const auto sid=Need(hooks_.world.create)(globals_.rangeType,2);Raw(sid);pool_.AllocatedBytes(sid)[40]|=8;
            Put(fields,20+4*index,sid.value);Put(fields,0x1ac,U(fields,0x1ac)+1);Need(hooks_.world.position)(sid,globals_.coordinates.invalidPoint,globals_.coordinates.invalidPoint,0);
        }
    }
    PutF(fields,0x1c0,15);
}
// 원형 표시8개는 현재A/B 첫 프레임을 적용한 뒤 실제 가상 Pop에 넘긴다.
void RawCursorRange::CreateCircle(CursorRangeFields& fields) const {
    Put(fields,0x1a4,8);Put(fields,0x1ac,8);
    // 각 생성/위치 효과 뒤 현재 개수·변형을 다시 읽는다.
    for (std::int32_t index=0;index<I(fields,0x1a4);++index) {
        if (index>=100) throw std::out_of_range("사정거리 원형 생성 목록 범위 오류");const auto sid=Need(hooks_.world.create)(globals_.rangeType,2);auto raw=pool_.AllocatedBytes(sid);raw[40]|=8;
        const auto first=Need(hooks_.frameRun)(raw[10],U(fields,0x1ec)).first;std::memcpy(raw.data()+36,&first,4);Put(fields,20+4*index,sid.value);Need(hooks_.world.position)(sid,globals_.coordinates.invalidPoint,globals_.coordinates.invalidPoint,0);
    }
}
// 현재 TYPE 포인터와 초기 부모 주소를 유지하되 콜백 뒤 필요한 필드는 다시 읽는다.
void RawCursorRange::Construct(CursorRangeFields& fields,o::Sid parent,o::Sid form,const CursorRangeAttach& attach) const {
    Put(fields,0,kRangeVtable);Put(fields,4,0);Put(fields,8,0);PutF(fields,0x1c4,-1);PutF(fields,0x1c8,-1);PutD(fields,0x1d0,0);PutD(fields,0x1d8,0);PutD(fields,0x1e0,0);
    auto raw=Raw(parent);const auto type=raw[10];if (type>=types_.size()) throw std::out_of_range("사정거리 부모 TYPE 범위 오류");const auto& definition=types_[type];Put(fields,12,1);Put(fields,16,definition.group==6 ? 1U : 0U);
    if (definition.group==1 || U(fields,16)) Put(fields,12,0);
    if (raw[10]==globals_.targeting.windArcherType) {
        const auto mode=Need(hooks_.mode)(parent);const auto angle=Normalize(static_cast<float>(static_cast<double>(mode)*45.0));
        PutF(fields,0x1c4,Normalize(Normalize(static_cast<float>(static_cast<double>(angle)-globals_.targeting.windHalfAngle))));PutF(fields,0x1c8,Normalize(Normalize(globals_.targeting.windHalfAngle+angle)));
        PutF(fields,0x1c4,Normalize(F(fields,0x1c4)));PutF(fields,0x1c8,Normalize(F(fields,0x1c8)));PutF(fields,0x1cc,static_cast<float>((360.0-AngleDistance(F(fields,0x1c4),F(fields,0x1c8)))*0.5));
    }
    raw=Raw(parent);const auto currentType=raw[10];if (currentType>=types_.size()) throw std::out_of_range("사정거리 현재 TYPE 범위 오류");std::uint32_t rangeType=currentType;
    if (types_[currentType].flags2&0x30000) { const auto content=Raw(Need(hooks_.firstContent)(parent));rangeType=content[18]|(static_cast<std::uint32_t>(content[19])<<8); }
    PutF(fields,0x1b0,Need(hooks_.range)(rangeType));PutF(fields,0x1bc,5);Put(fields,0x1a8,std::bit_cast<std::uint32_t>(TimeInteger(static_cast<double>(F(fields,0x1b0))*static_cast<double>(0.20000000298023224f))));
    // 목록100칸만0으로 초기화하고 원본의 다른 padding을 유지한다.
    for (std::size_t index=0;index<100;++index) Put(fields,20+4*index,0);
    Put(fields,0x1ec,(definition.flags2&0x100) ? 1U : 0U);
    if (!U(fields,12)) CreateRays(fields,parent);else if (U(fields,12)==1) CreateCircle(fields);
    PutF(fields,0x1b4,0);Need(attach)(fields,parent,form,0x10);Put(fields,0x1b8,0);PutD(fields,0x1d0,clock_.Now());PutD(fields,0x1d8,clock_.Now());PutD(fields,0x1e0,0);Put(fields,0x1e8,0);
}
// 종료 시 preview extra8은 다른 현재 토큰도 지우므로 소멸자의 동일성 검사로 대체하지 않는다.
void RawCursorRange::Stop(CursorRangeFields& fields,const std::function<void()>& kill) const { if (Raw(Parent(fields))[40]&8) cursor_.cursorGump=0;Need(kill)(); }
// 실제 직선 간격·이동 위상·프레임을 현재 자료로 계산한다.
bool RawCursorRange::Rays(CursorRangeFields& fields,const std::function<void()>& kill) const {
    const auto step=static_cast<float>(static_cast<double>(F(fields,0x1b0))/I(fields,0x1a8));const auto grown=(clock_.Now()-D(fields,0x1d8))*F(fields,0x1c0);const auto visible=static_cast<float>(grown);const auto phase=static_cast<float>(std::fmod(grown,static_cast<double>(step)));
    if (U(fields,0x1b8)) { Stop(fields,kill);return false; }
    const auto parent=Parent(fields);const auto type=Raw(parent)[10];if (type>=types_.size()) throw std::out_of_range("사정거리 직선 TYPE 범위 오류");const auto& definition=types_[type];const auto parentRaw=Raw(Parent(fields));const auto x=RawFloat(parentRaw,14),y=RawFloat(parentRaw,18);static_cast<void>(Need(hooks_.footprint)(Parent(fields)));
    constexpr std::array<std::array<int,2>,4> directions{{{0,-1},{1,0},{0,1},{-1,0}}}; // 실제0052f83c/0052f85c의 짝수 방향이다.
    // 네 방향의 현재 부모 가상 프레임과 발사구를 각각 읽는다.
    for (std::int32_t side=0;side<4;++side) {
        const auto frame=Need(hooks_.directionFrame)(Parent(fields),side*2);const auto offset=Need(hooks_.firingOffset)(type,frame);
        const auto originX=static_cast<float>(static_cast<double>(x)+static_cast<double>(offset[0])/globals_.targeting.scaleX);const auto originY=static_cast<float>(static_cast<double>(y)+static_cast<double>(offset[1])/globals_.targeting.scaleY);
        const auto beginX=static_cast<float>(static_cast<double>(originX)-static_cast<double>(definition.hotspotX)/globals_.targeting.scaleX);const auto beginY=static_cast<float>(static_cast<double>(originY)-static_cast<double>(definition.hotspotY)/globals_.targeting.scaleY);
        // 숨김/미허용이어도 현재 Run을 원본처럼 읽는다.
        for (std::int32_t item=0;item<I(fields,0x1a8);++item) {
            const auto variant=U(fields,0x1ec);const auto count=Need(hooks_.frameRun)(globals_.rangeType,variant).count;const auto first=Need(hooks_.frameRun)(globals_.rangeType,variant).first;
            const auto product=static_cast<double>(item)*step;
            if (U(fields,0x1f0+4*side) && !U(fields,0x1e8) && product<visible) {
                const auto sid=Marker(fields,I(fields,0x1a8)*side+item);if (!(Raw(sid)[11]&4)) Need(hooks_.world.detach)(sid,0);
                const auto distance=static_cast<float>(static_cast<double>(static_cast<float>(product))+phase);auto point=Clamp({static_cast<float>(static_cast<double>(directions[side][0])*distance+beginX),static_cast<float>(static_cast<double>(directions[side][1])*distance+beginY)});
                if (count<=0) throw std::logic_error("사정거리 실제 Run 개수 오류");const auto animation=TimeInteger(static_cast<double>(distance)*2.0)%count+first;auto bytes=pool_.AllocatedBytes(sid);std::memcpy(bytes.data()+36,&animation,4);Need(hooks_.world.position)(sid,point[0],point[1],0x10);
            }
        }
    }
    return true;
}
// 실제 원형 반경과 Wind 관측각의 근사 상수를 유지하고 각 효과 뒤 현재 상태를 읽는다.
bool RawCursorRange::Circle(CursorRangeFields& fields,float delta,const std::function<void()>& kill) const {
    const auto radius=F(fields,0x1b4);
    if (!U(fields,0x1b8)) { if (radius<F(fields,0x1b0)) { const auto value=static_cast<double>(delta)*15.0+radius;PutF(fields,0x1b4,static_cast<float>(value>=F(fields,0x1b0) ? F(fields,0x1b0) : value)); } }
    else { if (radius<=0.10000000149011612f) { Stop(fields,kill);return false; }const auto value=static_cast<double>(radius)-static_cast<double>(delta)*30.0;PutF(fields,0x1b4,static_cast<float>(value<=0 ? 0 : value)); }
    if (!U(fields,0x1e8)) {
        // 반복마다 현재 개수와 raw dead/void를 읽는다.
        for (std::int32_t index=0;index<I(fields,0x1a4);++index) {
            const auto sid=Marker(fields,index);const auto raw=Raw(sid);if (raw[11]&2) { Need(hooks_.diagnostic)(29,"Dead circle range!");continue; }if (!(raw[11]&4)) Need(hooks_.world.detach)(sid,0);
            const auto phase=F(fields,0x1b4)==F(fields,0x1b0) ? (clock_.Now()-TimeInteger(clock_.Now()))*3.1415927410125732 : 0.0;
            auto angle=static_cast<float>((static_cast<double>(6.283180236816406f)/I(fields,0x1a4))*index+phase);const auto degrees=Normalize(static_cast<float>((static_cast<double>(1.5707900524139404f)-angle)*57.29583));
            double factor=1;double theta=angle;
            if (F(fields,0x1c4)>=0 && F(fields,0x1c4)<360 && !Contains(degrees,F(fields,0x1c4),F(fields,0x1c8))) {
                angle=AngleDistance(degrees,F(fields,0x1c8))<=AngleDistance(degrees,F(fields,0x1c4)) ? F(fields,0x1c8) : F(fields,0x1c4);
                factor=1.0-AngleDistance(degrees,angle)/F(fields,0x1cc);theta=static_cast<double>(1.5707900524139404f)-static_cast<double>(angle)*static_cast<double>(0.01744999922811985f);
            }
            const auto x=static_cast<float>((Trig(theta,true)*factor)*F(fields,0x1b4)),y=static_cast<float>(-(Trig(theta,false)*factor)*F(fields,0x1b4));const auto center=Need(hooks_.world.center)(Parent(fields));const auto point=Clamp({center[0]+x,center[1]+y});Need(hooks_.world.position)(sid,point[0],point[1],0);
        }
    }
    return true;
}
// 내부 frame은 현재 값을 다시 읽으므로 바깥 몸체는 시간 차와 종료 시각을 계산한다.
void RawCursorRange::Frame(CursorRangeFields& fields,const std::function<void()>& kill) const {
    static_cast<void>(Raw(Parent(fields)));const auto delta=static_cast<float>(clock_.Now()-D(fields,0x1d0));if (D(fields,0x1e0)!=0 && D(fields,0x1e0)<clock_.Now()) Put(fields,0x1b8,1);
    const auto mode=U(fields,12);if ((!mode && !Rays(fields,kill)) || (mode==1 && !Circle(fields,delta,kill))) return;PutD(fields,0x1d0,clock_.Now());
}
// event1은100칸 고정으로 SID0을 건너뛴다. 마나의 현재 개수 반복과 구별한다.
void RawCursorRange::Notify(CursorRangeFields& fields,std::uint32_t event,std::uint32_t info,std::uint32_t) const {
    if (event==3) { Put(fields,0x1b8,1);return; }if (event!=1) { PutD(fields,0x1e0,clock_.Now()+3.0);return; }Put(fields,0x1e8,info);
    // 실제 가상 효과 뒤 다음 칸과 현재 숨김 값을 읽는다.
    for (std::int32_t index=0;index<100;++index) { const auto sid=Marker(fields,index);if (!sid.value) continue;const auto raw=Raw(sid);if (!U(fields,0x1e8)) { if (raw[11]&4) Need(hooks_.world.unpop)(sid,0); }else if (!(raw[11]&4)) Need(hooks_.world.detach)(sid,0); }
}
// 원본 소멸자는 free만 제외한다. dead도 제외하는 마나와 구별한다.
void RawCursorRange::Destroy(CursorRangeFields& fields,std::uint32_t token) const {
    if (cursor_.cursorGump==token) cursor_.cursorGump=0;
    // 고정100칸의 현재 SID/free를 읽고 선택한 가상 삭제를 실행한다.
    for (std::int32_t index=0;index<100;++index) { const auto sid=Marker(fields,index);if (sid.value && !(Raw(sid)[11]&1)) Need(hooks_.world.destroy)(sid,0); }
    Put(fields,0,kBaseVtable);Put(fields,4,0xffffcfc7);Put(fields,8,0xffffcfc7);
}
// 실제 단일 풀과 현재 타입을 연결 시점에 검증한다.
const o::SidPool& RawCursorRange::Pool() const { return pool_; }
std::uint32_t RawCursorRange::ProcessType() const { return globals_.processType; }
// 프레임/소멸이 동일한 필드를 공유한다.
CursorRangeProcess::CursorRangeProcess(const RawCursorRange& logic):logic_(logic),fields_(std::make_shared<CursorRangeFields>()) {}
CursorRangeProcess::~CursorRangeProcess() { logic_.Destroy(*fields_,Token()); }
// 몸체에서 즉시 자기 삭제해도 현재 호출의 필드는 살아 있다.
void CursorRangeProcess::RunFrame() { const auto fields=fields_;const auto* logic=&logic_;logic->Frame(*fields,[this] { auto* host=Host();if (!host) throw std::logic_error("미부착 사정거리 실행");host->Kill(*this,0); }); }
// 실제 부착 결과를 원본 DWORD 폭으로 반영한다.
void CursorRangeProcess::OnAttached() { Put(*fields_,4,Form().value);Put(*fields_,8,Parent().value); }
void CursorRangeProcess::Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) { logic_.Notify(*fields_,event,info,value); }
std::uint32_t CursorRangeProcess::Token() const { return Pid(); }
CursorRangeFields& CursorRangeProcess::Fields() { return *fields_; }
// 실제 new 실패와 생성 후 부착을 구별한다.
CursorRangeProcess* CreateCursorRangeProcess(o::SquidProcessHost& host,const RawCursorRange& logic,o::Sid parent,o::Sid form) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("사정거리/Host SID 풀 불일치");std::unique_ptr<CursorRangeProcess> process;
    try { process=std::make_unique<CursorRangeProcess>(logic); }catch (const std::bad_alloc&) { return nullptr; }auto* result=process.get();
    logic.Construct(result->Fields(),parent,form,[&](CursorRangeFields&,o::Sid actual,o::Sid actualForm,std::uint32_t flags) { if (!host.Attach(std::move(process),logic.ProcessType(),actual,flags,actualForm)) throw std::logic_error("사정거리 부모 Attach 실패"); });return result;
}
// 같은 수명의 remove/event는 마나 쪽 공유 분배를 사용하고 createRange만 추가한다.
void BindCursorRangeLifecycle(CursorLifecycleHooks& hooks,o::SquidProcessHost& host,const RawCursorRange& logic) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("사정거리/수명 SID 풀 불일치");hooks.createRange=[&](o::Sid parent,std::uint32_t form) { if (form>65535) throw std::out_of_range("사정거리 Form WORD 범위 오류");auto* process=CreateCursorRangeProcess(host,logic,parent,o::Sid{static_cast<std::uint16_t>(form)});return process ? process->Token() : 0U; };
}
}
