// 파싱 객체의 사본 변경 대신 검사 소유 Fort의 실제 Chaff/Terr stream에 준비 항목만 삽입한다.
#pragma once
#include "o/Template.h"
#include "o/RiftType.h"
#include <algorithm>
#include <span>
#include <stdexcept>
#include <vector>

namespace netstorm::test::readyconstruction { // 실제 제품/원본 파일을 쓰지 않는 준비 입력 전용 도구다.
struct ReadySplice { // 원래 섹션의 count와 목록 끝만 바꾸며 객체/청크의 다른 바이트를 보존한다.
    std::size_t section{},count{},end{}; // source sections 인덱스, 내용물 개수 BYTE, 목록 끝 offset이다.
    std::uint8_t previous{}; // 스캔한 원래 count이며 적용 전에 같은 바이트인지 확인한다.
};
class ReadySectionScanner { // 004bdc60/004bd130의 원문 읽기 폭으로 삽입 위치만 찾는다.
public:
    ReadySectionScanner(const o::FortTemplate& fort,const o::RiftTypeTable& table,std::span<const std::uint8_t> bytes,std::size_t section,std::uint32_t factory,std::vector<ReadySplice>& patches)
        :fort_(fort),table_(table),bytes_(bytes),section_(section),factory_(factory),patches_(patches) {} // 원시 입력과 TYPE은 전체 스캔 동안 읽기 전용이다.
    void Scan() { // 빈/버전 전용 섹션은 그대로 두고 모든 기존 청크의 원문 범위를 검증한다.
        if (bytes_.empty()) return;const auto version=Byte();
        while (position_<bytes_.size()) { // 실제 저장 순서의 모든 청크를 소비한다.
            if (Byte()!=99) throw std::logic_error("준비 입력의 원시 청크 표지 손상");const auto count=Word();
            for (std::uint32_t index=0;index<count;++index) Object(version); // WORD 객체 수를 줄이거나 늘리지 않는다.
        }
    }
private:
    std::uint8_t Byte() { if (position_>=bytes_.size()) throw std::out_of_range("준비 입력의 원시 BYTE 소진");return bytes_[position_++]; } // 실제 저장 BYTE 하나만 소비한다.
    std::uint16_t Word() { const auto low=Byte();return static_cast<std::uint16_t>(low|(std::uint16_t(Byte())<<8)); } // 정렬/호스트 endian과 무관한 WORD다.
    const o::RiftTypeRecord& Type(std::uint32_t kind) const { const auto types=table_.Types();if (kind>=types.size()) throw std::out_of_range("준비 입력의 실제 TYPE 범위");return types[kind]; } // 실제 현재 TYPE 플래그로만 폭을 정한다.
    std::uint32_t Convert(std::uint8_t stored) const { const auto kind=fort_.ConvertType(stored);if (kind<0) throw std::out_of_range("준비 입력의 음수 타입 변환");static_cast<void>(Type(static_cast<std::uint32_t>(kind)));return static_cast<std::uint32_t>(kind); } // TypeNames의 저장 번호를 실제 타입으로 변환한다.
    void Quantities(std::uint32_t flags) { if (flags&o::TypeFlag1::kSaveQA) static_cast<void>(Byte());if (flags&o::TypeFlag1::kSaveQB) static_cast<void>(Word()); } // QA와 QB가 둘 다 있으면 원문처럼 둘 다 소비한다.
    void Contents(std::uint32_t parent,std::size_t depth=0,bool selected=false) { // 부모의 raw +10 TYPE을 사용하고 중첩 Form은 실제 내장6으로 취급한다.
        if (depth>bytes_.size()) throw std::out_of_range("준비 입력의 내용물 중첩 물리 범위");const auto countOffset=position_;const auto count=Byte();
        for (std::uint32_t index=0;index<count;++index) { // 기존 형제/하위 내용물을 건드리지 않고 끝 위치만 찾는다.
            const auto kind=Convert(Byte());if (!kind) throw std::logic_error("준비 입력의 불완전한 내용물 타입0");const auto& type=Type(kind);Quantities(type.flags1);
            if (!type.contentListFlags && !Type(parent).containerListFlags) static_cast<void>(Byte());
            if (type.flags1&o::TypeFlag1::kContainer) Contents(6,depth+1); // 원문004afa70은 저장 종류와 달리 실제 자식을 ContentForm6으로 만든다.
        }
        if (selected) { if (count==255) throw std::out_of_range("준비 입력의 내용물 count255 상한");patches_.push_back({section_,countOffset,position_,count}); }
    }
    void Object(std::uint8_t version) { // position/storedType와 판본별 선택 필드를 같은 순서로 건너뛴다.
        static_cast<void>(Byte());const auto stored=Byte();const auto kind=stored>=0xf6 ? table_.BridgeType() : Convert(stored);
        if (!kind) { if (Byte()) throw std::logic_error("준비 입력의 타입0 추가 BYTE 손상");return; }
        if (kind==static_cast<std::uint32_t>(table_.IslandType())) { static_cast<void>(Byte());return; } //검증된 타입 번호의 unsigned 폭을 맞춘다.
        const auto& type=Type(kind);if (type.flags1&o::TypeFlag1::kSaveFrame) static_cast<void>(Byte());Quantities(type.flags1);
        if (type.flags2&o::TypeFlag2::kBridge) static_cast<void>(Byte());if (!version) static_cast<void>(Byte());
        if (version>1 && (type.flags2&o::TypeFlag2::kFactory)) static_cast<void>(Byte());if (version && (type.flags2&o::TypeFlag2::kOwnerSavedMask)) static_cast<void>(Byte());
        if (type.flags1&o::TypeFlag1::kContainer) Contents(kind,0,kind==factory_);
    }
    const o::FortTemplate& fort_; // 저장 타입 변환표만 빌린다.
    const o::RiftTypeTable& table_; // 조건 필드의 원문 폭/부모 목록 플래그다.
    std::span<const std::uint8_t> bytes_; // 하나의 원래 Chaff/Terr 섹션 물리다.
    std::size_t section_{},position_{}; // 현재 섹션 번호와 실제 원시 커서다.
    std::uint32_t factory_{}; // 검사 입력을 추가할 실제 부모 종류다.
    std::vector<ReadySplice>& patches_; // 모든 검증 성공 전까지 적용하지 않는 삽입 계획이다.
};
inline std::vector<std::uint8_t> ReadyPayload(const o::FortTemplate& fort,const o::RiftTypeTable& table,std::uint32_t factory,std::uint32_t ready) { // 저장 번호 역변환과 실제 listFlags 생략 조건으로 새 항목 하나만 만든다.
    const auto types=table.Types();if (factory>=types.size() || ready>=types.size()) throw std::out_of_range("준비 입력의 부모/내용물 TYPE 범위");
    std::uint32_t stored=246;for (std::uint32_t index=0;index<246;++index) if (fort.ConvertType(static_cast<std::uint8_t>(index))==static_cast<int>(ready)) { stored=index;break; } // 현재 번호를 저장 번호로 가정하지 않는다.
    if (stored==246) throw std::logic_error("준비 입력75의 저장 타입 역변환 부재");const auto& type=types[ready];std::vector<std::uint8_t> payload{static_cast<std::uint8_t>(stored)};
    if (type.flags1&o::TypeFlag1::kSaveQA) payload.push_back(0);if (type.flags1&o::TypeFlag1::kSaveQB) { payload.push_back(0);payload.push_back(0); }
    const auto required=type.contentListFlags ? type.contentListFlags : types[factory].containerListFlags;
    if (!required) payload.push_back(0x10);else if (!(required&0x10U)) throw std::logic_error("준비 입력 부모/내용물의 실제 목록은10이 아님"); // 부모124가10이면 저장 flags BYTE를 추가하면 안 된다.
    if (type.flags1&o::TypeFlag1::kContainer) payload.push_back(0);return payload; // 새 항목의 내용물이 있으면 실제 빈 count만 기록한다.
}
inline std::size_t AddReadyConstructionInput(o::FortTemplate& fort,const o::RiftTypeTable& table,std::uint32_t factory,std::uint32_t ready=75) { // 모든 원시 검증 뒤에만 입력 사본을 교체하고 파싱 보조 자료도 새 원문에서 갱신한다.
    const auto payload=ReadyPayload(fort,table,factory,ready);std::vector<ReadySplice> patches;
    for (std::size_t section=0;section<fort.sections.size();++section) { // Chaff4와 Terr15..34만 스캔하며 다른 표준 섹션은 수정하지 않는다.
        if (section!=4 && (section<15 || section>=35)) continue;ReadySectionScanner(fort,table,fort.sections[section],section,factory,patches).Scan();
    }
    if (patches.empty()) throw std::logic_error("준비 입력의 실제 원시 공장 부재");auto candidate=fort;
    for (auto patch=patches.rbegin();patch!=patches.rend();++patch) { // 같은 섹션의 뒤 offset부터 삽입해 앞서 기록한 위치를 보존한다.
        auto& bytes=candidate.sections.at(patch->section);if (patch->count>=bytes.size() || patch->end>bytes.size() || patch->count>=patch->end || bytes[patch->count]!=patch->previous) throw std::logic_error("준비 입력 원시 splice 위치 불일치");
        bytes[patch->count]=static_cast<std::uint8_t>(patch->previous+1U);bytes.insert(bytes.begin()+static_cast<std::ptrdiff_t>(patch->end),payload.begin(),payload.end());
    }
    candidate.RefreshObjectRecords(table);fort=std::move(candidate);return patches.size(); // 손상/할당/재파싱 실패는 호출자의 원래 sourceFort 사본을 바꾸지 않는다.
}
}
