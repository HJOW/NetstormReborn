# 기본 HTML 본문 → StyleText 변환 복원

현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 보호 PE와 기존 정밀 C를 읽었고 실제 게임·GUI·OS API를 실행하지 않았다. `AGENTS.md`, 보호 원본, `dotnetpj`는 수정하지 않았다. 인수인계 갱신과 단위 커밋은 부모 작업에서 통합한다.

## 이번 단위의 결과

`client/HtmlTextParser`는 실제 기본 HtmlGump 파서의 바이트 출력·현재 상태·순회를 복원한다. 입력 HTML을 임의 문단/글꼴로 재해석하지 않고 기존 `StyleText`, `StyleTextBuffer`, `ScrollingTextGumpMarkup`이 받는 제어문자열을 반환한다. `<i>edge</i>`는 `~N~jedge~.`이며 italic 글꼴이 아니다. `<b>open</b>`는 `~N~Bopen~N`이다. 두 태그는 자체 줄바꿈을 쓰지 않는다.

이 모듈은 독립 복원/검사 단위다. 기존 `DialogScript::Paragraphs`, `UberGump`, `Renderer`는 수정하지 않았다. 따라서 기존 Tell 화면의 inline 줄바꿈/색 문제 해결은 이 반환값을 실제 텍스트 경로에 연결한 뒤 별도로 확인해야 한다. 조건 계산, 명령 큐, 그림 리소스, 링크 소유권, 배경 전역, TextHtml 파생 정보 출력 전체를 복원했다고 주장하지 않는다.

## 실제 함수와 가상 표

| 역할 | PATCH 10.78 | CD 10.72 |
|---|---|---|
| 생성 | `0046bea0` | `00495270` |
| 출력/용량/링크 저장소 설정 | `0046bed0` | `004952a0` |
| 입력 포인터 설정 | `0046bf00` | `004952d0` |
| NUL 문자열 추가 | `0046bf10` | `004952f0` |
| 그림 제어문자 helper | `0046bf40` | `00495310` |
| 링크 제어문자/저장소 helper | `0046bfc0` | `00495370` |
| 태그 변환 | `0046c090` | `00495420` |
| 바이트 전체 순회 | `0046c810` | `004957b0` |

PATCH 기본 VT `0050bca0`의 슬롯은 `[004437f0, 004437f0, 0040b380]`이다. 앞의 두 함수는 빈 반환이고 마지막 함수는 `ret 4`다. 순회는 각각 태그 내부 일반 문자, 닫힘 slash로 입력 이동 뒤, `>`를 읽고 `inTag=0`으로 쓴 뒤 호출한다. 기본 VT에서 이 호출들을 빈 훅으로 처리하는 것은 검증된 기본 동작이다.

PATCH TextHtml 생성 `004c4c00`은 기본 생성 뒤 VT `00514ca4`를 쓰며 `shown=0`, 이름/길이/정보 종류/Gump/원문 스트림을 파생 필드에 보관한다. 파생 슬롯은 `[004c4cc0,004c5240,004c4c60]`이다. `004c4cc0`은 name anchor 선택과 타입별 정보/번역·Config·현재 객체 조회 및 중첩 HTML 파싱을 수행한다. `004c4c60`은 원문 스트림이 있으면 `>` 다음 원문을 실제 stream append로 복사한다. 정밀 C에 별도 함수로 남지 않은 `004c5240`도 PE 명령을 직접 확인했다. 현재 입력 바로 앞 문자가 LF/CR이면 `textAllowed=0`을 쓴다(`004c5240..004c5255`). 이 파생 동작들은 기본 파서와 같다고 처리하지 않고 `insideTag`, `beforeClose`, `afterTag` 훅으로 분리했다. 파생 TextHtml 전체의 공급자는 후속 복원 대상이다.

일반 Tell `004cf270`은 로컬 기본 파서에 용량 `0x2000`을 설정하여 본문을 변환한다. 버튼/입력 처리 뒤 다시 출력/입력을 설정하고 변환할 때도 같은 객체의 `shown/hiddenDepth`가 남는다. 새 `Parse()`는 이 생성 이후 출력설정→입력설정→전체순회를 하나의 호출로 제공한다.

## 원문 규칙

첫 태그 바이트만 `movsx`로 signed char 확장 후 CRT `tolower`에 보낸다. 둘째/셋째 바이트는 literal 비교다. `<BR>`는 `B`만 소문자가 되고 둘째 `R`은 유지되어 bold 분기를 고른다. `<br>`만 BR 분기를 고른다. `<a href=...>`의 셋째 `h`는 링크 열림을 선택하지만 `<ahref=...>`는 셋째 `r`이므로 선택하지 않는다. 순회의 anchor 카운터 증감도 첫 바이트 literal 소문자 `a`만 사용한다. 태그 전체 소문자화·HTML 표준 정규화·entity decoding은 수행하지 않는다.

