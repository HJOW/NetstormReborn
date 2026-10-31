# 위치 음향의 현재 snapshot·지도 영역 — 단위672

2026-11-01 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 **HJOW-Athlon / 2026-10-23**이다. 원문 10.78의 보호 PE·정밀 C를 읽었으며 디컴파일 산출물을 수정하지 않았다. 10.72 현재 전역의 전체 복원은 이번 대상이 아니다.

## 확인한 제품 차이

원문 위치 음향은 계산할 때마다 공용 snapshot의 카메라와 현재 Gump 지도 영역을 읽는다. 기존 제품은 GameWorld의 `soundViewChanged`가 넘긴 `SoundState.view` 사본을 사용했다. 실제 State3의 Renderer 초기화와 HUD 숨김은 올바르게 실행돼도 World가 반납된 뒤에는 그 사본을 갱신할 주체가 없어 이전 카메라·지도 영역이 남았다.

이 차이는 기존 `cpp_audio_smoke.check_view`의 메인 복귀 기대값을 원문과 대조하면서 확인했다. 제품의 관측값에 맞춰 기대값을 바꾸거나 `RetireWorldForState`에 편의 SetView0을 추가할 근거가 아니다. 현재 저장소를 원문 소비 위치에서 읽어야 한다. GuideEnd 단독은 [671](cpp-guide-release-20261101.md)대로 World·카메라·음향을 보존한다.

## 원문 저장소와 순서

| 원문 | 현재 조회·호출 순서 |
| --- | --- |
| `00497220` | `0059a91c/0059a920`의 snapshot 픽셀을 투영 좌표에서 뺀다. 현재 Renderer 포인터를 대신 읽지 않는다. PE의 `00497242/00497257` 직접 읽기를 확인했다. |
| `004c6b30` | `005ca9cc/005ca9d0/005ca9d4/005ca9d8`의 현재 지도 LTRB로 화면 안을 판정한다. |
| `004c6a40`, `004c6ac0` | 같은 현재 지도 영역에서 좌우·음량을 각각 계산한다. |
| `004a9c30` | 투영→화면 안 판정→현재 좌우→현재 음량→일반 재생 순서다. |
| `004a9860`→`004a9840` | 투영→화면 안 판정→상태/적재/재생 장치→현재 좌우→현재 음량이다. 앞서 투영한 점은 보존하되 장치 효과 뒤 지도는 다시 읽는다. |

State1/2/3/6의 공통 Fort prefix `004b6dd0`은 전투 정리→Chat 숨김·listener/Fort/현재 Core→`004993c0` Renderer 초기화→Chunk/옵션→HUD mode1 순서다. State3 Guide 해제는 그 뒤다. PE의 `004b71a6` 정리, `004b7204` Renderer, `004b722e` HUD 호출을 확인했다.

Renderer 초기화는 현재/목표 origin을0으로 쓰고 Renderer16DWORD를 `0059a900` snapshot에 복사한다. 따라서 cleanup 시작부터 카메라0이라고 취급하지 않고 실제 초기화 반환 뒤0을 읽는다. 일반 프레임도 `00499fe0` 뒤 같은 snapshot을 복사한다. 지도 영역은 `004c6530`의 divider/리사이즈 writer와 현재 HUD가 갱신한다.

정밀 C 위치는 `Netstorm.c:121512`, `134442`, `134460`, `134647`, `145714~145740`, `157862~158057`이다. 읽기 전용 조사 자료는 `extracted/cpp-audio-menu-view-672-audit.md`에 있다. 이 정적 대조는 새 Native 실행 결과가 아니다.

## 제품 연결

`SoundState.currentView`와 값 반환 `SoundPlayer::CurrentView()`를 추가했다. 한 번/반복 재생은 투영·판정·좌우·음량의 각 원문 위치에서 조회하며 함수 입구의 한 사본으로 고정하지 않는다. 반복 재생의 좌우를 음량보다 먼저 계산하도록 원문의 순서도 반영했다. 실제 버퍼 음량 설정→좌우 설정 순서는 유지한다.

`ClientAudio::BindCurrentView`가 같은 source를 연결하고 `View()`와 진단 보고도 이를 사용한다. 실제 10.78 Client는 자신의 `Camera().Snapshot().pixels`와 `UberGump::CurrentMapView()`가 읽는 실제 `RawButtonControls::MapViewRect()`를 공급한다. Gump 생성 전에는 초기 전체 화면 입력을 사용한다. World가 없는 State3 메뉴도 같은 현재 데이터를 읽는다.

