# 일반 타입·패턴 배치의 접두와 실제 픽셀 getter 연결

2026-10-09, **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** AGENTS.md와 두 LEFT_JOBS/내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. 새 `canonplacement` 목록(10.78 3개·CD/추가 10.37 각각 4개)을 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창 실행, 원본 파일·AGENTS.md·dotnetpj 변경, 커밋/푸시는 하지 않았다.

## 원본 주소와 인자

| 항목 | 10.78 | CD·추가 10.37 |
|---|---|---|
| MayPlace 진입 | `0049b510` | `00445200` |
| 접두 끝/후반 경계 | `0049b661` | `0044537c` |
| 픽셀 모양 조회 | `0043cbd0` | `004ed7c0` |
| 실제 CRT 절삭 | `004e49c0` | `004f161c` |
| 타입 번호 조회 | this 포인터 차 / 500 | `004448e0` |
| 강제 허용 DWORD | `0055a488` | `00537e98` |
| 로컬 플레이어 DWORD | `00540c70` | `0050f6c8` |
| 차단 관계 DWORD | `0059ab2c` | `0051cbfc` |

this는 타입 레코드다. 여섯 스택 인자는 `(argument,x,y,flags,owner,mode)`이며 `argument`는 this의 타입 번호와 별개다. 기존 사제 경로는 두 번호를 같은 값으로 전달했지만, 다리·섬·공유 전투 패턴은 자체 패턴 번호를 전달해야 한다. 모양 조회 인자는 `(out,type,argument,flags,outAnchorX,outAnchorY,0)`이다. 마지막 0은 비패턴의 명시 프레임 선택을 끈다.

## C++ 연결과 계약

[`RawCanonPlacement`](../../cpppj/src/o/RawCanonPlacement.h)는 유효 자산 타입(70부터)의 접두를 공유한다. 원본처럼 차단 관계를 먼저 0으로 지우고, 강제 허용 DWORD 또는 현재 genus의 `0x02000000`이면 모양/후반 처리를 생략한다. 그렇지 않으면 owner의 low BYTE를 부호 확장해 전체 로컬 DWORD와 비교한 결과를 **모양 조회 전에 캡처**한다. 별도 argument/flags로 픽셀 모양을 읽고, 여백과 네 지도 경계를 검사한 뒤 후반 정책에 값 인자와 캡처한 비교 결과를 전달한다. 모양 조회 중 전역/타입/호출자 요청이 바뀌어도 원본 스택 값과 이미 캡처한 비교를 유지한다.

여백은 가로/세로 픽셀 크기를 각각 1/16·원본 float 1/11로 환산한 큰 값의 CRT 절삭이다. 패치는 가로 차를 넓은 정밀도로 유지하고 세로만 float로 저장한다. CD는 두 합을 float로 저장한다. `(left=2^-20,right=32,top=bottom=0)`에서 여백이 패치 1·CD 2가 되는 차이와 빈 모양 `(1000,1000,0,0)`의 음수 여백을 보존한다. 음수 여백을 0으로 보정하지 않는다. 비유한 좌표/모양·잘못된 타입·C++ 정수 범위 밖 변환은 C++ 계약 오류로 진단한다.

`MakeCanonShapePlacementHooks`가 [전체 일반/패턴 픽셀 getter](cpp-canon-shape-reconstruction.md)를 연결한다. 후반 미리보기/충돌/지형/관계 판정은 반드시 별도 경계로 공급하며 누락하면 구성 오류다. 접두 통과가 실제 배치 허용이나 전체 MayPlace 복원을 뜻하지 않는다.

기존 `RawPriestPlacement`는 사제 genus와 기존 인자 계약을 유지하고 공통 접두로 위임한다. 사제 생성·미리보기·후보·지형·전체 모양 순회에 대한 기존 검사도 함께 재생한다. 공통 접두는 기존 `PriestPlacementState`/사각형 자료를 공유하며 상태를 별도 복사하지 않는다.

## 독립 원본 관찰

