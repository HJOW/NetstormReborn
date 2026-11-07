// R87h 명령이 사용하는 실제 서버 템플릿/섹션·공통 치환 장치를 복원한다.
#pragma once
#include "root/SharedFiles.h"

namespace netstorm::root { //00451f30/452170/452350/452460/452570의 현재 파일/존 표를 사용한다.
class CommandPages final { // 실제 서버 owner는 이 장치와 빌린 상태/파일을 CCh callback보다 오래 보관한다.
public:
    CommandPages(ConnectionState& state,SharedFileStorage& storage); // 참조만 보관하며 파일/폴더를 만들거나 읽지 않는다.
    CommandPages(const CommandPages&)=delete;CommandPages& operator=(const CommandPages&)=delete; // 같은 현재 상태와 callback this를 복제하지 않는다.
    std::string Load(); // Admin.server→Update.server→Guild.server를 매회 실제 working/Saves에서 읽어 구분자 없이 잇는다.
    static std::string Section(std::string_view source,std::string_view name); //452170의 첫 [name] 뒤→첫 [END] 전을 자른다. 누락 marker의 원문 -1+길이 계산도 유지하며 물리 밖은 거부한다.
    static void Replace(std::string& page,std::string_view tag,std::string_view value,std::int32_t limit=-1); //44a5d0의 원문 사본상 모든 일치/위치 보정·선택 교체 한계를 유지한다. 새로 삽입한 tag를 다시 검색하지 않는다.
    static void ReplaceInteger(std::string& page,std::string_view tag,std::int32_t value,std::int32_t limit=-1); //44b3d0의 signed %d와 같은 문자열을 같은 치환 장치에 전달한다.
    void Expand(std::string& page); //452570의 실제 OS 현지시각 TIME→현재51칸 ZONELIST 치환 순서다.
    void Bind(ChallengeRuntimeHooks& challenge); // 같은 템플릿 로드/공통 치환 장치를 실제 CCh 부모가 선택하도록 빌린다.
private:
    ConnectionState& state_;SharedFileStorage& storage_; // 같은 현재 R 존 표/실제 사용자 홈 우선 파일 장치다.
    static std::string Time(); //452350의 _time32/localtime/asctime 앞19BYTE와 ': '를 실제 OS 시각에서 만든다.
    std::string Zones(); //452460의 현재 비NULL0..50 존과 원문 herf/X1 링크 문자열을 그대로 잇는다.
};
}
