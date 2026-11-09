# 단위 1217 — World 없는 Regular·Form 분배의 잔여 조사

**정적 조사만 마감했다. 제품은1216 그대로이며 구현/전체 NoWorld는 미완료다.**

## 수량과 범위

원본10.78 PE에서 실제 자산83VT+Depend/Process/Gump/Content4VT의 +5c를 직접 읽고 현재 `RawSurfaceStorage::HandleDefaultRegular`의 분기/상수 목록과 대조했다. **87VT 중 연결36(자산33+Form3), 미연결51(자산50+Content1), 고유 함수26**이다. 연결36은 base16·Damageable14·고유6이다. 이는 분배가 존재하는 수이며 선택되는 큰 하위까지 모두 장기 연결됐거나 실행 검증을 통과했다는 수가 아니다.

51VT는26함수를 공유한다. 아래26묶음은 M6-n의 일부이며 상위16계획·34개 버그와 별도로 더하는 전체 계획 수가 아니다. 함수별로 원문의 실제 NoWorld 도달성·현재 owner·하위 장치를 조사한 뒤 같은 기존 본문을 연결한다. 원문에서 도달하지 않는다는 근거가 있으면 그 근거를 남긴다. 기존 몸체의 단순 주소 언급은 연결/완료 증거로 쓰지 않는다.

## 남은26함수

| 원본 +5c | VT 수 | 기존 파일의 주소 언급 후보(완료 판정 아님) |
| --- | --- | --- |
| `0041e6a0` | 16 | RawBombRegular.h |
| `0041f760` | 1 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `0041f9d0` | 2 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `00422740` | 1 | RawBridgeEvents.h |
| `0042a750` | 6 | RawCarrierRegular.cpp·RawBalloonBody.h·RawCarrierRegular.h |
| `00448e00` | 1 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `0044a3a0` | 1 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `0044a5e0` | 1 | RawAltarSacrifice.h |
| `0044b150` | 1 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `00450fa0` | 1 | RawDudeRegular.cpp·RawDudeRegular.h |
| `004549c0` | 2 | RawAssetRegular.cpp·RawFactoryUpgrade.h |
| `0045a250` | 1 | RawFlare.h |
| `0045a6e0` | 1 | RawGunImpact.h |
| `0045a8e0` | 1 | RawManaBoltRegular.h |
| `0045e8b0` | 1 | RawWindFlyer.h |
| `00462420` | 2 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `00469540` | 1 | RawDiscRegular.h |
| `004794b0` | 1 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `0048c6e0` | 1 | RawArcSpire.h |
| `0048ea30` | 1 | RawAcidFence.h |
| `00490810` | 1 | RawPlayerRegular.h |
| `00494580` | 1 | RawPriestRegen.cpp·RawPriestFall.h·RawPriestPrayer.h |
| `0049a4e0` | 1 | RawAssetRegular.cpp |
| `004af070` | 1 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `004dc220` | 3 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |
| `004dcf00` | 1 | 직접 언급 없음 — 실제 본문/호출자 추가 조사 |

각 묶음의 정확한 VT 목록과87개 전체 행은 [집계 JSON](../../cpppj/no-world-regular-audit-20261109.json)에 있다. 다음 구현은 다리 `00422740`의 같은 장기 섬/표면/삭제 장치를 우선 조사하고, 이어서 공통 Carrier·다른 고유 Regular를 기능별로 연결한다. 공중3종 전용 동작·희생/MP 사례는 정적/컴파일만 수행한다.

## 다른 내장 Form의 삭제 분배

Storage의 `ProcessFormDispatch()`는 실제 Process(type2/VT00512098)만 직접 등록한다. Depend(type1/00512110)·Gump(type3/00512188)·Content(type6/00512208)는 등록하지 않았고, `SquidFormDispatch::Route()`는 해당 현재 VT 선택 시 명시 오류를 낸다. 현재 World는 별도 같은 풀 분배기에 이들 처리기를 연결한다. 이 세 종은 장기 연결과 실제 NoWorld caller/수명 감사를 진행해야 한다. Regular의 Form3 연결과 삭제 Form3 누락은 서로 다른 가상 동작의 집계이므로 합산하지 않는다.

프로세스 클래스/Kernel·PID·실제 VT별 caller/수명 전체를 이번87개 +5c 조사로 대신하지 않는다. 큰 비영 AI·다른 배치/Fort 상위 Finish·Source/listener/두 표/Poll/태그/무태그/Snapshot/debugBurst·지연큐/JoinTestBattle도 여전히 남는다.

## 검증과 보존

HJOW-Athlon에서 마지막 Client양판본 정밀 디컴파일2026-11-03/R전용2026-11-07과 같은 호스트임을 확인했다. 새 디컴파일/원문 변경0이다. 함수 주소는 보호 PE를 읽기 전용으로 조회했고 게임/MP/패킷/전용 공중3종 검사는 실행하지 않았다. 보조 집계의 최초 함수 범위 검색 오류와 교정은 원시에 보존했다. 제품 코드/검사 기대값은 바꾸지 않았다.

문서/정적 조사 단위이므로 새 빌드·집중·GUI는 수행하지 않았다. 최신 제품1216의313/313 비MP·Tutorial4/Editor 결과를 이번 조사 실행 결과로 올리지 않는다. 제품/고정 소스12·보호2800파일 불변과 문서 링크·집계 합을 별도 확인한다. AGENTS.md·README 개요·원본/CD·DN/WB 인계는 수정하지 않는다.

[현행 인계](../../LEFT_JOBS.md)·[제품1216 근거](cpp-asset-notice-owner-20261109.md), 원시 `extracted/cpp-resume-1217/`의 inventory.py/inventory.json/검증 결과를 따른다.
