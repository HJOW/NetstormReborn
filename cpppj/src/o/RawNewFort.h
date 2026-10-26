// 10.78의 새 Template manager·신규 자료·파일 부재 부모를 현재 실제 장치에 연결하는 준비본이다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // 원문 Native NewFort 부모의 제어 순서만 소유한다.
struct NewFortManagerHooks { // manager 할당/생성·실제 파일 삭제·섹션 초기화의 선택된 경계다.
    std::function<void(std::uint32_t)> assertion; // 기존 header.manager 비영의 Template.cpp:0x172 진단이다.
    std::function<std::int32_t(std::string_view)> exists; //0041ab10이며0외의 출처값은 모두 삭제 임시 manager 경로다.
    std::function<std::uint32_t(std::uint32_t)> allocate; //operator new(0x114)의 실제 토큰/0이다.
    std::function<std::uint32_t(std::uint32_t,std::string_view)> construct; //004bcc60이며 기존 ConstructTemplateDataManager와 같은 SerializationVersion을 사용한다.
    std::function<std::string_view(std::uint32_t)> fileName; // 임시 manager의 가상+0에서 읽는 실제 현재 이름이다.
    std::function<void(std::string_view)> erase; //0041aef0이며 실제 삭제는 같은 EditorFiles namespace에서 수행한다.
    std::function<void(std::uint32_t)> destroy,release; // 임시 manager의0044da60과CRT free를 순서대로 실행한다.
    std::function<void(std::uint32_t)> initialize; //0044d870의 실제 canonical 섹션 초기화다.0도 전달되며 물리 실패를 성공으로 바꾸지 않는다.
};
void CreateNewFortManager(std::span<std::uint8_t> header,std::string_view fileName,std::uint32_t argument,const NewFortManagerHooks& hooks); //004bfba0의 정확한20BYTE 헤더 부분쓰기와 현재 token 공개다.

struct NewFortDataGlobals { // 새 사본을 만들지 않고 Client/Player/난수 owner의 현재 참조를 빌린다.
    std::uint32_t& subscriber; //00540c74이며 최초-1 뒤 등록 부모가 같은 위치를 쓴다.
    std::function<std::span<std::uint8_t>()> handle; //PTR00540ca8의 현재 NUL포함 출력 버퍼다.
    std::function<std::uint32_t()> editor,playerKnown,localOwner,templateOwner; //005c85a4/00595344/00540c70/00540cac의 각 현재 조회다.
    std::uint32_t& coreActive; //00595348은 SP 차감 뒤1로 쓴다.
    std::uint32_t& coreSeed; //005c89b4 전체DWORD이며 random 뒤 lowWORD만 바꾸고 현재 상위WORD를 보존한다.
};
struct NewFortDataHooks { // 실제 공유 하위는 선택한 원문 위치에서만 요구한다.
    std::function<std::string_view(std::string_view)> translate; //004de9a0의 Unnamed 원본 BYTE 문자열이다.
    std::function<void(std::string_view,std::string_view)> configure; //004414d0이며 원문 fort.mySubHandle 철자를 그대로 사용한다.
    std::function<void()> title,registerSubscriber; //00460730과 기존 FinishFortSubscriberRegistration004b38d0다.
    std::function<float(std::uint32_t)> sp; //known비영일 때만0040ec50(owner)의float32를 읽는다.
    std::function<void(float)> addSp; //0040eec0(-SP)의 현재 local owner 부수효과다. 선행 owner를 대신 캡처하지 않는다.
    std::function<std::span<std::uint8_t>()> coreData; //0048f0d0의 현재 실제10BYTE CoreData다.
    std::function<void(std::uint32_t)> coreWrite; //004beb30이며 Editor의 자연 skip도 이 실제 부모가 결정한다.
    std::function<void()> prepareRandom; //004559d0의 같은 Client GameMode/CRT/GameRandom 준비다.
    std::function<std::uint32_t(std::uint32_t)> nextRandom; //004558c0의 현재 공유 난수이며254 또는65536을 그대로 받는다.
    std::function<void(std::uint32_t,std::uint32_t,std::span<const std::uint8_t>)> subscriber; //004bd400(owner,ID,현재handle)의실제포인터/물리뷰다.등록helper뒤strlen하므로부모에서문자열길이를미리캡처하지않는다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t,120>)> territory; //004bd600의 로컬20×6BYTE 전체 저장이다.
    std::function<void(std::uint32_t)> money; //004beec0의 실제 Money 섹션 생성/저장이다. Deck 저장이 아니다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> state; //004bd550의 현재 State포인터를 빌린다.
    std::function<void(std::uint32_t)> notify; //004bd360의 같은 현재 Template 알림이다.
};
void CreateNewFortData(NewFortDataGlobals globals,const NewFortDataHooks& hooks); //004b3920의 실제 현재 조회/초기120BYTE·Core10BYTE·seed·State kind 순서를 수행한다.

struct NewFortMissingFileHooks { //004b5ca0의 최초 파일 부재 분기를 별도 한정한다.
    std::function<std::string_view()> path; //00460680은 존재검사 전과 새 manager 생성 전에 따로 호출한다.
    std::function<std::int32_t(std::string_view)> exists; //0041ab10의 현재 작업본 파일 출처다.
    std::function<std::uint32_t()> owner; // 각 원문 효과 직전의 현재 Template owner다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t)> manager; // 현재 header에 CreateNewFortManager(name,0)을 실제 실행한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> state; // 새 manager의 실제 State포인터다.
    std::function<void(std::uint32_t)> notify,subscriber; // State|1 알림과 마지막004bd450 Subscriber 읽기다.
    std::function<void()> data,title; // 실제004b3920과00460730이다.
    std::function<std::string_view()> handle; // Subscriber 읽기 뒤의 현재 handle이다.
    std::function<void(std::string_view,std::string_view)> configure; // 같은 fort.mySubHandle을현재내용으로설정한다.
};
bool InstallMissingNewFort(std::uint32_t& alternate,const NewFortMissingFileHooks& hooks); //존재파일이면false로반환하고아무신규효과도하지않는다.파일부재의정상경로만true다.
}
