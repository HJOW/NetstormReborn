# cpppj 중단 인수인계 — 훨리베이스 단위75

2026-10-16, 현재 PC **HJOW-Athlon**. 사용자가 작업 중지를 요청한 뒤 **“검사도 다음으로 넘겨. 검사 필요까지 바로 인수인계해줘”**라고 지시했다. 구현과 검사를 중단했으며 새 검사 없이 현재 자료를 로컬 커밋으로 보존한다. 전체 목표는 미달성 상태로 일시 중단한다. 서브 AI `construction_audit`·`energy_finish`·`session_audit`는 모두 중지 상태다. 사용자가 재개하기 전에는 구현이나 검사를 다시 시작하지 않는다.

## 작업 기준과 보존할 변경

- 직전 기준 커밋은 `446713e` / `20261016-02 CP`다. 이번 중단 커밋은 `20261016-03 CP` 형식을 사용한다. 원격 push는 하지 않는다.
- 마지막 전체 정밀 디컴파일 PC/날짜는 **HJOW-Athlon / 2026-10-15**다. `extracted/refined/originals/Netstorm.c`와 `functions.tsv`를 사용했다. PC가 달라지면 AGENTS 규칙에 따라 디컴파일을 다시 수행해야 한다.
- `RawSunAviaryProcess.h/.cpp`는 실제0045ec50 후처리,0045d690 생성자,0045d8c0 프레임,0045d6e0→0045b070 패킷을 복원한다. 같은 Kernel의 타입32·flags8 Form으로 붙인다. 생성 시 현재48바이트를 보존하고 원본이 쓰는 위치만 변경한다.
- `RawCampaignSession.cpp`는 실제 월드/부모/시각/음향/공통 전송 및 실제 ProcessForm 수명을 연결한다. `RawAssetPostPop.*`는 VT00509930만 이 전체 후처리에 분배한다. 큰 공용 PostPop 레이아웃은 바꾸지 않았다.
- 신규 `SunAviaryProcessTests.cpp`, `sun-aviary-x86.tsv`, `decomp_sun_aviary.py`, `sun-aviary.json`, CMake 등록, 기존 `CampaignMissionStartTests.cpp`의 실제 객체 검사와 관련 문서/줄바꿈 규칙을 보존한다. `type-required-energy.json`은 변경된 CMake SHA 기록만 갱신하며 기존 TYPE 제품/2,352입력 fixture를 바꾸지 않는다. 그 메타데이터 변경 뒤 검증도 다음으로 넘긴다.
- 원본/클론 GUI 실행, 보호 원본/AGENTS/dotnetpj 변경, 원격 push는 없다. Windows10/11 전체화면 DirectDraw 오류는 계속 수정 범위에서 제외한다.

## 중단 전에 이미 완료된 검사

- Release 빌드 exec40417 종료0. 로그: `extracted/goal-sun-aviary-build.log`.
- 원본 고유 입력 **1,734개**: 생성자18·후처리98·프레임594·writer1,024. 같은 입력을 x87 정밀도027f/037f로 재생했고 출력이 같았다. 3,468개로 중복 합산하지 않는다. OS/GUI 실행이나 자연 플레이 관찰이 아니다.
- `SunAviary_` **5개·실패0**: 전체1,734입력의 C++ 비교와 필수 경계 검사를 통과했다.
- `CampaignMissionStart_` **3개 중2개 통과·1개 실패**. Tutorial1~5의 Load/Start를 통과했다. Tutorial5의 buried 두 객체 QA114/실제 두 Regular, 실제 아비어리 Form/Kernel/stage1/전송 후 dirty0 검사에 도달해 통과했다. 로그: `extracted/goal-sun-aviary-focused.log`.
- 실패는 `CampaignMissionStart_ProtectedEightMissions`의 **Tutorial6 Load: SID15006·TYPE136·VT00509470(10진5280880)** 미복원 Pop 파생 가상 후처리다. Tutorial6 이후 캠페인 두 개는 이 순회에서 도달하지 못했다.

## 다음 재개 때 필요한 검사 — 지금은 실행하지 않음

시작했던 `ctest --test-dir cpppj/build -C Release --output-on-failure`와 자식 테스트/래퍼 프로세스를 사용자 요청으로 종료했다. `extracted/goal-sun-aviary-ctest.log`는 중단 로그이며 이번 전체 결과가 없다. `cpppj/build/Testing/Temporary/LastTest.log`는 이번 완료 근거로 쓰지 않는다. 단위74의 전체1,316개 중1개 실패는 과거 결과다. 새 테스트 수와 회귀 여부는 재개 후 실제 결과로 확정한다.

1. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽고 호스트/커밋/작업 트리를 확인한다. 현재 소스는 이미 빌드했지만 PC나 소스가 달라졌다면 먼저 Release 빌드를 갱신한다.
2. 다음 두 native 보고서의 source/fixture/CMake SHA 검증과 Python 구문, 변경 파일의 UTF-8/한국어 주석/공백 감사를 수행한다. 중단 인수인계와 메타데이터 변경 뒤 마감 감사는 하지 않았다.

```powershell
python tools/decomp_sun_aviary.py --verify
python tools/decomp_type_required_energy.py --verify
python -m py_compile tools/decomp_sun_aviary.py
git show --check HEAD
```

3. 다음 전체 회귀를 **새 로그로** 수행한다. 알려진 Tutorial6 실패를 포함해 다른 회귀가 없는지 확인하고 실제 개수/실패/시간을 문서에 갱신한다. 중단 직전 집중 검사는 위 기록으로 구분한다.

```powershell
ctest --test-dir cpppj/build -C Release --output-on-failure
```

4. 다음 구현 후에는 해당 집중 검사도 다시 실행한다. 실패를 없애려고 빈 callback이나 다른 VT 허용, 기대값 완화로 우회하지 않는다.

```powershell
cpppj/build/bin/Release/netstorm_tests.exe --filter SunAviary_
cpppj/build/bin/Release/netstorm_tests.exe --filter CampaignMissionStart_
```

## 이후 복원과 목표의 한계

첫 구현 재개 지점은 Tutorial6의 **TYPE136·VT00509470** 실제 파생 후처리다. 이름/가상 함수 주소와 원본 호출 경계를 정밀 산출물에서 확인한 뒤 복원해야 한다. 그 이후에도 현재 보호8미션의 전체 통과를 확인해야 한다.

훨리베이스 stage6의 훨리기그73 파생 후처리/이동과0045c1b0 BaseFlyer 연결은 미복원이다. 현재 Session은 해당 미복원 경계를 명시적으로 실패시킨다. 개발자 frameCheck는 정상 타입 코드 범위만 연결했으며 잘못된 메모리/GUI 진단 전체를 복원한 것은 아니다. 힙·부착·Damageable·장치·플라이어 생성/Owner/Pop·BaseFlyer·재전송은 native 관찰의 명시 외부 경계다.

Early Missions1~6, 데모3개(`DemoStormRages`·`GuardMyBack`·`DissolvedAlliance`), 캠페인1-1/1-2의 실제 GUI 진입과 자연 플레이·승패·다음 미션·저장·그래픽/음향 동일성은 여전히 미완료다. 자세한 구현 경계는 [훨리베이스 기록](cpp-sun-aviary-20261016.md)과 [LEFT_JOBS](../../LEFT_JOBS.md)를 따른다. 이번 인수인계가 과거의 재개/active 문구보다 우선한다.
