// Factory와 전체 지식 창이 공유하는 원본70바이트 행의 의미 필드와 CRT 정렬이다.
#pragma once
#include "o/RiftType.h"
#include <span>
#include <string>
namespace netstorm::client {
// 원본 행의 번호/사용 가능/생산 중/마나/번역 설명이며 포인터나 원문 메모리 배치는 호스트에 노출하지 않는다.
struct KnowledgeRecord {
    std::uint32_t type{}; // +0의 실제 타입 번호다.
    bool available{},inProduction{}; // +8/+c의 원본 비제로 판정이다.
    std::string mana,description; // +14/+1e부터 첫NUL까지의 원본 글꼴 바이트다.
};
// 00453f70: 첫 공통 우선 flags2 분류 뒤 설명을 C 로캘 ASCII로 비교한다.
int CompareKnowledgeRecords(const KnowledgeRecord& first,const KnowledgeRecord& second,std::span<const o::RiftTypeRecord> types);
// 004e6010/shortsort의 피벗·동률 교환을 기존 Factory 구현에서 그대로 공유한다.
void SortKnowledgeRecords(std::span<KnowledgeRecord> entries,std::span<const o::RiftTypeRecord> types);
}
