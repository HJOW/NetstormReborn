# 미션 시작 안내의 본문 조립과 HTML 부모 재사용

## 이번 단위의 범위

2026-10-23, 현재/마지막 전체 정밀 디컴파일 PC는 VM-W11-Codex다. AGENTS·공통/닷넷 인수인계와 기존 정밀 원문/보호PE를 재사용했다. 새 `MissionLoadingMessage`는 **이미 확장printf를 거친 실제 원문 바이트**에 대한 `cf570`의 LF/Cancel 조립과 lookup/create/metadata/body/serial 저장 순서만 복원한다. 전역 임시 문자열 고리 `b9320`, 확장printf `4de490`, 실제 HTML 생성·메타/본문·Fit·GDI는 외부 경계다.

초기 분석 모듈 작성 시점에는 원본 실행·GUI·기존 등록제품/CMake 변경·빌드·커밋을 하지 않았다. 이 초안 자료는 초기 두루마리 GDI 로딩의 표시 보장과 별도인 **후속 미션 HTML 연결 준비**다. 초기 로딩은 `436260`과 PE bitmap147이며 이 모듈로 대체하지 않는다.

## b6090에서 고르는 실제 문구

`Netstorm.c` 144708~144737의 `004b6090`은 현재 mission 전역 `00594fa4 != 0`이면 번역된 **`Starting Mission...`**을 `004cf570`에 넘긴다. 미션이 없을 때는 현재 전송/재연결 전역에 따라 `Connected to server %s - Transferring.` 또는 `Reconnecting to new Game Server. Transferring.`를 고른다. 문자열 자체는 원본에 있으며 현재 cpp 중앙 직출력의 **생성 경로/문맥**이 임시 구현이다.

현재 목표의 단일 플레이 호출자는 `client.Translate("Starting Mission...")`를 원본 글꼴 바이트로 변환한 결과를 새 함수에 전달할 수 있다. `%s`·위치 지정 확장printf까지 이 모듈이 복원한 것은 아니다. 일반 `std::format`/`printf`로 원본 `4de490`의 모든 확장 서식을 조용히 대체하지 않는다.

## 정확한 본문과 기본 메타

보호PE에서 직접 읽은 문자열은 다음과 같다.

| 주소 | 실제 C문자열 |
|---|---|
| `00504b0c` | LF 한 바이트 `\n` |
| `00515ee0` | `$ButtonInstant=` |
| `00515ed0` | `,LeaveBattle,3` |
| `00515ef0` | `$Button` |

`cf570`은 `b9320(1024)`로 임시 버퍼를 얻고, 입력 포인터가 그 버퍼와 다르면 `4de490(buffer,format,varargs)`를 호출한다. 같은 포인터면 포맷을 생략한다. 원문 생성기에서 두 입력을 구별해 이 분기도 관측했지만 제품 API는 그 경계 **뒤**의 formatted C문자열을 받는다.

첫NUL 이전의 본문 마지막 바이트가 LF가 아니면 LF를 붙인다. 이후 첫 두 바이트가 `~.`이거나 **대소문자 구별** `$Button` 부분문자열이 어디든 있으면 기본 버튼을 추가하지 않는다. 그렇지 않으면 prefix→현재 Cancel 번역→suffix를 순서대로 붙인다. Cancel 번역의 쉼표·공백·`$`를 임의 escape하지 않는다. `~.` 앞에 공백이 있으면 해당 예외가 아니고 `$button`은 `$Button`과 다르다. `$Buttonish`도 부분문자열 조건 때문에 기본 버튼을 억제한다.

영문 기본 입력의 조립 직후 바이트는 정확히 다음과 같다. 마지막 `3` 뒤에는 새 LF를 붙이지 않는다.

```text
Starting Mission...
$ButtonInstant=Cancel,LeaveBattle,3
```

이는 `DialogPage`에 **instant=true, enabled=true, menu=false인 Cancel/LeaveBattle/3 항목 한 개**를 만든다. Starting 본문 자체는 새로운 Background·Timeout·OnExit 메타를 만들지 않는다. 기존 HTML 부모 재사용 시 전에 저장된 다른 메타 필드까지 지운다는 의미는 아니다.

