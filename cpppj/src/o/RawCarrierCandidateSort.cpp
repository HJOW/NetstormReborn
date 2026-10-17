// 표준 라이브러리 정렬의 동률 순서는 원본 CRT와 다르므로 실제 제어 흐름을 사용한다.
#include "o/RawCarrierCandidateSort.h"
#include <array>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// PATCH의50바이트 슬롯 산술을 CD의 다른 슬롯 폭에 적용하지 않는다.
RawCarrierCandidateSort::RawCarrierCandidateSort(const SidPool& pool,std::function<std::uint32_t(const void*)> relocate)
    :pool_(pool),relocate_(std::move(relocate)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("운반 후보 정렬의 판본 오류");
}
// 원본 주소 산술은64비트 호스트에서도32비트로 감긴다.
std::uint32_t RawCarrierCandidateSort::Address(const void* pointer) const {
    return relocate_ ? relocate_(pointer) : static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer));
}
// 후보 값/슬롯 정렬 의도로 고치지 않고 실제 unsigned 주소 비교와 WORD 차이를 보존한다.
std::int32_t RawCarrierCandidateSort::Compare(std::uint32_t firstAddress,std::uint32_t secondAddress) const {
    const auto raw=pool_.Bytes();const auto base=Address(raw.data());
    const auto first=firstAddress*50U+base,second=secondAddress*50U+base;
    const auto low=base+50U,high=base+pool_.Capacity()*50U;
    const auto validFirst=first>=low && first<high,validSecond=second>=low && second<high;
    if (!validFirst && !validSecond) return 0;
    if (!validFirst) return 1;
    if (!validSecond) return -1;
    // 감김으로 슬롯 중간을 가리키는 주소도 나눗셈으로 정렬하지 않는다. 실제 확보 범위 밖 읽기는 거부한다.
    const auto word=[&](std::uint32_t address) -> std::int32_t {
        const auto offset=static_cast<std::uint32_t>(address-base)+8U;
        if (offset>=raw.size() || raw.size()-offset<2) throw std::out_of_range("운반 후보 비교기의 raw WORD 범위 오류");
        return raw[offset]|(std::int32_t(raw[offset+1])<<8);
    };
    const auto secondWord=word(second);return word(first)-secondWord;
}
// 원본은재귀 대신 큰 구간을30쌍 스택에 두고 작은 구간부터 처리한다.
void RawCarrierCandidateSort::Sort(std::span<std::uint32_t> candidates) const {
    if (candidates.size()<2) return;
    if (candidates.size()>0x3fffffffU) throw std::length_error("운반 후보 정렬의32비트 폭 초과");
    // 비교 인자는 배열 요소의 주소다. 평가 순서를 명시해 관측용 재배치도 원본 호출 순서로 읽는다.
    const auto compare=[&](std::ptrdiff_t first,std::ptrdiff_t second) {
        const auto firstAddress=Address(&candidates[static_cast<std::size_t>(first)]);
        const auto secondAddress=Address(&candidates[static_cast<std::size_t>(second)]);
        return Compare(firstAddress,secondAddress);
    };
    // 폭4의 바이트 교환과 같은 DWORD 교환이며 비교기는 교환 뒤 현재 위치를 다시 받는다.
    const auto swap=[&](std::ptrdiff_t first,std::ptrdiff_t second) {
        if (first!=second) std::swap(candidates[static_cast<std::size_t>(first)],candidates[static_cast<std::size_t>(second)]);
    };
    std::array<std::pair<std::ptrdiff_t,std::ptrdiff_t>,30> pending{}; // 원본 두30항목 스택의 경계다.
    std::size_t depth{}; // 아직 처리하지 않은 큰 구간의 수다.
    std::ptrdiff_t low{},high=static_cast<std::ptrdiff_t>(candidates.size()-1); // 현재 구간의 포함 양끝이다.
    // 정렬이 끝난 구간은 스택에서 다음 구간을 꺼내고, 두 구간이면 작은 쪽부터 반복한다.
    for (;;) {
        if (high-low+1<=8) {
            //4e5fa0은 엄격한 >로 최대 위치를 고른 뒤 끝과 교환하므로 동률도 회전한다.
            for (auto last=high;last>low;--last) {
                auto maximum=low; // 동률이면 처음 발견한 현재 위치를 유지한다.
                // 현재 마지막 위치까지 비교하며 SID 값은 전혀 읽지 않는다.
                for (auto probe=low+1;probe<=last;++probe) if (compare(probe,maximum)>0) maximum=probe;
                swap(maximum,last);
            }
        } else {
            auto middle=low+(high-low+1)/2; // 세 위치의 중앙 pivot이며 교환 뒤 위치가 바뀔 수 있다.
            if (compare(low,middle)>0) swap(low,middle);
            if (compare(low,high)>0) swap(low,high);
            if (compare(middle,high)>0) swap(middle,high);
            auto left=low,right=high; // 안쪽 탐색의 현재 두 위치다.
            //4e6140: pivot 앞/뒤의 전진 조건과 pivot 교환 뒤 재배치를 보존한다.
            for (;;) {
                if (middle>left) {
                    // pivot 전까지는 <=인 요소를 지나간다.
                    do { ++left; } while (left<middle && compare(left,middle)<=0);
                    if (middle>left) goto scan_right;
                }
                // pivot을 만났거나 지났으면 상한까지 같은 조건으로 전진한다.
                do { ++left; } while (left<=high && compare(left,middle)<=0);
scan_right:
                // 오른쪽은 pivot 위치에서 멈추며 엄격한 >인 위치만 지나간다.
                do { --right; } while (right>middle && compare(right,middle)>0);
                if (left>right) break;
                swap(left,right);
                if (middle==right) middle=left;
            }
            ++right;
            if (middle<right) {
                // pivot 오른쪽 동률 구간의 끝을 제외한다.
                do { --right; } while (right>middle && compare(right,middle)==0);
                if (middle<right) goto partition_done;
            }
            // pivot 왼쪽도 같은 키를 제외하되 low는 다시 비교하지 않는다.
            do { --right; } while (right>low && compare(right,middle)==0);
partition_done:
            // 현재 큰 구간만 저장한다. 원본의 signed 길이 비교를 요소 인덱스로 표현한다.
            const auto push=[&](std::ptrdiff_t first,std::ptrdiff_t last) {
                if (depth==pending.size()) throw std::length_error("운반 후보 정렬의 구간 스택 초과");
                pending[depth++]={first,last};
            };
            if (right-low>=high-left) {
                if (low<right) push(low,right);
                if (left<high) { low=left;continue; }
            } else {
                if (left<high) push(left,high);
                if (low<right) { high=right;continue; }
            }
        }
        if (!depth) break;
        const auto next=pending[--depth];low=next.first;high=next.second;
    }
}
}
