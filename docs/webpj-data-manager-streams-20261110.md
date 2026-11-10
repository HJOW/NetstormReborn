# webpj DataManager 스트림 묶음(MemStream·Lifetime·List) — WB04-E274 (2026-11-10, 4-B)

3.1절 4-B 의 처음 세 항목 **RawMemStream**(94줄)·**RawDataManagerLifetime**(60줄)·**RawDataManagerList**(67줄)을 한 단위로 옮겼다. MemStream 압축(`0044cd40`)이 소멸(`0044d040`)을 부르고, 연결 목록 제거(`0044d0f0`)가 같은 소멸을 부르며, cpppj 고정 관측 `datamanager-streams-x86.tsv` 404행이 세 단위를 한 메모리 위에서 함께 실행하므로 따로 나눌 수 없다. 기준선(`1f00a25a`) 뒤 세 단위의 소스와 두 관측 파일에는 변경이 없다(3.6절 ①). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 옮긴 모듈

| 웹 모듈 | cpppj / 원본 | 하는 일 |
| --- | --- | --- |
| `core/raw-mem-stream.js` | `RawMemStream` · `0044cc20/cc80/cce0/cd40` | 실제 16 바이트 MemStream(+0 버퍼 핸들 · +4 pos · +8 end · +12 alloc)의 할당·대입·쓰기·압축. `allocateMemStream`: 이미 버퍼가 있으면 진단(ExistingData) 뒤 alloc 을 먼저 쓰고 할당 → 포인터 → **그 시점의 alloc** 만큼 0 채움 → pos/end 0. `copyMemStream`: 기존 버퍼 반납 → 첫 16 바이트 비움 → 원문 end 로 할당 → DWORD 단위 읽고 쓰기(겹침 허용) + 나머지 바이트 → end 는 원문 end. `writeMemStream`: **signed** 용량 부족이면 진단(WriteCapacity)을 내고도 계속하며 진단 뒤 stream 필드를 다시 읽어 pos 위치에 복사하고 pos/end(signed)를 갱신. `compactMemStream`: end==alloc 이면 무동작, signed 양의 end 는 새 버퍼로 옮기고 옛 버퍼 반납(반납 뒤 필드 변이를 다시 0 으로 덮지 않음), 그 밖은 소멸 |
| `core/raw-data-manager-lifetime.js` | `RawDataManagerLifetime` · `0044d040`, `004bd330 → 0044da60` | `destroyMemStream`: 비0 버퍼를 먼저 반납한 뒤 pointer/alloc/end/pos 순서로 0. `destroyDataManager`: base 가상 표 식별자 `0x00507dd8` 을 쓰고 현재 head 를 먼저 unlink 한 뒤 각 노드를 소멸·반납하며 매 반복 뒤 head 를 다시 읽는다 |
| `core/raw-data-manager-list.js` | `RawDataManagerList` · `0044ce10`, `0044d0f0` | `appendDataManagerStream`: 빈 목록은 head/current 두 필드만 쓰고, 아니면 next 를 따라 마지막 노드를 찾아 압축한 뒤 next 와 current 를 쓴다. `removeDataManagerStream`: head 이면 head 를 옮기고 아니면 직전 노드를 찾아 건너뛰게 한다(선행 링크가 없으면 진단 뒤 같은 선행 노드를 다시 읽음). 노드 소멸 → 반납 |

실제 호스트 포인터 대신 DWORD 핸들과 `hooks`(`buffer` · `allocate` · `release` · `assertion` · `lifetime.stream/releaseBuffer/releaseStream` · `compact` · `missingPredecessor`)로 같은 버퍼·노드를 조회한다. 도달한 경계만 요구하고 진단 소비자가 없으면 빈 소비자를 만들지 않고 거부한다. 일반 `memcpy`/`copyWithin` 으로 바꾸지 않고 원본의 한 DWORD 읽기 뒤 쓰기 반복을 유지한다.

## 2. 검사 (`tests/data-manager-streams.test.mjs` 5건)

- `datamanager-streams-x86.tsv` **404행 전부**(할당 48 · 쓰기 168 · 압축 48 · 추가 108 · 제거 32): 전체 716 바이트(노드 4개 · 버퍼 4개 · 원문) 위에서 실행한 뒤 전체 메모리와, 외부 효과(할당 A · 반납 F · 진단 D · 노드 반납 N) 직전의 전체 메모리를 모두 대조했다. variant 는 원본 관측의 명시 변이(할당 중 필드 변경 · 반납 뒤 manager 변경 · 진단 뒤 pos 변경 · 겹치는 원문)이다.
- `datamanager-lifetime-x86.tsv` **392행 전부**: 전체 412 바이트(manager 16 + 노드 5개)에서 MemStream 소멸(`S`)과 DataManager 소멸(`D`/`T`)의 free 순서와 외부 효과 뒤 head 재조회를 대조했다.
- 손 시험: 실제 폭 부족 거부(15·11 바이트), 길이 0 할당과 빈 목록 추가·소멸이 외부 장치를 요구하지 않음, 진단 뒤 수리된 같은 선행 노드를 다시 읽음(cpppj `DataManagerStreams_RemoveRequeriesRepairedPredecessor`), 쓰기의 signed 용량·end 비교, 대입(`copyMemStream`)의 pos 0·원문 end.
- 변형 확인 26개 중 3개가 처음에 살아남았다: 쓰기의 **signed** 용량 비교와 end 비교(고정 관측이 양수 범위뿐), 대입의 마지막 end 쓰기(`copyMemStream` 은 이 두 관측에 없고 Create/Resize 관측이 쓴다). 음수 pos/end 와 대입 시험을 더해 모두 잡았다. 한 변형(`destroyDataManager` 의 `node = head`)은 head 가 매 반복 끝에 같은 필드에서 다시 읽히므로 동치 변형이다.

## 3. 연결·남은 경계

- `copyMemStream` 의 본격 소비자는 다음 항목 **RawDataManagerResize**(Create/복제/대입 220행)와 **RawDataManagerImage** 이고, Template 저장소 소비자(`CampaignTemplates_*` 3건)는 **Template** 항목에서 옮긴다.
- 이 묶음으로 3.1절 4-B 의 RawMemStream · RawDataManagerLifetime · RawDataManagerList 가 끝나고(3/29), 남은 DataManager 항목은 Image · Resize · Selection 이다.
