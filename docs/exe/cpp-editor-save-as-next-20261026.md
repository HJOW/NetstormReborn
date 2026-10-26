# 다음 단위 준비: 실제 Save as 창과 편집 작업본 저장

이 문서는 다음 구현을 위한 **읽기 전용 조사 결과**다. Save as GUI·이름 확정·자연 저장/재로드는 아직 완료하지 않았다. 조사 PC는 **HJOW-Athlon**이며 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. `cpppj/src`, `cpppj/tests`, 원본/CD, 디컴파일 결과를 변경하거나 실제 게임을 실행하지 않았다.

Save as는 Add Island 전체 모달보다 먼저 완료할 수 있는 단위다. 실제 편집 버퍼·장식·돌 버튼·일반 모달·Fort 전체 snapshot·파일 writer·현재 이름 변경 부모가 이미 있다. 새로 필요한 것은 작은 2버튼 입력 대화창의 부모와 Save as의 두 확정 callback, 같은 공통 UI에 대한 조립이다.

## 1. 정확한 원본 진입과 함수 주소

주 소스는 `extracted/refined/originals/Netstorm.c`이며 helper의 정확한 body 범위는 같은 폴더 `functions.tsv`를 읽는다. CD 10.72 비교 자료는 대응 폴더에 있다. 이번 범위의 일부 decompile에는 누락된 레지스터/인자가 있으므로 아래 명시한 기계어 확인을 함께 사용한다.

| 역할 | 10.78 | CD 10.72 |
| --- | --- | --- |
| Save as 메뉴 명령 전역/값 | `005428dc` / `0x02000079` | 별도 확인 필요 |
| Save as 전체 공장 | `004d3490` | `004c4330` |
| Save as 파생 생성자 | `004d3250` | `004c3f70` |
| 2버튼 장식 대화창 기본 생성 | `00487500` | `00413c30` |
| 제목 StyleText 생성 | `00487260` | `00413cb0` |
| OK/Cancel 버튼 생성 | `00487530` | `00413dc0` |
| 두 버튼의 배치/크기 갱신 | `00487360` | `00413eb0` |
| 대화창 입력 부모 | `00487460` | 대응 별도 확인 필요 |
| Save 확정 가상 `+0x3c` | `004cf9a0` | `004c41d0` |
| Cancel 가상 `+0x40` | **`004cfab0`** | 대응 별도 확인 필요 |
| 현재 Fort 이름 전체 변경 | `00460980` | `004ca650` |
| 이름으로 Fort 경로 선택 | `00460650` | `004ca3d0` |
| 파일/디렉터리 존재 확인 | `004dd560` | `004be410` |
| 이름 확정 뒤 ReadFort 큐 | `00542a1c` | 별도 확인 필요 |

`004cfab0`은 현재 `functions.tsv`에 별도 행이 없는 leaf다. 보호 PE 바이트를 읽어 Capstone으로 직접 확인했다. 범위 `004cfab0..004cfad1`의 본문은 `[this+0xd8]`을 검사해 0이면 `Tell("NameRequired",0)` 후0, 비영이면1을 반환한다. 이번 조사에서는 디컴파일 결과에 함수를 추가하거나 변경하지 않았다.

실제 Save as VT는 `00517bc8`이다. 보호 PE의 직접 DWORD 조회로 `+0x28=00487460`, `+0x3c=004cf9a0`, `+0x40=004cfab0`을 확인했다. 기존 vtables.tsv가 VT를 여러 조각으로 나눠 표시하므로 행의 상대 index만 보아 handler를 오인하지 않는다.

메인 입력 부모의 실제 `004d7446..004d7457`은 `005428dc` 비교 뒤 `push EDI; push 1; push EAX; call 004d3490; add esp,12`다. 메뉴의 원래 숫자 인자1을 보존해 **fortMode=1, canCancel=1, initial=0이면 현재 Fort 이름**으로 연결해야 한다. prototype은 실제 세 인자 `(fortMode, canCancel, initialName)`다. ctor는 첫 인자를 `+d4`, 둘째를 `+d8`, 셋째가0이면 `0055b490` 현재 이름을 초기 텍스트로 사용한다.

## 2. Save 확정과 Cancel의 정확한 동작

`004cf9a0`의 순서는 다음과 같다.

