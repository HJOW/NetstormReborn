// 소리 이름 표의 찾기/추가를 원본의 순회·쓰기 순서대로 복원한다. 소리 장치 호출은 여기에 없다.
#include "client/Sound.h"
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// CD판이 순회 중 항목 이름의 끝 글자를 검사할 때 보고하는 조건식과 줄 번호다.
// 사용: 이름의 마지막 글자가 소문자 'v'가 아니면 SoundList::Lookup이 이 값으로 보고한다(패치판에는 없다).
// 원본: CD 00437b97의 assert("s->isRobust()", "sound.cpp", 0x1b6).
// 이력: 2026-10-10 추가.
constexpr std::string_view kCdRobustExpression="s->isRobust()";
constexpr int kCdRobustLine=0x1b6;
// 순회가 끝난 위치와 기록해 둔 빈 위치가 다를 때 보고하는 조건식과 줄 번호다(두 판본 공통).
// 원본: 004a8df3 / CD 00437bfb의 assert("s == firstFreeSound", …, 0x1c7).
// 이력: 2026-10-10 추가.
constexpr std::string_view kFirstFreeExpression="s == firstFreeSound";
constexpr int kFirstFreeLine=0x1c7;
// C 로캘 _stricmp처럼 ASCII 대문자 A~Z만 소문자로 접는다. 그 밖의 글자(상위 비트 포함)는 그대로 둔다.
// 원본: 004ef510(__ascii_stricmp) / CD 004f2980의 `sub al,0x41; cmp al,0x1a` 접기.
// 이력: 2026-10-10 추가.
std::uint8_t Fold(char value) {
    const auto byte=static_cast<std::uint8_t>(value);
    return static_cast<std::uint8_t>(byte-0x41U)<0x1aU ? static_cast<std::uint8_t>(byte+0x20U) : byte;
}
// 두 이름이 ASCII 대소문자를 무시하고 같은지 본다. 길이가 다르면 다르다.
// 사용: 원본은 비교 결과의 부호를 쓰지 않고 0인지만 본다.
// 이력: 2026-10-10 추가.
bool SameName(std::string_view left,std::string_view right) {
    return left.size()==right.size() && std::equal(left.begin(),left.end(),right.begin(),[](char a,char b) { return Fold(a)==Fold(b); });
}
}

// 표 전체를 0으로 채워 둔다. 빈 위치는 표의 시작이고 대체 소리는 아직 없다.
SoundList::SoundList(o::OriginalEdition edition,SoundAssertReport report):edition_(edition),report_(std::move(report)),raw_(kSoundListBytes) {}

// 원본은 표 포인터가 null일 때만 확보와 첫 등록을 한다. 여기서는 등록 여부를 따로 기억한다.
void SoundList::Initialize() {
    if (initialized_) return;
    initialized_=true;
    fallback_=Lookup(kFallbackSoundName);
}

// 표 밖의 오프셋은 빈 이름(표의 끝)으로 취급한다. 원본은 이 경우 표 밖 메모리를 읽는다.
std::string_view SoundList::NameAt(std::uint32_t offset) const {
    if (offset>=kSoundListBytes) return {};
    const auto begin=raw_.begin()+static_cast<std::ptrdiff_t>(offset);
    const auto end=std::find(begin,raw_.end(),std::uint8_t{});
    return {reinterpret_cast<const char*>(raw_.data())+offset,static_cast<std::size_t>(end-begin)};
}

