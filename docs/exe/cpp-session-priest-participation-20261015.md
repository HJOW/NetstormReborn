# 실제 Session의 사제 참여 조회와 기존 상태 연결 — 2026-10-15

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. AGENTS와 양 LEFT를 확인했다. 원본 게임/GUI/OS를 실행하지 않고 보호 PE/자산을 읽었다. AGENTS·보호 원본·dotnetpj는 변경하지 않았으며 CMake·공용 빌드·LEFT·커밋은 통합 담당자가 맡는다.

`RawCampaignSession`은 첫 저장 객체 생성 전에 `RawPriestParticipation`을 하나 소유한다. 같은 `priests->Carrier()`와 `priests->Priests()`를 사용하므로 사제 PostPop/PreDestroy·기존 live 조회·MissionOutcome과 **동일한 목록/ownerState 저장소**를 읽고 쓴다. 풀/해시/TYPE은 단일 `core`의 참조다. 목록·현재 상태·포획 bool의 사본을 만들지 않는다.

현재 사제 타입은 기존 `priestState.movement.priest`를 직접 참조한다. Session에는 Altar/Dais의 장기 전역이 없었으므로 기존 원본 `RunePostPopState`를 소유해 같은 `altarType/daisType`을 공급한다. 보호10.78 PE의 실제 DWORD 초기값을 직접 확인했다.

| 원본 전역 | 실제 초기값 | Session 공급 |
|---|---:|---|
| `005412d0` |158| 기존 `priestState.movement.priest` |
| `005412e8` |164| 같은 `runeState.altarType` |
| `005412ec` |165| 같은 `runeState.daisType` |
| `005412f0` |166| 같은 `runeState.runeType`; 이번 참여 조회는 읽지 않음 |

이 상태는 후속 실제 Rune/Altar/Dais 모듈에서도 재사용해야 한다. Dais를 Rune166으로 잘못 공급하지 않는다. 전체 AltarSacrifice가 아직 설치되지 않았으므로 참여 모듈의 실제 `RawSquidFinder` typed 경로를 사용한다. 직접 조합은 [원문·native 상세](cpp-priest-participation-20261015.md)를 따른다.

공개 API는 다음 세 개다.

- `FindPriestParticipation(owner) const`: 현재 live→Carrier→Altar fallback을 읽으며 결과 상태를 쓰지 않는다.
- `RefreshPriestParticipation(owner)`: 같은 조회 뒤 기존 ownerState의1..8만0/1/2로 갱신한다.
- `PriestParticipation() const`: 동일 풀·동일 사제 상태의 실제 모듈을 읽기 전용으로 빌린다.

constructor/Load/RunFrame은 `Refresh`를 호출하지 않는다. 기존 실제 사제 PostPop이 해당 소유자의0을 쓰는 동작은 그대로 수행한다. 원본 로그인 종료 타이머의 `004b67c0` 소비자가 명시 Refresh 위치를 결정해야 한다. 이번 API 설치만으로 로그인 종료·Guide·Stage3나 자연 미션 진행을 완료했다고 계산하지 않는다.

## 보호 실제 월드 검사

`CampaignSessionTests.cpp`에2개 검사를 추가했다. 보호 `d/thewarbegins.fort`·188TYPE·원본 프레임과 실제 Session을 사용한다.

첫 검사는 첫 객체 전 같은 풀/사제 상태의 주소, 기존 live 조회와 같은 실제 내 사제 SID, 존재하지 않는 owner8의 Load 전 표식 보존, 명시 조회/정지 프레임의 상태 불변, 명시 Refresh의0/2 기록과 owner0/9/`0xffffffff`의 기존 상태 보존을 검사한다. 외부 상태 사본이나 alive bool로 조건을 바꾸지 않는다.

둘째 검사는 실제 `SquidFactory`가 생성한 sunWalker133/ContentForm6/Altar164/Dais165를 같은 풀에 둔다. 포획/typed 조회를 위한 완성된 raw 연결과 해시 등록을 **명시 입력으로** 공급한다. 실제 capture 트랜잭션이나 Altar Pop 전체를 수행한 것으로 계산하지 않는다. 조회는 같은 실제 Carrier/Contained/4단계 finder를 실행한다. 첫 일치 종속의 원소유자 불일치에서 같은 Carrier의 두 번째 종속을 고르지 않는 동작, BYTE136의 낮은7비트8, Carrier 즉시 반환, 마지막 Altar, 죽은 Dais 제외, Altar WORD264와 player8의 불일치, 기존 ownerState1/2를 확인한다. TYPE genus/발자국/생성자 VT를 검사 입력으로 수정하지 않는다.

최초 집중 검사에서 새 독립3개와 기존 Lookup8/Altar1 및 다른 회귀는 통과했지만 실제 Session17개 중 발자국 복합 CHECK1개가 실패했다. `extracted/goal-guide-participation-focused.log`를 그대로 보존했다. 독립 Altar 관찰의 합성3×3을 보호 자산에도 적용한 검사 전제가 원인이었다. 보호 `netstorm.tarc`의 `d/dais.type`과 `d/altar.type`을 다시 읽어 **둘 다 실제7×7**임을 확인했고, 제품/TYPE를 바꾸지 않고7×7 검사와 Carrier 우선순위 검사를 각각 독립 CHECK로 교정했다. 초기 로그에는 이름이 다른 Carrier 필터로0개 선택한 실패도 있으므로 최종 회귀의 `carrier_predestroy_` 실행과 구분한다. 첫 빌드의 BYTE 마스크 signed/unsigned 경고1개는 명시 `uint32_t`와 `0x7fU`로 제품의 의미를 보존해 교정했다.

## 출처와 검증 범위

`cpppj/priest-participation.json`은 새304입력의 실제10.78 원본 관찰 출처다. 새 native 입력을 이전 live lookup 관찰과 중복 합산하지 않는다. `cpppj/session-priest-participation.json`은 이번 실제 Session 소스와 재사용 fixture/하위 모듈의 현재 SHA 및 통합 검사 결과를 별도로 기록한다.

기존 `session-ai.json`의 소스는 **c9bc0f4 당시 역사 스냅샷**, `session-construction.json`은 **81fae7c 당시 단위65 역사 스냅샷**, `client-ai-chat.json`은 **fb1b290 당시 역사 스냅샷**으로 해석한다. 그 문서의 과거 검증 결과는 보존하며 이번 Session 소스와 같다고 주장하지 않는다. 이번 현재 Session SHA는 새 보고서를 따른다.

현재 원본 관찰은304개 고유 입력·두 x87 정밀도·1,216정상 반환·실제19개 전체 함수·대체0/OS0/GUI0이며 `--verify` 종료0이다. 최종 Release는 **경고0·오류0**, 새 독립3개와 실제 Session2개를 포함한 **집중60개 실패0·전체 CTest1/1·내부1,283개 실패0·216.33초**다. Guide4·사제참여3·Session17·후반튜토리얼2·로그인3·타이머5·Lookup8·Carrier4·Altar1·Chat5·로컬전송3·Player참여2·요구에너지3을 같은 최종 소스로 실행했다. `extracted/goal-mission-entry-final-{build,focused}.log`, `goal-mission-entry-{ctest,tests}.log`를 보존했다.
