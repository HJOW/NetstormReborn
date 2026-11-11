# webpj DataManager 임시 stream·이름 교체·최소 확장·섹션 생성/복제(RawDataManagerResize) — WB04-E283 (2026-11-11, 4-B)

3.1절 4-B 의 **RawDataManagerResize**(100줄)를 `core/raw-data-manager-resize.js` 로 옮겼다. 원본 `0044d070`(stream 생성자)·`0044d160`(이름 교체)·`0044d1b0`(섹션 생성)·`0044d230`(복제)·`0044d290`(최소 확장)이며 기준선(`1f00a25a`) 뒤 `RawDataManagerResize.cpp/.h` 에는 변경이 없다(3.6절 ①). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 선행: E274(MemStream·목록·소멸)·E282(Selection).

## 1. 규칙

- **`constructDataManagerStream(stream, name, size, hooks)`:** 노드 첫 16BYTE(MemStream)를 비우고 `max(size, 1)` 로 할당(`allocateMemStream`)한 뒤 이름과 NUL 만 쓰고(`name` 이 `null` 이면 이름 첫 BYTE 만 0, 나머지 이름 BYTE 는 초기화하지 않음) next(`+0x38`)를 0 으로 쓴다. 같은 핸들을 돌려준다. 이름은 첫 NUL 까지이고 40BYTE 이상이면 거부한다.
- **`createDataManagerStream(manager, name, hooks)`:** 같은 이름이 있으면 제거 → 새 노드 60BYTE 할당 → 비영이면 용량 16384 로 생성 → 목록에 추가 → **새 핸들이 아니라 추가 효과 뒤의 현재 manager.current** 를 돌려준다. 새 노드 할당이 NULL 이면 임의 노드를 만들지 않고 0 을 추가한다.
- **`cloneDataManagerStream(manager, sourceManager, name, hooks)`:** 다른 manager 의 현재 원문을 찾고(없으면 진단 `missingSource` 뒤 계속) 대상을 만든 뒤 원문 data(`+0`)가 있을 때만 대입(`copyMemStream`)하고 대상 pos 를 0 으로 돌린다. 원문의 data/end 는 대상 생성이 끝난 뒤 다시 읽는다.
- **`renameDataManagerStream(manager, name, previousName, hooks)`:** 대상 이름이 있으면 제거한 **뒤에** 원래 이름을 찾고 대상 이름과 NUL 만 쓴다(선행 current 를 캐시하지 않음).
- **`ensureDataManagerStreamSize(manager, name, minimum, hooks)`:** 원문 end 가 `minimum`(signed) 이상이면 할당 없이 current 를 돌려준다. 아니면 임시 이름 `T$e$m$p` 의 새 stream 을 `minimum` 으로 만들어 원문 내용을 쓰고(`writeMemStream`) 남은 구간(`alloc - pos`)을 0 으로 채우며 pos/end 를 용량으로 올린 뒤(end 비교는 signed) 목록에 추가하고 임시 이름을 원래 이름으로 교체한다. 원문 노드는 생성 전에 보관하되 data/end 는 생성이 끝난 뒤 다시 읽는다.

## 2. 검사 (`tests/data-manager-resize.test.mjs` 6건)

- `datamanager-resize-x86.tsv` **286입력**: 전체 796BYTE(manager 12 + 노드 4개 + 버퍼 5개 + 이름 2개)에서 최소 확장(G)·이름 교체(R)·생성자(그 밖)를 이름 종류와 변이 4종(생성 중 원문 data/end/current 변경 · 반납 뒤 current 변경 · 할당 중 임시 alloc 변경)으로 실행해 반환 핸들·실행 뒤 전체 메모리·외부 효과(비교 `Q` · 할당 `A` · 노드 할당 `S` · 버퍼 반납 `F` · 노드 반납 `N`) 순서와 그때의 전체 메모리를 대조했다.
- `datamanager-create-x86.tsv` **220입력**: 전체 17380BYTE(manager 둘 + 노드 5개 + 버퍼 6개 + 16384 큰 버퍼 + 이름, `~값x개수~` 반복 표기 압축)에서 섹션 생성(N)·복제(C)·MemStream 대입(M)을 변이 4종으로 실행해 **매 경계 직전 전체 메모리**와 최종 메모리를 대조했다. 두 fixture 모두 **첫 실행에 통과했다.**
- 손 시험: 빈 목록의 NULL 노드 할당(0 추가·반환·필요 장치 누락 거부), MemStream 자기 대입(반납 뒤 비워진 길이 0 으로 재할당, 버퍼 조회 금지).
- 계약: 이름 39/40글자·`null` 이름·첫 NUL 까지만 쓰기·크기 0/음수 → 1 로 할당·노드 조회 장치 누락/NULL 핸들 거부·signed 최소(음수 최소는 항상 충분)·원문 누락 진단 유무·원문 길이 0(NULL 데이터)에서도 버퍼 조회 없이 확장·복사 뒤 남은 구간을 0 으로 채움·end 올림의 signed 비교(버퍼 조회 훅으로 꼬리를 더럽히고 end 를 `0xffffffff` 로 바꿔 구별).
- 변형 확인 32개 중 처음에 6개가 살아남았다. 3개(남은 구간 0 채움, end 올림의 signed 비교, 길이 0 버퍼 조회 생략)는 위 마지막 두 계약으로 잡았다. 나머지 3개는 **동치 변형**이다: 생성 반환을 새 핸들로 바꾸기(추가가 current 를 새 핸들로 쓰는 마지막 효과라 같음), 복제 후 pos 0 쓰기(새 stream·대입 모두 pos 0), 복제 원문 노드를 생성 뒤에 조회하기(노드 조회가 안정적).

## 3. 남은 경계

- cpppj `CampaignTemplates_MinimumStateUsesTemporarySectionAndSharedData`·`TailCopiesBeforeCompaction`·`StateCreationParentPreservesSharedOrderAndTail`·`DataManagerResize_SufficientStateAndRequiredBinding` 은 `CampaignTemplateStorage`(4-D)의 `EnsureStateSize`/`InitializeState` 시험이다. 4-D 에서 이 모듈을 쓸 때 닫는다.
- 다음은 **RawDataManagerImage**(142줄)와 client `DataManagerFileRead` 이다.
