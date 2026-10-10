// 소리 이름 표의 찾기/추가와 재생 계층을 원본의 순회·쓰기·장치 호출 순서대로 복원한다. 장치 호출은 SoundDeviceHooks로 나간다.
#include "client/Sound.h"
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
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

// 머리 전체(0x1c바이트)가 표 안에 들어가는 위치만 받는다.
std::uint32_t SoundList::Field(SoundHandle entry,SoundField field) const {
    if (entry<kSoundListBase || entry-kSoundListBase>kSoundListBytes-kSoundHeaderBytes) throw std::out_of_range("소리 항목 값이 표 밖입니다");
    const auto offset=entry-kSoundListBase+static_cast<std::uint32_t>(field);
    std::uint32_t value=0;
    // 낮은 바이트부터 네 바이트를 조합한다.
    for (std::uint32_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw_[offset+i])<<(8*i);
    return value;
}

// 읽기와 같은 범위 검사를 거쳐 쓴다.
void SoundList::SetField(SoundHandle entry,SoundField field,std::uint32_t value) {
    if (entry<kSoundListBase || entry-kSoundListBase>kSoundListBytes-kSoundHeaderBytes) throw std::out_of_range("소리 항목 값이 표 밖입니다");
    Put(entry-kSoundListBase+static_cast<std::uint32_t>(field),value);
}

// 쓰는 순서는 원본과 같다: 이름 → Root → Next → Buffer → 앞 항목의 Next → 빈 위치.
SoundHandle SoundList::AppendDuplicate(SoundHandle named,SoundHandle previous,SoundBuffer buffer) {
    const std::string name(Name(named));
    const auto root=Field(previous,SoundField::Root);
    const std::uint32_t at=firstFree_;
    const auto next=static_cast<std::uint64_t>(at)+kSoundHeaderBytes+name.size()+1;
    if (next>kSoundListBytes) throw std::length_error("복제 소리 항목이 표의 끝을 넘습니다");
    // 이름의 각 바이트와 끝의 NUL을 새 항목의 이름 칸에 옮긴다.
    for (std::size_t i=0;i<name.size();++i) raw_[at+kSoundHeaderBytes+i]=static_cast<std::uint8_t>(name[i]);
    raw_[at+kSoundHeaderBytes+name.size()]=0;
    Put(at+4,root);Put(at+8,0);Put(at,buffer);
    SetField(previous,SoundField::Next,kSoundListBase+at);
    firstFree_=static_cast<std::uint32_t>(next);
    return kSoundListBase+at;
}

// 생성할 때 받은 판본이다.
o::OriginalEdition SoundList::Edition() const { return edition_; }

