# webpj 초기 지형 도장 월드 연결(RawTerrainWorldBinding) — WB04-E301 (2026-11-11, 4-B)

3.1절 4-B 의 **RawTerrainWorldBinding**(client, 기준선 35줄 = `.cpp` 23 + `.h` 12; RawNormalTerrainWorldBinding·RawFortWorldLoadBinding 의 선행으로 3.1절에 추가)을 `core/raw-terrain-world-binding.js` 로 옮겼다. 선행: E295(RawTerrainStamp)·E299(RawSurfaceWorld). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `makeTerrainStampWorldHooks(world, stampState, regionDiagnostic, effects)`: 초기 도장(`RawTerrainStamp`)의 훅 묶음을 만든다. `effects` 의 원본 ChunkMap 조회(`chunkRegion`)와 stamp 진단(`assertion`), 그리고 `regionDiagnostic` 가 하나라도 없으면 **월드에 아무것도 만들기 전에** `TypeError` 로 거부한다(묵시 기본값 없음). `effects` 는 복사하며 호출자 객체를 바꾸지 않는다.
- 나머지 네 훅은 월드의 실제 몸체이다: `create` = `world.create`(flags 는 지도의 makePredictable), `setOwner` = 본섬과 파생 받침/종유석의 가상 소유자 분배(`makeOwnerDispatch`)로 월드의 `SquidOwner`, `pop` = `world.pop`(공간 해시·후처리·표시), `setRegion` = 원문 지역 setter(`setSquidRegion`, 현재 `stampState.noRegion` 기준 — 다리에 지역 번호를 쓰려 하면 원본 진단 뒤 반환).
- 기준선 뒤 cpppj 변경은 없다(3.6절 ①). CD(10.72) 판본 장면은 웹 범위 밖이다.

## 2. 검사 (`tests/terrain-world-binding.test.mjs` 3건)

cpppj `TerrainWorldBindingTests.cpp` 의 두 장면(실제 Factory·소유자·Pop·지역 / 누락된 외부 효과 거부)을 10.78 로 옮기고, 네 훅을 직접 부르는 검사를 더했다. 입력은 합성 isle 타입 97(실제 생성자·플래그·발자국 1, P/P 후보 8개)과 합성 SHP 이다.
- 한 칸 도장은 같은 SID 로 `find`·해시 0단계·섬 번호 WORD(지역 5)·타입·소유자 0·전역 집계 1·후처리 깊이 0·객체 목록 1건·99 캐시 생성을 모두 이룬다.
- 직접 호출: 생성 flags 2 는 지역 번호(15000 미만), 소유자 3 은 raw 소유자 바이트, Pop flags 는 월드 송신기까지, 지역 setter 는 섬 genus 에 저장·다리에는 현재 noRegion 이 아닐 때만 진단.
- 변형 확인 13개 중 **2개만 남았고 동등**하다: 모듈의 `>>> 0` 변환(`setSquidRegion` 이 같은 변환을 다시 함)과 소유자 분배 래퍼(`makeOwnerDispatch` 의 파생 타입 분기는 raw-bridge-connect 단위 시험이 소유; 도장은 owner 0 만 쓴다).

## 3. 남은 경계

- 3.1절 4-B 는 32/35 이다(이 항목과 RawNormalTerrainWorldBinding 을 선행으로 추가하며 분모가 35 가 됨). 다음: RawNormalTerrainWorldBinding(client 318줄, 기준선 뒤 `f7f58f5a` 판정 필요), client RawFortWorldLoadBinding, 4-B 적재 통합 시험.
