# webpj 경로 패킷 RawPathData (WB04-E350, 2026-11-13, 6-P 경로 부품)

3.1절 6-P 둘째 항목 "RawPathData 136줄(+151)"(기준선 `.h` 56 + `.cpp` 80 = 136줄, 기준선 뒤 `20261106-63`·`20261107-46`·`20261108-205 CP` 세 커밋이 송신 DWORD 길이 헤더·Path 전체 수신 복원·Native 송신 본문을 추가)를 `core/raw-path-data.js` 로 옮겼다. 서버가 로컬 연결로 이동 경로를 보낼 때 쓰는 패킷(번호 0x30)의 **송신 작성기 둘(typed `write`·Native `writeCurrent`)·수신 복원기(`read`)·길이 observer(`observePathData`)·Form 직렬화 분배기(`makePathFormTransmission`)** 이다. 호스트의 flags 0 서버 부착(`attachProcessForm`)이 이 분배기를 직렬화 경계로 쓴다.

## 1. 규칙

- **패킷 모양:** 24바이트 머리(+0 플래그 하위 7비트·+1 번호·+2 WORD 길이·+4 Form·+6 부모·+11 helper WORD·+13 nodeCount WORD·+15/+16 시작 좌표 BYTE·+17 시작 시각 DWORD·+21 명령 꼬리 WORD·+23 꼬리 단계 BYTE) 뒤에 방향/거리 run 또는 직접 좌표 BYTE 가 온다. 쓰지 않는 머리 3바이트와 반환 길이 밖의 기존 공유 버퍼는 지우지 않는다. 길이는 공통 헤더 DWORD(`createPathDataHeader().packetLength`)가 4 이상일 때만 낮은 WORD 로 쓴다(`max(길이, 4)`).
- **`write(source, form, parent, parentGenus, header, buffer)` (typed 004895b0):** `source` 는 100,080바이트 경로 이미지 또는 `RawPathRuntime`, `buffer` 는 24바이트 이상. 꼭짓점 개수가 0 이거나 5000 을 넘으면 거부, 시작 좌표·첫 도착 시각(×8 절삭)·helper·명령 꼬리를 쓰고 `nodeCount` 를 0 으로 만든 뒤, 부모 genus 0x120000 이면 이후 꼭짓점의 절삭 좌표 BYTE 두 개씩을, 아니면 좌표가 바뀐 꼭짓점마다 방향의 하위 3비트와 **31칸 단위로 쪼갠 거리**(홀수 방향은 한 축, 짝수 방향은 두 축의 합)를 `(거리<<3)|방향` BYTE 로 쓴다. 매 BYTE 뒤 `nodeCount` WORD 를 갱신하고 좌표는 signed32 `_ftol` 범위 밖을 거부한다. 반환은 `written + 24`, 이미지는 바꾸지 않는다.
- **`writeCurrent(runtime, form, parent, header, buffer, hooks)` (Native 4895b0):** typed 의 스냅샷/자격 guard 없이 원문 순서대로 쓴다. 매 꼭짓점 읽기마다 **현재 nVert 를 다시 읽어** `i < nVert` 진단(`hooks.assertion`, 줄 0x1f/0x24/0x29, 정상 복귀하면 같은 물리 바이트를 계속 읽는다), 좌표·시각은 signed64 절삭의 **낮은 DWORD**(비유한/범위 밖은 0), 점/run 로그(`[i] (x,y) , 방향\n`·`n[현재 WORD] 방향 | 거리\n`)를 `hooks.trace` 로 내고, 부모 genus 는 고정 머리 뒤 **현재 패킷 +6 의 SID** 로 `hooks.parentGenus` 를 한 번 묻는다. 거리 계산은 DWORD 감김을 보존하고 payload 주소·반환은 현재 signed WORD 이다. 송신 버퍼 범위를 넘는 쓰기는 거부하며 앞선 prefix 는 되돌리지 않는다. 정상 범위 입력에서는 typed 와 같은 바이트다.
- **`read(runtime, packet, pool, types, hooks)` (패치 00489a10):** 완료 표시·두 목표를 0 으로, helper·명령 꼬리·꼬리 단계를 패킷 값으로 쓴 뒤 부모 genus 가 0x120000 이 아니면 방향 run 을 순서대로 읽는다 — 시작점을 첫 방향으로 추가하고, 방향이 바뀔 때마다 **모서리를 이전 방향과 새 방향으로 두 번** 추가하며, 끝에 마지막 점을 더한다(좌표는 8방향 단위벡터 × 거리, DWORD 감김 뒤 signed int → float). 직접 좌표면 시작점 뒤 좌표쌍을 방향 0 으로 추가하고 홀수 nodeCount 는 `(zwp->nodeCount & 1) == 0`(Pathprocess.cpp 0x1ca) 진단 뒤 계속한다. 꼬리 WORD 가 가리키는 살아 있는(free 아닌) 대상이 있으면 스테이지 1..4 는 시작점을 대상 코너로 바꾸고(부모와의 거리 **1.5 미만**일 때만), 스테이지 0..4 는 마지막 점을 현재 스테이지 코너로 바꾼다. 마지막으로 `metadata`(속도/회전 속도)·`now`·시작 시각(DWORD×0.125)으로 `prepare` 하고 `clearAnimation`(첫 0x5105 Regular/flags 40 삭제)한다. 점 append 는 배열 용량 5000 에 닿으면 `nVert < MAX_PATH_LENGTH`(c:\ns\o\path.h 0x16) 진단을 낸다.
- **`observePathData(packet, sending, assertion)` (40a660):** 수신은 무동작, 송신은 nodeCount 가 -1 이면 `nodeCount != (short)-1`(_p_packets.cpp 0x716) 진단 뒤(정상 복귀하면 계속) 길이 WORD 를 `nodeCount + 24` 의 하위 WORD 로 확정한다.
- **`makePathFormTransmission(pool, types, header, buffer, transport, other, nativeWriter)`:** 같은 풀의 `RawPathProcess` 는 위 writer(`nativeWriter` 가 있으면 Native, 아니면 typed)로, 다른 프로세스는 `other` 로 위임한다(없으면 거부). 반환 `{ writeData, packet, writeFullData, transmit }` 은 마지막 writer 선택을 공유해, 다른 프로세스를 골랐다가 Path 를 고르면(또는 반대로) 버퍼 조회와 송신이 같은 쪽으로 간다. `writeFullData(process, force)` 의 Path 는 force 로 필드를 고르지 않고 현재 전체 경로를 쓴다(Native 는 force 를 그대로 넘긴다).

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD) 판본(WORD 길이 헤더·배열 600·tail 위치 `0x2f1c`)은 옮기지 않았다(웹은 10.78 만, 풀 판본을 확인해 거부한다). `RawPathDataHeader.packetLength` 는 기준선 뒤 `20261108-205 CP` 에서 WORD 가 DWORD 로 바뀐 판을 따른다.
- 기준선 뒤 변경(3.6절 ③ Native/실제 연결 몫) 세 커밋을 모두 이 단위에서 반영했다: 수신 복원 `read`·observer(`20261107-46`), 참가자별 월드 동기화용 `packet`/`writeFullData` 분배(`20261106-63`), Native 본문 `writeCurrent`·`nativeWriter`(`20261108-205`). PORT_MAP base `2b5ee903`.
- cpppj 의 `friend RawPathData`(경로 이미지 raw 쓰기/읽기)는 웹에서 `runtime.bytes`(쓰기 가능한 `Uint8Array`)와 `DataView` 로 대체했다(3.6절 ③ "RawPathRuntime friend RawPathData 웹 대응"). 위치는 `{ x, y }`(float32), SID 는 숫자, 공유 버퍼는 `Uint8Array`, 헤더는 `{ packetId, packetFlags, packetLength }` 이다.
- 이 모듈은 DOM 을 쓰지 않는다. 앱 수명 연결(미션 월드의 호스트 `setFormTransmit` 에 이 분배기를 끼우고 Session 이 받은 패킷을 `read` 로 넘기는 일)은 4-D/6 단계 조립 몫이다. `read` 의 `stagePoint`(447b30 신전/룬 SHP 코너)·`metadata`·`clearAnimation` 훅 구현과 `nativeWriter` 의 `parentGenus` 연결도 같다.

