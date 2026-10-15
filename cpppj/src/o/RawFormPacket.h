// 원본 Form 가상+54와 공통 삭제 가상+58의 실제 패킷을 같은 raw 슬롯에서 작성한다.
#pragma once
#include "o/SidPool.h"

namespace netstorm::o {
// 원본 등록 표에서 writer가 호출마다 읽는 헤더 필드다. length는 반환 크기와 별개다.
struct FormPacketHeader {
    std::uint8_t message{},protocol{1}; // 메시지 BYTE와 bit7을 지우기 전의 프로토콜 BYTE다.
    std::uint16_t length{}; // 원본 헤더의 선언 길이다. 4보다 작으면4로 기록한다.
};
// 두 PE의 초기 메시지/프로토콜/길이는 같다. 현재 값은 세션이 소유한다.
struct FormPacketHeaders {
    FormPacketHeader dependency{0x65,1,13}; // 감시 Form의 SID/부모/상태/상대 Form 두 개다.
    FormPacketHeader content{0x2d,1,18}; // 내용물 Form의 WORD 필드와 판본별 추가 WORD/순번 BYTE다.
    FormPacketHeader deletion{0x2e,1,8}; // 공통 삭제 SID/플래그/최근 표식이다.
};
// 삭제 writer가 한 번 소비하는 원본005c845c/60 ↔ CD005395d4/d8 전역이다.
struct FormDeletionStamp {
    std::uint32_t sid{},token{}; // SID DWORD가 일치하면 sid만0으로 지우고 token의 낮은 WORD를 OR한다.
};
// 순수 직렬화 몸체다. pool/headers/stamp는 이 객체보다 오래 살아야 한다.
class RawFormPacket {
public:
    // 장치/송수신을 열지 않고 같은 Pool과 현재 헤더/삭제 표식을 빌린다.
    RawFormPacket(const SidPool& pool,const FormPacketHeaders& headers,FormDeletionStamp& stamp);
    // 실제 Depend/Content VT만 선택한다. 두 writer의 미사용 두 번째 인자는 결과에 영향을 주지 않는다.
    // 반환값은13/18이며 버퍼의 그 뒤 바이트는 보존한다. 미복원 다른 VT는 거부한다.
    std::uint16_t WriteData(Sid form,std::span<std::uint8_t> packet) const;
    // 실제 공통+58에 도달한 SID를 쓴다. flags>>16의fff0과 최근 표식 WORD를 합친다.
    // 서버 번호/버퍼를 먼저 검사하며 정상 반환은8이다. 호출자가 파생+58 분배를 확인해야 한다.
    std::uint16_t WriteDeletion(Sid sid,std::uint32_t flags,std::span<std::uint8_t> packet) const;
private:
    std::uint16_t Parent(Sid form) const; // 실제 Form VT의+0 getter와 같은+10 WORD다. 상태 필터는 추가하지 않는다.
    void ServerSid(std::uint16_t sid) const; // 원본의 client SID5..serverFirst-1 assert 경로를 거부한다.
    const SidPool& pool_; // 같은 세션의 단일 실제 raw 슬롯 저장소다.
    const FormPacketHeaders& headers_; // 요청 중에도 현재 선언 헤더를 직접 읽는다.
    FormDeletionStamp& stamp_; // 다른 삭제 요청과 공유하는 한 번 소비 표식이다.
};
}
