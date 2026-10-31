// 원본004b72eb~004b743b의기존Fort Editor선택외곽과004b5ca0의정상기존파일꼬리를복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include "o/RawNewFort.h" // 같은 파일 부모가 부재 분기에서 기존 신규 자료 생성을 재사용한다.

namespace netstorm::o {
// 이한정모듈이실제구현없이정상성공으로통과시키지않는원문다른분기다.
enum class ExistingEditorInstallOutside { DefaultName,NewFort,InvalidFile,Mission }; //각원문진입점에서명시거부하며파일삭제/새Fort/미션을대신실행하지않는다.
// 현재슬롯과같은실제State/파일/Subscriber/표시를연결하는기존파일부모의하위경계다.
struct ExistingEditorFileHooks {
    std::function<std::string_view()> path; //00460680현재Fort경로다.존재검사/실제읽기전에각각다시조회한다.
    std::function<std::int32_t(std::string_view)> exists; //0041ab10실제파일존재다.0이면미복원NewFort분기로간다.
    std::function<std::uint32_t()> owner; //00540cac현재슬롯이며각하위앞에서다시읽는다.
    std::function<void(std::uint32_t,std::string_view)> load; //004bf550의실제같은파일읽기/manager수명설치다.
    std::function<std::int32_t(std::uint32_t)> valid; //0041aad0의실제서명/유효성부모다.0을기본성공으로바꾸지않는다.
    std::function<std::uint32_t(std::uint32_t)> manager; //004bd340의고정this에서현재첫DWORD만읽는다.
    std::function<void(std::uint32_t)> assertion; //004bd340의Template.cpp410진단이다.복귀해도실제본문은항상0반환한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> state; //004bd550가돌려준같은State의첫BYTE를직접수정한다.
    std::function<void(std::uint32_t)> notify; //004bd540/004bd360실제통지이며먼저쓴State를자동복구하지않는다.
    std::function<void(std::uint32_t)> subscriber; //004bd450가현재등록ID00540c74와현재handle에읽는다.이름을임의생성하지않는다.
    std::function<std::string_view()> handle; //Subscriber복귀뒤현재00540ca8의NUL제외이름이다.
    std::function<void(std::string_view,std::string_view)> setConfig; //현재fort.mySubHandle을실제설정에쓴다.
    std::function<void()> title; //00460730실제제목갱신이다.Subscriber/config뒤World보다앞이다.
    std::function<void(ExistingEditorInstallOutside)> outside; //원문다른분기의명시인계다.복귀하면이모듈이예외로거부한다.
};
// 초기cleanup/모드/UI/State요청복사뒤의선택본문에서실제로도달한외부효과만요구한다.
struct ExistingEditorFortHooks {
    std::function<std::string_view()> currentName,pendingName; //0055b490/494의현재NUL제외이름이다.이름변경/Guide뒤다시조회한다.
    std::function<std::int32_t(std::string_view,std::string_view)> compare; //원문CRT대소문자무시비교다.0이면같은이름이다.
    std::function<std::uint32_t()> inMission; //00594fa0현재DWORD이며이름비교뒤와파일설치뒤에각각읽는다.
    std::function<std::int32_t(std::string_view)> missionName; //00460c40의현재pending이름판정이다.미션이비영일때만선택한다.
    std::function<void()> closeGuide; //00463b20(00562ca0)의실제종료다.복귀뒤현재pending을다시읽는다.
    std::function<void(std::string_view)> setName; //00460980의실제전체이름부모다.이외곽은그하위Snapshot/삭제를대신하지않는다.
    std::function<void()> defaultName; //00460c20의 현재 newFort 조회→같은 전체 이름 선택이다. 빈/점 시작 이름 뒤 한 번만 호출한다.
    std::function<void()> file; //아래InstallExistingEditorFile로정상기존파일/State|1/Notify/Subscriber/config/title까지설치한다.
    std::function<void()> world; //004b736a~7397의현재Player.Create→CoreData(0)→Recover→World(0)다.실제Session.LoadExistingEditorWorld한번에연결한다.
    std::function<std::uint32_t()> owner; //Deck/Core쓰기/Snapshot/State/Notify앞에서각각읽는현재00540cac다.
    std::function<void(std::uint32_t)> deck; //004bf190의같은변환표/실제Deck부모다.
    std::function<void()> priests,camera,production; //004923e0/004340d0/0047d890이다.사제자연생략도해당실제부모가판정한다.
    std::function<void(std::uint32_t)> coreWrite,snapshot; //004beb30의진단/자연관문과004bf9a0전체저장본문이다.설치중private본문에연결한다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> state; //반환State주소를유지한채첫BYTE만&fe한다.
    std::function<void(std::uint32_t)> notify; //State수정뒤현재owner로다시통지한다.
    std::function<void()> startupStateCheck; //004b5ba0필수부모다.Editor자연생략을추가하지않고반환0/1은이외곽에서사용하지않는다.
    std::function<std::uint32_t()> editor; //004b741a현재005c85a4다.진입시값을캐시해lastEditFort/Player를강제하지않는다.
    std::function<void(std::string_view,std::string_view)> setConfig; //현재curFort로lastEditFort를쓴다.복귀뒤EditorPlayers가따른다.
    std::function<void()> editorPlayers; //004904b0과끝0046e980의실제본문이다.
    std::function<void(ExistingEditorInstallOutside)> outside; //빈/점이름/현재Mission분기를완료로가장하지않는명시인계다.
};
// 정상기존파일부모만실행한다.범위밖/선택경계실패는앞선부분효과를보존한다.
void InstallExistingEditorFile(const ExistingEditorFileHooks& hooks);
// 004b5ca0의 최초 exists가 비영으로 확정한 뒤 경로를 다시 읽는 기존 파일 몸체다. 앞선 존재 검사를 반복하지 않는다.
void InstallPresentEditorFile(const ExistingEditorFileHooks& hooks);
// 잘못된 파일의 반납·진단·기본 이름 선택을 같은 실제 owner/작업 파일에 연결한다.
struct InvalidEditorFileHooks {
    std::function<std::span<std::uint8_t>(std::uint32_t)> header; // 현재20BYTE의+8 저장 억제 DWORD를 직접 수정한다.
    std::function<void(std::uint32_t)> clear; //004bf690의 실제 Template 반납이며 예외 때 억제1을 되돌리지 않는다.
    std::function<void(std::uint32_t,std::string_view)> log; //004c2630의29번 원문 진단이다. 로그 뒤 현재 이름을 다시 판정한다.
    std::function<std::int32_t()> isDefaultName; //00460bf0의 현재 newFort 조회 뒤 CRT 대소문자 무시 비교다.
    std::function<void(std::string_view)> erase; // 기본 이름과 같을 때만 현재 경로를 다시 읽어 작업본을 삭제한다.
    std::function<void()> defaultName; //00460c20의 실제 전체 이름 선택이며 복귀 뒤 최초 exists부터 반복한다.
};
//004b5ca0의 실제 반복 부모다. 부재/유효 파일의 기존 child와 손상 파일 재선택을 같은 순서로 실행한다.
void InstallEditorFortFile(std::uint32_t& alternate,const NewFortMissingFileHooks& missing,const ExistingEditorFileHooks& present,const InvalidEditorFileHooks& invalid);
// 004b743b의부분checkpoint까지다.전체State부모반환/후속UI/최종loaded는설정하지않는다.
void InstallExistingEditorFort(const ExistingEditorFortHooks& hooks);
}
