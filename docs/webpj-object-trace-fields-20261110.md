# webpj 객체 trace 필드 서식(RawObjectTraceFields) 이식 — WB04-E230

2026-11-10, HJOW-Athlon. 객체 Base(004ac400)와 내장 Form 네 종류(Form 004ae1e0·Process 004ae3f0·Depend 004ae530·Content 004aea90)의 원문 진단 서식과 현재 필드 조회를 옮겼다. 개발자 로그(trace)용 서식이지만 원문 바이트까지 정해진 순수 규칙이라 원본 관측과 한 바이트도 다르지 않게 맞췄다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E229 Chat manager 랭크 prefix 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawObjectTraceFields.cpp/.h`)·시험(`ObjectTraceFieldsTests`)·fixture(`object-trace-fields-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `4d0b8061`(2026-10-29).
- 원본 fixture: `object-trace-fields-x86.tsv` 308행(10.78 전용 관측; 5개 본체 × 상태 16조합·덤프 모드 0/1/2·이름 10종·정수 경계·기록 뒤 필드 변경). 입력 6칸(진입 본체·SID·덤프 모드·변경 여부·이름 hex·50 BYTE 슬롯 hex), 출력 1칸(실제 로그 장치가 받은 `레벨:서식 완료 바이트 hex` 를 `;` 로 연결).
- 공유 장치: `sid-pool.js`(`slot`·`physical`·`capacity`). 시험의 Content 이름 서식 일부(폭·정밀도·정렬)는 원본 관측이 아니라 표준 C 규칙이다(아래 3절).

## 1. 옮긴 것

`www/core/raw-object-trace-fields.js`(DOM 없음, 10.78 전용)

- `ObjectTraceFieldKind`(`Base`·`Form`·`Process`·`Depend`·`Content`)·`traceRawObjectFields(pool, types, suppress, sid, kind, log)`: 공통 Base 한 줄 `~1~y<SID 4자리>~n Base type=<TYPE>/~y<이름 앞 20자>~n [next=~y<4자리>~n ][head=~y<4자리>~n ][~rFREE~n ][~rDEAD~n ][~rVOID~n ][CONTAINED ]\n` 을 레벨 29 로 기록한 **뒤에** 슬롯을 다시 읽어(기록 장치가 첫 줄 직후 필드를 바꿔도 그 값) 종류별 두 번째 줄을 기록한다: Form 부모(+16)·이전(+14, 0 이면 생략), Process 부호 있는 PID(+18, 3자리), Depend 감시 쌍(+18·+20), Content 내용 TYPE(+18)·이름(`sprintf(name,"XXX")` 치환 뒤 20자)·수량(+12)·flags(+20, 소문자 16진수).
- next/head 는 값이 있고 `suppress` 가 0 일 때만 출력한다. 이름은 첫 NUL 에서 끝나며 Base 줄에서 20자로 줄인다. 숫자는 C 의 `%0*d`(폭이 부호를 포함).
- Content 이름 서식은 단일 `%s`(→`XXX`)·`%%` 만 받는다. 추가 인자·`%d` 같은 다른 서식·`-` 외 플래그·80 BYTE 임시 버퍼 초과는 거부한다(앞선 Base 줄은 이미 기록됨). 폭·정밀도·왼쪽 정렬은 표준 C 규칙으로 구현했다.
- 풀/표 판본 검사(TypeError)·범위 밖 SID/TYPE(RangeError)·로그 장치 부재(TypeError, 첫 줄 이전 거부).

## 2. 검증

`tests/object-trace-fields.test.mjs`(3건)

- 원본 관측 재생: 308행 전부. 행마다 새 풀에 50 BYTE 슬롯과 TYPE 이름 표(46·82 에 같은 이름, 94 에 `islandAfterCallback`)를 설치하고 로그 장치가 받은 `레벨:hex;…` 전체를 대조한다. 변경 입력은 첫 로그 직후 슬롯의 +14·+16·+18·+12·+22 를 바꿔 다시 읽기를 확인한다.
- cpppj 계약 포팅 1건: 잘못된 `%s/%s`(추가 CRT 인자)와 80 BYTE 이름(임시 버퍼 초과)을 거부해도 첫 Base 로그 1건은 보존, 로그 장치 부재는 로그 0건으로 거부.
- 웹 계약 1건: Content 이름 서식(`[%s]`→`[XXX]`, `%5s`·`%-5s`·`%.2s`·`%.s`, `100%%`→`100%`)과 `%+s`·`%d` 거부, 범위 밖 SID(24000)·범위 밖 현재 TYPE(200)·다른 판본 풀 껍데기·187개 TYPE 표 거부.

최초 실패와 교정: 모듈은 처음부터 308행이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 40건 중 6건이 살아남아 3건은 시험을 보강해 잡았다(`%.s` 정밀도 0, 판본/표 크기 검사).

- 실패한 변형: 로그 레벨·이름 길이·버퍼 한도·필드 위치(TYPE/상태), 정수 폭/부호/채움, 이름 범위·NUL 절단, `%%`·단일 `%s`·플래그·정렬·정밀도·버퍼 검사, 판본·표 크기 검사, 덤프 모드 조건(next/head 각각), 상태 비트 네 종류 교환, 파생 필드 재조회 제거, Base 의 조기 반환, 종류별 필드 위치(Form 부모/이전·Process 부호·Depend 쌍·Content flags/수량/TYPE/20자 절단).
- 적용되지만 통과한 변형(동등 변형): ① Content 이름 한 변환의 80 BYTE 검사 — 결과 전체 80 BYTE 검사가 항상 같이 걸린다. ② 로그 함수 존재 검사 제거 — 함수가 아니면 호출 자체가 TypeError 라 같은 결과다. ③ 모듈 쪽 SID 범위 검사 제거 — `SidPool.slot` 이 같은 RangeError 를 던진다.

## 3. 경계와 남은 것

- 실제 로그 장치(레벨 29, 서식 파서 `~1`·`~y`·`~n`)는 연결하지 않았다. 호출 지점(각 객체 vtable +70 과 파생 자산의 Main/Task)은 순서4 몫이다.
- Content 이름의 폭·정밀도·`-` 정렬은 원본 관측이 아니라 표준 C 규칙이다(원본 관측 이름은 `%sGenerator`·`percent%%name` 등 단일 `%s`/`%%`). `+`·공백·`#`·`0` 플래그는 CRT 마다 달라 거부한다.

## 4. 다음 시작점

WB04-E231 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`SquidSpatial`·`RawTerrainGrowth`·`RawTotalMade`·`RawMissionFrameGate`)를 권한다.
