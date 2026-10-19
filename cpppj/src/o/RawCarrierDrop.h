// 원본427070 요청·42aa50 명령·444bc0 서버 수신의 Cast/Drop 제어 흐름이다.
#pragma once
#include "o/RawContainedFinder.h"
#include "o/RawFormPacket.h"
#include <functional>

namespace netstorm::o {
// 실제 Session 전역이며 외부 효과 뒤 다시 읽는 값은 참조 상태에 남긴다.
struct CarrierDropState {
    FormPacketHeader request{0x50,1,9},activation{0x5c,1,6}; // 각각40a9e0/40ac60이 읽는 현재 헤더다.
    std::uint32_t localOwner{1},server{1},checking{},numTypes{188}; // 현재 소유자/서버/강한 진단/TYPE 개수다.
    std::uint32_t contentType{6},notifyProcessType{51},queueCommand{0x030000b2}; // 현재 내용물/통지 프로세스/입력 큐 토큰이다.
    std::uint32_t selectCommand{0x03000073},castCommand{0x0300011a},dropCommand{0x03000076},pickupCommand{0x03000075}; // 실제 메뉴 분배 토큰이다.
    std::int32_t mapLimit{256}; //40e800이 비교하는 현재 지도 한계다.
};
// 큰 월드/프로세스/장치 효과는 실제 하위 몸체로 연결하며 선택된 경계 누락을 성공으로 처리하지 않는다.
struct CarrierDropHooks {
    std::function<bool(Sid)> immobile; //492090의 현재 지면 정지 판정이다.
    std::function<std::int32_t(std::uint32_t)> cost; //41df60과 같은 현재 TYPE별 복호화 비용 단위다.
    std::function<bool(float)> afford; //425e90의 현재 로컬 비용 승인이다.
    std::function<void(std::uint32_t,float)> subtract; //41dff0의 로컬 SP 증감이다.
    std::function<std::array<std::int32_t,2>(std::array<float,2>)> project; //497220의 현재 카메라 투영이다.
    std::function<void(std::int32_t,std::array<std::int32_t,2>,std::uint32_t)> popup; //43dd20의 비용/화면/마지막0이다.
    std::function<void(Sid,float)> regular; //425ef0의493/지연0.1 예약이다.
    std::function<void(Sid,const std::function<void(Sid)>&)> attached; //425e60/4b1b80의 현재 부착 체인을 순서대로 순회한다.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> notify; //4ae2d0 조회 뒤 프로세스 가상+1c 통지다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> select; //4d5c90의 실제 선택/해제다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //4e2dc0의 서버 요청 송신이다.
    std::function<void(Sid)> pickup; //428e00의 실제 포획 요청이다.
    std::function<std::uint32_t(Sid,std::uint32_t,Sid)> baseMenu; //다른 메뉴 명령의 공통4ae010이다.
    std::function<void(std::uint32_t)> assertion; //서버/내용물 타입 범위의 복귀 가능한 원본 진단이다.
    std::function<std::array<float,2>(Sid,std::uint32_t)> dropPoint; //426a70의 현재 내려놓기/시전 위치다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue; //452fe0(현재명령,내용물kind,0)이다.
    std::function<void(std::uint32_t,std::uint32_t)> badOwner; //442a40(kind,송신자)의 거부 통지다.
    std::function<float(std::uint32_t)> money; //40ef30의 현재 소유자 SP다.
    std::function<void(std::uint32_t,std::uint32_t,std::array<float,2>,std::uint32_t,std::uint32_t)> refund; //442b50의 실패 비용 통지이며 뒤쪽 생성 처리를 중단하지 않는다.
    std::function<std::uint32_t(std::uint32_t)> movement; //488fe0의 현재 부모 타입 경로 종류다.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> removeMovement; //4b1a90(parent,종류,0,1)이다.
    std::function<void(Sid,std::uint32_t)> detach,unpop,destroy; //각각 부모+48/+4c와 내용물+10의 flags0 호출이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; //4af530(kind,0)의 실제 주문 생성이다.
    std::function<void(Sid,std::uint32_t)> owner,argument; //현재 가상+74와4ac220의 QA WORD 쓰기다.
    std::function<void(Sid,std::array<float,2>,std::uint32_t)> pop; //새 주문의 가상+90(x,y,0)이다.
    std::function<void(std::uint32_t,std::uint32_t,std::span<const std::uint8_t>)> activate; //41e860(9,0,packet)의 실제 로컬 후속 수신이다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; //로컬 후속 수신 뒤4e2e30으로 배포한다.
    std::function<bool(std::uint32_t,std::array<float,2>,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,double,std::uint32_t)> place; //444760의 일반 배치(kind,x,y,owner,QA,0,1,0.0,0)다.
    std::function<bool(Sid)> standing; //40f100의 현재 부모 정지 상태다.
    std::function<void(Sid,float,std::int32_t)> escape; //429010(parent,1.0,-1)의 실제 회피다.
};
// 같은 Pool/TYPE/contained 상태를 빌리며 요청에서 수신이나 주문 효과를 직접 대체하지 않는다.
class RawCarrierDrop {
public:
    RawCarrierDrop(SidPool& pool,std::span<const RiftTypeRecord> types,const ContainedFinderState& contained,CarrierDropState& state,CarrierDropHooks hooks); //10.78의188 TYPE/50BYTE 슬롯만 사용한다.
    void Request(Sid parent,Sid content={}) const; //선택0은 첫 비주문 내용물을 찾아9바이트 요청을 보낸다.
    std::uint32_t Command(Sid parent,std::uint32_t code,Sid listParent) const; //Cast는 최초 내용물SID를 보관한 뒤 플래그를 검색하는 원본 순서다.
    void Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const; //0x50 서버 수신과0x5c 후속 배포를 수행한다.
    const SidPool& Pool() const; //서로 다른 Session의 하위 연결을 검사할 때 사용한다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; //현재 물리 슬롯의 지정 폭만 읽는다.
    std::uint32_t Genus(Sid sid) const; //현재 자산 TYPE의 flags2다.
    std::uint32_t Kind(Sid form) const; //현재 내용물의+18 WORD다.
    std::uint32_t KindGenus(Sid form) const; //효과 뒤 다시 읽은 현재 내용물 kind의flags2다.
    void Mark(Sid form,std::uint8_t value) const; //원본처럼 free 여부를 추가 검사하지 않고 내용물+22를 쓴다.
    SidPool& pool_; //원본 물리 상태를 공유하는 같은 저장소다.
    std::span<const RiftTypeRecord> types_; //현재 TYPE 표이며 복제하지 않는다.
    const ContainedFinderState& contained_; //현재 종속 탐색기의 공통 전역이다.
    CarrierDropState& state_; //효과 사이에 다시 읽는 실제 Session 값이다.
    CarrierDropHooks hooks_; //도달한 큰 하위의 실제 구현이다.
};
}