기존 SetView는 독립 소비자와 이번에 현재 전역을 분석하지 않은10.72 어댑터에 남긴다. 실제 10.78 provider가 연결된 뒤 이 사본을 바꿔도 원문 현재값을 덮지 않는다. 소리 옵션·장치 재초기화는 source를 반납하지 않는다. 음악 스레드는 이 주 스레드 위치 조회에 접근하지 않는다.

## 옛 스모크의 입력·판정 교정

`cpp_menu_smoke`, `cpp_thunder_smoke`, `cpp_weather_smoke`, `cpp_audio_smoke`, `check_dialog_controls`의 제거된 Game 버튼·일반 Battle 이탈 가정은 실제 ESC→Game 하위→Leave Mission 행 클릭→ABORT의 Main Menu/Continue Mission으로 교정했다. 숫자 menu-command는 보고된 실제 행 좌표를 찾는 검사 입력이며 명령을 큐에 직접 주입하는 우회가 아니다.

원문 SETSIZE의 No는 화면과 저장값을 보존하고 Yes는 저장 뒤 종료한다. 새 공통 왕복은 No·Yes·크기 덮어쓰기 없는 새 프로세스와 독립 설정 reader를 구별한다. 기존 같은 프로세스의 즉시800→640→1024 기대는 유지할 수 없다. 전체화면 활성/창 복귀와 시작 시 실패 표식 제거도 현행 호환 동작에 맞췄다. Windows10/11의 DIB 호환 처리는 기존495/501의 제품 차이이며 이번에 새 독점 DirectDraw를 추가하지 않았다.

메뉴 GIF 전체 픽셀 비교는 cloudSpeed0의 명시 입력과 실제0 확인으로 유지한다. 동적 구름 전체 동일성으로 확대하지 않는다. 위치 음향은 실제 Alt 포인터 pan/Home과 현재 raw_uber 지도 영역을 비교한다. HTML 검사에서는 실제 부모·자식·serial·캡처·Warning 토큰·스택·등록표를 읽어 가짜1+2N과 고정 serial 증가 기대를 교체했다. Warning의 별도 루트와 HTML 가족 반납을 구별한다.

준비본 정적 리뷰에서 남아 있던 초기 fullscreenMarker1 기대도 바로잡았다. 실제 시작은 화면 모드 설정 전에 표식을 제거한다. 오디오 메인 복귀0 기대는 관측값에 맞춰 약화하지 않았으며 위 원문 현재 조회 차이를 제품에서 수정했다.

## 검증 결과와 한계

Release 제품과 전용 검사 빌드는 종료0·경고0·오류0이다. 집중30개(새 독립6·실제 Client2·기존22)를 한 번 실행해 실패0을 확인했다. 기존 SoundPlay4824행·SoundControl1063행의 세 판본 합계17661행은 기존 기대값의 C++ 재생이며 새 Native 실행/fixture가 아니다. 실제 Client2개도 창 없는 snapshot 변경·Renderer 초기화0·CD fallback 검사다.

빌드 로그는 `extracted/cpp-sound-current-view-672-build-v1.log`, 집중 로그는 `extracted/cpp-sound-current-view-672-focused-v1.log`다. 실제 GUI 관측 동안 제품 SHA는 다음 값으로 같았다.

```text
74b98b7726fd10072ab8b217e8beb0745ffb4aca35edee00cbb843f5a9e003a8
```

같은 제품의 최종 기능별 실행은 다음과 같다. 상세 집계와 로그 경로는 [검증 보고서](../../cpppj/sound-current-view-20261101.json)에 있다.

| 묶음 | 결과 | 범위 |
| --- | --- | --- |
| audio-view-v3 | GUI10프로세스 종료0 | 실제 Alt pan·pan 전후 같은 Home·메뉴의 카메라0·소리 off/on·800/640/1024의 No/Yes/새 프로세스·미션 재진입 |
| menu-v3 | GUI11·읽기 전용 CLI6 종료0 | 메뉴/UI38상태, 캠페인1875객체·튜토리얼826객체, 창/전체화면 왕복·옵션 저장 재시작·세 해상도. GIF831008픽셀을 Patch/CD 각각 비교하고 정적 메뉴 배경774432픽셀 비교 |
| dialog-v1 | GUI2프로세스 종료0 | 메뉴84상태·About/Blank32회, 미션21상태·바깥 뗌2회·캡처/Warning/모달 스택·같은 World 보존 |
| thunder-v3 | GUI2프로세스 종료0 | 일반/소리·음악 해제 각각22상태·캡처4개. 실제 메뉴4곡 순환에서 색인2는 한 번이며 부모·번개 효과는 없음 |
| weather-v2 | GUI10프로세스 종료0 | 옵션1/0/2/무음, 네 팔레트와 크기 승인/재실행. 장치 준비1·worker 실패0. 별도 복사 자료의 합성 COL이며 원본 RGB 동일성 증거가 아님 |
| audio-v2 | **전체 묶음 실패** | 완료된 menu/battle/options/disabled4그룹은 통과. 마지막 `view-1024-restart`가 options.cfg 저장 오류로 종료1. 위치/해상도10프로세스는 위 audio-view-v3에서 별도 통과 |

