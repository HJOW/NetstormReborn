# webpj: AssetDeletionVtables.inc 대조 (WB04-E1209, 2026-11-22)

cpppj `client/AssetDeletionVtables.inc` 는 10.78 PE 의 자산 생성자 83개가 가진 가상 표(VT)마다 `+14`(전처리)·`+18`(후처리)·`+34`(부모 삭제 통지) 진입점 주소를 모은 표다. 장기 Native 삭제(`RawSurfaceAssetDeletion.cpp`)가 현재 슬롯의 VT DWORD 로 행을 골라 가상 선택에만 쓰며 주소를 실행하지 않는다.

## 확인한 것

| 항목 | 결과 |
| --- | --- |
| 가상 표 83개 | 원문 주소 오름차순·중복 없음, 웹 `SQUID_ASSET_VTABLES`(`www/core/squid-vtables.js`) 83칸과 같은 주소·같은 순서 |
| 전처리/후처리 묶음 | 원문의 (전처리, 후처리) 묶음이 웹 삭제 자격 함수(`kinds`)와 일대일: 공통 Damageable 14·Blocker 3(완성형 Ice 후처리 `48edf0`)·성장형 Ice 1(`48eb80`)·신전 4(`4dca50`)·Fence 수명 1(`457990`)·너겟 1(`484f80`) |
| VT+TYPE 쌍 자격 | 다리·연결기·사제·사제 보호막·묻힌 Squid·Fence 두 쌍·표시 더미·플라이어 세 쌍이 원문의 같은 묶음에 있고, 같은 VT 의 다른 TYPE 은 자격이 아니다 |
| 폭탄 | 후보 genus 비트가 있는 TYPE 에서 자격이 되는 VT 는 정확히 열 개(`0x501dd0`…`0x503188`), Point Blast(`0x502370`)만 전용 전처리(`41e960`), 나머지는 공통 삭제, 163 전용 그림(`0x509b98`)도 공통 |
| 통지 열(VT+34) 구성 | 77행은 빈 반환(`41c100`), 3행(`0x509148`·`0x509210`·`0x50eb98`)은 현재 자산을 flags 0 으로 삭제(`48f0a0`), 3행은 비행장 홈 통지(`45b4a0` 훨리베이스 `0x509930`·`45b5b0` 맨 오 워 `0x5099e8`·`45b680` 데빌메이커 `0x509aa0`) |

시험: `tests/asset-deletion-vtables.test.mjs`(원문 표를 읽어 대조, 없는 환경에서는 건너뜀), 교차 확인 소스 등록 `tests/fixtures/manifest.json`(`src-asset-deletion-vtables`).

## 남은 것

- 통지 열(VT+34)은 원문에서 `NotifyAssetDependency`(Native 전용: `RawCampaignSession.cpp` ReleaseDependent·홈 통지·`RawSurfaceDepend.cpp` 감시)만 쓴다. 웹은 typed 경로를 따라 훨리베이스(`0x509930`)·맨 오 워(`0x5099e8`) 홈 통지만 `raw-campaign-session-flyers.js` 에 가지고, 데빌메이커 홈(`0x509aa0`)은 비행 정리(`raw-flyer-frame.js` `parentDying`)가 데빌 자신(바람 TYPE)을 건너뛰어 도달하지 않는다. flags 0 삭제 세 VT 와 데빌메이커 홈 통지를 웹이 실제로 받는 경로가 있는지는 자연 진행에서 다시 확인할 수 있다.
- 표를 소비하는 `RawSurfaceAssetDeletion.cpp` 는 별도 partial 행이다(Bind*Runtime 대여·장기 소유 구조 재대조).