원본 임시 버퍼의 논리 크기는1024이며 끝NUL까지 포함한다. 제품은 정상 최대1023바이트 결과를 허용하고 그 밖 쓰기는 `length_error`로 거부한다. 원본 밖 쓰기를0/clamp해 정상 복원이라고 하지 않는다. 빈 문자열의 원본 직전 바이트 읽기는 실제 임시 고리의 D5 guard 문맥으로 관측했고 결과는 LF 추가다.

## 현재 serial을 통한 두 경로

| 원본 위치 | 실행 순서 |
|---|---|
| `004cf695`~`004cf6d7` | `64020(5cab34)` → 부모가 있으면 현재+21bc 증가 → `c83b0(parent,buffer)` → `c7aa0(parent,buffer)` → 현재+21bc 감소 → **그 뒤 현재+2c**를5cab34에 저장·반환. 새 부모는 만들지 않는다. |
| `004cf6d8`~`004cf72c` | 기존 검색 실패 → `cf270(buffer,0,400,30)` 반환을 보관 → 생성 콜백 뒤 **현재5cab38**를 다시 읽어64020 검색 → 부모가 있으면 같은 메타/본문 갱신 → **처음 create 반환**을5cab34에 저장·반환. |

`5cab34/5cab38`은 등록 id가 아니라 Gump serial이다. lookup0 실패를 임의 성공 부모로 만들지 않는다. 기존 부모의 metadata/body 콜백이 serial을 바꾸면 갱신 후의 serial이 저장되지만, 신규 경로에서는 현재 노드 serial이 달라져도 최초 create 반환이 최종값이다. depth도 콜백 뒤 현재 DWORD를 감소시키므로 중간 변경을 보존하며 UINT32 감김을 사용한다.

**`c83b0`은 전달된 버퍼를 수정한다.** `Netstorm.c` 159476~159503에서 처리한 메타 구간을 다음 `$`/LF 경계까지 건너뛰고 memmove로 지운다. 뒤 `c7aa0`은 제거된 같은 버퍼를 읽는다. 따라서 새 `metadata` 훅은 `std::string&`이며 `body`는 그 변경 뒤 첫NUL 앞 문자열을 받는다. 이 연결을 const 본문 복사본 두 개로 구현하면 순서가 달라진다.

## cf270의 실제 초기 문맥과 크기

`cf570`의 create 요청은 `cf270(body,0,400,0x1e)`다. `cf120`은 기본 장식 배경 `465d10(this,0,0)`을 호출하고 visible flags1인 HTML 부모를 만든다. 부모+21c0에400, +21c4에30을 저장하며 자동0x4000 replace 플래그를 만들지 않는다.

배경 `465d10`은 현재 배경 TYPE/프레임 전역 `5ca920/5ca924`와 flags `03e00000`, 현재 모서리 TYPE `5411c8`의 SHP/클러스터를 사용한다. Uber 생성자 `4c7160`이 기본 `5ca920=5411c8`, `5ca924=0`, `5ca928=2`를 설치한다. 페이지에 새 Background 지시를 붙이거나 임의 질감을 선택하는 계약이 아니다.

`c7aa0`의 본문 자식은 `bc060(0,0,font0,currentWhite5bec2c)`다. `5bec2c`는 팔레트 로더가 `FindColor(255,255,255)`로 만든 현재 색이며 현재 `PaletteColorTable.basic[7]`에 해당한다. 실제8DWORD 문맥은 `{0,0,white,0,0,0,1,1}`로 기존 TryTell의 흰색 font0 문맥을 재사용할 수 있다. body 자식의 +34는ffffffff, scroll은0이다. body 갱신은 기존 자식을 Kill하고 실제 StyleText/ScrollingText 자식을 만들어 부착/재배치한다.

**400×30은 최종 픽셀 크기가 아니다.** `c7d40` 배치는 요청 폭에서40을 뺀 본문/버튼 최소 폭을 검토하고 실제 줄/버튼 크기에 따라 부모 크기를 다시 결정한다. 요청 높이30도 내부 계산에서65를 빼고 비교하므로 고정30픽셀 창으로 만들면 안 된다. 화면/내용 클램프와 버튼 실제 Fit가 있다. 신규 cf270은 최종 부모를 현재 backbuffer 크기 기준으로 중앙 부착하고 modal 등록 `66600(parent,1)`을 한다. 이 원문 Fit/장식/버튼 배치 전체를 이번 모듈에서 복원한 것은 아니다.

