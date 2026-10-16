//일반 객체 생성과 화면 효과 경계는 외부에 두고 원본 좌표/태그/호출 순서를 유지한다.
#include "o/RawFenceCreate.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kMarkerRoundBias=0.5f; //0043fa50은 현재float에0.5f를 x87에서 더한 뒤 CRT 절삭한다.
//선택한 실제 소비자만 요구하고 앞선 쓰기/생성을 되돌리지 않는다.
template<class Callback> const Callback& NeedFenceCreate(const Callback& callback) { if (!callback) throw std::logic_error("펜스 생성의 실제 하위 미연결");return callback; }
//원본 지정 폭의 비정렬 필드만 쓴다.
void FenceCreatePut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); //낮은 바이트부터 원본 폭만 기록한다.
}
//유한 정상 보드 좌표의 원본 CRT signed 절삭이다.
std::int32_t FenceCreateInt(double value) { if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("펜스 생성 CRT 좌표 범위 오류");return static_cast<std::int32_t>(value); }
//원본 정상 좌표의 방향0/2/4/6을 선택하며 동률은 세로를 우선한다.
std::array<float,2> MarkerStep(std::array<float,4> points) {
    const auto dx=static_cast<float>(static_cast<double>(points[2])-points[0]),dy=static_cast<float>(static_cast<double>(points[3])-points[1]);
    if (std::abs(dx)<=std::abs(dy)) return {0,dy>0 ? 1.0f : -1.0f};return {dx<=0 ? -1.0f : 1.0f,0};
}
//모든 외부 좌표는 실제 Pop 계약과 같은 유한 입력이어야 한다.
void RequireFencePoints(std::array<float,4> points) {
    for (const auto coordinate:points) if (!std::isfinite(coordinate)) throw std::out_of_range("펜스 생성의 비유한 좌표"); //원본 배열 밖/비유한 메모리 입력은 명시 거부한다.
}
}
//현재 프레임/중심의 다른 월드를 섞지 않는다.
RawFenceMarkerCreate::RawFenceMarkerCreate(SidPool& pool,const RawGunTarget& target,const std::uint32_t& type,FenceCreateHooks hooks)
    :pool_(pool),target_(target),type_(type),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || &target.Pool()!=&pool) throw std::invalid_argument("fencemark 생성 판본/풀 불일치");
}
//스냅한 구간은 이동 전 위치에서 구하고 생성 좌표는 한 걸음 이동한 뒤의float다.
std::vector<std::array<float,2>> RawFenceMarkerCreate::Points(std::array<float,4> centers) {
    RequireFencePoints(centers);const auto step=MarkerStep(centers);
    const auto incomingX=std::clamp(centers[2],1.0f,255.0f),incomingY=std::clamp(centers[3],1.0f,255.0f);
    const bool valid=centers[0]>0 && centers[1]>0 && centers[0]<256 && centers[1]<256;
    const std::array<float,4> bounds{valid ? std::min(centers[0],incomingX) : incomingX,valid ? std::min(centers[1],incomingY) : incomingY,
        valid ? std::max(centers[0],incomingX) : incomingX,valid ? std::max(centers[1],incomingY) : incomingY};
    const auto inside=[&](std::array<float,2> point) { return bounds[0]<=point[0] && point[0]<=bounds[2] && bounds[1]<=point[1] && point[1]<=bounds[3]; }; //x87 원본은 경계 동일점도 허용한다.
    std::array<float,2> current{centers[0],centers[1]};std::array<std::int32_t,2> previous{-1,-1};std::vector<std::array<float,2>> result; //현재float/직전4칸 버킷/생성 위치 순서다.
    while (inside(current)) { //원본은 보드 사각형을 벗어날 때 종료한다.
        const std::array<std::int32_t,2> cell{std::min(FenceCreateInt(static_cast<double>(current[0])+kMarkerRoundBias)/4,63),std::min(FenceCreateInt(static_cast<double>(current[1])+kMarkerRoundBias)/4,63)};
        current[0]+=step[0];current[1]+=step[1];if (!inside(current)) break;
        if (cell!=previous) { previous=cell;result.push_back(current); }
    }
    return result;
}
//원본 생성자는 QA WORD를 쓴 뒤 통지하고 끝점의 high8/low8을 분리해서 기록한다.
void RawFenceMarkerCreate::Create(Sid parent,Sid other) const {
    const auto second=target_.Center(other),first=target_.Center(parent);
    for (const auto& point:Points({first[0],first[1],second[0],second[1]})) { //부모/대상 좌표의 이후 변경은 이미 정해진 로컬 이동 경로를 바꾸지 않는다.
        const auto created=NeedFenceCreate(hooks_.create)(type_,2);if (!created.value) throw std::logic_error("fencemark 생성의 실제 새 SID 누락");
        FenceCreatePut(pool_.AllocatedBytes(created),12,parent.value,2);NeedFenceCreate(hooks_.notifySurface)(created);
        FenceCreatePut(pool_.AllocatedBytes(created),26,(other.value>>8)&255U);pool_.AllocatedBytes(created)[30]=static_cast<std::uint8_t>(other.value);
        NeedFenceCreate(hooks_.setOwner)(created,pool_.Slot(parent)[34]);NeedFenceCreate(hooks_.pop)(created,point[0],point[1],0x40);
    }
}
//두 판본의 포스 필드 생성자를 동일하게 취급하지 않는다.
RawFenceShieldCreate::RawFenceShieldCreate(SidPool& pool,std::span<const RiftTypeFrames> frames,const std::uint32_t& type,FenceCreateHooks hooks)
    :pool_(pool),frames_(frames),type_(type),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || frames.size()!=188) throw std::invalid_argument("포스 필드 생성 판본/TYPE 불일치");
}
//원본mode1은 좌표를 정수로 바꾸지 않고 현재float를 그대로 가상 Pop에 전달한다.
std::vector<std::array<float,2>> RawFenceShieldCreate::Points(std::array<float,4> endpoints) {
    RequireFencePoints(endpoints);auto x=endpoints[0]-1.0f,y=endpoints[1]-1.0f,endX=endpoints[2]-1.0f,endY=endpoints[3]-1.0f;float dx{},dy{}; //원본 로컬float의 현재 끝점/증분이다.
    if (x==endX) {
        if (endY<=y) { dy=-1;y-=3;if (y<endY) endY=y; } else { dy=1;endY-=3;if (endY<=y) y=endY; }
        if (x<=endX) x=endX;if (dy>0) { if (endY<=y) y=endY; } else if (y<=endY) y=endY;
    } else {
        if (endX<=x) { x+=1;dx=-1;endX-=1; } else { x-=1;dx=1;endX+=1; }
        y-=1;endY-=1;x=dx+dx+x;endX-=dx+dx;
        if (dx<=0) { if (x<=endX) x=endX; } else if (endX<=x) x=endX;if (y<=endY) y=endY;
    }
    std::vector<std::array<float,2>> result; //원본100회 상한의 실제float Pop 지점이다.
    for (std::uint32_t remaining=100;remaining;--remaining) { //끝점은 한 번 포함하고 그 뒤 종료한다.
        result.push_back({x,y});if (x==endX && y==endY) break;x+=dx;y+=dy;
    }
    return result;
}
//직접 프레임 쓰기와 생성 직후 bit20의 조기 종료를 유지한다.
void RawFenceShieldCreate::Create(std::array<float,4> endpoints,std::uint32_t firstOwner,std::uint32_t secondOwner,std::int32_t firstId,std::int32_t secondId) const {
    const auto letter=static_cast<std::uint8_t>(endpoints[0]==endpoints[2] ? 'J' : 'K'); //수직J/수평K의 실제TYPE 첫 프레임이다.
    for (const auto& point:Points(endpoints)) { //현재 타입 전역은 각 객체 생성 직전에 다시 읽는다.
        const auto created=NeedFenceCreate(hooks_.create)(type_,2);if (!created.value) throw std::logic_error("포스 필드 생성의 실제 새 SID 누락");
        const auto raw=pool_.Slot(created);if (raw[40]&0x20U) return;if (raw[10]>=frames_.size()) throw std::out_of_range("포스 필드 생성의 현재 TYPE 범위 오류");
        FenceCreatePut(pool_.AllocatedBytes(created),36,static_cast<std::uint32_t>(frames_[raw[10]].Run(letter).first));
        NeedFenceCreate(hooks_.setOwner)(created,firstOwner==secondOwner || secondId<firstId ? firstOwner : secondOwner);
        NeedFenceCreate(hooks_.pop)(created,point[0],point[1],0x40);
    }
}
}
