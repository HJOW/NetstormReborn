# 일반 Dude 작업 설정 복원

## 출처와 현재 범위

- 주 복원 대상은 패치 **10.78**, 비교 대상은 CD **10.72**다.
- 마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 현재 PC와 일치한다.
- `dude-task-functions.json`의 양판본 각3개 함수를 같은 정밀 프로젝트에서 읽기 전용으로 내보냈다. 보호 PE와 기존 Notice414 자료는 수정하지 않았다.
- 새 독립 원본 관찰은 **780개: 정상684/assert96**다. 패치는 정상288/assert96, CD는 정상396/assert0이다. 원본 assert 도달은 해당 호출에서 중단한 진단 관찰이며 정상 반환이나 전체 예외 처리 완료로 세지 않는다.
- C++의 새3검사는 준비했으며 **공유 빌드/대조는 아직 미수행**이다. 이 자료는 Burning 생성자256·입자564 및 Notice414와 별도 입력이다.

## 실제 몸체

| 역할 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 일반 task 설정 | `00450670`, thiscall 두 인자 | `004e8ba0`, cdecl SID/event/delay |
| Regular 생성 | `00496e80` | `0048efd0` |
| task 하위 WORD 쓰기 | `004ac220` | `004abb00` |
| 현재 Regular 조회 경계 | `004afbe0` | `004ac9c0` |
| 다리 표면 통지 경계 | `004214a0` | `00448c10` |

현재 task가 `5105`인 경우 패치는 다른 DWORD event로 교체하면 `Dude.cpp:156` 진단을 호출한다. CD는 이를 `5105`로 강제한다. CD는 SID0 및 raw 상태의 free/dead 비트에서 조회나 생성 전에 반환한다. 패치의 SID0/free/예약 머리 미정의 쓰기는 지원 입력에서 제외하고 C++에서 쓰기 전에 거부한다.

원본은 mask `fff0`, 값 `5100`의 Regular를 찾는다. 있으면 event DWORD와 count0을 쓰며 payload를 유지한다. 없으면 크기40 객체에 payload5/flags50 Regular 생성자를 실행한다. 이후 현재 double 시각에 float 지연을 더하고 Regular의 double 예약 시각에 저장한다. 마지막 raw+12에는 event 하위 WORD만 쓰며 타입82이면 표면 통지를 요청한다.

패치의 디컴파일 C는 현재 시각을 float로 절삭한 표현을 보이지만 실제 `00450726 fld DWORD(delay)`, `0045072b fadd QWORD(now)`를 기준으로 복원했다. CD도 QWORD 현재 시각과 DWORD 지연을 더한다. `16777217.125` 시각과 분수 지연 입력으로 정밀도를 관찰한다.

## 같은 Host 소비자

`RawDudeTask::Set(Sid,uint32_t event,float delay)`와 `MakeDudeTaskHostHooks`는 같은 `SidPool`의 실제 `SquidProcessHost::FindEventMasked`/`AddRegular`를 사용한다. 현재 `RegularProcess::ResetTask`의 event/count/time 쓰기를 재사용하며 payload와 Form/PID를 보존한다. 기존 Regular의 count7/time27은 검사에서 실제 `RunFrame` 일곱 번으로 만든 입력이다.

검사는 type70/82, 현재 task5101/5105/cafe, 새 event5103/5104/5105/12345103, 새/기존 Regular, 지연0/0.1/−0.125/333, 시각10/16777217.125를 교차한다. CD의 SID0/free/dead 반환도 별도로 포함한다. 출력은 원본의 정상/assert 분류, 호출 순서, source 슬롯 전체, Regular time/event/payload/count다.

힙 할당·현재 Regular 조회·Form 부착·다리 표면 통지는 원본 실행기의 명시 관찰 경계다. C++는 이 중 조회/생성/부착을 실제 Host로 소비한다. 누락된 생성/표면 훅은 실제 호출 시점에 예외를 발생시키며 빈 효과로 완료 처리하지 않는다.

## 후속

실제 Dude70 생성 `00450a20/CD004e8b30`, 원본 factory/owner 가상 쓰기/Pop과 Path24, 드롭5103/5104의 부모 호출은 아직 별도 복원 대상이다. 이 task 모듈만으로 Damageable PostDestroy의 전체 드롭이나 캠페인1-1 피해 반응을 완료했다고 표시하지 않는다. 비유한 시각·힙 실패의 미정의 접근·전체 원본 진단창/예외 처리도 현재 관찰 범위 밖이다.

등록 파일은 `RawDudeTask.cpp`, `DudeTaskTests.cpp`이며 fixture 매크로는 `NETSTORM_DUDETASK_FIXTURE`다. 근거는 `dudetask.json`과 `dudetask-x86.tsv`, 검증 명령은 `python -X utf8 tools/decomp_dudetask_oracle.py --verify`다.
