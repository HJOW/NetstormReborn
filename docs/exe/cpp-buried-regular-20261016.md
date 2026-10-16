# 오벨리스크의 실제 Regular 사건 처리

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-16**이며 일치한다. AGENTS.md와 양 인수인계를 다시 읽고 단위81의 실제 연속 Kernel 실패를 이어서 복원했다. 보호 원본·AGENTS·dotnetpj는 변경하지 않았고 게임/GUI를 실행하지 않았다. 목표는 active·미달성이다.

## 실제 이름과 원본 분기

TYPE112의 내부 이름 `buried`는 표시명 **Obelisk/오벨리스크**다([명칭 대응표](../unit-source-name-map.md)). 생성자의 가상 표0050a748+5c는 Handle **004622f0**이다. 최초 사건0/1 예약은 기존 `RawBuriedPostPop`과 같은 Kernel에 이미 연결되어 있다.

- event0: 현재TYPE의 B구간수(+178)에1을 더하고 signed count를 `idiv`한 나머지를 사용한다. 나머지가 있으면 B첫프레임(+138)+나머지-1로 지정한 뒤0.1f를 반환한다. 나머지0이면 A첫프레임(+130)을 지정한 뒤 실제 공유 난수 `004558f0(2,8)`을 한 번 소비하고 정수2..7의 float를 반환한다. unsigned modulo나8을 포함한 난수로 바꾸지 않는다.
- event1: QA getter004acd10→TYPE주소0049a840의 반환값을 버린다. 메모리/효과 변경 없이 기본 Handle00496e70으로 payload를 반환한다. 보상 생성/타입 추첨/피해 처리가 아니다.
- 나머지 사건도 기본 Handle의 payload 반환이다. Damageable6699/6700으로 잘못 분배하지 않는다.

잘못된 TYPE/가상 표/다른 풀/판본은 명시 거부한다. 원본 idiv의0/overflow는 정상 입력으로 고쳐 성공시키지 않는다. 실제 구간 계산기가 만들 수 있는 입력을 사용한다. CD10.72의 후보 대응00416c50은 현재 정밀 표에 있으나 이번 C++ 구현/독립 관찰은 **10.78만** 지원한다.

## 복원과 동일 저장소 연결

`RawBuriedRegular.*`는 현재 Pool·TYPE의 실제 RiftTypeFrames·SquidFrame·GameRandom을 참조한다. 기본 RawAssetRegular에 보상/예약/프레임 사본을 더하지 않았다. `RawCampaignSession`의 실제 Regular 분배기에 합성했고 기존 다른 부모는 기존 처리기로 전달한다. 같은 현재 프레임 지정기는 앞/뒤 표시 갱신과 원본 StoreLevel을 수행한다.

`tools/decomp_buried_regular.py`는 현재 PC의 정밀 함수표에서 Handle·직접 QA/TYPE getter·공유 난수·기본 Handle의 **6개 실제 명령 범위**를 선택한다. 프레임 지정은 명시 외부 경계이며 원본 호출의 앞/뒤 표시와 frame DWORD 쓰기를 기록한다. C++ 검사는 실제 SquidFrame으로 이 순서를 대조한다. 입력288개는 signed횟수8×B구간수3×난수3×현재프레임2×효과 후 변경2다. 기본 사건96개는 event4×count2×QA3×payload4다. 총 **384고유 입력**, 두 x87 정밀도 **768정상 반환**이며 비휘발 레지스터/정밀도/스택 복원을 매번 확인한다. 게임/OS/창 진입은 없다.

Frame 효과 뒤 난수/QA를 바꾸는 명시 입력도 포함해 현재 난수 재조회와 앞선 raw 변경을 보존했다. 반환 float 비트·전체 부모50바이트·최종 난수·모든 표시 순서가 C++의 집중2개에서 전수 일치한다. 연결 거부/fallback 검사1개도 통과했다. 최초 검사의 SID128 크기는 원본의 고정 예측 free list를 포함하지 못해 거부됐다. 올바른24000 크기의 합성 풀로 고쳤고 원본 관찰/기대값은 임의 변경하지 않았다.

## 실제 미션 진행과 남은 경계

Early Missions5(Tutorial5)는 오벨리스크를 지나 **400프레임/10초**를 통과했다. 실제11미션 중7개(Tutorial1~5·TheWarBegins·MasterOfWhirligigs)가 동일 Kernel 연속 검사에 성공한다. 나머지4개는 아래에서 실패하며 성공 예외/건너뜀으로 바꾸지 않는다.

| 실제 입력 | 현재 첫 미복원 경계 |
| --- | --- |
| Tutorial6 / GuardMyBack |1프레임: TYPE136 Sun Barricade event0 |
| DemoStormRages |103프레임/2.575초: 생성 rainFlyer(TYPE144) 파생 Pop |
| DissolvedAlliance |오벨리스크 실패를 지나3프레임/0.075초: Archer 외 다른 실제 총 부모의 animation 미연결 |

이 검사는 Guide/전체 GUI/AI전략/사용자 명령/승패/저장/다음 미션까지 포함한 자연 완주가 아니다. 원본 그래픽·음향의 전체 동일성도 아직 미검증이다. DirectDraw 오류 수정은 cpppj2차 목표의 필수 항목으로 유지하며 이번1차 미션 복원에서는 제외한다.

출처/고유 입력 감사는 `cpppj/buried-regular.json`, 로그는 `extracted/goal-buried-20261016-{native,build,final-build,focused,focused-final,frames,ctest}.log`다. CMake 추가 등록의 이전/현재 SHA를 요구에너지 이력에 보존하여 기존2352입력 감사도 유지한다.

최종 Release 경고0·오류0, 집중3개/384행 실패0이다. 전체 CTest는 **1356개 중1352통과·4실패**, 검사116.09초/전체116.16초이며 위의 연속 미션4개만 실패한다. 기존1340개·추가 거주 구역2개·새 오벨리스크3개는 모두 통과했다. 새384/거주 구역1728/요구에너지2352 SHA와 Python구문·UTF-8·링크·공백을 확인했다.
