# controlled 메뉴 항목·제목·머리글·CD 자식 문맥

후속 [GumpForm SID·소유 UI 알림·수명](cpp-gump-form.md)에서 메뉴의 Form 공급과 Gump Kill을 실제 SID 풀/종속 체인/Unpop/반납에 연결했다. 이 문서와 근거 해시는 controlled 항목 구현 당시 상태이며 남은 전체 Form/Client 연결은 후속 기록을 따른다.

2026-10-13, **마지막 디컴파일 PC: HJOW-Athlon**. 현재 호스트/IP는 **HJOW-Athlon / 192.168.0.94**다. AGENTS.md와 두 인수인계를 읽고 같은 PC의 정밀 프로젝트에서 두 PE를 읽기 전용으로 분석했다. 게임/GUI를 실행하지 않았다.

[메뉴 생성/장식/수명](cpp-menu-gump-widgets.md), [메뉴 항목/레이아웃](cpp-menu-gump-layout.md), [메뉴 출력/입력/객체 부착](cpp-menu-gump-interaction.md)에 controlled 작성 래퍼를 추가했다. **Client/UberGump는 기존 메뉴 어댑터를 사용하며 자동 raw UI·건설·미션 완주는 미완료다.**

## 정적 근거

계획은 `tools/ghidra/menucontrols-functions.json`이며 패치 **10개**, CD **12개** 함수를 읽기 전용으로 내보냈다. `extracted/menucontrols/{originals,originalCD}/creation.c` / `functions.tsv` / `assembly.txt`에 본문·범위·명령을 보관한다. 공용 서식/설정 함수는 참조 자료이며 전체 공용 서식기 복원 완료를 뜻하지 않는다.

`tools/analyze_menucontrols.py`는 두 실제 PE의 명령에서 직접 참조한 ROM 색 키/서식·controlled 표/개수/등록표·빈 구분선 문자열과 현재 소스/빌드 해시를 `cpppj/recovery-menucontrols-evidence.json`에 기록한다.

| 기능 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 일반 항목 | 00477cb0 | 00451100 |
| 체크 항목 | 00477dd0 | 00451150 |
| 제목 | 00477ee0 | 004511b0 |
| 머리글 | 00478020 | 00451210 |
| 자식 메뉴 항목 | 00478160 | 00451270 |
| 구분선 | 00478260 | 004512e0 |
| 메뉴 없음 | 00475ef0 | 00451620 |
| fancy 제목의 다중 줄 파서 전달 | 제목/머리에 인라인 | 004516f0 |
| 자식 문맥 복사 | 대응 없음 | 00452a00 |
| 선택 강조 정리/표시 자식 조회 | 대응 없음 | 004529b0 |
| 서식 wrapper/kernel 참조 | 004a89c0 / 004de490 | 00420560 / 004bcd80 |
| 패치 색 설정 ReadInt | 00441270 | 위 CD 래퍼는 조회하지 않음 |

## 실제 연결

`MenuGumpControls`는 같은 MenuGump/controlled 표/등록 표/메뉴 노드, 기존 MenuGumpLayout 및 ScrollingTextGumpMarkup을 사용한다. 별도 메뉴 항목 배열이나 제목 버퍼를 만들지 않는다. `BindMenuGumpControlConfig`는 기존 ConfigInterface.ReadInt를 호출하며 원본 설정 파일을 쓰지 않는다.

패치 일반/체크/자식 래퍼는 현재 메뉴의 존재를 먼저 확인한다. 일반/체크는 include가 false여도 이 확인을 먼저 수행한다. 이후 색 전경→배경 순서로 조회하고 **Append 직전에 현재 controlled 칸을 다시 읽는다.** CD 일반/체크/자식은 처음의 현재 칸이 null이면 조용히 생략하고 별도 색/서식 조회를 하지 않는다.

| 래퍼 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 일반 | MenuColor / MenuBGColor, enabled 또는 forceColor이면 색 접두, 기본 flags 0x10480 | 원문 그대로, 기본 flags 0x10480 |
| 체크 | MenuCheckedColor / MenuCheckedBGColor, enabled에서 접두, 기본 flags 0x10481 | 원문 그대로, 명령 값에 따라 command bit 선택 |
| 자식 | MenuSubColor / MenuSubBGColor, enabled에서 접두, 항상 styled | 첫 NUL 앞 원문에 `~`가 있으면 styled |
| 제목 | MenuTitleColor / MenuTitleBGColor, `~3~E~[C.%d]~[B.%d]%s~.` | `~3%s~.` |
| 머리글 | MenuHeaderColor / MenuHeaderBGColor, `~E~[C.%d]~[B.%d]%s~.` | 무인자 원문 서식 복사 |