1. `00451e80(this+0xd0, local256,256)`으로 현재 실제 편집 버퍼를 복사한다.
2. 첫 LF를 찾으면 그 위치에 NUL을 쓴다.
3. 빈 문자열이 아니면 C CRT `_isalnum((int)*p)`을 호출한다. 영숫자·`_`·공백 이외의 각 문자를 공백으로 바꾼다. 공백 trim/collapse나 새 이름 자동 생성은 하지 않는다. 한국어 Unicode 문자 분류로 바꾸는 작업도 이 원본 1차 범위가 아니다.
4. 빈 이름이면0을 반환해 창을 유지한다.
5. `00460650(name)`으로 **fortSpec**을 선택해 경로를 만든 뒤 `004dd560`으로 검사한다. 이 함수는 `GetFileAttributesA != INVALID_FILE_ATTRIBUTES`이므로 디렉터리도 비영이다. VFS 아카이브 안의 존재 여부로 대신하지 않는다.
6. 대상 경로가 존재하면 `Tell("NameRequired",0)` 후0이다. 임의 overwrite 확인창이나 덮어쓰기를 추가하지 않는다.
7. 존재하지 않고 `+d4 != 0`이면 현재 Template manager가 있는 경우 **header+8 readonly를0으로 먼저 쓴다.** 그 뒤 전체 `SetCurrentFortName(name)`을 호출하고1을 반환한다.
8. `+d4 == 0`이면 문자열 고리 `004c7910(name)`의 같은 토큰을 명령 `00542a1c`의 두 인자에 전달해 일반 입력 큐 `00452fe0`에 예약한 뒤1이다. Fort 저장 분기와 구별한다.

`004cfab0` Cancel은 `+d8 != 0`일 때1이다. 0이면 NameRequired 안내 후0이므로 창을 닫지 않는다. Save as에서는 canCancel1이고, 미션 `saveFort`의 이름 강제 입력에서는0일 수 있다.

`00487460`은 OK 버튼 click(`005427a8`, 해당 OK 등록ID) 또는 Enter(`005427d8`)에서 가상 `+3c`를 실행하고 비영 반환일 때만 `00466480(this)`로 Kill을 예약한다. Cancel 등록ID click은 가상 `+40` 반환이 비영일 때만 Kill한다. 나머지 사건은 원본 기본 `00464990`에 위임한다. callback 안에서 동기 소멸/Cleanup으로 현재 부모를 반납하지 않는다.

## 3. readonly·snapshot·old 파일 삭제와 작업본 장치

현재 구현된 `RawFortCurrentName::SetCurrentFortName`은 원본 전체 순서를 이미 소유한다.

1. 현재 manager가 있으면 현재 Fort의 **old 경로**를 지역256BYTE에 보관한다.
2. 요청 이름의 점 진단을 수행한다.
3. 현재 manager가 있고 readonly가0이면 현재 이름으로 첫 전체 snapshot을 실행한다.
4. 현재 이름 버퍼를 새 이름으로 복사한다.
5. 현재 manager가 있으면 현재 readonly를 캡처하고 header+8을0으로 바꾼다. 현재 새 경로를 manager 파일 이름에 반영하고 두 번째 전체 snapshot을 실행한다.
6. 캡처한 readonly가0이고 old/new 경로가 대소문자 무시 비교로 다르면 **old 파일 삭제 요청**을 한다.
7. 창 제목을 갱신하고 같은 목록 dirty DWORD를 증가시키며 현재 모드에 따라 lastEditFort를 기록한다.

Save 확정이 이 부모보다 먼저 readonly를0으로 만들기 때문에, 원본 Save as 경로에서 writable old 파일 삭제가 선택될 수 있다. 이를 임의의 “이름만 바꾸기” 또는 “old 파일을 항상 유지하는 복사”로 바꾸지 않는다. 원본 파일 보호는 **모든 쓰기/삭제를 EditorFiles 작업 namespace에서 실행하는 것**으로 만족한다.

재사용할 기존 연결은 다음과 같다.

| 현재 모듈 | 재사용할 경계 |
| --- | --- |
| `RawCampaignSession::BindFortCurrentName` | 같은 현재 Template/State, 전체 `SnapshotFortBody`, Notify를 제공한다. Battle snapshot 미복원은 명시 거부한다. |
| `RawCampaignSession::BindFortFileSave` | 실제 `CampaignTemplateStorage::SaveFile` → 원본 이미지 생성/파일 저장 부모 → 같은 EditorFiles.Writer에 연결한다. |
| `FortCurrentNameDevices` | Config pathSpec·현재 이름·manager setter·경로 비교·삭제·설정을 기존 전체 이름 부모에 연결한다. **선택적 마지막 인자 `const EditorFiles* work`를 이미 지원한다.** |
| `EditorFortFileDevices` | 현재 fortSpec을 정규화하고 작업 루트에 ResolvePath/PrepareCurrent/Exists/Read한다. |
| `EditorFiles` | 보호 원본/CD 밖의 작업 namespace 검증·읽기 전용 속성·쓰기·삭제를 수행한다. |