audio-v2 마지막 stderr의 `SaveOptions fopen errno=22 / doserrno=1224 / last_error=1224`는 기존 설정 저장 이슈의 재현이다. 원인은 미확정이며 H 볼륨 형식과의 인과로 단정하지 않는다. 전체 음향 묶음 성공으로 세거나 이 오류를 무시하도록 검사하지 않았다. 완료된4그룹은 실제 종료0의 원시 보고서를 같은 현재 단언으로 재평가해 `extracted/cpp-sound-current-view-672-audio-v2-completed-groups.json`에 기록했다. 이 재평가의 새 게임 실행은0이다.

최초 실패9묶음도 원시 로그에 보존했다. Home과 초기 사제 Reset의 차이, 실제 UserNotUpdated의 OK, 일반 Tell의 phase1/자기 정지1, 캠페인 controlled 행, 비동기 설치/Play 후 Tutorial 정지 해제 시점을 교정했다. 메뉴 색인3 고정 가정은 원문의 cleanup 재선택과 State3의 직접 ser22 요청을 대조해 실제4곡 순환 검사로 교체했다. audio-v1의 songEnd도 원문 cleanup의 직접0쓰기와 같은 이름 요청의 즉시 반환에 따라0을 확인한다. 이를 곡 길이로 임의 설정하지 않았다. 마지막 I/O 오류는 미해결로 남겼다.

전체 wrapper14회에서 실제 GUI 자식64프로세스·읽기 전용 C++ CLI12프로세스·GUI 비정상 종료2개였다. 이 수에는 최초 실패와 재실행이 포함되며64개 검사 통과나 전체34 회귀를 뜻하지 않는다. 저널 설치 전3프로세스는 각 wrapper 오류·상태/음향 보고서로 따로 계수했다. 모든 프로세스는 끝났으며 추가 재실행은 하지 않는다.

보호 범위 `originals`1394파일·`originalCD`425파일·`cpppj/tests/fixtures`779파일(TSV776)·AGENTS1파일의 총2599개는 전후 경로/길이/SHA가 같다. 허용 설정·표식도 원래 바이트로 복구했다. 지정 집합이 다른645의2632/773과 합치지 않는다. `extracted/cpp-sound-current-view-672-protected-before.json`과 `-after.json`이 근거다.

앱 내 검사 입력이며 물리 입력·청취/실제 PCM 검증은0이다. 원본 게임·새 Native·새 fixture·신규 MP·포획/희생·특정 적 공격·승리·공중3종 새 행동은 실행하지 않았다. 현재 제품의 전체34·전체 C++·새 ZIP은 만들지 않았다. 최신 전체34/ZIP은645의644제품이며 자연 완주와 전체 동일성은 사용자 확인 대상이다.

## 일시 중지 인계

사용자의 **진행 중 작업 완료·문서 반영·커밋 뒤 목표 일시 중지** 요청에 따라 단위672를 여기서 마친다. 전체 목표는 미완료이며 재개 지시 전 새 제품 작업을 시작하지 않는다.

병행 정적 조사에서 곡 만료 시 `00469f60`이 현재 battle을 읽는 별도 차이를 확인했다. 이는 이번 위치 음향 수정과 다르며 실제 제품은 여전히 StartScene의 과거 사본을 사용한다. 이미 진행 중이던 [673 준비본](prepared/scene-current-battle-673-20261101/README.md)만 Git에 보관했다. 원시8파일의 SHA와 격리 patch 재현5파일을 확인했지만 **제품 미적용·C++ 미빌드·새8검사 미실행**이다. 실제 SurfaceStorage 생성 뒤 최초 음악 Start 앞의 소유자 연결·CMake 등록은 재개 후 작업이다.

같이 보관한 시작 안내 조사는 기본 UserUpdated0이 원문의 Auto Updater 검증 결과임을 확인했다. 싱글/LAN은 허용되며 인터넷 MP의 구 검증 플래그를1로 위조하지 않는다. 현재 도구는 실제 OK로 안내를 닫는다.
