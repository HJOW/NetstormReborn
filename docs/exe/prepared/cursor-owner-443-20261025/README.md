# 443 커서 장기 데이터와 실제 Cancel 연결 준비본

이 자료는 **main 미적용, CPP 미빌드·미검사**인 별도 준비본이다. 사용자 요청에 따라 현재 준비 범위만 마감하고 일시정지 인계용으로 보관한다. 현재 root의 439~442 제품 묶음에 443을 넣지 않았다. 목표의 전체 완료나 실제 Edit 진입 완료를 뜻하지 않는다.

마지막 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 이번에는 보호된 PE와 기존 디컴파일을 읽고 Capstone으로 정적 해석했다. 원본 PE/DLL, 에뮬레이터, 게임, GUI, C++ 빌드나 C++ 검사를 실행하지 않았다.

## 입력 기준과 겹침

- 입력은 `original/`의 28개 파일이다. 출처와 원시 SHA는 `inputs.json`, 입력·결과 SHA와 EOL은 `manifest.json`에 있다.
- 기존 main 입력은 **815fbd67**이며, Camera API가 필요한 파일은 동결된 `extracted/cpp-camera-state-439440-integration/final/`에서 가져왔다. `ConnectionActivitySessionTests.cpp`는 그 위의 별도 `cpp-camera-session-439-test-fix/` 결과를 입력으로 썼다. 이 파일의 actual Load setup 교정은 이미 입력에 포함된다.
- 늦게 추가한 `RawCursorDrop.h`는 815와 동일한 본문인지 확인했다. `CampaignSessionTests.cpp`, `CampaignObjectMenuTests.cpp` 2개는 현재 root 본문을 복사하지 않고 **`git show 815fbd67:<path>`**로 명시 캡처했다. `inputs.json`의 `capturedAfterRoot93`는 캡처 시점과 기준 본문을 구별한다.
- **442 본문을 입력으로 삼지 않았다.** 준비 도중 root가 439~442를 main에 적용한 사실을 이 작업의 변경으로 계산하지 않는다. `mainSHA256AtCapture`와 `mainSHA256AtPackaging`은 시점 관측일 뿐 패치의 base가 아니다.
- 향후 적용은 현재 main에 파일 전체를 덮어쓰지 말고, 443의 입력·결과를 기준으로 **hunk/3way 병합**해야 한다. ClientGameplayGlobals, RawCampaignSession, CMake, Session 검사에는 441 GameAux와 442 ProcessHost 및 기존 권한·FullDeck·Camera 변경을 보존해야 한다. root가 별도로 고친 `RawCampaignCarrierDrop.inc`의 현재 snapshot 투영도 보존한다. 443은 그 inc를 포함하지 않는다.
- `context/cpppj/src/client/RawCursorPlacement.h`는 선행439의 `sharedScreenOrigin` API를 읽기 위해 보관한 **변경 없는 문맥 파일**이다. 443 패치 대상은 아니다.

## 데이터와 실행기의 수명

`ClientGameplayGlobals::CursorGlobals()`가 새 data-only `ClientCursorGlobals`를 제공한다. 이 소유자는 `CursorLifecycleState`, `CursorPlacementState`, snapToGrid DWORD만 가진다. 실제 Session은 같은 상태 주소를 빌리고, 직접 구성하는 순수 fixture에만 `RawCampaignCursor`의 독립 fallback 소유자가 있다.

`RawCampaignCursor`, Session 콜백, GUI 실행기, 예약 중인 `pendingTimer`는 계속 지역 수명이다. Placement의 `sharedScreenOrigin`은 먼저 구성되고 나중에 파괴되는 같은 Gameplay Camera snapshot의 픽셀 배열만 빌린다. Session pointer나 함수를 장기 데이터에 넣지 않았다.

`AcquirePrepared()`와 prepared 반납·생성 실패는 기존 30DWORD, count, 글자·설명·배치 값과 snap을 유지한다. 일반 `Acquire()`와 일반 반납은 기존 지역 Runtime의 fresh/반납 호환을 같은 주소의 `ResetForGeneralWorld()`로 유지한다. 이 reset은 Native Cancel이나 Native State cleanup을 실행한 것으로 세지 않는다. 첫 Edit의 실제 부모 연결에서는 prefix 준비값을 보존하는 prepared 경로를 사용해야 한다.

