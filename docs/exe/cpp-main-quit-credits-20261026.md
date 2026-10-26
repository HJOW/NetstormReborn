# 메인 Quit의 원본 입력 큐 연결과 Credits 경로 점검

- 작업 PC: HJOW-Athlon, 2026-10-26.
- 마지막 전체 정밀 디컴파일: HJOW-Athlon / 2026-10-23. `extracted/refined/`의 소스는 변경하지 않았다.
- 범위: 메인 Quit의 직접 종료를 원본 RequestQuitApp → QuitApp 경로에 연결하고, 이미 복원된 Credits의 정확한 페이지·시간·복귀 경로를 확인한다.

## Quit의 원문 근거와 수정

원본 10.78의 메인 메뉴 생성자 `004cf100`은 Quit 버튼에 `00542944`(RequestQuitApp, `02000094`)와 매개변수0을 넣는다. `004cebf0`은 실제 버튼 사건에서 전체화면 시작 표시를 지운 뒤, 같은 명령을 일반 입력 큐에 넣는다. 일반 입력 `004d62b0`의 RequestQuitApp은 현재 Editor/fort/challenge 모드를 읽는다. Editor면 QuitEditMode, fort와 challenge가 모두0이면 QuitAbandon을 연다. 그 외에는 QuitApp(`02000001`, 두 번째 매개변수99)을 예약한다. QuitApp은 매개변수99를 확인하고 `selectTab.wav`를 재생한 뒤1을 반환한다.

기존 cpppj의 메인 Quit은 호스트 전환 큐에서 `quit_=true`를 직접 설정하여 이 과정을 생략했다. 수정 후에는 메인 버튼도 실제 일반 입력 큐에 RequestQuitApp을 넣고, 이미 복원된 `GameMenuActions`의 동일한 관문·효과음·종료 반환을 사용한다. 이전 호스트 Quit 명령도 RequestQuitApp으로 변환한다.

QuitApp이 종료를 반환한 같은 Input 배치에서는 호스트도 즉시1을 반환한다. 원본처럼 뒤에 남은 입력 사건을 더 꺼내거나 Tell 상태 동기화·UI 합성을 수행하지 않는다. 첫 Request만으로는 종료하지 않으므로 확인 창을 여는 모드의 정상 입력 수명은 그대로다.

원본 `004b6dd0`의 State1·2·3·6 공통 prefix는 정리와 로컬 연결 뒤 `00594fb8=1`을 쓴다. 메인 화면은 State3에 해당한다. 현재 호스트 MainMenu에는 이 확인된 fort writer를 같은 Surface 물리와 기존 Player 별칭에 연결했다. 월드가 없을 때 메뉴에 합성0을 제공하던 여섯 전역도 Client 수명의 실제 Editor/fort/challenge/missionPost/playerKnown/battleOwner 값을 읽는다. 전체 State3 부모가 복원됐다는 뜻은 아니다.

`Client::Run`의 종료 반환 후 화면 fallback → WM_CLOSE → WM_QUIT/0 흐름은 원본 WinMain의 같은 종료 경로를 이미 사용하므로 이번에 바꾸지 않았다. C++ 소멸자는 월드/음향/화면/창의 현재 소비자를 반납한다. WM_DESTROY의 전체 Native `004b3490` 정리와 전체 State3/7 연결은 별도 잔여 작업이다.

## Credits의 실제 원본 자료

원본 자산을 `NetstormCpp.exe --read-game originals d/tell.english`의 창 없는 읽기 전용 경로로 확인했다. 원본 파일이나 자산 복사본은 수정하지 않았다.

| 선택/페이지 | 다음 자동 페이지 | 시간 | 실제 버튼 |
| --- | --- | --- | --- |
| Credits | 자동 전환 없음 | 없음 | Netstorm 10.72 Patch Credits → CreditsNew, Netstorm Original Credits → Creditsold, Cancel → Blank |
| CreditsNew | CreditsNew1 | 20초 | More, Back → Credits, Cancel |
| CreditsNew1 | CreditsNew2 | 20초 | More, Back → CreditsNew, Cancel |
| CreditsNew2 | CreditsNew3 | 20초 | More, Back → CreditsNew1, Cancel |
| CreditsNew3 | Blank | 20초 | Back → CreditsNew2, Cancel |
| Creditsold | Credits1 | 20초 | More, Back → Credits, Cancel |
| Credits1~12 | 번호가1 큰 다음 Credits | 각20초 | More, Back → 직전 페이지, Cancel |
| Credits13 | Blank | 20초 | Back → Credits12, OK → Blank |

Credits1의 Back은 Creditsold다. Patch는4쪽/총80초, Original은14쪽/총280초이다. Credits·CreditsNew 계열·Creditsold는 `nsstart` 배경을 명시한다. Credits1~13은 배경을 새로 지정하지 않아 이전 배경을 유지한다. 문서와 화면 비교에서 페이지 제목·버튼의 대소문자를 보존한다.

현재 Tell → 1차 HTML/메타 → 같은 공통 창 → `HtmlWindowGump::Idle`의 벽시각 기한 → 일반 입력 큐의 Tell → 창 Kill/공통 Cleanup 경로가 이 데이터를 처리한다. Credits만의 새 타이머나 페이지 목록을 제품 코드에 추가하지 않았다.

## 검증과 남은 범위

최종 Release 빌드 경고/오류0, 기존 Native610을 포함한 게임 메뉴 집중8개와 전체 C++3,165개·실패0을 확인했다. 같은 게임 `c8739bd5…`의 메인 Quit 두 실행은 정상 종료0과 재실행을 통과했고 종료 뒤의200프레임 사건은 실행되지 않았다. Credits는 Patch4쪽·Original14쪽의 실제 버튼/Back/최종OK·메인 복귀/종료를 확인했다.20초 자동롤과 원본 픽셀/음향 전체 동일성은 이번 실행 범위 밖이다. [통합 문서](cpp-editor-runtime-integration-20261026.md), `cpppj/editor-runtime-integration-20261026.json`에 결과를 기록했다.

좁은 GUI 검증 경로는 다음과 같다.

1. 메인 Credits → Patch → More → Back → Cancel → 메인, 재진입 Original →20초 자연 timeout → Back → Cancel → 메인.
2. 같은 제품의 메인 Quit 실제 버튼 → 확인 창 없는 정상 종료0 → 프로세스·창 소멸 → 같은 데이터 복사본 재실행 → 메인 Quit 종료0.
3. 전체 자동 전환을 검사할 때는 Patch4쪽과 Original14쪽의 실제20초 기한을 각각 기다린다. 명시 시계 변경이나 직접 Tell 호출을 자연 자동 전환 증거로 세지 않는다.

Editor → MainMenu의 prepared 대여 반납은 Editor 모드를 보존할 수 있다. 월드 밖의 합성0을 제거하면 이 잔류가 드러날 수 있으므로 실제 메뉴 복귀의 전체 State3 정리를 후속에서 확인해야 한다. 이 문서의 확인된 fort writer만으로 Editor 모드0·전체 State3/WM_DESTROY 정리 성공을 주장하지 않는다.
