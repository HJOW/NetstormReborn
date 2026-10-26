// 기존 RawDataManagerImage의 Name 몸체를 공유하며 이름 선택과 strlen 시점을 분리한다.
#include "o/RawSnart.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::o { // 같은 표를 파일 읽기와 신규 초기화가 함께 빌린다.
std::span<std::uint8_t> SelectSnartName(std::span<std::uint8_t> names,std::uint32_t index,const SnartSeparatorAssertion& assertion) { // 한 번 변환한 첫BYTE0과 마지막FF는 다음 호출에도 그대로 남긴다.
    if (names.empty()) throw std::out_of_range("SNART 실제 이름표 누락");
    if (names[0]) {
        if (names[0]!='/') { if (!assertion) throw std::logic_error("SNART separator 실제 진단 미연결");assertion(); }
        const auto end=std::find(names.begin(),names.end(),std::uint8_t{});if (end==names.end()) throw std::out_of_range("SNART 입력 NUL 누락");*end=0xff;
        // 원문 첫FF 전의 slash만NUL로 바꾸며 나머지 원시 BYTE는 보존한다.
        for (std::size_t at=0;at<names.size() && names[at]!=0xff;++at) if (names[at]=='/') names[at]=0;
    }
    std::size_t at=1; // index0은 같은 표의 첫 separator 바로 다음 포인터다.
    // 기존 파일 reader와 같은 현재NUL 개수만 지나고 다음FF를 만나면 멈춘다.
    while (index) { if (at>=names.size()) throw std::out_of_range("SNART 이름 번호 범위");if (!names[at]) { --index;++at;if (at>=names.size()) throw std::out_of_range("SNART 끝 누락");if (names[at]==0xff) break; } else ++at; }
    if (at>=names.size()) throw std::out_of_range("SNART 이름 시작 범위");return names.subspan(at);
}
std::string_view ReadSnartName(std::span<std::uint8_t> names,std::uint32_t index,const SnartSeparatorAssertion& assertion) { // 초기화 부모와 달리 실제 기존 reader가 선택하는 현재길이 읽기다.
    const auto tail=SelectSnartName(names,index,assertion);const auto end=std::find(tail.begin(),tail.end(),std::uint8_t{});if (end==tail.end()) throw std::out_of_range("SNART 이름 NUL 누락");return {reinterpret_cast<const char*>(tail.data()),static_cast<std::size_t>(end-tail.begin())};
}
}
