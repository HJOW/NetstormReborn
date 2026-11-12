# webpj 미션 기술 허용 목록 — MissionTechnology (WB04-E338, 2026-11-12, 4-D B 구획)

3.1절 4-D B 구획 첫 항목의 세 번째 파일인 client **MissionTechnology**(기준선 `.h` 20 + `.cpp` 50 = 70줄, 기준선 뒤 변경 없음)를 `core/mission-technology.js` 로 옮겼다. 원본 `00482eb0` 이 Mission 의 `techAllowed` 목록(`;` 구분)을 현재 건설/생산이 함께 쓰는 허용 표(`RawTotalMade`)에 적용하는 몸체이다.

## 1. 규칙

- **applyMissionTechnology(hooks):** `allowed = 1` 에서 시작해 인덱스 0 부터 `hooks.item(index)` 로 **매 항목마다 현재 목록을 다시 조회**한다(이전 항목의 효과가 목록을 바꿔도 다음 조회에 반영). 항목의 첫 NUL 앞이 비면 종료하고 그 뒤는 읽지 않는다. 항목을 ASCII 소문자로 낮춘 값이 `deny` 면 `allowed = 0`, `allow` 면 `allowed = 1`(둘 다 표를 쓰지 않음), `all` 이면 `hooks.allowAll(allowed)`(첫 255칸에 금지값), 그 밖이면 `hooks.allow(hooks.findType(원문 항목), allowed)` 로 타입 한 칸을 쓴다. **공백은 새로 다듬지 않고** 이름은 대소문자도 그대로 찾는다. 미등록 이름의 `-1` 도 그대로 쓰기 몸체에 넘겨 실제 저장소의 범위 거부(`RangeError`)를 성공으로 바꾸지 않는다. 필요한 효과는 도달한 위치에서만 요구한다.
- **applyMissionTechnologyToTotalMade(script, types, totalMade):** C++ 오버로드 `(MissionScript&, RiftTypeTable&, RawTotalMade&)` 에 대응한다. `script.get('techAllowed')`(없으면 빈 목록)을 원본 좁은 문자(Windows-1252 바이트)로 바꿔 읽고, 첫 NUL 앞이 512BYTE 이상이면 원본 임시 버퍼 범위 오류(`RangeError`)로 거부한다. `types.find(이름)` 이 번호(-1 미등록)를 주고 쓰기는 월드의 `RawTotalMade.allowAll/allow` 가 한다.
- **applyMissionTechnologyToRestrictions(script, types, restrictions):** 월드 설치 전 Init Header 가 쓰는 전역 금지 표(`Uint32Array`)에 `setTotalMadeAllowAll/Allow` 로 같은 쓰기를 한다. 이후 Session 이 같은 배열을 빌린다(별도 사본 없음).
- JS 에는 오버로드가 없어 세 C++ 함수를 이름으로 나눴다. 스크립트는 `get(key)` 만 가진 대역으로 받는다(실제 `MissionScript` 는 B 구획 `Mission` 단위).

## 2. 웹 범위

- 기준선 뒤 cpppj 변경이 없다(`.cpp`·`.h` 모두). PORT_MAP: `MissionTechnology.cpp` port(base `1f00a25a`). 앱 수명 연결(Init Header 가 호출)은 B 구획 `Mission`·4-D 조립 때 한다(PORT_MAP session 0).
- cpppj 시험 `MissionTechnology_ProtectedElevenAndInvalidStorage`(보호 미션 11개의 `techAllowed` 를 실제 `MissionScript` 와 보호 타입 표에 적용해 `windVortex`/`rainCannon`/`dais`/`sunArcher` 허용 여부를 확인)는 실제 MissionScript 가 필요해 `Mission` 단위에서 옮기기로 했고, **E342 에서 이식을 마쳤다**(첫 실행 일치).

## 3. 검사 (`tests/mission-technology.test.mjs` 10건)

- **원본 독립 관측:** `mission-technology-x86.tsv` 55행(목록 문자열·호출 도중 바뀌는 현재 목록·초기 표 시드): 257DWORD 표(앞 한 칸 + 물리 256칸, 256번째와 앞 칸 보존 포함)와 효과 호출 순서, 변하지 않아야 하는 집계 장부를 전부 대조한다. **첫 실제 실행에서 55행이 모두 일치했다.** manifest 에 `mission-technology` 항목을 추가했다.
- **cpppj 시험 이식:** `MissionTechnology_Native55`. 시험의 `-1` 쓰기 격리(표 앞 한 칸)도 그대로 옮겼다.
- **웹 계약:** 키워드 대소문자·혼합 대소문자 이름 그대로 전달·공백 비정리, 첫 빈 항목 종료와 NUL 뒤 비조회, 매 항목 재조회와 인덱스 증가, 도달한 효과만 요구, 미등록 `-1` 전달, 실제 `RawTotalMade`/전역 금지 표 래퍼(deny 뒤 개별 금지·장부 무변경·속성 없음·`null` 은 빈 목록), 512BYTE 경계(511 허용·512 거부 시 표 무변경), Windows-1252 바이트 보존(`é`→0xe9, `€`→0x80), 표현 불가 문자 거부, 미등록 이름 거부 시 표 무변경.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 24개를 돌려 첫 실행에서 4개가 남았다: `techAllowed` 키 변형은 변형 도구가 헤더 주석의 첫 일치에 적용해 무효였고(코드 한 줄로 다시 지정해 잡힘), 나머지 둘은 `deny` 뒤 개별 금지 시험이 없어 `allow(type, 1)` 변형이 살아남았다(시험 보강 후 잡힘). **남은 것은 동등 1개**: `need()` 의 `typeof effect !== 'function'` 방어 — 제거해도 `undefined(...)` 호출이 같은 종류(`TypeError`)로 실패한다.
- 남은 경계: 앱 수명 연결(Init Header 호출). 보호 미션 11개 시험은 E342 에서 실제 MissionScript 로 이식했다.
