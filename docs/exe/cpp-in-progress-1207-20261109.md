# 1207 아이스 타워 — 최초 실패 인계(해소됨)

> **1207 마감:** 아래는 문서 정리 1208 당시의 미완료 기록이다. 이후 실제 입력을 교정해 194/194와 허용 GUI 회귀를 통과하고 구현을 마감했다. 현재 결과와 남은 범위는 [1207 마감 문서](cpp-ice-owner-20261109.md)·[현행 인계](../../LEFT_JOBS.md)를 따른다. 아래의 “현재·다음·미완료”는 당시 표현이며 현행 작업 목록으로 사용하지 않는다.

# 1207 아이스 타워 장기 연결 — 미완료 인계

2026-11-09 / HJOW-Athlon. 확정 제품은 `f0d065dc / 20261109-86 CP`의 1206이다. **1207은 미커밋·집중 검사 3실패로 완료하지 않았다.** 문서 정리 1208에서 현재 변경·실패를 보존했다. 전체 목표와 우선순위는 [LEFT_JOBS.md](../../LEFT_JOBS.md)가 기준이다.

## 현재 결과

- Release/x64 제품 2·집중 5대상, 4병렬 빌드 **55994 종료 0**, 109.16초, 경고·오류 0. 빌드 중 소스 11파일 불변이다.
- 비MP 집중 검사 **55389 종료 1**, **194건 중 191통과·3실패**, 37.72초. 기존 원문 1,426행 IceTower fixture도 포함한다. 새 GUI 검증은 미실행이다.
- 실행한 빌드·집중 검사 핸들은 종료했다. MP·원본 게임·디컴파일을 새로 실행하지 않았다.
- 현재 `cpppj/build/bin/Release/Netstorm.exe`는 1207 작업 빌드다. 1206에서 실제 GUI 확인한 바이너리와 구별한다. 현재 산출물 SHA는 [이번 문서 정리 보고서](cpp-handoff-review-20261109.json)의 `productsPreserved`에 있다.

## 변경 파일 11개

| 파일 | 작업 중인 내용 |
| --- | --- |
| `cpppj/src/o/RawIceTower.h`, `RawIceTower.cpp` | 3역할의 현재 DWORD getter를 통해 같은 실제 TYPE 역할을 읽는다. 독립 fixture의 기존 값도 유지한다. |
| `cpppj/src/client/RawSurfaceWorld.h` | 장기 IceTower 역할·본체와 공개 getter를 보유한다. |
| `cpppj/src/client/RawSurfaceIce.cpp`(신규) | 같은 Pool/Frame/Host·실제 생성/Owner/Pop/삭제·공통/피해 Post를 연결한다. 큰 섬 드롭은 기존 실제 장치를 선택한다. |
| `cpppj/src/client/RawSurfaceStorage.cpp` | 성장형 PostPop/Regular 기본 분배를 연결한다. |
| `cpppj/src/client/RawSurfaceAssetDeletion.cpp` | 기존 아이스 타워 3VT와 성장형 1VT의 Post 부모를 연결한다. |
| `cpppj/src/o/SquidPostPop.cpp` | 성장형 VT의 실제 기본 분배를 선택한다. |
| `cpppj/src/client/RawCampaignSession.cpp` | Native Session은 장기 본체/역할을 빌린다. typed 경로의 서버 조건과 가상 Owner도 원문에 맞춘다. |
| `cpppj/src/client/CMakeLists.txt`, `cpppj/tests/CMakeLists.txt` | 신규 본체와 기존 IceTower fixture를 관련 대상에 등록한다. |
| `cpppj/tests/ClientRegionOwnerTests.cpp` | 같은 실제 장기 장치 위 신규 검사 10개를 추가했다. 아래 3개가 실패했다. |

파일별 현재 SHA는 정리 보고서 `inProgress.sourceSha256`에 있다. 이 소스는 이번 문서 커밋에 포함하지 않는다.

## 보존한 실패와 바로 할 일

