# webpj Dude 충돌 반응(RawDudeNotice) 이식 — WB04-E207

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "Dude 생성" 묶음의 두 번째 부분이다. 충돌 주변 Dude 가 반응해 현재 task/Regular 를 바꾸고, Burning 을 붙이고, 같은 소유자의 가장 가까운 거주 구역으로 대피 경로를 요청하는 원본 몸체를 옮겼다(00451470·00450db0·0048fd30·004505a0·이동).

- cpppj 기준선 `1f00a25a`. 소스·시험·fixture 모두 기준선 뒤 변경 없음. 근거 커밋: 소스 `e210c755`, 시험 `fbdcb4df`, fixture `a4b63270`.
- 원본 fixture: `dudenotice-x86.tsv` originals 207행(SHA-256 `5bb484c7…`). originalCD 207행은 범위 밖이다.
- 공유 장치:
  - WB04-B `raw-squid-finder.js`(around 의 실제 사각형 순회)
  - D5c `squid-process.js`(Host Regular 조회/생성·`resetTask`)·`kernel.js`·`squid-destroy.js`·`squid-unpop.js`
  - E2 `squid-postpop-state.js`(거주 구역 목록)
  - E66 `raw-priest-panic.js` 와 같은 `{ speed }` 188칸 현재 속도 표 형식

## 1. 옮긴 것

`www/core/raw-dude-notice.js`(DOM 없음, 10.78 전용)

- `createDudeNoticeState(clock, options)`: Host 시계(현재 double 게임 시각)와 원본 분류 전역이다. Burning 14·경로 24·Bridge 82·높이 보정 genus 마스크 0x50444200(005424b8).
- `RawDudeNotice(pool, hash, types, frames, speeds, residences, state, hooks)`
  - `around(x, y)`(00451470): (x−1, y−1)을 float 0.9999 로 거의 올림한 칸 기준 ±4 사각형을 실제 `RawSquidFinder` 로 훑는다. flags2&0x400(Dude genus) 후보마다 `notice` 한다. 각 notice 가 raw 를 바꾼 뒤에도 finder 커서로 다음 후보에 간다.
  - `notice(sid)`(00450db0)
    1. dead 이거나 작업이 0x5107(반응)/0x5105(낙하)면 반응하지 않는다.
    2. 현재 Regular(마스크 fff0/값 5100)를 찾고, 없으면 만든다(payload 5·flags 0x50). 그 Regular 를 0x5107·현재 시각으로 재설정한다.
    3. task WORD 를 0x5107 로 쓴다. 현재 TYPE 이 Bridge 면 표면 통지를 부른다.
    4. 종속 Burning(14)이 없으면 생성 경계를 부른다.
    5. 같은 소유자의 가장 가까운 거주 구역 진입점으로 빠르게 이동한다. 대피 표적이 해제/dead 면 원본 assert(오류)다.
  - `nearestResidence(sid)`(0048fd30): 현재 소유자가 같은 거주 구역 가운데, float 로 저장한 최선 거리(초기 9999)보다 엄격히 가까운 첫 후보다. 없으면 0 이다.
  - `residencePoint(sid)`(004505a0): x − footX×0.5 를 절삭해 +0.5 한다. y 는 y − footY 에 genus 마스크에 따라 −0.5 또는 +1 을 더한다. 일반 Gun 중심과 다른 표적 발자국 진입점이다.
  - `move(sid, destination, quickly)`
    1. 현재 경로(24)의 끝점이 거의 올림한 목적지와 같으면 바로 성공한다.
    2. 아니면 속도를 정수로 백업하고, 2배로 써서 경로를 요청한 뒤 정수 백업으로 복원한다.
    3. 경로가 성공했고 현재 프레임 코드가 'I' 단계면 현재 TYPE 의 'A' 첫 프레임을 쓴다.
  - 도달한 하위 장치만 요구한다(Regular 조회/생성·Burning 생성·경로 끝점/요청·프레임·표면 통지). 미연결은 그 위치에서 거부한다.
