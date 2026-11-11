# webpj 장기 월드 저장소(RawSurfaceStorage) — WB04-E298 (2026-11-11, 4-B)

3.1절 4-B 의 **RawSurfaceStorage**(client, 기준선 98줄; RawSurfaceWorld 의 선행으로 3.1절에 추가한 항목)를 `core/raw-surface-storage.js` 로 옮겼다. Client 의 실제 배열을 Session 보다 오래 보관하고 같은 물리를 두 월드에 중복 설치하지 않는 장기 저장소이다. 선행: E297(RawProcessHostStorage). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawSurfaceStorage(types, frames, shapes, { source, authority, gameAux })`: 같은 길이의 입력 표만 받아 복사한다(`types` 는 항목까지 복사, `frames`/`shapes` 는 배열 복사, `codes[type]` 는 프레임 코드 배열 복사). `authority`/`gameAux` 는 Client 와 공유하는 장기 owner(없으면 새로 만듦)이고, SID 풀(32,768)의 서버 권한은 `authority.server` 의 현재 값을 읽는다.
- 대여와 동일하게 유지되는 물리(같은 객체): `pool`·`hash`·`spots`(65,536)·`types`·`frames`·`shapes`·`codes`·`graph`(기록 255행+스택 4096)·`sidLists`·`totalMade`(3×256)·`islands`(3,204BYTE)·`chunks`·`priestOwnerState`(9 DWORD)·`ownerMode`·`links`·`bridgeDecayScan`·`bridgeDecayPeriod`·`wind`·`boardTargetState`·`disallowSquidAllocation`. 생성 때 풀 reset 꼬리(`finishSquidPoolReset`)를 이어 붙이고 표면 종류를 82/94/95/155/157 로, 붕괴 스캔을 first/last -1·cursor/기한 0 으로, 주기를 10 으로 둔다(PATCH 의 초기 PE 값).
- `acquire()`(일반 설치): 중복 대여를 **먼저** 거부한 뒤 fresh 입력만 호환한다 — GameAux 일반 월드 재설정, 실패점 -10000, 서버/보스 true, 두 번째 세대부터 풀·해시·spot 재초기화(첫 세대는 쓰지 않음), 사제 owner 0, Graph 전체 0 후 작은 reset, State SID 목록, 집계 3표 전체 0, 섬 0 후 지정 필드 reset, 청크 지도 기본(16×16·mode 1·항목 비움). 같은 객체를 제자리에서 초기화하므로 대여 사이에 참조가 유지된다. TYPE 전역과 삭제 기록은 되돌리지 않는다.
- `acquirePrepared()`: 앞선 명시 정리가 끝난 물리를 초기화 없이 대여하고 같은 관문에서 중복을 거부한다(바이트·모드·권한·GameAux 보존). 둘 다 세대 번호를 올린다. 반환 `SurfaceUse.release()` 는 대여만 풀고(C++ `~Use`) 낡은 대여의 release 는 새 대여에 영향이 없다(멱등).
- 장기 장치(Client 수명): `bindChecking`/`checking()`(기본 0), `bindDisplayDiagnostics`(상세 owner `{ value }` + 진단 함수)·`recordAssertionDetail`·`recordDisplayFrameError`·`assertDisplay`(미연결은 선택 시에만 명시 실패), `bindAllocationDiagnostics`(CD 소진 전용 덤프/Tell 보관), `bindGraphClock`/`readGraphClock`(기본 0)·`bindGraphTrace`/`traceGraph`(기본 무출력), `angleDiagnostics()`, `updateAmbientWind(random)`(Client 난수 직접 소비), `processes`(지연 생성 `RawProcessHostStorage`), `geyserType`(GameAux 별칭), `dispose()`(process owner 의 잔류 반납).
- 아직 옮기지 않은 것: 길 찾기 작업 배열(`pathSearchWork`, 6-P RawPathSearch), SID 소진 복구 할당 연결(C++ `pool.BindAllocation`; 웹 풀은 소진을 거부만 하므로 소진 진단/회수/덤프/Tell 연결은 4-D), 실제 자산에서 만드는 `MakeRawSurfaceStorage`(Client 자산 단위). CD 시작 값/분기는 옮기지 않았다.
- 기준선 뒤 cpppj(`.cpp` +413줄, 판정 커밋 `14fd017c`)는 자산 Regular/PostPop/Pop/Unpop/전송/표시의 **장기 소유자 연결**(월드 없는 자산 후처리·Native 연결)이라 **3.6절 ③ 으로 4-D 에서 반영**한다(대응표 base 를 올림).

## 2. 검사 (`tests/raw-surface-storage.test.mjs` 11건)

원본 관측 파일이 없는 단위 계약 검사이다. cpppj `RawSurfaceStorageTests` 의 장기 물리 계약을 RawSurfaceWorld 없이 저장소 수준으로 옮겼다(월드 모듈이 빌리는 동일 객체는 RawSurfaceWorld 단위에서 다시 확인).
- 구성·초기 시작 값(입력 복사, 풀 서버 alias, 종류/스캔/주기/바람/실패점), 일반 대여의 중복 거부와 두 번째 세대부터의 재초기화·TYPE 보존, 준비 대여의 물리/모드/권한 보존과 이어지는 일반 대여의 재초기화, Graph 기록/스택의 동일 객체와 fresh 비교(독립 `RawGraphStorage`+작은 reset), 집계 3표·사제 owner·목록·섬·청크의 준비 보존/일반 초기화와 동일 객체 유지, 권한 owner·실패점·GameAux 의 일반 대여 전용 쓰기, 장기 진단 장치와 주변 바람(독립 오라클 `stepAmbientWind` 와 같은 상태·난수), 지연 생성 process owner, 첫 세대 보존·낡은 대여·섬 padding·청크 크기·최종 소멸의 process 잔류 반납.
- 변형 확인 81개 중 처음에 13개가 살아남았고 보강 뒤 **4개만 남았는데 모두 동등 변형**이다: 입력 표의 `Array.isArray` 검사(배열이 아니면 어차피 TypeError), 풀 서버 owner 의 setter(풀은 읽기만 함), 풀 reset 꼬리(`finishSquidPoolReset` 의 효과인 이력 비움/실패점 -10000 은 새 풀/새 상태가 이미 같은 값), 주변 바람의 진단 인자(`atan2` 각도 정규화가 361 이상을 만들 수 없어 진단이 선택되지 않음).

## 3. 남은 경계

- 월드 모듈이 이 저장소의 같은 객체(풀·해시·Graph·청크 등)를 빌리는 일은 다음 항목 RawSurfaceWorld 에서 다시 확인한다. 3.1절 4-B 는 29/33 이다.
