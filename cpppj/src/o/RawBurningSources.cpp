// 원본 연소점 추가/제거의 행 표식과 입자 부족 시 순서를 보존한다.
#include "o/RawBurningSources.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본의 연소점 행 폭·시도 상한·투명색과 고정 source 수명이다.
constexpr std::int32_t kRowWidth=5,kAttempts=300,kFallback=7;
constexpr std::uint8_t kTransparent=0xff;
constexpr float kSourceLife=9999;
// 원본 비정렬 HP의 두 저장폭을 안전하게 읽는다.
std::int32_t SourceHitPoints(std::span<const std::uint8_t> raw,bool patch) {
    std::uint32_t value=0;
    // 패치는 DWORD, CD는 signed WORD다.
    for (std::size_t index=0;index<(patch ? 4U : 2U);++index) value|=static_cast<std::uint32_t>(raw[26+index])<<(index*8);
    return patch ? std::bit_cast<std::int32_t>(value) : static_cast<std::int16_t>(value);
}
}
// 기존 Append/난수와 현재 타입 저장소를 사용하며 다른 풀의 시각 효과를 합치지 않는다.
RawBurningSources::RawBurningSources(const SidPool& pool,std::span<const RiftTypeRecord> types,
    const RawBurningVisual& visual,GameRandom& random,const BurningSourcesState& state):pool_(pool),types_(types),visual_(visual),random_(random),state_(state) {
    if (&visual.Pool()!=&pool_) throw std::invalid_argument("Burning sources/Append SID 풀 불일치");
}
// 게임 정수난수의 실제 상한은 제외이며 CRT rand를 사용하지 않는다.
std::int32_t RawBurningSources::Random(std::int32_t minimum,std::int32_t maximum) const {
    if (maximum<=minimum) throw std::invalid_argument("Burning sources 원본 난수 나눗셈 범위 오류");
    return minimum+static_cast<std::int32_t>(random_.Next(static_cast<std::uint32_t>(maximum-minimum)));
}
// 현재 캡처 버퍼의 정상 인덱스만 허용하며 색 의미를 palette RGB로 바꾸지 않는다.
bool RawBurningSources::Opaque(const BurningVisualFields& fields,std::int32_t x,std::int32_t y) const {
    if (x<0 || y<0 || x>=fields.image.width || y>=fields.image.height) throw std::out_of_range("Burning sources 캡처 픽셀 범위 오류");
    const auto index=static_cast<std::size_t>(y)*fields.image.width+x;
    if (index>=fields.image.pixels.size()) throw std::out_of_range("Burning sources 캡처 버퍼 미연결");
    return fields.image.pixels[index]!=kTransparent;
}
// 비유한/범위 밖 좌표는 원본 ftol의 미정의 후속 포인터 접근 전에 거부한다.
std::int32_t RawBurningSources::Row(const BurningParticle& particle) const {
    if (!std::isfinite(particle.x) || !std::isfinite(particle.y) || particle.x<-2147483648.0 || particle.x>=2147483648.0 || particle.y<-2147483648.0 || particle.y>=2147483648.0) throw std::invalid_argument("Burning sources 연소점 좌표 오류");
    const auto sum=static_cast<std::int64_t>(static_cast<std::int32_t>(particle.x))+static_cast<std::int32_t>(particle.y);
    if (sum<std::numeric_limits<std::int32_t>::min() || sum>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("Burning sources 행 덧셈 범위 오류");
    return static_cast<std::int32_t>(sum)/kRowWidth;
}
// 원본의 선택 개수는 실제 추가 성공 수와 다르며 제거 후에도 selected를 감소시키지 않는다.
void RawBurningSources::Select(BurningVisualFields& fields) const {
    if (!fields.victim.value || fields.victim.value==65535) return;
    const auto raw=pool_.Slot(fields.victim);const auto type=raw[10];const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (type>=types_.size()) throw std::out_of_range("Burning sources 현재 타입 범위 오류");
    const auto& record=types_[type];const bool hasHp=(record.flags2&0x400000U) ? (raw[patch ? 40U : 35U]&1)!=0 : (record.flags1&TypeFlag1::kHasHitPoints)!=0;
    if (!hasHp) { if (patch) throw std::logic_error("Burning sources hasHitPoints 원본 assert");return; }
    const auto maximum=state_.WeakenedMana() && type==state_.manaType ? record.maxHitPoints/4 : record.maxHitPoints;
    const auto divisor=maximum-maximum/2;
    if (divisor<=0) throw std::invalid_argument("Burning sources 최대HP 나눗셈 범위 오류");
    const auto numerator=static_cast<std::int64_t>(SourceHitPoints(raw,patch))*5;
    if (numerator<std::numeric_limits<std::int32_t>::min() || numerator>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("Burning sources HP 곱셈 범위 오류");
    if (fields.image.width<0 || fields.image.height<0 || static_cast<std::int64_t>(fields.image.width)+fields.image.height>std::numeric_limits<std::int32_t>::max()) throw std::invalid_argument("Burning sources 이미지 크기 오류");
    auto wanted=std::max(1,std::min(6-static_cast<std::int32_t>(numerator)/divisor,(fields.image.width+fields.image.height)/kRowWidth));
    if (record.flags2&0x400U) wanted=1;
    if (fields.selected==wanted) return;
    if (fields.image.height<3) { fields.selected=wanted;return; }
    if (fields.total>fields.capacity || fields.particles.size()!=fields.capacity) throw std::invalid_argument("Burning sources 현재 입자 배열 오류");
    const auto rowCount=(fields.image.width+fields.image.height)/kRowWidth;
    if (fields.rows.size()<static_cast<std::size_t>(rowCount)) throw std::out_of_range("Burning sources 행 표식 미연결");
    if (fields.selected<wanted) {
        const auto xMaximum=std::min(fields.image.width-1,std::max(8,fields.image.width-7));
        wanted=std::min(wanted,(xMaximum+fields.image.height-3)/kRowWidth);
        std::int32_t attempts=kAttempts;
        // 원본은 300번째 시도 뒤 목표에 닿아도 남은 횟수가0이면 폴백을 실행한다.
        while (fields.selected<wanted) {
            const auto before=attempts--;
            if (!before) break;
            const auto x=Random(0,xMaximum),y=Random(2,fields.image.height-1),row=(x+y)/kRowWidth;
            if (patch && !Opaque(fields,x,y+1)) continue; // 10.78은 아래 픽셀이 투명하면 rowTaken에 도달하지 않는다. 작은 캡처의 끝 후보도 이 순서로 단락한다.
            if (row<0 || static_cast<std::size_t>(row)>=fields.rows.size()) throw std::out_of_range("Burning sources 새 행 인덱스 오류");
            if (fields.rows[row] || (!patch && !Opaque(fields,x,y+1))) continue; // CD는 행 표식을 먼저 읽고 비어 있을 때만 픽셀을 읽는다. 실제 선택된 잘못된 행의 보호는 유지한다.
            const auto count=Random(6,10);std::int32_t accepted=0;
            // 같은 대각선의 각 점은 Append 실패와 무관하게 rowTaken을 표식한다.
            for (std::int32_t index=0;index<count;++index) {
                const auto px=x+index/2,py=y-index/2;
                if (px<fields.image.width-1 && py>=0 && Opaque(fields,px,py)) {
                    static_cast<void>(visual_.Append(fields,{static_cast<float>(px),static_cast<float>(py),0,0,kSourceLife,true}));++accepted;fields.rows[row]=1;
                }
            }
            if (accepted>3) ++fields.selected;
        }
        if (attempts<1) {
            auto left=kFallback;
            // 폴백은 마지막 행을 제외한 y/x 순서에서 최초7개 불투명 점을 사용한다.
            for (std::int32_t y=0;y<fields.image.height-1 && left;++y) {
                // 원본은 Append가 실패해도 불투명 점 하나를 사용한 것으로 센다.
                for (std::int32_t x=0;x<fields.image.width && left;++x) if (Opaque(fields,x,y)) { static_cast<void>(visual_.Append(fields,{static_cast<float>(x),static_cast<float>(y),0,0,kSourceLife,true}));--left; }
            }
        }
        return;
    }
    auto first=fields.total;
    // 선택 개수가 줄면 실제 life와 무관하게 첫 source 포인터 표식을 찾는다.
    for (std::uint32_t index=0;index<fields.total;++index) if (fields.particles[index].source) { first=index;break; }
    if (first==fields.total) return;
    const auto row=Row(fields.particles[first]);
    if (row<0 || static_cast<std::size_t>(row)>=fields.rows.size()) throw std::out_of_range("Burning sources 제거 행 인덱스 오류");
    if (fields.rows[row]!=1) throw std::logic_error("Burning sources rowTaken 원본 assert");
    fields.rows[row]=0;
    // 같은 source 행만 포인터 표식/life를0으로 바꾸고 total/selected는 보존한다.
    for (std::uint32_t index=0;index<fields.total;++index) { auto& particle=fields.particles[index];if (particle.source && Row(particle)==row) { particle.source=false;particle.life=0; } }
}
// 후속 실제 Step/현재 피해 월드 연결의 풀 동일성 검사다.
const SidPool& RawBurningSources::Pool() const { return pool_; }
}
