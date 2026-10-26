# 기존 맵의 실제 Edit 진입 접합 — 464

- 작업 PC: HJOW-Athlon, 2026-10-26.
- 마지막 전체 정밀 디컴파일: HJOW-Athlon / 2026-10-23. `extracted/refined/`의 소스는 변경하지 않았다.
- 목표: 실제 메인 Edit 목록에서 기존 Fort를 선택하고 원문 State1/다음 Poll4를 지나 편집 장면과 ESC 메뉴에 진입한다.
- 최종 build9 경고/오류0, 집중298고유/309관측·신규6개, 전체 C++3154개·실패0. CTest wrapper는1개다.
- 같은 게임 SHA256: `661a19c19766c74f1743f40d57b241739e82a7b0803d01b6e619eabef95c77cf`. 실제 Battle1/TEST01 자연 GUI2건이 통과했다.
- 최종 결과: `cpppj/existing-editor-entry-20261026.json`. 원시: `extracted/cpp-editor-entry-464-*`.

## 실제 연결

`UberGump::Tick`의 원문 State 자리에서 Edit가 예약한 next1과 그 뒤 next4만 `RawLocalMissionState`에 전달한다. 캠페인 Login의 다른 요청은 이 어댑터가 소비하지 않는다. old/phase/immediate/current/next는 같은 Client의 다섯144BYTE이며 직접 phase를 쓰는 별도 성공 경로를 만들지 않았다.

`UberEditor.inc`의 State1 부모는 아래 순서를 사용한다.

1. 463의 실제 `CleanupNativeStateWorld(0)`을 호출한다. 기존 AI/Deck/Kernel/Core/Pool/Player/공통 HUD 정리를 재구현하지 않는다.
2. 현재 Chat 부모의 숨김 명령 뒤 실제 로컬 listener/client9를 연다. 전체 Client 연결표와 같은 Player9 물리를 빌리며 로그인/Player 재배치를 하지 않는다.
3. fort/editor/missionPost, 전체 genus 마스크와 FullDeck, server/boss, Core1, Native Renderer, Chunk reset/mode1,87BYTE 미션 옵션과00594fc4의 직접 prefix 쓰기를 원문 순서에 연결한다.
4. 준비된 물리를 빌리는 지연 ExistingEditor Session을 구성한다. Template 파일 설치는 실제 checkpoint까지 미룬다. 같은 장부·타입 변환표·옵션·SP·난수·연결표를 사용한다.
5. 파일 전 HUD mode1은 `WithExistingEditorPrefixUi`의 한정 동기 범위에서 분배한다. 원문43df80은 현재 Deck을 확보하고 기술 SID0도 실제 예약 슬롯으로 순회한다. 새 Deck 블록은 열기 전에 Kernel 프로세스를 만들지 않는다.
6. 현재 이름 선택, 작업본 파일, Subscriber/World/Deck/사제/카메라/생산/Core/Snapshot/State/Startup/EditorPlayers의 기존 checkpoint를 실행한다.
7. 기존 후반 UI 범위에서 실제 커서·HUD를 준비한다. 현재 HUD 명령, 벽시각+10, `ser22.mus`, 전체 Fort palette와 다음 State4 예약은 기존 tail을 그대로 사용한다.
8. 원문 공통 State ret가 전체144BYTE를 확정한 뒤 `CommitExistingEditorInstall`이 loaded와 실제 수신기를 공개한다. 그 뒤에 호스트 선택/운반체/사용자 입력을 연결한다. UI scope 안의 기존 `MakeUserSelection` 차단은 보존한다.
9. 다음 Tick의 실제 Poll이 State4를 소비한다. Editor의 ESC는 현재 Session/Template으로 일반 Game/Edit/View/Options 메뉴를 연다. 미션이 없을 때의 Guide getter는0이며 미션 스크립트를 대신 만들지 않는다.

## 파일과 수명

- 편집 파일의 기본 위치는 자산 폴더의 형제 `자산폴더이름-editor/d/맵이름.fort`다. 현재 원본 파일을 읽어 없는 작업본만 만들고 기존 사용자 작업본은 재복사하지 않는다.
- `EditorFiles`가 작업 namespace와 보호 루트를 검사한다. 실제 Template의 저장 부모는 같은 장치만 빌린다. 원본과 CD 파일은 수정하지 않는다.
- EditorRuntime은 실제 파일/경로 scratch/전송/State 분배기를 Session보다 오래 보관한다. 일반 반납에서는 Session 수신기를 먼저 닫고 남은 양끝/리스너를 반납한다.
-00542640은 실제 Client의 현재 DWORD를 객체 읽기와 Damageable 해방이 함께 빌린다. 기존 독립 raw 입력은 종전 값형 fallback을 유지한다.
- State의 seed/metaRefresh/metaRebuild/simulconnect/battleByte를 Client 수명으로 옮겨 Login과 Editor가 같은 현재 주소를 읽는다.
- Native fort/battle 쓰기는 같은 주소를 읽는 기존 Player/Bridge 어댑터의 호환 별칭에도 전달한다.
- Editor 파일 이름과 현재 모드가 실제 `EditorWindowName`을 선택하도록 제목을 연결했다.

