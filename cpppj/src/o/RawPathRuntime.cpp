// x87 53비트 계산의 저장 지점을 보존한다. 패치/CD의 다른 중간 float 저장도 구별한다.
#include "o/RawPathRuntime.h"
#include "o/RawSquidNeighbors.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
// 원본 cleanup의 현재 보조 객체 DWORD 조회다.
std::uint32_t RawPathRuntime::Helper() const { return static_cast<std::uint32_t>(Read(layout_.helper)); }
// 보조 객체를 예약했을 때 원본 필드 하나만 지운다.
void RawPathRuntime::ClearHelper() { Write(layout_.helper,0); }
// 실제 Moveaway getter00427d30/CD004e7660이 기존 Path 객체에서 읽는 signedDWORD다.
std::int32_t RawPathRuntime::MoveAwayDepth() const { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Read(layout_.helper+4))); }
// 실제 BridgeCommand00449900/CD00481ab0이 마지막 Prepare 뒤 깊이+1을 저장한다.
void RawPathRuntime::SetMoveAwayDepth(std::int32_t depth) { Write(layout_.helper+4,std::bit_cast<std::uint32_t>(depth)); }
namespace {
// 두 PE의 방향 배율/한 바퀴/늦은 시작 허용 간격은 각각 45도/360도/2초다.
constexpr float kDirectionDegrees=45.0F,kFullTurn=360.0F;
constexpr double kLateStart=2.0;
// raw float를 little endian 정수 패턴으로 바꾼다.
std::uint32_t Bits(float value) { return std::bit_cast<std::uint32_t>(value); }
}
// 실제 코드가 읽는 배열 길이와 저장 폭이며, CD의 끝에 없는 두 DWORD를 패치와 공유하지 않는다.
RawPathLayout RawPathRuntime::Layout(OriginalEdition edition) {
    if (edition==OriginalEdition::Patch1078)
        return {5000,0x186f0,0x9c58,0x13898,0x186b8,0x186c0,0x186c4,0x186c8,0x186cc,0x186d0,0x186d4,0x186ec,0x50df64,0x50e284,0x50e458};
    return {600,0x2f28,0x12d8,0x2598,0x2ef8,0x2f00,0x2f04,0x2f08,0x2f0c,0x2f10,0x2f14,0x2f24,0x503d28,0x503d60,0x503eb0};
}
// 미초기화 heap 입력을 보존하면서 실제 원본 생성자에서 쓰는 부분만 지운다.
RawPathRuntime::RawPathRuntime(OriginalEdition edition,bool priest,std::uint8_t heapFill,bool flying,bool worm,bool balloon,bool dude,bool bulf,bool walker)
    :edition_(edition),layout_(Layout(edition)),bytes_(layout_.size,heapFill) {
    Write(0,priest ? layout_.priestVtable : layout_.teleportVtable);Write(4,0);Write(8,0);Write(0x10,0);
    if (flying) { if (edition!=OriginalEdition::Patch1078 || priest) throw std::invalid_argument("실제 fly39 생성자 판본/종류 오류");Write(0,0x50e2ec); }
    if (worm) { if (edition!=OriginalEdition::Patch1078 || priest || flying) throw std::invalid_argument("실제 worm40 생성자 판본/종류 오류");Write(0,0x50e4c4); }
    if (balloon) { if (edition!=OriginalEdition::Patch1078 || priest || flying || worm) throw std::invalid_argument("실제 벌룬42 생성자 판본/종류 오류");Write(0,0x50e2b8); }
    if (dude) { if (edition!=OriginalEdition::Patch1078 || priest || flying || worm || balloon) throw std::invalid_argument("실제 Dude24 생성자 판본/종류 오류");Write(0,0x50e3d8); }
    if (bulf) { if (edition!=OriginalEdition::Patch1078 || priest || flying || worm || balloon || dude) throw std::invalid_argument("실제 Bulf41 생성자 판본/종류 오류");Write(0,0x50e354); }
    if (walker) { if (edition!=OriginalEdition::Patch1078 || priest || flying || worm || balloon || dude || bulf) throw std::invalid_argument("실제 Walker38 생성자 판본/종류 오류");Write(0,0x50e320); }
    // 원본 두 생성자 모두 시각 배열 전체만 0으로 만들고 꼭짓점/방향 배열은 보존한다.
    for (std::size_t i=0;i<layout_.capacity;++i) Write(layout_.times+i*8,0,8);
    Write(layout_.nextFrame,0,8);
    // current/angle/finished/두 대상/helper/helper+4는 두 판본에서 같다.
    for (auto offset:{layout_.current,layout_.angle,layout_.finished,layout_.targetA,layout_.targetB,layout_.helper,layout_.helper+4,layout_.deadFlag}) Write(offset,0);
    const auto tail=edition==OriginalEdition::Patch1078 ? 0x186e4U : 0x2f1cU;
    Write(tail,0);Write(tail+4,0);
}
// 실제 비행 Init은 생성자의 기존 count를 초기화하지 않고 두 꼭짓점을 추가한다.
void RawPathRuntime::InitializeFlight(RawPathPoint parent,RawPathPoint destination,std::int32_t cruiseHeight,std::uint8_t currentHeight) {
    if (edition_!=OriginalEdition::Patch1078) throw std::invalid_argument("CD 비행 Init 미복원");
    const auto append=[this](RawPathPoint point,std::int32_t direction) {
        const auto index=Count();if (index<0 || static_cast<std::size_t>(index)+1>=layout_.capacity) throw std::out_of_range("비행 경로 append 범위 오류");
        Write(0x14+static_cast<std::size_t>(index)*8,Bits(point.x));Write(0x18+static_cast<std::size_t>(index)*8,Bits(point.y));Write(layout_.times+static_cast<std::size_t>(index)*8,0,8);Write(layout_.directions+static_cast<std::size_t>(index)*4,std::bit_cast<std::uint32_t>(direction));Write(0x10,static_cast<std::uint32_t>(index)+1);
    }; // 4882e0의 좌표/방향/시각 쓰기와 마지막 count 증가 순서다.
    constexpr float kHeightToCell=0.09090909361839294F; //ROM504194의 실제 고도→칸 배율이다.
    append(parent,0);const auto rise=static_cast<float>(static_cast<double>(cruiseHeight)*kHeightToCell); //첫 fild/fmul/fstp DWORD다.
    if (static_cast<std::int32_t>(currentHeight)<std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(cruiseHeight)-1U)) append({parent.x,static_cast<float>(static_cast<double>(parent.y)-(static_cast<double>(rise)-static_cast<double>(currentHeight)*kHeightToCell))},0);
    if (cruiseHeight) append({destination.x,static_cast<float>(static_cast<double>(destination.y)-rise)},0);
    append(destination,RawSquidNeighbors::Direction(parent.x,parent.y,destination.x,destination.y));ResetCurrent();
}
// 경로 개수 밖이어도 실제 배열/인접8BYTE 안이면 원본 읽기를 보존한다.
double RawPathRuntime::PhysicalTime(std::int32_t index) const {
    if (index< -1 || index>=static_cast<std::int32_t>(layout_.capacity)) throw std::out_of_range("벌룬 고도 시각의 물리 배열 범위");
    return std::bit_cast<double>(Read(static_cast<std::size_t>(static_cast<std::int64_t>(layout_.times)+static_cast<std::int64_t>(index)*8),8));
}
// 같은 실제 QWORD 예약 시각을 bit 그대로 읽는다.
double RawPathRuntime::NextFrame() const { return std::bit_cast<double>(Read(layout_.nextFrame,8)); }
// 현재 비행 frame의 다음 실행 시각만 쓴다.
void RawPathRuntime::SetNextFrame(double time) { Write(layout_.nextFrame,std::bit_cast<std::uint64_t>(time),8); }
// byte 배열 자체는 호출자에게 읽기 전용으로 제공한다.
std::span<const std::uint8_t> RawPathRuntime::Bytes() const { return bytes_; }
// 배열 밖의 메모리를 읽기 전에 진단한다.
std::uint64_t RawPathRuntime::Read(std::size_t offset,std::size_t width) const {
    if (offset+width>bytes_.size()) throw std::out_of_range("경로 raw 읽기 범위");
    std::uint64_t value=0;
    // 원본 DWORD/QWORD의 낮은 바이트부터 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint64_t>(bytes_[offset+i])<<(i*8);
    return value;
}
// 배열 밖의 메모리를 쓰기 전에 진단한다.
void RawPathRuntime::Write(std::size_t offset,std::uint64_t value,std::size_t width) {
    if (offset+width>bytes_.size()) throw std::out_of_range("경로 raw 쓰기 범위");
    // 원본 필드 폭만 쓰며 옆의 미초기화 값은 보존한다.
    for (std::size_t i=0;i<width;++i) bytes_[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// SetPath는 경로 탐색 결과를 받는 경계이며 경로를 추측해서 생성하지 않는다.
void RawPathRuntime::SetPath(std::span<const RawPathPoint> points,std::span<const std::int32_t> directions,std::span<const double> times) {
    if (points.size()!=directions.size() || points.size()>=layout_.capacity || (!times.empty() && times.size()!=points.size()))
        throw std::invalid_argument("경로 입력 크기");
    Write(0x10,points.size());
    // 실제 append와 같은 꼭짓점/방향/시각 배열의 인덱스를 사용한다.
    for (std::size_t i=0;i<points.size();++i) {
        Write(0x14+i*8,Bits(points[i].x));Write(0x18+i*8,Bits(points[i].y));
        Write(layout_.directions+i*4,std::bit_cast<std::uint32_t>(directions[i]));
        Write(layout_.times+i*8,times.empty() ? 0 : std::bit_cast<std::uint64_t>(times[i]),8);
    }
}
// 음수/과대 인덱스는 원본 assert에 대응하는 명시적 예외다.
void RawPathRuntime::Check(std::int32_t index) const {
    if (index<0 || index>=Count()) throw std::out_of_range("경로 꼭짓점 범위");
}
// 배열 접근은 같은 raw 인덱스를 사용한다.
RawPathPoint RawPathRuntime::Point(std::int32_t index) const { Check(index);return {std::bit_cast<float>(static_cast<std::uint32_t>(Read(0x14+index*8))),std::bit_cast<float>(static_cast<std::uint32_t>(Read(0x18+index*8)))}; }
// 현재 시각 표의 double을 반환한다.
double RawPathRuntime::Time(std::int32_t index) const { Check(index);return std::bit_cast<double>(Read(layout_.times+index*8,8)); }
// signed 방향 번호를 반환한다.
std::int32_t RawPathRuntime::Direction(std::int32_t index) const { Check(index);return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Read(layout_.directions+index*4))); }
// 방향 번호를 제한하지 않는 원본 산술이다. 유효 0..7 입력에서는 최단 signed 회전이다.
double RawPathRuntime::DirectionDelta(std::int32_t previous,std::int32_t next) {
    auto forward=static_cast<std::int64_t>(next)-previous,backward=static_cast<std::int64_t>(previous)-next;
    if (next<previous) forward+=8;if (previous<next) backward+=8;
    return static_cast<double>(forward<=backward ? forward : -backward)*kDirectionDegrees;
}
// 패치는 reciprocal을 float으로 저장하고, CD는 turn 시간만 float으로 저장한다.
void RawPathRuntime::Prepare(double now,double startTime,float speed,float turningSpeed,std::int32_t firstDirection) {
    if (Count()<1 || speed==0 || turningSpeed==0) throw std::invalid_argument("경로 시각 입력");
    double time=now-startTime>kLateStart ? startTime : now;Write(layout_.times,std::bit_cast<std::uint64_t>(time),8);
    Write(layout_.directions,std::bit_cast<std::uint32_t>(firstDirection));
    const float inverseSpeed=static_cast<float>(1.0/static_cast<double>(speed)),inverseTurning=static_cast<float>(1.0/static_cast<double>(turningSpeed));
    // 원본 현재 시각은 꼭짓점마다 double에 저장되고 다음 꼭짓점에서 다시 읽힌다.
    for (std::int32_t i=1;i<Count();++i) {
        const auto a=Point(i-1),b=Point(i);const double dx=static_cast<double>(b.x)-a.x,dy=static_cast<double>(b.y)-a.y;
        const double distance=std::sqrt(dy*dy+dx*dx),delta=std::abs(DirectionDelta(Direction(i-1),Direction(i)));
        if (edition_==OriginalEdition::Patch1078) time=static_cast<double>(inverseTurning)*static_cast<float>(delta)+distance*inverseSpeed+time;
        else time=distance/static_cast<double>(speed)+static_cast<float>(delta/static_cast<double>(turningSpeed))+time;
        Write(layout_.times+i*8,std::bit_cast<std::uint64_t>(time),8);
    }
}
// 도착 시각보다 빠를 때만 보간하며, 원본 currentPathVert를 한 번씩만 전진한다.
RawPathSample RawPathRuntime::Step(double now,RawPathPoint parent,RawPathObservation& observation,const std::function<void()>& stepSound,std::uint64_t cdObservationStamp) {
    RawPathSample result{parent,Angle(),false};const auto current=Current();
    if (current>=Count()) {
        if (current<1 || current!=Count()) throw std::out_of_range("경로 종료 단계");
        return {Point(current-1),static_cast<float>(Direction(current-1))*kDirectionDegrees,true};
    }
    observation.flags|=1;observation.stamp=edition_==OriginalEdition::Patch1078 ? 0 : cdObservationStamp;
    const auto target=Point(current);const auto targetTime=Time(current);
    if (now<targetTime) {
        if (current!=0) {
            const double previousTime=Time(current-1);const auto previous=Point(current-1);
            if (previousTime<=now) {
                const float scale=static_cast<float>((now-previousTime)/(targetTime-previousTime));
                if (!(scale>=0 && scale<=1)) throw std::out_of_range("경로 보간 비율");
                const float base=static_cast<float>(Direction(current-1))*kDirectionDegrees;
                const float change=static_cast<float>(DirectionDelta(Direction(current-1),Direction(current))*scale);
                double angle=static_cast<double>(base)+change;
                if (edition_==OriginalEdition::Cd1072) angle=static_cast<float>(angle);
                if (angle<0) angle+=kFullTurn;else if (angle>=kFullTurn) angle-=kFullTurn;
                result.angle=static_cast<float>(angle);Write(layout_.angle,Bits(result.angle));
                result.position={static_cast<float>((static_cast<double>(target.x)-previous.x)*scale+previous.x),static_cast<float>((static_cast<double>(target.y)-previous.y)*scale+previous.y)};
            }
        }
    } else {
        result.position=target;result.angle=static_cast<float>(Direction(current))*kDirectionDegrees;Write(layout_.current,current+1);
        result.finished=current+1>=Count();
        if (!result.finished && ((current+1)&1)==0) { if (!stepSound) throw std::logic_error("경로 발소리 효과 누락");stepSound(); }
    }
    return result;
}
// 상태 필드는 원본 생성자/RunFrame과 같은 폭이다.
std::int32_t RawPathRuntime::Count() const { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Read(0x10))); }
// 현재 단계는 signed DWORD다.
std::int32_t RawPathRuntime::Current() const { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Read(layout_.current))); }
// 보간에서 마지막으로 저장한 각도다.
float RawPathRuntime::Angle() const { return std::bit_cast<float>(static_cast<std::uint32_t>(Read(layout_.angle))); }
// finished는 도착 가상 호출 직전에 켜는 별도 필드다.
bool RawPathRuntime::Finished() const { return Read(layout_.finished)!=0; }
// Init은 currentPathVert만 0으로 만든다.
void RawPathRuntime::ResetCurrent() { Write(layout_.current,0); }
// 도착1002는 finished를 다시 0으로 만든다.
void RawPathRuntime::SetFinished(bool value) { Write(layout_.finished,value ? 1 : 0); }
// 현재 단계는 nVert까지 허용하며 각도/finished는 원본 저장 폭으로 복사한다.
void RawPathRuntime::RestoreStep(std::int32_t current,float angle,bool finished) {
    if (current<0 || current>Count() || !std::isfinite(angle)) throw std::out_of_range("경로 상태 이관");
    Write(layout_.current,static_cast<std::uint32_t>(current));Write(layout_.angle,Bits(angle));SetFinished(finished);
}
// 첫 대상은 부모의 가상 도착 첫 인자다.
std::uint32_t RawPathRuntime::TargetA() const { return static_cast<std::uint32_t>(Read(layout_.targetA)); }
// 둘째 대상은 부모의 가상 도착 둘째 인자다.
std::uint32_t RawPathRuntime::TargetB() const { return static_cast<std::uint32_t>(Read(layout_.targetB)); }
// 두 번호를 원본 DWORD로 저장한다.
void RawPathRuntime::SetTargets(std::uint32_t first,std::uint32_t second) { Write(layout_.targetA,first);Write(layout_.targetB,second); }
// 원본 상위 시작 함수는 tailByte라는 인자도 실제로 DWORD 전체를 기록한다.
void RawPathRuntime::SetCommandFields(std::uint32_t helper,std::uint32_t tailWord,std::uint32_t tailByte,RawPathPoint destination) {
    Write(layout_.helper,helper);
    const auto tail=edition_==OriginalEdition::Patch1078 ? 0x186e4U : 0x2f1cU;
    Write(tail,tailWord);Write(tail+4,tailByte);
    if (edition_==OriginalEdition::Patch1078) { Write(0x186dc,Bits(destination.x));Write(0x186e0,Bits(destination.y)); }
}
// 원본이 비교한 연속 시작 꼭짓점의 좌표 쌍만 실제 배열에 기록한다.
void RawPathRuntime::ReplacePoint(std::int32_t index,RawPathPoint point) {
    Check(index);Write(0x14+index*8,Bits(point.x));Write(0x18+index*8,Bits(point.y));
}
// 원본 base +4/+8은 Form/부모 DWORD다.
void RawPathRuntime::SetAttachment(Sid form,Sid parent) { Write(4,form.value);Write(8,parent.value); }
}
