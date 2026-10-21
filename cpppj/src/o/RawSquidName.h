// 원본 객체 가상+68의 TYPE 설명 이름과 Factory 레벨 접미사를 같은 현재 자료에서 만든다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::o {
// 보호 PE에서 확인한83개 실제 가상 표만 이름 몸체로 분배한다.
std::uint32_t SquidNameEntry(std::uint32_t table);
//004a8980의 원본 로마 숫자와 마지막ff sentinel을 독립 문자열로 돌려준다.
std::string SquidLevelName(std::uint32_t level);
// 현재 언어/번역/설정 치환과 실제 가상 레벨을 빌린다.
struct SquidNameHooks {
    std::function<std::uint32_t()> language; //004de380의 현재 언어 번호를 원본 위치에서 두 번까지 읽는다.
    std::function<std::string(std::string_view)> translate; //004de9a0의 현재 Xlat이다. Factory 접미사는 언어 관문 없이 호출한다.
    std::function<std::string(std::string_view)> substitute; //00441740(name,80,0,0,0)의 현재 설정 치환이다.
    std::function<std::uint32_t(Sid)> level; //Factory 가상80의 현재 저장 레벨이다.
};
// 실제 raw TYPE/가상 표를 사용하고 이름을 새 고정값으로 추측하지 않는다.
class RawSquidName {
public:
    RawSquidName(const SidPool& pool,std::span<const RiftTypeRecord> types,SquidNameHooks hooks); //같은PATCH/188TYPE와장치경계를빌린다.
    std::string Name(Sid parent) const; //004ace20 또는00454d30의 실제 이름을 만든다.
    const SidPool& Pool() const; //같은 실제 월드 연결을 확인한다.
private:
    std::string Base(Sid parent) const; //현재 TYPE의 설명→언어 관문→80바이트 복사→설정 치환이다.
    const SidPool& pool_; //현재 TYPE/가상 표를 읽는 실제 풀이다.
    std::span<const RiftTypeRecord> types_; //현재 TYPE+18 설명을 빌리는 같은 표다.
    SquidNameHooks hooks_; //현재 언어/번역/설정/가상 레벨 경계다.
};
}
