// 10.78 파일 이미지의 같은 manager/MarkStream20/이름 표를 실제 섹션 목록에 읽는다.
#pragma once
#include "o/RawDataManagerResize.h"
namespace netstorm::o {
// 진단 뒤 계속하는 원본 위치를 구별하며 물리 범위 초과는 별도로 거부한다.
enum class DataManagerImageAssertion { SourcePosition,MissingMark,MarkLength,MarkPosition,NameSeparator };
// 가상 이름 표와 같은 노드/버퍼의 실제 경계를 제공한다.
struct DataManagerImageHooks {
    DataManagerResizeHooks streams; // 생성/검색/제거/압축은 같은 실제 노드와 버퍼를 빌린다.
    std::function<std::span<std::uint8_t>()> names; // manager 가상+4가 반환한 같은 변경 가능한 SNART 이름 표다.
    std::function<void(DataManagerImageAssertion)> assertion; // 해당 원본 진단이 선택된 위치에서만 호출한다.
};
// 0044cdb0: 이전 mark 길이 확인→현재 pos를 mark로 기록→signed pos/end 진단→pos+2다.
void AdvanceDataManagerMark(std::span<std::uint8_t> source,const DataManagerImageHooks& hooks);
// 0044d380: 이름 표 순회/생성/복사/빈 섹션 유지/기존 제거/추가/mark 진행을 수행한다.
void ReadDataManagerImage(std::span<std::uint8_t> manager,std::span<std::uint8_t> source,const DataManagerImageHooks& hooks);
}
