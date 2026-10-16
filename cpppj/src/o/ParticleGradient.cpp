// 원본의 세 차례 CRT 정렬과 투영/원통 선택을 다른 RGB 보간으로 바꾸지 않는다.
#include "o/ParticleGradient.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 RGB 정규화 double과 최근접/선택 초기 거리, 0 임계값 대체, 마지막 선택 여유다.
constexpr double kRgbUnit=std::bit_cast<double>(std::uint64_t{0x3f70101010101010}),
    kNearestLimit=100000.0,kSelectionLimit=10000.0,kFallbackThreshold=.05,kSelectionExtra=.25;
// 실제00509578/70/68/60의 double 비트다. 두 색 띠가 시작 주황색을 공유한다.
constexpr std::array<double,3> kFieryStart{1.0,std::bit_cast<double>(std::uint64_t{0x3fe8787878787878}),std::bit_cast<double>(std::uint64_t{0x3f98181818181818})},
    kFieryDark{std::bit_cast<double>(std::uint64_t{0x3fe7373737373737}),0.0,0.0},
    kFieryLight{1.0,1.0,std::bit_cast<double>(std::uint64_t{0x3fe5b5b5b5b5b5b6})};
// 10.78 CRT의 작은 구간 크기와 구간 스택 수다.
constexpr std::ptrdiff_t kSmall=8;
constexpr std::size_t kRanges=30;
// 포함되는 양 끝 인덱스로 정렬 구간을 표현한다.
struct Range { std::ptrdiff_t low{},high{}; }; // low/high는 같은 배열의 첫/마지막 원소다.
// 세 번의 qsort가 사용하는 서로 다른 비교 키다.
enum class SortKey { Distance, Cylinder, Selected };
// 원본은 차이를 double로 구한 뒤 부호를 비교한다. NaN/동률은0을 반환한다.
int Sign(double value) { return value<0 ? -1 : value>0 ? 1 : 0; }
// PATCH0043a0c0/0043a300/0043a3d0: 동률 처리와 선택되지 않은 항목의0 비교를 보존한다.
int Compare(const ParticleGradientEntry& a,const ParticleGradientEntry& b,SortKey key,double threshold) {
    if (key==SortKey::Distance) return Sign(a.distance-b.distance);
    if (key==SortKey::Cylinder) {
        if (a.distance>=threshold) return b.distance<threshold ? 1 : 0;
        if (b.distance>=threshold) return -1;
        return Sign(a.position-b.position);
    }
    if (!a.selected) return b.selected ? 1 : 0;
    if (!b.selected) return -1;
    return Sign(a.position-b.position);
}
// PATCH004e6010/004e5fa0의 피벗/교환 순서다. 80바이트 항목의 색/표식도 함께 교환한다.
void Sort(std::span<ParticleGradientEntry> entries,SortKey key,double threshold) {
    std::array<Range,kRanges> stack{};
    std::size_t used=0;
    Range range{0,static_cast<std::ptrdiff_t>(entries.size())-1};
    // 주소 대신 현재 인덱스의 원소를 비교한다.
    const auto compare=[&](std::ptrdiff_t a,std::ptrdiff_t b) { return Compare(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)],key,threshold); };
    // 원본 byte swap과 같은 항목 전체 교환이다.
    const auto swap=[&](std::ptrdiff_t a,std::ptrdiff_t b) { if (a!=b) std::swap(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)]); };
    // 큰 구간을 고정 스택에 보관하며 다음에 작은 구간을 처리한다.
    const auto push=[&](Range next) { if (used==stack.size()) throw std::logic_error("입자 색 정렬 스택 넘침");stack[used++]=next; };
    // 현재 구간과 보관했던 구간이 모두 끝날 때까지 원본 분기 순서를 따른다.
    for (;;) {
        if (range.high-range.low+1<=kSmall) {
            // small sort는 최대 항목을 마지막으로 보내는 선택 정렬이다.
            while (range.low<range.high) {
                auto maximum=range.low;
                // 엄격히 큰 항목만 최대 위치로 바꾼다.
                for (auto current=range.low+1;current<=range.high;++current)
                    if (compare(current,maximum)>0) maximum=current;
                swap(maximum,range.high);--range.high;
            }
        } else {
            auto middle=range.low+(range.high-range.low+1)/2;
            if (compare(range.low,middle)>0) swap(range.low,middle);
            if (compare(range.low,range.high)>0) swap(range.low,range.high);
            if (compare(middle,range.high)>0) swap(middle,range.high);
            auto left=range.low,right=range.high;
            // 실제 CRT처럼 피벗의 자기 비교를 건너뛰고 교차할 때까지 분할한다.
            for (;;) {
                if (left<middle) {
                    // 피벗 앞의 작은/동률 항목을 지난다.
                    do { ++left; } while (left<middle && compare(left,middle)<=0);
                }
                if (left>=middle) {
                    // 피벗 뒤의 작은/동률 항목을 끝까지 검사한다.
                    do { ++left; } while (left<=range.high && compare(left,middle)<=0);
                }
                // 오른쪽은 엄격히 큰 항목만 지난다.
                do { --right; } while (right>middle && compare(right,middle)>0);
                if (left>right) break;
                swap(left,right);if (middle==right) middle=left;
            }
            ++right;
            if (right>middle) {
                // 피벗 오른쪽의 실제0 비교 항목도 제거한다.
                do { --right; } while (right>middle && compare(right,middle)==0);
            }
            if (right<=middle) {
                // 피벗을 건너뛰고 왼쪽 끝까지 동률 구간을 제거한다.
                do { --right; } while (right>range.low && compare(right,middle)==0);
            }
            if (right-range.low<range.high-left) {
                if (left<range.high) push({left,range.high});range.high=right;
                if (range.low<range.high) continue;
            } else {
                if (range.low<right) push({range.low,right});range.low=left;
                if (range.low<range.high) continue;
            }
        }
        if (!used) return;
        range=stack[--used];
    }
}
// 원본 최근접 탐색은 B/G/R의 제곱을 그 순서로 더한다.
double Distance(std::array<double,3> a,std::array<double,3> b) {
    const auto red=a[0]-b[0],green=a[1]-b[1],blue=a[2]-b[2];
    const auto first=blue*blue+green*green;return first+red*red;
}
}
// 정규화→가장 가까운 시작/끝→평행 이동→축 투영→원본 정렬/선택 순서다.
ParticleGradient BuildParticleGradient(std::span<const std::uint32_t,kParticlePaletteColors> palette,
    std::array<double,3> from,std::array<double,3> to,std::int32_t colors,double width) {
    if (colors<2 || colors>256 || !std::isfinite(width) || width<=0) throw std::invalid_argument("입자 색 띠 개수/폭 오류");
    // 대상 RGB는 실제 함수와 같이 최근접 색을 고르며 보드/색 범위로 임의 clamp하지 않는다.
    for (std::size_t channel=0;channel<3;++channel)
        if (!std::isfinite(from[channel]) || !std::isfinite(to[channel])) throw std::invalid_argument("입자 대상 RGB 비유한 값");
    const double wanted=static_cast<double>(colors)*width;
    if (!std::isfinite(wanted) || wanted<1 || wanted>=255.5) throw std::out_of_range("입자 색 원통 후보 수 범위 오류");
    const auto rounded=static_cast<std::int32_t>(static_cast<double>(static_cast<float>(wanted))+.5);
    const auto cylinder=static_cast<std::int32_t>(wanted);
    if (rounded>=256) throw std::out_of_range("입자 색 임계 항목 범위 오류");
    ParticleGradient result;result.count=colors;result.from=result.to=-1;
    double bestFrom=kNearestLimit,bestTo=kNearestLimit;
    // 원래 팔레트 번호와 RGB double을 원본 순서로 만든다. 동일 거리에서는 먼저 나온 색을 유지한다.
    for (std::size_t index=0;index<result.entries.size();++index) {
        auto& entry=result.entries[index];entry.index=static_cast<std::uint32_t>(index);
        // 플래그 BYTE는 RGB 계산에 참여하지 않는다.
        for (std::size_t channel=0;channel<3;++channel) entry.translated[channel]=static_cast<double>((palette[index]>>(8*channel))&255U)*kRgbUnit;
        const auto nearFrom=Distance(entry.translated,from),nearTo=Distance(entry.translated,to);
        if (nearFrom<bestFrom) { bestFrom=nearFrom;result.from=static_cast<std::int32_t>(index); }
        if (nearTo<bestTo) { bestTo=nearTo;result.to=static_cast<std::int32_t>(index); }
    }
    if (result.from<0 || result.to<0) throw std::out_of_range("입자 시작/끝 최근접 색 없음");
    const auto origin=result.entries[static_cast<std::size_t>(result.from)].translated;
    // 각 RGB 차이를 double로 저장한 뒤에만 축 벡터를 읽는다.
    for (auto& entry:result.entries) {
        // RGB 세 채널을 원래 팔레트 순서로 뺀다.
        for (std::size_t channel=0;channel<3;++channel) entry.translated[channel]-=origin[channel];
    }
    const auto axis=result.entries[static_cast<std::size_t>(result.to)].translated;
    const double firstNorm=axis[2]*axis[2]+axis[1]*axis[1],norm=firstNorm+axis[0]*axis[0];
    if (!(norm>0)) throw std::logic_error("입자 색 띠 영길이 축 미복원");
    // PATCH0043a660의 R/B/G 내적과 B/G/R 제곱 거리 저장 순서다.
    for (auto& entry:result.entries) {
        const auto& color=entry.translated;
        const double firstDot=axis[0]*color[0]+axis[2]*color[2],dot=firstDot+axis[1]*color[1];
        entry.position=dot/norm;
        const std::array<double,3> projection{entry.position*axis[0],entry.position*axis[1],entry.position*axis[2]};
        entry.distance=Distance(color,projection);
    }
    Sort(result.entries,SortKey::Distance,0);
    result.threshold=result.entries[static_cast<std::size_t>(rounded)].distance;
    if (result.threshold==0) result.threshold=kFallbackThreshold;
    Sort(result.entries,SortKey::Cylinder,result.threshold);
    const double step=1.0/(colors-1),stop=step*kSelectionExtra+1.0;
    // 같은 색이 여러 위치에 가장 가까울 수 있다. 선택 표식을 중복 가산하지 않는다.
    for (double position=0;position<stop;position+=step) {
        double best=kSelectionLimit;std::int32_t chosen=-1;
        // 동일 오차이면 앞 항목을 유지하는 원본 엄격한 비교다.
        for (std::int32_t index=0;index<cylinder;++index) {
            const auto difference=std::abs(position-result.entries[static_cast<std::size_t>(index)].position);
            if (difference<best) { best=difference;chosen=index; }
        }
        if (chosen<0) throw std::logic_error("입자 색 띠 후보 선택 실패");
        result.entries[static_cast<std::size_t>(chosen)].selected=1;
    }
    // 실제 선택 수는 원통 후보 앞부분에서만 센다.
    for (std::int32_t index=0;index<cylinder;++index) result.selected+=result.entries[static_cast<std::size_t>(index)].selected!=0;
    Sort(result.entries,SortKey::Selected,result.threshold);
    result.entries[static_cast<std::size_t>(result.selected-1)].index=static_cast<std::uint32_t>(result.to);
    return result;
}
// 실제 Particle 팔레트 병합은 선택 수가 아닌 각 색 띠의 요청 수를 복사한다.
std::vector<std::uint32_t> MergeParticleGradients(std::span<const ParticleGradient> gradients) {
    if (gradients.empty() || gradients.size()>3) throw std::invalid_argument("입자 색 띠 병합 개수 오류");
    std::vector<std::uint32_t> result;
    // null 구멍이 없는 연속 색 띠를 원본 순서대로 병합한다.
    for (const auto& gradient:gradients) {
        if (gradient.count<0 || gradient.count>256) throw std::out_of_range("입자 색 띠 병합 길이 오류");
        // 요청 수가 실제 선택 수보다 커도 나머지 정렬 항목의 번호를 그대로 읽는다.
        for (std::int32_t index=0;index<gradient.count;++index) result.push_back(gradient.entries[static_cast<std::size_t>(index)].index);
    }
    return result;
}
// 실제 생성자는 어두운 빨강→주황8색, 주황→밝은색3색을 순서대로 요청한다.
std::vector<std::uint32_t> BuildFieryTailPalette(std::span<const std::uint32_t,kParticlePaletteColors> palette) {
    const std::array<ParticleGradient,2> gradients{BuildParticleGradient(palette,kFieryDark,kFieryStart,8),
        BuildParticleGradient(palette,kFieryStart,kFieryLight,3)};
    return MergeParticleGradients(gradients);
}
// 실제 빔의 두 색 띠는0.5가 아닌 PE binary64의128/255 회색을 사용한다.
std::vector<std::uint32_t> BuildLightningPalette(std::span<const std::uint32_t,kParticlePaletteColors> palette) {
    const std::array<double,3> blue{0,0,1},white{1,1,1},gray{128.0/255.0,128.0/255.0,128.0/255.0}; // 실제 RGB 끝점이다.
    const std::array<ParticleGradient,2> gradients{BuildParticleGradient(palette,blue,white,8),BuildParticleGradient(palette,white,gray,3)}; // 원문 순서의8+3색이다.
    return MergeParticleGradients(gradients);
}
}