namespace {
// 원본 기록 채널 번호다: 9는 오류, 0x1d는 소리 관련 알림이다(이름은 추정, 번호는 원본 그대로).
// 원본: 004c2630 / CD 0048ca70의 첫 인자.
// 이력: 2026-10-10 추가.
constexpr std::uint32_t kLogError=9,kLogSound=0x1d;
// 장치 상태 비트: 재생 중, 버퍼를 잃음(DSBSTATUS_PLAYING, DSBSTATUS_BUFFERLOST).
// 이력: 2026-10-10 추가.
constexpr std::uint32_t kStatusPlaying=1,kStatusLost=2;
// 버퍼 도우미와 이름 기반 반복 재생이 보고하는 조건식과 줄 번호다. 유일성 보고의 줄만 판본마다 다르다.
// 원본: 004a8a89("buffer", 0x82), 004a8ada("buffer", 0x9b), 004a8b11("!isLost()", 0x9c), 004a9cd5("qq", 0x3ef) / CD 00438b7d("qq", 0x3ea).
// 이력: 2026-10-10 추가.
constexpr std::string_view kBufferExpression="buffer",kLostExpression="!isLost()",kUniqueExpression="qq";
constexpr int kPlayingLine=0x82,kStopLine=0x9b,kLostLine=0x9c,kPatchUniqueLine=0x3ef,kCdUniqueLine=0x3ea;
// 복제 실패 기록의 줄 번호(되살리기, 쉬는 항목 다시 채우기, 새 항목)와 위치 음량/좌우 설정 실패 기록의 판본별 줄 번호다.
// 원본: 004a95c7·004a9680·004a96c6, 004a9bd6·004a9c00 / CD 00438a5e·00438a88.
// 이력: 2026-10-10 추가.
constexpr int kRestoreLine=0x310,kRefillLine=0x33c,kAppendLine=0x348;
constexpr int kPatchVolumeLine=0x3d0,kPatchPanLine=0x3d5,kCdVolumeLine=0x3cb,kCdPanLine=0x3d0;
// 화면 안 판정의 좌우/상하 여유(픽셀)와 월드 한 칸의 화면 크기, 반올림용 0.5다.
// 원본: 004c6b38·004c6b55의 0x28·0x1e, 00497225의 16.0f·0049723c의 11.0f·0.5.
// 이력: 2026-10-10 추가.
constexpr std::uint32_t kMarginX=40,kMarginY=30;
constexpr double kCellWidth=16.0,kCellHeight=11.0,kHalfPixel=0.5;
// 원본 CRT _ftol처럼 64비트로 0쪽 절삭한 뒤 하위 32비트를 돌려준다. NaN이거나 64비트 범위를 넘으면 0이다.
// 원본: 004e49c0 / CD 004f161c.
// 이력: 2026-10-10 추가.
std::uint32_t TruncateLow(double value) {
    if (!(value>-9223372036854775808.0 && value<9223372036854775808.0)) return 0;
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value)));
}
// (high − low)를 32비트로 뺀 뒤 0쪽으로 절삭해 반으로 나눈다(원본의 cdq; sub; sar 1).
// 이력: 2026-10-10 추가.
std::int32_t Half(std::int32_t low,std::int32_t high) {
    const auto span=static_cast<std::int32_t>(static_cast<std::uint32_t>(high)-static_cast<std::uint32_t>(low));
    return (span-(span>>31))>>1;
}
// 원본 idiv와 같은 0쪽 절삭 나눗셈이다. 원본이 나눗셈 예외로 멈추는 입력(0으로 나누기, 몫 넘침)은 std::domain_error로 알린다.
// 이력: 2026-10-10 추가.
std::int32_t Divide(std::int32_t dividend,std::int32_t divisor) {
    if (divisor==0 || (divisor==-1 && dividend==(-2147483647-1))) throw std::domain_error("소리 위치 계산의 화면 폭이 너무 작습니다");
    return dividend/divisor;
}
// 위치 계산 결과를 [-10000, 10000]으로 자른다.
// 이력: 2026-10-10 추가.
std::int32_t Limit(std::int32_t value) { return value>kSoundMaximum ? kSoundMaximum : value<kSoundMinimum ? kSoundMinimum : value; }
}

// 곱과 덧셈은 x87에서 하고 절삭은 CRT가 한다. 카메라 원점은 32비트로 뺀다.
SoundPoint SoundToScreen(const SoundView& view,float x,float y) {
    const auto px=TruncateLow(static_cast<double>(x)*kCellWidth+kHalfPixel)-static_cast<std::uint32_t>(view.cameraX);
    const auto py=TruncateLow(static_cast<double>(y)*kCellHeight+kHalfPixel)-static_cast<std::uint32_t>(view.cameraY);
    return {static_cast<std::int32_t>(px),static_cast<std::int32_t>(py)};
}

// 여유를 더한 경계는 32비트로 계산하고 비교는 부호 있는 비교다.
bool SoundOnScreen(const SoundView& view,SoundPoint point) {
    // 경계에 여유를 더하거나 뺀 값을 32비트로 구한다.
    const auto edge=[](std::int32_t value,std::uint32_t margin,bool add) {
        return static_cast<std::int32_t>(add ? static_cast<std::uint32_t>(value)+margin : static_cast<std::uint32_t>(value)-margin);
    };
    return point.x>=edge(view.left,kMarginX,false) && point.x<=edge(view.right,kMarginX,true) &&
        point.y>=edge(view.top,kMarginY,false) && point.y<=edge(view.bottom,kMarginY,true);
}

