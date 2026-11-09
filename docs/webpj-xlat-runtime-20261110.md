# webpj 10.78 번역 장부(RawXlat) 이식 — WB04-E242

2026-11-10, HJOW-Athlon. 10.78 Xlat(번역표)의 원문 바이트 파서와 번역 장부를 옮겼다. 일곱 개의 파일 버퍼(textBuffer[0..6])에 읽은 파일을 설치하고, 004de710 의 파일 읽기 뒤 몸체처럼 마지막 BYTE 를 NUL 로 쓰고 CR 을 한 칸씩 당긴 뒤, `*` 키 줄·`-` 값 줄·`=` 끝 줄 레코드를 키 앞 BYTE 와 구분자 줄 첫 BYTE 를 NUL 로 쓰며 병합한다. 조회(004de9a0)는 영어 우회(언어 0·1)·정확 키·NULL 칸 폴백을 따르고, 시작 적재(004dea00)는 German(3)→English(1)→Portuguese(6)로 d/xlat.* 세 파일을 읽는다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "자료/설정 adapt 경계" 묶음 옆에서 `o/` 단위로 남아 있었고 기준선 뒤 변경이 없으며 원본 관측이 있어 E241 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawXlat.cpp/.h`)·시험(`XlatRuntimeTests`)·fixture(`xlat-runtime-x86.tsv`·`xlat-assets-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스 `9960f8b7`(2026-11-03), fixture `2c490626`(2026-11-02).
- 원본 fixture(10.78 전용): `xlat-runtime` 2,031행(사례 43개의 언어별 적재 45회: 원문 파일 바이트 → 정규화·NUL 쓰기를 마친 전체 버퍼와 고유 키 수, 조회 1,935회: 언어·키 바이트 → 결과 바이트, 시작 적재 8가지: 존재 마스크 → 존재 확인/열기/닫기/9번 진단 순서), `xlat-assets` 44행(실제 보호 TARC 의 xlat.german/english/portuguese 97,526바이트가 fixture 안에 hex 로 들어 있어 게임 파일 없이 전체 버퍼·고유 키 수와 조회 40회를 대조).
- 웹의 기존 `www/assets/xlat.js`(PORT_MAP `Xlat.cpp`, partial)는 생성된 용어표를 UTF-8 문자열로 조회하는 다른 계층이며 이 모듈과 무관하게 남긴다. 이 모듈은 인코딩을 바꾸지 않는 원문 바이트 계층이다.

## 1. 옮긴 것

`www/core/raw-xlat.js`(DOM 없음, 10.78 전용)

- 키와 번역은 원문 바이트 문자열(문자 코드 0~255 하나 = 원문 1바이트)이다. `xlatBinaryString(bytes)` 가 바이트열을 이 표현으로 바꾼다(브라우저에 `Buffer` 가 없고 `TextDecoder('latin1')` 은 0x80~0x9f 를 바꾸므로 직접 구현, 4096 바이트 묶음으로 변환).
- `RawXlat`: `load(which, bytes)`(재적재는 `Error`, 길이 0·칸 범위 밖은 `RangeError`, 설치 뒤 정규화·병합 실패는 설치를 되돌리지 않음), `hasBuffer`(길이 0 설치와 미설치를 구별), `allocateFileBuffer`(영으로 채운 정확 길이, 중복 거부), `normalizeFileBuffer`(마지막 BYTE NUL·NUL 전까지의 CR 이동·이동 뒤 같은 위치 재검사), `parseFileBuffer`(레코드 병합: 키/값/끝 줄의 상태 순서, 불완전한 마지막 레코드는 저장하지 않음, 여러 줄 키·번역 허용, 다른 언어 칸 보존), `translate`(언어 0·1 원문 / 첫 NUL 앞 정확 키 / 없는 키 입력 전체 / NULL 칸 원문 / 빈 번역은 빈 문자열 / 키가 있는데 언어 7 이상·음수는 `RangeError`), `buffer`(사본 없이 빌림, 미설치는 빈 배열), `size`.
- `initializeRawXlat(xlat, hooks)`: 고정 순서 German(3)→English(1)→Portuguese(6). 존재 장치·부재 로그·읽기 장치는 그 지점에 도달할 때만 요구하고, 부재 파일은 9번 진단 뒤 다음으로 가며, 읽기 오류는 앞서 끝난 언어의 표와 버퍼를 남기고 전파한다.