// 호출자가 offset + 4가 표 안임을 보장한다.
void SoundList::Put(std::uint32_t offset,std::uint32_t value) {
    // 낮은 바이트부터 네 바이트를 쓴다.
    for (std::uint32_t i=0;i<4;++i) raw_[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}

// 원본 assert 보고는 돌아온 뒤 실행을 계속하므로 여기서도 흐름을 바꾸지 않는다.
void SoundList::Report(std::string_view expression,int line) const { if (report_) report_(expression,line); }

// 순회 → (가득 참이면 대체 소리) → 빈 위치 확인 → 이름 복사 → 머리 초기화 → 빈 위치 갱신 순서다.
SoundHandle SoundList::Lookup(std::string_view name) {
    if (name.empty() || name.find('\0')!=std::string_view::npos) throw std::invalid_argument("소리 이름이 비었거나 NUL을 포함합니다");
    std::uint32_t at=0;
    // 첫 항목부터 이름이 빈 항목(표의 끝)까지 차례로 비교한다. 같은 이름의 복제 항목이 있어도 처음 만난 항목을 돌려준다.
    for (;;) {
        const auto existing=NameAt(at+kSoundHeaderBytes);
        if (existing.empty()) break;
        // CD판만 비교 전에 항목 이름의 끝 글자를 검사한다. 보고 뒤에도 비교는 계속한다.
        if (edition_==o::OriginalEdition::Cd1072 && existing.back()!='v') Report(kCdRobustExpression,kCdRobustLine);
        if (SameName(existing,name)) return kSoundListBase+at;
        at+=kSoundHeaderBytes+static_cast<std::uint32_t>(existing.size())+1;
    }
    // 한도 이상에서 끝났으면 새 항목을 넣지 않고 대체 소리를 돌려준다.
    if (at>=kSoundListLimit) return fallback_;
    if (at!=firstFree_) Report(kFirstFreeExpression,kFirstFreeLine);
    const auto next=static_cast<std::uint64_t>(at)+kSoundHeaderBytes+name.size()+1;
    if (next>kSoundListBytes) throw std::length_error("소리 이름이 표의 끝을 넘습니다");
    // 이름(끝의 NUL은 0으로 채운 표에 이미 있다)을 먼저 쓰고 머리 필드를 초기화한다. +0x10은 쓰지 않는다.
    // 이름의 각 바이트를 표의 이름 칸에 옮긴다.
    for (std::size_t i=0;i<name.size();++i) raw_[at+kSoundHeaderBytes+i]=static_cast<std::uint8_t>(name[i]);
    raw_[at+kSoundHeaderBytes+name.size()]=0;
    Put(at,0);Put(at+4,kSoundListBase+at);Put(at+8,0);Put(at+0xc,0);Put(at+0x14,0);Put(at+0x18,0);
    firstFree_=static_cast<std::uint32_t>(next);
    return kSoundListBase+at;
}

// null 이름은 원본처럼 "없음"으로 답한다. 그 밖에는 문자열 길이를 재어 위 조회로 넘긴다.
SoundHandle SoundList::Lookup(const char* name) { return name ? Lookup(std::string_view(name)) : SoundHandle{}; }

// 표를 읽거나 바꾸지 않는다. 결과는 표의 시작보다 앞일 수 있다(부호 없는 뺄셈 그대로).
SoundHandle SoundList::LookupPointer(SoundHandle pointer) const {
    if (pointer<kSoundListBase || pointer-kSoundListBase>=kSoundListBytes) throw std::out_of_range("표 안을 가리키는 소리 이름 포인터가 아닙니다");
    return pointer-kSoundPointerBias;
}

// Initialize가 등록한 첫 항목 값이다.
SoundHandle SoundList::Fallback() const { return fallback_; }
// 원본 firstFreeSound 포인터에 해당한다.
SoundHandle SoundList::FirstFree() const { return kSoundListBase+firstFree_; }

// 이름 필드가 표 안에 있는 항목 값만 받는다.
std::string_view SoundList::Name(SoundHandle entry) const {
    if (entry<kSoundListBase || entry-kSoundListBase>=kSoundListBytes-kSoundHeaderBytes) throw std::out_of_range("소리 항목 값이 표 밖입니다");
    return NameAt(entry-kSoundListBase+kSoundHeaderBytes);
}

// 원본 배치 그대로의 0x8000바이트다.
std::span<const std::uint8_t> SoundList::Raw() const { return raw_; }
}