현재 `UberGump::SelectCurrentFortName`은 `FortCurrentNameDevices`를 만들 때 **work 포인터를 전달하지 않는다.** 최초 기존 Editor 설치에서는 아직 manager가 없어 경로/저장이 선택되지 않지만, Save as에서는 manager가 있으므로 그대로 쓰면 새 경로와 old 삭제가 작업 namespace에 머물지 못한다. 실제 Editor Save as 조립에서 `&editor_->files`를 전달해야 한다. 생성자에 원본 경로를 고친 별도 규칙을 추가하지 않는다.

작업본의 현재 manager 이름은 이미 `<게임 루트>-editor/...`의 절대 경로다. 새 이름에 대한 exists 검사와 manager pathSpec도 같은 작업 루트를 사용한다. `lastEditFort`에는 파일 시스템 절대 경로가 아니라 원본 논리 이름을 기록한다. `SelectCurrentFortName` 안의 mission 조건도 `mission_` 캐시 대신 실제 수명/전역 상태와 일치하는지 다음 통합에서 확인한다.

## 4. 실제 GUI 부품과 최소 모듈 설계

문자열 입력을 새로 만들 필요는 없다.

- `TextEntryGump::Construct`는 원본 `00452430` 전체 본체다. `SetExcluded`=`00451b80`, `CopyText`=`00451e80`, Draw/Idle/문자 입력/선택/붙여넣기도 이미 있다.
- `RawTextEntryControls::Allocate(false)`로 기본 편집 노드를 얻고 `Backend().Construct`를 사용한다. Console 파생의 제출/완성 callback은 이 창에 필요 없다.
- `RawMenuControls::EnableTextEntry`가 같은 공통 트리·글꼴·배경·Slider·문자 물리를 조립한다. 실제 Chat 창도 `RawMenuChat.inc::BindChat`에서 이 장치를 사용한다. 새 Save as는 기존 문자 처리기의 부모 bubble만 자기 대화창에 연결하고 두 번째 입력/버퍼 저장소를 만들지 않는다.
- `RawTextEntryContext`의 벽시계/clipboard 장치는 기존 Client/Chat 연결을 재사용한다. 실제 표면 입력은 공통 Gump Route가 모달 우선 처리한다.
- 장식 `GumpDecoration::Construct`=`00465d10`, 기존 StyleText 제목, `StoneButtonGump`, 기본 배경·공통 Draw/Redraw/소멸이 있다.
- 공통 모달은 `RawMenuTree.input.AddDialog`이고 기본 Kill/RemoveDialog/자식 우선 Cleanup은 기존 GumpLifetime이 소유한다.

최소 설계는 `SaveFortNameDialog` 같은 원문 부모 모듈과 `RawMenuControls`의 실제 저장소 조립이다. 파생 root는 `GumpDecorationNode` 위에 제목(+c4), OK(+c8), Cancel(+cc), 편집(+d0), fortMode(+d4), canCancel(+d8)를 보관한다. 0xdc 요청의 실제 파생 노드 할당, 기본 조회, 가상 Handle/Draw/Redraw/scalar·자식 소유권을 같은 기존 분배에 추가한다. 공장은 현재 Client 번역·초기 이름·프레임/팔레트·현재 크기·두 확정 callback을 빌린다.

원문 생성과 기하는 다음을 보존한다.

