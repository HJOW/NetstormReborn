# HTML 조건의 실제 토큰·Config 치환·가림 상태 복원

현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 보호 PE와 같은 PC의 정밀 C를 읽었으며 실제 게임·GUI·OS API를 실행하지 않았다. 원본 정밀 C·보호 파일·AGENTS·dotnetpj는 변경하지 않았다. `DialogScript`, `UberGump`, `Renderer`, CMake와 LEFT는 부모 소유이며 이 단위에서 직접 수정하지 않는다.

## 구현 범위와 연결

`client/HtmlConditionEffect`는 PATCH `0046c090`의 `?` 분기와 CD `00495420`의 boolean 분기를 `HtmlTextParserHooks::condition`에 공급한다. 현재 `HtmlTextParserState`를 참조하여 `shown/hiddenDepth`를 읽고 쓴다. 기존 `FilterDialogConditions`의 자체 문법/고정 참거짓 결과를 재사용하지 않는다.

```cpp
HtmlTextParserHooks hooks; // 다른 실제 효과와 함께 같은 훅을 사용한다.
BindHtmlConditionConfig(hooks,configuration,edition); // 현재 ConfigInterface를 매 치환 시 다시 읽는다.
HtmlTextParser parser(state,globals,hooks,edition); // 기존 전체 순회가 조건 실행 여부를 정한다.
auto styleBytes=parser.Parse(rawHtml); // 현재 상태의 참조를 같은 effect에 전달한다.
```

독립 검사/다른 Config 공급자는 `HtmlConditionEffectHooks::substitute(token)`와 `HtmlConditionEffect::Apply(state,raw,closing)`를 사용한다. `raw`는 `?`부터 NUL까지의 원문이며 닫힘에서도 `?`를 포함한다. binder는 각 실제 치환에서 기존 `ConfigInterface::SubstituteLocal(token)`을 호출한다. 외부값 snapshot이나 별도 Config parser를 만들지 않는다.

이 모듈은 조건 효과이며 페이지 메타/버튼 추출기를 새로 구현하지 않는다. 실제 본문에서 기존 선행 Filter가 조건 태그/구간을 이미 제거했다면 effect가 원문을 다시 판정할 수 없다. 부모의 실제 통합에서 원본 조건을 남기고, 조건 안 명령/버튼 메타가 표시 상태를 존중하도록 기존 페이지 parser의 선택 경계도 함께 연결해야 한다. 이 단위만으로 기존 페이지 전처리의 동일성이 완료되지는 않는다.

후속 연결을 위한 원문 확인에서 `004cf270` 자체가 먼저 `00441660(...,0x2000,0,0,0)`로 **본문 전체 Config 치환을 실행한 뒤** 기본 HTML의1차순회를 수행함을 확인했다. 따라서 현재 초기 전체 치환을 조건 평가 뒤로 옮기는 것도 원본과 다르다. 이 effect의 치환은1차 전체 치환 결과의 조건 토큰을 다시 치환하는 다음 시점이다.1차 HTML 출력은 `004c8d50`의 alnum 존재 검사→`004c83b0` 메타/버튼 처리→LF를`<br>`로 치환(`00504b0c`→`00515eb8`)→같은 HTML 객체의 출력/입력 재설정·2차순회 순서로 이어진다. 이 전체 Tell 부모는 아직 이 단위의 native 실행/복원 범위에 포함하지 않는다.

## 실제 하위 호출과 원문 규칙

| 역할 | PATCH | CD |
|---|---|---|
| 태그 분기 | `0046c090` | `00495420` |
| 토큰 현재 포인터 wrapper | `00487710` | `0041a4a0` |
| 최대78입력 바이트 토큰 | `00487620` | `0041a3d0` |
| 임시 결과/Config 치환 wrapper | `00441740` | `004a9bd0` |
| 빈/인자 local 객체와 실제 치환 | `00441660`→`00440b60` | `004a9ae0`→`0042a960` |
| 실제 숫자 변환 | `004e66de` alias→`004e6656` atol | `004f1ea0` atoi→`004f1df0` atol |

PATCH 태그의 첫 `?` 다음에서 optional `!` 하나를 소비한다. 이어 optional `?`는 문자열 동등 연산(1), `g/G`는 숫자 greater(2), 뒤 `e/E`는 greater-or-equal(3), 그 뒤 `l/L`는 less(4), 뒤 `e/E`는 less-or-equal(5)를 선택한다. 원본의 연속 literal 검사이므로 태그 전체를 소문자로 만들거나 연산자 문자열을 표준 문법으로 재해석하지 않는다. `g` 뒤의 `l`처럼 비정상으로 보이는 조합도 실제 다음 바이트 검사와 마지막 연산자 선택을 따른다. 연산자 다음 optional `!`도 하나 소비한다.