## 2. 검증

`tests/xlat-runtime.test.mjs`(8건)

- 원본 관측 재생 2건: `xlat-runtime` 전체(적재 45회의 버퍼·키 수, 조회 1,935회, 시작 8가지), `xlat-assets` 전체(97,526바이트, 조회 40회). 모듈은 처음부터 모두 통과했다.
- cpppj 계약 포팅 1건: 시작 읽기 실패 뒤 German 표·버퍼 보존. 생략: cpppj 의 네 번째 시험(Client 의 언어 변경)은 client 이식 몫이고, `xlat-file-read` fixture 는 `XlatFileRead`(adapt) 단위 몫이다.
- 웹 계약 5건: 개별 단계(설치·정규화·병합)와 거부(중복 설치·길이 0·칸 범위), 언어별 병합과 폴백 순서, 시작 장치 요구, 내장 NUL·입력 배열 복사·종료 NUL 없는 버퍼, 구분자 상태 순서와 여러 줄 키·언어 0 적재·키 형식.

시험 쪽 결함 하나를 고쳤다(정규화 기대 문자열: NUL 앞의 CR 이 모두 당겨진 뒤 뒤쪽 다섯 칸이 NUL 로 남는다).

모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 변형 61건 중 `next plus`(다음 줄 위치 오류)·`norm shift end`(당기는 범위 오류)는 무한 반복이어서 제한 시간 뒤 종료(탐지로 판정)했고, 12건이 처음에 살아남아 9건은 웹 계약 둘(내장 NUL·입력 복사·종료 NUL 없음, 구분자 상태·언어 0·키 형식)을 더해 잡았다. 3건은 동등 변형이다.

- 동등 변형: ① 문자열 시작 위치가 버퍼 끝 이상인지 검사 제거 — 병합은 항상 NUL 이 보장된 위치만 시작으로 삼고 끝 검사(`NUL 범위`)가 이 경우도 먼저 던진다. ② 줄 다음 위치가 없을 때 `null` 대신 NUL 위치 반환 — 그 위치의 값은 0 이라 루프가 같은 곳에서 끝나고 완성된 레코드가 아니라서 키/값 위치를 읽지 않는다. ③ 병합 바깥 루프의 `cursor !== null` 제거 — `bytes[null]` 은 `undefined` 라 거짓으로 같은 종료를 낸다.

## 3. 경계와 남은 것

- 실제 파일 읽기(`XlatFileRead`: TARC/TAFF 에서 d/xlat.* 읽기, 10.78 `xlat-file-read` fixture 253행)와 클라이언트 `Translate`/언어 번호(`Client`) 연결은 하지 않았다. 호출자가 `read`/`exists`/`missing` 장치와 `Uint8Array` 를 공급한다. UTF-8 변환(1252 → 화면 글꼴)도 호출자 몫이다(PORT_MAP session 0, 순서5·8).
- 프랑스어(2)·스페인어(4,5) 등 다른 언어 파일은 원문 시작이 읽지 않으므로 임의로 추가하지 않았다.

## 4. 다음 시작점

WB04-E243 은 3.1절에서 고른다. 기준선 뒤 변경이 없고 직접 관측이 있는 단위를 `PORT_MAP.tsv` 에서 다시 찾는다(`RawColorMapCache`(adapt)·`RawTerrainGrowth` 는 선행 의존이 있어 미룬다).
