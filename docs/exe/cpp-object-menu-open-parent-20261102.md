# 객체메뉴 반환·열기 음향과 Ctrl+special 직접 지점 진단 (단위750)

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 내보내기는 바꾸지 않았다. 이전749의 편집 취득/우클릭 취소·검증·커밋을 유지한다. 현재 제품은 **750 / 표시11.01**이다. 사용자의 최신 요청에 따라 이번 단위를 검사·문서·로컬커밋까지 마친 뒤 목표를 일시 중지한다. 전체 목표는 미완료이며 재개 요청 전 추가 목표 작업을 하지 않는다.

## 원문과 수정

원문 `004d9df0..004d9e1e`는 실제 객체의 가상+ac에 같은12BYTE 사건을 전달하고, **반환 DWORD가 비영일 때만 opengump.wav와0 다섯 개 인자**로 음향을 한 번 재생한다. 현재 메뉴의 visible이나 루트 토큰으로 성공을 다시 계산하지 않는다.

기존 `CampaignObjectMenuClient::Open`은 실제 Dispatch 반환을 내부 루트 선택에만 쓰고 void로 버렸다. `UberGump::OpenObjectMenu`도 이 호출만 하고 성공 음향을 실행하지 않았다. 현재는 실제 반환값을 상위에 보존하며 `OpenPointerObjectMenu`가 위 분기를 실행한다. noAudio는 기존 검사 장치 정책을 유지한다. 호스트 도우미가 돌려주는 accepted는 보존한 열기 결과이며, 음향 호출이 끝난 원문 EAX와 동일하다는 주장은 아니다.

원문 `004da263..004da2e6`은 일반 좌클릭 Select 뒤 **현재 special0055a480과 Ctrl**을 검사한다. 선택된 경로는 사건좌표를 현재 카메라로 역투영하고 `4202f0`의 단일 지점 finder를 시작하여, 각 후보의 **직접 가상+70** 뒤 같은 finder를 진행한다. 객체 진단 출력 경로이며 공격이나 소유자 변경 명령이 아니다. Form을 재귀 순회하는44dab0과 구별한다.

`GameWorld::SelectFromPointer`는 같은 실제 Select 뒤 위 진단 부모를 호출하고, 그 뒤748의 편집 소유자 추적/Ctrl 다리 순환을 실행한다. 현재 Pool/TYPE/해시를 빌리는 RawSquidFinder와 기존 TruncatePoint를 사용해0방향 절삭한 단일 셀에서 flags0으로 순회한다. 가상효과 뒤 현재 finder를 진행하며 중복 후보를 임의 제거하지 않는다. UserInput의 실제 하위는 기존 `TraceRawObjectVirtual`과 같은 Client suppress/로그/들여쓰기에 연결했다. 개발자0055a484를 켜거나 원본 설정을 수정하지 않는다.

## 독립 원문 근거

새 `tools/decomp_pointer_menu_parent.py`는 보호 PE의 검토한 두 기본블록을 게임/창 없이 실행한다.

- **메뉴8행:** 가상ac 반환0/1/2/ffffffff와 두 사건 좌표를 교차해 반환0의 음향 생략과 비영의 정확한 이름/0 다섯 개 인자를 관측했다.
- **진단36행:** special0/1/2×Ctrl2×빈/단일/중복 후보3×사건 좌표2다. 단락과 투영/finder/가상70/Next 호출 순서를 실제 x86에서 관측했다.
- 가상ac/70·음향·투영·finder는 명시 외부 경계다. 두 새 fixture를 최종 도구로 재관측해 바이트가 일치하고 기본블록 코드가 불변임을 확인했다. 실제 외부 하위/게임 전체 관측이나 전체 finder 검증으로 확대하지 않는다.10.72의 다른 부모는 이번 실행 범위가 아니다.

## 실제 연결과 검사

창 없는 실제 **Client→UserInput→GameWorld→Session**과 제품 `CampaignObjectMenuClient`, 기존 실제 글꼴/공통 메뉴 트리를 사용한다. 편집용 워크샾의 건설 중 비트가 있으면 반환0/메뉴 없음/음향 없음이고, 이를 해제하면 반환1/실제 부착/열기음향 부모1회임을 확인했다. Player 하위 메뉴는 같은 Client의 실제 초기 빈 Chat Console/공유 버퍼에서 수신 대상 포함 여부를 조회한다. 메시지 전송이나 멀티플레이 상태 검사는 실행하지 않았다.

같은 워크샾에 Form 자식을 부착하고 Ctrl+special 좌클릭을 실제 입력에 전달했다. 직접 Base/Main 로그가 남고 Form 재귀의 END 로그가 없으며, 현재 depth7/indent3과 사건과 다른 폴링 좌표(123,145)가 유지됨을 확인했다. special0에서는 같은 입력의 로그 버퍼가 바뀌지 않는다. 새 소스의 직접 진단이 현재 월드를 읽는 것을 확인했으며 다른 Pool을 가진 재귀 진단으로 대체하지 않았다.

최초 전체 집중 실행은 **71개/1단언 실패**였다(`tests-v2.log`). 창 없는 검사 장치에 현재 Chat 연결이 빠졌던 원인이며 실제 Chat 공장/버퍼를 연결했다. 그 과정에서 ScriptStringRing 선언 헤더 누락에 따른 컴파일 연쇄 진단도 `build-v3.log`에 보존했고 헤더를 추가했다. `tests-v4-actual.log`의 실제 연결1개 통과 뒤 문자열 고리가 UI 소멸보다 오래 살도록 선언 순서를 정리했다. 최종 `build-v5.log`는 제품/집중 대상 성공·경고/오류0이며 `tests-v5.log`가 **집중71개/실패0**의 근거다. 새4개(실제 연결1·원문 부모2·누락 경계1)이며 실행 대상66개/전체71대상·주 검사 캐시ON을 유지했다. 전체 C++ 재실행은 아니다.

EXE SHA-256은 `4b9401c8f830cedba06fa0d3c8ff75775d706c11b6c7034e8aecdd83a5a1719a`다. 기존 보호 **2659개(AGENTS1·원본1394·CD425·fixture839)** 모두 불변이고 새fixture2를 포함해 **2661개/fixture841**다. 다음 기준은 Git 제외 `extracted/cpp-object-menu-open-parent-750/protected-after.json`이다.

이번 새 게임 프로세스/독립CLI/GUI/전체34/새ZIP/전체C++/신규MP 검사는0이다. 마지막 GUI746과 전체34/ZIP645·644는 별도 제품 근거다. 실제 PCM/물리 입력·자연 완주·포획/희생/특정 적 공격/승리·공중3종 전용 행동은 추가하지 않았다.

## 일시 중지와 재개

사용자 요청은 **현재 작업 마무리→문서 반영→git 커밋→목표 일시 중지**다. 이번750에서 이 순서로 마감하고 다음 단위를 시작하지 않는다. 재개 시 AGENTS.md/LEFT_JOBS.md/LEFT_JOBS.dotnetpj.md와 현재 작업트리를 다시 확인한다. 남은 비멀티 명령/UI·시나리오/파일·수명/호환성을 점검하며 challenge 선행 클릭/빈점 항해 메뉴·모드6ChalSailPos 소비자는 MP 후속 무시험 범위로 구별한다.3~8차 전체 목표도 남아 있다.

보고서는 `cpppj/object-menu-open-parent-20261102.json`, 원시 로그와 최종 보호 기준은 Git 제외 `extracted/cpp-object-menu-open-parent-750/`다.
