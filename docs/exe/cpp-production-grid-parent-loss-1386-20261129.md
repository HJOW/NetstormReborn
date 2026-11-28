# 생산 격자 부모 소실 뒤 Form 번호 재삭제 방지 — CP1386

2026-11-29, HJOW-Athlon. 사용자 요청으로 dotnetpj 1.116의 의도적 차이(R5-4c4a)를 cpppj 현행 동작과 대조했다. 작업 PC와 LEFT_JOBS.md의 마지막 디컴파일 PC가 같으므로 다시 디컴파일하지 않았다. 원본 파일과 디컴파일 자료는 바꾸지 않았다.

## 원본과 현행 흐름

원본 `004644e0`은 부모가 0이 아닐 때만 Form을 만든다. Form의 소유 ID(`+0x12`)를 쓰고 가상 `+0x74`로 부모에 붙인 뒤 Gump `+4 |= 0x40`, `+0x24 = 부모`, `+0x28 = Form SID`를 기록한다. 부모가 0이면 아무것도 하지 않는다. cpppj `RawProductionControls::Impl::Bind`가 이 동작을 그대로 옮겼다.

1. 부모 객체가 삭제되면 공통 삭제가 종속 Form마다 `ReleaseDependent`(소유 UI의 `ParentGone` 통지)를 보낸다. 이어서 Form이 아직 반납되지 않았으면 `DestroyDependent`(Form 삭제)를 보낸다(`RawSquidDestroy.cpp`).
2. 생산 격자의 `InventoryGump::ParentGone`은 `bind(token,0)`을 부른다. 원본 무동작이라 격자에 `0x40`과 이미 반납될 Form 번호가 남는다.
3. 패널 `Close`가 격자를 Kill하면 `GumpLifetime.cpp`가 `0x40`을 보고 남은 번호로 `deleteForm`을 다시 부른다.

루트 `Idle`은 현재 Player `+0x9c` 기술 부모가 격자 `+0x24`와 다를 때만 다시 `Bind`한다. 그래서 부모를 잃은 뒤 새 부모로 다시 연결되기 전에 패널이 닫히는 경우에만 문제가 된다.

## 재현 검사

`CampaignProductionLifetime_GridParentLossKeepsReturnedFormNumbers`(`cpppj/tests/CampaignSessionTests.cpp`)를 추가했다. 실제 Tutorial2 Start와 `RawButtonControls`의 생산 패널을 쓴다. `ProductionLifetimeScene`에는 제품 `CampaignClientServices`처럼 Session의 GumpForm 부모 소실 통지를 같은 UI의 `ObjectNotice`로 보내는 선택 인자를 더했다.

- 배치 자산 TYPE80을 현재 Player `+0x9c`로 지정한다. 원본 HUD 사건 `0x0200003d`로 두 격자(bridge·teleport)를 다시 연결한 뒤 부모를 삭제한다.
- 두 경우를 나눈다. ① 반납 직후 패널을 닫는다. ② 실제 클라이언트 FIFO 풀에서 반납된 두 번호가 다른 할당에 재사용될 때까지 보유하고 TYPE 표식을 쓴 뒤 닫는다.
- 시작 Player 객체(TYPE152)를 부모로 삭제하는 경우는 이 로컬 캠페인 세션에서 `캠페인 다른 실제 자산 가상 삭제 미연결 TYPE152 SID5` 명시 오류로 거부된다. 격자에 도달하지 않으므로 검사에서 뺐다(`extracted/cpp-grid-parent-1386-before-type152.log`).

| 경우 | 수정 전(`extracted/cpp-grid-parent-1386-before.log`) | 수정 후(`extracted/cpp-grid-parent-1386-after.log`) |
| --- | --- | --- |
| 부모 소실 직후 격자 | 두 격자 모두 연결1·부모14·Form15/16 | 연결0·부모14·Form0 |
| ① 반납 직후 닫기 | 예외 없음 | 예외 없음 |
| ② 재사용 뒤 닫기 | **`GumpForm 삭제 타입 오류`**. 이어서 장면 소멸에서 다시 던져 프로세스가 0xC0000409로 종료 | 예외 없음, 재사용된 두 슬롯의 TYPE 표식·할당 상태 보존 |

