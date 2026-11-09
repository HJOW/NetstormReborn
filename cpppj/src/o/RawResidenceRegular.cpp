// 실제 거주 구역의 현재 조회와 signed 산술/난수 소비 순서를 유지한다.
#include "o/RawResidenceRegular.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace netstorm::o { // 새 World/Session이나 원본 전역 사본을 만들지 않는다.
namespace { // 지정 물리 폭과 선택 장치 사본만 내부에서 처리한다.
template<class Function> Function Need(const Function& call) { if (!call) throw std::logic_error("거주 구역의 선택된 실제 본문 미연결");return call; } // 호출 중 연결 교체에도 callable 수명을 유지한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(raw[offset+byte])<<(byte*8);return value; } // 비정렬 DWORD 네 바이트만 조합한다.
std::int32_t Frame(const SidPool& pool,Sid sid) { return std::bit_cast<std::int32_t>(Read(pool.Slot(sid),36)); } // callback 뒤 현재 DWORD를 다시 읽는다.
std::int32_t Add(std::int32_t value,std::int32_t delta) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+std::bit_cast<std::uint32_t>(delta)); } // signed 덧셈의 원문 감김이다.
std::int32_t Truncate(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) throw std::out_of_range("거주 구역 좌표 CRT 물리 범위");return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); } // x87 ftol의64비트 절삭 뒤 low DWORD다.
void SetEven(const SidPool& pool,Sid sid,const ResidenceRegularHooks& hooks) { const auto frame=Frame(pool,sid);Need(hooks.frame)(sid,Add((frame/2)*2,pool.Slot(sid)[12]&15U),0); } // 음수 frame도0쪽 나눗셈 후 현재 주민 nibble을 더한다.
}
float RunResidenceRegular(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,const SquidPostPopList& temples,Sid sid,std::uint32_t event,std::uint32_t,float payload,const ResidenceRegularHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("거주 구역49a4e0은 PATCH 본문입니다");
    if (event==0) {
        const auto raw=pool.Slot(sid);const auto low=raw[12];const_cast<std::uint8_t*>(raw.data())[12]=static_cast<std::uint8_t>((low&0xf0U)|((low&15U) ? 0U:1U));
        Need(hooks.notify)(sid);SetEven(pool,sid,hooks);return static_cast<float>(static_cast<double>(Need(hooks.random)().Next(65536))*(20.0/65536.0)+15.0);
    }
    if (event==1) {
        const auto raw=pool.Slot(sid);const double x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));
        const auto range=[&hooks](std::int32_t low,std::int32_t high) { return Add(low,std::bit_cast<std::int32_t>(Need(hooks.random)().Next(static_cast<std::uint32_t>(high)-static_cast<std::uint32_t>(low)))); }; // 두 원문 정수 난수 범위를 같은 seed에서 차례로 소비한다.
        auto searchX=range(Truncate(x-48.0),Truncate(x+32.0));auto searchY=range(Truncate(y-32.0),Truncate(y+32.0));searchX=std::clamp(searchX,10,246);searchY=std::clamp(searchY,10,246);
        const auto bottom=Truncate(std::min(static_cast<double>(static_cast<float>(y+32.0)),246.0));RawSquidFinder finder(pool,hash,types);auto candidate=finder.Begin({searchX-1,searchY-3,searchX+1,bottom});bool changed{};std::int32_t checked{};
        while (candidate.value && !changed) { // 첫 변경 뒤에도 실제 Next를 한 번 수행한다.
            ++checked;if (pool.Slot(candidate)[10]==Need(hooks.decorationType)()) {
                const auto flags=[&] { const auto current=pool.Slot(candidate);const auto frame=Frame(pool,candidate);if (current[10]>=frames.size()) throw std::out_of_range("거주 구역 현재 장식 TYPE 범위");const auto codes=frames[current[10]].Codes();if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("거주 구역 현재 장식 frame 범위");return codes[static_cast<std::size_t>(frame)].flags; }; // 매 원문4ad060의 현재 TYPE/frame을 재조회한다.
                if (flags()&8U) { Need(hooks.frame)(candidate,Add(Frame(pool,candidate),1),0);changed=true; }
                else if (flags()&16U) { Need(hooks.frame)(candidate,Add(Frame(pool,candidate),-1),0);changed=true; }
            }candidate=finder.Next();
        }Need(hooks.recordScan)(checked);return 1.0F;
    }
    if (event==2) Need(hooks.dude)(sid);
    if (event==3) {
        const auto raw=pool.Slot(sid);const auto support=Need(hooks.surface)(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));if (support.value) Need(hooks.owner)(sid,pool.Slot(support)[34]);
        std::int32_t theme{};const auto owner=pool.Slot(sid)[34];
        for (const auto value:temples.Items()) { // 원문이 캡처한 목록/count와 owner에서 첫 일치 신전만 선택한다.
            const Sid temple{static_cast<std::uint16_t>(value)};if (pool.Slot(temple)[34]!=owner) continue;const auto element=Need(hooks.element)(temple);const auto enabled=Need(hooks.themeMode)();if (enabled) theme=element==2 ? 1:element==0 ? 2:element==1 ? 3:0;break;
        }Need(hooks.frame)(sid,Add(std::bit_cast<std::int32_t>(Read(pool.Slot(sid),42)),theme*6),0);SetEven(pool,sid,hooks);return 0.1F;
    }return payload;
}
void RunResidenceDudeSpawn(const SidPool& pool,const bool& fort,const SquidPostPopList& population,const SquidPostPopList& residences,Sid source,const ResidenceDudeHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("거주 구역450a90은 PATCH 본문입니다");
    if (!fort) return;const auto dudeCount=std::bit_cast<std::int32_t>(population.Count());const auto residenceCount=std::bit_cast<std::int32_t>(residences.Count());if (dudeCount>=residenceCount) return; // 원문처럼 Dude count를 먼저 읽고 거주 구역 count를 다음에 읽는다.
    if (!source.value) Need(hooks.assertion)();if (pool.Slot(source)[11]&1U) return;
    const auto point=Need(hooks.entrance)(source);const auto born=Need(hooks.spawn)(point[0],point[1],pool.Slot(source)[34]);if (born.value && !(pool.Slot(born)[11]&3U)) Need(hooks.task)(born,0x5102,0.0F);
}
}
