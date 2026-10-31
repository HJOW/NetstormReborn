# 메뉴 복귀 음악 색인과 현재 battle 조회 감사 — 673

2026-11-01, HJOW-Athlon. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md의 제한과 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23을 재확인했다. 이번 감사는 기존 정밀 C·현행 소스·root가 이미 만든 672 thunder-v2 보고서를 읽었다. src/tools/원본 수정, 새로운 게임·Native·검사·빌드·커밋 실행은 모두0이다. 허용된 이 감사 파일만 작성했다.

## 결론

**관측된 `menu.index=0 → 명시 music-next 3회 → index=3`은 원문에서 가능한 정상 색인 흐름이다.** 옛 스모크의 `menu 진입이면 index=3`, 따라서 세 번 Next 뒤2라는 고정 기대는 잘못됐다. 이를 제품에 맞춰 임의 허용할 문제가 아니라 원문 cleanup/State3의 실제 쓰기 순서에 맞춰 검사해야 한다.

**별개의 제품 연결 차이도 확정된다.** 원문 `469f60`은 곡 만료 조건을 통과한 시점마다 현재 `00594fbc`를 읽는다. 제품은 `StartScene(bool)`이 `SceneMusicState.battle`에 복사한 과거 값을 `Frame()`이 읽는다. Battle에서 State3으로 복귀하면 cleanup의 음악 Start는 아직 battle1에서 실행하고, 뒤 실제 owner만0이 된다. 따라서 음악 복사본은1이 남아 다음 곡 만료 때 메뉴 곡 대신 Next를 선택할 수 있다. 이 차이는 바로잡아야 하나, 현재 스모크의 **명시 Next3회 뒤 색인2 단언 실패의 원인은 아니다.** 원문 `469f00/Next` 자체에는 battle 조회가 없다.

## 원문 순서

### cleanup `004b2df0`

`extracted/refined/originals/Netstorm.c`에서 다음 순서다.

1. `142225`: `FUN_00469fc0()`.
2. `142256`: `DAT_00594fbc = 0`.

따라서 Battle1에서 cleanup을 시작하면 음악 Start가 읽는 값도1이다. 현행 `ClientStateWorldCleanup.cpp:65`의 `StartScene(c.surface.ownerMode.battle)`와 `o/RawStateWorldCleanup.cpp`의 SceneMusic effect 뒤 battle0 쓰기는 이 부분과 일치한다. 여기에0을 강제로 넘기거나 battle0 store를 음악 호출 앞에 옮기면 원문 난수 소비/색인/날씨 효과가 달라진다.

### State3 및 State7

- State1/2/3/6 공통 prefix는 `145714` cleanup → `145715` Chat → `145716` listener → `145717` fort1 → `145727` Renderer 초기화 → `145732` HUD1이다. State3은 `145740`에서 Guide를 해제한다.
- 공통 tail `145798`은 `FUN_00469db0("ser22.mus")`를 **직접 요청**한다. `469fc0/Start` 재호출이 아니므로 색인3을 다시 쓰지 않는다. cleanup 이후 State3의 battle1 쓰기도 없다.
- State7은 `145839` cleanup → 지역값 복원 → `145846` battle1 → `145847` `469fc0`다. 현재 `UberGump.cpp:660~666`도 이 순서를 유지한다.
- 현재 직접 대입으로 확인한 원문의 battle writer는 cleanup의0과 State7의1이다. 이것을 간접 alias writer 전체 부재의 증거로 확대하지 않는다.

`UberEditor.inc:47~58`, `:122~140`의 실제 Native State3 준비/직접 메뉴 음악 요청도 이 순서와 맞는다. parent cleanup의 호출 인자나 GuideEnd671에 편의 음악 전환을 추가할 근거가 없다.

### 음악 `00469fc0/00469f60/00469f00`

- `469fc0`, `86531~86541`: 현재 battle0이면 색인3과 메뉴 곡을 고른다. battle 비영이면 실제 게임 난수 `Next(4000)/1000`을 색인에 쓰고 `469f00`을 호출한다. 이어 `469f60`을 호출하고 현재 색인의 tint를 쓴다.
- `469f60`, `86505~86516`: 현재 실시간이 songEnd 이상일 때만 현재 `DAT_00594fbc`를 읽는다. 비영이면 Next,0이면 대기실 판정 뒤 anticipation 또는 ser22를 요청한다. **메뉴 Frame의 ser22 요청은 색인을3으로 바꾸지 않는다.** 만료 전에는 battle도 조회하지 않는다.
- `469f00`, `86478~86487`: 색인을 signed C 나머지 규칙으로 증가시킨 뒤 현재 player-ready/희생 판정 또는 해당 원소 곡/날씨를 요청한다. 이 함수에는 battle 조회가 없다. 따라서 메뉴의 검사 전용 `music-next`도 현재 색인에서 같은4곡을 순환한다.

