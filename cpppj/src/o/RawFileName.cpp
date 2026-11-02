// 경로 정규화 API 대신 원문 각 부품의 선행/후행 처리와 현재 출력만 사용한다.
#include "o/RawFileName.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace netstorm::o { // 원본 전체459c60의 정상 물리 범위와 throw 위치를 보존한다.
std::uint32_t RawFileName::Index() const { return index_; } // 조회는 고리를 전진시키지 않는다.
std::span<const std::uint8_t,256> RawFileName::CurrentBytes() const { return buffers_[index_]; } // 현재 한 칸의 실제 바이트다.
// 원문은 전체 FileSpec을 먼저 복사하고 UNC의 서버 끝/colon 다음 또는 첫BYTE에만 NUL을 쓴다.
std::string_view RawFileName::Device(std::string_view fileSpec,char separator) {
    index_=(index_+1U)%10U;auto& bytes=buffers_[index_];bytes.fill(0);fileSpec=fileSpec.substr(0,fileSpec.find('\0'));
    if (fileSpec.size()>=bytes.size()) throw std::length_error("원문 FileSpec 장치 조회의256BYTE 물리 범위 초과");
    std::copy(fileSpec.begin(),fileSpec.end(),bytes.begin());auto size=fileSpec.size();
    if (size>=2 && fileSpec[0]==separator && fileSpec[1]==separator) {
        const auto end=fileSpec.find(separator,2);if (end!=std::string_view::npos) { bytes[end]=0;size=end; }
    } else {
        const auto colon=fileSpec.find(':');size=colon==std::string_view::npos ? 0 : colon+1;bytes[size]=0;
    }
    return {reinterpret_cast<const char*>(bytes.data()),size};
}
std::string_view RawFileName::Name(std::string_view fileSpec,bool extension,char separator) { // 원문은 파일명만 옮겨 쓰지 않고 전체 입력의 고리 내부 포인터를 반환한다.
    index_=(index_+1U)%10U;auto& bytes=buffers_[index_];bytes.fill(0);fileSpec=fileSpec.substr(0,fileSpec.find('\0'));
    if (fileSpec.size()>=bytes.size()) throw std::length_error("원문 FileSpec 이름 조회의256BYTE 물리 범위 초과");
    std::copy(fileSpec.begin(),fileSpec.end(),bytes.begin());const auto end=fileSpec.size();std::size_t offset=end;bool found{};
    while (offset>0) { --offset;if (bytes[offset]==static_cast<std::uint8_t>(separator) || bytes[offset]==':') { ++offset;found=true;break; } } // 원문 마지막 구분자/colon을 뒤에서 찾아 그 다음 주소를 빌린다.
    if (!found) { bytes[0]=0;return {reinterpret_cast<const char*>(bytes.data()),0}; }
    auto size=end-offset;
    if (!extension && size>1) {
        for (auto at=end-1;at>offset;--at) { //459550은 선두 dot을 확장자로 지우지 않으며 구분자/colon을 만나면 중단한다.
            if (bytes[at]=='.') { bytes[at]=0;size=at-offset;break; }
            if (bytes[at]==static_cast<std::uint8_t>(separator) || bytes[at]==':') break;
        }
    }
    return {reinterpret_cast<const char*>(bytes.data()+offset),size};
}
std::string_view RawFileName::Format(std::span<const FileNamePart> parts,char separator) {
    index_=(index_+1U)%10U;auto& bytes=buffers_[index_];bytes.fill(0);std::size_t size{};
    const auto append=[&](std::string_view text) { // 원문 memcpy의 전체 NUL 포함 폭을 확인하고 부분 문자열로 성공시키지 않는다.
        if (size+text.size()>=bytes.size()) throw std::length_error("원문 경로 작성기의256BYTE 물리 범위 초과");
        std::copy(text.begin(),text.end(),bytes.begin()+static_cast<std::ptrdiff_t>(size));size+=text.size();bytes[size]=0;
    };
    const auto last=[&] { return size ? static_cast<char>(bytes[size-1]) : '\0'; }; // 정상 고리의 이전 칸 끝 BYTE도0이므로 빈 출력의 원문[-1]과 같다.
    std::int32_t previous{};bool firstDirectory=true;
    for (const auto& part:parts) { // 종료0 뒤의 text는 조회하지 않으며 각 문자열은 해당 처리 직전에 복사한다.
        if (!part.kind) break;
        auto text=std::string(part.text.substr(0,part.text.find('\0'))); // 고리 별칭이 이번 선행 clear로 빈 문자열이 됐을 수도 있다.
        if (text.size()>=256) throw std::length_error("원문 경로 작성기의 지역256BYTE 입력 범위 초과");
        for (auto& byte:text) if (byte=='/' || byte=='\\') byte=separator; // 원문은 두 종류의 구분자를 현재 BYTE로 치환한다.
        text.resize(text.find('\0')==std::string::npos ? text.size() : text.find('\0'));
        const auto stripLeading=[&](char byte) { text.erase(0,text.find_first_not_of(byte)); }; // 같은 문자 선행 연속분만 제거한다.
        const auto stripOneTrailing=[&] { if (text.size()>1 && text.back()==separator) text.pop_back(); }; // 원문 slash 루프는 포인터를 증가시켜 마지막 한 문자만 지운다.
        switch (part.kind) {
        case -1:
            if (previous) throw std::invalid_argument("원문 경로 루트는 첫 부품이어야 함");
            stripOneTrailing();append(text);
            if (!text.empty() && text.front()!=separator && text.back()!=':') append(":");
            break;
        case -2:
            if (previous==-3 || previous==-4) throw std::invalid_argument("원문 파일/확장자 뒤 디렉터리 부품");
            if (!firstDirectory) stripLeading(separator);firstDirectory=false;stripOneTrailing();
            if (!previous) {
                if (text.find(separator)==std::string::npos && text.find(':')==std::string::npos) append(".\\");
            } else if (size && last()!=separator && (text.empty() || text.front()!=separator)) append("\\");
            append(text);if (last()!=separator) append("\\");
            break;
        case -3:
            if (previous==-3 || previous==-4) throw std::invalid_argument("원문 파일/확장자 뒤 파일 부품");
            stripLeading(separator);stripOneTrailing();
            if (!previous) { if (text.find(separator)==std::string::npos) append(".\\"); }
            else if (last()!=separator) append("\\");
            append(text);break;
        case -4:
            if (!previous || previous==-1 || previous==-2) throw std::invalid_argument("원문 파일 없는 확장자 부품");
            stripLeading('.');
            while (text.size()>1 && text.back()=='.') text.pop_back(); // 확장자의 실제 후행 dot 루프는 감소하므로 전부 제거한다.
            if (last()!='.') append(".");append(text);break;
        default:throw std::invalid_argument("원문 경로 작성기의 알 수 없는 부품 종류");
        }
        previous=part.kind;
    }
    return {reinterpret_cast<const char*>(bytes.data()),size};
}
}
