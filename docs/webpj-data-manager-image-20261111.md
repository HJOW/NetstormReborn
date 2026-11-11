# webpj DataManager 파일 이미지 읽기(RawDataManagerImage · DataManagerFileRead) — WB04-E284 (2026-11-11, 4-B)

3.1절 4-B 의 **RawDataManagerImage**(142줄)를 `core/raw-data-manager-image.js` 로, client **DataManagerFileRead**(E272 이월)를 웹 어댑터 `core/client-data-manager-file-read.js` 로 옮겼다. 원본 `0044cdb0`(mark 진행)·`0044d380`(섹션 본문)·`004bce10`(Template 파일 서명)·`0044d980`(파일 읽기 부모)이며 기준선(`1f00a25a`) 뒤 `RawDataManagerImage.cpp/.h`·`DataManagerFileRead.cpp/.h` 에는 변경이 없다(3.6절 ①). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 선행: E274(MemStream·목록·소멸)·E282(Selection)·E283(Resize)·E19(SNART).

## 1. 규칙

- **`advanceDataManagerMark`(0044cdb0):** 이전 mark 가 있으면 mark 위치의 부호 있는 16비트 길이가 `pos − mark`(32비트 wrap 뒤 signed)와 같은지 확인하고(다르면 `MarkLength` 진단), 현재 pos 를 새 mark 로 기록한 뒤 signed `end < pos` 이면 `MarkPosition` 진단, 마지막으로 pos 에 2 를 더한다. 진단이 mark/pos/end 를 바꾸면 그 현재 값이 후속 쓰기에 쓰인다.
- **`readDataManagerImage`(0044d380):** 이름 표(가상 +4 의 같은 SNART 표)를 먼저 조회하고, 시작 pos 가 2 가 아니면 `SourcePosition` 진단 뒤 mark 를 진행한다. 이름 표 순서대로 빈 이름이 나올 때까지: mark 가 없으면 `MissingMark` 진단 → 길이 = (mark 길이 − 2)의 16비트 wrap → 새 노드(60) 할당·생성(`constructDataManagerStream`) → 입력 pos 를 payload 뒤로 갱신(end 는 signed 로 올림) → 입력 payload 를 노드 stream 에 쓰고 pos 0 → 같은 이름이 이미 있고 새 섹션이 비었으면(버퍼 또는 end 0) 새 노드의 stream 소멸과 노드 반납(기존 것 유지), 아니면 기존 것을 제거하고 새 노드를 목록에 추가 → mark 진행(마지막 mark 는 end+2 까지 갈 수 있음).
- **`readTemplateFileSignature`(004bce10):** data(+0)와 end(+8)가 둘 다 비영일 때만 현재 타입 수(signed ≥ 256 이면 진단 뒤 계속)를 확인하고, 매 BYTE 현재 data/pos 를 다시 읽어 첫 두 BYTE 를 읽는다. manager `+0x10c` 에 첫 BYTE, `+0x110` 에 둘째 BYTE 가 `0xfe` 인지(1/0)를 쓴다. 첫 BYTE 가 `'F'` 가 아니면 현재 파일 이름으로 거부 요청(`rejectFile`)을 보내고 false. 논리 end 의 두 BYTE 길이는 추가로 검사하지 않고 물리 범위는 별도로 보호한다.
- **`readDataManagerFile`(0044d980):** 20BYTE MarkStream(mark −1) → `readFile(이름, 1, 길이 DWORD, 0)` 의 data 토큰 → end = 길이 → 없으면 false → 가상 서명 → 실패면 반납 후 false → 섹션 본문 → 현재 data 할당 반납. 서명/본문이 source.data 를 바꾸면 마지막의 현재 포인터를 반납하고, 예외에서도 같은 반납을 **한 번만** 시도한다.
- **client `readDataManagerFileImage` / `bindDataManagerFileRead`(웹 어댑터):** cpppj 는 BaseFile 의 FILE·키·scratch 해독을 실제 CRT 에 연결하지만 웹은 `GameFiles.read` 로 대체한다. 보존하는 계약: 미발견은 `null`, 길이 0 인 존재 파일도 **최소 4BYTE 할당을 소유**(빈 파일을 새 파싱 오류로 주장하지 않음), `{ storage, length }` 로 할당과 논리 길이를 분리, storage 는 파일 바이트의 사본, 이름은 첫 NUL 까지. `bindDataManagerFileRead(hooks, files)` 는 `readWhole` 만 설치하고 기존 훅은 그대로 둔다.

