# 가이저 채굴 통지 복원 — 2026-10-13

마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-13**이다. 같은 PC의 정밀 프로젝트를 읽기 전용으로 내보낸 `extracted/geysermined`의 함수 범위를 사용한다. 보호 원본과 AGENTS.md는 바꾸지 않았으며 실제 게임/GUI 실행은 없다.

## 두 판본 주소와 등록

| 역할 | originals 10.78 | originalCD 10.72 |
| --- | --- | --- |
| 채굴 패킷 handler | `004266c0` | `004e41e0` |
| 수량 WORD setter | `004ac220` | `004abb00` |
| 난수 [1000, 7000) | `004558f0` | `0048cc10` |
| float 좌표 → isle 검사 | `0046f6e0` → `0046f680` | `004bdd80` → `004bdd20` |
| Bridge setter 후처리 | `004214a0` | `00448c10` |
| 타입 표 조회 | `0049a840` | handler가 직접 조회 |
| CRT `__ftol` | `004e49c0` | `004f161c` |
| 빈 패킷 observer | `0040b380` | `0040ac60` |

두 판본 모두 초기화에서 **타입0x5b, version1, mismatch0**으로 handler를 등록한다. 원본 길이 표의 항목은 **6바이트**이며 SID는 packet+4의 unsigned WORD다. sender와 connection 인자는 handler가 읽지 않는다. 등록 함수는 `RegisterGeyserMined`이며 기존 `RawPacketDispatch`의 현재 길이 표와 단일 분배를 재사용한다. CD의 주변 패킷 등록/길이에는 차이가 있으므로 다른 번호까지 같다고 일반화하지 않는다.

## 실제 몸체

1. 현재 SID 슬롯의 free 비트가 켜져 있으면 난수와 효과 없이 반환한다.
2. 게임 난수를 정확히 한 번 전진시켜 `geyserMined1.wav`부터 `geyserMined6.wav` 중 하나를 현재 raw 위치에서 flags0으로 요청한다.
3. 음향 callback 뒤 **현재 수량 unsigned WORD와 현재 nugget 타입 cost float**를 다시 읽는다. double 차이를 CRT `__ftol`처럼 signed64로 절삭하고 하위 WORD를 저장한다. 음수/초과 수량의 감김과 NaN/무한의 불확정 하위0도 보존한다.
4. setter 뒤 현재 boss가 참이고 수량이0일 때만 좌표를 보존하고 가상+0x10(flags0x1000)을 요청한다.
5. 삭제 callback 뒤 현재 hash0 머리의 isle(type97) 여부를 검사한다. 좌표에는 거의 올림 편향을 추가하지 않는다. `__ftol`의 하위 DWORD를 사용하므로 특수값/큰 좌표의 감김도 보존한다.
6. isle이 없으면 현재 emptyGeyser(type84)를 flags0으로 생성하고 보존 좌표에서 가상+0x90(flags0)으로 Pop한다.

가이저는 타입122이고 실제 nugget은 타입154다. PathStart의 가이저 찾기를 nugget 찾기로 이름 붙이지 않는다. 순서대로 읽는 mutable 전역과 raw 필드를 별도 경제 캐시에 복사하지 않는다.

10.78의 Bridge setter는 WORD를 **먼저 쓴 뒤** 현재 타입82/미매장/checking일 때 `num<=7`을 assert한다. 10.72의 같은 위치 하위 함수는 슬롯 주소를 반환할 뿐 assert가 없다. 패치의 실패 입력에서 C++는 기록 후 예외로 진단하고 PE는 assert 보고 경계의 호출을 기록한 뒤 반환한다. 이를 정상 실제 Bridge 작업으로 세지 않는다.

## 독립 대조와 실제 소비자

`tools/decomp_geysermined_oracle.py`는 두 보호 PE를 Unicorn 메모리에 읽고 새 manifest의 실제 handler·난수·수량 setter·isle 판단·CRT 몸체만 실행한다. 서식정리/음향/가상 삭제·생성·Pop과 assert 보고는 이름과 호출 규약을 명시한 관찰 경계다. 외부 게임이나 OS 함수는 실행하지 않는다.

고유 입력은 **양판본 합계2312개(각1156개)**이며 **정상2304개와 패치 assert8개**를 구분한다. 수량0/199/200/201/400/2000/65535, 분수/음수/큰 cost, free/권한/표면, 음향 뒤 수량·비용·권한·좌표 변경, 삭제 뒤 표면 변경, 좌표 절삭 특수값, Bridge checking/매장 차이를 포함한다. 각 입력의 x87 53/64비트 반복은 같은 출력인지 검사하며 고유 입력 합계에 다시 더하지 않는다. 근거/SHA는 `cpppj/geysermined.json`, fixture는 `cpppj/tests/fixtures/geysermined-x86.tsv`다.

`GeyserMinedTests.cpp`의 첫 검사는 PE의 난수·효과 순서와 가상 표 주입 주소를 제외한 두 raw 슬롯 전체를 C++와 비교한다. 두 번째 검사는 **원본 등록 → 실제 단일 PacketDispatch → 같은 SidPool의 채굴 handler → 기존 SquidFactory/공통 RawSquidDestroy/SquidPop/Unpop**를 연결한다. 수신 시각/기록/중첩 깊이, client 가이저 반납, server emptyGeyser 생성과 현재 hash 등록을 확인한다. 해당 검사의 기반 자산 생성/공통 삭제 범위를 파생 가이저/emptyGeyser 전체 수명 완료로 세지 않는다.

검사 명령:

```text
python -X utf8 tools/decomp_geysermined_oracle.py --verify
```

C++ 집중 **2개 검사 통과**: 전수2312개 대조(정상2304/assert8)와 실제 Packet/Factory/공간 연결이 성공했다. 초기06 빌드의 C4457 두 경고는 callback 안의 좌표 이름을 cellX/cellY로 바꾸어 교정했다. `tools/analyze_geysermined_shadow_lineage.py`가 이름 변경을 제거한 전체 테스트 소스와 이전 native 보고서/fixture의 불변을 증명하며 새 관찰0개로 현재 SourceSHA를 연결한다. **최종 Release 두 타깃 종료0·경고/오류0, 전체974개 검사/실패0**이다. 통합 스냅샷은 `run6.json`이며 최종 로그는 `extracted/run6-*`에 보존했다. 새 수신 처리기를 작성한 것만으로 실제 GameWorld가 연결되었다고 표시하지 않는다.

## 남은 실제 경계

- 실제 raw 월드의 packet 등록/송수신, 사제·골렘의 CarrierArrival이 contained nugget을 생성하고 채굴 통지를 작성/송신/직접 처리하는 상위 사슬은 계속 미연결이다.
- 파생 가이저 삭제/emptyGeyser 생성·postPop 및 실제 음향 자산 재생은 같은 월드의 실제 소비자를 연결해야 한다.
- PathProcess cleanup의 helper 인접 finder → SharedRegular(지연1) → helper0, 사제 주변 신전 ritual, abstract helper 환불/삭제는 남아 있다. 기존 +cc CarrierCheck와 건설 운반 type23의 ConstructionArrival을 재사용한다.
- **10.78 cleanup0048aa00에는 TempleAt·contained nugget·동일 owner/graph 조건의 nugget 삭제가 추가되어 있고 CD10.72 cleanup0047f0a0에는 이 분기가 없다.** 다음 복원에서 두 몸체를 하나의 동일 분기로 합치지 않는다.
- 캠페인1-1 실제 클릭 명령/애니메이션·채굴 운반·의식·UI·승패와 완주 대조는 이 단위의 독립 대조 범위를 넘는다.
