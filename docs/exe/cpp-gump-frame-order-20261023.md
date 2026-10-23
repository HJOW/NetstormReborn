# 10.78 Gump 입력 통지·삭제 정리·Idle의 실제 프레임 순서

## 기준과 조사 범위

2026-10-23, 현재 PC와 마지막 전체 정밀 디컴파일 PC는 모두 **VM-W11-Codex**다. `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 다시 확인하고 기존 `extracted/refined/originals/Netstorm.c`와 보호 PE `originals/Netstorm.exe`를 읽었다. PE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 이 문서는 단위370 빌드 대기 중인 제품 소스의 정적 감사이며 제품 C++·CMake·원본·기존 디컴파일 산출물을 바꾸지 않는다. 게임·GUI·빌드·검사는 실행하지 않았다.

결론은 **입력 중 마우스 통지와 갱신 중 Gump Idle을 분리하고, 공통 지연 삭제는 설정 저장 뒤·Idle 앞에서 실행해야 한다**는 것이다. 게임 시계 정지는 이 세 처리기의 공통 실행 관문이 아니다. 전 프레임 갱신을 막는 별도 Moviegump 잠금 깊이를 사용한다.

이 문서의 x86 주소는 보호 PE의 실제 호출·메모리 읽기 주소다. Capstone으로 기존 바이트를 정적 판독했고 게임 본체를 실행한 관측 fixture를 새로 만들지는 않았다. `Netstorm.c`의 불완전한 무인자 호출 복구보다 실제 인자 전달 바이트를 우선했다. 10.72는 기존 `GumpFrame`/`GumpLifetime`의 별도 판본 계약을 보존하며, 이번 프레임 배치는 10.78 기준이다.

## 원본 정상 루프의 확정 순서

주요 근거는 `Netstorm.c` 47200~47282와 같은 본문을 다시 복구한 47475~47559, PE `004399ef`~`00439b6e`다. 실제 명령 시작점에서 확인한 호출을 다음에 적었다. 중간의 복원되지 않은 State·환경음·다리·AI 등은 순서를 생략한 것이며 다른 단계로 합쳐도 된다는 의미가 아니다.

| 순서 | PE 호출 또는 분기 주소 | 실제 대상 | 실행 의미 |
|---|---|---|---|
| 1 | `004399ef` | `00460e90` | 이번 루프의 게임 시각·벽 시각을 고정한다. |
| 2 | `004399f4` | `004b88c0` | 예약된 State 전환을 반영한다. 현재 `UberGump::Tick`의 대응 자리다. |
| 3 | `004399f9` | `004a8e60` | 재생 중인 소리 수를 센다. 뒤에 네트워크 처리와 Windows 메시지 드레인이 온다. |
| 4 | `00439a90` | `00463f70` | 현재 Guide를 실행한다. 함수 본문은 `463bb0(&562ca0)` 호출이다. |
| 5 | `00439a95` | `004d62b0` | 폴링·호버·입력 큐·명령을 처리한다. 0이 아닌 반환은 바로 종료 분기로 간다. |
| 6 | `00439abe`~`00439ac4` | `[0054dc58] > 0` 검사 | Moviegump 갱신 잠금이 양수이면 일반 갱신 전체를 건너뛰고 `439b69`로 간다. |
| 7 | `00439ad3` | `00441de0` | 설정 변경이 있으면 `options.cfg`를 저장한다. |
| 8 | `00439b04` | `00469f60` | 배경 음악 전환을 처리한다. |
| 9 | `00439b09` | `00446c50` | 커서 모양·애니메이션을 갱신한다. |
| 10 | `00439b0e` | `00481320` | 현재 미니맵 dirty를 갱신한다. |
| 11 | `00439b13` | `004662b0` | 그때까지 Kill된 Gump의 실제 가상 소멸·해제를 실행한다. |
| 12 | `00439b18` | `00464170` | 현재 idle 표를 한 번 순회한다. |
| 13 | `00439b2f` | `00471a30` | Kernel의 현재 프로세스를 실행한다. 그 앞에는 격자 검증·AI 정리가 있다. |
| 잠금 대체 | `00439b69` | `00464170` | 잠금이 양수여도 **Idle은 한 번** 실행한다. 일반 갱신·Cleanup·Kernel은 실행하지 않는다. |
| 14 | `00439b6e` | `00436450` | 프레임 제한과 그리기를 처리한다. 양쪽 갱신 가지가 이곳에서 합쳐진다. |

따라서 게임 시계가 정지해도 일반 갱신 목록에 진입할 수 있다. 반대로 Moviegump 잠금 중에는 게임 시계 상태와 무관하게 SaveOptions·미니맵·Cleanup·Kernel을 건너뛴다. Guide와 입력은 그 검사보다 앞이므로 두 가지 상태에서도 계속 실행된다.

QuitApp99는 `004d62b0` 안에서 바로 1을 반환한다(`Netstorm.c` 168841~168847). 호출자는 `00439a9e`~`00439ab3`에서 fallback 모드와 WM_CLOSE를 요청하며 정상 저장 분기로 가지 않는다. WM_CLOSE의 `4a4810`, WM_DESTROY의 `4b3490` 및 WM_QUIT 반환에는 추가 설정 저장 호출이 없다. 실제 설정 파일 쓰기 `440f30`의 전체 직접 호출자는 초기화용 `441d10`과 정상 프레임용 `441de0`뿐이다. OnExit Quit를 현재 입력 드레인에서 즉시 생성·소비하면 해상도 Config가 저장되기 전에 종료되는 차이가 생긴다.

## 서로 다른 두 정지 소유자

| 원본 상태 | 변경 소유자와 근거 | 막는 대상 | 계속 실행하는 대상 |
|---|---|---|---|
| `0055b4b0` 게임 시계 정지 깊이 | `460df0`은 0→1에서 게임 시각을 보관하고 깊이를 증가시킨다. `460e30`은 감소 후 0일 때 정지 시간을 보정한다. `460de0`은 signed 양수 여부를 읽는다. | `460cd0`이 돌려주는 게임 시각의 전진. | 벽 시각 `460d70`, Guide·입력, Gump Cleanup·Idle, 미니맵과 Kernel 호출 자체. 개별 로직은 멈춘 게임 시각을 보고 자신의 작업 여부를 결정한다. |
| `0054dc58` Moviegump 갱신 잠금 깊이 | `435dc0`의 `435dc6` 읽기, `435dce` 증가 저장, `435dd5` 감소 저장. 실제 호출자는 `483680`과 `4837f0` 두 개다. | main의 일반 갱신 목록 전체. 저장·미니맵·Cleanup·Kernel도 포함한다. | 시각 고정·State·메시지·Guide·입력, Gump Idle, 그리기. |

Moviegump의 `483680`은 `this+0xa8 != 0`이면 `004837c9`에서 `435dc0(1)`을 부른 뒤 `004837d1`에서 `460df0()`도 부른다. 대응 `4837f0`은 같은 필드 조건에서 `00483875`의 `435dc0(0)` 뒤 `0048387d`의 `460e30()`을 부른다. `Netstorm.c` 106029~106108과 `Moviegump.cpp` 진단 문자열·Smacker 호출이 소유 근거다. 영상 창이 두 깊이를 함께 소유할 수 있다는 사실과 두 전역의 역할이 같다는 주장은 구별해야 한다.

현재 `Client::Paused()`/`GameClock`는 첫 번째 계약이다. 이를 두 번째 상태 대신 쓰면 Pause 메뉴·브리핑·스크립트 Pause에서 삭제가 계속 미뤄진다. 원본은 이 경우에도 Cleanup을 실행한다. 향후 영상 바인더는 두 상태의 push/pop을 각각 연결해야 하며, 게임 정지를 해제한다고 Moviegump 갱신 잠금을 임의로 0으로 만들면 안 된다. 원본 `435dc0`에는 0 포화 보정도 없다.

## 입력 안의 마우스 통지와 호버

원본 `464100` Broadcast와 `464170` Idle은 서로 다른 함수이며 실행 위치도 다르다.

`004d69db`~`004d69f4`는 현재 폴링 사건의 code/x/y를 스택 값으로 복사한 뒤 `464100`에 전달한다. `464100`은 등록표 `563e3c`부터 루트 칸을 제외하고 현재 count에 도달할 때까지 순회하며, visible·notify(0x100)·비 dead 노드만 virtual `+0x2c`로 통지한다. 이 호출은 일반 사건 큐 Pop `004d6da8` 및 그 뒤 Guide `004d6e6b`→Gump Route `004d6e92`보다 앞이다. 첫 Guide용 폴링 통지 등 입력 함수 내부의 다른 준비 단계도 있으므로 Broadcast를 Guide 전체 앞에 옮긴다는 의미는 아니다.

현재 `RawButtonControls::Poll`은 `Broadcast(0,x,y)`와 `Idle()`을 한 메서드에서 순서대로 호출하고, `UberGump::Input`은 사건 큐를 모두 비운 뒤 이것을 실행한다. 따라서 현재 배치는 다음 두 차이가 있다.

1. Broadcast가 늦어 입력 콜백이 새로 만든 노드까지 같은 입력 단계에서 통지할 수 있다. 원본에서는 그 노드가 다음 프레임의 마우스 통지부터 참여한다. 원본은 실제 현재 사건 code도 전달한다. `ButtonGump::Broadcast`는 현재 event를 사용하지 않지만 모든 후속 HUD 가상 처리기까지 0을 전달해도 같다는 근거는 없다.
2. Idle이 빨라 설정 저장·미니맵·실제 Cleanup보다 먼저 실행된다. 메뉴 확장, 버튼 반복, 미니맵 드래그, 비용 표시 수명과 생산 패널 유지의 관측 순서가 함께 달라진다.

오버레이 호버 `466150`도 별도 경계다. UserInput `004d68c2`는 현재 dialog/조작 모드·커서 활성 등 원래의 분기를 통과한 경우에 이를 호출한다. 함수는 `563ca8`~`563e38`의 100칸을 순서대로 검사해 visible·비 dead·모든 부모 visible·Bounds 포함·현재 전체 Pick 일치인 첫 대상을 찾고, `004661d6`에서 **현재 전역 `5427e4`**를 읽어 `004661ec`의 virtual `+0x28`로 전달한다. 호출자가 넘긴 event 인자를 그대로 사용하는 함수가 아니다. 첫 일치 대상의 반환이 0이어도 다음 후보로 넘어가지 않는다.

제품의 `GumpInput::Overlay`는 이 본체를 이미 갖고 있지만 현재 client 소스에는 호출자가 없다. `UserInput::UpdateCursor`는 월드 커서만 갱신하며, `GumpControlInput::Move`는 기존 직사각형 메뉴 강조 상태를 계산한다. 이 두 동작을 원본 오버레이 호버의 실제 연결로 간주하면 안 된다. HUD 바인더에 Overlay 노출 API를 추가하되, 무조건 매 입력에 호출하는 대신 원본 UserInput의 dialog·모드·커서 분기를 같은 현재 상태로 연결해야 한다. `input_.Move`의 기존 목록 강조는 남아 있는 임시 메뉴 표시 기능이므로 해당 메뉴가 raw로 교체될 때 범위를 줄인다.

폴링 밖 상태에서 `RawButtonControls::Cancel`이 만드는 합성 release와 원본 UserInput의 창 안팎 전환/버튼 보정은 아직 완전히 같은 경로가 아니다. Poll 분리와 함께 무조건 release 정책까지 확정된 원본으로 승격하지 않는다. 마우스 좌표는 코드·x·y의 같은 스냅샷으로 Broadcast에 전달하고, 버튼 Idle의 실제 poll 훅은 원본처럼 필요한 시점에 현재 상태를 다시 읽도록 보존한다.

## 현재 제품 배치와 남은 차이

조사 시점의 `ClientMain.cpp` 337~367, `UberGump.cpp` 864~902, `RawButtonControls.cpp` 206~210, `RawMenuControls.cpp` 486을 기준으로 한다.

| 현재 위치 | 현재 실행 | 판정 |
|---|---|---|
| Client Capture 다음 | `UberGump::Tick` | 예약 State와 UI 갱신의 기존 대응 자리다. 이 작업을 Idle 위치로 통째로 옮기면 State가 메시지·Guide보다 뒤로 밀린다. |
| 메시지 드레인 다음 | `MissionFrame` | Guide의 원본 main 호출 위치와 맞는다. |
| Uber Input 시작 | 월드 `UpdateCursor` | 현재 월드 커서 어댑터다. 원본 입력 내부 세부 순서는 별도 연결 범위다. |
| Uber Input 중 | 모든 사건 Event 처리 | 현재 Guide→공통 Gump→명령 우선순위를 보존해야 한다. |
| Uber Input 드레인 뒤 | `warnings_->Idle()` | 독립 Warning 표의 Idle 뒤 Cleanup까지 한 번에 실행한다. 원본의 Cleanup→Idle 순서 및 전역 단일 표 순서와 다르다. |
| 그 다음 | `!client_.Paused()`일 때 메뉴 `CleanupFrame()` | 실제로는 같은 RawButtons 공통 lifetime을 정리하지만 실행 시점과 관문이 다르다. |
| 그 다음 | RawButtons `Poll()` | Broadcast/Idle 결합과 위치가 다르다. |
| 그 다음 | legacy `input_.Move`, 월드 `Tick` | legacy 호버 및 카메라 폴링 어댑터다. Gump Idle과 함께 옮길 대상이 아니다. |
| Client Input 정상 반환 뒤 | SaveOptions→SceneFrame→RefreshMinimap | 상대 순서는 맞다. 향후 Moviegump 잠금의 일반 갱신 가지 안에 둬야 한다. |
| 미니맵 다음·Kernel 앞 | `CleanupDialogs()` | 단위370 논리 HTML 부모의 지연 OnExit 예약은 원본 Cleanup 위치에 맞는다. 현재 전체 Gump 표/큐와 합쳐졌다는 뜻은 아니다. |
| Kernel 뒤 | `UberGump::Frame` | 월드 변경·렌더 합성의 호스트 경계다. 원본 Idle의 대응 위치로 사용하면 Kernel보다 늦어진다. |

특히 `WarningMessages::Idle`은 `frame->Idle(); lifetime->Cleanup();`이다. 원본 Idle에서 막 Kill된 노드는 통상 다음 프레임의 Cleanup까지 실제 소멸되지 않지만, 현재 Warning은 같은 호출에서 바로 소멸한다. 명시적 State 종료·Reset/파괴에서 원본이 별도로 Cleanup하는 경로는 이 정상 프레임 규칙과 구별한다.

## HUD 바인더 통합 시의 구체 변경안

아래 API 이름은 설계안이며 이번에 제품에 추가하지 않았다.

1. **RawButtons의 프레임 처리기를 세 경계로 분리한다.** `NotifyPointer(InputEvent)`는 현재 폴링 스냅샷의 저장과 `GumpFrame::Broadcast(event.code,x,y)`만 담당한다. `CleanupGumps()`는 같은 lifetime의 `Cleanup()`만, `IdleGumps()`는 같은 frame의 `Idle()`만 실행한다. 기존 Poll의 Broadcast+Idle을 호출부에서 추가로 병행하면 안 된다. 기존 단위 검사 편의를 위해 Poll을 남기더라도 실제 Client 경로는 새 단계만 사용한다.
2. **입력 준비에 NotifyPointer를 둔다.** Guide의 폴링 통지·현재 커서·조건부 Overlay를 처리하는 원본 UserInput 위치를 연결한 뒤, 일반 사건 큐를 비우기 전에 한 번 호출한다. 현재 `rawButtons_->Poll(mouse)` 뒤쪽 호출은 제거한다. 마우스 보정·legacy 호버·카메라 Tick은 자신의 입력 단계로 남기며, Gump Idle 이동과 묶지 않는다.
3. **Client에 별도 MovieUpdatePause 현재 깊이를 연결한다.** 기본 BSS0의 현재 깊이를 보유하되 실제 Moviegump 소유자가 push/pop한다. Input 정상 반환 후 현재 깊이를 검사한다. 0 이하인 일반 가지에서 SaveOptions→기존 갱신→미니맵→공통 Cleanup→공통 Idle→기존 AI/Kernel 순서를 사용한다. 양수 가지에서는 이 목록 대신 공통 Idle만 한 번 호출한다. Guide·입력·Frame 그리기는 가지 밖이다. `Client::Paused()`는 이 관문에 사용하지 않는다.
4. **현재 Input 안의 CleanupFrame을 제거하고 같은 공통 큐를 Client Cleanup 위치로 옮긴다.** 메뉴 전용처럼 보이는 이름과 달리 `RawMenuControls::CleanupFrame`은 `common.lifetime.Cleanup()`을 호출하므로 버튼·생산·미니맵 등 공유 노드도 처리한다. 클래스별 Cleanup을 연달아 중복 실행하지 않는다.
5. **논리 HTML, Warning/MessageBoard와 남은 HUD를 같은 저장소에 묶는다.** 최종적으로 동일 GumpCreate/등록표/idle 표/kill 큐에 파생 분배만 추가해야 원본 생성 순서와 콜백 중 표 재조회가 보존된다. 단위370의 `retiredPageExits_`와 Warning의 별도 저장소를 잠시 유지한다면 Cleanup/Idle 메서드를 분리해 같은 Client 단계에 둘 수 있으나, 이는 순서 어댑터이며 전역 슬롯 간 완전 일치가 아니다. 서로 다른 큐를 drain하는 순서만 임의로 정해서 원본 전체 동일성 완료라고 표시하지 않는다.
6. **OnExit는 실제 반납 단계에서 한 번만 정수 큐에 예약한다.** 이미 파싱 시점에 command/parameter를 resolve한 값을 쓰며 Dispose 전에 보관 상태를 비운다. Cleanup 중 생성한 Quit를 같은 Input 드레인에서 다시 처리하지 않는다. 공통 큐로 합친 뒤에는 가상 소멸에서 `cf1c0`과 같은 순서로 예약하여 원본 kill 큐 순서에 자연스럽게 포함시킨다.
7. **새 이벤트 생성 뒤 별도 재드레인을 넣지 않는다.** Cleanup·Idle이 생성한 명령은 다음 프레임에 State/Guide를 거친 입력에서 처리된다. 한 프레임 내 UI 반응을 빠르게 하려는 이유로 이를 Input 직후 소비하면 저장/OnExit/버튼 반복 순서가 다시 달라진다.

정상 루프의 형태는 다음과 같다. 의사 코드의 기존 갱신 생략 부분에는 원본 순서대로 후속 복원 함수를 끼운다.

```text
CaptureTime → StateTick → Sound/Network → WindowsMessages → GuideFrame
UserInput:
  현재 폴링/Guide 폴링 통지/커서/조건부 Overlay
  NotifyPointer(code,x,y)
  툴팁·추가 폴링 등 원본의 입력 준비
  일반 사건 큐: Guide → 공통 Gump → 각 명령
  종료 반환이면 fallback/WM_CLOSE; 아래 갱신은 실행하지 않음
