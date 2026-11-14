# webpj 문자 편집 버퍼·저장소 TextEntryBuffer/TextEntryStorage (WB04-E438, 2026-11-14)

RawMenuControls 선행 **TextEntryStorage 66줄·TextEntryBuffer 226줄(기준선 `1f00a25a`의 .cpp/.h 줄 수, todo→port, 합계 292줄)**을 새 모듈 [`www/ui/text-entry-buffer.js`](../webpj/www/ui/text-entry-buffer.js)·[`www/ui/text-entry-storage.js`](../webpj/www/ui/text-entry-storage.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약·손 계산 값·실제 저장소 위의 동작으로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

- **TextEntryStorage**: 변경 없음.
- **TextEntryBuffer**: cpppj `1d5b1f437`(6차 한국어 K1)이 `Paste(excluded, maxLength)`에 `DisplayTextInput` 기반 확장 표시 토큰 분기(한글 본문의 완전한 문자 경계·`maxLength` 물리 BYTE 상한)를 더했다. 웹은 문자열을 UTF-16으로 다루는 자체 한국어 경로를 쓰므로(3.6절 K1 행, 순서 10) **이 분기는 이식하지 않고 기준선 `Paste`만 옮겼다.** 판정 기록으로 PORT_MAP 행의 base를 `1d5b1f43`으로 올렸고, LEFT_JOBS 3.6절 K1 행의 영향 단위 목록에 `TextClipboard`·`TextEntryBuffer`·`TextEntryGump`를 더했다(둘째 셋째는 아직 웹에 없음).

## 구현

- **`TextEntryStorage`**: 편집 본문/줄 offset 배열을 단조 증가 가상 주소 `0x20000000`..`0x60000000` 미만에 두고 `주소+오프셋` 조회를 허용한다. `allocate(size)`는 간격 `(size+31)`을 16의 배수로 내려 주소를 올리고(0바이트도 16), 범위가 `0x60000000`에 닿으면 Error, 실제 확보 실패(RangeError)는 0이며 주소를 소모하지 않는다. `release`는 0을 허용하고 블록 시작 주소만 해제하며 내부 오프셋/이중 해제는 Error다. `resize`는 0 주소=새 할당, 크기 0=해제 뒤 0, 새 확보가 성공한 뒤에만 `min(새, 옛)` 바이트를 복사해 이동하고 옛 블록을 반납하며 실패하면 원래 할당을 유지한다. `bytes(주소)`는 그 주소 이하의 가장 큰 블록에서 `주소+오프셋` 뒤 남은 범위를 빌리고(할당 끝은 빈 범위) 없는 주소/오프셋 범위 초과는 RangeError다. `create(token)`은 같은 고정 상태/처리기를 새 0x28 객체 토큰에 만들고(0·중복 Error), `releaseBuffer`는 본문(data)이나 줄표(offsets)가 하나라도 살아 있으면 거부한다. clipboard 읽기/쓰기는 실제 입력 분기에서만 요구한다. 웹 적응: `Uint8Array`는 0으로 채워진다(원문의 미초기화 값 하나로 보며 시험은 내용에 의존하지 않는다).
- **`TextEntryBuffer`**(0x28바이트 `TextEntryBufferState`: vtable·offsets·offsetCapacity·lineCount·dirty·선택 시작/끝·length·capacity·data): `construct(requested)`는 본문 2048(요청이 있으면 그 크기)·줄표 512바이트를 할당하고 **성공 시 요청 크기와 무관하게 capacity에 2048**을 쓴다. `destroy`는 data가 있을 때만 본문→줄표 순으로 반납한다. `select`는 두 위치를 정렬해 `0..length`로 clamp한다. `insert`는 NUL 앞 바이트로 선택을 교체한다(빈 입력은 선택 삭제): 지운 폭보다 넣는 폭이 크고 필드 용량이 모자라면 `((필요−1)>>10)+1)<<10`(1024 배수)로 재할당하고 실패(0)면 아무것도 바꾸지 않으며, 꼬리를 `memmove`로 옮기고 입력은 원문 REP MOVSD처럼 4바이트씩 읽은 뒤 쓰므로 겹치는 복사 결과가 바이트 단위 전방 복사와 다르다. 끝 NUL을 쓰고 선택은 삽입 끝, dirty 1이다. `reset`은 첫 NUL·길이·선택·dirty를 초기화한 뒤 같은 `insert`다. `ensureLines`는 LF마다 줄 offset을 쓰고 마지막 칸에 length를 쓰며 줄표 128칸이 차면 `4×칸+512`바이트로 확장해 복사한 뒤 옛 줄표를 반납하고(할당 실패는 이미 쓴 첫 offset/줄 수/dirty를 되돌리지 않고 false), `lineCount`/`lineLength`/`copyLine`은 그 결과와 별개로 현재 값을 읽는다(LF는 앞 행에 포함, 범위 밖은 −1/0xffffffff, 선택 구간 쌍은 행 기준). `copyAll`/`copy`/`cut`/`paste`는 clipboard와의 복사다(빈 선택은 전체, 길이 0은 장치 없이 무효과, `paste`는 256바이트에서 제외문자를 지우되 삭제한 칸을 다시 검사하고 비면 삽입 없이 dirty 1만 남긴다).
- 훅: `allocate/release/resize/bytes`(필수)·`clipboardRead(Uint8Array(256))`·`clipboardWrite(Uint8Array)`. 입력 문자열은 바이트 문자열(글자 하나가 한 바이트) 또는 NUL 이 든 `Uint8Array`이며 NUL 이 없는 바이트 배열은 TypeError(원문 `invalid_argument`)다. 웹 적응 표기: `logic_error`→`Error`, `out_of_range`/`length_error`→`RangeError`, `invalid_argument`→`TypeError`.

## 검사와 발견

새 시험 파일 2개, **34건 통과**: `text-entry-storage.test.mjs`(9, 손 계산 주소 `100→128, 0→16, 1→32, 16→32, 17→48`·실제 확보 실패는 `Uint8Array` 대역으로 재현)와 `text-entry-buffer.test.mjs`(25, 실제 `TextEntryStorage` 위의 동작과 훅을 바꾼 실패 분기). 줄 offset의 상위 바이트를 모두 쓰는 `0x01020304`바이트 줄(약 16.9MB)과 줄표 마지막 칸(127번)·겹치는 입력 복사·선택 경계가 시험에 들어 있다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 2건):** 구성하지 않은 버퍼의 `insert`는 용량 0 < 필요 1이라 오류가 아니라 1024로 재할당해 새 본문에서 시작한다는 점, 그리고 확보 실패 뒤 다음 주소는 간격 32(첫 블록 8바이트)라는 점을 바로잡았다.

