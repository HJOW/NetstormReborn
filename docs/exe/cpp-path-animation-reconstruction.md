# Squid 방향 조회·PathProcess 도착 프레임 접두 복원

2026-10-08, 마지막 디컴파일 수행 PC **VM-W11-CODEX**. 같은 PC의 기본 Ghidra 프로젝트에서 `pathanimation`을 세 판본 각각 읽기 전용으로 내보냈다. 1차 기준은 10.78이다. 원본 게임이나 복사본은 실행하지 않았다.

[`RawPathAnimation`](../../cpppj/src/o/RawPathAnimation.h)은 현재 프레임의 방향 조회와 **도착 시 프레임 전환 접두**를 복원한다. [`GameWorld::Frame`](../../cpppj/src/client/GameWorld.cpp)은 보행 종료 표시에서 이 모듈의 `RestFrame`을 사용한다. 실제 raw SID의 Unpop/프레임 쓰기/Repop은 콘솔 통합 검사에 연결했다. GUI 유닛 전체의 raw 전환이나 PathProcess 전체 복원은 아직 완료하지 않았다.

## 원본 주소와 복원 범위

| 처리 | 10.78 | CD/추가 10.37 | 복원 내용 |
| --- | --- | --- | --- |
| side 조회 | `004ad680` | `004ae3f0` | 현재 FrameCode의 첫 바이트를 signed char로 읽음 |
| direction 조회 | `004ad710` | `004ae4b0` | signed side − `'A'`, 0~7 제한 없음 |
| 도착 프레임 접두 | `0048bd90` | `00480580` | 방향별 첫 물리 프레임 +1, 가상 Unpop/Repop |

원본 프로세스의 `+8` 부모 SID로 walker 슬롯을 찾는다. 타입의 `+0x130` 글자별 첫 프레임 표에서 현재 방향의 값을 읽고 **Unpop(0) → 프레임 쓰기 → Repop(0x40)** 한다. Repop은 현재 raw 위치에서 Pop하는 실제 `+0x4c` 연결 함수다. 이미 같은 정지 프레임이어도 Unpop/Repop을 생략하지 않는다.

정지 프레임은 `FindNumber(side, 'P', 1)`이 아니다. 예를 들어 같은 글자의 물리 코드가 `P00, P04, P01, P07` 순서이면 두 번째 물리 항목 `P04`를 선택한다. 비연속 글자/다른 variant도 원본 글자 표의 첫 번호를 그대로 사용한다. 글자 표의 생성은 기존 `RiftTypeFrames::Run` 복원과 실제 x86 표 생성 함수를 재사용했다.

10.78은 raw `+36`의 DWORD를 쓴다. CD/10.37은 `+34`의 BYTE만 쓰므로 `+35` extra를 보존하며 하위 8비트 넘침이 발생한다. 호스트는 free/예약 SID, 자산 타입 밖, 현재 프레임 배열 밖, 정지용 A~P 표 밖 입력을 **가상 효과 전에 예외로 거부**한다. getter의 signed side 자체에는 A~P 제한을 추가하지 않았다.

## 독립 기계어 대조

[`decomp_pathanimation_oracle.py`](../../tools/decomp_pathanimation_oracle.py)는 세 실제 PE를 Unicorn 32비트 가상 메모리에 올려 허용된 함수 몸체만 실행한다. 게임/OS/API/창 실행은 없다. 실제 side/direction, 글자 표 생성, 접두의 부모 SID/프레임 읽기·쓰기, Repop 연결 함수를 실행한다. **가상 Unpop/Pop은 호출 순서·인자·호출 당시 프레임만 기록하는 대체**다.

패치판은 Repop 직후 `0048bdd0`에서 멈춘다. 이후 디컴파일 C에 인라인으로 이어지는 경로 정리/재배치/후속 명령 처리는 실행하지 않는다. CD/10.37은 별도 종료 함수 `0047e220` 진입에서 멈춘다. 접두 실행기가 스택/종료 주소를 맞추는 것은 검사 어댑터이며 원본 함수 전체의 반환이나 종료 함수 복원을 증명하지 않는다.

