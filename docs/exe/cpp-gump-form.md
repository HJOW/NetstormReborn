# GumpForm의 객체 부착·소유 UI 알림·SID 수명

2026-10-13, **마지막 디컴파일 수행 PC: HJOW-Athlon**. AGENTS.md와 두 인수인계를 확인하고 현재 호스트가 마지막 정밀 디컴파일 PC와 같은 것을 확인했다. 두 PE와 기존 정밀 프로젝트를 읽기 전용으로 분석했다.

[메뉴 객체 부착](cpp-menu-gump-interaction.md)의 Form 공급 경계를 기존 `SidPool`/`SquidFactory`/`RawSquidDestroy`에 연결했다. `GumpFormHost`는 메뉴·UI 부착용 **타입 3 GumpForm**을 처리하고, `GumpFormBinding`은 같은 Gump 등록 표/메뉴/지연 수명으로 연결한다. **실제 Client/UberGump의 자동 raw UI와 전체 월드 통합은 후속이다.**

## 원본 근거

계획은 `tools/ghidra/gumpform-functions.json`, 내보내기는 `extracted/gumpform/{originals,originalCD}/creation.c`, 실제 범위는 `functions.tsv`다. 각 PE에서 **20개** 함수를 내보냈다. `tools/analyze_gumpform.py`는 실제 PE 기계어를 `assembly.txt`에 기록하고 가상 표/내장 타입 초기값/등록 표/슬롯 폭·현재 코드/빌드 해시를 `cpppj/recovery-gumpform-evidence.json`에 남긴다. 실행 관찰이나 C++ 동작 대조를 생성하지 않는다.

| 기능 | 10.78 | CD 10.72 |
| --- | --- | --- |
| GumpForm 생성자 | 004ae9f0 | 004afd60 |
| 가상 표 | 00512188 | 00505a80 |
| 타입 전역·PE 초기값 | 0054107c = 3 | 0051c96c = 3 |
| 부모 읽기 / 이전 읽기 | 004ad010 / 004abac0 | 004b04a0 / 004b04b0 |
| 부모 쓰기 / 이전 쓰기 | 004abad0 / 004abae0 | 004b04c0 / 004b04d0 |
| 가상 +74 / 공통 contained 부착 | 004af160 / 004ac800 | 004b04e0 / 004ac2e0 |
| 부모 삭제 알림 +1c / 직접 알림 +78 | 004ae9d0 / 004ae9b0 | 004afd40 / 004afd20 |
| 가상 Unpop / 종속 체인 해제 | 004ae140 / 004ac6b0 | 004af270 / 004ac160 |
| 공통 Pop / 빈 postPop 깊이 감소 | 004ac560 / 004ac1d0 | 004abfb0 / 004abab0 |
| pre / post 삭제 깊이 감소 | 004ac1c0 / 004ac1b0 | 004abaa0 / 004aba90 |
| 등록 ID로 UI 통지 / 기본 가상 +4 | 00464190 / 00466740 | 00492be0 / 00492ff0 |
| 기본 Gump 객체 부착 helper | 004644e0 | 00492f80 |
| 공통 factory / 공통 destroy | 004af530 / 004af780 | 004ab390 / 004ab7e0 |

GumpForm의 기본/메뉴 가상 +4는 같은 함수다. 이 함수는 전달받은 객체 SID를 버리고 원본의 Gump Kill을 호출한다. 다른 파생 Gump가 +4를 재정의하는 경우에는 그 처리기를 별도로 공급해야 한다.

## 같은 SID 풀과 소유 필드

`SquidFactory::CreateGumpForm`은 기존 `SidPool::Allocate(flags)` 뒤 판본별 가상 표와 타입 BYTE만 쓴다. 원본 GumpForm 생성자는 슬롯을 다시 지우지 않고 가상 표 하나만 쓴다. 공통 factory의 +28 postCreate는 빈 함수이므로 자산의 owner/HP/zOrder를 적용하지 않는다.

