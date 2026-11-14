# webpj 비용 표시·생산 실패 안내 Gump FloatingSpGump/ProductionHintGump (WB04-E434, 2026-11-14)

RawMenuControls 선행 **FloatingSpGump 87줄·ProductionHintGump 85줄(.cpp/.h, todo→port)**을 새 모듈 [`www/ui/floating-sp-gump.js`](../webpj/www/ui/floating-sp-gump.js)·[`www/ui/production-hint-gump.js`](../webpj/www/ui/production-hint-gump.js)로 이식했다. 둘 다 한 줄 `TextGump`(E423) 파생이며 기본 생성/수집/수명/시계/StyleText는 훅으로 연결하는 소형 처리기다. 기준선 `1f00a25a` 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손으로 계산한 값이다. 원본 직접 관측 수치로 세지 않는다. CD 10.72의 가상 표/몸체는 옮기지 않았다.

## 기준선 뒤 변경 (3.6절)

두 파일 모두 기준선 뒤 변경이 없다.

## 구현

- **`FloatingSpGump`(비용 표시, 0xb8)**: `construct`는 한 줄 생성 → 가상 표 `0x50688c`·시각 0·가속도(float32)·속도 0 → Clear → 서식 `~3~E~y<비용 signed 십진>~[IfortGump.3]` → 측정 → idle 등록 → **현재 시각을 stamp로, 깊이 `0xffffffd8`** 순서이며 노드를 단계마다 다시 조회한다. `show`는 `new(0xb8)` → 가속도 -2 생성 → 참조가 있으면 객체 참조 경계(없으면 거부) 후 위치 (0,0), 없으면 float32로 좁힌 (x,y) → 실제 루트에 `Attach(mode 0)`한다. `idle`은 시각 차이를 double로 구해 stamp에 쓰고 redraw → `y = fround(속도+y)` → **범위 이탈**(y<0, x<0, 화면 폭<x, 화면 높이<y — x87 C0/C2 parity라 NaN은 이탈 아님)이면 idle 해제 뒤 Kill, 아니면 redraw 후 `속도 = fround(가속도×시간차+속도)`이다. `deleteScalar`는 기존 TextGump 소멸/반납에 위임한다.
- **`ProductionHintGump`(생산 실패 안내, 0xb8)**: `construct`는 한 줄 생성 → 가상 표 `0x50c2b4`·만료 0·`command=signed 종류 BYTE`(sign 확장)·문맥 `[0,6,흰색,0,0,검정,1,1]`·기준 0 → 문자열 복제(소유) → 파싱 → 측정(6,4) → idle → **만료 = 현재 벽시계+4초**다. `show`는 두 원문(`~0~w~ENone in inventory`/`~0~w~ENot enough Storm Power to build this`)을 wrapper 위치에서 번역해 생성하고 `flags|=0x4000`, 기준 영역(l,t,r,b)을 float32 비트로 `positioning[2..5]`에 저장, `flags|=0x8100`·깊이 `0xffffff9c`·기준 토큰, `Attach(x, y+20(DWORD 감김), 루트, 0)` 뒤 마지막 serial을 기록한다. `idle`은 `만료 <= 현재 시각`이면 Kill(NaN은 만료 아님), `draw`는 경계·위치를 읽고 head를 (3,2) 여백으로 출력(빈 head도 StyleText 경계 호출), `deleteScalar`는 가상 표 → 소유 문자열 반납(필드는 0으로 지우지 않음) → TextGump 소멸이다.

## 검사와 발견

새 `tests/floating-sp-gump.test.mjs` 7건·`tests/production-hint-gump.test.mjs` 7건 **통과**. 한 줄 TextGump/StyleText/수명/시계/번역/문자열 소유는 호출 기록 대역이다.

**최초 실패와 교정(제품 결함 아님, 시험 입력 오류 2건):** 속도/위치 float32 시험에 화면 밖 값(16777216)을 줘서 이탈 분기로 들어갔고, 첫 redraw 콜백이 두 번 모두 위치를 덮어썼다.

**변형 확인:** 연산자/정수 상수 변형 78개(floating 30·production 48) 중 77개를 잡았다(처음 생존 13개 → 필드 기본값·생성 중 콜백이 보는 시각/만료·참조 1·미사용 `need` 제거 보강). 남은 1개는 `floatBits`의 쓰이지 않는 여분 버퍼 크기라 동등 변형이다.

## 남은 경계

- 실제 RawMenuControls 허브가 이 두 처리기를 등록표/수명/수집에 조립한다. 앱의 비용 표시/생산 안내는 아직 기존 간소화 경로이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
