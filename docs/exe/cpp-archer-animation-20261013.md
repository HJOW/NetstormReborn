# 썬 디스크 발사기 회전과 발사 (2026-10-13)

현재 분석 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 보호 원본 10.78과 CD10.72의 현재 `gunarcher`/`gunarcher-helper` 정밀 내보내기와 실제 PE 가상 표를 읽었다. 게임/OS/DirectDraw는 실행하지 않았다.

## 복원 몸체와 타입

원본 `sunArcher`71은 화면의 **Sun Disc Thrower(썬 디스크 발사기)**다. Fire는 `sunDisc`83·속도35·비행 효과0을 실제 같은 Missile 생성자에 요청한다. `RawArcherAnimation`은 기존 썬 캐논의 고정 Fire/Rotate 프레임표와 구별하는 전용 각상태 객체다.

| 몸체 | 10.78 | CD10.72 |
| --- | --- | --- |
| 실제 회전 ctor | `004687a0` | `004a18f0` factory 내부 |
| 실제 Fire ctor | `00468140` | `004a3960` |
| 회전 Frame | `00467940` | `004a26c0` |
| 근접 방향 선택 | `004676c0` | `004a2640` |
| 초기 Fire 각 | `00467b20` | `004a28a0` |
| Fire 각→frame | `00467b90` | `004a2910` |
| Fire Frame | `00467c80` | `004a2a10` |
| launch | `00466f50` | `004a12f0` |
| 부모·중심→방향 | `0041ce20` | `0043f530` |
| 좌표 선분→도 | `0041d7e0` | `00440ac0` |

Fire의 원본 객체는88바이트이며 공통 Form/부모4/8, 표적 좌표12/16·중심20/24, inner가상표32, dirty40, gunForm42/target44, time48, rest56, 각위치64·각속도68·각가속도72·sprung76·fired80을 사용한다. 회전의 실제 객체는56바이트다. C++는 전용88바이트 소유 이미지로 관찰되지 않은 padding을 보존하며 회전에서 Fire 각필드를 임의로 초기화하지 않는다.

회전은 due 검사 전에 현재 표적을 재조회한다. free/dead이면 이전 좌표를 유지하며 target0이면 실제 방향 BYTE getter를 사용한다. 타입의 방향 수와 물리 프레임 수를 나눠 현재 방향을 구하고20칸 비균등 각도표에서 가장 가까운 항목을 고른다. 양쪽 회전 거리가 같으면 역방향 한 칸을 선택한다. Unpop0→frame쓰기→Repop0x40, 다음 시각+float0.1, 실제 난수1000..10000과 `sunDiscThrowerRatchet0N.wav` 순서를 유지한다.

Fire ctor는 현재 물리 프레임0..32에서 초기 각을 구하고 각속도/가속도를 원본 전역180으로 설정한다. 같은 Host에 부착한 뒤 부모 출력WORD bit0을 해제하고 실제 공간 통지를 요청한다. Fire는 매 틱 현재 delta가0보다 크고2보다 작을 때 각위치/속도를 원본 float 저장 순서로 진행한다. 현재 표적을 따라 spring·발사·복귀 상태를 갱신하고 `sasprung.wav`, `safired.wav`, `sarachet.wav`, `sunDiscThrowerWhoosh.wav`의 실제 분기를 요청한다. 발사 때 부모 출력bit0과 통지를 갱신한 뒤 실제 launch를 수행한다. 끝 조건에서 GunProcess에 event1/info6/value7을 통지하고 flags0x10으로 현재 Form을 Kill한다.

## 실제 x86 관찰

`tools/decomp_gunarcher_oracle.py`는 양판본 **1,690개 고유 입력**을 관찰했다. 판본마다 **정상844 + 별도 launch의 Missile ctor 직전 접두1**, 총 정상1,688과 접두2다. 정상에는 Rotate, Fire, 방향 선택, 초기 각, Fire frame helper 및 **Fire ctor**가 포함된다. 회전 ctor 전체의 독립 원본 반환 검사는 아직 포함하지 않는다.

Fire 관찰은 실제 launch 몸체도 실행한다. 이어지는 Missile ctor는 별도 `archermissile`1,888행 단위로 이미 검증한 하위 관찰 경계다. 직접 launch 관찰의 접두2는 그 ctor 진입 직전에서 정지하므로 실제 투사체 생성 완료로 세지 않는다. 임의 no-op 생성 완료를 주장하지 않는다.

각도기하·x87 fpatan·실제 CRT fmod/절삭은 현재 정밀 PE 몸체를 실행한다. CRT dispatch/table은 PE 원문이고 FPCW0x027f/FPSW0, 패치 `005e4acc=0`/CD `00546778=0`으로 정상 하드웨어 경로를 공급한다. 유한 각도/분모360의 정상 경로만 관찰했으며 errno/하드웨어 보정·비유한 입력 완료를 포함하지 않는다. 실제 짧은 virtual thunk는 PE의 첫 RET까지 바이트를 기록해 CD 함수 소유 문제를 구분했다.

