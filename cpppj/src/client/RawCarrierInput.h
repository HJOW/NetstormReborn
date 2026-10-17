// 원본 Carrier.cpp의 커서 평가·강조·클릭 명령·이동 확인 음향을 현재 월드에 연결한다.
#pragma once
#include "o/RawCarrierMove.h"
namespace netstorm::client {
// 원본 TYPE+78/+80의 현재 음향 핸들이다. 실제 TYPE 속성의 변화를 조회 시점에 읽는다.
struct CarrierInputType {
    std::uint32_t moveSound{},dropSound{}; //일반 이동 확인/운반물 내려놓기 음향이며 문자열 포인터와 구별한다.
};
// 선택·강조 전역과 다른 Carrier.cpp 고유 전역의 단일 저장소다.
struct CarrierInputState {
    std::uint32_t cursor{},dais{}; //5453d8/5453dc의 현재 커서 코드와 희생 대상이다.
    std::uint32_t priestType{158},golemType{133}; //5412d0/54126c의 현재 타입 getter이며 원본 초기 번호다.
    std::array<float,2> insidePoint{}; //5453ec/f0이며 자기 사각형에서만 -10000을 기록한다.
    bool markers{}; //5453e0의 진단용 이동점 표시다. 일반 미션 부모가0으로 초기화한다.
    std::int32_t markerCounter{}; //54541c의 signed 나머지와 DWORD 감김 증가를 보존한다.
    std::span<const std::uint32_t> markerTypes; //54d928/531858의 실제 표시 타입 배열이다.
};
// 기존 큰 월드/포획/전송/희생 UI와 실제 커서·음향 장치를 공급한다.
struct CarrierInputHooks {
    std::function<bool(o::Sid)> immobile; //492090의 사제 HP/지면 검사다.
    std::function<o::Sid(o::Sid,std::uint32_t)> content; //4ac9f0(parent,mask,0,0)이며 두 호출을 합치지 않는다.
    std::function<std::pair<o::Sid,bool>(o::Sid,o::RawPathPoint)> candidate; //426900의 현재 포획 후보/너겟 여부다.
    std::function<std::array<float,2>(o::Sid,std::uint32_t)> dropPoint; //426a70의 기존 전체 배치 검사다.
    std::function<std::uint32_t(o::Sid)> graph; //462c80의 현재 Graph 번호다.
    std::function<bool(o::RawPathPoint,o::RawPathPoint)> connected; //488340의 같은 지면 연결 검사다.
    std::function<std::uint32_t(o::RawPathPoint)> surface; //40d9d0의 현재 spot BYTE다.
    std::function<o::Sid(o::RawPathPoint)> geyser,returnPlace,daisAt; //4b1fa0/4263a0/4b2070의 서로 다른 조회다.
    std::function<bool(o::Sid,o::Sid)> validGeyser,altarAllowed; //427580(parent,target)/4498e0(dais,parent)의 별도 조건이다.
    std::function<std::uint32_t()> localOwner,multiple; //현재540c70과4d5580을 효과 뒤 다시 조회한다.
    std::function<bool()> checking,earlySacrifice; //5e4794와447a50의 실제 조건이다.
    std::function<void(std::uint32_t)> assertion,highlight,selectCursor; //원본 행 진단/4d5bb0/4a4410의 실제 소비자다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> select; //4d5c90의 세 독립 DWORD다.
    std::function<void(o::Sid,o::RawPathPoint,std::uint32_t)> request; //427340의 실제 요청이다. 서버 Start를 직접 호출하지 않는다.
    std::function<void(o::Sid)> pickup; //428e00의 현재 포획 요청이다.
    std::function<void(o::Sid,std::uint32_t)> dropCargo; //427070의 실제 운반물 내려놓기 요청이다.
    std::function<void()> earlyLocal,refreshCursor; //4cf960/4a46d0의 서로 다른 실제 UI다.
    std::function<void(std::uint32_t)> earlyRemote,showSacrifice; //4c9150(owner,EarlySacrifice)/4cfc10(dais)다.
    std::function<void(o::RawPathPoint,std::uint32_t)> marker; //434970의 현재 표시 타입과 원본 snap 점이다.
    std::function<std::int32_t(std::int32_t,std::int32_t)> random; //4558f0의 실제 상한 미포함 난수다.
    std::function<void(bool,std::uint32_t)> namedSound; //4a9cb0의 현재 사제/골렘 이름 표 인덱스다. 인접 index4도 경계에 그대로 전달한다.
    std::function<void(std::uint32_t)> sound; //4a9550(handle,0,0,0,1,0)의 실제 음향이다.
};
// 참조 대상은 현재 입력 몸체보다 오래 살아야 하며 별도 경로나 선택 사본을 만들지 않는다.
class RawCarrierInput {
public:
    RawCarrierInput(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const CarrierInputType> audio,
        std::span<std::uint32_t> stamps,const std::uint32_t& frame,CarrierInputState& state,CarrierInputHooks hooks); //같은188TYPE/음향장부/PATCH를 검증한다.
    std::uint32_t Evaluate(o::Sid parent,o::RawPathPoint& point) const; //4277f0: 반환 코드와 caller의 점 수정을 보존한다.
    void Highlight(std::uint32_t parent,o::RawPathPoint point,std::uint32_t force) const; //4284a0: 진단점/현재 다중선택/평가 뒤 커서를 지정한다.
    void Commit(std::uint32_t parent,o::RawPathPoint point) const; //42a360: 클릭 의미를 분배하고 실제 요청만 송신한다.
    void Acknowledge(o::Sid parent,std::int32_t index) const; //426290: 같은 TYPE당 프레임 중복 음향을 억제한다.
private:
    o::Sid Number(std::uint32_t value) const; //원본 물리 SID 범위만 확인한다.
    std::span<const std::uint8_t> Raw(o::Sid parent) const; //현재50BYTE를 효과 뒤 다시 읽는다.
    std::uint32_t Type(o::Sid parent) const; //현재 TYPE 번호를 검증한다.
    std::uint32_t Genus(o::Sid parent) const; //현재 TYPE+ec를 읽는다.
    std::uint32_t Flags(o::Sid parent) const; //현재 TYPE+e8을 읽는다.
    static std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4); //비정렬 원문 WORD/DWORD다.
    o::RawPathPoint Position(o::Sid parent) const; //현재 두 float 비트를 보존한다.
    const o::SidPool& pool_; //동일한 실제 객체/내용물 풀이다.
    std::span<const o::RiftTypeRecord> types_; //현재188TYPE다.
    std::span<const CarrierInputType> audio_; //현재 TYPE 음향 핸들이다.
    std::span<std::uint32_t> stamps_; //실제 SoundProcess와 공유하는TYPE+1f0 장부다.
    const std::uint32_t& frame_; //같은 원본55b4a8 프레임이다.
    CarrierInputState& state_; //커서/희생/진단 표시의 현재 전역이다.
    CarrierInputHooks hooks_; //선택된 큰 실제 하위와 장치 경계다.
};
}
