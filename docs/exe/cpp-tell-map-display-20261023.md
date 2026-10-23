# 실제 Tell 맵 목록 표시와 Client 수명 연결

단위358(`adb059fc / 20261023-23 CP`)의 맵 메뉴 부모를 실제 UberGump에 연결했다. 작업 중 별도 배포 스크립트 단위359(`12b1b56e / 20261023-24 CP`)가 커밋되어 이 작업은 **단위360**으로 구별한다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 기존 원문만 참고했으며 새 디컴파일/원본 게임 실행은 없다.

## 변경

- UberGump는 같은9BYTE FileSpec 버퍼, SoloMapCatalog의 파일 캐시/dirty, 월드 해제 뒤에도 남는 사용자 이름을 소유한다. 페이지마다 캐시를 새로 만들지 않는다. 실제 파일 타입 개수/번역/현재 키를 기존 목록 모듈에 공급한다.
- RawCampaignSession::CurrentHandle은 현재 월드 부모가 복사한 같은64BYTE handle을 NUL까지 빌린다. 목록은 월드가 있으면 이 현재 버퍼를 읽고, ClearWorld는 Session을 해제하기 전에 이름을 Client 저장소에 복사한다. 최초 이름은 빈 문자열이며 임의 userName 설정이나 현재 Player 표시 이름으로 대체하지 않는다.
- MapMetadata는 첫 HTML 뒤 메타 추출 시점에 실제 파일 목록과 열을 추가한다. 현재 미션 Fort 이름과 별도 pending 이름을 구별한다. 원본 행의 좁은 라벨/이름은 페이지의 기존UTF-8 경계에서만 변환한다. 원본 명령020000ca는ReadFort로 연결 자료에 보존한다.
- 페이지는 열 생성 플래그를 소유하고, 항목은 해당 열 번호를 가진다. 현재 표시 어댑터는 가장 긴 열의 행 수로 패널 높이를 계산하고 각 열을 같은 첫 행 위치에서 표시한다. 입력 사각형·하이라이트·체크·텍스트 클립도 같은 열을 사용한다.
- `$Checked`는 체크값이0이어도 `$Menu` sentinel 판별로 보내지 않도록 메타 종류를 별도로 보존한다.
- UI 보고서의 map_menu는 현재 열 수/캐시 개수/dirty/키를 읽기만 한다. 검사 전용 단계는 일반 Kernel Tell을 호출하며 실제 메뉴의 메타/표시 경로를 대신하지 않는다.

## 검증

Release 빌드1/2는 모두 **종료0·경고0·오류0**이다. 최종 전체 CTest1은 **2,347검사·실패check0·종료0/194.37초**이며 LastTest.log 내부2347/0도 확인했다. 제품/검사 C++는 최종 빌드2와 집중/GUI/전체 검사 뒤 바꾸지 않았다.

관련20검사(TellMap4/Catalog9/HTML6/실제 Session1)는 실패check0이다. 기존 Session 검사는 생성 직후 빈 이름, 실제 Load 뒤You를 확인하고 같은 원본 SP/월드 계약을 계속 검사한다. 기존 HTML 검사는 Menu/Checked 종류 보존을 추가로 확인한다. 독립 원본1,728관측과 실제 파일 목록 검사는 이전 단위의 같은 fixture/기대를 사용한다.

최종 실행 파일로 `cpppj/tests/fixtures/tell-map-display-ui.tsv`를 실행했다. 현재 PC의 보호 디스크433파일에서 SOLO50개/두 열, FORT1개/한 열, ESC 취소/재진입, SOLO 끝dirty1/fortSpec, FORT 끝dirty0/fortSpec을 같은 실제 UI 보고에서 확인했다. 1024×768 창 모드·음향 장치 없는18프레임 실행이 종료0이며 모든 require를 통과했다. 캡처된 SOLO/FORT 화면을 직접 확인했다. 첫/재진입 SOLO는 동일 파일·열 순서를 보존한다. 저장 게임 목록은 공통 파일/메뉴 부모 검사용이며 메인 Multiplayer 기능을 활성화하지 않는다.

첫/재진입 SOLO 패널(256,105)-(768,664)의 RGB 차이 영역은없음이며, 같은 두 보고의항목/버튼52개 입력 사각형·라벨·상태가 모두 같다. 이는 재진입의 안정성 검사이며 원본 게임 캡처와 픽셀 동일성을 증명하는 수치는 아니다.

보고서는 `extracted/cpp-tell-map-display-20261023-ui2.tsv`, 캡처는 같은 접두사의solo/fort/solo-again.bmp, 빌드/검사 로그도 같은 접두사다. GUI 실행은 호스트 예외VM-W11-Codex에서 복원 클론만 실행했고 원본 게임은 실행하지 않았다. 실행 직전 originals/d/options.cfg 바이트를 보관해 finally에서 복구했으며 보호 경로 변경은 없다.

변경 파일의UTF-8/추가 한국어 주석·diff·보호 경로 미변경을 확인하고 인수인계와 함께 **20261023-25 CP**로 로컬 커밋한다. 별도 배포/dotnet 커밋과 파일은 변경하지 않는다.

## 실제 잔여 범위

이번 연결은 목록 표시 어댑터다. **ReadFort의 pending/mode/다음State와 실제 Editor 월드는 아직 미연결**이며 기존 Supported 관문이 선택 행을 비활성화한다. 메인 Edit 버튼도 아직 NotImplemented다. 새 맵/NewFort·파일명 입력·전체 명령 표/80칸 인자 고리·원본 Tell Gump의 완전한 배치/다른 해상도 동일성은 완료로 세지 않는다. 패널의 기존 높이 제한도 그대로여서 긴 목록의 낮은 해상도 배치는 원본 Gump 부모로 교체할 때 검증/복원해야 한다.

정상 보호 파일은 기존 State를 읽으며 이번 실제 파일/GUI에서 빈 State 생성은 선택되지 않았다. 해당 경계에 필요한 원본의 미정 할당 바이트 장치가 없으므로 임의0을 공급하지 않고 선택 시 명시 거부한다. 파일 소실/빈 State의 독립 목록 검사는 자기 명시 할당 입력으로 계속 검증한다.

다음은 AskEditFort/ReadFort의 실제 입력 순서·nextState 덮어쓰기와 Editor 초기화 부모다. 자세한 c9/ca·pending/current 차이, State1의 필수 현재 월드 snapshot 및 Editor Player/Deck/기술 의존성은 [단위358 기록](cpp-tell-map-menus-20261023.md)에 남겼다. 전체 Early Missions·캠페인·데모3개 사용자 완주, Edit/커스텀 시나리오와 메인 메뉴/GUI·음향 동일성·배포 목표는 계속 남아 있다. 원본/CD·AGENTS/별도 배포·dotnet 파일 수정이나 원격 push는 없다.