[`decomp_canonplacement_oracle.py`](../../tools/decomp_canonplacement_oracle.py)는 세 PE 각각 **1,120개**, 총 **3,360개**를 두 x87 정밀도로 실행한다. 모양 10종×좌표 16종×호출자/전역 7종에 별도 argument(0·1·25·67·0xffffffff·158), 사제/비사제 genus, 상위 owner 비트·음수 BYTE·전체 로컬 DWORD, 조기 허용·후반 거부/허용·호출 중 전역 변화를 공급한다. 기대값은 실제 명령 관찰에서만 저장한다.

실제 진입은 판본당 두 정밀도 합계 2,240회, CRT는 1,600회다. CD/10.37 타입 번호 조회도 1,600회다. 모양 함수 진입은 1,600회 대체한다. 후반 중간 구간 대체는 패치 890회, CD/10.37 860회이며 `0049b661`→`0049bfbb`/거부 반환, CD `0044537c`→`00445ce2`/거부 반환으로 이동한다. **해당 후반 구간은 실제 실행한 것이 아니다.** 초기화·즉시 허용·타입 번호·여백·지도 조건·보존 레지스터·ESP/x87 복구·정상 반환은 실제 명령이다. 허용 코드/쓰기·assert/OS 0회·SHA·행/실제/대체 호출 수를 감사한다.

새 fixture와 SHA 근거는 기존 사제 접두 fixture/감사와 분리했다. 누적 독립 fixture는 **272,473개 = 269,113 + 3,360**이다.

## 실제 TYPE/SHP 연결 검사

새 콘솔 명령은 `NetstormCpp.exe --inspect-canon-placement originals`이며 CD는 `originalCD --cd`를 사용한다. 기존 전체 자산/패턴 열거를 재사용해 각 조합에서 여덟 좌표/호출자/후반 반환 프로필을 검사한다. 실제 모양 getter를 접두에 연결하고 반환·후반 진입·캡처한 로컬 비교·차단 관계를 출력한다.

[`cpp_canonplacement_smoke.py`](../../tools/cpp_canonplacement_smoke.py)는 TYPE 코드/defaultFrame/기준점과 SHP 물리 헤더를 독립 파싱한다. 원본 전체 getter를 두 정밀도로 실행한 뒤 그 실제 출력만 접두의 모양 함수 진입에 공급한다. **getter와 접두는 별도 원본 실행이며, 접두의 모양/후반 경계는 명시 대체**다. 접두의 즉시 허용 비트만 TYPE의 `not_real`/`notreal`에서 공급하며 다른 genus 비트는 접두 분기에 영향이 없다. 원본 자산 로더나 전체 MayPlace/GUI/게임을 실행한 근거가 아니다.

10.78 **116개 자산·512개 조합·4,096개 관찰**, CD **101개 자산·497개 조합·3,976개 관찰**, 총 **8,072개**가 일치했다. 결과는 Git 제외 `extracted/cpp-canonplacement-assets-report.json`이다.

## 최종 검증과 다음 범위

Release 경고/오류 **0**, CTest 내부 **377개·실패 0**(108.26초), 기존 근거 포함 감사 **54종 모두 통과**다. 새 C++ 검사 5개는 세 PE fixture 재생, 일반 타입/입력 캡처/조기 허용/오류 계약, 실제 복원 다리 패턴·SHP→접두 연결이다. 로그는 `extracted/cpp-canonplacement-export.log`, `cpp-canonplacement-oracle.log`, `cpp-canonplacement-build-final.log`, `cpp-canonplacement-ctest-final.log`, `cpp-canonplacement-audits-final.log`, `cpp-canonplacement-assets-final.log`다.

다음은 일반 타입의 미리보기/충돌 정책·지형/지역/관계 판정이다. 이어 일반 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사·비표면 raw GUI·건설·경제·전투·승패를 복원한다. 실제 플레이의 현재 범위는 [복원 계획](../cpp-playable-plan.md), 최신 인계는 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다. 30분 작업 범위와 장시간 변이/최대 지도/창 회귀 인계를 유지한다.
