// 큰 원본 부모의 인라인 본문도 같은 공통 하위와 효과 뒤 재조회 순서로 복원한다.
#include "o/RawDataManagerImage.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace netstorm::o {
namespace {
// 비정렬DWORD의 현재 네 바이트를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    // 원본 little endian 순서대로 네 바이트만 조합한다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);return value;
}
// 지정한DWORD만 쓰며 인접 보호 바이트를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    // 낮은 바이트부터 같은4BYTE 필드에 기록한다.
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 미연결 진단을 빈 성공으로 채우지 않는다.
void Assert(const DataManagerImageHooks& hooks,DataManagerImageAssertion kind) { if (!hooks.assertion) throw std::logic_error("DataManager 파일 이미지의 실제 진단 미연결");hooks.assertion(kind); }
// MarkStream은 첫16BYTE와 추가 mark DWORD가 모두 있어야 한다.
void Source(std::span<std::uint8_t> source) { if (source.size()<20) throw std::invalid_argument("DataManager 파일 이미지의 실제 MarkStream20 누락"); }
// 실제 접근할 범위만 빌리며0길이는 포인터 조회를 생략한다.
std::span<std::uint8_t> Range(const DataManagerImageHooks& hooks,std::uint32_t pointer,std::uint32_t position,std::uint32_t length) {
    if (!length) return {};if (!pointer || !hooks.streams.memory.buffer) throw std::logic_error("DataManager 파일 이미지의 실제 버퍼 미연결");auto bytes=hooks.streams.memory.buffer(pointer);
    if (position>bytes.size() || length>bytes.size()-position) throw std::out_of_range("DataManager 파일 이미지의 실제 버퍼 물리 범위");return bytes.subspan(position,length);
}
// 원본 getMark는 부호 있는16비트이며 길이 계산의 짧은 정수 wrap도 보존한다.
std::int32_t Mark(std::span<const std::uint8_t> source,const DataManagerImageHooks& hooks) {
    const auto bytes=Range(hooks,Read(source,0),Read(source,16),2);const auto value=static_cast<std::uint16_t>(bytes[0]|std::uint16_t(bytes[1])<<8);return std::bit_cast<std::int16_t>(value);
}
// 실제60BYTE 노드만 빌리고 잘못된NULL을 임의 새 노드로 대체하지 않는다.
std::span<std::uint8_t> Node(const DataManagerImageHooks& hooks,std::uint32_t handle) {
    if (!handle || !hooks.streams.selection.stream) throw std::logic_error("DataManager 파일 이미지의 실제 노드 미연결");auto bytes=hooks.streams.selection.stream(handle);if (bytes.size()<60) throw std::out_of_range("DataManager 파일 이미지의 실제 노드60 누락");return bytes;
}
// 004a8800/004a8850의 같은 SNART 표를 한 번 변환하고 지정 이름/NUL을 빌린다.
std::string_view Name(std::span<std::uint8_t> names,std::uint32_t index,const DataManagerImageHooks& hooks) {
    if (names.empty()) throw std::out_of_range("DataManager 파일 이미지의 실제 이름 표 누락");
    if (names[0]) {
        if (names[0]!='/') Assert(hooks,DataManagerImageAssertion::NameSeparator);const auto end=std::find(names.begin(),names.end(),std::uint8_t{});if (end==names.end()) throw std::out_of_range("DataManager SNART 입력 NUL 누락");*end=0xff;
        // 원본처럼 첫FF 전의 구분자만NUL로 바꾸고 나머지는 보존한다.
        for (std::size_t at=0;at<names.size() && names[at]!=0xff;++at) if (names[at]=='/') names[at]=0;
    }
    std::size_t at=1;
    // 지정한 개수의NUL을 지나되 다음FF를 만나면 원본처럼 그 위치에서 멈춘다.
    while (index) { if (at>=names.size()) throw std::out_of_range("DataManager SNART 이름 번호 범위");if (!names[at]) { --index;++at;if (at>=names.size()) throw std::out_of_range("DataManager SNART 끝 누락");if (names[at]==0xff) break; } else ++at; }
    if (at>=names.size()) throw std::out_of_range("DataManager SNART 이름 시작 범위");const auto tail=names.subspan(at);const auto end=std::find(tail.begin(),tail.end(),std::uint8_t{});if (end==tail.end()) throw std::out_of_range("DataManager SNART 이름 NUL 누락");return {reinterpret_cast<const char*>(tail.data()),static_cast<std::size_t>(end-tail.begin())};
}
}
// 진단이 mark/pos/end를 바꾸면 원본 본문처럼 그 현재 값을 후속 쓰기에 사용한다.
void AdvanceDataManagerMark(std::span<std::uint8_t> source,const DataManagerImageHooks& hooks) {
    Source(source);const auto mark=Read(source,16);if (mark!=0xffffffffU && Mark(source,hooks)!=std::bit_cast<std::int32_t>(Read(source,4)-mark)) Assert(hooks,DataManagerImageAssertion::MarkLength);
    Put(source,16,Read(source,4));if (std::bit_cast<std::int32_t>(Read(source,8))<std::bit_cast<std::int32_t>(Read(source,4))) Assert(hooks,DataManagerImageAssertion::MarkPosition);Put(source,4,Read(source,4)+2);
}
// 입력pos는payload 뒤에 갱신하고 빈 새 섹션이 기존 것을 덮지 않는 원본 병합 순서를 유지한다.
void ReadDataManagerImage(std::span<std::uint8_t> manager,std::span<std::uint8_t> source,const DataManagerImageHooks& hooks) {
    Source(source);if (manager.size()<12 || !hooks.names) throw std::invalid_argument("DataManager 파일 이미지의 manager/가상 이름 표 미연결");auto names=hooks.names();if (Read(source,4)!=2) Assert(hooks,DataManagerImageAssertion::SourcePosition);AdvanceDataManagerMark(source,hooks);
    // 마지막 빈 이름까지 원래 이름 표 순서대로 읽는다. 마지막 mark는end+2까지 진행할 수 있다.
    for (std::uint32_t index=0;;++index) {
        const auto name=Name(names,index,hooks);if (name.empty()) return;if (Read(source,16)==0xffffffffU) Assert(hooks,DataManagerImageAssertion::MissingMark);
        const auto narrow=static_cast<std::uint16_t>(static_cast<std::uint32_t>(Mark(source,hooks))-2);const auto signedLength=static_cast<std::int32_t>(std::bit_cast<std::int16_t>(narrow));const auto length=static_cast<std::uint32_t>(signedLength);
        if (!hooks.streams.allocateStream) throw std::logic_error("DataManager 파일 이미지의 실제 새 노드 할당 미연결");const auto stream=hooks.streams.allocateStream(60);if (stream) ConstructDataManagerStream(stream,name,signedLength,hooks.streams);auto target=Node(hooks,stream);
        const auto position=Read(source,4),pointer=Read(source,0),end=position+length;Put(source,4,end);if (std::bit_cast<std::int32_t>(Read(source,8))<std::bit_cast<std::int32_t>(end)) Put(source,8,end);
        WriteMemStream(target,Range(hooks,pointer,position,length),length,hooks.streams.memory);Put(target,4,0);
        const auto existing=FindDataManagerSection(manager,name,hooks.streams.selection);
        if (existing && (!Read(target,0) || !Read(target,8))) {
            DestroyMemStream(target,hooks.streams.list.lifetime.releaseBuffer);if (!hooks.streams.list.lifetime.releaseStream) throw std::logic_error("DataManager 빈 새 섹션의 실제 노드 반납 미연결");hooks.streams.list.lifetime.releaseStream(stream);
        } else {
            if (existing) RemoveDataManagerStream(manager,existing,hooks.streams.list);AppendDataManagerStream(manager,stream,hooks.streams.list);
        }
        AdvanceDataManagerMark(source,hooks);
    }
}
}
