// 원본 마나 표시의 필드 폭·현재 값 재조회·가상 호출 순서를 보존한다.
#include "client/RawCursorMana.h"
#include "client/RawSurfaceWorld.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kManaVtable=0x50c468,kBaseVtable=0x5019ec; // 비교용 원본 주소이며 호스트 함수 포인터가 아니다.
constexpr std::array<std::uint32_t,11> kThemeFrames{4,5,6,7,4,5,6,7,8,9,10}; // wrtsWRTS.?>를 대문자로 조회한 초기 프레임이다.
// 선택된 실제 효과만 요구하며 빈 경계로 동작을 생략하지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("실제 커서 마나 경계 미연결");return function; }
// 비정렬 원본 DWORD를 호스트 정렬과 무관하게 읽는다.
std::uint32_t U(const CursorManaFields& fields,std::size_t offset) { std::uint32_t result{};std::memcpy(&result,fields.bytes.data()+offset,4);return result; }
// 원본 signed 필드의 비트 패턴을 유지한다.
std::int32_t I(const CursorManaFields& fields,std::size_t offset) { return std::bit_cast<std::int32_t>(U(fields,offset)); }
// 원본 float 필드의 비트 패턴을 유지한다.
float F(const CursorManaFields& fields,std::size_t offset) { return std::bit_cast<float>(U(fields,offset)); }
// 원본 double 시각의 전체64비트를 읽는다.
double D(const CursorManaFields& fields,std::size_t offset) { double result{};std::memcpy(&result,fields.bytes.data()+offset,8);return result; }
// DWORD 필드의 현재 값만 바꾼다.
void Put(CursorManaFields& fields,std::size_t offset,std::uint32_t value) { std::memcpy(fields.bytes.data()+offset,&value,4); }
// 원본 fstp DWORD의 단정도 저장 경계를 보존한다.
void PutF(CursorManaFields& fields,std::size_t offset,float value) { Put(fields,offset,std::bit_cast<std::uint32_t>(value)); }
// 시각을 중간 float 변환 없이 저장한다.
void PutD(CursorManaFields& fields,std::size_t offset,double value) { std::memcpy(fields.bytes.data()+offset,&value,8); }
// raw 객체의 단정도 좌표를 비정렬 읽기한다.
float RawFloat(std::span<const std::uint8_t> raw,std::size_t offset) { float result{};std::memcpy(&result,raw.data()+offset,4);return result; }
// 실제 _ftol의64비트 결과 중 EAX만 사용하는 원본 시간 절삭이다.
std::int32_t TimeInteger(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// x87 fcos/fsin은 범위 밖 유한 입력을 그대로 남긴다. 게임 범위에서는 실제 삼각 함수를 계산한다.
double Trig(double angle,bool cosine) { if (std::isfinite(angle) && std::fabs(angle)>=9223372036854775808.0) return angle;return cosine ? std::cos(angle) : std::sin(angle); }
}
// 실제 타입/슬롯 레이아웃이 다른 판본을 연결하지 않는다.
RawCursorMana::RawCursorMana(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,o::SquidProcessState& clock,
    CursorManaGlobals& globals,CursorDropState& cursor,CursorManaHooks hooks):pool_(pool),types_(types),clock_(clock),globals_(globals),cursor_(cursor),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("커서 마나10.78 Pool/TYPE 계약 오류");
}
// 원본 SID 범위 밖의 호스트 메모리를 읽지 않는다.
std::span<const std::uint8_t> RawCursorMana::Raw(o::Sid sid) const {
    if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("커서 마나 SID 범위 오류");return pool_.Slot(sid);
}
// 목록은 원본 DWORD SID이며 호스트의16비트 SID로 조용히 절단하지 않는다.
o::Sid RawCursorMana::Marker(const CursorManaFields& fields,std::int32_t index) const {
    if (index<0 || index>=30) throw std::out_of_range("커서 마나30칸 목록 범위 오류");const auto value=U(fields,12+4*index);
    if (value>=pool_.Capacity()) throw std::out_of_range("커서 마나 목록 SID 범위 오류");return o::Sid{static_cast<std::uint16_t>(value)};
}
// 0041bfb0의 현재 부모와 종속 머리 진단을 유지한다.
o::Sid RawCursorMana::Parent(const CursorManaFields& fields) const {
    const auto value=U(fields,8);if (value>=pool_.Capacity()) throw std::out_of_range("커서 마나 부모 SID 범위 오류");const o::Sid sid{static_cast<std::uint16_t>(value)};
    const auto raw=Raw(sid);if (!(raw[6] || raw[7])) throw std::logic_error("parent->getHead() != 0 원본 마나 진단");return sid;
}
// NaN/끝점도 원본의 엄격한 부등식을 통과하지 못한다.
std::array<float,2> RawCursorMana::Clamp(std::array<float,2> point) const {
    const auto limit=static_cast<float>(globals_.mapLimit);if (!(point[0]>0 && point[0]<limit && point[1]>0 && point[1]<limit)) point={globals_.invalidPoint,globals_.invalidPoint};return point;
}
// 원본처럼 TYPE 포인터는 유지하고 theme·현재 개수·효과 뒤 시각은 매번 다시 읽는다.
void RawCursorMana::Construct(CursorManaFields& fields,o::Sid parent,o::Sid form,const CursorManaAttach& attach) const {
    Put(fields,0,kManaVtable);Put(fields,4,0);Put(fields,8,0);PutD(fields,0xc0,0);PutD(fields,0xc8,0);PutD(fields,0xd0,0);
    const auto raw=Raw(parent);const auto type=raw[10];if (type>=types_.size()) throw std::out_of_range("커서 마나 부모 TYPE 범위 오류");const auto& definition=types_[type];
    Put(fields,0x98,0);PutF(fields,0xbc,static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(definition.footX)-1U)));
    PutF(fields,0xb8,static_cast<float>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(definition.footY)-1U)));const auto parentX=RawFloat(raw,14),parentY=RawFloat(raw,18);
    // 객체 목록30칸을 초기화하며 원본의 나머지 padding을 건드리지 않는다.
    for (std::size_t index=0;index<30;++index) Put(fields,12+4*index,0);
    // 각 표시 객체는 실제 생성·WORD setter·extra·프레임 쓰기 뒤 목록에 들어간다.
    const auto create=[&](std::int32_t index) {
        const auto sid=Need(hooks_.create)(globals_.manaType,2);Raw(sid);Need(hooks_.setWord)(sid,1);auto bytes=pool_.AllocatedBytes(sid);bytes[40]|=8;
        const auto frame=Need(hooks_.themeFrame)(definition.theme);std::memcpy(bytes.data()+36,&frame,4);Put(fields,12+4*index,sid.value);return sid;
    };
    if (!U(fields,0x98)) {
        Put(fields,0x84,8);
        // 가상 위치 효과가 현재 개수를 바꾸는 경우에도 다음 조건은 현재 필드를 읽는다.
        for (std::int32_t index=0;index<I(fields,0x84);++index) { if (index>=30) throw std::out_of_range("커서 마나 생성 목록 범위 오류");const auto sid=create(index);Need(hooks_.position)(sid,globals_.invalidPoint,globals_.invalidPoint,0); }
    } else {
        Put(fields,0x88,5);Put(fields,0x84,20);
        // 원본 생성자는5변을 생성한다. 프레임/소멸의4변·개수20과 임의로 통일하지 않는다.
        for (std::int32_t side=0;side<5;++side) {
            // 원본의 현재 변별 개수를 생성 효과 뒤마다 다시 읽는다.
            for (std::int32_t index=0;index<I(fields,0x88);++index) {
                const auto offset=I(fields,0x88)*side+index;if (offset<0 || offset>=30) throw std::out_of_range("커서 마나 사각 생성 목록 범위 오류");const auto sid=create(offset);
                const auto delta=static_cast<double>(index)*static_cast<double>(1.600000023841858f);std::array<float,2> point{};
                if (!(side&1)) point={static_cast<float>(static_cast<double>(parentX)-delta),parentY-(side==0 ? F(fields,0xb8) : 0)};
                else point={parentX-(side==1 ? F(fields,0xbc) : 0),static_cast<float>(static_cast<double>(parentY)-delta)};
                point=Clamp(point);Need(hooks_.position)(sid,point[0],point[1],0x10);
            }
        }
        // 사각형 위상7칸은 이 생성 분기에서만 초기화된다.
        for (std::size_t index=0;index<7;++index) Put(fields,0x9c+4*index,0);
    }
    PutF(fields,0x8c,globals_.radiusLimit);PutF(fields,0x90,0);Need(attach)(fields,parent,form,0x10);
    Put(fields,0x94,0);PutD(fields,0xc0,clock_.Now());PutD(fields,0xc8,clock_.Now());PutD(fields,0xd0,0);Put(fields,0xd8,0);
}
// float 저장 위치를 제외한 연산은 원본 기본53비트 정밀도의 double로 수행한다.
void RawCursorMana::Frame(CursorManaFields& fields,const std::function<void()>& kill) const {
    const auto parent=Parent(fields);const auto raw=Raw(parent);const auto parentX=RawFloat(raw,14),parentY=RawFloat(raw,18);const auto delta=static_cast<float>(clock_.Now()-D(fields,0xc0));
    if (D(fields,0xd0)!=0 && D(fields,0xd0)<clock_.Now()) Put(fields,0x94,1);
    if (U(fields,0x98)) {
        auto index=0;const auto phase=[&](std::int32_t item) { if (item<0 || item>=7) throw std::out_of_range("커서 마나 사각 위상 범위 오류");return F(fields,0x9c+4*item); };
        // 앞 절반은 현재 위상을 증가시키고 뒤 절반은 원본의 대칭 순서로 복사한다.
        for (;index<=I(fields,0x88)/2;++index) { if (index>=7) throw std::out_of_range("커서 마나 사각 위상 범위 오류");PutF(fields,0x9c+4*index,static_cast<float>((static_cast<double>(index)*0.03+phase(index))+0.3)); }
        // 원본의 두 칸씩 증가하는 대칭 간격이다.
        for (std::int32_t distance=2;index<I(fields,0x88);++index,distance+=2) { const auto value=phase(index-distance);if (index>=7) throw std::out_of_range("커서 마나 사각 위상 범위 오류");PutF(fields,0x9c+4*index,value); }
        std::int32_t processed{};
        // 원본 frame은 생성자의5변 중4변을 이동시킨다.
        for (std::int32_t side=0;side<4;++side) {
            // SID0은 건너뛰며 삭제한 뒤 다음 인덱스 계산도 현재 변별 개수를 읽는다.
            for (std::int32_t item=0;item<I(fields,0x88);++item) {
                const auto sid=Marker(fields,I(fields,0x88)*side+item);if (!sid.value) continue;++processed;Need(hooks_.detach)(sid,0);auto current=Raw(sid);
                auto dx=static_cast<float>((static_cast<double>(parentX)-(side==1 ? F(fields,0xbc) : 0))-RawFloat(current,14));
                auto dy=static_cast<float>((static_cast<double>(parentY)-(side==0 ? F(fields,0xb8) : 0))-RawFloat(current,18));if (side>=2) { dx=-dx;dy=-dy; }
                const auto movement=static_cast<double>(delta)*phase(item);std::array<float,2> point{};
                if (side&1) point={static_cast<float>((static_cast<double>(parentX)-(side==1 ? F(fields,0xbc) : 0))+(static_cast<double>(dx)+movement)*(side==1 ? -1.0 : 1.0)),RawFloat(current,18)};
                else point={RawFloat(current,14),static_cast<float>((static_cast<double>(parentY)-(side==0 ? F(fields,0xb8) : 0))+(static_cast<double>(dy)+movement)*(side==0 ? -1.0 : 1.0))};
                point=Clamp(point);Need(hooks_.position)(sid,point[0],point[1],0x10);
                if (std::fabs(dx)>F(fields,0x8c) || std::fabs(dy)>F(fields,0x8c)) { Need(hooks_.destroy)(sid,0);const auto offset=I(fields,0x88)*side+item;if (offset<0 || offset>=30) throw std::out_of_range("커서 마나 사각 삭제 목록 범위 오류");Put(fields,12+4*offset,0); }
            }
        }
        if (!processed) Need(kill)();
    } else {
        const auto radius=F(fields,0x90);
        if (!U(fields,0x94)) { if (radius<F(fields,0x8c)) { const auto value=static_cast<double>(delta)*15.0+radius;PutF(fields,0x90,static_cast<float>(value>=F(fields,0x8c) ? F(fields,0x8c) : value)); } }
        else { if (radius<=0.10000000149011612f) { Need(kill)();return; }const auto value=static_cast<double>(radius)-static_cast<double>(delta)*30.0;PutF(fields,0x90,static_cast<float>(value<0 ? 0 : value)); }
        if (!U(fields,0xd8)) {
            // 가상 해제/중심/이동이 다음 현재 시각·개수·부모를 바꿀 수 있어 반복마다 다시 읽는다.
            for (std::int32_t index=0;index<I(fields,0x84);++index) {
                const auto sid=Marker(fields,index);if (Raw(sid)[11]&3) { Need(kill)();return; }Need(hooks_.detach)(sid,0);
                const auto integer=TimeInteger(clock_.Now());const auto angle=(static_cast<double>(6.28000020980835f)/I(fields,0x84))*index+((clock_.Now()-integer)*static_cast<double>(3.140000104904175f))*0.5;
                const auto x=static_cast<float>(Trig(angle,true)*F(fields,0x90)),y=static_cast<float>(Trig(angle,false)*F(fields,0x90));const auto center=Need(hooks_.center)(Parent(fields));
                const auto point=Clamp({center[0]+x,center[1]+y});Need(hooks_.position)(sid,point[0],point[1],0);
            }
        }
    }
    PutD(fields,0xc0,clock_.Now());
}
// event1의 숨김 값은 DWORD 전체를 유지하고 각 객체의 현재 void 여부를 읽는다.
void RawCursorMana::Notify(CursorManaFields& fields,std::uint32_t event,std::uint32_t info,std::uint32_t) const {
    if (event==3) { Put(fields,0x94,1);return; }if (event!=1) { PutD(fields,0xd0,clock_.Now()+3.0);return; }Put(fields,0xd8,info);
    // 효과가 개수/숨김/목록을 바꾸면 다음 조건에서 그 변경을 읽는다.
    for (std::int32_t index=0;index<I(fields,0x84);++index) { const auto sid=Marker(fields,index);const auto raw=Raw(sid);if (!U(fields,0xd8)) { if (raw[11]&4) Need(hooks_.unpop)(sid,0); }else if (!(raw[11]&4)) Need(hooks_.detach)(sid,0); }
}
// 원본은 커서 토큰을 먼저 지워 중첩 삭제에서 같은 프로세스를 재취득하지 않게 한다.
void RawCursorMana::Destroy(CursorManaFields& fields,std::uint32_t token) const {
    if (cursor_.cursorGump==token) cursor_.cursorGump=0;
    // 현재 목록과 현재 signed 개수를 유지하며0/free/dead는 원본처럼 건너뛴다.
    for (std::int32_t index=0;index<I(fields,0x84);++index) { const auto sid=Marker(fields,index);if (sid.value && !(Raw(sid)[11]&3)) Need(hooks_.destroy)(sid,0); }
    Put(fields,0,kBaseVtable);Put(fields,4,0xffffcfc7);Put(fields,8,0xffffcfc7);
}
// 실제 연결 대상의 자료를 확인하는 조회다.
const o::SidPool& RawCursorMana::Pool() const { return pool_; }
std::uint32_t RawCursorMana::ProcessType() const { return globals_.processType; }
// 실제 theme 문자 표에서 확인한 프레임만 공급한다.
std::uint32_t CursorManaThemeFrame(std::int32_t theme) { if (theme<0 || theme>=static_cast<std::int32_t>(kThemeFrames.size())) throw std::out_of_range("마나 theme 프레임 범위 오류");return kThemeFrames[static_cast<std::size_t>(theme)]; }
// 프로세스와 프레임이 같은 필드 저장소를 공유한다.
CursorManaProcess::CursorManaProcess(const RawCursorMana& logic):logic_(logic),fields_(std::make_shared<CursorManaFields>()) {}
CursorManaProcess::~CursorManaProcess() { logic_.Destroy(*fields_,Token()); }
// 실제 Kill이 자기 객체를 즉시 삭제해도 local 필드는 원본 꼬리 쓰기까지 살아 있다.
void CursorManaProcess::RunFrame() { const auto fields=fields_;const auto* logic=&logic_;logic->Frame(*fields,[this] { auto* host=Host();if (!host) throw std::logic_error("미부착 커서 마나 실행");host->Kill(*this,0); }); }
// Attach는 원본 생성자의 시각 초기화보다 먼저 발생한다.
void CursorManaProcess::OnAttached() { Put(*fields_,4,Form().value);Put(*fields_,8,Parent().value); }
void CursorManaProcess::Notify(std::uint32_t event,std::uint32_t info,std::uint32_t value) { logic_.Notify(*fields_,event,info,value); }
std::uint32_t CursorManaProcess::Token() const { return Pid(); }
CursorManaFields& CursorManaProcess::Fields() { return *fields_; }
// 실제 메모리 확보 실패와 정상 생성의 효과를 구별한다.
CursorManaProcess* CreateCursorManaProcess(o::SquidProcessHost& host,const RawCursorMana& logic,o::Sid parent,o::Sid form) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("커서 마나/Host SID 풀 불일치");std::unique_ptr<CursorManaProcess> process;
    try { process=std::make_unique<CursorManaProcess>(logic); }catch (const std::bad_alloc&) { return nullptr; }auto* result=process.get();
    logic.Construct(result->Fields(),parent,form,[&](CursorManaFields&,o::Sid actualParent,o::Sid actualForm,std::uint32_t flags) {
        if (!host.Attach(std::move(process),logic.ProcessType(),actualParent,flags,actualForm)) throw std::logic_error("커서 마나 부모 Attach 실패");
    });return result;
}
// 제거/이벤트는 현재 Kernel 객체를 조회하므로 이후 사정거리 프로세스에도 같은 분배기를 쓴다.
void BindCursorManaLifecycle(CursorLifecycleHooks& hooks,o::SquidProcessHost& host,o::Kernel& kernel,const RawCursorMana& logic) {
    if (&host.Pool()!=&logic.Pool()) throw std::invalid_argument("커서 마나/수명 SID 풀 불일치");
    hooks.createMana=[&](o::Sid parent,std::uint32_t form) { if (form>65535) throw std::out_of_range("커서 마나 Form WORD 범위 오류");auto* process=CreateCursorManaProcess(host,logic,parent,o::Sid{static_cast<std::uint16_t>(form)});return process ? process->Token() : 0U; };
    // 원본 포인터 토큰은 현재 Kernel 번호로만 바뀌며 실제 객체를 삭제한다.
    hooks.removeProcess=[&host,&kernel](std::uint32_t token,std::uint32_t flags) { auto* process=dynamic_cast<o::SquidProcess*>(kernel.Get(token));if (!host.IsRegistered(token,process)) throw std::logic_error("현재 커서 프로세스 Host 불일치");host.Kill(*process,flags); };
    // 원본 +1c의 세 인자는 DWORD 폭으로 전달한다.
    hooks.processEvent=[&host,&kernel](std::uint32_t token,std::uint32_t event,std::uint32_t info,std::uint32_t value) { auto* process=dynamic_cast<o::SquidProcess*>(kernel.Get(token));if (!host.IsRegistered(token,process)) throw std::logic_error("현재 커서 이벤트 Host 불일치");process->Notify(event,info,value); };
}
// 원본 가상 효과를 실제 표시/공간 수명에 넘기며 마나용 임시 좌표 사본을 만들지 않는다.
void BindCursorManaWorld(CursorManaHooks& hooks,RawSurfaceWorld& world,std::function<std::array<float,2>(o::Sid)> center,std::function<void(o::Sid)> notifySurface) {
    const auto core=world.Core();
    hooks.create=[&world](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
    hooks.setWord=[&pool=core.pool,&surfaces=core.bridgeState,notifySurface=std::move(notifySurface)](o::Sid sid,std::uint32_t word) {
        auto bytes=pool.AllocatedBytes(sid);const auto value=static_cast<std::uint16_t>(word);std::memcpy(bytes.data()+12,&value,2);
        if (bytes[10]==surfaces.bridgeType) Need(notifySurface)(sid);
    };
    hooks.themeFrame=CursorManaThemeFrame;hooks.center=std::move(center);
    hooks.detach=[&world](o::Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
    hooks.unpop=[&world](o::Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
    hooks.position=[&world](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
    hooks.destroy=[&world](o::Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
}
}
