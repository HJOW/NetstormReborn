// Bulf의 회전 E/F와 이동 A~D는 원본의 서로 다른 선택/반올림/나머지 계산을 유지한다.
#include "o/RawBulfFrame.h"
#include "o/RawPathAnimation.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>

namespace netstorm::o {
namespace {
constexpr float kAngleScale=0.0027777778450399637F; // 원본0050b534의 float 1/360이다.
constexpr double kFtolLimit=9223372036854775808.0; // 원본CRT signed64 불확정 반환의 경계다.
constexpr std::array<std::int32_t,4> kAfterTurning{3,9,17,30}; // E/F에서 각 A~D로 돌아올 때 사용하는 원본 중간 프레임이다.
// 비정렬 현재 DWORD를 읽으며 frame의 부호는 호출자가 결정한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{};
    // 지정한 네 바이트만 낮은 자리부터 조합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
// 원본 frame DWORD와 같은 네 바이트를 쓰며 다른 객체 필드는 보존한다.
void WriteFrame(SidPool& pool,Sid parent,std::int32_t frame) {
    auto raw=pool.AllocatedBytes(parent);const auto value=std::bit_cast<std::uint32_t>(frame);
    // frame은 패치판 +24의 DWORD다.
    for (std::size_t index=0;index<4;++index) raw[36+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// sound와 공간 효과 뒤에도 보관 좌표 사본 대신 현재 부모의 두 float를 읽는다.
RawPathPoint Position(const SidPool& pool,Sid parent) {
    const auto raw=pool.Slot(parent);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))};
}
}
// round가 아니라 원본43fa50의 +0.5 뒤 signed64 절삭/하위DWORD 감김이다.
std::int32_t BulfTurningFrame(const RiftTypeFrames& frames,float angle,std::uint32_t counterClockwise) {
    if (counterClockwise>1) throw std::invalid_argument("Bulf 회전 방향은0/1이어야 한다");
    const auto run=frames.Run(static_cast<std::uint8_t>(counterClockwise ? 'F' : 'E'));
    const auto scaled=static_cast<float>(static_cast<double>(angle)*kAngleScale*static_cast<double>(run.count-1));
    const auto rounded=static_cast<double>(scaled)+0.5;std::uint32_t integer{};
    if (std::isfinite(rounded) && rounded>=-kFtolLimit && rounded<kFtolLimit)
        integer=static_cast<std::uint32_t>(static_cast<std::int64_t>(rounded));
    return std::bit_cast<std::int32_t>(integer+static_cast<std::uint32_t>(run.first));
}
// Finished보다 먼저 immobile을 조회하고, 다음 프레임을 바꾸지 않는 경우 위치도 갱신하지 않는다.
void RunBulfFrame(SidPool& pool,Sid parent,RawPathRuntime& runtime,
    std::span<const RiftTypeFrames> frames,std::span<const SquidDisplayShape> shapes,
    const BulfFrameHooks& hooks,const std::shared_ptr<bool>& watch) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Bulf41은10.78 전용이다");
    if (!hooks.immobile || !hooks.halt || !hooks.step || !hooks.arrive || !hooks.unpop || !hooks.pop || !watch)
        throw std::invalid_argument("Bulf41 프레임의 필수 연결 누락");
    if (hooks.immobile()) return;
    const auto previous=runtime.Angle();
    if (runtime.Finished() || hooks.halt() || !*watch) return;
    const auto sample=hooks.step();if (!*watch) return;
    if (sample.finished) { hooks.arrive();return; }
    const auto raw=pool.Slot(parent);const auto type=raw[10];
    const auto current=std::bit_cast<std::int32_t>(Read(raw,36));
    if (type>=frames.size() || type>=shapes.size() || current<0 || static_cast<std::size_t>(current)>=shapes[type].frames.size())
        throw std::out_of_range("Bulf41 현재 TYPE/SHP 프레임 범위");
    const auto position=Position(pool,parent);const auto& shape=shapes[type].frames[current];
    const auto footX=static_cast<float>(static_cast<double>(position.x)+shape.walkStepX);
    const auto footY=static_cast<float>(static_cast<double>(position.y)+shape.walkStepY);
    const auto direction=runtime.Direction(runtime.Current())/2;auto next=current;
    if (previous!=sample.angle) {
        const auto counterClockwise=sample.angle<previous || static_cast<double>(sample.angle)-previous>180.0;
        next=BulfTurningFrame(frames[type],sample.angle,counterClockwise ? 1U : 0U);
        if (next>=static_cast<std::int32_t>(frames[type].Codes().size())) throw std::out_of_range("Bulf 회전 프레임 원본 진단5bc");
    } else if (std::fmod(static_cast<double>(sample.angle),90.0)==0) {
        const double dx=static_cast<double>(sample.position.x)-position.x,dy=static_cast<double>(sample.position.y)-position.y;
        const double fx=static_cast<double>(footX)-position.x,fy=static_cast<double>(footY)-position.y;
        if (fx*fx+fy*fy<=dx*dx+dy*dy) {
            if (direction<0 || direction>=4) throw std::out_of_range("Bulf41 원본 A~D 방향 범위");
            const auto side=RawPathAnimation::Direction(frames[type],current);
            if (side==4 || side==5) next=kAfterTurning[direction];
            const auto run=frames[type].Run(static_cast<std::uint8_t>('A'+direction));
            if (!run.count) throw std::out_of_range("Bulf41 보행 구간0");
            next=(next+1)%run.count+run.first;
            if (next>=static_cast<std::int32_t>(frames[type].Codes().size())) throw std::out_of_range("Bulf 보행 프레임 원본 진단5cd");
        }
    }
    if (next==current) return;
    hooks.unpop(parent,0);if (!*watch) return;
    WriteFrame(pool,parent,next);hooks.pop(parent,sample.position,0x40);if (!*watch) return;
    if ((static_cast<std::uint32_t>(next)+1U)%4U==0) {
        if (!hooks.randomInt || !hooks.sound) throw std::logic_error("Bulf41 공용 난수/발소리 미연결");
        const auto variant=hooks.randomInt(1000,6000)/1000;const auto point=Position(pool,parent);
        hooks.sound(point.x,point.y,"bulfWalk"+std::to_string(variant)+".wav");
    }
}
}
