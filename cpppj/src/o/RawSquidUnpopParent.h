// 원문 공통 공간 해제의 현재 진단과 이미 실행한 부분 변경을 보존한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include "o/SquidHash.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 실제 풀/해시/점유 지도 위에서 원문 순서를 실행한다.
struct SquidUnpopDiagnostics { // 실제 World는 같은 Client owner와 전체 TYPE 표를 연결한다.
    RawCheckingFlag checking; // 원문 진단 위치마다 현재 DWORD를 읽는다.
    std::span<const RiftTypeRecord> types; // 현재 raw TYPE의 레코드를 같은 표에서 재조회한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 정상 복귀하면 저장값/현재값을 구분해 본문을 계속한다.
};
struct SquidUnpopParentHooks { // 큰 실제 하위 몸체를 선택한 순간에만 요구한다.
    std::function<const RiftTypeRecord&(std::uint8_t)> type; // 초기 저장 TYPE 포인터와 현재 TYPE 재조회에 함께 쓴다.
    std::function<void(Sid,std::uint32_t)> miniMap; // void 설정 뒤 기존00481510의 실제 지도 통지다.
    std::function<void(int,int)> side; // 같은 표면 WORD/실제 건물 옆면과 통지 몸체다.
    std::function<void(Sid)> contained; // 실제004ac6b0→004ad230의 분리/내용물 UI 종단이다.
    std::function<void(Sid,std::uint32_t)> update; // 현재 가상88/flags2000의8c 표시 갱신이다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 식/파일/줄 번호를 보존한다.
};
// 물리 주소 대신 같은 SID 슬롯을 사용하며 삭제 전 사전 전수검사로 부분 효과를 생략하지 않는다.
void RunSquidUnpopParent(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,Sid sid,
    std::uint32_t flags,RawCheckingFlag checking,const SquidUnpopParentHooks& hooks);
}