현재 Uber 일반 대화는 contentWidth=min(480,W-64), panelWidth=contentWidth+32인 호스트 배치다. 이 값을400 요청과 같다고 표시하면 안 된다. 원본 모양을 완료 판정하려면 별도 실제 부모 Fit/장식까지 연결해야 한다.

## 현재 DialogScript/BodyLayout/TryTell에 연결하는 최소 경계

새 `BuildMissionLoadingMessageBody`는 원문 바이트 조립만 필요할 때 사용할 수 있다. `ShowMissionLoadingMessage`는 현재 두 serial을 보유한 `MissionLoadingMessageState`와 실제 lookup/깊이 참조/현재serial/메타/본문/create 훅을 받아 원문 호출 순서를 수행한다. 이 상태를 페이지 표시와 별개의 가짜 Gump로 자동 생성하지 않는다.

신규 create의 `cf270` 경로는 기존 `ParseDialogPage`의 1차HTML→메타→LF를BR로 재구성→`DialogBodyLayout::SetBody`의2차HTML과 대응한다. 실제 Config 전체 치환·inline 명령·raw OnExit resolve는 기존 TryTell과 같은 현재 환경에서 수행한다. 파일/섹션 조회용 TryTell을 가짜 섹션 이름으로 호출할 필요는 없으며, 원문 본문을 받는 공통 표시 진입점을 따로 노출하는 편이 명확하다.

반대로 기존 부모 재사용은 **새 cf270을 호출하지 않는다**. 이미 있는 부모의 메타를 처리하고 같은 문자열의 StyleText 본문을 직접 갱신한다. 여기에 `ParseDialogPage` 전체를 매번 실행하면 없는 새 HTML 조건/명령 순회를 추가할 수 있다. 현재 `ExtractRuntimeMetadata`가 private이고 반환용 새 DialogPage를 만들기 때문에 그대로 새 페이지로 갈아끼우는 것도 실제 부모 수명과 다르다. 최소 연결에는 다음 경계가 필요하다.

1. 메타만 처리하여 원문 버퍼의 제거 결과와 실제 항목을 돌려주는 진입점. 기존 raw 캡처/명령 resolve 계약을 그대로 쓴다.
2. 기존 부모와 pageExit 수명을 유지한 채 버튼/본문을 갱신하는 진입점. 재사용을 ClearPage→새 부모로 바꾸면 이전 OnExit가 조기에 실행된다.
3. 메타에 새 Timeout/OnExit가 없는 경우 기존 부모의 해당 필드를 유지하는 정책. c83b0은 기존 버튼 목록부터 정리하지만 모든 부모 메타를 초기화하지 않는다.
4. 신규 경로에서는 cf270의 첫 메타/본문 처리 **뒤에** cf570의 같은 부모 메타/본문 갱신이 다시 오는 순서를 유지한다. 이를 중복 버그라고 임의 생략하면 원문의 객체 생성·버튼 반납·serial/깊이 관측과 달라진다.
5. Cancel은 이미 있는 instant 버튼과 같은 정수 큐로 LeaveBattle/3을 예약한다. 즉시 MainMenu 호출이나 게임 Quit로 바꾸지 않는다.

이들은 root가 후속 실제 표시 단위에서 연결할 영역이다. 초기 두루마리 표시 보장 단위에 섞어 완료 판정하지 않는다.

## 독립 원문과 새 검사

`decomp_mission_loading_message.py`는 보호PE의 **cf570 전체와 strstr 실제 명령**을 실행한다. 임시 버퍼/확장printf 결과/Cancel 번역/serial 검색/HTML 하위 효과는 명시 경계다. 형식 입력과 임시버퍼 자체를 넘긴 alias 입력을 모두 실행하여 원문의 format 생략도 확인한다. 제품에는 그 임시 고리나 확장printf를 새로 구현하지 않았다.

