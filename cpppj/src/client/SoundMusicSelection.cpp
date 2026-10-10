// 현재 곡 이름 저장/존재 확인/옵션·특수 곡 loop와 원본 fallback 순서를 복원한다.
#include "client/SoundMusicSelection.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본 C 로캘 비교와 같이 ASCII A~Z만 접는다. 파일 이름 자체/현재 저장 이름의 대소문자는 바꾸지 않는다.
bool SameName(std::string_view a,std::string_view b) {
    if (a.size()!=b.size()) return false;
    // 길이가 같은 이름의 각 바이트를 대소문자 구분 없이 비교한다.
    for (std::size_t i=0;i<a.size();++i) {
        const auto fold=[](char c) { return c>='A' && c<='Z' ? static_cast<char>(c-'A'+'a') : c; }; // ASCII 한 바이트를 접는다.
        if (fold(a[i])!=fold(b[i])) return false;
    }
    return true;
}
}
// 전체 저장 공간의 가변 보기다.
std::span<std::uint8_t> MusicSelectionState::Raw() { return name_; }
// 전체 저장 공간의 읽기 전용 보기다.
std::span<const std::uint8_t> MusicSelectionState::Raw() const { return name_; }
// 원본 이름은 NUL 종료 문자열이다. 호스트에서 종료 없는 raw를 읽지는 않는다.
std::string_view MusicSelectionState::Current() const {
    const auto end=std::find(name_.begin(),name_.end(),std::uint8_t{});
    if (end==name_.end()) throw std::out_of_range("현재 음악 이름에 종료 문자가 없습니다");
    return {reinterpret_cast<const char*>(name_.data()),static_cast<std::size_t>(end-name_.begin())};
}
// 첫 255바이트는 strncpy처럼 복사/패딩하고 +255는 보존한다.
void MusicSelectionState::Assign(std::string_view name) {
    const auto size=std::min(name.size(),name_.size()-1);std::copy_n(name.begin(),size,name_.begin());
    std::fill(name_.begin()+size,name_.end()-1,std::uint8_t{});
}
// demo 선택은 직접 Play가 아니라 전역 옵션을 다시 읽는 실제 상위 래퍼로 돌아온다.
MusicSelection::MusicSelection(MusicSelectionState& state,SoundMusic& music,MusicDirectories directories,MusicOpenHooks files,MusicBufferHooks buffers)
    :state_(state),music_(music),directories_(std::move(directories)),files_(std::move(files)),buffers_(std::move(buffers)) {
    if (!files_.find || !files_.open) throw std::invalid_argument("음악 선택의 파일 경계 누락");
    files_.fallback=[this](std::string_view name) { const std::string text(name);Select(text.c_str()); }; // 원본 옵션 래퍼와 같은 재선택 경계다.
}
// 공개 래퍼는 현재 전역 옵션을 그대로 넘긴다.
void MusicSelection::Select(const char* name) { Select(name,state_.enabled); }
// 새 곡을 못 찾으면 이전 곡을 먼저 정지하지 않는다. 같은 곡도 공개 Play 또는 Stop은 요청한다.
void MusicSelection::Select(const char* name,std::int32_t enabled) {
    if (!name) throw std::invalid_argument("선택할 음악 이름이 null입니다");
    const std::string_view selected(name),current=state_.Current();
    if (!SameName(selected,current)) {
        if (!selected.empty() && !current.empty() && FindMusicFile(selected,directories_,files_.find)) music_.Stop();
        state_.Assign(selected);
    }
    if (enabled!=0) music_.Play(name,SameName(selected,"fanfare.mus") || SameName(selected,"defeat.mus") ? 0U : 1U,directories_,files_,buffers_);
    else music_.Stop();
}
}
