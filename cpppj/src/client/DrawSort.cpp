// 004e6010/004e5fa0와 CD 004f1c00/004f1d60/004f1dc0의 실제 분할·교환 순서를 유지한다.
#include "client/DrawSort.h"
#include "client/Renderer.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본은 8항목 이하를 shortsort로 처리하고 큰 쪽 구간을 30칸 스택에 보관한다.
constexpr std::ptrdiff_t kSmall=8;
constexpr std::size_t kStack=30;
// 원본의 signed 바이트 거리 비교가 넘치지 않는 16바이트 항목 수 상한이다.
constexpr std::size_t kMaxEntries=static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())/16;
// 두 포인터 대신 안전한 항목 인덱스로 원본의 양 끝 포함 구간을 표현한다.
struct Range { /* low: 현재 구간의 시작 인덱스. */ /* high: 현재 구간의 끝 인덱스. */ std::ptrdiff_t low{},high{}; };
// 원본 비교 함수의 signed WORD와 float 차이를 사용하며 동률을 0으로 바꾸지 않는다.
int Compare(std::span<o::RenderCollectionEntry> entries,std::ptrdiff_t a,std::ptrdiff_t b) {
    const auto& x=entries[static_cast<std::size_t>(a)];const auto& y=entries[static_cast<std::size_t>(b)];
    return CompareDrawOrder({x.x,x.y,x.depth},{y.x,y.y,y.depth});
}
// 항목 전체를 교환한다. 종류·tail·객체 토큰도 원본의 byte swap과 같은 위치로 옮긴다.
void Swap(std::span<o::RenderCollectionEntry> entries,std::ptrdiff_t a,std::ptrdiff_t b) {
    if (a!=b) std::swap(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)]);
}
// 두 판본의 작은 구간 정렬은 최대 항목을 뒤로 보내는 같은 선택 정렬이다.
void SmallSort(std::span<o::RenderCollectionEntry> entries,Range range) {
    // 마지막 칸을 확정할 때마다 남은 구간의 오른쪽 끝을 줄인다.
    while (range.low<range.high) {
        auto maximum=range.low;
        // 원본은 비교 결과가 엄격히 양수일 때만 최대 위치를 바꾼다.
        for (auto current=range.low+1;current<=range.high;++current)
            if (Compare(entries,current,maximum)>0) maximum=current;
        Swap(entries,maximum,range.high);--range.high;
    }
}
// 더 큰 구간을 보관하고 작은 구간부터 처리하여 원본 고정 스택의 깊이를 유지한다.
void Push(std::array<Range,kStack>& stack,std::size_t& used,Range range) {
    if (used==stack.size()) throw std::logic_error("표시 정렬 구간 스택 넘침");stack[used++]=range;
}
// 패치의 세 지점 중앙값·피벗 위치 갱신·동일 키 뒤쪽 검사까지 원본 분기 순서다.
void PatchSort(std::span<o::RenderCollectionEntry> entries) {
    std::array<Range,kStack> stack{};std::size_t used=0;Range range{0,static_cast<std::ptrdiff_t>(entries.size())-1};
    // 새 구간 또는 저장했던 구간이 남아 있는 동안 실제 CRT와 같은 순서로 처리한다.
    for (;;) {
        if (range.high-range.low+1<=kSmall) SmallSort(entries,range);
        else {
            auto middle=range.low+(range.high-range.low+1)/2;
            if (Compare(entries,range.low,middle)>0) Swap(entries,range.low,middle);
            if (Compare(entries,range.low,range.high)>0) Swap(entries,range.low,range.high);
            if (Compare(entries,middle,range.high)>0) Swap(entries,middle,range.high);
            auto left=range.low,right=range.high;
            // 피벗의 자기 비교를 생략하며 양쪽 커서를 교차시킨다.
            for (;;) {
                if (left<middle) {
                    // 피벗 앞의 작은/동률 항목을 지난다. 피벗 칸은 비교하지 않는다.
                    do { ++left; } while (left<middle && Compare(entries,left,middle)<=0);
                }
                if (left>=middle) {
                    // 피벗을 지난 커서는 마지막 항목까지 검사하며 그 다음 칸에서는 비교하지 않는다.
                    do { ++left; } while (left<=range.high && Compare(entries,left,middle)<=0);
                }
                // 오른쪽은 피벗에서 멈추며 엄격한 양수만 건너뛴다.
                do { --right; } while (right>middle && Compare(entries,right,middle)>0);
                if (left>right) break;
                Swap(entries,left,right);if (middle==right) middle=left;
            }
            ++right;
            if (right>middle) {
                // 피벗 오른쪽에서 실제 0 비교 항목만 더 제거한다. 원본 표시 동률 비교는 1이다.
                do { --right; } while (right>middle && Compare(entries,right,middle)==0);
            }
            if (right<=middle) {
                // 피벗 칸은 생략하고 왼쪽 구간 끝을 정한다.
                do { --right; } while (right>range.low && Compare(entries,right,middle)==0);
            }
            if (right-range.low<range.high-left) {
                if (left<range.high) Push(stack,used,{left,range.high});range.high=right;
                if (range.low<range.high) continue;
            } else {
                if (range.low<right) Push(stack,used,{range.low,right});range.low=left;
                if (range.low<range.high) continue;
            }
        }
        if (!used) return;range=stack[--used];
    }
}
// CD는 가운데 항목을 첫 칸으로 옮긴 뒤 첫 칸을 피벗으로 유지하는 이전 CRT 알고리즘이다.
void CdSort(std::span<o::RenderCollectionEntry> entries) {
    std::array<Range,kStack> stack{};std::size_t used=0;Range range{0,static_cast<std::ptrdiff_t>(entries.size())-1};
    // 작은 쪽 우선 처리와 저장 구간 복귀 순서는 CD의 주소 거리 비교를 보존한다.
    for (;;) {
        if (range.high-range.low+1<=kSmall) SmallSort(entries,range);
        else {
            Swap(entries,range.low+(range.high-range.low+1)/2,range.low);auto left=range.low,right=range.high+1;
            // 왼쪽은 <=0, 오른쪽은 >=0인 비교를 각각 건너뛴다.
            for (;;) {
                // 첫 피벗 다음 칸부터 검사한다. 범위 밖에서는 비교하지 않는다.
                do { ++left; } while (left<=range.high && Compare(entries,left,range.low)<=0);
                // 피벗 칸에서 종료하며 자기 비교를 호출하지 않는다.
                do { --right; } while (right>range.low && Compare(entries,right,range.low)>=0);
                if (left>right) break;Swap(entries,left,right);
            }
            Swap(entries,range.low,right);
            // 원본은 왼쪽 주소 거리에서 1바이트를 뺀 뒤 오른쪽 거리와 비교한다.
            if ((right-range.low)*16-1<(range.high-left)*16) {
                if (left<range.high) Push(stack,used,{left,range.high});
                if (range.low+1<right) { range.high=right-1;continue; }
            } else {
                if (range.low+1<right) Push(stack,used,{range.low,right-1});range.low=left;
                if (range.low<range.high) continue;
            }
        }
        if (!used) return;range=stack[--used];
    }
}
}
// 검증한 원본 CRT 몸체를 선택하여 동률의 비교/교환 순서를 유지한다.
void SortDrawEntries(std::span<o::RenderCollectionEntry> entries,o::OriginalEdition edition) {
    if (entries.size()>kMaxEntries) throw std::length_error("표시 정렬 항목 수 범위 오류");
    // 계약 오류를 발견해도 목록을 부분 교환하지 않도록 먼저 모든 키를 검사한다.
    for (const auto& entry:entries)
        if (!std::isfinite(entry.x) || !std::isfinite(entry.y)) throw std::invalid_argument("표시 정렬 좌표 오류");
    if (entries.size()<2) return;
    if (edition==o::OriginalEdition::Patch1078) PatchSort(entries);else CdSort(entries);
}
// 스프라이트의 색 변환/그림자/클립/자산 참조를 원래 인덱스로 함께 재배치한다.
void SortRenderSprites(std::vector<RenderSprite>& sprites,o::OriginalEdition edition) {
    if (sprites.size()>kMaxEntries) throw std::length_error("스프라이트 정렬 항목 수 범위 오류");
    std::vector<o::RenderCollectionEntry> entries;entries.reserve(sprites.size());
    // 원본 draw entry에 해당하는 키와 이동 전 인덱스만 만든다.
    for (std::size_t index=0;index<sprites.size();++index) {
        const auto& order=sprites[index].order;entries.push_back({order.x,order.y,static_cast<std::uint32_t>(index),order.depth,0,0});
    }
    SortDrawEntries(entries,edition);std::vector<RenderSprite> sorted;sorted.reserve(sprites.size());
    // 정렬된 원래 인덱스는 서로 다른 전체 스프라이트를 한 번씩 이동한다.
    for (const auto& entry:entries) sorted.push_back(std::move(sprites[entry.token]));sprites=std::move(sorted);
}
}
