# HTML 원소별 기술 목록과 공유 기술 필터 — 단위705

2026-11-02, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 원문 시나리오 HTML의 기술 목록이 파서까지 있고 실제 Client 목록 생성기는 미연결임을 확인했다. 이번 단위는 전체 목록이 공유할 두 필터와 **원소별 ThemeTechList 생성/실제 연결**을 복원한다. 전체 TechList와 선택 후 요청 처리는 아직 남는다.

## 발견과 구현

`HtmlWindowMetadata`는 *TechList*와 *ThemeTechList*를 인식하지만 `UberGump::LoadingHtmlContext`가 두 목록 서비스를 제공하지 않았다. 보호 TARC의 `d/tell.english`에도 `$Menu=*TECHLIST*,DoNothing,{param.1}`이 있고, 원문004c83b0은004cb690/004cb4b0을 호출한다. 이는 loose499개 파일의 명시 `<$태그>`만 세었던687의 목록 범위 밖이다. 같은 파서가 있다는 이유로 실제 연결까지 완료로 세지 않았다.

`TechnologyListMenu`의 세 몸체는 다음을 보존한다.

- **CountKnown004cb320:** level이0..2 또는-1이 아니면 진단 Ui.cpp0xb94 뒤 계속한다. 현재 기술 부모를 한 번 조회하고 TYPE0부터 현재 signed count 순회다. 사제 타입과group10을 제외하고 theme5이면 레벨도 무시한다. 나머지는 정확한 원소/레벨을 비교한 뒤 종류WORD 보유 여부를 읽는다.
- **CountCandidates004cb3f0:** 같은 필터·진단0xbad이며 소유 여부를 읽지 않는다. 사용하지 않더라도 원문 선행 currentTechnology 호출을 유지한다.
- **AppendTheme004cb4b0:** 대상은 TYPE 번호가 아닌 실제 SID다. 가상+80의 level-1을 사용해 각각40칸의 알려진/후보 목록을 수집한다. 전체 일치 수와 저장 수를 구별한다. 넘친 후보도 전체 수만큼 순회하며 저장된 개수의 나머지로 다시 읽는다. 원문처럼 이미 보유한 기술을 숨기지 않고 비활성 행으로 둔다. templeProduct만 추가 제외한다. 번역 뒤 level+1을 먼저 보관하고 설명 getter→printf→현재 명령→행 순서다.

원문 두 heap0xa0BYTE는 호스트 고정 배열로 보관한다. 할당 실패/전체 예외 ABI·손상된 포인터의 메모리 침범까지 동일하다는 주장은 하지 않는다. 배열을 원문보다 크게 만들어 넘침을 숨기지 않는다.

## 실제 연결과 남은 선택

`RawCampaignSession::AppendThemeTechnologyMenu`가 같은 실제 TYPE/기술 부모/내용물 종류WORD와 기존 RewardCount의 가상 레벨을 공급한다. `RawMenuControls::AppendThemeTechnology`가 현재 HTML이 이미 연 controlled 열에 같은 AddText를 쓴다. 새 지식 카드/루트를 만들지 않는다. `UberGump::LoadingHtmlContext`는 현재 World를 호출 시 조회하는 themeTech 서비스를 연결했다. 표시 문구는 기존 번역/설명과 고정256BYTE 두 인자 서식을 사용한다.

행 명령은 보호 PE의 **GetNewTech00542a48/020000d5**를 그대로 둔다. 선택 후 현재 제단/요청 전송 부모는 아직 미연결이다. 이 명령을 즉시 기술을 지급하는 GetTechnology0200009f로 바꾸지 않는다. 따라서 원소별 목록의 생성/표시 연결이 전체 선택 동작 완료는 아니다.

**다음 필수 작업:** 전체 *TechList*004cb690 부모의 현재 메뉴 객체 바인딩·네 원소 머리/구분선·같은 후보 목록·제단 업그레이드/최대/랭크 행을 복원하고 같은 HTML 서비스에 연결한다. GetNewTech/RequestStormpower는004d82ed/004d8351의0040af50 요청 작성과현재005453dc를, UpgradeAltar는별도00542a44 입력을 따른다. 원문 packet/서버/로컬 분기를 확인하기 전에 직접 획득으로 합성하지 않는다. 새 MP/통신 검사는 하지 않는다. HTML Input004d3ed0/Zsend 서비스도 별도 미연결이다.

## 원문과 검사

[원문 도구](../../tools/decomp_technology_list_menu.py)는 실제004cb320/004cb3f0/004cb4b0을 **367입력**으로 실행했다. TYPE주소·가상레벨·현재기술/보유종류·번역/설명/printf·메뉴·heap·진단은 명시 경계다. 반환/저장 개수·40DWORD 꼬리와 호출 순서를 [fixture](../../cpppj/tests/fixtures/technology-list-menu-x86.tsv)에 기록했다. [원문 보고서](../../cpppj/technology-list-menu-native-20261102.json)를 따른다. 독립 타입80개이며 실제 공중 유닛을 생성/생산하거나 행동시키지 않는다.

새 C++3개는367행 전체 대조, 실제 HTML 메타의 packed SID/원소→원소 목록 호출과 같은 메뉴 열, 미선택 장치/실패 선행 효과다. 기존 HTML 메타128행 검사1개와 함께 **4개 통과·실패0**이다. 기존128행은 재생이며 새 원문 실행 수에 합산하지 않는다. `build-v1` Release 경고·오류0, `focused-v1` 첫 실행 통과다.

## 제품·보호·범위

실행 제품705/표시11.01 SHA-256은 `3dc77042d1fb25b51db6667014177092a579731f99ca185740a3987b0c3c4d99`다. [제품 보고서](../../cpppj/technology-list-menu-20261102.json)와 원시 `extracted/cpp-technology-list-menu-705/`를 따른다. 기존 AGENTS·원본1394·CD425·fixture803의2623개 변경0, 새fixture1 포함 현재2624개를 확인했다. AGENTS/전체 디컴파일은 수정하지 않았다.

이번 게임/GUI 실행은0이다. 최신 실제 GUI는704 제품이다. 실제 제단 선택/희생·전체 TechList/랭크/요청 동작의 성공으로 확대하지 않는다. 전체 C++·전체34·새 ZIP·실제 PCM·물리 입력·원본 전체 픽셀·자연 완주·포획/희생·특정 적 공격·승리·공중3종 전용 행동·MP/통신 검사는 없다. 전체 목표는 미완료이며 이후 MP 무시험 사전 구현과4~7차 순서를 유지한다.
