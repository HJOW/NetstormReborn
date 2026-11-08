# webpj 구름 캐시(RawKencloud) 이식 — WB04-E118

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열여덟 번째 부분이다. 구름 배경 캐시(Kencloud, 512×512바이트)의 파일 부모(00470bb0)·생성(00471040)·행 이동(00470b20)을 cpppj 기준선 `1f00a25a` 의 `o/RawKencloud` 에서 옮겼다. 선행은 `GameRandom` 뿐이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-kencloud.js` — `loadKencloudFile`, `stepKencloud`, `buildKencloud`, `KencloudDomainError`

캐시(0x40000바이트)를 uint32 주소 토큰으로 다루며 메모리 소유권·파일 장치는 훅으로 빌린다. 이 모듈은 난수나 캐시를 새로 소유하지 않고 호출자의 전역(`globals`, 호출 중 계속 읽고 쓰는 살아 있는 객체)과 `GameRandom` 을 직접 읽고 쓴다.

- `loadKencloudFile(scratch, hooks)`(00470bb0): `pathSpec("CloudFileSpec", 0, 0, 0)` 의 NUL 물리를 256바이트 scratch 에 NUL 포함 한 바이트씩 복사한다(읽기/쓰기 물리를 넘으면 오류). `exists` 가 비영일 때만 `decode(path, scratch)` 를 부르고, 치수가 512×512 이면 이미지 주소를 돌려주며 아니면 반환 주소(0 도) 그대로 `release` 한 뒤 0 을 돌려준다. 중단해도 현재 치수와 이름의 부분 쓰기는 남는다.
- `stepKencloud(globals, hooks)`(00470b20): 캐시 첫 바이트를 읽고(물리 read 실패면 phase 는 아직 변하지 않음), end 를 캡처한 뒤 phase 를 `(phase+1)&15` 로 올리고 `end[-1]` 에 첫 바이트를 쓴다. 현재 위상의 행 번호(`phaseRows[phase]`, 512 이상이면 끝)부터 16행 간격으로 행을 한 칸씩 `memmove` 한다(매 행 현재 캐시/end 를 다시 읽고 끝 주소를 `end-1` 로 제한, 겹침은 실제 memmove, dirty 행 표 쓰기). 난수를 쓰지 않는다.
- `buildKencloud(globals, random, hooks)`(00471040): 캐시가 비영이면 표시 요청(전체 1·부분 0)만 하고 끝낸다. 아니면 파일 부모로 읽고, 실패하면 0x40000 을 할당해 `lowColor` 로 채운다. 이어 시드(seedCount > 0 이면 행 1..510·열 0..511 난수 두 번씩, 현재 count 를 저장 뒤 다시 읽음), 간격 난수(256)+5 를 정하고, **파일 대신 생성한 경우에만** `10000/fury`(부호 있는 나눗셈, 0 이면 `KencloudDomainError`)만큼 `speed` 번의 step 을 되풀이하는 warm-up 을 한다. 캐시가 0 이면 재귀로 다시 만들고, speed 는 재귀 복귀 뒤 한 번만 캡처한다.
- 물리 범위 오류는 `RangeError('Kencloud <원인>')`(`step-read`/`step-tail`/`move-physical`/`fill-physical`/`seed-physical`/`dirty-physical`/`file-path-read`/`file-path-write`)이다. 도달하지 않은 훅은 요구하지 않고(캐시가 있으면 하위를 자연 생략), 도달한 훅이 없으면 `TypeError` 로 부분 효과 뒤 거부한다(성공으로 우회하지 않음).

## 2. 검증

- `tests/kencloud.test.mjs`(2건):
  1. `kencloud-v2-x86.tsv` 246행 전부(정상 반환 208 + 명시/물리 중단 38 — 10.78 단일 판본). 진입점(build/step/file)·초기 캐시/이미지 토큰·치수·exists·할당·색·speed·fury·seed 수·phase·난수 상태·end·변화(mutation, 하위 효과 뒤 전역/난수/경로 변경과 명시 중단)·경로 사례로 중단 원인, 현재 12개 필드(파일 반환·난수·색·speed·fury·count·간격·호출 횟수), 인접 전역 8KB·세 512×512 할당 전체·파일 scratch 의 SHA-256, 호출 순서(trace)가 같다.
  2. 계약: 캐시가 있으면 하위 훅을 요구하지 않고 난수를 쓰지 않음, 파일 장치 미연결은 `TypeError` 로 거부되며 난수·전역이 그대로임, `exists` 가 0 이면 scratch 의 부분 쓰기만 남음.
- `kencloud-x86.tsv`(구 관측)는 v2 가 대체한 이전 자료이므로 사용하지 않는다.

최초 실패와 교정: 새 모듈은 246행 fixture(해시 포함)를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 파일 시스템/GIF 디코더(`pathSpec`/`exists`/`decode`/`release`), 메모리 할당(`allocate`/`memory`)은 훅 경계이다. 실제 소유자 전역은 `ClientKencloudGlobals`(Client 전역 단위)가 공급한다.
- 구름을 화면에 그리는 `RawKencloudRender`(`kencloud-render-x86.tsv`)는 별도 단위이다.

## 4. 다음 시작점

`RawKencloudRender`, 번개 수명/프레임/화면(`RawLightningLifetime`·`RawLightningFrame`), `RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`, Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
