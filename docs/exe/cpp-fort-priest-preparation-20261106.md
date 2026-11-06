# 일반 Fort 기본 기술·지역 사제 후처리 — 단위953

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon/2026-11-03이다. 정밀 산출물은 수정하지 않았다. 사용자 정정대로 LEFT_JOBS 3절3번의 앞선 미완료 항목을 이어갔다.

## 변경

952의 실제 비편집 Fort는 File/Player/CoreData/Territory/World/Deck 뒤 `004923e0` 미복원에서 멈췄다. 953은 이 부모와 `00492120`/`004921a0` 목록 helper를 `RawFortPriestPreparation`으로 복원하고 Runtime/Session의 같은 현재 저장소에 연결했다.

- missionPost0에서 기본 TYPE 세 종류(현재54126c/541174/541258)와 현재 subscriber%3의 테마 TYPE를 보장한다. TYPE → 현재 기술 부모 → mask4/type/index0 내용물 종류WORD → 없으면 실제 contained 생성 순서다. 기존 SID 비영 여부로 종류WORD0을 대신하지 않는다.
- 기술 처리 뒤 현재 Editor/missionPost를 다시 읽는다. 현재 Template의 State+0x11이 비영이면 지역 처리를 생략한다.
- 같은 Template의 Territory120BYTE를 읽고 record BYTE1 bit0인 지역20개를 처리한다. 현재 playerOwner/지역으로 찾은 섬이127이면 건너뛴다.
- 현재 사제 목록을 끝까지 재조회해 같은 지역 사제를 최대20개 모은다. free/dead/owner 필터를 추가하지 않는다. 첫 raw+12 bit0x80 사제가 있으면 선택하고, 없으면 첫 사제의 실제 가상74에 현재 owner를 전달한다. 모든 같은 번호를 안정적으로 제거한다. 남은 사제는 실제 가상74로 owner0을 쓰고 양수 개수만 `%d Priest%s` 번역/형식화 후 global.priests에 쓴다. bit80을 '자기 소유 사제'라고 의미 변환하지 않는다.
- 목록이 비면 기존 실제 `RawIslandPopulation::ReplaceInitial`과 배치/Factory/HP/가상owner/Pop을 호출한다. 기존 Editor Drop 장치와 같은 원문 하위를 빌리며 배치 성공을 합성하지 않는다. 원문 Chunk bounds의 DWORD16배→float32 변환, 지역 조회의0041d7a0 snap/저장/절삭 경계를 유지한다.

RawCampaignWorldLoad의 missionPost1 자연 반환은 유지하고0일 때만 실제 부모 장치를 요구한다. 새 장치는 기존 aggregate 필드 뒤에 추가했다. 준비된 일반 Fort의 Editor/post0을1로 바꾸지 않는다. 로컬80BYTE 목록 할당은 RAII로 반납하고 이미 생긴 기술/소유자/설정/객체 효과는 예외 시 rollback하지 않는다. 원문32비트 SEH·힙 실패 자체 동일성은 별도 한계다.

## 독립 원문 대조

새 도구 `tools/decomp_fort_priest_preparation.py`는 보호 PE의004923e0/00492120/004921a0/0040ea00/00470a90 실제 명령만 실행한다. 큰 게임/파일/번역/힙/가상owner 장치는 명시 입력으로 공급한다. 게임 프로세스·창·통신은 실행하지 않는다. 모드/State17/빈record/bit80/첫항목선택/중복/빈목록배치/없는섬/두지역/목록개수변경/처리도중post변경/20칸포화를 포함한13개 입력의 실제 호출 trace와 최종post를 고정했다. 기하·실제 배치까지 모두 PE로 실행한 관측은 아니다.

최초 도구 실행에서 contained의 thiscall 인자와 생성자·좌표·변환 helper의 callee 스택 정리를 순차 교정했다. 앞선 실패 로그를 보존했으며 최초 fixture는 최종13개 전체 관측 성공 뒤에만 생성했다. 생성 후 독립 재관측 `--verify`도13개/차이0이다. 기존967fixture는 수정하지 않았고 새 fixture1개를 추가했다. 이 CPU 관측의 입력/장치/결과 범위를 모든 상태·전체 동일성으로 확대하지 않는다.