- 판본마다 방향 조회 **608행**(한 행에 side와 direction 관찰), 도착 접두 **1,408행**.
- 세 PE 합계 **6,048개** = 조회 **1,824** + 접두 **4,224**. x87 53/64비트 정밀도 두 실행에서 같은 관찰만 저장한다.
- A~P 물리 프레임 전체, 비연속 코드/variant, 이미 정지 프레임, owner 0~8, extra 0/1/16/255, signed side 0~255, CD BYTE 넘침을 포함한다.
- 넘침 입력의 새 물리 번호 256은 **Pop 대체 상태에서 필드 쓰기만 관찰**한다. 실제 SHP가 그 프레임을 지원한다는 뜻이 아니다.
- [`pathanimation-x86.tsv`](../../cpppj/tests/fixtures/pathanimation-x86.tsv)의 사건·슬롯 전체를 C++에서 비교한다. [`recovery-pathanimation-evidence.json`](../../cpppj/recovery-pathanimation-evidence.json)은 PE/내보내기/도구/입력 목록/fixture SHA와 진입·경계 도달 횟수를 기록한다.

## 실제 연결·검증

[`PathAnimationTests.cpp`](../../cpppj/tests/PathAnimationTests.cpp)는 저장 fixture를 사용하므로 CTest에는 원본 파일/Python/Ghidra가 필요 없다. 보호 입력의 변경 전 거부를 검사하고, 두 판본에서 실제 Factory·Unpop·Pop을 연결했다. 일반 가상 경로를 지원하는 bridgeConnector 타입 155와 합성 코드/크기를 사용해 **기존 해시 단계 1 해제 → 새 프레임 기록 → 단계 2 재등록 → 같은 프레임 반복에도 호출/단일 체인 유지**를 확인한다. 표시/Graph/사제 전용 postPop이 없는 콘솔 검사다.

GUI의 건물/walker는 기존 어댑터를 유지한다. 보행 중 12Hz 임시 프레임 선택은 그대로이며 **종료 프레임 계산만 복원 모듈로 교체**했다. 실제 자산의 1-1 사제는 동쪽 C 구간의 두 번째 물리 번호 17로 도착한다. 기존 창 검사에서 1-1/TEST01 선택·이동·정지·재개·도착·카메라·복귀/재진입 **20개 상태**가 통과했고 도착 캡처를 직접 확인했다. 표면 창 검사도 지연 삭제·재개·픽셀 변화·재진입과 10.78/CD 5개 raw 미션 로드를 통과했다. 두 창 도구는 원본 디렉터리 전체 해시/존재 상태가 변하지 않았음을 확인했다.

최종 Release **경고/오류 0**, CTest 내부 **266개·실패 0**(기존 261 + 새 5, 60.46초), 관련 감사 **7종 모두 통과**(pathanimation/setframe/bridgeevent/process/display/pop/unpop). 새 독립 x86 6,048개를 더해 누적 **137,590개**다. 로그는 `extracted/pathanimation-export.log`, `pathanimation-generate.log`, `build-pathanimation-final.log`, `ctest-pathanimation.log`, `audit-pathanimation.json`, `gui-pathanimation-world.log`, `gui-pathanimation-surface.log`에 있다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name pathanimation
python -X utf8 tools/decomp_pathanimation_oracle.py
python -X utf8 tools/decomp_pathanimation_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
python -X utf8 tools/cpp_world_smoke.py
python -X utf8 tools/cpp_surface_world_smoke.py
```

## 다음 구현에 필요한 경계

2026-10-08 후속에서 아래 1번의 **사제 소유자 재정의는 복원 완료**했다([근거·범위](cpp-priest-owner-reconstruction.md)). 사제 postPop/preDestroy와 보행 진행/종료·raw 유닛 전환은 계속 남았다.

1. **사제 전용 lifecycle/소유자/postPop**부터 복원한다. 실제 priest 타입은 **158**, 생성자는 `004952a0`/CD `0040d790`, 가상 표는 `0050f210`/CD `005003e0`이다. postPop `004950f0`/CD `0040c110`은 regen Regular(`0x25a`)·carrier/damageable 처리·낙하/경로 무효화를 포함한다. 소유자는 `00491790`/CD `0040bda0`, preDestroy는 `004919b0`/CD `0040c330`이다. 공통 가상 표로 바꾸어 현재 표면 Pop에 넣으면 이 전용 효과를 잃는다.
2. **PathProcess 보행 프레임 진행** `0048bb40`/CD `004805d0`과 도착 뒤 종료/명령 처리를 별도로 복원한다. SHP 추가 헤더의 float 이동량·정지/목표 판정·물리 프레임 진행을 포함하므로 현재 12Hz 타이머로 원본을 재현했다고 볼 수 없다.
3. 검증된 유닛 lifecycle·process·표시를 raw 월드/Kernel로 전환한 뒤 실제 walker 낙하·파편/소리, 배치·건설·경제·전투·승패를 연결한다. 장시간 변이 검사/최대 지도·SID 소진 등은 기존 인수인계를 유지한다.