// 가운데(거리 0)는 나눗셈 없이 0이다. 그 밖에는 부호를 붙인 거리 제곱 × 10000을 가로 절반의 제곱으로 나눈다.
std::int32_t SoundPan(const SoundView& view,SoundPoint point,bool swap) {
    const auto half=Half(view.left,view.right);
    auto distance=static_cast<std::uint32_t>(point.x)-static_cast<std::uint32_t>(view.left)-static_cast<std::uint32_t>(half);
    if (swap) distance=0U-distance;
    const auto offset=static_cast<std::int32_t>(distance);
    if (offset==0) return 0;
    const auto scaled=static_cast<std::int32_t>(distance*distance*(offset<0 ? static_cast<std::uint32_t>(kSoundMinimum) : static_cast<std::uint32_t>(kSoundMaximum)));
    return Limit(Divide(scaled,static_cast<std::int32_t>(static_cast<std::uint32_t>(half)*static_cast<std::uint32_t>(half))));
}

// 가운데여도 나눗셈을 한다(원본과 같다). 세로 거리도 가로 절반의 제곱으로 나눈다.
std::int32_t SoundVolume(const SoundView& view,SoundPoint point) {
    const auto halfWidth=Half(view.left,view.right),halfHeight=Half(view.top,view.bottom);
    const auto dx=static_cast<std::uint32_t>(point.x)-static_cast<std::uint32_t>(view.left)-static_cast<std::uint32_t>(halfWidth);
    const auto dy=static_cast<std::uint32_t>(point.y)-static_cast<std::uint32_t>(halfHeight)-static_cast<std::uint32_t>(view.top);
    const auto scaled=static_cast<std::int32_t>((dy*dy+dx*dx)*static_cast<std::uint32_t>(-1000));
    return Limit(Divide(scaled,static_cast<std::int32_t>(static_cast<std::uint32_t>(halfWidth)*static_cast<std::uint32_t>(halfWidth))));
}

// 실행 중에 빠진 장치 효과를 만나지 않도록 구성 단계에서 확인한다.
SoundPlayer::SoundPlayer(SoundList& list,SoundState& state,SoundDeviceHooks hooks,SoundAssertReport report)
    :list_(list),state_(state),hooks_(std::move(hooks)),report_(std::move(report)) {
    if (!hooks_.status || !hooks_.play || !hooks_.setPosition || !hooks_.setVolume || !hooks_.setPan || !hooks_.stop || !hooks_.duplicate || !hooks_.load)
        throw std::invalid_argument("소리 재생 계층의 필수 장치 효과 누락");
}

// 표의 범위 검사를 그대로 거친다.
std::uint32_t SoundPlayer::Get(SoundHandle entry,SoundField field) const { return list_.Field(entry,field); }
// 표의 범위 검사를 그대로 거친다.
void SoundPlayer::Set(SoundHandle entry,SoundField field,std::uint32_t value) { list_.SetField(entry,field,value); }
// 기록 경계는 선택이다. 문장은 원본 형식 문자열을 채운 완성본이다.
void SoundPlayer::Log(std::uint32_t channel,const std::string& text) const { if (hooks_.log) hooks_.log(channel,text); }
// 보고 경계는 선택이다. 원본 assert 보고는 돌아온 뒤 실행을 계속한다.
void SoundPlayer::Report(std::string_view expression,int line) const { if (report_) report_(expression,line); }

