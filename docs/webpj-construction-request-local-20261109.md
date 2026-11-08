# webpj 건설 요청(RawConstructionRequest)·로컬 건설 배치(RawConstructionLocal) 이식 — WB04-E140

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔 번째 부분이다. 서버 건설 요청(Construction.cpp `00444760`)과 로컬 건설 배치(`004433b0`)를 옮긴 `o/RawConstructionRequest`·`o/RawConstructionLocal`(cpppj 기준선 `1f00a25a`, 기준선 뒤 두 파일 모두 변경 없음)이다. 클라이언트가 커서로 내려놓으면 로컬 배치가 미리보기를 없애고 abstract 예측 조각을 놓은 뒤 서버에 요청 메시지를 보내고, 서버 쪽 요청은 배치 가능 판정·충돌·밀어내기·받침 선행 확정·본 확정을 거쳐 E139 확정으로 이어진다. 선행은 E136 Canon 디코더, E137 `cellArea`, E135 효과, E139 확정이다.

## 1. 옮긴 것

`www/core/raw-construction-request.js`

- `ConstructionRequestState` — `editor`(00594fb8)·`localPlayer`(00540c70)·`displaceGenus`(005325b4)·`patternTypes`·`decoderDiagnostics`.
- `RawConstructionRequest(pool, hash, types, frames, spots, state, hooks)` — 10.78 `SidPool`·타입 표 188칸·256×256 이상 spot 지도·필수 경계(`mayPlace`·`displace`·`confirm`·`cancel`, 관찰용 `scan` 선택)를 생성 때 확인한다.
  - `request(req)` (00444760): 타입(255 이하·표 안)·섬/받침 패턴 타입·좌표 유한성을 효과 전에 확인한다. 편집 모드이면 요청 플레이어를 로컬 번호로 바꾸고 → `mayPlace({type, argument, x, y, flags: direction, owner: player, mode: 0})` 거짓이면 거짓 → 디코더의 모든 유효 칸에서 `cellArea` 범위의 abstract 예측 중 받침이 아닌 것이 같은 플레이어·타입·정확한 좌표(float 같음)가 아니면 첫 충돌에서 효과 없이 거짓 → 타입 genus 가 `displaceGenus` 면 `displace` 거짓이면 거짓 → 받침 필요 타입(flags1 0x400)이면 표면 조회(`+0.9998999834f` 절삭, 범위 밖은 슬롯 0)를 읽고 발자국 사각형 spot 에 bit 2 가 없거나 표면이 abstract 이면 받침을 `(x+1−섬 폭, y+1−섬 높이)` 로 먼저 확정(반환 무시) → 본 `confirm` 참이면 참, 거짓이면 현재 flags1 을 다시 읽어 받침 필요 타입이면 `cancel(x, y, 0x1000, player)` 뒤 거짓.
- `makeConstructionRequestHooks(pool, placement, confirm, hooks)` — `mayPlace` 를 `placement.mayPlace`, `confirm` 을 `confirm.confirm` 에 잇는다(풀 불일치 거부; 밀어내기·실패 정리·진단은 받은 경계 유지, E135 `makeConstructionEffectsRequestHooks` 로 채움).

`www/core/raw-construction-local.js`

- `ConstructionLocalState` — `localPlayer`·`patternTypes`·`decoderDiagnostics`.
- `RawConstructionLocal(pool, types, frames, spots, state, hooks)` — 필수 경계 7개(`previewSource`·`destroyPreview`·`returnPiece`·`create`·`place`·`moveBuilder`·`send`)와 지도 크기를 생성 때 확인한다.
  - `place(request)` (004433b0): 요청을 복사해(원본 스택 인자) 미리보기 번호(5 이상·풀 안)·좌표·타입 번호·받침 타입의 spot 범위를 효과 전에 확인하고, 미리보기 raw 의 타입 WORD(+18)를 먼저 읽은 뒤 `previewSource` → (그 뒤의 현재 raw 에서) 반환 플래그 WORD(+20)·품질 BYTE(+30) 읽기 → `destroyPreview(미리보기, 0)` → 플래그 비트3 이면 `returnPiece` → 본 조각 SID 확보(디코더 유효 칸마다 `create(type, 2)`, 18개 초과·번호 오류 거부) → 받침이 필요한 타입이면 시작 칸 하나의 spot bit 2 가 없을 때 받침을 확보해 `place`(abstract 1, 품질 0) → 본 `place`(abstract 1, 품질) → 배치 경계 뒤의 현재 flags1 에 사제 필요(0x8000)가 있으면 `moveBuilder(로컬, 첫 SID, 0, 0)` → `send(9, 서버 요청 메시지)`.
