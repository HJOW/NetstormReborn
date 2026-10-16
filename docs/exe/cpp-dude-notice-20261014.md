# Dude 주변 통지와 대피 요청 복원

- 작업·마지막 정밀 디컴파일 PC: **HJOW-Athlon, 2026-10-14**.
- 목표 판본은 10.78이며 CD 10.72의 인라인 통지·필드 폭·호출 인자 차이를 함께 기록한다.
- 현재 독립 원본 관찰은 **414정상, assert0**다. 두 판본 각각207이다. x발자국/정수폭 교정 뒤 **C++3검사 전부 성공**했으며 최종 공유 Release 경고/오류0·집중고유34·전체1,038검사 실패0이다.
- 보호 PE는 읽기 전용 Unicorn 메모리에 매핑했다. 원본 게임·운영체제 호출·별도 창 실행은 없었다.

## 몸체와 순서

| 기능 | 10.78 | CD 10.72 | 복원 범위 |
|---|---|---|---|
| 주변 통지 | `00451470` | `004e8e70` | 좌표−1 거의올림, ±4 사각형의 실제 finder와 genus400 필터 |
| 피해 통지 | `00450db0` | 주변 몸체에 인라인 | dead/task5107/task5105 자연반환, 현재 Regular/task, Burning, 대피 순서 |
| task 변경 | `00450670` | 주변 몸체에 인라인 | event5107·delay0 소비 경로, 기존 count0/event 재쓰기 또는 payload5 생성 |
| 같은 owner 거주 구역 선택 | `0048fd30` | `00406ef0` | active DWORD 목록 순서, 엄격한 거리 비교, best float 저장 |
| 거주 구역 진입점 | `004505a0` | `004e8a30` | x−footX×0.5 절삭+0.5, y−footY, 실제 mask `50444200` 및 genus 보정 |
| 대피 요청 | `00450760` | 주변 몸체에 인라인 | 기존 type24 endpoint 비교, speed2배, 경로 요청, speed 정수절삭 복원, I→A frame |

현재 `RawDudeNotice`는 같은 SID 풀·해시·타입·프레임·mutable speed·거주 구역 목록·Host 시계를 참조한다. `MakeDudeNoticeHostHooks`는 현재 실제 Host의 masked Regular 검색과 AddRegular를 연결한다. 기존 Regular가 있으면 같은 객체의 payload/Form/부착을 유지하고 `ResetTask(5107, now)`로 event/count/time만 바꾼다.

패치 task 시각 대입은 decompiler가 표시한 `float(now)`가 아니다. 실제 `00450726 fld float(delay) → 0045072b fadd double[0055b4d0] → 00450731 fstp double`을 따르므로 delay0인 현재 경로는 정밀 double now를 보존한다. CD는 `005484a8`의 두 DWORD를 그대로 복사한다. 일반 task5103/5104와 nonzero delay는 현재 이 모듈의 완료 범위가 아니다.

## type14와 type24의 실제 의미

**type14는 Burning 효과 프로세스다.** `RiftTypeTable.cpp`의 내장 이름은 `burningSystemProcessType`이며 `00423b20/CD004a5a50` 생성자와 후속 몸체의 원본 문자열도 `Burning.cpp`다. 주변 통지는 `00424030/CD004a66a0` wrapper를 통해 같은 생성자를 **모드0**으로 부른다. 기존 `RawDamageableBurning`의 저HP 효과는 `00424090/CD004a6720` wrapper의 **모드1**이다.

**type24가 실제 dudeProcessType 이동 파생 클래스다.** 현재 통지는 기존 type24의 마지막 endpoint를 조회하거나 PathStart 주소 `00489080/CD0047d150`으로 새 이동을 요청한다. 기존 `RawPathStart`는 부모158/134와 프로세스54/23을 지원하며 현재 Dude70/type24 생성·Init·부모 통지를 지원하지 않는다. 두 경로를 같은 것으로 간주하지 않는다.

새 공개 경계 이름은 `burningProcessType`과 `createBurning`이다. `createBurning`은 같은 Burning 생성자의 모드0을 구현해야 한다. 아직 실제 생성 몸체를 복원하지 않았으므로 누락 시 원본 호출 지점에서 예외를 낸다. type24 경로 생성과 SetFrame도 필요한 호출 시점에서 누락을 거부한다.

## 독립 원본 격자와 관찰 경계

각 판본207행은 Around199, nearest4, 진입점4다. task5101/5104/5105/5107, dead0/2, 기존 Regular/Burning 유무, 거주 구역 없음/같은 owner/다른 owner/둘, 기존 경로 없음/endpoint25·22/endpoint100·100을 교차한다. 추가 입력은 speed2/2.75/−2.75, 현재 A/I frame, 경로 성공/실패 및 double now10/16777217.125다. 중복 기본 입력은 제거했다.