// 세 값의 합은 32비트로 넘치게 두고 그 결과를 부호 있는 수로 잘라 낸다.
std::int32_t SoundPlayer::Gain(SoundHandle entry,std::int32_t volume) const {
    const auto sum=static_cast<std::int32_t>(static_cast<std::uint32_t>(volume)+static_cast<std::uint32_t>(state_.masterVolume)-Get(entry,SoundField::Attenuation));
    return sum<kSoundMinimum ? kSoundMinimum : sum>0 ? 0 : sum;
}

// 버퍼가 0인 항목은 호출자가 먼저 걸러야 한다. 원본은 보고 뒤 null을 따라가 멈추므로 여기서는 보고하고 예외로 알린다.
std::uint32_t SoundPlayer::BufferPlaying(SoundHandle entry) {
    const auto buffer=Get(entry,SoundField::Buffer);
    if (buffer==0) { Report(kBufferExpression,kPlayingLine);throw std::logic_error("버퍼가 없는 소리 항목의 상태 조회"); }
    if (buffer==kSilentSoundBuffer) return 0;
    return hooks_.status(buffer)&kStatusPlaying;
}

// 상태 조회(잃음 확인) → 정지 → 처음 위치 → 재생 수 감소 순서다. 재생 중인지는 호출자가 먼저 확인한다.
void SoundPlayer::StopBuffer(SoundHandle entry) {
    const auto buffer=Get(entry,SoundField::Buffer);
    // 원본은 버퍼가 0이면 두 조건을 모두 보고한 뒤, 소리 없음 표식이면 상태 조회에서 멈춘다. 둘 다 호출 계약 위반이다.
    if (buffer==0) { Report(kBufferExpression,kStopLine);Report(kLostExpression,kLostLine);throw std::logic_error("버퍼가 없는 소리 항목의 정지"); }
    if (buffer==kSilentSoundBuffer) throw std::logic_error("소리 없음 항목의 정지");
    if (hooks_.status(buffer)&kStatusLost) Report(kLostExpression,kLostLine);
    hooks_.stop(buffer);
    hooks_.setPosition(buffer,0);
    state_.playing=static_cast<std::int32_t>(static_cast<std::uint32_t>(state_.playing)-1U);
}

// 재생 중인지 한 번 묻고, 재생 중이면 정지 도우미가 다시 상태를 묻는다(원본도 두 번 묻는다).
void SoundPlayer::Stop(SoundHandle sound) {
    if (sound==0 || Get(sound,SoundField::Buffer)==0 || !BufferPlaying(sound)) return;
    StopBuffer(sound);
}

// 준비·항목·옵션·버퍼 순서로 걸러 낸 뒤 장치에 묻는다.
std::uint32_t SoundPlayer::IsPlaying(SoundHandle sound) {
    if (!state_.initialized || sound==0 || !state_.enabled || Get(sound,SoundField::Buffer)==0) return 0;
    return BufferPlaying(sound);
}