## 현행 복사본의 실제 차이

`ClientAudio.cpp:122~124`는 `scene_.battle=battle ? 1U : 0U` 뒤 `sceneMusic_.Start()`를 호출한다. `SoundSceneMusic.cpp:99~102/108~115`는 Start와 Frame 모두 이 복사본을 읽는다. `ClientStateWorldGlobals.cpp:55`의 `00594fbc` writer는 현재 `surface.ownerMode.battle`과 `links.battle`을 바꾸지만 이 복사본을 바꾸지 않는다.

State3에서 cleanup의 Start는 원문대로 battle1을 읽어 새로운 색인을 고른다. 뒤 실제 owner가0이 되어도 복사본은1이다. State3 tail의 `RequestMusic("ser22.mus")`는 곡 이름/songEnd만 바꾸므로 복사본1이나 색인을 고치지 않는다. 그러므로 메뉴 곡이 끝날 때 현행 Frame은 Next로 잘못 갈 수 있다. `StartScene(false)`를 tail에 추가하면 색인3 및 추가 Frame/tint 효과까지 만들어 원문과 달라진다.

최소 제품 접합은 실제10.78 음악 감독이 **각 원문 소비 위치에서 현재 owner의 battle을 조회**하는 것이다. 독립/10.72의 기존 명시 state는 fallback으로 유지할 수 있다. `Start`의 최초 조건과 `Frame`의 만료 조건 뒤 조건을 각각 조회해야 하며, `SceneFrame` 입구에서 무조건 동기화하거나 Start 입구의 값을 Frame에 재사용하지 않는다. `Next`에는 새 battle 관문을 추가하지 않는다. 진단 `sceneBattle`도 연결된 현재값과 과거 fallback을 혼동하지 않게 해야 한다.

이번 범위의 수정 후보는 `SoundSceneMusic.h/.cpp`의 optional 현재 battle reader와 원문 두 소비 위치, `ClientAudio`의 같은 reader 연결 및 진단, `Client.InitializeNativeAudio`의 Patch1078 owner reader다. 실제 API/구현/검사/커밋은 root가 결정한다. `ClientStateWorldCleanup`의 현재값 전달/원문 writer 순서와671 GuideEnd 보존은 바꾸지 않는다.

## 기존 실제 관측 해석과 스모크 제안

`extracted/cpp-sound-current-view-672-live/thunder-v2/thunder-states.tsv`는 브리핑 색인3에서 네 번 Next로0/1/2/3, 천둥 색인2의 생성 및 뒤 반납을 기록했다. frame46/47의 main은 색인0, frame51의 menu-thunder는 색인3이다. 그 사이 명시 Next는3회여서 `(0+3)%4=3`과 맞는다. `thunder.txt`는 **재진입 후 최종 상태**이므로 그 파일의 `sceneBattle=1`을 메뉴 당시의 직접 관측으로 사용하지 않는다. 과거 battle 복사본 잔류 판단은 위 제품 writer/소비 코드에서 확정했다.

메뉴 부모 없는 천둥 요청을 약화하지 않는 스모크 교정은 다음과 같다.

1. 메뉴 도달 직후 실제 색인을 시작점으로 기록한다. 고정3을 기대하지 않는다.
2. 메뉴에서 **4회 Next를 각각 보고**해 매번 `(직전+1)%4`, 실제 menu/World 부재, thunder_processes0을 확인한다.
3. 이4회에서 색인2가 정확히1회 포함됨을 요구한다. 따라서 시작 색인이2여도 천둥 요청을 빼먹지 않으며 실제 부모 없는 천둥 분기를 반드시 지나간다.
4. 4회 뒤 색인이 메뉴 시작값으로 돌아오는지 요구하고, 기존 브리핑4곡·천둥 생성/반납·팔레트·재진입 단언은 유지한다.

단순히 실패값2를3으로 바꾸거나 색인 단언을 없애지 않는다. 별도의 현재 battle 접합 검사는 Start가 battle1을 읽은 뒤 실제 owner만0으로 바뀐 장면에서, songEnd 이전의 Frame은 조회하지 않고, 만료 뒤 Frame은 ser22 요청 경로를 고르며 색인을 변경하지 않는지를 명시 장치/현재 Client 경계로 검증하는 것이 적절하다. 실제 몇 분 대기·원본 게임·승패·새 멀티플레이 실행은 필요하지 않다.