원시 경로는 저장소의 `extracted/cpp-resume-1207/`다. `tests.json`과 해당 `test-이름.log`를 보존하고, 다음 실행 전에 별도 최초 실패 이름으로 복사한다. 아래는 **실패한 기대**이며 실제 원인 판정은 아직 하지 않았다.

| 실패 검사 / 위치 | 기대와 조사 출발점 |
| --- | --- |
| `ClientIce_GrowingDeletePreservesSelectedDropFailure` / 검사 파일 270행 | 실제 받침을 찾은 뒤 선택된 큰 섬 드롭의 금지 예외·부분 삭제 상태를 기대했으나 복합 단언이 실패했다. `GenusAt(0x1000000)`의 실제 검색 단계·hash/Pop 입력부터 대조한다. |
| `ClientIce_RegrowthCreatesCurrentRoleBeforeExternalFailure` / 281행 | 현재 성장형 TYPE70 생성·WORD/QA/Owner 복사 뒤 금지된 Native 방송을 기대했으나 선택 예외/새 객체 수/깊이 조건이 실패했다. 현재 역할·surface getter·extra/flags·서버 조건·가상 Unpop 순서를 대조한다. |
| `ClientIce_RegrowthUsesVirtualOwnerOfCurrentRole` / 295행 | 현재 성장형 역할을 신전 TYPE124로 바꿨을 때 가상 Owner가 실제 Player의 신전을 등록해야 하나 SID가 없었다. 생성 자체와 가상+74 도달 여부를 먼저 구별한다. |

1. AGENTS.md·현행 인계·DN 인계를 다시 읽고, 11파일과 원시 해시를 대조한다.
2. 원문/기계어와 실제 입력으로 3실패를 진단한다. 금지 통신·큰 드롭을 가짜 성공으로 대체하거나 실패 기대를 약화하지 않는다. `RawSurfaceIce.cpp`의 주소 주석 `48ec4f`는 실제 `48ec34`로 교정할 오타다.
3. 소스 교정 후 같은 4병렬 빌드의 단일 핸들을 종료까지 확인하고 관련 비MP 집중 검사를 수행한다. 빌드 중 소스 변경은 하지 않는다.
4. 허용되는 Tutorial4 단순 생산/건설·대기와 Edit 왕복을 해당 최종 EXE로 확인한다. 공격/포획/희생/승리·공중 3종 전용 관측·MP 실행 검사는 제외한다.
5. 실제 결과·원시·보호 해시를 남기고 인계/단위 보고서 작성 뒤 1207 구현 파일을 로컬 커밋한다. 그 뒤 썬 펜스의 이웃/표식/연결선/재생성/수명을 함께 장기화한다.

## 원문 기준과 완료로 세지 않을 범위

- 마지막 정밀 디컴파일: Client 10.78/10.72 **2026-11-03 / HJOW-Athlon**, R **2026-11-07 / 같은 호스트**. 현재도 같은 호스트며 이번 정리에서 디컴파일 소스 변경은 없다.
- 아이스 본문 `48ead0/48edf0/48eb80/48ebe0`와 하위 `40eaf0/496e80/4b0d30`, CD 대응 2개 등 9함수의 C/PE 범위 증거는 `original-evidence.json`에 있다.
- 재성장은 **서버 `00540bc0`** 조건이다. 전투 모드 `00594fbc`와 구별한다. 생성 자식의 Owner는 실제 가상+74(`48eec5/48ec34`)를 호출한다. 이 두 지점은 첫 빌드 후 원문 대조로 교정했으나 최종 검증은 미완료다.
- 예상 부모 분배 70/83·잔여 13은 현재 변경을 포함한 예정 수다. **확정값은 1206의 66/83·잔여 17**, 통지 잔여 6이다. 큰 섬 드롭의 실제 Runtime 의존, 큰 건설 효과 16개·Client 장치 10개는 그대로다.
- 썬 펜스·큰 비영 AI·다른 Form/Regular/호출자·M6-l/m·M7/M8·K/S/B는 완료하지 않았다. 특히 이번 아이스 연결을 **NB33 원인 확인/수정 완료**로 표시하지 않는다.