## 실제 Session과 회귀

기존952의 중단 확인 검사를 확장했다. 명시 title 재선택 current1/2 각각 실제 파일→CoreData→월드→Deck→기술/사제→CoreWrite/전체snapshot→checkpoint→UI/tail→공통State확정→Session 최종commit/커서 공개를 통과했다. prefix/HUD 한 번과 firstMenu 유지, Editor/post0, 실제 prior/phase/current0/next4를 확인했다. Guide 요청의 실제 Champion/Zone/next7·조기 반환과 잘못된 대여 거부도 유지한다. 이 명시 title 입력은 자연 GUI에서 State3 특이 분기가 도달했다는 증거가 아니다.

CoreData의 정상/불일치 입력도 실제 checkpoint가 정상 복귀한다. 읽은 Core10BYTE/active/seed.low16/로컬 진단은 새 CoreWrite 전에 원문 외부 카메라 호출 위치에서 관측해 후속 재저장과 구별한다. 별도 Native helper 검사2개는 PE13행과 선택된 두 번째 기술 생성 실패의 앞선 효과 보존을 확인한다.

최종 고유 C++13검사/실패0: 새2, 갱신된 continuation4, 기존 prefix3/UI확정1/Guide1/양판본 캠페인 부모2다. 마지막 두 검사는 기존 고정130행(판본별65)을 대조하며 새 CD 비미션 몸체 검사가 아니다. 실제 일반 Fort 정상확정 검사가 사제 중단을 성공으로 바꾸는 대신 실제 부모를 실행한 뒤 완료됨을 확인한다.

최초 제품 빌드는 지역 `finish` 이름 가림 경고1이었다. `localFinish`로 교정해 최종 제품/집중 빌드 경고·오류0이다. 기존952 중단 기대를 그대로 둔 첫 실행은 사제 부모가 정상 복귀하면서4단언이 실패했다. 정상확정 검사로 갱신했다. 새 Native 소비자 첫 실행의2실패는 테스트 Pool 크기가 기존 PATCH predictable 경계보다 작았기 때문이며, 실제 기존30000슬롯 입력으로 교정해 통과했다. 실패를 최종 통과에 합산하지 않았다.

제품953/11.01, EXE `e9c2190a19eb392f776ba533a09decac7a52038283f5b8fdf230a231b3c5f5d0`, 11,279,872바이트. 기존 보호2787/fixture967 불변·새fixture1개→현행2788/fixture968이다. 원시/로그/다음 보호 기준은 `extracted/cpp-resume-953/`와 `protected-final.json`이다. 새 게임/GUI/포획·희생/특정적공격/승리/전용공중3종/멀티플레이 검사는 하지 않았다. 마지막 실제 GUI는952의 편집/Test Battle/복귀/메인 왕복이다. 전체 CTest/34미션/ZIP도 반복하지 않았다.

## CD와 다음 작업

CD 대응0040daf0은0052e170/00518904 관문과 State+17·20×6BYTE/20칸 지역 목록을 사용한다. 기술 보장은0040dab0, 테마 TYPE는00412d20, 목록 helper는0040d860/0040d8f0, 초기 배치는0042cbf0이다. CD raw는36BYTE이고 PATCH는50BYTE이므로 새 실제 Session 연결은 PATCH10.78 전용으로 명시했다. CD의 별도 호출 규약·직렬화/위치 몸체 전체 구현 완료로 쓰지 않는다.

다음954도 같은3절3번의 Native3 다른 선택·State1/단독State7·전투 cleanup의 Tick/연속 전환·LeaveBattle 남은 이유/CD·State6 대조다. State3의 특이 분기는 본문 접합/일반 Fort 정상 확정 코드와 통합 검사가 생겼지만 자연 도달은 아직 미입증이다. 일반 전투/State6의 MP 범위는8번으로 구분하고 테스트하지 않는다. 전체 목표/3번 셋째 체크 항목은 계속 미완료다.
