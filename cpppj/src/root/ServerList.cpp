// Root 문자열 표의 원문 순서/대소문자·체크섬 및 세미콜론 행 연산을 복원한다.
#include "root/ServerList.h"
#include "root/MainLoop.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace netstorm::root { // 현재 Root 응답/메인 감시가 같은 표와 checksum을 읽는다.
namespace { // 원문의 첫NUL과 CRT 소문자 변환·고정 캡처 경계다.
std::string_view CString(std::string_view text) { return text.substr(0,text.find('\0')); } // strdup/strcmp가 소비하는 실제 C문자열 범위다.
std::string Lower(std::string text) { for (auto& value:text) value=static_cast<char>(std::tolower(static_cast<unsigned char>(value)));return text; } // 현재 CRT locale의 바이트 소문자 변환이며 UTF-8 의미 변환을 하지 않는다.
void Capture(std::span<char> output,std::string_view text) { if (output.empty()) return;const auto count=std::min(output.size()-1,text.size());std::copy_n(text.begin(),count,output.begin());output[count]=0; } //47c790의 지정 최댓값 복사/마지막NUL과 같은 유효 출력 범위다.
std::string Row(std::string_view name,std::string_view description,std::int32_t value) { return std::string(name)+';'+std::string(description)+';'+std::to_string(value); } // 원문256BYTE 지역에 들어가는78+78BYTE 필드/정수 형식이다.
}
ServerList::ServerList(std::uint32_t& checksum,o::RawFileName& names,SharedFileStorage& storage):checksum_(checksum),names_(names),storage_(storage) { entries_.reserve(capacity_);checksum_=0; } // 원문 최초 생성자의 체크섬0과20칸 할당이다.
void ServerList::Clear() { entries_.clear();checksum_=0; } // 현재 flags와 논리 capacity는 보존하고 문자열/현재 count만 비운다.
const std::string* ServerList::Entry(std::int32_t index) const { if (index<0 || static_cast<std::size_t>(index)>=entries_.size()) return nullptr;return entries_[static_cast<std::size_t>(index)].get(); }
std::int32_t ServerList::Find(std::string_view text,bool partial) const {
    const std::string needle(CString(text));
    for (std::size_t index=0;index<entries_.size();++index) { // 원문처럼 현재 순서의 첫 비NULL 일치를 선택한다.
        if (!entries_[index]) continue;const auto& entry=*entries_[index];if (!partial) { if ((flags_&2U ? ::_stricmp(needle.c_str(),entry.c_str()):std::strcmp(needle.c_str(),entry.c_str()))==0) return static_cast<std::int32_t>(index); }else { const auto current=flags_&2U ? Lower(std::string(CString(entry))):std::string(CString(entry));if (current.find(needle)!=std::string::npos) return static_cast<std::int32_t>(index); }
    }
    return -1;
}
bool ServerList::Add(std::string_view text,bool recalculate) {
    if (entries_.size()>=capacity_) { const auto grown=std::uint64_t(capacity_)+capacity_/2U;if (grown>static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) throw std::overflow_error("R 서버 목록 논리 capacity의 signed DWORD 범위");capacity_=static_cast<std::uint32_t>(grown);entries_.reserve(capacity_); } // 중복 검사보다 먼저 확장하는 원문 순서다.
    if ((flags_&4U) && Find(text,false)>=0) return false;entries_.push_back(std::make_unique<std::string>(CString(text)));if (recalculate) Recalculate();return true;
}
void ServerList::Remove(std::string_view text,bool recalculate) { const auto index=Find(text,false);if (index>=0) entries_.erase(entries_.begin()+index);if (recalculate) Recalculate(); } // 순서를 당겨 count를 줄이며 원문 해제 누락은 호스트 소유권으로 정리한다.
void ServerList::Replace(std::int32_t index,std::string_view text) { if (Entry(index)) entries_[static_cast<std::size_t>(index)]=std::make_unique<std::string>(CString(text)); } // 원문처럼 교체 뒤 자동 checksum/중복 검사를 추가하지 않는다.
std::uint32_t ServerList::Recalculate() {
    checksum_=0;
    for (const auto& entry:entries_) if (entry) for (const auto value:CString(*entry)) checksum_+=static_cast<unsigned char>(value)+1U; // 구분자/NUL 없이 각 unsigned 바이트+1을 DWORD 모듈러 합산한다.
    return checksum_;
}
std::int32_t ServerList::BufferLength(std::int32_t separatorBytes) const {
    std::uint32_t count{};for (const auto& entry:entries_) if (entry) count+=static_cast<std::uint32_t>(CString(*entry).size())+static_cast<std::uint32_t>(separatorBytes);return std::bit_cast<std::int32_t>(count); // 실제 strlen+매행 separator의 low DWORD다.
}
void ServerList::ParseRow(std::string_view row,std::span<char> name,std::span<char> description,std::int32_t* value) {
    std::fill(name.begin(),name.end(),'\0');std::fill(description.begin(),description.end(),'\0');if (value) *value=0;row=CString(row);const auto first=row.find(';');if (first==row.npos) return;const auto second=row.find(';',first+1);Capture(name,row.substr(0,first));Capture(description,row.substr(first+1,second==row.npos ? row.size()-first-1:second-first-1));if (value && second!=row.npos) { const std::string text(row.substr(second+1,79));*value=static_cast<std::int32_t>(std::atoi(text.c_str())); }
} // 고정 패턴 ([^;]*);([^;]*);?(.*)? 의 정상 C문자열 캡처와 같은 결과다.
void ServerList::Decode(std::int32_t index,std::span<char> name,std::span<char> description,std::int32_t* value) const { if (const auto* entry=Entry(index)) ParseRow(*entry,name,description,value); } // 범위 밖/NULL이면 caller의 기존 출력도 보존한다.
void ServerList::Upsert(std::string_view row,bool preserveValue) {
    std::array<char,80> name{},description{};std::int32_t value{};ParseRow(row,std::span(name).first(79),std::span(description).first(79),&value);const auto lowered=Lower(name.data());const auto index=Find(lowered,true);if (index<0) { Add(Row(lowered,description.data(),value),true);return; }std::int32_t previous{};Decode(index,{},{},&previous);Replace(index,Row(lowered,description.data(),preserveValue ? previous:value));
} // 원문 기존행 교체 경로의 stale checksum도 임의 갱신하지 않는다.
void ServerList::SetValue(std::int32_t index,std::int32_t value) { if (!Entry(index)) return;std::array<char,80> name{},description{};Decode(index,std::span(name).first(79),std::span(description).first(79));Upsert(Row(name.data(),description.data(),value),false); } //721b0의 실제 false 인자를 통해 새 값을 반영한다.
void ServerList::Bind(MainLoopHooks& hooks) { hooks.reloadRootServers=[this] { Reload(); }; }
}