## 3. 검사 (`tests/raw-path-data.test.mjs` 25건)

- **원본 독립 관측:** `pathdata-x86.tsv` 의 `originals` 384행(12개 경로 프로필 × 부모 genus × 헤더 변형)의 패킷 길이와 2048바이트 공유 버퍼 전체(반환 길이 밖 stale 바이트 포함)가 typed `write` 와 **Native `writeCurrent`** 모두 첫 실행에서 일치하고 이미지는 바뀌지 않는다. Native 교차 확인에서는 진단이 한 번도 나지 않고, 점 로그 개수는 꼭짓점 수, run 로그 개수는 쓴 BYTE 수와 같다.
- **cpppj 시험 이식:** 서버 flags 0 부착(25바이트 송신·Form/부모 연결·`(상태&12)==8`·삭제 전파·cleanup 한 번·부모 머리 링크 해제)과 필수 경계(송신 장치 없음·판본/버퍼 부족·다른 풀·다른 프로세스·빈 경로) 거부.
- **웹 계약:** 로그 서식과 run 분할(60칸 → 31 + 29), 직접 좌표 genus, 진단 줄 순서(0x1f/0x1f/0x24/0x1f/0x1f)와 진단·로그·부모 genus 장치 누락 거부, 송신 버퍼 물리 범위, 헤더 최소 길이 4·플래그 하위 7비트, `read` 의 모서리 이중 추가·직접 좌표·홀수 nodeCount 진단·스테이지 보정(거리 1.5 경계 포함)·시간표·배열 용량 진단과 입력 거부·훅 누락, 송신 → 수신 왕복, observer, 분배기의 선택 공유.
- 첫 실행의 실패 3건은 시험 기대값 오류였다: 다른 프로세스 선택 뒤 `packet(65)` 는 other 로 위임되고, 홀수 nodeCount 는 진단 뒤 둘째 쌍을 읽어 payload 가 더 있어야 하며, 풀 용량 32,768 에서 부모 번호 0x7fff 는 범위 안이다. 모듈은 바뀌지 않았다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 108회(1차 87개 + 재확인 21개)를 `raw-path-data.js` 에 적용해 확인했다. 1차에서 18개가 남았고 대부분 **시험의 공백**이었다. 다음을 보강해 모두 잡았다:
  1. 꼭짓점 개수 정확히 5000(허용)·helper/명령 꼬리/꼬리 단계의 쓰기 폭(WORD·WORD·BYTE, typed 와 Native 각각)·쓴 BYTE 32767 개 허용/32768 개 거부(31칸 단위 run 의 개수 상한).
  2. Native: 비직접 분기 첫 점 로그 앞의 진단 줄 0x29, WORD 가 0x8000 이 되는 순간의 음수 signed 반환(-32768+24)과 그다음 쓰기의 음수 주소 거부, 부모 SID 가 WORD 를 넘을 때 패킷에 쓰인 낮은 WORD 로 묻는 것.
  3. 수신: 서쪽 이동으로 음수가 되는 좌표(signed int → float)·첫 방향의 하위 3비트 전체·signed 시작 시각(-16 → -2)·홀수 nodeCount 진단이 비트 0 만 보는 것(5·3 진단, 2·4 없음)·수신 전에 남은 완료 표시/두 목표 지우기·시간표가 Prepare 한 값과 같음·nodeCount 0 의 점 로그 두 번·시작점 보정 거리 1.5 경계.
  4. observer: 길이의 상위 바이트(200 → 0xe0 0x00, 0x1f4 → 0x0c 0x02). 분배기: 부착 전 Path 거부와 부모 TYPE genus 0x120000 의 직접 좌표 패킷.
