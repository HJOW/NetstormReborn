# webpj Tell 섹션 조회 (WB04-E461, 2026-11-14)

기준선 `1f00a25a`의 `TellSection.cpp/.h`를 `www/ui/tell-section.js`로 옮겼다. 원본 `4cf730`의 파라미터 처리 뒤 부분(현재 Tell 이름 기록, 세 단계 섹션 조회, 실제 Config 연결, 지역 param 수명을 둔 전체 공장 호출)이다. 파라미터 해석 자체(`TellParameters`)는 기존 `tell-parameters.js`(원본 1,070관측 대조)를 그대로 쓴다.

## 이식한 것

- `createTellSectionState()`: Client 수명의 현재 Tell 이름 256BYTE(`005cab40`). `ClientHtmlWindowGlobals.tell`이 이것을 쓴다.
- `findTellSection(state, name, hooks)`: 이름을 NUL 앞 지역 복사로 만들고(256BYTE 이상이면 아무것도 쓰기 전에 `RangeError`, 현재 이름 저장소에 NUL이 없어도 같다), 이전 이름과 **대소문자 무시로 다를 때만** `prior(이전 이름)`을 부른 뒤 새 이름을 NUL까지 기록한다(NUL 뒤 기존 바이트는 지우지 않으며 조회가 실패해도 기록은 남는다). 이어서 이름에 점이 있으면 `named(접두, 섹션)` → `mission(이름)` → `fallback(이름)` 순으로 조회한다. 없는 섹션(`null`/`undefined`)과 존재하는 빈 본문(`''`)을 구별하고, 선택되지 않은 조회는 요구하지 않는다(미연결은 `Error('Tell 섹션의 선택된 실제 조회 미연결')`).
- `findTellSectionInConfig(state, name, config, mission)`: 실제 설정 인터페이스(`GameData`)에 연결한다. `prior`는 `global.priorTell`에 기존 원문 이름을 쓰고(저장 통지 없이), `named`는 **등록된** 접두 객체의 섹션만 읽으며(파일을 새로 적재하지 않고, 없는 접두어는 오류 없이 없음), `fallback`은 같은 이름의 기존 `tell` 객체와 구별되는 새 임시 `Config('tell')`에 `missionSpec`의 tell 파일을 **매번** 적재해 섹션을 복사한 뒤 반환 전에 등록부에서 내린다. 이름/접두/섹션/이전 이름은 원본 Windows-1252/UTF-8 경계에서만 해독한다(`embedded-tell.js`의 `decodeKey` 재사용).
- `showTellInConfig(state, original, suppress, config, mission, show)`: 지역 `param` 객체를 먼저 등록하고, `parseTellParameters`가 인자를 치환(해독 → 현재 Config 치환 → 원본 BYTE로 되돌림)해 `param`에 줄로 덧붙인 뒤 조회한다. 섹션이 없으면 공장을 부르지 않고 `{ found: false, serial: 0 }`이며, 있으면(빈 본문 포함) `show(본문, suppress, 0, 0)`의 DWORD를 `{ found: true, serial }`로 돌려준다. `param`은 두 번의 HTML 변환과 메타 처리 동안 같은 등록부에 남아 있어야 하므로 `show`가 끝난 뒤(오류 포함)에 내린다.
- 웹 경계 보호: 이름에 255를 넘는 글자가 있으면 `Uint8Array`가 조용히 잘라 쓰므로 아무것도 쓰기 전에 `RangeError`로 거부한다.
- `embedded-tell.js`의 `decodeKey`를 내보냈고(동작 변경 없음), `ClientHtmlWindowGlobals.tell`을 `createTellSectionState()`로 바꿨다. 앱의 기존 Tell 창은 아직 `GameData.findTellSection`을 쓴다(허브 결선 때 교체).

## 기준선 뒤 판정

`TellSection.cpp/.h`는 기준선 뒤 변경이 없다(base `1f00a25a`). 대응표의 이전 메모(fallback tell 적재의 `config.LoadFile` 경로, 표시 부호화)는 기준선 소스에 이미 들어 있는 내용이며, 표시 4BYTE 부호화(6차 한국어)는 E269 정책대로 **① 웹 제외**(웹은 UTF16/D2Coding 자체 경로)다.

## 검증

- 원본 10.78 관측 **858행** 전체 첫 실행 일치(`tell-section-x86.tsv`, 두 FPU 정밀도): 현재 256BYTE 이름, 전체 256BYTE의 FNV 지문, 본문/없음, 호출 순서(`P`·`N`·`M`·`F`)를 대조했다. 입력은 대상/이전 이름과 mask(이름·미션·fallback 존재, 빈 본문, 이전 이름 훅이 현재 이름을 바꾸는 경우)다. manifest `tell-section`으로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- cpppj `TellSectionTests` 5건을 옮겼다(858행 외 4건): 등록 객체 섹션 우선·미등록 접두어의 미션 폴백·빈 섹션, 매번 재적재와 실패한 이름의 priorTell, 첫 등록 객체와 지역 param 범위, 선택 장치/물리 범위. 실제 공개 자료의 `GameData`를 설정 인터페이스로 썼다.
- 독립 시험 7건: 상태/NUL 뒤 보존/255·256BYTE 경계/BYTE 아닌 글자, 조회 순서와 빈 본문·`undefined`, 이름 해독(1252 전용 BYTE `0x92`·UTF-8 BYTE 열)과 저장 통지 없음, `ShowTellInConfig`의 param 수명(mode `0/1/0xffffffff`·폭높이 0·serial DWORD·없는 섹션·빈 본문·공장 예외·미연결·1252로 못 나타내는 값), 임시 Config 이름.
- 변형 확인: 자동 변형 37개 중 생존 2개(`for` 끝에서 `NaN` 한 칸을 더 쓰거나 읽는 `<`→`<=`, 같은 결과)와 수동 의미 변형 31개(대소문자 비교·NUL 쓰기·빈 본문 판정·해독 경계 여섯 곳·param/임시 Config 반납·공장 인자·serial 감김·`silent` 등)를 모두 확인했다. 수동 변형 첫 라운드의 생존 8개는 해독 경계였고 1252 전용 BYTE와 UTF-8 BYTE 열 시험으로 모두 잡혔다.
- 첫 실행이 일치하지 않은 곳은 시험 도구의 부호화 실수 하나뿐이었다(Config 평문 공급자가 `latin1`로 잘라 `’`가 `0x19`가 됨 → UTF-8로 고침). 모듈은 고치지 않았다.

## 남은 것

앱의 Tell 창은 기존 `GameData.findTellSection`을 쓰고, 실제 Client의 전체 공장 결선(`UberGump`의 `ShowTellInConfig` 호출)은 RawMenuControls 허브 몫이다. 다음은 RawMenuControls 허브(선행 단위가 모두 끝났는지 PORT_MAP/체크 목록으로 확인)와 RawBackgroundGifControls다.
