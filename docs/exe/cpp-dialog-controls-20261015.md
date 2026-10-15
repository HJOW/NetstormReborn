# 일반 Tell의 페이지·입력 트리 교체

2026-10-15, `VM-W11-Codex`. **현재/마지막 정밀 디컴파일 PC는 VM-W11-Codex**다. 이번에는 같은 PC의 기존 정밀 C를 읽었으며 새 디컴파일 내보내기와 독립 원본 실행 대조는 수행하지 않았다. REA 6.5.0의 현재 세션/공급자 가용성은 확인했으나 새 대상은 열지 않았다.

일반 Tell이 실제 Kernel 실행 중 페이지를 바꾸어도 그 프레임의 입력 영역·명령·raw 돌 버튼을 함께 교체하도록 수정했다. 페이지를 바꾸지 않는 Warning과 표시 갱신은 기존 캡처와 버튼 serial을 유지한다. 이 완료 범위는 UI 교체이며 Early Missions/캠페인 1-1~1-2 완주 동일성의 완료를 뜻하지 않는다.

## 원인과 원본 근거

기존 `UberGump::Tick`은 `Compose()`의 기본 인자 `controls=true`로 입력 트리를 교체했지만 `Input`/Kernel 뒤 `Frame`은 `Compose(false)`를 사용했다. `Tell`은 어느 호출 시점에서든 페이지·phase와 `rebuild_`만 바꾸었다. 따라서 Kernel 중 일반 Tell을 받으면 새 페이지를 그리면서 이전 페이지의 `labels_`·`actions_`·영역 및 raw 버튼을 사용했다. 새 버튼 ID가 이전 ID에 없으면 예외, 있으면 이전 라벨과 명령 재사용이 가능했다. 반대로 다음 Tick에서 표시만 갱신해도 기존 캡처를 취소하고 버튼을 전부 새로 만들 수 있었다.

다음 근거를 우선 확인했다.

| 근거 | 확인 범위 |
| --- | --- |
| 정밀 C `004cf730 → 004cf270`, `004cf960` | Tell 섹션 선택과 일반 대화상자 생성 진입이다. |
| `004cf120`, `004c83b0`, `004c7aa0`, `004cf270` | 새 대화상자 생성, 현재 본문의 Button/Menu/Timeout 해석, 현재 텍스트 설정·부착/활성화다. 이전 화면의 버튼 라벨·명령 배열을 재사용할 근거가 없다. |
| `004cebf0` | 메인 메뉴 click은 현재 버튼의 실제 serial과 비교하며 해당 명령을 보낸다. 화면 인덱스와 Gump serial은 별개다. |
| 기존 `GumpLifetime`·`RawButtonControls` | 버튼 교체는 자식 Kill/Cleanup, capture/idle 해제, 같은 루트/등록표 및 증가하는 serial을 사용한다. |
| 기존 `MenuGump`·`GumpInput`·`ButtonGump` 검사 | 목록 누름 즉시 실행, 돌 버튼의 캡처/부모 통지, 페이지 삭제 수명의 기존 독립 원본 근거를 유지한다. |
| [단위35 Warning](cpp-run35-20261015.md) | `004d88c5 → 004d3090(17,0,1,0,text)`는 비모달 안내다. 일반 Tell의 controls 교체와 구분한다. |

원본 근거 파일은 `extracted/refined/originals/Netstorm.c`다. 대응 CD 주소는 기존 정밀 C의 주석에 `004cf730→004c3a20`, `004cf270→004c3430`, `004cf120→004c2210`, `004c83b0→004c2e00`로 기록되어 있다. **이번 작업에서는 CD 대응 함수를 다시 검증하지 않았다.**

## 변경 내용

- `rebuild_`는 픽셀 합성, 새 `controlsChanged_`는 페이지·입력 트리 교체 예약을 나타낸다. `Tick`/`Input`/`Frame`은 모두 같은 `Compose()`가 이 예약을 소비한다.
- 일반 Tell/Blank·미션 진입/브리핑·메인 메뉴/팝업·옵션/해상도의 실제 페이지 변경은 `RebuildControls()`를 호출한다. 교체할 때 라벨·명령·영역과 실제 raw 버튼을 한 번에 갱신하고 새 노드로 최종 합성한다.
- Warning-only Tell·월드 변화·팔레트/단순 `Refresh`·버튼 눌림/hover 표시 갱신은 입력 트리를 교체하지 않는다. 새 페이지 예약 뒤 같은 Kernel 프레임에 Warning이 와도 페이지 교체 예약을 유지한다.
- `--ui-script` 검사에 `kernel-tell`과 `refresh`를 추가했다. 전자는 필요할 때 한 번 등록한 `UiKernelTell` 프로세스에서 실제 Kernel 순회 중 일반 `Tell`을 호출한다. 후자는 화면을 그린 뒤 다음 Tick의 표시 갱신만 예약한다. 일반 실행 및 이 연산을 쓰지 않는 기존 UI 스크립트는 검사 프로세스를 생성하지 않는다.
- [새 집중 검사](../../tools/check_dialog_controls.py)는 실제 메뉴와 1-1 월드에서 버튼 사건을 보내고 같은 그리기 영역·raw 등록/serial·정지·명령 결과를 읽는다. `InspectTell`은 이 검사 호출 시점만 제공하며 일반 게임 입력에는 노출하지 않는다.