최종 입력 **1,540개**, 두 x87 정밀도 **3,080회 정상 RET**, **1,638,460명령**, 정밀도 차이0이다. 매번 cdecl ESP와 네 비휘발 레지스터, x87 상태를 확인했다.1024바이트 정상 결과의 D5 끝 guard도 유지됐다. callback이 depth/serial/전역/본문을 바꾸는 입력, 신규 반환과 현재created serial이 다른 입력, 생성 실패0, `$Button` 대소문자/부분문자열, Cancel의 쉼표/`$`, LF/CR/NUL, 최대 정상 길이를 포함한다.

fixture SHA-256은 `9770007e448fa9710c3e7fe3eee8c26503cd21bddcfee3231b91db0ebc47a0c5`다. 보고서 `extracted/cpp-mission-loading-message-20261023-native.json`과 `--verify`로 바이트를 확인한다. **실제 regex/HTML 생성·GDI·Fit를 실행한 원문 관측은 아니다.**

새 검사 macro는 `NETSTORM_MISSION_LOADING_MESSAGE_FIXTURE`다. `Native1540`은 전행 본문/호출 순서/최종 필드를 대조하고, `StartingMissionDialogMetadata`는 실제 현재 파서가 instant Cancel/LeaveBattle3과 추가 스타일/OnExit 부재를 추출하는지 확인하며, `ConditionalCallbacksAndBounds`는 선택되지 않은 경계와 물리 버퍼 한도를 검사한다. 작성만 했으며 이번 에이전트가 C++ 빌드/검사를 실행하지 않았다.

## 사용자 중단 요청 시점의 실제 연결 상태 (단위373)

사용자가 확인한 원본 화면은 실제 사본 세션 `20261023T082847962Z-44a65267bba2`의 `90f2ef29695deffae34521b73857194f6a42ca44fedc7bad6870f45e802da772.png`다. Early Missions 첫 행에서 관측했다. 게임은 사용자가 직접 종료했으며 세션 정리/전체 originals 전후 해시 동일을 확인했다. 캠페인 진입을 별도로 관측한 증거는 아니다.

현재 변경은 MissionLoadingMessage를 CMake에 등록하고 RawMenuControls의 같은 실제 Gump 등록표에 돌 장식 부모/ScrollingText/Instant StoneButton을 연결한 **진행 중 코드**다. cf570의 기존 transient 재사용/새 created 재조회는 원문 모듈을 호출한다. 기본 미션 안내의 메타만 처리하며 일반 HTML 전체를 구현했다는 뜻은 아니다. 부모의 파생 필드는 별도 호스트 LoadingFields에 보관한다. native 0x21e8 전체 일반 HTML 레이아웃/8192바이트 입력/메타20버튼/OnExit/Timeout/메뉴 열은 아직 공통 부모로 통합하지 않았다.

원본 c7d40의 보호PE 상수는5069bc=40.0f,51508c=65.0f,504818=30.0f,504a90=15.0f다. 짧은 자연 본문은 ScrollingText FitContent/FitHeight 폭을 유지하고 부모 폭=max(Cancel FitWidth,400-40)+40, 높이=본문높이+65다. 본문은 y20/가운데, 버튼은 y본문높이+30/가운데이며 부모 깊이-80이다. 일반 긴 본문 분기는 명시 미복원 예외다. 지도 영역은 아직 초기 left85 고정이며 실제 HUD 드래그의 현재 사각형과 연결하는 작업이 남았다.

UberGump의 Courier 단독 문구는 제거했고 실제 raw 창 Draw를 사용한다. Client PresentScene은 일반 프레임과 같은 Draw/Lock/Update를 공유하여 미션 파일 적재 전 창을 출력한다. 비 Tutorial의 기존 동기 생성은 다음 State 한 단계로 예약하여 첫 입력/Cancel을 처리하게 했다. 이 일정 변경은 사용자 플레이 회귀가 필요하다. Cancel 클릭은 c79b0처럼 LeaveBattle(3,3)을 같은 큐에 예약한 뒤 부모를 Kill한다.

중단 직전 빌드/관련 검사 결과와 재개 절차는 LEFT_JOBS.md 최상단을 따른다. **실제 수정 제품의 미션 로딩 GUI·원본 픽셀 비교는 아직 실행하지 않았다.** 초기 두루마리 로딩 완료 커밋42021fd2와 구별한다. 원본과 같은 모습이라는 최종 판정은 보류한다.
