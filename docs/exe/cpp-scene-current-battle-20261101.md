# 곡 만료 시 현재 전투 상태 조회 — 단위673

2026-11-01 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 **HJOW-Athlon / 2026-10-23**이다. 이번에는 기존 정밀 C를 읽었으며 디컴파일 결과를 수정하지 않았다.

## 원문과 제품 차이

10.78 `00469f60`은 현재 벽 시각이 곡 종료 시각 이상일 때만 현재 `00594fbc`를 읽는다. `00469fc0`도 첫 조건에서 현재 값을 읽고, 곡 요청/날씨 뒤의 내부 Frame이 만료됐다면 다시 읽는다. `00469f00`의 다음 곡 선택에는 이 조회가 없다.

기존 제품은 `StartScene(bool)`이 복사한 값을 읽었다. 전투 정리 `004b2df0`은 음악 Start 뒤에 전투 플래그를0으로 쓰므로, 이후 메뉴에서 곡이 만료되면 남은 사본1 때문에 다음 원소 곡을 고를 수 있었다. State3의 직접 `ser22.mus` 요청은 Start 재호출이 아니며 색인3을 다시 쓰지 않는다.

원문 호출 순서와 난수 소비를 보존하면서 `SceneMusicState.currentBattle`의 선택적 조회를 두 소비 위치에 연결했다. 독립/10.72 소비자는 기존 명시값을 사용한다. `ClientAudio`의 진단도 같은 현재 조회를 사용하며, Shutdown은 연결을 반납하지 않는다.

실제 Client는 SurfaceStorage 생성/장치 연결 뒤, 최초 음악 Start 앞에서 같은 `ownerMode.battle`을 읽는 함수를 설치한다. shared_ptr를 캡처해 장기 소유자의 수명을 보존한다. 설치 전에는 가짜0 조회를 넣지 않는다. cleanup의 Start→battle0, State3의 직접 곡 요청, Guide/World 수명은 바꾸지 않았다.

## 준비본 채택

[준비본](prepared/scene-current-battle-673-20261101/README.md)의 실제 base4파일 SHA가 현재 입력과 일치했다. patch 적용 뒤5파일의 LF 정규화 SHA도 manifest의 제안값과 같았다. 최초 raw SHA 검사는 Git의 CRLF 변환 때문에 실패했으며, 내용 차이와 구별해 기록한다. 이후 실제 Client 연결·CMake 등록·추가 회귀1개를 구현했다. 보관본은 당시 미적용 기록으로 유지한다.

## 검증

Release 제품/전용 검사 빌드 종료0·경고0·오류0, 집중24개(새9·기존15) 실행/실패0이다. 기존 세 판본 SceneMusic9339행은 기존 독립 기대값의 C++ 재생이며 새 Native 실행이 아니다. [보고서](../../cpppj/scene-current-battle-20261101.json)와 `extracted/cpp-scene-current-battle-673-build-v1.log`·`-focused-v1.log`가 근거다.

원본1394·CD425·fixture779·AGENTS1의 총2599파일은 전후 경로/길이/SHA가 같다. 제품 SHA는 `a61a275b01048c7bb22b5a9480893442ddd1c854f4865fb005426edb6e5df153`이다. 실제 Run 연결은 컴파일과 정적 코드로 확인했으며 이번에 GUI 전환은 재실행하지 않았다.

 신규9개는 시각/NaN 관문, 효과 뒤 재조회, Next의 무조회, 진단/fallback, 실제 ClientAudio의 Start(true)→현재 owner0→직접 메뉴 요청→Shutdown→만료를 검사한다. 마지막 회귀는 창·장치 없는 명시 모드 입력이며 실제 GUI cleanup 전체 실행으로 세지 않는다.

기존 세 판본 SceneMusic 원문 관측 재생과 ClientAudio 검사도 함께 실행한다. 새 Native·fixture·MP 검사, 원본 게임, 전체34 진입, 자연 완주, 물리 입력/PCM 출력은 이 단위의 대상이 아니다.
