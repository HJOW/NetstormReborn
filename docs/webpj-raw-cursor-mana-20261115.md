# 커서 마나 표시 이식 (WB04-E549)

F(입력·커서·선택)의 "커서" 중 `client/RawCursorMana`(원문 10.78 004737f0 생성자·00473d00 프레임·00473c30 통지·00473b90 소멸과 `CursorManaProcess`)를 `webpj/www/core/raw-cursor-mana.js`로 옮겼다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. 기존 `CursorDropState`(소멸 때 같은 커서 토큰을 지움)·`SquidProcessState`(현재 double 게임 시각)·`SquidProcessHost`/`Kernel`(프로세스 수명)을 그대로 빌린다.
원본 10.78의 `Cursor`가 부르는 `createMana`/`removeProcess`/`processEvent`(`raw-cursor-lifecycle.js`)에 `bindCursorManaLifecycle`로 연결한다.

## 옮긴 범위

- 필드: 원본 `new(0xe0)` 224바이트를 `Uint8Array`로 보존한다(미사용 padding 포함). float는 저장 위치에서만 단정도로 좁히고 나머지 계산은 double로 한다. 목록 30칸은 DWORD SID이며 호스트의 16비트로 절단하지 않는다.
- `construct`(004737f0): 마나 객체 8개를 만들고(타입 103·flags2, WORD 1, extra 비트 8, 현재 theme 프레임) 범위 밖 대체점에 둔 뒤 `Attach(type 50, flags 0x10)`와 시각/숨김 초기화를 한다. 사각 모드 분기(5변)는 원문대로 보존하지만 모드 칸을 곧바로 0으로 쓰므로 생성자에서는 도달하지 않는다.
- `frame`(00473d00): 원형(기본)은 반경을 15/초로 키우고 닫는 중이면 30/초로 줄여 0.1(단정도) 이하에서 종료하며, 객체마다 `2π/개수×번호 + (시각−정수부)×π×0.5` 각도로 부모 중심 둘레에 놓는다.
  사각 모드는 위상 7칸을 증가/대칭 복사한 뒤 4변을 부모 좌표로 이동하고 한계 반경을 넘은 객체를 삭제하며 남은 객체가 없으면 종료한다. 가상 효과 뒤에는 현재 개수·시각·부모·목록을 매번 다시 읽는다.
- `notify`(00473c30): 이벤트 3은 종료 표식, 1은 숨김 값 전체 DWORD와 각 객체의 현재 void 여부 반영, 그 밖은 3초 뒤 기한이다.
- `destroy`(00473b90): 현재 커서 토큰을 먼저 지운 뒤 free/dead가 아닌 표시 객체를 삭제하고 필드를 기반 클래스 값으로 무효화한다.
- `CursorManaProcess`/`createCursorManaProcess`/`bindCursorManaLifecycle`: 실제 Host/Kernel 부착·프레임·통지·제거(자기 삭제 프레임의 꼬리 쓰기까지 필드가 살아 있음), `bindCursorManaWorld`: 실제 월드의 생성/WORD(다리 TYPE 통지)/해제/재등록/배치/삭제 연결, `cursorManaThemeFrame`: theme→프레임 표(`[4,5,6,7,4,5,6,7,8,9,10]`).
- 공개 정적 보조 `RawCursorMana.timeInteger`(`_ftol` EAX)·`trig`(x87 범위 밖 입력 유지)는 직접 값 검사용이다.

## 시험

`webpj/tests/raw-cursor-mana.test.mjs` 15건:

1. 원본 관측 `cursor-mana-x86.tsv` **642행**: 36 DWORD 독립 입력(생성·프레임·통지·소멸·연속 호출)의 효과 순서·224바이트 필드·전역(커서 토큰·시각 비트·반환 표식)·raw 슬롯 31개·단계별 필드 전부와 기본/64비트 두 관측의 일치. **첫 실행에서 일치**했다.
2. 표시 두 건: `cursor-mana-display-x86.tsv`(473770/8c)·`mana-bolt-display-x86.tsv`(45ac90/8c) 각 **432행**을 실제 `SquidDisplay`·`DirtyRegions`로 재생(WORD 조건·10픽셀 확대·억제). 첫 실행 일치.
3. 계약: 미연결 경계·판본/TYPE 표 크기(187/189)·theme 범위·목록 범위 거부, 실제 Host/Kernel 프로세스 수명(부착·프레임·통지·제거·낡은 번호 거부·flags 전달), 월드 연결.
4. 변형 확인에서 드러난 미관측 경계의 계약 시험: 원형 델타의 float 저장(성장·축소), 시간 역행, 종료 경계(단정도 0.1과 다음 값), free/dead/void 종료와 시각 필드 미갱신, 가상 해제 중 부모 필드 변경, 중심+x의 단정도 합 경계(255.99998…+7.7e-6),
   사각 모드의 델타 float 저장·삭제 경계(`>`/`||`)·위상 범위·음수 절반 개수(0 쪽 절삭), 생성의 대체점/공급 반경/발자국 감김/부모 TYPE/목록 넘침/Attach 쓰기, 좌표 제한 경계와 정수 감김, 소멸 토큰의 부호 없는 폭과 free/dead 건너뜀, 시간 절삭/삼각 함수 범위.

**변형 확인:** 테마 표·시간/삼각 범위·좌표 제한·생성/프레임/통지/소멸의 모든 비교·상수·필드 오프셋·효과 인자와 프로세스/수명/월드 연결 165가지를 넣어 153가지가 시험에서 잡혔다.
처음에는 50가지가 살아남았고 위 계약 시험으로 38가지를 잡았다. 남은 12개는 동등 변형이다:
① `Math.trunc(v)|0`은 `BigInt` 하위 32비트와 같다. ② 부모 SID/표 접근이 풀 크기와 같을 때도 뒤따르는 슬롯 조회가 같은 `RangeError`를 낸다(2개).
③ 사각 위상 접근 상한(`item>=7`)·하한(`item<0`)·첫 복사 상한은 도달할 수 없거나 같은 오류를 낸다(개수 8 이상은 둘째 복사에서 먼저 거부, 인덱스는 음수가 될 수 없음; 3개). ④ 사각 삭제 목록 범위 확인은 앞선 목록 조회가 이미 거부한다.
⑤ 사각 종료 조건(`!processed`↔`processed<1`)은 정수라 같다. ⑥ 반경이 한계와 같을 때 성장 상한(`>=`↔`>`)과 닫는 중 하한 `value<0`↔`<=0`은 같은 값을 저장한다(−0은 도달 불가). ⑦ 각도의 시간 정수부(`timeInteger`↔`trunc`)는 게임 시각이 2^31초 미만이면 같다.
⑧ 수명 연결의 Form 상한(65535↔65536)은 호스트가 같은 `RangeError`로 거부한다.

`webpj/tests/fixtures/manifest.json`에 `cursor-mana`(642행)·`cursor-mana-display`(432행)·`mana-bolt-display`(432행) 세 항목을 추가했다.

## 남은 경계

- 월드 연결(`bindCursorManaWorld`)의 중심 조회·표면 통지 장치와 실제 `Cursor` 수명 연결은 호출자가 공급한다(PORT_MAP `port`). 앱 수명에는 연결하지 않았다.
- 사각 모드 생성 분기는 원문 보존용이며 현재 관측으로 도달하지 않는다. 사정거리 표시(`RawCursorRange`)는 별도 단위다.
