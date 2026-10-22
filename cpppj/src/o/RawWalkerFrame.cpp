// 원본의 직각/회전 프레임과 0.08초 레인 보행 주기를 복원한다.
#include "o/RawWalkerFrame.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr float kAngleScale=0.0027777778450399637F,kTurningFrames=19.0F; //0050b534/0050e1a0이다.
constexpr double kFrameDelay=0.07999999821186066,kFtolLimit=9223372036854775808.0; //00509c60 및 CRT signed64 경계다.
// 부모의 정렬되지 않은 DWORD만 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t result{};
    // 주변 바이트는 포함하지 않는다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(raw[offset+index])<<(8*index);
    return result;
}
// 43fa50에 넘기기 전 float 저장과 +0.5/CRT 절삭의 하위 DWORD를 보존한다.
std::uint32_t TurningFrame(float angle) {
    const auto scaled=static_cast<float>((1.0-static_cast<double>(angle)*kAngleScale)*kTurningFrames);
    const auto rounded=static_cast<double>(scaled)+0.5;std::uint32_t result{};
    if (std::isfinite(rounded) && rounded>=-kFtolLimit && rounded<kFtolLimit)
        result=static_cast<std::uint32_t>(static_cast<std::int64_t>(rounded));
    return result+16U;
}
}
// 프레임이 같더라도 좌표가 달라지면 Unpop/Pop하며 비직각 레인 예약 분기는 이동도 건너뛴다.
void RunWalkerFrame(SidPool& pool,Sid parent,RawPathRuntime& runtime,double now,
    std::span<const RiftTypeFrames> frames,std::uint32_t windType,std::uint32_t rainType,
    std::uint32_t& windCounter,const WalkerFrameHooks& hooks,const std::shared_ptr<bool>& watch) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || !hooks.immobile || !hooks.halt || !hooks.step ||
        !hooks.groundStop || !hooks.arrive || !hooks.unpop || !hooks.pop || !watch)
        throw std::invalid_argument("Walker38 판본/필수 효과 누락");
    if (hooks.immobile() || !*watch || runtime.Finished() || hooks.halt() || !*watch) return;
    const auto sample=hooks.step();if (!*watch) return;
    if (sample.finished) { hooks.arrive();return; }
    if (hooks.groundStop(sample.position) || !*watch) return;
    const auto raw=pool.Slot(parent);const auto type=raw[10];const auto current=Read(raw,36);auto next=current;
    if (type==windType) {
        windCounter=(windCounter+1U)&3U;next=windCounter;
        if (sample.angle==90.0F) next+=12;
        else if (sample.angle==180.0F) next+=8;
        else if (sample.angle==270.0F) next+=4;
        else if (sample.angle!=0.0F) next=TurningFrame(sample.angle);
    } else if (type==rainType) {
        if (runtime.NextFrame()<=now) {
            std::uint8_t side{}; // 레인 A는 수직, B는 수평 이동에 사용한다.
            if (sample.angle==0.0F || sample.angle==180.0F) side='B';
            else if (sample.angle==90.0F || sample.angle==270.0F) side='A';
            else return;
            if (type>=frames.size()) throw std::out_of_range("Walker38 레인 TYPE 범위");
            const auto run=frames[type].Run(side);const auto frame=std::bit_cast<std::int32_t>(current);
            if (!run.count) throw std::out_of_range("Walker38 레인 보행 구간0");
            next=static_cast<std::uint32_t>(frame>=run.first && frame<run.first+run.count ? (frame-run.first+1)%run.count+run.first : run.first);
            runtime.SetNextFrame(now+kFrameDelay);
        }
    } else throw std::invalid_argument("Walker38 원본 부모 진단553");
    if (sample.position.x==std::bit_cast<float>(Read(raw,14)) && sample.position.y==std::bit_cast<float>(Read(raw,18)) && next==current) return;
    hooks.unpop(parent,0);if (!*watch) return;
    auto writable=pool.AllocatedBytes(parent);
    // 원본 부모+24의 네 바이트만 바꾼다.
    for (std::size_t index=0;index<4;++index) writable[36+index]=static_cast<std::uint8_t>(next>>(8*index));
    hooks.pop(parent,sample.position,0x40);
}
}