| 태그 계열 | 열림 | 닫힘/효과 |
|---|---|---|
| `b` | `~N~B` | `~N` |
| `br` | LF, `afterHeading=0` | 동일 |
| `c` | `~N~i` | `~.` |
| `i` | `~N~j` | `~.` |
| `q` | `~N~q` | `~.` |
| `h1` PATCH | `~N~4~E` | `~*~N~.` + LF, `afterHeading=1` |
| `h2` PATCH | `~N~3~E` | 동일 |
| `h3` 양판 | `~N~5~B~E` | 동일 |
| 나머지 `h` PATCH | `~N~1~E` | 동일 |
| `h1` CD | `~N~3~E` | `~*~N~.` + LF + 공백 + LF, `afterHeading=1` |
| 나머지 `h` CD | `~N~3~E` | `~*~N~.` + LF, `afterHeading=1` |
| `p` | 보통 `~*` + LF + 공백 + LF | 닫힘도 동일. `afterHeading!=0`이면 `~*` + LF만 쓴다 |
| `~x` | `~x` | 동일 |

P 전역 `PATCH00565ec0/CD00536368`이 비영이면 해당 P는 출력 없이 전역만 0으로 돌린다. 이때 `afterHeading`은 지우지 않는다. 별도 공유 객체 `HtmlTextParserGlobals`가 이 전역을 소유한다. heading 뒤 출력 가능한 평문을 쓸 때 `afterHeading`은 현재 문자가 공백인지만 남기므로 공백 아닌 본문 뒤 P는 다시 빈 줄을 쓴다.

PATCH 정밀 C의 `~` 분기는 로컬 문자열에 둘째 문자를 쓰는 store를 표현하지 못했다. 실제 `0046c0e5..0046c0f7`의 `mov cl,[eax+1]`/`mov [esp+c],cl`과 원본 fixture로 `~x` 출력을 확인했다. CD 정밀 C에는 이 대입이 남아 있다.

평문은 unsigned byte `>31`만 그대로 복사한다. LF는 직접 줄바꿈을 쓰지 않으며 출력이 비어 있지 않고 마지막 출력이 공백/LF가 아닐 때 공백 하나를 추가한다. CR/tab/그 밖의 낮은 제어 바이트는 건너뛴다. 태그 효과가 만든 LF는 그대로 남는다. 매 순회 마지막에 LF 하나와 NUL을 쓴다. C++ 반환 문자열의 `size()`는 NUL을 포함하지 않는다.

용량은 평문을 처리하기 전에 **현재 출력 길이 > capacity-256**이면 중단하는 strict 비교다. 따라서 경계와 같은 길이에서 평문 한 바이트를 더 쓰며 태그 확장에는 이 논리 검사가 없다. 호스트는 실제 물리 용량 밖의 쓰기를 `length_error`로 거부하며 `capacity<2`도 거부한다. 원본의 실제 메모리 오버런을 재현하지 않는 범위 보호다. 입력은 첫 NUL에서 끝나고 각 virtual/effect callback 뒤 현재 커서/출력/상태를 다시 읽는다.

## 연결 API와 명시 효과 경계

```cpp
HtmlTextParserState state; // 같은 Tell의 shown/depth를 유지한다.
HtmlTextParserGlobals globals; // 같은 P 억제 전역을 연결한다.
HtmlTextParserHooks hooks; // 선택될 실제 외부 효과만 공급한다.
HtmlTextParser parser(state,globals,hooks,edition);
auto styleBytes=parser.Parse(rawHtml,8192); // 기존 다중 줄 텍스트 파서로 전달한다.
```

`state.shown/hiddenDepth`는 호출 사이 유지한다. 출력설정과 같은 `output.clear()/afterHeading=0` 뒤 입력 위치를 0으로 설정하고 `textAllowed=shown/inTag=0`으로 순회를 시작한다. 순회 자체는 매 태그를 통째로 건너뛰지 않으며 파생 훅이 바꾼 `inputOffset`을 다음 조회에 사용한다.