메뉴의 기본 할당 flags는 **2**이며 기존 client 영역의 번호/자유 목록/카운터를 사용한다. 반납도 같은 풀의 FIFO와 최근 삭제 기록으로 돌아간다. 별도 UI Form 풀이나 호스트 포인터 테이블을 만들지 않았다. 슬롯 stride는 패치 **50바이트**, CD **36바이트**다.

| raw 위치 | 폭 | 의미 |
| --- | --- | --- |
| +4 / +6 | WORD / WORD | 종속 next / 자기 종속 head |
| +10 / +11 | BYTE / BYTE | 타입 / free·dead·void·contained 상태 |
| +14 / +16 | WORD / WORD | Form의 previous / 부모 SID |
| +18 | DWORD | 소유 Gump 등록 ID |

contained 자산은 previous를 +16에 둔다. 체인의 이웃 쓰기는 이 위치 차이를 구분한다. 소유 ID는 **DWORD 전체**를 사용하며 하위 WORD로 절단하지 않는다. Form 부모/소유 ID와 Gump의 +24 object/+28 Form SID는 별도 필드다.

## 객체 부착과 해제

`GumpFormHost::Attach`는 가상 +74가 호출하는 공통 contained 부착을 복원한다. ProcessForm 파생 Attach의 process debug 조건을 추가하지 않는다.

1. 자신의 부모 WORD를 기록한다.
2. 부모의 기존 head를 자신의 next로 기록한다.
3. 기존 head의 가상 previous를 자기 SID로 바꾼다.
4. 부모 head를 자기 SID로 바꾸고 자신의 previous를 0으로 기록한다.
5. contained를 켜고 Pop으로 void를 끈다.

클라이언트 Form은 서버에서도 client Pop 플래그를 받아 전송을 생략한다. GumpForm postPop은 공통 깊이 증가를 즉시 되돌리는 빈 몸체이므로 별도 장부/UI 효과가 없다. 서버 번호 Form의 실제 Pop 직렬화/전송이 필요한 요청은 변경 전에 거부한다.

**패치의 부모 0은 무동작**이다. **CD의 부모 0은 assert 뒤 계속 실행**하므로 호스트에서 거부한다. 예약/손상 번호·free/dead 부모·non-void 재부착 등 정상 메뉴 경로 밖 입력도 호스트 계약으로 거부한다. 이러한 거부 분기는 원본 assert 이후 실행을 대조한 결과가 아니다.

Unpop은 void를 먼저 켠다. 부모가 free이면 즉시 끝나고, dead이면 체인을 고치지 않는다. 두 경우 모두 contained/링크 필드는 유지한다. 살아 있는 부모에서는 previous.next→next.previous→필요한 부모 head→contained 해제 순서다. 자신의 부모/next/previous 필드를 별도로 0으로 만들지 않으며 GumpForm은 부모의 자산 제거 알림 +24를 호출하지 않는다.

## 알림과 중첩 삭제

가상 +1c와 +78은 같은 몸체다. Form의 **현재 소유 ID가 0이면 생략**하고, 비영이면 현재 부모 WORD와 함께 Gump 등록 표로 보낸다. +1c의 호출 인자로 받은 삭제 부모를 대신 쓰지 않는다. 소유 ID/부모는 콜백 전에 읽으며 콜백 뒤 Form을 다시 읽지 않는다.

객체 삭제와 UI 삭제는 다음 순서로 연결된다.

1. 전체 월드 삭제의 공통 몸체가 부모를 dead로 표시하고 종속 next를 먼저 보관한다.
2. GumpForm의 부모 삭제 알림이 현재 Gump 등록 ID를 조회하고 가상 +4를 호출한다.
3. 기본/메뉴 가상 +4는 실제 `GumpLifetime::Kill`을 호출한다.
4. Gump Kill이 소유 Form을 공통 destroy로 지운다. 이때 부모는 dead이므로 Form Unpop은 부모 체인을 유지한다.
5. Form pre/post 깊이가 원래 값으로 복구되고 같은 SID 풀에 반납된다. UI는 지연 삭제 큐에 들어간다.
6. 부모 삭제 몸체가 Form의 free 상태를 재조회하여 중복 종속 삭제를 건너뛰고 저장한 next로 계속한다.

