// 원본 x87의 저장 전/후 값과 현재 raw 조회·미도달 장치·타원 요청 순서를 보존한다.
#include "o/RawEvilRayFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // EvilRay의 독립 프레임/그리기 몸체다.
namespace { // 실제 DWORD/CRT 변환과 장치 선택의 작은 연산이다.
constexpr double kClockScale=10,kEmissionInterval=15,kLife=13,kRadiusScale=8,kRadiusUnit=std::bit_cast<double>(std::uint64_t{0x3fb3b13b13b13b14}); // PE00501450/508ed0/508f00/508ef8/508ef0의 실제 값이다.
std::int32_t Integer(double value) { // 실제 __ftol의 signed64 결과 하위DWORD이며 비유한/범위 밖은 indefinite 하위0이다.
    if (!std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b)); } // x86 signed 덧셈의32비트 감김이다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) { // 비정렬 raw float의 모든 비트를 현재 물리에서 읽는다.
    std::uint32_t bits=0; // little endian DWORD를 조합한다.
    for (std::size_t index=0;index<4;++index) bits|=std::uint32_t(raw[offset+index])<<(index*8); // 지정 네 바이트만 읽는다.
    return std::bit_cast<float>(bits);
}
template<class Function> Function Need(const Function& function) { // 현재 선택 callable의 수명을 호출 종료까지 유지한다.
    if (!function) throw std::logic_error("EvilRay 현재 선택 장치 미연결");return function;
}
}
RawEvilRayFrame::RawEvilRayFrame(const SidPool& pool,const FieryTailFrameState& state,std::span<const std::uint32_t> colors,EvilRayFrameHooks hooks)
    :pool_(pool),state_(state),colors_(colors),hooks_(std::move(hooks)) { // 실제 효과는 Step/Draw가 원문 순서로 실행한다.
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("EvilRay 프레임 판본 오류");
}
void RawEvilRayFrame::Diagnose(std::string_view expression,std::uint32_t line) const { Need(hooks_.assertion)(expression,".\\Evilray.cpp",line); } // 진단 정상 복귀 뒤 caller가 현재 값을 계속 읽는다.
std::span<const std::uint8_t> RawEvilRayFrame::Physical(std::uint32_t sid) const { // 원문32비트 포인터 감김/호스트 밖 메모리는 명시 경계로 남긴다.
    if (sid>=pool_.Capacity() || sid>65535U) throw std::out_of_range("EvilRay 현재 SID 물리 범위");return pool_.Slot(Sid{static_cast<std::uint16_t>(sid)});
}
void RawEvilRayFrame::Invalidate(float left,float top,float right,float bottom) const { // 공통 dirty는 float를 먼저 정수화하는 Draw와 구분한다.
    Need(hooks_.invalidate)({Integer((static_cast<double>(left)-1)-state_.cameraX),Integer((static_cast<double>(top)-1)-state_.cameraY),Integer((static_cast<double>(right)+1)-state_.cameraX),Integer((static_cast<double>(bottom)+1)-state_.cameraY)});
}
void RawEvilRayFrame::Step(EvilRayFields& fields) const { // source 포인터는 진단 전에 저장하고 현재 TYPE/좌표는 진단 뒤 읽는다.
    const auto source=fields.source; // 원문에서 먼저 계산한 source 물리 주소의 번호 표현이다.
    if (!source || source>=pool_.Capacity()) Diagnose("sourceSquid.isValid()",0x55);
    const auto raw=Physical(source);const auto sid=Sid{static_cast<std::uint16_t>(source)};const auto width=Need(hooks_.shapeWidth)(sid); // 현재 가상 프레임 뒤 현재 raw 좌표를 조회한다.
    const float x=static_cast<float>(static_cast<double>(state_.pixelScaleX)*Coordinate(raw,14)-width/2),y=static_cast<float>(static_cast<double>(state_.pixelScaleY)*Coordinate(raw,18));float left=x,top=y,right=x,bottom=y; // signed 폭의2쪽 절삭과 원문 float 저장이다.
    if (!fields.first) { fields.lastTime=state_.now;fields.elapsed=0;fields.first=true; }
    const double difference=(state_.now-fields.lastTime)*kClockScale;fields.delta=static_cast<float>(difference);fields.scaledDelta=static_cast<float>(difference*state_.rate);fields.lastTime=state_.now;fields.elapsed+=static_cast<double>(fields.delta); //418454는 기존 누적 double에 FADD하며 clamp/시간 조각 반복이 없다.
    if (static_cast<double>(fields.nextEmission)<=fields.elapsed && !fields.stopping) {
        auto sound=Need(hooks_.soundId)(sid,false); // 최초 source+70 조회 뒤0일 때만 현재 부모+70을 읽는다.
        if (!sound) {
            const auto parent=static_cast<std::uint32_t>(fields.registration.parent);const auto parentRaw=Physical(parent); //0041bfb0이 진단 전에 저장한 부모 물리 주소다.
            if (!parentRaw[6] && !parentRaw[7]) Need(hooks_.assertion)("parent->getHead() != 0","\\Ns\\O\\Baseprocess.cpp",0xa4);
            sound=Need(hooks_.soundId)(Sid{static_cast<std::uint16_t>(parent)},false);
        }
        Need(hooks_.sound)(Coordinate(raw,14),Coordinate(raw,18),sound);
        if (fields.total<static_cast<std::int32_t>(fields.particles.size())) { // 원문487b20은 끝에만 붙이고 죽은 슬롯을 재사용하지 않는다.
            if (fields.total<0) throw std::out_of_range("EvilRay 현재 입자 꼬리 물리 범위");auto& particle=fields.particles[static_cast<std::size_t>(fields.total++)];particle.velocityY=3;particle.x=x;particle.y=y;particle.life=static_cast<float>(kLife);
        }
        fields.nextEmission=static_cast<float>(fields.elapsed+kEmissionInterval);
    }
    bool active=false; // 이번 호출에서 양수 수명으로 진입한 입자 유무다. 이번에 죽은 입자도 한 번 더 활성으로 센다.
    for (std::int32_t index=0;index<fields.total;++index) { // 실제 사용 꼬리는 음향 callback 뒤 현재 값으로 재조회한다.
        auto& particle=fields.particles.at(static_cast<std::size_t>(index));if (!(particle.life>0)) continue;particle.life=static_cast<float>(static_cast<double>(particle.life)-fields.delta);particle.x=static_cast<float>(static_cast<double>(particle.velocityX)*fields.scaledDelta+particle.x);
        const double wideY=static_cast<double>(particle.velocityY)*fields.scaledDelta+particle.y;particle.y=static_cast<float>(wideY);const auto px=Integer(particle.x),py=Integer(wideY); // y는FST 뒤 x87의 저장 전 값을 __ftol에 넘긴다.
        const float lowX=static_cast<float>(Add(px,-8)),lowY=static_cast<float>(Add(py,-8)),highX=static_cast<float>(Add(px,8)),highY=static_cast<float>(Add(py,8)); // 먼저 int32로 감긴 뒤 float 사각형에 넣는다.
        left=left<lowX ? left : lowX;top=top<lowY ? top : lowY;right=highX<right ? right : highX;if (bottom<=highY) bottom=highY;active=true;
        if (particle.life<=0) {
            const auto parent=Physical(static_cast<std::uint32_t>(fields.registration.parent));const auto parentX=Coordinate(parent,14),parentY=Coordinate(parent,18); // 원문은 메타데이터 호출 앞에 현재 부모 좌표를 저장한다.
            const auto end=Need(hooks_.soundId)(Sid{static_cast<std::uint16_t>(fields.registration.parent)},true);Need(hooks_.sound)(parentX,parentY,end);
        }
    }
    if (!active && fields.stopping) { Need(hooks_.kill)(0);return; }
    auto& rectangle=fields.registration;Invalidate(rectangle.left,rectangle.top,rectangle.right,rectangle.bottom);Invalidate(left,top,right,bottom);rectangle.left=left;rectangle.top=top;rectangle.right=right;rectangle.bottom=bottom;
}
void RawEvilRayFrame::Draw(const EvilRayFields& fields) const { // 원문 진단/clip 뒤에만 현재 사용 꼬리와 색을 읽는다.
    if (fields.total>static_cast<std::int32_t>(fields.particles.size())) Diagnose("totalParticles <= maxParticles",0x8c);
    Need(hooks_.clip)({0,0,639,479});
    for (std::int32_t index=0;index<fields.total;++index) { // 용량 밖은 앞선 진단/clip 뒤 명시 물리 경계로 드러난다.
        const auto& particle=fields.particles.at(static_cast<std::size_t>(index));if (!(particle.life>0)) continue;
        const float x=static_cast<float>(static_cast<double>(Integer(particle.x))-state_.cameraX);const auto y=Integer(static_cast<double>(Integer(particle.y))-state_.cameraY);const auto radius=std::max(1,Integer(((kLife-static_cast<double>(particle.life))*kRadiusScale)*kRadiusUnit)); // 원문 두 좌표 정수화와 두 double 곱의 순서다.
        if (colors_.size()<3) throw std::out_of_range("EvilRay 현재 공유3색 미연결");
        Need(hooks_.circle)({Integer(x),y,Add(radius,-2),colors_[2]});Need(hooks_.circle)({Integer(x),y,Add(radius,-1),colors_[1]});Need(hooks_.circle)({Integer(x),y,radius,colors_[0]});
    }
}
}