두 `!`는 XOR로 상쇄되지 않는다. 앞/뒤 중 하나라도 있으면 기대 predicate가 false이고, 둘 다 없어야 true다. CD는 첫 `!`와 boolean 숫자만 사용하며 PATCH의 비교 연산자를 해석하지 않는다.

boolean은 실제 구분자 **`=>`**로 토큰 하나를 읽는다. PATCH `0050bcb4`, CD `0053641c`의 PE 문자열을 확인했다. 비교는 먼저 왼쪽 토큰을 구분자 `=`, 그 다음 오른쪽 토큰을 구분자 `>`로 읽는다(PATCH `00506c68`, `00505318`). 인용/앞 space/tab/뒤 공백/최대78개 입력 바이트/연속 구분자/현재 포인터 이동은 기존 `ReadRawTextToken`의 원본 규칙을 공유한다. 토큰에서 이름이 나왔다고 Config 키 조회로 바꾸지 않으며 원문 전체 치환을 수행한다. `{A}`는 값 치환이지만 `A`는 문자 A다.

## 조회 시점과 비교

원본은 두 토큰을 모두 읽은 뒤 다음 순서를 사용한다.

1. 오른쪽 토큰을 `ConfigInterface`의 **결과 크기100/지역 인자0·0·0** 치환으로 보낸다.
2. 숫자 비교이면 오른쪽 결과의 atol을 즉시 완료한다.
3. 왼쪽 토큰을 같은 방식으로 **그때의 현재 Config**에서 다시 치환한다.
4. 문자열 동등이면 두 NUL 문자열을 대소문자 구분·원시 바이트 동등으로 비교한다. 숫자 비교이면 왼쪽 atol을 수행한 뒤 signed32 값을 비교한다.

문자열 동등은 `stricmp`나 locale 문자열 정렬이 아니다. 실제 태그 몸체의 인라인 BYTE 비교는 대소문자를 구분하고 첫 NUL에서 끝난다. 동등성만 필요하므로 새 모듈은 같은 NUL 앞 byte 문자열의 `==`를 사용한다.

숫자는 CRT C locale의 앞 whitespace(공백/탭/LF/VT/FF/CR), optional 부호, 연속 십진수만 읽고 최초 비숫자에서 멈춘다. 32비트 overflow는 감긴다. 실제 PATCH CRT에서 `2147483648→80000000`, `4294967296→00000000`, `-2147483649→7fffffff`를 확인했고, 이 차이가 predicate를 바꾸는 native 입력도 새 fixture에 포함했다. 기존 `ConfigParseLong`을 이 native 결과와 대조하여 재사용하며 현재 호스트의 long 크기/`std::strtol` 포화값을 가져오지 않는다.

원본 치환 wrapper는 임시 문자열 저장소에100바이트를 예약하지만 Config 본체는 쓰기 길이를 강제하지 않는다. 새 모듈은 NUL 앞 결과가100바이트 이상인 원본 오버런을 `length_error`로 거부한다. 이를 99바이트로 잘라 성공시키지 않는다. 깊이32의 Config 자기참조 제한 등 기존 Config 호스트 안전 경계도 그대로 유지한다.

## shown/depth와 전체 순회

`shown==0 && hiddenDepth==0`이면 조건 효과 자체가 무동작이다. 닫힘은 Config나 토큰을 읽지 않고 비영 depth를 한 번 감소시킨다. 감소 결과가0일 때만 `shown=1`이다. 표시 중 열림의 predicate가 앞/뒤 부정으로 정한 기대값과 다르면 `shown=0`을 쓰고 depth를 DWORD로 하나 증가시킨다. 숨김 중 열림은 새로운 조건을 평가하거나 깊이를 자동 추가하지 않는다.

기존 `HtmlTextParser` 전체 순회는 표시 중 태그, 또는 숨김 깊이가 비영인 `</?...>`만 태그 분기에 보낸다. 따라서 숨긴 nested 열림은 실제로 실행하지 않고 첫 닫힘에서 depth가0이 되면 안쪽 뒤 본문도 즉시 다시 보일 수 있다. 이를 일반 HTML처럼 올바른 중첩 stack으로 바꾸지 않았다. `textAllowed` anchor 카운터 등 다른 현재 필드와 다음 바이트의 조기 종료는 기존 전체 순회가 처리한다. effect가 자체 문단/태그 필터를 중복 구현하지 않는다.

