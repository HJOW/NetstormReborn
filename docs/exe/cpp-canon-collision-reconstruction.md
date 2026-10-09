# 일반 타입 배치의 후보 충돌과 실제 패턴 연결

2026-10-09, **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** AGENTS.md·두 인계 문서·cpppj 내부 문서를 읽고 현재/마지막 호스트 일치를 확인했다. `canoncollision` 목록을 같은 PC에서 읽기 전용으로 내보냈다(10.78 5개·CD/추가 10.37 각각 4개). 원본 게임/복사본·클론 창을 실행하지 않았으며 원본·AGENTS.md·dotnetpj는 변경하지 않았다.

## 복원과 연결

[`RawCanonPlacementCollision`](../../cpppj/src/o/RawCanonPlacementCollision.h)은 사제 genus 제한 없이 현재 후보의 타입/좌표→클라이언트 SID→타입별 무시→로컬 발자국 표시→extra/mode 거부를 처리한다. 기존 [`RawPriestPlacementCollision`](../../cpppj/src/o/RawPriestPlacementCollision.h)은 매 후보의 사제 genus 계약과 기존 모양/후보 순회를 유지하고, 후보 계산 본문만 공통 검사기에 위임한다. 상태는 같은 클라이언트/무시 전역을 공유한다.

| 실제 구간/자료 | 10.78 | CD·추가 10.37 |
|---|---|---|
| 타입별 무시 helper | `0049ade0` | `00444900` |
| 후보 구간 시작 | `0049b9c4` | `004456ad` |
| 허용 후 지형 처리 시작 | `0049bb1a` | `004457f7` |
| 실제 충돌 거부 분기 | `0049bcd1` | `00445ad4` |
| 현재 후보 타입 조회 | `004ac550`→`0049a840` | `004abf50` |
| CRT 절삭 | `004e49c0` | `004f161c` |

무시 함수의 분기 순서·반환 DWORD 마스크·판본 차이와 표시 인덱스 `dx×12+dy+13`은 [기존 사제 후보 기록](cpp-priest-collision-reconstruction.md)과 같다. 무시/후보 본문은 이름/진단 문자열 외에 기존 본문과 같음도 정적으로 확인했다. 일반 자산의 0 발자국은 허용하며 빈 축에서는 표시를 수행하지 않는다. 표시 결과와 실제 충돌 허용을 구별한다.

실제 `RawSquidFinder`의 flag 0/default filter Begin/Next를 사용한다. 거부한 후보 뒤에는 Next나 후보 지형 경계를 호출하지 않는다. 무시한 후보는 지형 경계를 받으며, 다음 후보는 현재 타입/전역과 finder가 미리 저장한 next를 사용한다. 이미 거부한 scan을 외부 정책이 다시 호출하거나 true로 덮어써도 결과는 false다.

`MakeCanonPlacementCollisionHooks`가 공통 미리보기의 후반 경계를 연결한다. `MakeCanonGeometryCollisionHooks`는 **원래 타입/별도 argument/좌표/flags/owner/mode로 지역 정책을 만드는 필수 factory**를 받고 실제 decoder의 각 모양/finder 범위를 연결한다. 정책은 원래 요청을 캡처할 수 있으며 준비→후보 검사→모양 종료→다음 모양→최종 처리 순서를 따른다. 필수 지역/지형 경계를 임의로 허용하지 않는다.

두 판본 합성 검사는 배치 접두→로컬 미리보기→실제 3×3 받침 decoder→실제 해시/finder→후보 검사까지 연결한다. 첫 모양의 후보는 walker 무시 후 지형 효과를 받는다. 그 효과가 무시 전역을 바꾸면 두 번째 모양의 후보가 거부되고 모양 종료/나머지 칸/최종 처리를 생략한다. 준비/후보/종료 호출 순서와 36개 표시를 확인한다. 초기 픽셀 shape는 공급한 경계이며 이 검사를 실제 SHP getter/전체 MayPlace의 원본 실행으로 해석하지 않는다.

## 독립 원본 대조

[`decomp_canoncollision_oracle.py`](../../tools/decomp_canoncollision_oracle.py)는 세 실제 PE 각 **7,560개, 총 22,680개**를 두 x87 정밀도로 실행했다. 일반 genus 9종×후보 genus 7종×문맥 10종×소수/절삭 경계 좌표 6종×무시 전역 2종이다. flags1 4종, 0/1/전체 DWORD mode, SID 4/5/서버 시작 주변, 0 발자국, extra, 비로컬/기존 표시 보존을 분산했다. 기대 표시/거부를 Python/C++ 규칙으로 재계산하지 않는다.

**MayPlace 중간 후보 구간에 필요한 지역 변수와 현재 SID/타입을 공급하고 지형 진입 또는 거부 분기에서 관찰을 끝낸다.** 후보 구간·현재 타입·무시 helper·CRT·표시/거부는 대체 없이 실행한다. 전체 MayPlace ABI/접두/모양/미리보기/finder/지형·Pop/GUI 실행 근거가 아니다. 전체 DWORD helper 단독 호출/ABI 근거는 기존 `priestcollision` fixture를 유지한다.

두 정밀도 합계로 각 PE의 후보 진입/종료·현재 타입 조회는 15,120회, 실제 무시 helper는 12,096회다. CRT는 패치 12,888회·CD/10.37 각각 15,018회다. 현재 raw 전체와 144바이트 표시·허용 코드/쓰기·스택/x87 균형·assert/OS 0회·SHA/행/실제 호출 수를 감사한다. 새 근거는 [`recovery-canoncollision-evidence.json`](../../cpppj/recovery-canoncollision-evidence.json)이며 기존 fixture/감사 도구는 변경하지 않았다.

## 검증과 다음 작업

Release 경고/오류 **0**, CTest 내부 **389개·실패 0**(103.33초), 새 근거/기존 감사 **56종 모두 통과**했다. 새 독립 입력을 더한 누적 x86은 **300,013개**다. 완료 내역과 후속은 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 기록했다. 로그는 `extracted/cpp-canoncollision-export.log`, `cpp-canoncollision-oracle.log`, `cpp-canoncollision-build.log`, `cpp-canoncollision-build-final.log`, `cpp-canoncollision-ctest-final.log`, `cpp-canoncollision-audits.log`다.

다음은 일반 타입/패턴의 후보별 지형·지역 효과와 모양 종료/최종 관계 판정이다. 이어 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사, raw GUI·건설·경제·전투·승패를 진행한다. 여러 판본 실제 자산 전체 합성·변이 전수·창/픽셀·최대 지도 회귀는 계속 인계한다.