포구/현재 중심·타입 query·공간·사운드/format·난수·부착·출력 통지·완료·Missile 생성은 명시 관찰 경계다. 정상 반환과 인자/순서/raw는 그 경계의 실제 UI/음향/월드 몸체 완료와 구분한다. 첫 공유 Release의4개 검사에서 missing 경계는 통과했고 두 판본 전수와 ActualHost에 총13checks 차이가 있었다. 이 단위는08에 등록됐으며07 투사체 완료 수와 합치지 않는다.

첫 C++ 대조의 target0 차이를 실제 계수 `0050afd8/CD00505354=90`으로 교정했다. 초안의45도 계수는 원본이 아니었다. 실제 Host 검사의 입력에 type71.group0도 공급했다. 수정 전 source/h/test/oracle/전체1,690행/전체보고서를 `recovery-history/gunarcher-initial`에 실제 바이트로 보존했다.

두 번째 공유 대조는 target0 차이를 통과한 뒤 Fire의 프레임1/19를0/18로 계산하는 차이와 실제 서버 SID 할당 예외로13checks 실패했다. 두 번째 교정 전에 당시 전체 source/h/test/oracle/fixture/report와 첫 교정 도구/보고서 및 실패 로그를 `recovery-history/gunarcher-aim-corrected`에 실제 바이트로 보존했다. PE의 `0043fa50/CD0048ee10`은 float 입력을 읽고 `00500ee4/CD00504654`의 실제0.5f를 더한 뒤 `__ftol`로 절삭한다. C++ FireFrame에도 float 저장 후 이 가산/절삭을 적용했다. 현재 원본 fixture 전체는 두 snapshot 모두와 동일하고 신규 고유 입력은0이다. `analyze_gunarcher_correction.py`의 schema2 보고서는 각 명시 수정의 역변환, 두 당시 전체보고서의 SHA, 실제 helper15바이트와0.5f 상수를 검사해 현재 SHA를 연결한다. 두 번째 교정 후 최종 공유 Release 빌드71712 종료0, ArcherAnimation의4검사/실패0으로 두 판본1,690행 전수·누락 생성 거부·실제 Host 송신과 수명이 모두 통과했다. 초기 로그는 `campaign-runtime8-ArcherAnimation_-initial-tests.log`, 두 번째 로그는 `campaign-runtime8-ArcherAnimation_repaired-tests.log`, 최종 로그는 `campaign-runtime8-ArcherAnimation_final-tests.log`다. 단위08 전체 검사는 별도로 관리한다.

## 실제 프레임 자료와 후속 연결

`sunArcher.type`에는 P00..P33/A00 총35클러스터가 있으며 `orientations` 속성은 없다.20칸 선택표나35클러스터만으로 실제 TYPE+0x110 방향 수를20으로 단정하지 않는다. 현재 독립 격자는 방향20/물리40을 명시 입력으로 공급한다. 실제 타입 초기화의 방향 기본값 및 회전에서 사용하는 현재 메타데이터를 같은 월드에서 연결해야 한다. `sunDisc.type`의 명시 `orientations=1`,3클러스터와 `fireBow.wav`/`sunDiscThrowerImpact.wav`는 별개다.

`CreateArcherAnimation`은 같은 Host/Kernel의 실제 GunProcess type71과 단계2/5를 검사하고 process36/35·flags0x10으로 붙인다. `MakeArcherAnimationProcessHooks`는71을 이 factory에 연결하며 `MakeArcherMissileHooks`는 이미 검증한 실제 Missile factory를 공유한다.4번째 C++ 검사는 서버 풀에서 실제 GunProcess를 만들고 기존 `GunProcess::WritePacket`과 송신 packet 관찰 경계를 공급한다. Archer Form은 실제 flags0x10 로컬 부착으로 animation writer를 요구하지 않는다. 두 판본에서 같은 실제 Form·부모 연결, 회전 완료3→4·발사 완료6→7 통지 및 Kernel 삭제가 최종 공유 검사에서 통과했다. 서버 animation 직렬화·실제 Rotate ctor 전체·부모 기본 프레임 자료·일반 피해 및 충돌 효과 하위·GameWorld 연결은 후속이다.

등록 파일은 `RawArcherAnimation.cpp`, `ArcherAnimationTests.cpp`, fixture macro `NETSTORM_GUNARCHER_FIXTURE`이며 감사 명령은 `python -X utf8 tools/decomp_gunarcher_oracle.py --verify`다. 이 독립 단위만으로 캠페인1-1 전투/완주 동일성 완료를 주장하지 않는다.