최초 초안은 path1을 같은 endpoint라고 이름 붙였으나 실제 x발자국 보정 뒤 진입점은23.5/21.5, Snap은24/22다. **기존 두 endpoint 입력 모두 실제 목적지와 다르며 같은 endpoint 자연반환의 원본 관찰은 이414행에 없다.** 해당 비교 몸체는 구현했지만 그 분기를 별도 신규 원본 격자로 대조하는 작업은 남았다. 도구와 fixture의 기존 입력/출력은 바꾸지 않았다.

원본 finder·typed 체인·genus·Regular 생성자·task WORD setter·거리·스냅·진입점·속도 대입·현재 단계 조회는 보호 PE 몸체를 실행한다. heap, masked Regular 검색, Form 부착, Burning 모드0 ctor, type24 PathStart, 기존 경로 객체와 SetFrame은 명시 관찰 경계다. 반환된 번호/contained 링크는 해당 경계의 입력 효과이며 실제 Burning/경로 생성 완료로 세지 않는다. 현재 두 생성 몸체 완료 수는 각각 **0**이다.

C++ fixture 장면은 같은 실제 Kernel/Host의 Regular를 사용한다. 기존 count7/time27은 기대값 직접 대입 없이 실제 Regular7회 실행으로 준비한다. 생성 번호83/84, 기존 type14/24 Form, A/I 코드와 거주 구역 순서는 native와 같은 명시 입력이다. 실제 protected Dude 자산 전체·Burning 시각 효과·경제 작업·전체 경로 이동·GunImpact Ground의 월드 설치는 아직 미완료다.

## 자료와 현재 검사 상태

- 구현: `cpppj/src/o/RawDudeNotice.h`, `.cpp`.
- 새 C++3검사: 두 판본 전수 재생과 `DudeNotice_MissingBurningAndPathFactoriesRejected`. 등록 macro는 `NETSTORM_DUDENOTICE_FIXTURE`다.
- native 도구: `tools/decomp_dudenotice_oracle.py`; fixture `cpppj/tests/fixtures/dudenotice-x86.tsv`; 보고서 `cpppj/dudenotice.json`.
- 현재 SHA·정상반환·고유입력 감사는 `python -X utf8 tools/decomp_dudenotice_oracle.py --verify`다.
- 최초6전체 자료는 `cpppj/hist/dudenotice-initial`에 보존했다. 처음 붙인 잘못된 Dude 작업자 명칭을 Burning으로 교정했고 fixture 주석 외 **414개 입력/출력행 전부 동일**이다. 전체 fixture 원문 SHA와 관찰행 불변을 각각 구분한다.
- `tools/analyze_dudenotice_draft_lineage.py`와 `cpppj/dudenotice-draft-lineage.json`이 정확한 이름 교정·현재 전체 SHA·이전 전체보고서 연결을 감사한다. 이 교정의 새 native/고유입력은 **0**이다.
- 첫 전수 실패에서 원본23.5와C++24.5의 차이를 확인했다. decompiler가 생략한 실제 `fild footX→fmul0.5→fsubr x→ftol`(패치004505e2/CD004e8a75)을 복원하고 검사 tuple 입력의 uint16/uint8 폭만 명시해 MSVC C4244를 제거했다. 변경 전 전체 source/test/414fixture/report/tool·모든 기존 lineage·빌드/실패 로그를 **50파일**로 `dudenotice-b-7ab49`에 보존했다. 원본414행/도구는 전부 불변이다.
- `tools/analyze_dudenotice_point_lineage.py`와 `dudenotice-point-lineage.json`은 두 최소교정의 정확한 역변환·전체 이전보고서/본문/현재SHA를 감사한다. 현재 draft schema2가 초기 이름교정과 이 교정을 함께 연결하며 새 native/고유입력은 **0**이다.
- 공유 ResetTask는 기준 `a349e63`의 Git LF 및 작업 CRLF 원문·기존 관찰을 별도 보존한 `dude-regular-reset-lineage.json`에 연결한다. 기존 완료 Regular/총/효과 보고서는 덮어쓰지 않았다.

현재 전체 native414와 두 SHA 감사는 성공했다. 최초12checks 차이를 위처럼 수정한 뒤 C++3검사 전부 성공했고, 단위11 전체15감사/공통현재SHA 감사도 통과했다. 최종 공유 Release 경고/오류0·집중고유34·전체1,038검사 실패0이며 상세LEFT 갱신 뒤 root가 커밋한다. 같은endpoint 자연반환·Burning 실제생성·type24 이동·캠페인 전체 완료는 이성공의 범위가 아니다.