1. `00487500`: 장식 200×100 생성 뒤 같은 kind/VT를 설정한다.
2. `004d3250`: 폭은 현재 부모폭×**float32 0.8**의 CRT _ftol, 높이18로 TextEntry를 생성한다. font/background 인자0, 글자색은 현재0059afd8이다. 초기 이름0이면 현재 Fort 이름을 사용한다.
3. 제외문자는 원본 주소 `00542680`의 NUL 문자열을 그대로 사용한다. 아래 원문을 literal로 옮길 때 backtick·따옴표·백슬래시를 손실하지 않는다.

   ```text
   .?=()[]~"`;:^&%$#@!*-+'{}|\/<>,
   ```
4. 편집 노드 `maxLength(+100)=0x28`, `changeNotice(+f4)=0`. 부모 가운데 X와 Y=45에 부착한다. 양수 maxLength의 입력 허용 비교는 기존 TextEntry 본체에 맡긴다.
5. 테두리 배경은 편집폭+8, 편집높이+4이며 배경 flags `0x03e40000`, kind1, 원본 현재 배경 TYPE/프레임을 사용한다. 위치는 편집 x−4, y−2다.
6. `fortMode`가 비영이면 제목 “Save Fortress As?”, 0이면 “New Save-Game Name?”이다. refined C의 `unaff_EBX` 표시는 부정확하다. 기계어 `004d33e6`은 실제 `[esp+0x28]`의 첫 함수 인자를 EBX=0과 비교한다.
7. StyleText 제목은 font20/기존문맥, y10, 현재 편집 x에 부착하고 재측정한다. 부모폭은 제목폭×**float32 1.25**다.
8. OK/Cancel은 원본 70×18 돌 버튼으로 생성해 FitToText하고, 제목/편집 다음으로 `00487360`의 원래 간격10·좌우 가운데 배치를 사용한다. 버튼 배치 이후 부모 최종높이에4를 더한다.
9. `004d3490`: 부모를 전체 화면 가운데에 붙이고 depth를 현재 depth−76으로 조정한 뒤 실제 AddDialog에 넣는다. 초기화 실패/부분 생성도 기존 소유 저장소와 Cleanup에서 반납한다.

형상/제목 측정 계산은 원문 부모 하나가 소유하도록 하고, 임시 HTML/Win32 InputBox로 대체하지 않는다. 현재 `RawMenuControls::Impl` 안에 decoration, textEntries, StyleText, 돌 버튼 할당/일반 노드가 있으므로 이곳에 얇은 조립을 추가하는 방식이 같은 장치를 공유하기 가장 쉽다.

## 5. NewFort와의 구분

새 맵은 기존 파일 부재 `004b5ca0`에서 `004bfba0`의 새 manager 생성, State|1/Notify, **`004b3920`의 전체 신규 Fort 초기화**, Subscriber/config/title 꼬리로 이어진다. 이후 World alternate1/지형/0개 섬일 때 AddIsland 자동 예약이 필요하다. 새 맵 이름을 입력받는 창만 만들어 기존 파일 분기를 억지로 통과시키면 새 맵 복원이 아니다.

`004b3920`은 Unnamed 번역·실제 Subscriber ID/handle·현재 SP 제거·Player known·기술·Core 저장·원본 재시드/비영 lowWORD·Subscriber·Territory·Technology·State+11·Notify를 소유한다. `004b37c0`은 Territory 레코드 한 개의 모양/flags/랜덤BYTE를 채우는 별도 helper다. 이름이 비슷하다고 신규 Fort 전체 부모와 혼동하지 않는다.

미션 Guide 종료 `004846d0`은 `saveFort` 이름의 대상 경로가 읽기 전용 등으로 충돌할 때 `004d3490(1,0,initial)`을 호출할 수 있다. 따라서 canCancel0 동작은 공용 모듈에 보존하되, 첫 실제 제품 연결은 기존 Editor의 Save as(canCancel1)에 한정할 수 있다. NewFort·미션 보상 저장·일반 SaveGame은 완료 근거를 별도로 남긴다.

## 6. 다음 구현과 검증 순서

1. 독립 PE 관찰을 먼저 만든다. `004cf9a0`의 빈/LF/문장부호/공백/기존경로/readonly0·1/현재manager0·1/중간실패와 `004cfab0` canCancel0·1을 관찰한다. UI 부모는 실제 부품 경계만 명시하고 전체 함수가 선택한 순서·필드·반환을 기록한다. 누락 leaf 범위는 읽기 전용 PE 바이트 근거를 별도로 기록하며 기존 디컴파일 파일을 조용히 바꾸지 않는다.
2. 원문 입력 Dialog 부모/Save callback을 공통 검사로 복원하고 기존 TextEntry/GumpDecoration/버튼 본문을 보존한다. Enter·OK·Cancel·ESC·모달 외부 클릭·빈 이름·이미 존재하는 이름을 검사한다.
3. 실제 Editor Session의 같은 작업본 장치를 연결한다. 새 이름 확정 뒤 현재/manager 파일명·readonly·목록 dirty·제목·lastEditFort·두 snapshot/old 삭제 순서와 실패 접두를 확인한다.
4. 자연 메인 Edit→기존 맵→ESC Edit/Save as→문자 입력→OK→실제 생성 파일 파싱→메뉴 복귀→새 이름 재진입으로 World/Deck/객체가 같은지 확인한다. 첫 Battle1과 사용자 TEST01을 각각 확인한다. 입력을 test-only 설정이나 직접 State 호출로 우회하지 않는다.
5. 원본/CD 전체 보호 해시·기존 검사 이름/본문·기존 fixture를 보존하고, 집중 검사와 전체 C++ 검사 및 같은 실행 파일의 GUI 증거를 기록한다. LEFT_JOBS.md에 완료와 남은 NewFort/AddIsland/Test Battle/시나리오를 나눠 기록한 뒤 로컬 커밋한다.