## 실제 Cancel과 첫 메뉴

- `RawCampaignCursor::CancelForState()`는 실제 `RawCursorLifecycle::Cancel()`만 호출한다. 기존 일반 `Cancel()`의 외곽 `game.prepare`는 그대로 구별한다.
- Session의 `HasCursorRuntimeForState()`는 설치된 실행기의 존재만 뜻한다. 정상 `Cursor()`의 loaded 관문을 완화하지 않았다. `CancelCursorForState()`는 실행기 미설치를 성공으로 넘기지 않는다.
- Session이 없는 첫 메뉴에는 `CancelClientCursorForState()`가 실제 `RawCursorPlacement::Prepare`와 `RawCursorLifecycle::Cancel`을 조립한다. 439의 실제 현재 snapshot Project와 매 요청 때 조회하는 Renderer provider를 사용한다. 장치 교체 뒤의 dirty도 그때의 Renderer에 간다.
- 잠금1·Form0이면 선택된 Form의 복구 분기만 자연 생략된다. **Activate(1) → Prepare → Project/dirty → 현재 목록 삭제 → process 정리 → 목록/잠금 꼬리**를 생략하지 않는다. 첫 메뉴 초기값의 두 dirty 요청도 발생한다.
- helper의 SurfaceStorage/TYPE/Pool은 호출자가 같은 실제 Client 물리와 수명으로 제공해야 한다. preview/프로세스가 존재하는 경로의 가상 효과는 실제 호출자가 제공한다. 누락된 선택 효과를 빈 성공으로 채우지 않는다. 실제 Session은 기존에 연결된 실행기의 효과를 사용한다.

## 원문 주소와 폭

| 주소 | 실제 역할 | 이번 처리 |
|---|---|---|
| 0055a1b8 | signed 현재 목록 개수 | Prepare 뒤 및 각 가상 삭제 뒤 반복 조건에서 재조회 |
| 0055a1d8..0055a24f | **30DWORD / 120BYTE** | `array<uint32_t,30>`; 높은 WORD도 그대로 보관 |
| 0055a274 | mana/range process token | Cancel의 Activate 뒤 현재 토큰을 조회하고 `Remove(process,0)` |
| 0055a278 | 현재 Gump | process와 구별하며 Cancel 꼬리가 임의로 지우지 않음 |
| 0055a27c | legal Gump | 현재 Gump와도 구별하며 Cancel 꼬리가 임의로 지우지 않음 |
| 0055a2d8 | snapToGrid DWORD | 같은 장기 주소를 위치 갱신이 현재 조회 |
| 0055a2f8/fc | hotspot 보정 커서 화면점 | 원시 mouse 및 카메라 원점과 구별 |
| 005cae94/98 | 원시 mouse | 이 Cancel 연결이 변경하지 않음 |
| 0059a91c/920 | 439 snapshot의 현재 픽셀 원점 | 같은 Camera owner를 현재 조회 |

PE 이미지의 목록/count/snap 초기값은0, lockDepth(005325c8)는1이다. 기존 구조체의 다른 기본값은 그대로 유지했다. 전체 커서 초기화 명령 등록까지 이번에 모두 연결했다고 주장하지 않는다. snapToGrid의 옵션/명령 등록은 남은 상위 경계다.

`CursorPreviewSid(DWORD)`는 실제 host SID 소비 직전에만 범위를 검사한다. 기존 host SID가16비트이므로 잘못된 큰 DWORD를 낮은 WORD로 접어 다른 정상 객체에 접근하지 않는다. 비활성 칸을 새 생성 번호로 덮어쓸 때는 먼저 SID16으로 변환하지 않는다. 생성기/collector/표시/비용/회전/수명과 bound 검사 입력을 이 API에 맞췄다. 독립 fixture의 기존 번호와 기대값은 바꾸지 않았다.

원본의 잘못된 큰 SID로 발생하는 OOB/메모리 손상까지 재현한 것은 아니다. **단일 `CursorDropState::preview`는 기존 Sid16 경계가 남는다.** 이번 범위는 목록의 실제 DWORD 폭과 정상 유효 SID 소비다.

## 정적 근거와 검사 준비