UI에서 먼저 Kill하면 Form 루트의 pre/post는 빈 깊이 감소뿐이므로 소유 UI 알림을 다시 보내지 않는다. 중첩된 같은 Form 삭제는 기존 공통 몸체의 dead 분기가 처리한다. 소유 ID/Gump Form 플래그를 콜백에서 임의로 지우거나 UI를 즉시 소멸시키지 않았다.

`GumpFormHost::Hooks()`는 GumpForm pre/post·종속 알림/삭제·Unpop을 분배하고 다른 타입/전송/선택 효과는 기존 fallback에 보낸다. 전체 월드의 삭제 진입점도 이 분배를 거쳐야 한다. 다른 파생 처리기가 내부에서 별도 훅으로 다시 삭제하는 경우에는 그 내부 분배까지 연결해야 한다.

## 연결 API

- `MakeGumpFormNotice`는 매 호출 시점의 `GumpCreateState::entries[owner]`를 조회한다. null 등록/범위 초과는 원본 assert 계약 밖으로 거부하며 visible/dead/serial 필터를 추가하지 않는다.
- `ReleaseGumpObject`는 기본/메뉴처럼 기본 +4를 상속한 노드의 실제 Kill이다. 다른 파생 +4는 호출자가 가상 분배를 공급한다.
- `BindMenuGumpForms`는 내장 타입 3과 Form 생성/소유 ID/객체 부착을 연결한다. Form 토큰과 인덱스에 **같은 SID**를 공급하여 원본 포인터 차이/stride 나눗셈과 같은 결과를 만든다. 기존 MenuGumpWidgets의 object=0 유지·필드 쓰기·현재 가상 Attach 순서는 그대로 사용한다.
- `BindGumpFormLifetime`는 Gump Kill의 Form 삭제를 실제 `GumpFormHost::Destroy`에 연결하고 다른 Form의 기존 삭제 분배를 보존한다.

처리기가 훅을 복사하기 전에 연결한다. 순환하는 UI/월드 알림은 resolver로 나중에 완성할 수 있지만 실제 호출 전에 모든 처리기를 구성해야 한다. 참조/등록 표/풀/저장소는 이 훅과 처리기보다 오래 살아야 한다. 메뉴와 Form 판본을 연결 단계에서 검사한다.

## 빌드와 남은 작업

게임 라이브러리/`NetstormCpp` **Release 최종 빌드 경고 0·오류 0**. SOURCE_MAP 생성기를 실행해 현재 존재 상태를 갱신했다. **새 테스트 추가/실행·전체 CTest·독립 PE/C++ SID/필드/알림/중첩 삭제 대조·게임/GUI 실행은 미수행**이다. 관찰 **523,634개**, 대조 **523,374개**, **854개 통과**는 앞 단계 기록이며 이번 추가 관찰/대조는 각각 **0**이다.

GumpForm의 클라이언트 생성/소유/부착/알림/Unpop/반납과 메뉴/수명 연결은 구현했다. **풀 양수 재확보·내장 지정 Take·공통 Unpop 및 Gump/Process 연결은 [2026-10-13 VM-W11-CODEX 후속](cpp-form-take.md)에서 완료했다.** 상위 초기화 전 UI/Kernel 정리·DependForm/ContentForm 전체 가상 수명·서버 전송·다른 파생 Gump의 +4·파생 처리기 내부의 중첩 분배, 실제 전체 Client/월드 연결은 남았다. 동적 메뉴·현재 전역/명령 큐/변경 영역/allocator·OS 큐/폴링/overlay·자동 raw UI와 신규/기존 전체 UI의 독립 대조도 후속이다. **raw 자동 GUI/건설·미션 완주는 미완료다.**

두 보호 PE SHA-256은 불변이다. AGENTS.md/보호 원본/dotnetpj/테스트를 편집하지 않았고 커밋/푸시를 수행하지 않았다.
