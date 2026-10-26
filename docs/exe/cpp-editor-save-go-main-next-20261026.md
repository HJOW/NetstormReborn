# Editor의 SaveGoMain 원문과 State3 연결 재개점

- 조사일/PC: 2026-10-26, HJOW-Athlon.
- 마지막 전체 정밀 디컴파일: HJOW-Athlon / 2026-10-23. 원본·디컴파일 소스를 변경하지 않았다.
- **읽기 전용 조사이며 아래 연결은 아직 미구현이다.** 이 문서 작성 중 제품 빌드의 소스 동결을 유지했다.

## 명령과 인자는 event.y

메인 Edit로 진입한 뒤 Game → Main Menu는 원본 Tell `[LeaveEditMode]`를 연다. Yes는 `SaveGoMain,1`, No는 `SaveGoMain,0`, Cancel은 DoNothing이다. Yes와 No 모두 같은 메인 메뉴 전환 명령이며 저장 여부가 다르다.

원본 10.78 PE의 `00542a20` DWORD는 `cb 00 00 02`, 명령 ID는 **020000cb**다. 실제 일반 입력 분기는 다음과 같다.

```text
004d815b  cmp ebp,[00542a20]
004d8161  jne 004d8176
004d8163  test eax,eax
004d8165  je 004d816c
004d8167  call 004bfb50
004d816c  call 004b3540
004d8171  jmp 004d8bda
```

**EAX/디컴파일의 local_26bc는 event.y다.** `extracted/refined/originals/Netstorm.c`의 `004d62b0`은 `00452f00`이 돌려준 사건의 `[0]`을 code, `[1]`을 local_26c0, `[2]`를 local_26bc에 대입한다. 실제 CPU `004d6dba`는 반환 사건의 `[eax+8]`을 EAX로 읽고, `004d6dc2`는 이를 현재 스택 `[esp+1c]`에 보관한다. 기존 `decomp_game_menu_actions.py`/`decomp_production_shortcuts.py`도 `[STACK+14]`에 `(code,first,second)`를 설치하므로 `[STACK+1c]`는 두 번째 정수 인자다.

따라서 SaveGoMain과 같은 local_26bc를 쓰는 SetAllBridge는 모두 `event.y`를 읽어야 한다. [이전 조사](cpp-editor-client-next-20261023.md)의 SaveGoMain `event.x` 표기는 잘못된 표기다. HTML 부모는 명령 매개변수를 x와 y 양쪽에 복제하므로 일반 버튼 GUI만으로 이 차이를 구분할 수 없다. 새 검사는 `(x=0,y=1)`의 저장과 `(x=1,y=0)`의 저장 생략을 구별해야 한다.

## Yes의 전체 저장 부모와 No의 생략

`004b3540`의 실제 몸체는 `005c8878=3` 한 번이며, 다른 State 블록·단계·표시 phase를 직접 고치지 않는다. 저장 분기의 정상 반환이 끝나야 이 예약을 실행한다.

`004bfb50`은 다음 순서다.

1. 현재 `00540cac` Template owner로 `005c89f0 + owner*20`의 첫 DWORD를 조회한다. manager0이면 전체 저장을 생략한다.
2. 현재 `00594fbc` Battle 모드가 비영이면 같은 현재 Template의 `004bf880` 전투 snapshot을 실행한다.
3. 그 뒤 현재 `00594fb8` Fort 모드가 비영이면 같은 현재 Template의 `004bf9a0` 전체 Fort snapshot을 실행한다.

두 모드는 독립 if다. Editor에서 일반적으로 Battle0/Fort1이라 Fort 부모만 선택되지만, 그 조건을 저장 함수 전체의 선행 Editor 관문으로 바꾸지는 않는다. No0은 이 부모를 전혀 호출하지 않고 next3만 예약한다. Yes의 저장 실패/명시 중단 뒤에 next3을 써서 성공한 이탈처럼 보이면 안 된다. 원문처럼 앞선 부분 저장 효과는 보존하며 자동 rollback은 추가하지 않는다.

`004bf9a0`은 TypeNames → Technology → Objects → Territory → Deck → Money → Mission → Badges → Notify 순서다. 기존 저장 파일 이미지나 Objects만 저장하는 것으로 대체하지 않는다. 전투 `004bf880`은 Technology → Deck → Objects → Territory → Money → Mission → Core → Notify이며 순서와 포함 항목이 다르다.

## 현재 Session에서 재사용할 연결

| 현재 구현 | 재사용과 제한 |
| --- | --- |
| `RawCampaignSession::SnapshotFort(slot)` | 설치된 실제 Session에서 전체 Fort 부모를 호출한다. 원문004bf9a0에 해당한다. 선택 slot은 현재 TemplateOwner다. |
| `SnapshotFortBody` | 같은 Template9/실제 TYPE/기술 내용물·Objects·섬/청크·Deck·분산 SP·Mission·Badges·공유 난수를 사용하고 마지막에 실제 Notify를 실행한다. |
| `BindFortFileSave(readOnlyForts,files)` | 이미 Editor checkpoint 시작에서 연결된다. Notify는 같은 callback으로 실제 현재 manager 파일명에 전체 이미지를 저장한다. |
| `RawPlayerTemplateState::Changed` | 같은 header+8/+12·Mission 모드·파일명 관문을 유지한다. 이 관문을 꺼서 저장되게 하지 않는다. |
| `CampaignTemplateStorage::SaveFile` | 현재 manager/각 섹션의 실제 생성 수명을 사용하여 전체 Fort 이미지를 만들고 readOnly/hidden/현재 owner 조건과 쓰기 장치를 호출한다. |
| `EditorFiles::Writer`와 `EditorFortFileDevices` | manager에는 이미 안전한 작업본 절대경로가 설치되어 있다. 현재 작업 namespace를 유지하고 원본/CD에 쓰거나 새 `ofstream`으로 관문을 우회하지 않는다. |
| `BindFortCurrentName(...).snapshot` | Fort snapshot 재사용 형태의 참고다. 전체 Battle snapshot은 현재 명시 미연결 오류이며 최초 manager0 관문까지 대신하는 공개 저장 부모는 아니다. |