**원문 경계 하나를 시험으로 고정:** `insert`의 용량 점검(`capacity < required`)에는 끝 NUL 한 바이트가 빠져 있어 필요 길이가 용량과 같으면 NUL이 할당 끝 바로 뒤에 놓인다. 웹 저장소는 물리 범위를 검사하므로 `2048`자를 넣으면 RangeError(`현재메모리범위`)가 난다(원문은 한 바이트 넘침). 2047자까지는 정상이다.

**변형 확인:** 연산자/정수 상수 변형 276개(storage 36·buffer 240) 중 249개를 잡았다. 처음 생존 58개 중 31개는 보강으로 잡았다: 기본 상태 0, 겹치는 입력의 4바이트 복사 순서, 지운 폭을 뺀 용량 계산(2047→2050 필요 3072), 같은 폭 덮어쓰기의 용량 미점검, 필요=용량 무이동, reset 실패 뒤 첫 NUL, 본문 0/NUL 없음의 줄 검색 오류, 줄표 마지막 칸·상위 바이트 읽기, `copyLine` 선택 경계, clipboard 256바이트·255자 본문, 8192바이트 넘는 `text()`, 본문/줄표 한쪽만 살아 있을 때의 해제 거부, 확보 실패 0/주소 미소모. 남은 27개는 동등 변형이다: 이 호출 경로에서는 도달할 수 없는 방어 검사(복사 범위 `||`, 출력 길이 `0x7fffffff`, 줄 복사 물리 범위), 범위를 한 칸 더 읽거나 쓰는 변형(정확한 길이의 부분 배열이라 무시됨), 값이 같아지는 clamp/선택 경계(`<`↔`<=`), 구성에서 곧바로 덮어쓰는 선택 초기화, 필요 ≤ 0 이면서 용량이 더 작은 불가능 상태, `dirty`가 곧 1로 덮이는 변형, 장치가 쓰는 NUL 뒤 꼬리 바이트 변형, 0바이트 백킹 크기.

## 남은 경계

- 실제 TextEntryGump(450줄)·Console/ChatWindow 입력과의 연결, 한국어 확장 토큰 분기, 실제 clipboard 장치(TextClipboard, adapt 단위)는 별도 단위다. 앱의 입력은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