// 순서: 준비 확인 → 동시 재생 한도 → (미적재면 적재/되살리기) → 소리 없음 확인 → 쓸 항목 고르기 → 필드 기록 → 음량·좌우·재생.
SoundHandle SoundPlayer::Play(SoundHandle sound,std::uint32_t loop,std::int32_t volume,std::int32_t pan,std::uint32_t priority,std::int32_t limit) {
    if (!state_.initialized || sound==0 || !state_.enabled) return 0;
    if (priority==0 && state_.maxPlaying!=0 && state_.playing>=state_.maxPlaying) return 0;
    const std::string file=list_.Edition()==o::OriginalEdition::Patch1078 ? ".\\Sound.cpp" : "sound.cpp";
    // 버퍼 복제 실패를 원본 문장(파일 이름과 줄 번호 포함)으로 기록한다.
    const auto failed=[&](int line) { Log(kLogError,"Failed to duplicate sound buffer ["+file+" line "+std::to_string(line)+"]\n"); };
    if (Get(sound,SoundField::Buffer)==0) {
        // 원본 항목이 아직 적재되지 않았으면 적재한다. 적재는 원본 항목의 버퍼와 감쇠를 채운다.
        const auto root=Get(sound,SoundField::Root);
        if (Get(root,SoundField::Buffer)==0) {
            const auto loaded=hooks_.load(list_.Name(root));
            Set(root,SoundField::Buffer,loaded.buffer);
            if (loaded.buffer!=0) Set(root,SoundField::Attenuation,static_cast<std::uint32_t>(loaded.attenuation));
        }
        // 그래도 이 항목의 버퍼가 없으면(버퍼가 지워진 복제 항목 등) 원본 항목의 버퍼를 복제해 되살린다.
        if (Get(sound,SoundField::Buffer)==0) {
            SoundBuffer copy=0;
            if (hooks_.duplicate(Get(Get(sound,SoundField::Root),SoundField::Buffer),copy)<0) {
                failed(kRestoreLine);
                Set(sound,SoundField::Buffer,0);
                return sound;
            }
            Set(sound,SoundField::Buffer,copy);
            Log(kLogSound,"Restored sound "+std::string(list_.Name(sound))+"\n");
        }
    }
    if (Get(sound,SoundField::Buffer)==kSilentSoundBuffer) return sound;
    SoundHandle entry=sound;
    SoundBuffer buffer=0;
    if (!BufferPlaying(sound)) buffer=Get(sound,SoundField::Buffer);
    else {
        // 요청한 항목이 재생 중이다. 같은 소리의 사슬을 원본 항목부터 따라가며 재생 중인 항목을 센다.
        SoundHandle walk=Get(sound,SoundField::Root),last=0;
        std::int32_t count=0;
        // 버퍼가 있고 재생 중인 항목이 이어지는 동안 다음 복제 항목으로 넘어간다.
        while (walk!=0 && Get(walk,SoundField::Buffer)!=0 && BufferPlaying(walk)) { last=walk;walk=Get(walk,SoundField::Next);++count; }
        if (limit!=0 && count>=limit) return 0;
        SoundBuffer copy=0;
        if (walk!=0) {
            // 쉬고 있는 항목이 있다. 버퍼가 남아 있으면 그대로 쓰고, 지워졌으면 요청한 항목의 버퍼를 복제해 채운다.
            buffer=Get(walk,SoundField::Buffer);
            if (buffer==0) {
                if (hooks_.duplicate(Get(sound,SoundField::Buffer),copy)<0) { failed(kRefillLine);Set(walk,SoundField::Buffer,0);return walk; }
                Set(walk,SoundField::Buffer,copy);
                buffer=copy;
            }
            entry=walk;
        } else {
            // 사슬이 모두 재생 중이다. 버퍼를 복제해 새 복제 항목을 사슬 끝에 잇는다.
            if (hooks_.duplicate(Get(sound,SoundField::Buffer),copy)<0) { failed(kAppendLine);return sound; }
            if (last==0) throw std::logic_error("소리 항목의 원본 연결이 비어 있습니다");
            entry=list_.AppendDuplicate(sound,last,copy);
            buffer=copy;
        }
    }
    Set(entry,SoundField::Priority,priority);
    Set(entry,SoundField::Serial,state_.serial++);
    Set(entry,SoundField::Volume,static_cast<std::uint32_t>(volume));
    if (hooks_.setVolume(buffer,Gain(entry,volume))<0) Log(kLogSound,"failed to set buffer volume\n");
    static_cast<void>(hooks_.setPan(buffer,pan));
    // [원본] 패치판은 여기서 변조 감지용 값(DAT_005318ec)이 7이면 null 버퍼로 재생을 불러 일부러 멈춘다. 옮기지 않았다.
    hooks_.play(buffer,loop!=0 ? 1U : 0U);
    state_.playing=static_cast<std::int32_t>(static_cast<std::uint32_t>(state_.playing)+1U);
    return entry;
}