- 남은 동등 변형 6개(시험으로 구별할 수 없다): ① `distance > 0x7fffffff` 대 `>=` — 2³⁰ 이상의 float32 좌표는 짝수 간격이라 거리 합이 정확히 2³¹−1 일 수 없다. ② 로그의 `step & 0x1f` — step 은 최대 31 이라 마스크가 항등이다. ③ 수신 `append` 의 시간 0 쓰기 — 바로 뒤 `prepare` 가 모든 꼭짓점의 시각을 다시 쓴다. ④ observer 의 `<< 16 >> 16` — `& 0xffff` 로 하위 WORD 만 남겨 부호 확장 여부가 결과에 영향이 없다. ⑤ 꼬리 단계 `readBits(…, STAGE_AT, 1)` 의 폭 — BYTE 하나만 쓰므로 둘째 바이트를 읽어도 같다. ⑥ 분배기의 `!process.parent` — `attach` 가 Form 과 부모를 함께 기록해 Form 만 있고 부모가 0 인 부착 프로세스를 만들 수 없다(방어선).
- 변형하지 않은 방어선: 이미지/버퍼 크기 거부 메시지 변형, 훅 누락 메시지 문구, 직접 좌표 분기의 `PATH_PACKET_PAYLOAD + before` 가 음수가 되는 경로(WORD 상한 시험이 같은 거부를 확인).
- 구현 오류로 시험이 실패한 적은 없다. 첫 실행에서 `pathdata` 384행이 typed 와 Native 모두 일치했고, 실패는 모두 시험 입력(다른 프로세스 선택 뒤 packet 위임·홀수 nodeCount 둘째 쌍·풀 용량 안의 부모 번호·count 5000 일 때 둘째 꼭짓점 차이)이었다.
- 앱 수명 연결(호스트 `setFormTransmit` 에 분배기 연결, Session 이 받은 패킷의 `read` 호출과 `stagePoint`·`metadata`·`clearAnimation` 훅 구현, `nativeWriter` 의 `parentGenus` 연결)은 4-D/6 단계 조립 몫이다.
- 3.1절 6-P 둘째 항목을 이 단위로 닫는다. 다음은 **RawPathStart**(199줄 +2)이며 pathstart 512행 시험이 이 모듈의 `makePathFormTransmission` 과 client `SelectMovementProcessType`(pathprocesstype 718행)을 쓴다.
