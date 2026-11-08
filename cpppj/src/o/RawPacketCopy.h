// 4dece0의 observer 뒤 실제 길이/순방향 DWORD·BYTE 복사와 소유 복제 token을 복원한다.
#pragma once
#include "o/RawPacketDispatch.h"
#include <map>
#include <vector>

namespace netstorm::o { //호스트 포인터를 DWORD로 자르거나 packet/observer 사본을 만들지 않는다.
struct RawPacketCopyResult { //원문 반환 포인터의 NULL과 길이0의 소유 할당을 구별한다.
    bool present{}; //실제 반환 주소가 있는지이며 빈 span과 같은 뜻이 아니다.
    std::span<std::uint8_t> bytes; //같은 원문 길이의 실제 목적지이며 외부 목적지도 별도 복사하지 않는다.
    std::shared_ptr<std::vector<std::uint8_t>> allocation; //자동 할당 분기만 갖는 실제 전체 바이트의 소유자다.
};
RawPacketCopyResult DuplicateRawPacket(std::span<std::uint8_t> packet,std::int32_t capacity,std::span<std::uint8_t> destination,RawPacketDispatch& dispatch); //NULL 입력은 조기 반환하고 observer(1)→현재 WORD→선택적 할당/작은 용량 거부→앞방향 복사를 수행한다.
class RawPacketCopies final { //Client 수명의 실제 복제본과 State+80의 포인터 대용 token을 보관한다.
public:
    RawPacketCopies()=default; //최초 token1만 준비하며 파일/소켓/할당 패킷은 만들지 않는다.
    RawPacketCopies(const RawPacketCopies&)=delete;RawPacketCopies& operator=(const RawPacketCopies&)=delete; //같은 Native 포인터의 소유권/해제 장부를 복제하지 않는다.
    std::uint32_t Duplicate(std::span<std::uint8_t> packet,RawPacketDispatch& dispatch); //원문4dece0(packet,0,NULL)의 실제 소유 할당만 장부에 넣고 NULL은0으로 반환한다.
    std::uint32_t Adopt(std::vector<std::uint8_t> packet); //이미 소유한 일반 LAN Zone의 같은 byte 할당을 이동하며 새 observer나 내용 복제를 더하지 않는다.
    std::shared_ptr<std::vector<std::uint8_t>> Acquire(std::uint32_t token) const; //현재 token의 같은 전체 바이트를 잠깐 pin한다. 실패를 빈 Zone으로 바꾸지 않는다.
    void Release(std::uint32_t token); //NULL0은 무시하고 현재 token만 반납한다. 다른 State 필드는 호출자가 원문 위치에서 지운다.
private:
    std::uint32_t Hold(std::shared_ptr<std::vector<std::uint8_t>> allocation); //완성된 실제 할당의 소유권을 성공적으로 장부에 넣은 뒤에만 token을 공개한다.
    std::map<std::uint32_t,std::shared_ptr<std::vector<std::uint8_t>>> copies_; //미소비/덮어쓴 Native 포인터의 물리도 Client 최종 반납까지 보유한다.
    std::uint32_t nextToken_{1}; //0은 원문 NULL이며 재사용하지 않는 DWORD opaque 별칭이다.
};
}
