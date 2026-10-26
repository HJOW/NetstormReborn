# 편집 저장·메인 메뉴 State3·초기 HUD와 미션 진입 통합 — 단위467

마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 이번 작업도 HJOW-Athlon(192.168.0.94)에서 수행했고 정밀 export와 보호 원본은 바꾸지 않았다. 시작 커밋은 `7711020e`다.

## 확인한 결과

최종 Source6 게임 SHA256은 `6bfa1865fcc6bee83a82577e740f55e4b6723fd9e03f7c5f9021ab43a4d88567`이다. Release 빌드6 경고·오류0, 집중 **181고유/189관측·신규29·실패0**, 전체 C++ **3,194개·실패0**이다. CTest wrapper는1개다. 같은 게임의 GUI8건과 원본 product timer1의 자연 Early2 A~I→Tutorial3 진입도 통과했다.

보호 원본/CD·AGENTS·기존 fixture **2,476파일**, 최종 src/tests **2,748파일**을 해시로 확인했다. 기존3,165개 검사 이름을 모두 유지했고, 기존 검사 본문은 바꾸지 않았다. 이전 checkpoint TU 끝에 새 검사 include만 추가했다. 전체 stdout/stderr의 이름3개 혼합은 실제 관측된 ASCII 이름 조각으로 검산했고 원시 로그를 보존했다.

최종 보고서는 [editor-menu-runtime-integration-20261026.json](../../cpppj/editor-menu-runtime-integration-20261026.json), 원시는 `extracted/cpp-editor-menu-467-*`다. 이 결과는 전체 미션 완주나 원본 표시·음향의 완전 동일성 완료를 뜻하지 않는다.

## 실제 제품 연결

### 저장 여부와 메인 복귀

`SaveGoMain(020000cb)`는 일반 입력의 **event.y**가 비영이면 현재 전체 저장 부모를 실행하고 정상 복귀한 뒤 next3을 예약한다. No0은 저장하지 않는다. 저장 도중 실패하면 앞선 효과와 기존 next를 보존한다. 현재 Template manager 첫 DWORD0은 하위를 생략하며, Battle과 Fort 저장은 서로 독립된 원문 if다. 큰 Battle snapshot은 여전히 선택 경계에서 명시 중단한다.

`NativeMainMenuState`는 원문 State3의 선택·두 이름 첫BYTE0·현재 flags 재읽기·공통 ret를 실제 Client 소유자에 연결한다. State1/3은 같은 전체 cleanup과 로컬 장치를 공유한다. State3은 실제 World를 반납하고 새 Client9를 만든 뒤 common commit과 다음 Poll4를 거친다. 그 후 호스트 표시만 공개한다. 이 위치에서 기존 `MainMenu()`를 호출해 실행 중 분배기/장치를 지우지 않는다.

최초 시작도 State3으로 들어간다. 기본 설정의 UserNotUpdated와 Tips 두 창을 실제로 만든다. 전자는 원문 보안 검증 writer가 없는 현재 기본0에 따른 안내다. 임의 검증1을 주입하지 않는다. `Blank`는 HTML/호스트 표시 수명만 정리하며 Native State3의 중간 장치를 반납하지 않는다.

### 장기 덱과 초기 안내 전역

World 없는 HUD의 mode1도 실제 Deck getter와 내용물 순회를 실행한다. ClientPlayerRecords가 유일한 `RawProductionDeckStorage`를 소유하고 Session은 같은 할당을 빌린다. 앞선 HUD의 두 getter는 같은352BYTE를 반환하며 다음 실제 Player 링크 정리가 반납한다. Runtime 대여가 있으면 기존 Session 설치 가드를 통과해야 하며 장기 경로로 우회하지 않는다. 선택된 삭제/생성 효과에 가짜 Runtime을 제공하지 않는다.