- `command`: `$` 다음부터 본문 NUL까지의 원시 remainder를 공급한다. 예를 들어 `<$foo 1>rest`는 `foo 1>rest`다. 실제 `004c9090`/명령 큐 공급자가 원본 토큰 규칙을 적용하며 본 모듈은 Config parser를 중복 구현하지 않는다.
- `condition`: `?`부터의 remainder와 닫힘 bool을 받고 현재 `shown/hiddenDepth`를 갱신한다. 실제 PATCH 비교 연산자·문자열/숫자 비교·Config 치환과 CD boolean 계산은 외부 공급자 경계다. 실제 조건 결과를 상수 true로 대체하지 않는다.
- `picture`: `!`부터의 원문을 받고 helper가 쓸 실제 제어문자열을 반환한다. 실제 helper의 `~[I...]` 및 그 이후 리소스/StyleTextMarkup 연결은 후속 공급자 범위다.
- `link`: 열림은 태그 원문, 닫힘은 빈 원문과 bool을 받는다. 실제 링크 저장소/주소가 들어간 `~[T...]`를 반환한 뒤 본 모듈이 원본 색/스타일 문자열을 추가한다. 호스트 토큰과 원본 주소를 혼용하지 않는다.
- `background`: `ba` 계열의 `=` 뒤 optional 첫 따옴표를 제거하고 다음 따옴표/`>`/NUL 앞까지의 값을 전달한다. 원본 배경 전역에 쓰는 실제 공급자가 필요하다.

해당 분기가 실제 선택됐는데 효과 훅이 없으면 `logic_error`다. 숨김 상태여서 분기가 선택되지 않은 비조건 태그는 훅을 요구하지 않는다. 알 수 없는 태그의 기본 무효 동작은 원본 태그 switch와 같다. 기본 VT의 세 슬롯 이외에 TextHtml 파생 기능을 원하면 파생 훅을 별도로 공급해야 한다.

## 독립 근거와 검사

`tools/decomp_htmltext_oracle.py`는 C++를 읽거나 실행하지 않고 두 보호 PE의 실제 생성/출력설정/입력설정/태그/순회/기본 VT 몸체를 Unicorn에서 실행한다. 합성 객체·입력·출력·스택을 원본과 분리하고 실행 주소와 native 쓰기를 제한한다. 최상위 thiscall 정상 반환·callee 스택 정리·비휘발 레지스터 보존·출력 NUL/물리 범위도 확인한다. 게임/GUI/OS 호출은 **0**이다.

**816 고유 입력/816 순회 정상 반환**(판본별 408)을 `tests/fixtures/htmltext-x86.tsv`에 저장했다. 실제 실행 명령은 PATCH851,720/CD719,316개다. `cpppj/htmltext.json`에 주소 범위/판본/host/입력집계/원본·생성기·fixture SHA-256을 보관한다. `--verify`는 해시와 사전에 정한 입력 계획·행수·반환 집계를 재실행 없이 감사한다.

CRT `tolower`는 **signed 인자와 ASCII 변환을 명시 ABI 경계**로 공급했다. 실제 호출 인자에는 -128/-1/0이 포함되며 high byte 평문도 검사한다. 비ASCII locale 변환을 복원/관찰했다고 주장하지 않는다. 링크 저장소는 실제 0으로 설정하여 실제 helper의 null 저장소 동작만 검사했다. 조건·명령·그림·배경 분기의 실제 효과와 TextHtml 파생 전체는 native fixture 범위에서 제외했고, 이 경계를 0 효과의 완료 경로로 세지 않는다.

검사 입력은 B/BR/A의 부분 case 차이, heading/닫힘/P 전역, 공백/LF/CR/tab/낮은 제어/high byte/NUL, malformed tag, anchor의 signed 카운터 중단, `shown=0/1/ffffffff`, `hiddenDepth=0/2`, 용량 2/255/256/257/258/512/8192의 strict 경계를 포함한다. 초기 native 대조가 **CD `</h1>` 추가 빈 줄의 오복원 9건**을 검출하여 실제 PE와 맞게 교정했다. 별도 효과 검사에 공백 없는 `ahref`를 넣은 setup 오류 1건도 원본 셋째 바이트 규칙에 맞는 `a href`로 고쳤다.

최종 Release 게임/검사 빌드는 경고·오류 0이다. 새 집중 **5개/실패 0**이다. native 바이트/최종 커서·6개 상태값의 전수 대조 외에 선택 효과 누락의 거부, 숨김 분기, 상태 callback 뒤 재조회, virtual 슬롯 순서/현재 커서, 실제 StyleText 조각의 inline 색/굵기를 확인한다. `edge`는 색 j와 style0, `open`은 style2이며 임의 inline LF가 생기지 않는다. 실제 게임 화면/기존 Tell 배치 통합과 전체 회귀는 실행하지 않았다.

로그는 `extracted/htmltext-build1..3.log`, `extracted/htmltext-tests1..3.log`에 보관했다. 원본 정밀 C는 수정하지 않았고 마지막 정밀 디컴파일 host는 계속 **VM-W11-Codex**다.
