# webpj 입력 명령 이름표·우선순위 루프 원본 관측 연결 (WB04-E479, 2026-11-15)

3.8절 이월 목록의 "입력 명령/설정" 중 이식 모듈이 이미 있는 다섯 fixture **222행**을 원본 10.78 독립 관측에 대조했다. 모두 첫 실행에서 전체 일치했고 이식 결함은 나오지 않았다.

| fixture | 행 | 대상 | 비교한 것 |
| --- | --- | --- | --- |
| `ai-utility-names` | 30 | `aiUtilityCommandScriptCode` + `queueScriptCommand` | 원래/대소문자·숫자/문자열 인자·미등록 입력의 명령 큐 사건(코드, x==y, 인자 값/고리 문자열)·문자열 고리 위치·진단 횟수 |
| `make-ai-names` | 12 | `makeAiCommandScriptCode` | 같은 항목(MakeAi 이름표) |
| `ai-debug-names` | 21 | `aiDebugCommandScriptCode` | 옛 별칭 21행: 전부 미등록(큐 비움·진단), 고리 위치 |
| `ai-debug-toggle-names` | 63 | 같은 함수 | 실제 Toggle 이름 63행: 등록 32행(숫자/문자열/첫 NUL)과 철자·공백이 어긋난 미등록 31행의 큐·고리·진단 |
| `priority-loop` | 96 | `planNativeThreadPriority` | WinMain 국소 우선순위 루프: raw 요청값(범위 밖 포함)·OS 등급 정규화(0/1/2)·설정 여부·선택 우선순위 |

- 이름표 시험은 원본의 명령 큐 넣기(004c9090)를 실제 `InputQueue`/`ScriptStringRing`에 재생한다. 행의 `events` 열에서 진단 `L` 횟수와 새 고리 문자열 `New` 횟수도 같이 대조했다(`New` 횟수 = 고리 위치).
- `ai-debug-names`(21행)와 `ai-debug-toggle-names`(63행)는 cpppj `AiDebugMenuTests`가 합쳐 84행으로 쓰므로 한 시험으로 묶었다.
- 입력 열만 읽고 관측 열은 비교에만 쓴다(cpppj 시험과 같은 방식). 원본 게임을 실행하지 않았다.

## 변형 확인

- 자동 변형(연산자/상수) 33개(AI 진단 6·MakeAi 5·설정 18·AI 일반 4): 29개 잡힘, 4개 생존. 생존 4개는 모두 동등하다 — 이름 조회의 `end < 0`/`nul < 0`을 `<= 0`/`< 1`로 바꾸면 선행 NUL 이름이 빈 문자열 대신 전체 문자열이 되는데, 어느 쪽도 등록 이름과 같지 않아 결과가 모두 `0xffffffff`이다.
- 수동 의미 변형 9개(대소문자 비교 제거·반환 코드 교체·별칭 추가·접두 일치·NUL 절단 제거 등): 처음에는 별칭 추가(`debug`·`crankmoney`·`aiparam`) 변형 3개가 fixture 가 담지 않은 입력이라 생존했다. 근접 이름(접두/접미/별칭/공백/선행 NUL/다른 모듈 이름) 독립 시험을 추가해 모두 잡았다.

## 남은 것

`audio-reinterpret`(193행)은 원본 장치 열기/닫기 반환과 음량 순서를 보는 `ClientAudio` 옵션 재해석인데, 웹의 `ClientAudio`는 Web Audio 로 바꾼 적응 모듈(PORT_MAP `adapt`·partial)이라 같은 형태의 재해석 경계가 없다. `font-face-enum`/`font-face-lookup`(389행)은 GDI 글꼴 열거 결과를 담는 `FontStore` 목록 조회라 브라우저에는 대응이 없다. 둘은 원본 동작을 웹에서 어디까지 보존할지의 적응 판정(3.6절 ①②)이 먼저이므로 이 묶음에서 제외하고 3.8절에 사유와 함께 남긴다. `detonate-input`(769행)은 `RawObjectConversion` 단위에서 대조한다. 3.8절 나머지 항목은 후속 단위다.