- `makeDudeNoticeHostHooks(pool, host, hooks)`: 기존 Regular 조회/생성만 같은 Host 에 연결한다. Burning/Path 생성 경계는 호출자 몫이다.
- CD 10.72 분기(인라인 notice·BYTE 프레임·소유자 +32)는 옮기지 않았다.

## 2. 검증

`tests/dude-notice.test.mjs`(3건)

- 원본 207행: cpppj DudeScene 과 같은 장면을 행마다 조립한다.
  - 실제 Kernel/SquidProcessHost Regular, E76 `dude-task` 시험과 같은 Form 타입 입력
  - Dude 70 과 거주 구역 89 두 개, 종속 Burning/경로 표식, 버킷 1 등록
  - 기존 Regular 가 있는 행은 실제 7회 반복으로 count 7/time 27 을 준비한다.
  - 행 종류(around/nearest/point)를 실행해 출력 6칸을 대조한다: 반환·호출 순서·슬롯 50바이트·최종 Regular·속도 비트·진입점 8바이트.
- 계약: 미복원 Burning/Path 생성 경계를 묵시 성공으로 바꾸지 않는다(cpppj 손검사). 판본/배열 크기·Host 풀 혼합·SID 범위 거부는 웹 계약 검사다.

**cpppj 시험 장면 결함과 웹 교정:** cpppj `DudeNoticeTests` 의 종속 표식 슬롯(Burning 77·경로 78)은 가상 표가 0 이다.

- 기존 Regular 를 붙이는 장면 준비(103행)에서 Form 부착이 표식의 이전 링크를 쓰다가, 2026-11-05 `d678fcad` 이후의 엄격한 공통 링크 선택(`FormLinkLayoutAtTable`)에서 "공통 contained 링크의 미복원 현재 가상 표" 오류가 난다.
- 기준선 무렵 빌드된 `cpppj/build/bin/Release/netstorm_pop_context_tests.exe --filter DudeNotice` 로 cpppj 도 `DudeNotice_PatchNativeTaskAndRetreat`·`DudeNotice_CdNativeTaskAndRetreat` 가 같은 오류로 실패함을 확인했다. `MissingBurningAndPathFactoriesRejected` 만 통과한다.
- 원본의 실제 Burning(14)/경로(24)는 Process Form 이므로, 웹 시험은 표식에 Process Form 가상 표(0x512098)만 주었다. Host 조회는 TYPE 으로 Regular 만 고르므로 표식을 Regular 로 오인하지 않는다.
- 모듈은 고치지 않았고, 이 교정만으로 원본 207행 전부가 통과했다.
- cpppj 파일은 이 작업에서 고치지 않는다. cpppj 쪽 시험 장면 수정은 CP 세션 몫으로 남긴다(3절).

최초 실패와 교정: 처음 실행에서 위 장면 결함으로 원본 대조 1건이 실패했다. 표식 가상 표를 교정한 뒤 통과했다. 비교가 실제로 작동하는지 보려고 다음 변형을 각각 돌렸다.

- 높이 보정 genus(0x10040000→0x10000000)·진입점 x 계산(+1)·2배 속도 쓰기: 원본 대조가 실패했다.
- 거의 올림 바이어스(0.9999→0.5): 통과했다. fixture 좌표(소수부 .75/.5)에서 결과가 같아 구별되지 않는 차이다.
- 확인 뒤 모두 되돌렸다.

## 3. 경계와 남은 것

- 실제 World 연결은 순서4~5 몫이다(PORT_MAP session 0): 충돌 호출자(폭발/피해의 around 호출)·실제 Burning 14 생성(BurningVisualSystem 부착)·DudePath 24 생성과 현재 경로 끝점 조회·실제 프레임 쓰기·Bridge 표면 통지·거주 구역 장기 목록·현재 속도 표.
- cpppj `DudeNoticeTests` 의 표식 가상 표 결함은 cpppj 시험 수정이 필요하다. 웹은 cpppj 파일을 고치지 않는다.

## 4. 다음 시작점

WB04-E208 은 같은 묶음의 마지막 RawDudeDrop(`DudeDropTests`, `dudedrop-x86.tsv`)이다.
