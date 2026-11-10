# webpj Fort 내용물 원시 읽기(RawFortContentsLoad) — WB04-E275 (2026-11-10, 4-B)

원본 `004bd130` 의 원시 stream 내용물(Contents) 읽기를 `core/raw-fort-contents-load.js` 로 옮겼다(cpppj `o/RawFortContentsLoad.cpp/.h`, 79줄, 기준선 `1f00a25a` 뒤 변경 없음 · 3.6절 ①). 3.1절 4-B 의 **RawFortObjectStream**(다음 항목)이 객체 안의 내용물을 읽을 때 `contents` 훅으로 이 읽기를 부르므로 먼저 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 분류 변경: `exclude-edit` → `port`

`tools/port-map.mjs` 의 `edit-existing-fort` 목록(기존 Editor 설치의 타입 변환·Deck·Subscriber·지형 회복)에 `RawFortContentsLoad` 가 들어 있었으나, cpppj `RawCampaignSession.cpp` 의 일반 Objects 적재(3201행 `hooks.contents`)와 기술 wrapper(3280행)가 모두 `LoadFortContents` 를 부른다. 웹의 Tutorial1 Fort 적재가 컨테이너(건물 안의 사제 등)를 읽으려면 필요하므로 목록에서 빼고 `PORT_MAP.tsv` 를 다시 만들었다(해당 행 `exclude-edit/-` → `port/todo`). 이 판정은 `RawFortTypeNamesLoad`·`RawFortDeckLoad` 등 목록의 나머지 단위에는 영향이 없다.

## 2. 읽기 규칙

한 항목: 개수 BYTE → 현재 변환표(`globals.conversion`, 읽을 때마다 현재 값)로 바꾼 종류 BYTE(0 이면 반환 false; `checking` 이 켜지고 `relaxedConversion` 이 0 이면 변환 결과 0 을 진단 0x79 로 알리고 현재 변환 토큰을 다시 읽음) → 첫 TYPE `flags1` 의 QA(BYTE)/QB(signed WORD, 읽은 뒤 pos+2 를 쓰고 signed pos>end 이면 진단 0x2e) 수량 → 두 번째 TYPE 조회의 `contentListFlags`(0 이면 부모 그릇 flags, 그것도 0 이면 BYTE) → `listFlags` 에서 0x100 을 지우고 0x10 이 있으면 attach flags 의 8 을 지우고 0x10 을 켠다 → `create(parent, kind, quantity, listFlags, attachFlags, 0)` → 첫 TYPE `flags1` 의 container(0x20000)이면 만든 자식을 부모로 재귀(빈 data/end 면 false). 한 `access` 호출 안에서 모든 재귀를 실행하고 밖에 fields/buffer 를 저장하지 않는다. 직접 BYTE 는 논리 end 를 보지 않고 실제 버퍼 끝만 거부한다.

## 3. 검사 (`tests/fort-contents-load.test.mjs` 2건)

- `fort-contents-load-x86.tsv` **668행 전부**(정상 622 · 호출 중단 44 · 변환 주소 0 실패 2): 두 stream 의 실제 16 바이트와 2048 바이트 물리 버퍼, 변환 토큰 11/22, 원본 관측의 명시 변이(중단 · 변환 교체 · stream 필드 교체 · TYPE 교체 · 부모 flags · checking 0 · Create 반환 0)에서 결과 · 두 stream 의 8 DWORD · 하위 호출 순서(T/P/C/A/F)를 대조했다.
- 손 시험(cpppj `FortContentsLoad_SelectedDevicesAndPhysicalBounds`): 빈 data 는 장치 없이 false, 선택된 Create 누락은 개수·종류·listFlags 를 소비(pos 3)한 뒤 실패, 논리 end 1 이어도 물리 끝(2048)에서만 거부.
- 첫 실행에 모두 통과했고, 변형 확인 14개(진입 end 관문 · 0xf6 경계 · relaxed 무시 · 진단 뒤 재조회 · 종류 0 · QA · QB signed/부호 확장 · 0x100 · 0x10 attach · container 재귀 · listFlags BYTE · 부모 flags · 첫 TYPE flags)를 모두 탐지했다.

## 4. 남은 경계

- 3.1절 4-B 에 항목 **RawFortContentsLoad**(79줄)를 추가해 완료로 표시했다(4/30).
- 다음은 **RawFortObjectStream**(121줄)이다: `RawFortObjectLoad::ObjectStream/ChunkStream/SectionStream/LoadStream` 을 `FortObjectLoad` 위에 옮기고 `fortobjectload-x86.tsv` 의 원본 609행(객체 573 · 전체 순회 36)을 stream 경로로 재생한다. 이 읽기를 `contents` 훅으로 쓴다.