## 기본 startupStateCheck

원본 기본 옵션을 강제로 끄지 않는다. 실제 `ReadSavedState`가 게임 폴더의 `d/!state.dat`에 `GetFileAttributesA` 존재 관문을 적용한다. 파일이 없으면 실제 reader가0으로 반환하고, 이어서 원문처럼 현재 벽시각도 조회한다.

저장 상태가 존재할 때의 open/8Player 복구/전역 owner·subscriber·이전 handle/질문 경로는 아직 미연결이다. 존재를 감지한 뒤 실제 선택된 open 경계에서 명시 중단하며 파일을 지우거나 복구 성공을 합성하지 않는다.

## 실패 원시와 보완

각 원시는 `extracted/cpp-editor-entry-464-*`에 별도로 보존한다.

- 첫 Battle1 실행: Native fort 쓰기가 기존 Player 별칭에 전달되지 않아 prepared 입력 검사가 거부됐다. 같은 주소의 Player/Bridge 별칭을 연결하고 회귀 검사를 추가했다.
- 두 번째 실행: 실제 Client 장부와 타입 변환표의 서비스 주입이 빠져 있었다. 캠페인과 같은 Client 소유자를 명시 전달했다.
- 집중1: 원문 Finder가 허용하는 기술 SID0을 생산 동기화 어댑터가 거부했다. 물리 범위 검사는 유지하고0도 같은 예약 슬롯으로 조회한다. 기존 Native98 기대값은 바꾸지 않았다.
- 새 prefix 검사: Deck 확보가 Kernel 프로세스를 만든다고 잘못 가정했다. 원문44f760의 저장 블록 생성과 열기를 구별하여 새 검사 입력/관측을 수정했다. 기존 검사 본문은 바꾸지 않았다. 새 최종 입력 공장 검사에서는 전송용 State 도우미에 실제 로컬 양끝을 제공하도록 입력을 보완했다.
- 세 번째 실행: 실제 기본 startupStateCheck의 reader가 빠져 있었다. 실제 파일 부재 관문과 벽시각을 연결했다. build4의 Win32 선언 누락은 명시 include로 수정했다.
- 네 번째 실행: loaded 앞의 호스트 입력 구성 시점이 차단됐다. UI scope의 입력 공장 허용으로 우회했던 시도는 집중3의 기존 차단 검사2실패로 폐기했다. 기존 차단을 복구하고 호스트 입력 구성을 공통 확정 뒤로 이동했다.
- 다섯 번째 실행: 장면 진입/State4까지 통과했지만 ESC가 미션 스크립트를 필수로 요구했다. Editor의 실제 Session/Template 메뉴로 연결했다.
- TEST01 첫 GUI: 명령줄 fortSpec 입력이 목록을 좁히지 않아50개 목록에 TEST01이 없었다. 새 복사본에 loose Fort만 TEST01로 제한하여 같은 실제 목록/클릭 경로를 검사한다. 기존 파일 삭제와 원본/fixture 수정은 하지 않았다.

## 검증의 범위

최종 검증은 집중 사례 이름/요약/종료 코드, 실제 전체 CTest 내부 사례 수, 같은 게임 SHA의 자연 메뉴 클릭과 BMP를 함께 확인한다. CTest wrapper1개와 내부 C++ 사례 수를 구별한다. 기존3148개 등록 이름과 기존 검사 본문, 명시 보호1820파일 및 fixture654파일을 대조한다.

기존 맵의 진입과 메뉴 표시는 Editor 전체 도구 완료가 아니다. 새 맵/기본 이름/잘못된 파일/미션 전환 분기, 실제 편집 도구와 Save as/NewFort/Test Battle/Main Menu 동작, 기존 전투 상태 복구, 전체 미션 자연 완주, 원본 화면·음향 동일성은 후속이다. 전체 목표는 계속 진행 중이다.

## 최종 관측

- Battle1은 전체 자산 복사본의 일반 Edit 목록에서 선택했다. TEST01은 새 복사본에서 loose Fort만 TEST01로 제한하여50개 한도 안에서 실제 목록 클릭을 했다. TEST01.fort와 TEST01.english는 보호 원본과 같은 바이트다.
- 두 실행 모두 ready의 Native phase4/current0/next0, 실제 현재 Fort 이름, commit 복귀1, client9/accepted -1 생존, 전체 HUD/World 대여를 확인했다. ESC serial/존재 비영, Game/Edit 행과 최종 ESC serial0/존재0을 확인했다.
- BMP의 장면·HUD·Game/Edit 팝업을 직접 확인했다. 사용한 연산은 실제 메뉴 클릭/ESC이며 test-state-*는 없었다. 원본 실행 파일은 실행하지 않았다.
- 기존3148개의 실제 등록 이름과 변경된 검사3파일의 기존 본문을 그대로 유지했고 신규6개만 추가했다. 보호1820파일/fixture654파일의 추가·삭제·바이트 변경은 없다.
- 새 맵/실제 도구/저장/시나리오와 캠페인 이탈 후 Edit 전환, 기존 저장 상태 복구, 끝 구분자가 있는 자산 경로는 후속 검증/구현 대상이다.
