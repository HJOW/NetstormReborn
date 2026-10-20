#include "client/KnowledgeSort.h"
#include <array>
#include <algorithm>
#include <stdexcept>
namespace netstorm::client {
namespace {
constexpr std::array<std::uint32_t,7> kPriority{0x800,0x8000000,0x10000,0x4000000,0x20000000,0x40000,0x20000}; // 453f70의 첫 공통 플래그에서 멈추는 순서다.
}
// 첫 공통 우선 플래그에서 분류를 끝내고 같은 분류는 원본 C 로캘 이름으로 비교한다.
int CompareKnowledgeRecords(const KnowledgeRecord& first,const KnowledgeRecord& second,std::span<const o::RiftTypeRecord> types) {
    if (first.type>=types.size() || second.type>=types.size()) throw std::out_of_range("지식 정렬 TYPE 범위");
    const auto a=types[first.type].flags2,b=types[second.type].flags2;
    // 한쪽에만 우선 플래그가 있으면 즉시 반환한다. 둘 다 있으면 뒤 플래그를 보지 않는다.
    for (const auto bit:kPriority) { if (a&bit) { if (!(b&bit)) return -1;break; }if (b&bit) return 1; }
    // 첫NUL까지의 unsigned ASCII 차이를 반환한다. 대문자만 소문자로 접는다.
    for (std::size_t index=0;;++index) {
        auto x=index<first.description.size() ? static_cast<unsigned char>(first.description[index]) : 0U;auto y=index<second.description.size() ? static_cast<unsigned char>(second.description[index]) : 0U;
        if (x>='A' && x<='Z') x+=32;if (y>='A' && y<='Z') y+=32;if (x!=y) return static_cast<int>(x)-static_cast<int>(y);if (!x) return 0;
    }
}
// 원본 CRT의 작은 구간 선택/중앙 피벗/동률 제거/큰 구간 보관을 같은 순서로 실행한다.
void SortKnowledgeRecords(std::span<KnowledgeRecord> entries,std::span<const o::RiftTypeRecord> types) {
    constexpr std::ptrdiff_t kSmall=8; // 원본 CRT가 shortsort를 선택하는 포함 구간 길이다.
    constexpr std::size_t kRanges=30; // 원본 CRT 구간 스택의 최대 쌍 개수다.
    struct Range { std::ptrdiff_t low{},high{}; }; // 현재 포함 구간의 첫/마지막 인덱스다.
    std::array<Range,kRanges> stack{};
    std::size_t used=0;
    Range range{0,static_cast<std::ptrdiff_t>(entries.size())-1};
    // 주소 대신 현재 인덱스의 원소를 비교한다.
    const auto compare=[&](std::ptrdiff_t a,std::ptrdiff_t b) { return CompareKnowledgeRecords(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)],types); };
    // 원본 byte swap과 같은 항목 전체 교환이다.
    const auto swap=[&](std::ptrdiff_t a,std::ptrdiff_t b) { if (a!=b) std::swap(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)]); };
    // 큰 구간을 고정 스택에 보관하며 다음에 작은 구간을 처리한다.
    const auto push=[&](Range next) { if (used==stack.size()) throw std::logic_error("Factory 지식 정렬 스택 넘침");stack[used++]=next; };
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
}