## 독립 native 근거

`tools/decomp_htmlcondition_oracle.py`는 두 PE를 읽어 주 VM에서 실제 기본 HTML 태그/전체 순회, 실제 토큰 wrapper/몸체, 실제 atol/atoi를 실행한다. 기존 HTML VM의 실행/쓰기 범위를 유지하고 필요한 실제 하위 범위만 추가한다. 최상위 정상 반환·thiscall 스택 정리·비휘발 레지스터 보존은 같은 기본 VM 검사를 사용한다. 실제 게임·GUI·OS 호출은0이다.

ConfigInterface ABI 호출은 **분리된 실제 Config VM**에 그 시점의 현재 설정과 빈 local 객체를 공급하여 `Config::Substitute` 실제 몸체/하위를 실행하고, 결과를 원래 주 VM에 반환한다. 태그 wrapper의 임시 메모리 할당·SEH/local 객체 생성/삭제를 하나의 VM에서 끝까지 실행했다고 주장하지 않는다. 각 호출에서 source/요청100/0인자들을 확인하며 원문과 실제 치환 바이트를 순서대로 기록한다. 기존 Config oracle의 ASCII toupper/strnicmp/strchr/getenv/sprintf/assert 경계, HTML의 ASCII tolower, PATCH CRT TLS 포인터 공급은 보고서에 명시했다. Config native 관찰은 C++ 치환 결과를 생성에 사용하지 않는다.

**754 고유 입력/정상 반환**(PATCH489/CD265), 실제 Config 치환 **825호출**(PATCH672/CD153)을 `tests/fixtures/htmlcondition-x86.tsv`/`cpppj/htmlcondition.json`에 보관했다. 직접 태그와 전체 HTML 순회에서 결과 출력·현재 커서·shown/depth/inTag/textAllowed/afterHeading/P전역·치환 인자/결과 순서를 저장한다. 이전 HTML816/그림276/Config2,440 관찰을 새 입력 수로 합산하거나 전체 반복 실행하지 않는다. 첫754행 뒤 생성기의 보고 문자열/한국어 주석을 정리해 같은754개를 다시 저장했으며 고유 입력을 늘린 것으로 세지 않는다.

입력은 표시0/1/ffffffff·깊이0/1/2/ffffffff·열림/닫힘, boolean/문자열/숫자 연산·부분 case·두 부정·연산자 연속 검사, 숫자 overflow/high byte·인용/토큰78/79바이트·비어 있는 값/기본값/미확정 키, 호출 사이 Config 변경, 실제 전체 순회의 숨긴 중첩·anchor·후속 본문을 포함한다. `--verify`는 보호 PE/생성기/의존 도구/fixture SHA와 사전에 정한 고유 입력 계획·행수·반환 수를 재실행 없이 감사한다.

## 집중 검사와 남은 연결

새 `HtmlConditionEffectTests.cpp`는 native754행 전수 대조 외에 오른쪽 치환 뒤 X를 바꿔 왼쪽이 새 Config를 읽는지, 숨긴 열림/닫힘의 무조회, 실제 `param` 이름 객체와 `ShardtoStone`의 less/부정 비교, 누락키 기본값, 미공급 치환과100바이트 오버런 경계를 검사한다. 원본 tell.english에 있는 `<?l {param.3}={ShardtoStone}>`/`<?l! ...>` 형태를 실제 binder와 같은 이름 객체에서 검사한다.

부모가 새 소스/검사 CMake 등록과 공용 Release 게임/검사 빌드를 수행했으며 **경고·오류0·집중4개/실패0**이다. 로그는 `extracted/lifetime-condition-root-build.log`, `extracted/html-condition-root-focused.log`다. Native754행 SHA/입력계획/집계 감사와 새 파일 UTF-8 해석도 통과했다. 같은 소스의 전체 CTest도 **1/1·내부1,218개·실패0, 118.02초**로 통과했다(`extracted/run52-53-ctest.log`).

기존 본문/HTML/UI 파일은 그림 통합을 위해 동결했고 이 단위에서는 새 파일만 수정했다. 실제 페이지에서 선행 Filter/치환/명령·버튼 추출을 원본 현재 표시 상태와 연결하는 일은 다음 별도 단계다. 최종 집중 뒤 등록된 소스/검사는 전체 회귀를 위해 고정한다.