firstMenu/userUpdated/tellTips/tipNumber는 독립 DWORD다. tellTips와 tipNumber는 실제 옵션 writer에서 연달아 읽으며 현재 signed32비트를 보존한다. 결과005c85ac는 기존 Audio Scene의 같은 주소를 Login과 공유한다. `TellTip(020000eb)`는 event.y를 한 번 잡고 이전1/다음2/OK3의 부호·감김·0하한, 지역 정수 서식과 Config.Set을 원문 순서로 실행한다. **OK3도 번호를 증가시키고 Tips 재표시만 생략한다.** 전역 임시 문자열 고리를 추가로 예약하지 않는다.

### Save as

실제 장식/제목/TextEntry/돌 버튼/모달/공통 자식 수명을 사용하는 전용 창을 연결했다. 일반 명령 인자는 `fortMode=event.y, canCancel=1, initial=NULL`이다. 확정은 현재20BYTE Template 헤더 readonly0과 전체 Fort 이름 부모를 사용하며 old/new 경로 모두 편집 작업 namespace 안에 있다. 원문에 따라 old 작업본 삭제가 선택될 수 있다. 원본 파일을 삭제하지 않는다. mode0의 ReadFort 예약은 같은 문자열 token을 두 정수 인자에 복제한다.

실제 버튼 발화는 자식 **serial(+2c)**을 전달한다. 등록 id와 비교했던 최초 초안을 GUI에서 발견해 교정했고, 이전 창을 반납한 뒤 새 창을 실제 마우스로 확인하는 검사도 추가했다. 원문 TextEntry 소멸자는 별도 제외문자 포인터를 반납하지 않는다. 비영 입력4회를 독립 CPU 재관측해 기존 본체를 보존하고 새 검사에서 버퍼 반납과 제외문자 한 블록 잔류를 구별했다. 최상위 저장소 RAII가 호스트 할당을 나중에 반납한다.

### 초기 메뉴에서 실제 미션으로

Native3의 World 없는 HUD를 이전 미션 Runtime으로 오인하던 Init 가드를 실제 World/두 Runtime 대여 확인으로 바꿨다. 기존 실제 Session 가드는 유지한다.

미션 State7의 `004b7528 push1 / 004b752a call004b2df0`에 해당하는 전체 reason1 정리를 실제 Session 생성 전에 연결했다. 이전 Client9·장기 Deck·HUD를 반납하고 새로 만든 같은 Player9를 Session이 대여한다. 생성자의 두 번째 Player 재할당을 실행하지 않는다. 호스트 로딩 안내는 정리 전 현재 HUD로 제출하고, 정리 후 도킹 전의 중간 HUD는 그리지 않는다. 배경 첫BYTE0도 원문 위치에 쓴다. 새 일반 World의 첫 미니맵 대여에서 실제 HUD3을 적용하며 Editor는 Native1을 유지한다.

이 보완은 전체 State7 prefix 복원 완료가 아니다. 현재 일반 World 어댑터의 fresh 초기화, CD 표식 검사00440c60, Renderer/배경 분모/음악·Guide 순서와 prepared World 인계는 후속 원문 대조가 필요하다. 00440c60의 반환을 GameRandom 시드로 해석하면 안 된다.

## 같은 최종 게임의 실제 관측

| 실행 | 관측 |
| --- | --- |
| no3 | Battle1 다리314개를 실제 Cracked 메뉴로 변경 → Main Menu/No → 실제 State3/Poll4 → 재진입. 초기와 재로드의 다리 프레임 차이0, 변경본과는314개 차이 |
| yes2 | 같은314개 변경 → Main Menu/Yes → 실제 저장/State3/Poll4 → 재진입. 변경본과 재로드 차이0, 초기와는314개 차이 |
| saveas3 | 실제 문자로 `Editor UI saveas3` 입력/마우스OK → 이름 갱신·창 반납·새 작업본 Fort 생성/전체 재파싱 성공 |
| duplicate2 | 현재 이름 입력/OK → NameRequired → 안내OK/Save as Cancel → 실제 Editor 복귀·모달0 |
| tips1 | Prior/Next/OK/하위 안내OK. 실제 번호15→14→15→16→16, HTML2→1→0, 메인 복귀 |
| cycle2 | 기본 안내 종료→Early1 MORE/OK→메인 복귀/전체 연결0→Early1 재진입. 일반 연결2개와 새 token 확인 |
| quit3/quit4 | 기본 안내 종료→메인 Quit 정상 종료0/재실행. 후행 frame200 사건은 실행하지 않음 |