②는 이전 격자가 다른 객체에 넘어간 번호를 Form으로 지우려 한 것이다. 로컬 경로는 타입 검사가 막아 예외로 끝난다. 제품 Native 경로는 실행하지 않았지만, 코드상 슬롯의 현재 가상표로 삭제를 고른다(`SquidFormDispatch::Destroy`). 그래서 반납 직후에는 dotnetpj 1.116에서 실제로 난 `PATCH 현재 가상+10의 미지 VT` 예외가 나고, 다른 자산이 번호를 받았다면 그 자산을 지우는 경로로 들어갈 것으로 본다. 원본 10.78도 같은 무동작이므로 같은 잠재 결함이 있을 것으로 보이지만 원본 실행으로 확인하지 않았다.

## 수정

`RawProductionControls.cpp`의 격자 `bind` 훅만 새 `UnbindLostParent`로 바꿨다. 이 훅은 `InventoryGump::ParentGone`에서만 불린다.

- 부모가 0이 아니면 원본 `Bind`를 그대로 실행한다.
- 부모가 0이면 격자의 `0x40`과 `+0x28` Form 번호만 지운다. Form은 이 통지 직후 공통 삭제가 원래 순서대로 종속 삭제한다. `+0x24` 부모 값은 원본처럼 남겨 `Idle`의 재연결 비교를 바꾸지 않는다.
- `Handle`·`Idle`이 쓰는 일반 `Bind`의 부모 0 무동작은 그대로다. 노드 `flags`의 `0x40`을 읽는 곳은 `GumpLifetime.cpp`의 Kill 하나뿐임을 확인했다.

이것은 원본과의 **의도적 차이**다. 2차 목표(Windows10/11에서 오류 없는 실행)를 위해 반납된 번호의 재삭제만 막는다.

## dotnetpj·webpj와의 관계

- **dotnetpj 1.116**은 통지 안에서 살아 있는 Form을 `DeleteForm`으로 먼저 반납하고 `Flags 0x40`·`Object`·`Form`을 모두 0으로 만든다. 재삭제는 막지만 Form 삭제 시점이 원본보다 앞선다. 또 `HostProductionWindows.Events.cs`의 `Idle`은 `Object != parent`일 때 다시 연결하므로, Player `+0x9c`가 소멸한 번호를 그대로 가리키면 죽은 부모에 새 Form을 붙일 수 있다. CP1386 방식(0x40·Form만 지우고 Object 유지)으로 맞출지는 [LEFT_JOBS.dotnetpj.md 0.1 6번](../../LEFT_JOBS.dotnetpj.md#now) DN-GRID-1~2에서 판정한다.
- **webpj**의 `client-gump-root.js` `bindObject(token,0)`도 원본처럼 무동작이라 같은 구조다. 또 `0x40`이 켜진 노드에 새 부모를 붙이면 수명 오류를 던지므로, 부모 소실 뒤 `Idle` 재연결도 막힌다. 재현과 보완은 [LEFT_JOBS.webpj.md 다음 작업 2](../../LEFT_JOBS.webpj.md#next) WB-GRID-1~2에서 진행한다.

## 검증

- Release `NetstormCpp`·`NetstormRoot`·`netstorm_world_generation_tests` 빌드 오류0·경고0(`extracted/cpp-grid-parent-1386-build-fix.log`). `Netstorm.exe` SHA-256 `813029fb0e7b4e2c3da3ab60a1edf8d7fe96589cccb637e616bbde86d6113520`.
- 새 검사 1/1 통과(두 경우).
- 같은 대상의 생산·인벤토리 관련 검사 13개를 하나씩 실행했다. 수정 전 13/13, 수정 후 13/13 통과(`extracted/cpp-grid-parent-1386-related-{before,after}.log`).
- `netstorm_issue_commands_tests`·`netstorm_main_popup_tests`의 생산 관련 24개도 수정 후 24/24 통과(`extracted/cpp-grid-parent-1386-other-after.log`).
- `InventoryGump`은 바꾸지 않았으므로 원본 170행 재생 검사 `InventoryGump_Native170`(통합 `netstorm_tests`)은 다시 실행하지 않았다.

## 한계

- 검사는 로컬 캠페인 세션 경로다. 제품 Native 경로와 TYPE152 부모는 이 장면에서 실행하지 않았다. 수정은 격자 노드 필드만 바꾸므로 두 경로에 같이 적용된다.
- 실제 플레이에서 생산 격자의 부모(Player `+0x9c`)가 미션 중 삭제되는 경우가 있는지는 확인하지 않았다. 게임 실행·GUI 확인·배포 ZIP 재작성은 하지 않았다.