## 검사 결과

최종 Release 빌드는 경고/오류 0, `git diff --check` 통과다. 전체 CTest는 상위 통합 단계에서 실행한다.

| 검사 | 결과 |
| --- | --- |
| 새 실제 GUI 집중 검사 | 메뉴 **82상태** + 실제 1-1 **13상태**, 실패 0 |
| 연속 Kernel 페이지 교체 | About↔Blank **32회**, 현재 노드 수 고정·serial 증가·capture/idle 0 확인 |
| 일반 Tell | 메뉴/목록/팝업/실제 미션에서 교체, 새 Cancel/OK/Stay/목록 명령 실행, 이전 release 거부 |
| 비모달 Warning | 메뉴/미션/기존 모달에서 페이지·정지·캡처·등록/serial 유지, 기존 캡처 release 명령 정상 실행 |
| Tick의 단순 Refresh | 누른 돌 버튼의 capture/idle/serial/등록 수 전부 유지 |
| 기존 `cpp_menu_smoke.py` | **54상태**, 캠페인/Early 브리핑·복귀, 800×600/640×480/1024×768 전환, 재시작 설정 저장 통과 |
| 기존 raw 돌 버튼 | **4개**, 실패 체크 0 |
| 기존 `Gump_` 필터 | **15개**(MenuGump·ButtonGump 포함), 실패 체크 0 |
| 기존 `Warning` 필터 | **6개**, 실패 체크 0 |
| 보호 원본 | `originals`·`originalCD` 전체 파일명/해시 동일, 원본 게임 실행 **0회** |

집중 검사에서 `Credits`와 실제 미션 위 `LeaveNormal`의 화면을 저장하고 눈으로 확인했다. 본문/현재 버튼 라벨이 정상 표시된다. 픽셀 전체의 원본 동일성을 새로 대조한 것은 아니다. 기존 메뉴 검사의 배경 **774,432픽셀** 대조와 두 판본 각각 GIF **831,008픽셀** 대조는 통과했다.

다음 임시 변이 **3개를 모두 실제 GUI 실패로 검출**하고 소스 바이트·최종 빌드를 복구했다.

1. 일반 Tell의 입력 트리 교체 예약 누락 → 새 `Cancel` 영역을 찾지 못해 실패했다.
2. Tick이 표시 갱신도 강제 controls 교체 → capture/등록 보존 판정이 실패했다.
3. Warning-only Tell도 controls 교체 → Warning의 capture/등록 보존 판정이 실패했다.

검증 자료는 `extracted/dialog-controls/`의 `report.json`·상태 TSV·화면, `extracted/dialog-controls-mutations/`의 개별 실패/빌드 로그·`report.json`, `extracted/dialog-controls-{focused,menu,rawbuttons,gump,warnings}.log`다. 마지막 복구 빌드는 `extracted/dialog-controls-mutations/restored-build.log`에 있다. 세 임시 변이는 최종 소스에 남지 않는다.

## 남은 경계

- Warning과 RawButtonControls의 Gump 루트·전역 serial 통합, 다른 메시지 생성자/전체 창 수명은 이번에 구현하지 않았다.
- 실제 튜토리얼 B./C. 등의 자동 발생·일반 Mission 상태의 미션 섹션 조회·승패/진행 저장을 새로 복원한 것은 아니다. 이번 일반 Tell은 원본 `tell.english`의 현재 지원 섹션으로 검사했다.
- CD 일반 Tell UI·모든 해상도에서의 Kernel 중 교체·전체 대화상자 레이아웃/픽셀/음향·원본 실행 동일성은 별도 대조가 필요하다. 기존 메뉴의 해상도 변경 회귀는 위 범위대로 통과했다.
- 전체화면 DirectDraw 오류는 사용자 지시대로 수정하지 않았다. dotnetpj·AGENTS.md·보호 원본은 변경하지 않았다. 상위 작업자가 LEFT_JOBS.md를 갱신하고 작업 단위 로컬 커밋을 수행한다.
