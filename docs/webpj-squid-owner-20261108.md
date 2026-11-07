# webpj 소유자 지정·장부 구조·공통 작은 setter 이식 — WB04-E2

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 두 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/SquidOwner`·`o/RawCommonOwner`·`o/RawSquidLevel`·`o/RawSquidRegion`·`o/RawSquidHitPoints`·`o/RawSquidWord.h`와 `o/SquidPostPop.h`의 **장부 구조**(목록·상태)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 옮긴 것

| 웹 모듈 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `www/core/squid-postpop-state.js` | `o/SquidPostPop.h/.cpp`(구조와 목록 메서드) | `SquidPostPopList`(entries/count 독립 목록 또는 `bindCurrent` 실제 Player +0/+4/+8 연결, `append`·`removeAll`(안정 압축, stale 꼬리 보존)·`items`(손상 count 거부)), `RawTotalMadeStorage`(세 256칸 표), `SquidPostPopState`(억제/Graph/보류 배치/로컬 소유자/무효 Graph 번호, AI 부착 9칸, 전역 비용 signed, 공급·재계산 알림·깊이, 세 Totalmade 표, 공급원/공장/소유자별 공장 목록과 `bindTotalMade`/`bindTotalCost`/`bindProviders`/`bindFactories` 장기 저장소 대여) |
| `www/core/raw-common-owner.js` | `o/RawCommonOwner.cpp/.h` | 004adf00: 소유자 범위 진단(일반 0..8은 0xa02, challenge 1..39는 0x9ff, 정상 복귀하면 계속) → 요새/전투이고 genus 0x4200이면 이전 소유자 목록에서 제거 → 소유자 BYTE 저장 → 현재 모드를 다시 읽어 void·buried가 아니면 새 소유자 목록에 추가 |
| `www/core/squid-owner.js` | `o/SquidOwner.cpp/.h` | `SquidOwner`(타입 표 복사 또는 `bindDiagnostics`로 빌린 현재 표, 소유자 0은 목록 없음, 9 이상은 현재 Player 물리 범위 밖이라 선택 시 거부), `SquidOwnerMode` |
| `www/core/raw-squid-level.js` | `o/RawSquidLevel.cpp/.h` | 상태 BYTE 상위 3비트 레벨: 항상 참인 `has`(원본 hasLevel의 OR 조건 그대로), `get`, `set`(signed 범위 진단 0x579 뒤에도 저장 계속, 하위 5비트 보존) |
| `www/core/raw-squid-region.js` | `o/RawSquidRegion.cpp/.h` | 004acd20 `setSquidRegion`(genus 분기: 미지원 진단·다리 region 진단·섬 WORD 저장)과 **004acda0 `getSquidRegion`**(기준선 뒤 `9cb0547f` 추가분, 3.6절 ②→반영) |
| `www/core/raw-squid-word.js` | `o/RawSquidWord.h`, `o/RawSquidHitPoints.cpp/.h` | 004ac220 표면 WORD setter(낮은 BYTE부터 저장, 현재 다리 TYPE이면 통지; 통지 부재는 저장을 되돌리지 않음), 004adca0 현재 HP DWORD setter(이웃 필드 보존) |

## 2. 검증

| 검사 | fixture | cpppj 대응 |
| --- | --- | --- |
| `squid-owner.test.mjs` | `owner-x86` 10.78(originals) Owner 행 전체: 소유자 BYTE·소유자별 개수와 저장 칸 6개 전체(stale 꼬리 포함)·슬롯 Adler-32 | OwnerTests(owner_patch_x86_fixture 등 10.78 비 bound 재생) |
| 같은 파일 | (독립 입력) | owner_rejects_out_of_range_players_before_writing, 진단 정상 복귀 뒤 하위 BYTE 저장, 빌린 현재 TYPE 표, 목록 구조(가득 참·안정 압축·손상 count·현재 연결) |
| `squid-small-setters.test.mjs` | `squid-level-x86` 10.78 행 | CampaignMotion_ReplaysActualLevelNativeBothEditions(10.78 부분) + 계약(범위 진단 뒤 저장, 하위 5비트 보존), 섬 번호 getter/setter 분기, 표면 WORD, HP 폭 |

최초 실패와 교정: 구현 직후 owner 1000행 이상과 레벨 행이 처음부터 통과했다.

## 3. 3.6절 판정

- `RawSquidRegion`: 기준선 뒤 `9cb0547f`(중앙섬/군도)가 getter `GetSquidRegion`(004acda0)을 추가했다. 단독 섬 지도에서 쓰이는 getter이므로 ②로 보고 `getSquidRegion`을 이식했다(base `9cb0547f`). 다른 5개 행은 기준선 뒤 변경이 없다.
- `SquidPostPop.cpp/.h`는 변경이 없다. 다만 처리기 본체는 아직 옮기지 않아 행 상태는 `partial`이다(장부 구조만 `port` 수준).

## 4. 경계와 다음 시작점

- 실제 Player 레코드 연결(`ClientPlayerRecords` bound 재생, 744행)은 Client/Player 단위(WB05~06)에서 옮긴다. `SquidPostPopList.bindCurrent`는 그 연결을 받는 인터페이스만 두었고 계약 검사로 확인했다.
- 소유자 지정에 필요한 현재 모드 전역(요새/전투/challenge)은 `SquidOwnerMode` 값 객체이며 실제 연결은 World 조립(WB05)에서 한다.
- 다음 WB04-E 단위: `SquidPostPop` 처리기 본체(Graph·Player 통지·마나 등록·종류별 후처리 분배)와 종류별 PostPop, `SquidDisplay`/`RawSquidDisplayParent`, `SquidFrame`, `RawSquidName`·`RawSquidDefense`·`RawSquidSelection` 등.