- `makeConstructionLocalHooks(pool, factory, place, hooks)` — `create`/`place` 를 같은 풀의 실제 factory·건설 배치에 잇는다.

## 2. 검증

`tests/construction-request.test.mjs`(4건):

1. `constructionrequest-x86.tsv` 의 `originals` 816행(`originalCD`·`original1037` 은 제외) — 요청·전역·발자국·프레임 표·spot 윤곽·표면 객체·후보 목록에서 반환값과 효과 순서(M 배치 가능 판정 인자/A 칸 탐색 범위/D 밀어내기/C 확정[받침 선행 포함]/K 실패 정리)가 모두 같다.
2. 필수 경계 4개 각각·짧은 지도·다른 판본 거부, 표 밖 타입·NaN 좌표는 첫 효과 전에 거부.
3. 확정 경계가 현재 flags1 의 받침 비트를 뒤집으면 실패 정리 여부를 다시 판정(첫 호출은 정리, 둘째는 정리 없음).
4. 어댑터: 같은 풀의 배치 가능 판정과 확정만 잇고 풀이 다르면 거부하며 밀어내기/정리 경계를 보존한다.

`tests/construction-local.test.mjs`(4건):

1. `constructionlocal-x86.tsv` 의 `originals` 558행 — 미리보기 Form·요청·전역·flags1·spot 윤곽·섬 발자국·프레임 표·효과 경계의 변동(Form 값 변경·타입 플래그 토글)에서 사건 순서(Q/D/R/C/P/B/S[서버 요청 21바이트])와 만든 SID 수가 모두 같다.
2. 필수 경계 7개 각각·짧은 지도·다른 판본, 미리보기 번호 0·NaN 좌표·타입 300·받침 타입의 지도 밖 spot 은 효과 전에, 생성 번호 0 은 배치/전송 전에 거부.
3. 미리보기 삭제 전의 타입 캡처, 연관 객체 호출 뒤의 반환 플래그·품질 재독(`R:8:0:3`), 배치 경계의 현재 사제 플래그 재독, 요청 사본 유지(인자 `0x12345678` 이 배치·전송에 그대로).
4. 어댑터: 같은 풀의 factory 생성과 건설 배치만 잇는다.

최초 실패와 교정: 두 모듈은 816행·558행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 서버 요청 메시지의 wire 직렬화와 수신(`RawConstructionCommand`: 25바이트 작성·`444af0` 수신·패킷 분배 등록)은 `RawPacketDispatch` 이식 뒤 옮긴다. 로컬의 `send` 경계는 논리 본문 `{type, x, y, argument, direction, quality, 0, 0}` 만 넘긴다.
- 전체 배치 가능 판정 파이프라인(`RawCanonPlacementPipeline`)은 별도 단위이며 요청 어댑터는 `placement.mayPlace(query)` 이름 계약만 시험했다. 요청→밀어내기→확정→통지→정리/배치를 실제 파이프라인으로 잇는 cpppj 통합 시험은 그 뒤에 옮긴다.
- 10.72(CD) 경로(`displaceCd`·소유자 +32·품질 BYTE +26)는 범위 밖이다.

## 4. 다음 시작점

`RawConstructionData`·`Builder`·`Command`(선행 `RawPriestPlacementGeometry`·`RawPacketDispatch`), `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