`static-evidence.json`과 `evidence/`에 보호 PE SHA, 디컴파일·함수표 SHA, 네 원문 본문, 원시 명령 바이트와 주소를 보관했다. 대상은004458e0(Prepare),00445aa0(Activate),00445b30(Previews),00446e70(Cancel)이다. 00445aa0의 현재 count 재조회와 두 정리 본문의 0x1e DWORD 지우기를 C 및 명령에서 대조했다. 새 Native 실행 관측이나 새 Native fixture는 없다.

새 검사 **8개 정의**를 작성했다. 실행 결과가 아니다.

1. `ClientCursorGlobals_DwordWidthAndPreparedOwnerIdentity`
2. `ClientCursorGlobals_GeneralFreshKeepsAddressesAndCameraBinding`
3. `ClientCursorGlobals_NativeCancelRequeriesCountAndProcessThenClearsAllDwords`
4. `ClientCursorGlobals_HighWordDoesNotAliasValidPreview`
5. `ClientCursorGlobals_FirstMenuCancelUsesActualProjectAndCurrentRenderer`
6. `ClientCursorGlobals_CreateOverwritesInactiveDwordsAtActualAppend`
7. `CampaignExistingEditorCheckpoint_CursorDataPreparedBorrowAndReleasePreserveCurrentValues`
8. `CampaignExistingEditorCheckpoint_CursorDataPreparedFailureKeepsOwner`

기존 `CameraView_ActualClientSessionAliasesFractionAndCurrentCursorReads`에는 같은 owner의 주소, 현재 snap 재조회, 외곽 prepare 없는 실제 Session Cancel을 추가했다. 기존 fraction/좌표 조건은 보존했다. 기존 Native 기대 파일은 읽기 전용으로 SHA를 기록했으며 숫자나 행을 수정하지 않았다.

`static-audit.json`은 기존 검사 이름과 숫자 토큰 순서 보존, 정적 괄호 균형, SID16 배열 잔여 사용, 입력 SHA를 확인한다. 이 감사는 C++ 구문 검사나 컴파일을 대신하지 않는다. 신규4파일 패치의 최초 형식 검사 실패(new file mode 누락)와 생성기 교정은 `evidence/patch-format-initial-failure.json`에 남겼다. CPP 본문 실패나 C++ 검사 실패가 아니다.

최종 격리 적용은 `package.py`가 기록한 새 `apply-check-final-*` 저장소에서만 수행했다. `git -c core.autocrlf=false apply --check --ignore-space-change` 및 실제 격리 적용 뒤 **32개 결과의 정규화 본문**을 준비본과 대조한다. 실제 main에 적용하지 않았다. 향후 main 병합에서는 입력 SHA 확인, 각 HEAD 파일의 EOL 복원, 정규화 결과 대조, 새 Source SHA 동결이 필요하다.

## 다음 연결과 보관

Native004b2df0/State1 전체 부모와 이 leaf의 실제 호출 위치, 실행기 없는 상태/활성 Session의 선택, UI·Process·AI·Links·Player 반납 coordinator는 미접합이다. 현재 owner 수명이나 Session 존재를 핑계로 Native 효과를 건너뛰는 부모를 만들지 않았다. 이번 인계에서 새 구현 단위를 시작하지 않는다.

재현에 필요한 최소 Git 보관 파일은 `archive-files.json`의 `files` 목록에 있다. `original/` 28개와 패치로 결과32개를 복원할 수 있으므로 큰 `cpppj/` 결과 사본, 실패/성공 apply-check 저장소, 중간 생성 스크립트는 필수 보관 목록에서 제외한다. 원시 ignored 경로는 그대로 유지한다. 보호 PE/에셋은 묶음에 복사하지 않으며 원래 저장소의 SHA로 식별한다.

복원은 `python <묶음>/replay.py <존재하지 않는 별도 출력 경로>`로 수행할 수 있다. 입력 SHA와 패치 SHA를 먼저 확인하고 새 격리 저장소에만 적용한 뒤 결과32개의 정규화 SHA를 동결 manifest와 대조한다. 기존 경로를 덮어쓰거나 C++를 실행하지 않는다.

`collect_static_evidence.py`는 같은 SHA의 원본 PE·디컴파일·함수표가 있는 저장소에서 정적 근거를 재생성하며 PE의 명령을 실행하지 않는다. 복원·근거 스크립트는 검토 가능성을 위해 보관한다. 중간 패키지 생성기 `package.py`는 원시 ignored 경로에 남기고 필수 Git 보관에서는 제외한다.