Early2는 `extracted/cpp-editor-menu-467-early2-fourth`다. A~I, 신전·워크샾·발사기 생산/정상 회수, SP10000→3075, 환불75, 다음 Tutorial3/SP2000/Guide4/브리핑을 확인했다. 초기 완료값1을 그대로 두었으므로 진행 저장0→1의 영속성 증거와 구별한다. 상태/SP/HP/객체삭제/시험용 다리 순서 주입은 사용하지 않았다.

모든 GUI/자연 실행에서 원본 및 해당 자산 복사본 설정을 보존했다. 실제 원본 게임은 실행하지 않았다. saved/메인 복귀/재진입 화면을 직접 보았지만 원본 픽셀·음향 동일성 대조로 합산하지 않는다.

## 독립 원문과 실패 보존

- [SaveGoMain166관측](../../cpppj/editor-save-go-main-native-20261026.json): 일반 Guide/Gump 관문, 현재 저장 선택과 next writer. 큰 snapshot만 명시 장치다.
- [State3 82관측](../../cpppj/native-main-menu-state-native-20261026.json): 41입력×두 x87, 정상58·선택/부분중단24, 5×144BYTE/현재DWORD/이름/기한/trace 대조.
- [Save as288관측](../../cpppj/save-fort-name-native.json), [serial/제외문자 추가 근거](../../cpppj/save-fort-name-ui-native-20261026.json).
- [TellTip388관측](../../cpppj/tell-tip-native-20261026.json): 실제 일반 입력의 부호/감김/OK·선소비 경계.

빌드1은 신규 검사에서 없는 팔레트 API를 호출해 실패했고, 실제 `BuildGumpShadeMaps(logical)` 호출로 고쳤다. 첫 집중 실패는 새 입력에서 bridge TYPE82가 Temple 분기를 먼저 선택한 점과 제외문자 잔류를 놓친 기대였다. 실제 isle83/선택 guard와 독립 원문4관측으로 입력·새 검사를 고쳤다. 기존 본체·기존 기대값을 완화하지 않았다.

최초 Save as 마우스 실패와 자연 Early2 세 실패(UI 가드, Client9 중복, 미도킹 HUD Draw)는 각 폴더에 남겼다. Source3 전체3194는 통과했지만 실제 미션 문제가 있어 Source6을 다시 빌드/전체 검증했다. 마지막 전체 출력의 진단3개 혼합과 최초 GUI 감사기의 main/pending 표 개수 가정 오류도 기록했다.

## 바로 이어갈 범위

저장한 새 이름의 작업본 목록 재조회·재진입은 아직 미연결이다. `SoloMapCatalog`가 읽기 전용 자산 경로만 열거하는 현재 경계를 다음 작업에서 작업본 읽기 장치에 연결한다. 원본50개 수락 상한은 보존한다. 따라서 Save as의 새 이름 메뉴 재진입까지 완료했다고 주장하지 않는다.

AddIsland population/실제 drop, clipboard, 작업본 목록, Early3 계획은 각각 `extracted/*468-staging`에 준비돼 있고 제품에 적용하지 않았다. NewFort/AddIsland GUI/Test Battle/SetOwner/언어별 시나리오, Native3 승패·Intro39·기존 저장상태 복구, 전체 State7과 나머지 자연 미션 완주/동일성은 계속 진행한다.