색 초기값은 각각 0이며 누락 설정에서도 두 조회를 생략하지 않는다. 색 접두의 공통 형태는 `~[C.%d]~[B.%d]~E%s`다. 설정 값은 signed DWORD 십진수로 넣고 원문은 %s 인자처럼 그대로 삽입한다.

일반 항목의 command bit는 command!=0이며 명령 0의 이전 필드는 보존한다. **패치 체크 항목은 command=0에도 command bit를 세우고 0을 쓴다. CD 체크는 명령 0의 필드를 쓰지 않는다.** 명시 parameter bit 0x80·checked bit 4·disabled bit 0x200을 유지한다. 자식 부착/등록 ID/숨김/0x20과 전체 Reflow는 기존 실제 Append에 맡긴다.

구분선은 빈 문자열/flags 0x10010이며 선두/연속/40개 한도와 미작성 명령/인자를 유지한다. 실제 두 ROM 구분선 문자열이 빈 문자열임을 확인했다. `NoMenus`는 실제 생성/소멸 개수==0이다. controlled 스택이 비었거나 지연 Kill한 메뉴가 숨겨져 있어도 실제 소멸 전이면 개수가 남는다.

## 제목과 서식 경계

fancy 제목/머리는 현재 title Gump에 **기존 실제 다중 줄 Parse만 호출**한다. 원본 wrapper에 없는 메뉴 Reflow/크기 변경을 덧붙이지 않는다. 일반 메뉴는 disabled 항목으로 Append한다. 패치는 제목/머리의 서식 결과에 Menu 색 접두를 다시 붙이고, CD는 일반 항목 wrapper에 enabled=false로 전달한다.

원본 256바이트 지역 포맷 결과는 최대 255바이트와 NUL이다. 결과가 이 범위를 넘으면 호스트에서 거부한다. 입력은 첫 NUL까지만 처리한다. CD 일반/체크/자식은 지역 포맷 버퍼를 거치지 않으므로 기존 Append/SetText의 실제 경계에서 검사를 수행한다.

고정 %d/%s 서식은 숫자와 원문 삽입으로 구현했다. 입력 안의 %는 %s 인자에서 재해석하지 않는다. **실제 추가 인자가 없는** 패치 비활성 항목과 CD 머리의 서식 경로는 리터럴/`%%`를 처리한다. 이 경로의 다른 % 변환은 원본도 대응 인자를 넘기지 않는 스택 입력이므로 호스트에서 거부한다. 공용 번호 인자 서식·임의 printf 변환·전역 작업표/포인터 alias·원본 버퍼 오버런은 이번 범위에 포함하지 않는다.

## CD 참고 helper

`CopyChildTextContextCd`는 자신을 바꾸지 않고 항목 0x20의 현재 등록 ID에서 자식을 찾는다. 자식의 기본 문맥 8DWORD를 순서대로 복사한 뒤 같은 입력 참조로 재귀한다. hidden/dead 필터·Redraw/Reflow·저장 StyleText 조각의 문맥 변경은 원본 몸체에도 없으므로 추가하지 않았다. 잘못된 등록 ID/원본 배열 밖 접근·순환 경로는 호스트 예외로 거부한다.

`ClearSelectionUntilVisibleChildCd`는 현재 항목의 선택 bit 8을 먼저 지운다. 이전 flags에 0x20이 있고 등록 토큰이 비영/visible이면 즉시 true를 반환한다. **그 뒤 항목의 강조는 유지**하며 끝까지 표시 자식이 없으면 false다. 현재 선택 인덱스는 별도로 지우지 않는다. 두 helper는 패치판 호출을 변경 전에 거부한다.

## 빌드와 다음 작업

`netstorm` / `NetstormCpp` **Release 최종 빌드 경고 0·오류 0**. SOURCE_MAP 생성/내용 차이 없음. **새 테스트 추가/실행·전체 CTest·독립 PE/C++ 항목/설정/콜백/태그/픽셀/문맥 대조·게임/GUI 실행은 미수행**이다. 이전 원본 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 이번 추가 코드의 검증 결과가 아니다. 이번 관찰/대조 추가는 각각 0이다.

다음은 전체 Form 풀/소유/객체 연결·동적 하위 메뉴·현재 전역/명령 큐/변경 영역/allocator 공급과 OS 큐/폴링/overlay·자동 raw Client UI다. 새 래퍼/제목/CD 문맥과 기존 UI 전체의 독립 동작/픽셀 대조, 공용 서식기/포인터 alias·그림 색 표/회전 VFX/flag 2 재사용·극단 x87/생성 EH·건설/미션 완주도 남았다.

두 보호 PE SHA-256은 불변이다. AGENTS.md/보호 원본/dotnetpj/테스트를 수정하지 않았고 커밋/푸시를 수행하지 않았다.
