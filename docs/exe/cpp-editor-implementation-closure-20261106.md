# Edit 원문 구현과 파일/전환 대조 범위 정리 — 단위943

2026-11-06, HJOW-Athlon. 마지막 정밀 디컴파일은 같은 PC/2026-11-03이다. 사용자 지시대로3절2번을 앞에서부터 진행해940~942의 시나리오·저장 실패·객체 적재를 처리한 뒤, 남은 편집 도구와 LeaveEditMode의 현재 연결을 대조했다. 새 제품 코드·검사·빌드·게임 실행은 없다. 제품939/11.01과 직전 집중 실행 파일을 사용했다.

## 현재 편집 진입점

| 진입 | 현재 소비자 | 대조한 범위 |
| --- | --- | --- |
| Edit 목록/새 이름/Save as | `SoloMapCatalog`, `EditorCatalogWorkspace`, `OpenSaveFortNameDialog`, `FortSelection` | 기존634/930/932~934와940/942. 5차로 이미 추가한 무제한 목록/사용자 작업본 정책을 유지한다. |
| Edit 도구 메뉴 | `RawGameMenus::Edit`, `UberGump::Event` | Save as 인자1, Add Island, Set All Bridge의 Cracked/Normal/Hard 인자0/1/2가 실제 소비자에 연결된다. 원문918행 메뉴·510행 메뉴바 검사를 현재 실행 파일로 재생했다. |
| 섬 추가/미리보기/배치/이동·삭제 | `EditorIslandDialog`, `RawMenuControls::ShowEditorIslandWindow`, `RawCampaignSession`의 편집 섬 경로 | 기존723/724의 Graph 근거를 유지한다. 현재 실제 기존/신규 월드 입력·부분 생성 실패/참조·드래그 반납·월드/Graph/영역·미리보기/읽기 전용을 확인했다. |
| 지형 그림/자르기 | `RawCampaignEditorIsland.inc`와 편집 Image/Bitmap 경로 | 같은 Session의 전체 맵/자른 그림 선택을 현재 검사로 확인했다. |
| 객체 취득/배치·선택·소유자 | `UserInput`, `GameWorld::SelectFromPointer`/`PrepareEditorObjectMenu`, `CursorObjectPickup`, `DispatchObjectOwner` | 현재 모드1 취득은 실제 커서 `TakeObject`를 호출한다. 일반 Ctrl 선택/소유자/다리 순환은748, 실제 워크샾 owner 저장/재적재는726, 메뉴는725/913~929 근거를 유지한다. 메뉴의 직접 자식/명령 작성도 현재 검사로 확인했다. |
| LeaveEditMode | 원본 TARC Tell → 실제 HTML → `DispatchEditorSaveGoMain` → `SaveCurrentMap` | 보호 TARC 본문은 Yes=`SaveGoMain,1`, No=`SaveGoMain,0`, Cancel=`DoNothing,0`이다.940의 현재 HTML/큐,941의 실제 전체 저장 공유거부/재시도, 기존716의 실제 메인 왕복 근거를 연결했다. |
| Test Battle/Return to Editing | `DispatchEditorTestBattle`, 현재 Guide, `FortEditEffects` | 원문004d81d0의 저장→Guide 생성→mode1과 반대쪽 Guide 반납→pending→선택모드2→State1 요청 순서가 유지된다. 기존174행/716 실제 왕복 근거를 새로 실행한 것으로 세지 않는다. |
| State1/지연 State7 | `InstallExistingEditorCheckpoint`, `UberEditor.inc`, `RunCampaignMissionStarting` |942의 파일·공개 guard/Guide 반납 집중 결과와 현재 정적 연결을 유지한다. 실패 때 선행 효과를 보존하며 편집 loaded를 임의 확정하지 않는다. |
| 손상 파일/기본 이름 반복 | `InstallEditorFortFile`, 현재 Template/작업본 | 손상 파일 두 번 거부 후 유효 파일·기본 파일 거부 후 새 Fort·Clear 실패 때 저장 억제 보존을 현재 검사로 확인했다. |

현재 이 편집 진입점에서 추가로 확인된 미연결 비멀티 명령은 없다. 단위943의 집중 검사는 섬2·섬 창2·손상 파일3·Image1·이동1·메뉴/메뉴바2·객체 메뉴1의 **고유12개·실패0**이다. 새 검사나 새 x86 fixture는 없으며 원문918/510행은 기존 관측 재생이다.

## 완료와 남는 경계

3절2번의 원문 Edit/커스텀 맵/시나리오 구현·대조를 정리한다. 임의 사용자 스크립트/손상 파일의 모든 조합, 실제 OS 메모리 고갈·모든 부분 I/O/SEH·픽셀 동일성·자연 완주를 증명한 것은 아니다. 알려진 구체적인 Editor 결함이 새로 보고되면 해당 항목을 다시 연다.

- State3 제목 뒤 특이 분기, 미션 전환의 연속 수명·타이머·데모 주기는 **3번**에서 계속한다.
- 전체 Factory/Pop·FILE/SEH·실제 부분 I/O/진단 장치 감사는 **4번**에 유지한다. 942의 검사 입력 수정이 전체 Factory 완료라는 뜻은 아니다.
- 모든 외관/자연 완주와 지정 플레이 재현은 **5·6번 사용자 확인**이다.
- 일반 Battle/MP 저장·재접속/메타맵과 새 시나리오 UI/english 자동 생성은 **8번**이다. 현재 english 부재의 원문 동작을 이 단위에서 바꾸지 않았다.

실제 게임 창·포획/희생·특정 적 공격·승리·공중3종 전용·신규 MP/패킷/통신·전체34/ZIP/전체 CTest는 실행하지 않았다. 보호2787개/fixture967개와 제품939 EXE는 불변이다. [완료 JSON](../../cpppj/editor-implementation-closure-20261106.json), Git 제외 `extracted/cpp-resume-943/`를 따른다. 다음944는 **3절3번 첫 항목의 Retry/Next Tutorial/Next Mission/Abort 수명**이다. 8번을 선행하지 않는다.
