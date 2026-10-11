// 건설 입자의 활성/만료/재배치와 판본마다 다른 HP 폭·실수 저장 순서를 보존한다.
#include "o/RawConstructionFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 raw 좌표·HP·abstract 위치와 투명/상태 비트다.
constexpr std::size_t kX=14,kY=18,kHp=26,kPatchExtra=40,kCdExtra=35,kState=11;
constexpr std::uint8_t kTransparent=255,kFree=1,kAbstract=1;
// 실제 시각 helper의 double 계수와 dirty 영역 여유다.
constexpr double kTimeScale=10.0,kMargin=1.0;
// 정렬되지 않은 little endian raw 값을 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t result=0;
    // 지정 폭만 낮은 자리부터 조합한다.
    for (std::size_t i=0;i<width;++i) result|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return result;
}
// 실제 CRT ftol의 하위 signed DWORD다. 비유한 값/64비트 범위 밖은 하위 DWORD 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 경계가 빠진 표시 갱신은 픽셀 변경 전에 거부한다.
RawConstructionFrame::RawConstructionFrame(SidPool& pool,ConstructionVisualState& visualState,ConstructionFrameState& state,
    const ConstructionFramePalette& palette,GameRandom& random,ConstructionFrameHooks hooks)
    :pool_(pool),visualState_(visualState),state_(state),palette_(palette),random_(random),hooks_(std::move(hooks)) {
    if (!hooks_.maxHitPoints || !hooks_.invalidate || !hooks_.draw || !hooks_.kill) throw std::invalid_argument("건설 프레임 경계 누락");
}
// 원본 포인터/카운터가 의미 있는 호스트 저장소 안인지 확인한다.
void RawConstructionFrame::Validate(const ConstructionFrameFields& fields) const {
    const auto& v=fields.visual;
    if (v.image.width<=0 || v.image.height<=0 || static_cast<std::uint64_t>(v.image.width)*v.image.height!=v.image.pixels.size() ||
        v.image.pixels.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) || v.output.size()!=v.image.pixels.size() ||
        v.lastRow!=(v.image.height-1)*static_cast<std::int64_t>(v.image.width) ||
        v.capacity>static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) || v.particles.size()!=v.capacity || v.total>v.capacity || v.active>v.total)
        throw std::invalid_argument("건설 프레임 저장소 오류");
    // 현재 사용 입자의 목적지는 모두 출력 버퍼 안이어야 한다.
    for (std::uint32_t i=0;i<v.total;++i) {
        if (v.particles[i].pixel>=v.output.size() || !std::isfinite(v.particles[i].life)) throw std::invalid_argument("건설 프레임 입자 오류");
    }
}
// 원본 float 난수는 저장 이전의 값을 되돌려준다. 재시작 계산은 마지막에만 float로 저장한다.
double RawConstructionFrame::Life(float minimum,float maximum) const {
    const auto fraction=static_cast<double>(random_.Next(65536))/65536.0;
    return fraction*std::fabs(static_cast<double>(maximum)-minimum)+minimum;
}
// CD의 잘못된 픽셀 경로는 확보 카운터만 먼저 늘린다. 패치의 범위 밖 접근은 호스트에서 거부한다.
bool RawConstructionFrame::Append(ConstructionFrameFields& fields,std::int32_t pixel) const {
    auto& v=fields.visual;
    if (v.total>=v.capacity) return false;
    if (v.particles.size()!=v.capacity) throw std::invalid_argument("건설 프레임 입자 용량 오류");
    const bool valid=pixel>=0 && static_cast<std::size_t>(pixel)<v.image.pixels.size() && v.output.size()==v.image.pixels.size();
    if (!valid && pool_.Edition()==OriginalEdition::Patch1078) throw std::out_of_range("건설 입자 픽셀 오류");
    auto& particle=v.particles[v.total++];if (!valid) return false;
    particle.life=static_cast<float>(Life(v.minimum,v.maximum));particle.pixel=static_cast<std::uint32_t>(pixel);
    particle.active=0;particle.color=v.image.pixels[pixel];v.image.pixels[pixel]=kTransparent;++v.selected;return true;
}
// 원본은 행의 좌우 경계를 별도로 검사하지 않는다. 선형 인덱스의 세 이웃 순서를 유지한다.
void RawConstructionFrame::Expand(ConstructionFrameFields& fields,std::uint32_t pixel) const {
    auto& v=fields.visual;Validate(fields);
    if (pixel<=static_cast<std::uint32_t>(v.image.width) || pixel>=static_cast<std::uint32_t>(v.lastRow)) return;
    // 오른쪽·왼쪽·위 이웃을 차례로 추가하며 앞 선택이 바꾼 버퍼/용량을 다시 읽는다.
    for (const auto neighbor:{pixel+1,pixel-1,pixel-static_cast<std::uint32_t>(v.image.width)}) {
        if (v.image.pixels[neighbor]!=kTransparent) static_cast<void>(Append(fields,static_cast<std::int32_t>(neighbor)));
    }
}
// 원본 색수는 배열 길이와 다르다. life>=maximum이 읽는 인접 DWORD도 명시 자료가 있어야 한다.
std::uint32_t RawConstructionFrame::Color(float life,float maximum) const {
    if (!std::isfinite(life) || !std::isfinite(maximum) || maximum<=0 || palette_.count<0) throw std::invalid_argument("건설 색 표 입력 오류");
    const auto value=std::min(life,maximum);double position;
    if (pool_.Edition()==OriginalEdition::Patch1078) {
        const auto reciprocal=static_cast<float>(1.0/static_cast<double>(maximum));
        position=static_cast<double>(static_cast<float>(palette_.count))*value*reciprocal;
    } else position=static_cast<double>(value)*palette_.count/maximum;
    const auto index=std::max(0,Ftol(position));
    if (static_cast<std::size_t>(index)>=palette_.entries.size()) throw std::out_of_range("건설 색 표 읽기 범위 오류");
    return palette_.entries[index];
}
// 두 판본 모두 반올림 전 시간 차이에 배율을 곱하고, 저장한 float delta를 double 누적 시각에 더한다.
void RawConstructionFrame::Clock(ConstructionFrameFields& fields) const {
    auto& v=fields.visual;
    if (!v.firstDraw) { v.lastTime=state_.now;v.elapsed=0;v.firstDraw=true; }
    const auto difference=(state_.now-v.lastTime)*kTimeScale;fields.delta=static_cast<float>(difference);
    fields.scaledDelta=static_cast<float>(difference*state_.rate);
    v.lastTime=state_.now;
    v.elapsed+=fields.delta;
}
// dirty 큐 경계가 카메라를 바꾸면 새 영역의 요청은 바뀐 카메라를 사용한다.
void RawConstructionFrame::Rectangle(ConstructionVisualFields& v,float left,float top,float right,float bottom) const {
    // 카메라와 1픽셀 여유를 적용한 영역을 원본 순서대로 정수로 변환한다.
    const auto rectangle=[this](float l,float t,float r,float b) {
        return ConstructionFrameRect{Ftol(static_cast<double>(l)-state_.cameraX-kMargin),Ftol(static_cast<double>(t)-state_.cameraY-kMargin),
            Ftol(static_cast<double>(r)-state_.cameraX+kMargin),Ftol(static_cast<double>(b)-state_.cameraY+kMargin)};
    };
    hooks_.invalidate(rectangle(v.left,v.top,v.right,v.bottom));hooks_.invalidate(rectangle(left,top,right,bottom));
    v.left=left;v.top=top;v.right=right;v.bottom=bottom;
}
// HP 활성화 목표는 루트에서 한 번 계산한다. 입자 확장으로 늘어나는 selected/total은 매 반복 다시 읽는다.
void RawConstructionFrame::Run(ConstructionFrameFields& fields) const {
    auto& v=fields.visual;const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!patch && (v.image.pixels.empty() || !v.parent.value || v.parent.value>=pool_.Capacity())) return;
    if (!v.parent.value) throw std::out_of_range("건설 프레임 부모 오류");
    const auto raw=pool_.Slot(v.parent);if (!patch && (raw[kState]&kFree)) return;Validate(fields);
    const auto lx=static_cast<double>(visualState_.scaleX)*std::bit_cast<float>(Read(raw,kX))-v.image.hotspotX;
    const auto ty=static_cast<double>(visualState_.scaleY)*std::bit_cast<float>(Read(raw,kY))-v.image.hotspotY;
    const auto left=static_cast<float>(lx),top=static_cast<float>(ty);
    const auto right=static_cast<float>((patch ? static_cast<double>(left) : lx)+v.image.width),bottom=static_cast<float>(ty+v.image.height);
    Clock(fields);
    const auto savedHp=static_cast<float>(std::bit_cast<std::int32_t>(Read(raw,kHp)));const auto maximum=hooks_.maxHitPoints(v.parent);
    const auto hp=patch ? static_cast<double>(savedHp) : static_cast<double>(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Read(raw,kHp,2))));
    const auto denominator=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(maximum)-1);
    const auto target=Ftol(hp/denominator*std::bit_cast<std::int32_t>(v.opaque));
    // 활성 앞 구간에서 아직 선택하지 않은 입자를 뽑고 이웃을 확장한 뒤 active 위치로 교환한다.
    while (std::bit_cast<std::int32_t>(v.selected)<target && v.active<v.total && v.total<v.capacity) {
        const auto index=v.active+random_.Next(v.total-v.active);const auto pixel=v.particles[index].pixel;
        Expand(fields,pixel);v.particles[index].active=1;std::swap(v.particles[v.active],v.particles[index]);++v.active;
    }
    std::uint32_t i=0;
    // 만료 입자는 마지막 활성 입자/마지막 전체 입자로 두 번 덮고 같은 위치를 다시 처리한다.
    while (i<v.active) {
        auto& particle=v.particles[i];const auto remainder=static_cast<double>(particle.life)-fields.delta;particle.life=static_cast<float>(remainder);
        if (remainder<=0) {
            v.output[particle.pixel]=particle.color;--v.active;--v.total;particle=v.particles[v.active];v.particles[v.active]=v.particles[v.total];
        } else { if (v.palette!=palette_.token) throw std::invalid_argument("건설 색 표 연결 오류");v.output[particle.pixel]=static_cast<std::uint8_t>(Color(particle.life,v.maximum));++i; }
    }
    // 미활성 입자는 원래 색을 확정하지 않고 난수 수명을 다시 정해 색 표 순환만 계속한다.
    for (;i<v.total;++i) {
        auto& particle=v.particles[i];const auto remainder=static_cast<double>(particle.life)-fields.delta;particle.life=static_cast<float>(remainder);
        if (remainder<=0) {
            const auto life=Life(0,v.maximum);
            particle.life=static_cast<float>(patch ? life+fields.delta+particle.life : life+particle.life+fields.delta);
        }
        if (v.palette!=palette_.token) throw std::invalid_argument("건설 색 표 연결 오류");
        v.output[particle.pixel]=static_cast<std::uint8_t>(Color(particle.life,v.maximum));
    }
    Rectangle(v,left,top,right,bottom);
    if (!(raw[patch ? kPatchExtra : kCdExtra]&kAbstract)) hooks_.kill(0);
}
// 원본은 total이 0이어도 capacity가 있으면 Draw를 제출한다. 입력으로 받은 두 가상 인자는 쓰지 않는다.
void RawConstructionFrame::Draw(const ConstructionFrameFields& fields) const {
    const auto& v=fields.visual;if (!v.capacity) return;
    if (v.total>v.capacity) throw std::invalid_argument("건설 Draw 입자 수 오류");
    if (!v.parent.value) throw std::out_of_range("건설 Draw 부모 오류");const auto raw=pool_.Slot(v.parent);
    const auto x=static_cast<float>(static_cast<double>(visualState_.scaleX)*std::bit_cast<float>(Read(raw,kX))-state_.cameraX);
    const auto y=static_cast<double>(visualState_.scaleY)*std::bit_cast<float>(Read(raw,kY))-state_.cameraY;
    hooks_.draw({Ftol(static_cast<double>(x)-v.image.hotspotX),Ftol(y-v.image.hotspotY),v.image.width,
        {0,0,v.image.width,v.image.height},v.output});
}
}
