// 원문 마지막BYTE 제거/CR 부분 이동·줄 첫 글자·NUL 쓰기와 언어별 병합을 보존한다.
#include "o/RawXlat.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace netstorm::o { // 보호 파일에는 쓰지 않고 Client 소유의 읽은 버퍼만 수정한다.
namespace { // 원문 strchr/strlen이 허용한 실제 C문자열 범위만 사용한다.
std::size_t End(std::span<const std::uint8_t> bytes,std::size_t start) { //현재 NUL을 물리 안에서 찾는다.
    if (start>=bytes.size()) throw std::out_of_range("Xlat 실제 문자열 시작 범위");const auto end=std::find(bytes.begin()+start,bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("Xlat 실제 문자열 NUL 범위");return static_cast<std::size_t>(end-bytes.begin());
}
std::optional<std::size_t> Next(std::span<const std::uint8_t> bytes,std::size_t start) { //strchr LF 뒤 주소이며 LF가 없으면 실제NULL이다.
    const auto end=End(bytes,start);const auto line=std::find(bytes.begin()+start,bytes.begin()+end,std::uint8_t{10});return line==bytes.begin()+end ? std::nullopt : std::optional<std::size_t>(static_cast<std::size_t>(line-bytes.begin())+1);
}
std::string Text(std::span<const std::uint8_t> bytes,std::size_t start) { const auto end=End(bytes,start);return {reinterpret_cast<const char*>(bytes.data()+start),end-start}; } //NUL 뒤를 키/번역에 포함하지 않는다.
}
// 파일별 버퍼를 설치한 뒤 같은 원본 레코드의 다른 여섯 언어 칸은 건드리지 않는다.
void RawXlat::Load(std::uint32_t which,std::span<const std::uint8_t> source) {
    auto& storage=buffers_.at(which);if (storage) throw std::logic_error("Xlat textBuffer[which] == NULL 원문 진단"); //원문 재적재 진단 뒤 해제된 옛 키 주소의 동작을 정상 경로로 합성하지 않는다.
    if (source.empty()) throw std::out_of_range("Xlat 원문 길이0의 마지막BYTE 물리 범위");storage.emplace(source.begin(),source.end());
    NormalizeFileBuffer(which);ParseFileBuffer(which);
}
// 현재 textBuffer 슬롯의 실제 설치 여부를 읽으며 빈 설치도 미설치와 구별한다.
bool RawXlat::HasBuffer(std::uint32_t which) const { return buffers_.at(which).has_value(); }
std::span<std::uint8_t> RawXlat::AllocateFileBuffer(std::uint32_t which,std::uint32_t length) { // 할당이 실패하면 아직 설치하지 않은 슬롯은 비어 있다.
    auto& storage=buffers_.at(which);if (storage) throw std::logic_error("Xlat 기존 버퍼 재사용의 원문 미정 포인터 경계");storage.emplace(length);return *storage;
}
void RawXlat::NormalizeFileBuffer(std::uint32_t which) { // 끝NUL/CR 정리는 같은 원래 크기의 버퍼에만 기록한다.
    auto& storage=buffers_.at(which);if (!storage || storage->empty()) throw std::out_of_range("Xlat 원문 길이0의 마지막BYTE 물리 범위");auto& bytes=*storage;bytes.back()=0;
    for (std::size_t at=0;at<bytes.size() && bytes[at];) { //원문은 파일 전체 크기로 memmove하고 이동 후 같은 위치를 다시 검사한다.
        if (bytes[at]==13) std::memmove(bytes.data()+at,bytes.data()+at+1,bytes.size()-at-1);else ++at;
    }
}
void RawXlat::ParseFileBuffer(std::uint32_t which) { // 같은 기존 레코드 파서를 File 소멸 뒤의 원문 위치에서 실행한다.
    auto& storage=buffers_.at(which);if (!storage || storage->empty()) throw std::out_of_range("Xlat 실제 파싱 버퍼 누락");auto& bytes=*storage;
    std::optional<std::size_t> cursor=0;
    while (cursor && bytes[*cursor]) { //각004de260은 실제 이전 cursor부터 새 레코드를 읽는다.
        std::uint32_t state{};std::optional<std::size_t> key,value;bool complete{};
        while (cursor && bytes[*cursor] && !complete) { //구분자는 줄 첫BYTE에서만 검사하며 불완전한 마지막 레코드는 저장하지 않는다.
            const auto at=*cursor;const auto next=Next(bytes,at);const auto first=bytes[at];
            if (!state && first=='*') { key=next;state=1; }
            else if (state==1 && first=='-') { if (!at) throw std::out_of_range("Xlat 원문 키 앞BYTE 쓰기");bytes[at-1]=0;bytes[at]=0;value=next;state=2; }
            else if (state==2 && first=='=') { if (!at) throw std::out_of_range("Xlat 원문 번역 앞BYTE 쓰기");bytes[at-1]=0;bytes[at]=0;complete=true; }
            cursor=next;
        }
        if (complete) { if (!key || !value) throw std::out_of_range("Xlat 원문 완성 레코드의NULL 주소");records_[Text(bytes,*key)][which]=Text(bytes,*value); }
    }
}
// 빈 번역은 등록된 비NULL 포인터이므로 원문 대신 빈 문자열을 돌려준다.
std::string_view RawXlat::Translate(std::string_view original,std::int32_t language) const {
    if (language==0 || language==1) return original;
    const auto key=original.substr(0,original.find('\0'));const auto found=records_.find(key);if (found==records_.end()) return original;
    if (language<0 || language>=7) throw std::out_of_range("Xlat 번역 언어 칸의 물리 범위");const auto& text=found->second[static_cast<std::size_t>(language)];return text ? std::string_view(*text) : original;
}
// 실제 파일 크기를 유지한 NUL/CR 이동 결과를 빌린다.
std::span<const std::uint8_t> RawXlat::Buffer(std::uint32_t which) const { const auto& value=buffers_.at(which);return value ? std::span<const std::uint8_t>(*value) : std::span<const std::uint8_t>{}; }
std::size_t RawXlat::Size() const { return records_.size(); } //원문 해시 충돌과 무관한 정확 키 개수다.
// 원문은 일부 파일 부재로 전체 초기화를 취소하거나 같은 언어로 다른 파일을 대신 읽지 않는다.
void InitializeRawXlat(RawXlat& xlat,const RawXlatStartupHooks& hooks) {
    constexpr std::array<std::pair<std::uint32_t,std::string_view>,3> kSources{{{3,"d/xlat.german"},{1,"d/xlat.english"},{6,"d/xlat.portuguese"}}}; //004dea00의 고정 순서다.
    for (const auto& [which,path]:kSources) { //각 파일의 존재/읽기/병합 뒤 다음 파일로 간다.
        if (!hooks.exists) throw std::logic_error("Xlat 실제 파일 존재 장치 미연결");
        if (!hooks.exists(path)) { if (!hooks.missing) throw std::logic_error("Xlat 실제 부재 로그 미연결");hooks.missing(path);continue; }
        if (!hooks.read) throw std::logic_error("Xlat 실제 파일 읽기 미연결");const auto bytes=hooks.read(path);xlat.Load(which,bytes);
    }
}
}
