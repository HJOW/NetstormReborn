// 원본 float 저장과 두 난수의 호출 횟수·순서를 유지한다.
#include "o/RawShrapnelFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// PE의 실제 binary32 CRT 단위와 붕괴 최소 수명/폭이다.
constexpr float kCrtUnit=std::bit_cast<float>(0x38000100U),kCollapseMinimum=std::bit_cast<float>(0x3dcccccdU);
// 실제 binary64 상수다. 폭2.4는 PE에 저장한 binary32 값의 넓은 표현이다.
constexpr double kGameUnit=1.0/65536.0,kSpread=.3,kCollapseWidth=2.4000000953674316,kTimeScale=10;
// 실제 CRT의0방향 절삭을 정상 signed32 범위에서만 수행한다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("픽셀 파편 정수화 범위 오류");return static_cast<std::int32_t>(value);
}
// 원본 비정렬 좌표 DWORD를 읽는다.
float Float(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t word=0;
    // 네 바이트의 little endian 위치를 유지한다.
    for (std::size_t index=0;index<4;++index) word|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return std::bit_cast<float>(word);
}
// 손상된 이미지/꼬리는 변경 전에 거부한다.
void Validate(const ShrapnelFrameFields& fields) {
    const auto size=static_cast<std::int64_t>(fields.image.width)*fields.image.height;
    if (fields.image.width<1 || fields.image.height<1 || size>std::numeric_limits<std::int32_t>::max() || static_cast<std::size_t>(size)!=fields.image.pixels.size() ||
        fields.particles.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()/24) || fields.total<0 || static_cast<std::size_t>(fields.total)>fields.particles.size()) throw std::invalid_argument("픽셀 파편 이미지/용량/꼬리 오류");
}
}
// 원문 계산 외의 출력 경계는 모두 명시 연결해야 한다.
RawShrapnelFrame::RawShrapnelFrame(const SidPool& pool,GameRandom& game,CrtRandom& crt,const ShrapnelFrameState& state,ShrapnelFrameHooks hooks)
    :pool_(pool),game_(game),crt_(crt),state_(state),hooks_(std::move(hooks)) {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || !hooks_.reserve || !hooks_.release || !hooks_.invalidate || !hooks_.pixel || !hooks_.removeForm) throw std::invalid_argument("픽셀 파편 원본/외부 경계 누락");
}
// 입력 끝점이20/14여도 원문은20에0..6을 더한다.
double RawShrapnelFrame::Random(float low,float high) const { return static_cast<double>(game_.Next(65536))*kGameUnit*std::abs(static_cast<double>(high)-low)+low; }
// 작업 bitmap을 먼저 표시하여 같은 픽셀을 다음 가상 묶음에서 재사용하지 않는다.
void RawShrapnelFrame::Split(ShrapnelFrameFields& fields,std::span<std::uint8_t> work,std::int32_t originX,std::int32_t originY,std::int32_t x,std::int32_t y,float vx,float vy,float life) const {
    std::int32_t dx=0;auto width=static_cast<std::int32_t>(crt_.Next()%3+3);
    // 마지막 실패 검사에도 CRT 난수를 소비한다.
    while (dx<width) {
        std::int32_t dy=0;auto height=static_cast<std::int32_t>(crt_.Next()%3+3);
        // 원문은3..5 범위를 매번 다시 뽑으므로 처음 정한 크기의 사각형과 다르다.
        while (dy<height) {
            if (x+dx>0 && x+dx<fields.image.width && y+dy>0 && y+dy<fields.image.height) {
                const auto pixel=static_cast<std::size_t>((y+dy)*fields.image.width+x+dx);
                if (work[pixel]!=255) {
                    work[pixel]=255;const auto remaining=static_cast<float>(static_cast<double>(crt_.Next())*kCrtUnit+life);
                    if (static_cast<std::size_t>(fields.total)>=fields.particles.size()) throw std::length_error("픽셀 파편 용량 원본 assert");
                    fields.particles[static_cast<std::size_t>(fields.total++)]={static_cast<float>((static_cast<double>(dx)+originX)+x),static_cast<float>((static_cast<double>(dy)+originY)+y),vx,vy,remaining,static_cast<std::uint32_t>(pixel+1)};
                }
            }
            ++dy;height=static_cast<std::int32_t>(crt_.Next()%3+3);
        }
        ++dx;width=static_cast<std::int32_t>(crt_.Next()%3+3);
    }
}
// 각 가상 생성기의 넓은 수학/float 인자 저장 위치를 보존한다.
void RawShrapnelFrame::Seed(ShrapnelFrameFields& fields,ShrapnelKind kind,std::span<std::uint8_t> work,std::int32_t originX,std::int32_t originY,std::int32_t x,std::int32_t y) const {
    float vx=0,vy=0,life=0;
    if (kind==ShrapnelKind::Explosion) {
        const double vertical=static_cast<double>(y-fields.image.height/2)*kSpread;
        vy=static_cast<float>(vertical-Random(20,14));life=static_cast<float>((static_cast<double>(crt_.Next())*kCrtUnit)*5+.5);vx=static_cast<float>(static_cast<double>(x-fields.image.width/2)*kSpread);
    } else if (kind==ShrapnelKind::Collapse) {
        const double random=(static_cast<double>(crt_.Next())*kCrtUnit)*kCollapseWidth+kCollapseMinimum;
        life=static_cast<float>((random*std::sqrt(static_cast<double>(fields.image.height-y+1)/fields.image.height))*10);
    } else {
        const double ratio=static_cast<double>(y)/fields.image.height,base=(1-ratio)*12+12.5;vx=static_cast<float>(ratio);
        life=static_cast<float>((static_cast<double>(crt_.Next())*kCrtUnit)*std::abs(3.5-base)+std::min(3.5,base));
    }
    Split(fields,work,originX,originY,x,y,vx,vy,life);
}
// 캡처 원본은 유지하고 임시 작업 복사본만0xff로 바꾼다.
void RawShrapnelFrame::Prepare(ShrapnelFrameFields& fields,ShrapnelKind kind) const {
    Validate(fields);const auto parent=fields.registration.parent;if (parent<1 || static_cast<std::uint32_t>(parent)>=pool_.Capacity()) throw std::out_of_range("픽셀 파편 부모 번호 오류");
    const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(parent)});fields.registration.zOrder=static_cast<std::int8_t>(raw[35]);
    const double x=static_cast<double>(Float(raw,14))*state_.pixelScaleX,y=static_cast<double>(Float(raw,18))*state_.pixelScaleY;
    auto& rect=fields.registration;rect.left=rect.right=static_cast<float>(x);rect.top=rect.bottom=static_cast<float>(y);
    const auto originX=Ftol(x-fields.image.hotspotX),originY=Ftol(y-fields.image.hotspotY);
    const auto count=std::max<std::size_t>(1,static_cast<std::size_t>(std::count_if(fields.image.pixels.begin(),fields.image.pixels.end(),[](auto color) { return color!=255; })));
    if (!fields.particles.empty()) hooks_.release(false);if (!hooks_.reserve(static_cast<std::uint32_t>(count*24))) throw std::bad_alloc();fields.particles.assign(count,ShrapnelParticle{});fields.total=0;
    if (!hooks_.reserve(static_cast<std::uint32_t>(fields.image.pixels.size()))) throw std::bad_alloc();auto work=fields.image.pixels;
    // 원본 행/열 순서로 아직 선택되지 않은 픽셀마다 가상 생성기를 호출한다.
    for (std::int32_t row=0;row<fields.image.height;++row) {
        // 같은 작업 그림의 다음 현재 값을 매번 읽는다.
        for (std::int32_t column=0;column<fields.image.width;++column) if (work[static_cast<std::size_t>(row*fields.image.width+column)]!=255) Seed(fields,kind,work,originX,originY,column,row);
    }
    hooks_.release(true);fields.lastTime=state_.now;fields.elapsed=0;
}
// 한 프레임의 delta/가속도를 먼저 저장하고 y 갱신/dirty에는 넓은 새 속도/좌표를 사용한다.
void RawShrapnelFrame::Step(ShrapnelFrameFields& fields) const {
    Validate(fields);const auto parent=fields.registration.parent;if (parent<1 || static_cast<std::uint32_t>(parent)>=pool_.Capacity()) throw std::out_of_range("픽셀 파편 부모 번호 오류");
    const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(parent)});const auto x=static_cast<float>(static_cast<double>(Float(raw,14))*state_.pixelScaleX),y=static_cast<float>(static_cast<double>(Float(raw,18))*state_.pixelScaleY);
    float left=x,top=y,right=x,bottom=y;const double wide=(state_.now-fields.lastTime)*kTimeScale;fields.delta=static_cast<float>(wide);fields.scaledDelta=static_cast<float>(wide*state_.rate);fields.lastTime=state_.now;fields.elapsed+=fields.delta;
    const auto acceleration=static_cast<float>(static_cast<double>(fields.gravity)*fields.delta);bool alive=false;
    // 사용 꼬리가 아닌 용량 전체에서 진입 양수 수명만 갱신한다.
    for (auto& particle:fields.particles) {
        if (!(particle.life>0)) continue;alive=true;particle.life=static_cast<float>(static_cast<double>(particle.life)-fields.delta);
        const double vy=static_cast<double>(acceleration)+particle.velocityY;particle.velocityY=static_cast<float>(vy);
        particle.x=static_cast<float>(static_cast<double>(particle.velocityX)*fields.scaledDelta+particle.x);const double nextY=vy*fields.scaledDelta+particle.y;particle.y=static_cast<float>(nextY);
        const auto ix=Ftol(particle.x),iy=Ftol(nextY);left=std::min(left,static_cast<float>(static_cast<double>(ix)-3));top=std::min(top,static_cast<float>(static_cast<double>(iy)-3));right=std::max(right,static_cast<float>(static_cast<double>(ix)+3));bottom=std::max(bottom,static_cast<float>(static_cast<double>(iy)+3));
    }
    if (!alive) { hooks_.removeForm();return; }auto& rect=fields.registration;Invalidate(rect.left,rect.top,rect.right,rect.bottom);Invalidate(left,top,right,bottom);rect.left=left;rect.top=top;rect.right=right;rect.bottom=bottom;
}
// 실제 공통487ba0의 표시 여유/카메라 적용이다.
void RawShrapnelFrame::Invalidate(float left,float top,float right,float bottom) const {
    hooks_.invalidate({Ftol((static_cast<double>(left)-1)-state_.cameraX),Ftol((static_cast<double>(top)-1)-state_.cameraY),Ftol((static_cast<double>(right)+1)-state_.cameraX),Ftol((static_cast<double>(bottom)+1)-state_.cameraY)});
}
// x는 카메라 감산 뒤 float에 저장하고 y는 넓은 값 그대로 절삭하는 원문 순서다.
void RawShrapnelFrame::Draw(const ShrapnelFrameFields& fields) const {
    Validate(fields);
    // 실제 사용 꼬리의 생존 픽셀만 출력한다.
    for (std::int32_t index=0;index<fields.total;++index) {
        const auto& particle=fields.particles[static_cast<std::size_t>(index)];if (!(particle.life>0)) continue;
        if (!particle.pixel || particle.pixel>fields.image.pixels.size()) throw std::out_of_range("픽셀 파편 색 주소 토큰 오류");
        const auto x=static_cast<float>(static_cast<double>(Ftol(particle.x))-state_.cameraX);const double y=static_cast<double>(Ftol(particle.y))-state_.cameraY;
        hooks_.pixel({Ftol(x),Ftol(y),fields.image.pixels[particle.pixel-1]});
    }
}
}