## 2. 검사 (`tests/data-manager-image.test.mjs` 11건)

- `datamanager-read-x86.tsv` **144입력**: 전체 1400BYTE(manager 12 + MarkStream20 + 노드 6개 + 버퍼 7개 + 입력 128BYTE 둘 + 이름 표 64BYTE, `~값x개수~` 압축)에서 섹션 본문을 변이(없음 · 할당 중 입력 data 교체 · 반납 뒤 current 변경)로 실행해 매 외부 효과(이름 표 `T` · 비교 `Q` · 노드 할당 `S` · 버퍼 할당 `A` · 반납 `F`/`N`) 직전 전체 메모리와 최종 메모리를 대조했다. **첫 실행에 통과했다.**
- 손 시험 6건(cpppj DataManagerRead_MarkAssertionUsesCurrentFields · InitialPositionAndRequiredNames · DataManagerFile_SignatureBytesAndSignedTypeLimit · SignatureFailureAndCurrentRereads · ReadParentBranchesAndCleanupCurrent · BodyAndReleaseFailureOnlyOnce): 모두 통과.
- 계약 3건: mark 진단의 signed 비교와 부호 있는 16비트 mark 길이, 섹션 본문 앞부분의 `MissingMark` 진단·16비트 길이 wrap(할당 크기 32766)·입력 end 올림의 signed 비교(센티널로 흐름을 끊어 관측), 서명은 data·end 가 모두 비영일 때만 장치를 부르며 manager 276BYTE 미만 거부.
- client 어댑터 1건: 미발견 null, 길이 0 → storage 4BYTE, 길이 3 → `[1,2,3,0]`, 사본(storage 를 바꿔도 보관 파일 유지), NUL 이후 이름 무시, 훅 설치.
- 변형 확인 44개 중 처음에 11개가 살아남았다(진단 분기 · 16비트 길이 · signed 비교는 원본 정상 입력이 진단/경계에 닿지 않기 때문). 8개를 위 계약 3건으로 잡았고 남은 3개는 **동치 변형**이다: NULL 새 노드에서 생성자 호출(이어서 조회가 거부됨), 빈 새 섹션 조건의 data 검사 제거(data 0 이면서 end 비영인 stream 은 만들 수 없음), 파일 없음에서 반납(포인터 0 이면 반납 함수가 아무것도 안 함).

## 3. 남은 경계

- cpppj `DataManagerLiveRead_*`(Native 64행)와 `DataManagerFile_*`/`CampaignTemplates_*`: 64행 fixture 는 BaseFile 의 FILE/키/seek/read/close 사건 로그(T/S/R/C/F)와 `ReadWholeBaseFile` 모델을 요구한다. 웹은 `RawBaseFileWholeRead` 를 `GameFiles.read` 로 대체(adapt)해 그 사건 로그 모델이 없으므로 이 fixture 는 이식하지 않는다. 같은 파일 부모(`readDataManagerFile`)의 서명·본문·반납 순서는 위 손 시험과 144행이 직접 검증한다. 실제 파일을 `readFile` 훅에 연결하는 부분은 `CampaignTemplateStorage`(4-D)가 `bindDataManagerFileRead` 로 한다.
- 3.1절 4-B 는 14/30 이다. 다음은 **FortSerializationGlobals 뒤의 RawFortRandomPlacement**(212줄)이다.