SaveGoMain용 Session 부모는 원문의 현재 manager0 관문을 먼저 적용하고, 각 읽기 위치의 현재 owner/모드를 조회하여 위 실제 하위를 선택해야 한다. 기존 전체 Battle snapshot은 미연결이므로 Battle0이라는 실제 Editor 조건에서 이를 자연스럽게 생략하는 것과, 선택된 Battle 경계를 빈 callback으로 성공시키는 것을 구별한다. 이번 Editor의 Yes/No 두 분기는 모두 연결하되 일반 전투 저장까지 복원했다고 주장하지 않는다.

Editor checkpoint에서 빌린 `readOnlyForts`와 `EditorFiles`는 SaveGoMain의 전체 snapshot/Notify/실제 쓰기가 끝날 때까지 살아 있어야 한다. Session보다 먼저 EditorRuntime·파일 장치·Template9를 반납하지 않는다. `global.inMission`과 Native `links.mission`은 실제 Guide 스크립트 수명에 묶인 현재값을 사용한다. 저장을 통과시키려고 임시0을 쓰거나 해제된 Mission을 대신 만들지 않는다. 실제 Mission 작성 하위와 serialization version/TYPE/기술 writer를 유지한다.

## next3만으로 메인 메뉴 복귀가 끝나지 않는다

현재 `RawLocalMissionState::Step`에는 State1/2/3의 공통 `parentState` 분배와 공통144BYTE 확정이 있지만, 호스트 `PollEditorState`는 next1과 Editor 뒤 next4만 소비한다. 실제 부모 `InstallExistingEditorState`도 State1만 허용한다. 따라서 정수 명령 표와 SaveGoMain 저장 분기만 연결하면 next3이 남을 뿐 자연 복귀는 완성되지 않는다.

다음 구현에는 **원문 State3 부모 → 공통 commit → 다음 Poll4**까지 필요하다.

1. 같은 실제 Tick의 Native 자리에서 next3을 소비한다. `state_.phase=MainMenu`나 `MainMenu()` 직접 호출로 Native 요청/확정을 건너뛰지 않는다.
2. 원문 `004b6dd0`의 State1·2·3·6 공통 prefix를 State3 선택으로 실행한다. 전체 `CleanupNativeStateWorld(0)`·기존 실제 Runtime 반납 조정기·Chat 숨김·로컬 listener/client9·Fort1·server/boss·Core/Renderer·Chunk/미션 옵션·HUD 모드를 같은 순서로 사용한다. 기존 State1의 bVar21 선택과 달리 State3은 Editor1/missionPost1/FullDeck1 쓰기를 추가하지 않는다.
3. State3의 현재/pending Fort 이름 첫BYTE0·제목 갱신·Guide 종료·메뉴/Tips/startup 선택 분기를 연결한다. 이름 초기화를 파일명 변경 부모 호출로 바꾸거나 기존 파일을 삭제하지 않는다. 현재 스크립트 해제에는 동일 `ReleaseGuideMissionScript` 수명을 재사용한다.
4. 공통 후반 HUD/벽시각·음악·Fort 팔레트와 next4 예약을 원문 State3 조건으로 실행한다. State3은 새 섬 생성 예약을 하지 않는다. Editor World/checkpoint를 다시 설치하지 않는다.
5. 원문 공통144BYTE 확정을 마친 뒤에만 호스트 메뉴 표시를 확정한다. State3에는 이미 반납한 Session이 없으므로 현재 State1 뒤의 `world_->Session().CommitExistingEditorInstall`·사용자 입력 구성 경로를 무조건 실행하면 안 된다.
6. 다음 Tick의 실제 Poll4까지 소비하고, 메인 메뉴에서 Edit 재진입과 Quit이 같은 현재 모드/연결표/새 Session을 읽는지 확인한다. EditorRuntime의 장기 State/전송 장치 수명과 listener/양끝 반납을 별도로 확인한다.

원문 State3에는 startup/Tips 등 분기가 있으므로 현재 기본 옵션을 강제로 꺼서 경계를 생략하지 않는다. SaveGoMain의 완성 범위는 일반 Editor에서 Yes/No/Cancel → 실제 저장 여부 → State3 정상 정리/확정 → Poll4 → 메인 표시/재진입까지다.

## 후속 검증

- 정수 큐의 x/y 불일치 입력, 뗌 사건, 비영 인자값·manager0·현재 TemplateOwner 변경·저장 중단 뒤 next 미변경을 확인한다.
- 동일한 변경을 가진 별도 작업본에서 Cancel은 Editor 유지, No는 저장 부모 미호출/작업 Fort 바이트 유지, Yes는 전체 snapshot/Notify 및 재로드 후 변경 보존을 확인한다.
- readOnlyForts·manager header·hidden·Mission 관문에 따른 자연 저장 생략/오류를 실제 원문과 대조하고, 원본/CD 전체 해시 및 기존 사용자 작업본 보호를 확인한다.
- State3의 전체 공통 확정 뒤 이전 World/AI/Deck/HUD/Player 대여 반납, 새 모드/연결, 실제 Poll4와 Edit 재진입을 관측한다. next3 예약만으로 메인 메뉴 복귀 성공을 기록하지 않는다.

새 저장 분기/State3 코드를 실행한 검증 결과는 아직 없으며, 이 문서는 원본 근거와 필요한 연결의 재개 자료다.