// 순서: 옵션/장치 확인 → 이름 비교(다르면 현재 항목 정지) → 화면 위치 → (필요하면 시작) → 화면 밖이면 정지 → 재생 중이면 음량/좌우 갱신.
SoundHandle SoundPlayer::PlayLoopAt(float x,float y,SoundHandle current,SoundHandle sound) {
    if (!state_.enabled || !state_.device) return 0;
    if (current!=0 && sound!=0) {
        // 같은 이름(복제 항목 포함)이면 지금 것을 계속 쓰고, 다른 이름이면 지금 것을 재생 여부와 무관하게 멈춘다.
        if (SameName(list_.Name(current),list_.Name(sound))) sound=0;
        else { StopBuffer(current);current=0; }
    }
    const auto point=SoundToScreen(state_.view,x,y);
    const bool visible=SoundOnScreen(state_.view,point);
    if (visible && !(current!=0 && Get(current,SoundField::Buffer)!=0 && BufferPlaying(current)))
        current=Play(sound!=0 ? sound : current,1,0,0,0,1);
    if (current==0) return 0;
    if (!visible) {
        if (Get(current,SoundField::Buffer)==0) return current;
        if (BufferPlaying(current)) { StopBuffer(current);return 0; }
    }
    if (Get(current,SoundField::Buffer)==0 || !BufferPlaying(current)) return current;
    const bool patch=list_.Edition()==o::OriginalEdition::Patch1078;
    const std::string file=patch ? ".\\Sound.cpp" : "sound.cpp";
    const auto volume=SoundVolume(state_.view,point);
    const auto pan=SoundPan(state_.view,point,state_.swapSpeakers);
    Set(current,SoundField::Volume,static_cast<std::uint32_t>(volume));
    const auto buffer=Get(current,SoundField::Buffer);
    // 이 두 설정은 0이 아닌 모든 반환을 실패로 기록한다(전역 재생은 음수만 기록한다).
    if (hooks_.setVolume(buffer,Gain(current,volume))!=0)
        Log(kLogError,"Failed to set sound buffer volume [file "+file+", line "+std::to_string(patch ? kPatchVolumeLine : kCdVolumeLine)+"]\n");
    if (hooks_.setPan(buffer,pan)!=0)
        Log(kLogError,"Failed to set sound buffer pan [file "+file+", line "+std::to_string(patch ? kPatchPanLine : kCdPanLine)+"]\n");
    return current;
}

// 화면 밖이면 장치에도 표에도 손대지 않는다. 한도(limit)는 없고 우선 인자는 그대로 넘긴다.
SoundHandle SoundPlayer::PlayOnceAt(float x,float y,SoundHandle sound,std::uint32_t loop,std::uint32_t priority) {
    const auto point=SoundToScreen(state_.view,x,y);
    if (!SoundOnScreen(state_.view,point)) return 0;
    const auto pan=SoundPan(state_.view,point,state_.swapSpeakers);
    return Play(sound,loop,SoundVolume(state_.view,point),pan,priority,0);
}

// 원본은 이름을 두 번 조회한다(유일성 확인용, 재생용). 조회가 표에 이름을 등록하므로 재생하지 못해도 항목은 남는다.
SoundHandle SoundPlayer::PlayByName(std::string_view name,std::uint32_t loop,std::int32_t volume,std::int32_t pan,std::uint32_t priority,std::int32_t limit) {
    if (loop!=0) {
        const auto entry=list_.Lookup(name);
        if (Get(entry,SoundField::Root)!=entry || Get(entry,SoundField::Next)!=0)
            Report(kUniqueExpression,list_.Edition()==o::OriginalEdition::Patch1078 ? kPatchUniqueLine : kCdUniqueLine);
    }
    return Play(list_.Lookup(name),loop,volume,pan,priority,limit);
}

// 화면 밖이어도 이름은 먼저 등록된다.
SoundHandle SoundPlayer::PlayNameAt(float x,float y,std::string_view name,std::uint32_t loop) {
    return PlayOnceAt(x,y,list_.Lookup(name),loop,0);
}
}