if MovieUpdatePauseDepth <= 0:
  SaveOptions → … → Scene/커서 → Minimap
  CleanupGumps(HTML OnExit 포함)
  IdleGumps(공통 표 한 번)
  … → Kernel → 후속 갱신
else:
  IdleGumps(공통 표 한 번)
FrameDraw → 루프 후속 처리
```

## 부작용과 필수 회귀

단순히 Poll 한 줄만 뒤로 옮기면 Broadcast도 뒤로 밀리므로 해결되지 않는다. 반대로 현재 Poll을 앞에 추가하면 Idle을 두 번 실행하고 반복 버튼·생산 유지·미니맵 드래그가 두 번 처리된다. 아래는 단계 분리 뒤 필요한 의미 있는 회귀다. 이번 조사에서는 실행하지 않았다.

| 회귀 | 검증해야 할 실제 효과 |
|---|---|
| 단계 순서 기록 | Capture/State/Guide/입력 Broadcast/사건 Route/Save/Minimap/Cleanup/Idle/Kernel을 현재 Client 연결로 관찰하고 입력·Idle을 각각 1회 확인한다. 단순 mock 함수 호출 순서만 맞추는 검사로 끝내지 않는다. |
| 해상도 Yes 종료 | 실제 Tell SetSize2의 Config 2개→MenuJustClosed→HTML Cleanup→OnExit QuitApp99를 거쳐 종료 전에 실제 저장 파일을 읽고 SCREENW/H가 800/600인지 확인한다. No는 종료/OnExit를 새로 만들지 않는다. |
| 정지 중 raw 메뉴 닫기 | GameClock 정지 상태에서 ESC/메뉴 교체로 Kill한 노드가 같은 정상 Cleanup에서 등록 해제되는지 확인한다. 등록 id와 serial을 의도적으로 다르게 하고 serial 조회에서 dead가 즉시 제외되는 계약도 유지한다. |
| Moviegump 깊이 | 깊이 1·2에서 Guide/입력/Idle/그리기는 유지되고 Save/Minimap/Cleanup/Kernel은 멈추는지, 한 번 pop 뒤 여전히 잠겨 있는지 확인한다. 게임 Pause 깊이도 따로 보존한다. |
| 입력 중 생성 메뉴 | 큐 사건으로 새 메뉴를 만든 프레임에는 그 노드가 이미 끝난 Broadcast를 받지 않으며, 같은 프레임 후속 Idle에는 현재 표 순서에 따라 참여하는지 확인한다. |
| Cleanup/Idle 재진입 | Cleanup 소멸자가 추가 Kill을 넣으면 원본처럼 재조회된 killCount까지 같은 Cleanup에서 처리한다. Idle이 Kill한 노드는 dead/표 해제는 즉시 반영하되 실제 소멸은 다음 Cleanup으로 남는다. |
| 버튼 캡처/반복 | 누른 채 경계 안팎 이동·창 밖 release·repeat 만료에서 current code/x/y 통지와 현재 poll 재조회가 유지되고 repeat가 1프레임 2회 발생하지 않는지 확인한다. |
| 미니맵 드래그 | 입력 capture, current poll Idle, 같은 프레임 Kernel의 카메라 소비를 확인한다. 게임 Pause에서도 드래그하고 MovieUpdatePause에서도 Idle만 계속되는 계약을 구별한다. |
| 생산·비용·힌트 | 공통 Idle 1회에서 현재 금액 보간·blink·maintenance가 한 번 실행되고 벽시계 만료 Kill의 실제 소멸이 올바른 Cleanup으로 넘어가는지 확인한다. |
| Overlay/legacy 호버 | 두 overlay가 겹친 경우 표 순서·부모 visible·Pick 일치·첫 반환0 종료를 확인한다. 모달 Guide/HTML 뒤 월드 선택과 단축키가 새로 통과하지 않는지, 기존 목록 강조가 사라지지 않는지도 확인한다. |
| 월드/페이지 교체 | 같은 등록표에서 페이지 버튼만 정리하는 preserveWorldUi, 실제 월드 종료의 전체 수명 반납, Player 하위 메뉴의 참조 수명과 Node 캐시 조회가 유지되는지 확인한다. 프레임 cleanup 이동만으로 State 파괴 경로의 명시 Cleanup을 제거하지 않는다. |

기존 회귀 기반으로는 `GumpFrame_OriginalPatch`, `GumpFrame_IdleEditionLimitAndUnfilteredNodes`, `GumpFrame_BroadcastCurrentCountAndRootExclusion`, `GumpLifetime_TreeReplacementDefersDestruction`, RawButtonControls의 capture/serial 검사와 RawMenuControls의 horizontal/deferred replace 검사를 재사용할 수 있다. 이들은 본체와 일부 바인더 계약을 확인하지만 Client 전체 순서·Moviegump 소유 깊이를 대신 증명하지 않는다.

## 남은 경계

현재 공통 RawButtons 표와 별도 Warning·논리 HTML 부모가 실제로 하나의 전역 Gump 표/kill 큐/idle 표에 합쳐진 상태는 아니다. Moviegump의 실제 생성·open/close와 `54dc58` 소유 바인딩도 별도 작업이다. Overlay의 본체 복원과 실제 UserInput의 조건부 호출 연결은 구별한다. 기존 카메라·선택·커서 입력의 근사 경계를 이 프레임 순서 조사로 완료 처리하지 않는다.

단위370은 HTML OnExit 파싱·수명·저장 뒤 예약이라는 좁은 결과로 판정한다. 이번 문서의 전체 HUD 순서 변경안은 후속 구현 계획이며, 제품 코드는 조사 전 상태로 동결했다.
