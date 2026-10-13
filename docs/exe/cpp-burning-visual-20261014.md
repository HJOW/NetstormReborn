# Burning 생성자와 150입자 초기화

- 작업·마지막 정밀 디컴파일 호스트: **HJOW-Athlon, 2026-10-14**.
- 목표는10.78이며 CD10.72의 별도 생성 보조 함수와 초기화 순서도 기록한다.
- 독립 원본 관찰은 **256정상(128×2), assert0**이다. 정상 반환한 전체 생성자 관찰이며 접두 중단이 아니다.
- **단위12 최종 공유 C++3검사가 통과했다.** 양판본 생성자256 전수와 첫팔레트 미연결 거부를 확인했다. 단위12 최종 Release 경고/오류0·고유집중42·전체1056/실패0이며 상세 인수인계 뒤 `db9b626`(`20261014-04`)로 커밋했다. 기존 원본 보고서는 관찰 당시의 SHA/공유 검사 전 상태를 그대로 보존한다.

## 실제 클래스와 호출

Burning은 양판본 내장 `burningSystemProcessType` **14**다. 피해 주변 Dude 통지는 `00424030/CD004a66a0`으로 **모드0**, 일반 Damageable의 저HP 효과는 `00424090/CD004a6720`으로 **모드1**을 같은 생성자에 넘긴다. 실제 생성자는 `00423b20/CD004a5a50`, 마지막 가상 표는 `00503730/CD00505870`이다. type24는 별도의 `dudeProcessType` 이동 클래스다.

생성자는 먼저 공통 RenderProcess 목록에 등록하고 부모가 있으면 flags10으로 부착한다. 그 뒤 피해 SID·stage0·캡처/행선택/소리 필드0을 쓰고 **150개×24바이트** 입자를 확보·초기화한다. 마지막으로 모드 인수를 보존하며 공유 Burning 팔레트가 없을 때 첫 색 표를 만든다. 현재 `RawBurningVisual`은 기존 `RawRenderRegistry`의 전체 등록 몸체를 재사용한다.

| 기능 | 10.78 | CD10.72 |
|---|---|---|
| 공통 표시 등록 | `00487fd0` | `0045c6c0` |
| ParticleSystem 생성 | `00488190` | `0045cc40` |
| 입자 저장소3필드 초기화 | 상위몸체에 인라인 | `0045caa0` |
| 용량150 확보 | `00488100` | `0045cad0` |
| 각 입자의 x/y 두 DWORD 초기화 | 확보몸체에 인라인 | **`0045d410`** |
| 전체 입자24바이트 zero/total0 | `00487ab0` | `0045cb80` |
| 공유 팔레트 전역 | `005453d0` | `00538d34` |

CD `0045d410`은 원본 PE의 `0045cb32`에서 각 입자에 호출된다. 현재128개의 CD 생성자에서 **19,200회** 직접 실행했다. 처음 smoke에서 이 함수가 미허용이라 정지한 뒤 root가 별도 readOnly1함수를 내보냈다. 기존18/19함수 자료를 덮어쓰지 않았으며 초기화 호출을 noop으로 바꾸지 않았다.

## 입력과 출력 범위

현재 격자는 **이미 팔레트 캐시가 존재하는 월드 상태**를 명시 입력으로 공급한다. 부모0/50, 모드0/1, 표시 배열 없음/빈칸/중간구멍/가득참, 부모 signed깊이−11/34, SID0깊이−8/10, 확보 메모리의 최초0/90을 교차한다. 원본이 첫 빈칸을 찾거나40칸을 확장하는 계산을 입력 생성기에서 대신하지 않는다.

출력은 실제 초기화하는27개의 의미DWORD·표시 토큰 전체 목록·heap/free/부착 순서·150입자 전체3,600바이트의 Adler/sum이다. 호스트 포인터와 padding을 읽지 않으며 객체의 미초기화 delta40/44·DWORD74/84..8c를 완료 값으로 만들지 않는다. 해당 필드 전체를 초기화한144바이트 원본 객체라는 주장을 하지 않는다.

힙은 크기별 격리 메모리를 반환하는 관찰 경계다. 실제 ProcessForm 부착도 원본 인자/순서와 form83/현재부모 쓰기를 공급하는 경계이며 **현재 생성자 격자는 실제 Host 부착 성공의 근거가 아니다.** 원본 전체 생성자·표시등록·150입자 초기화는 직접 실행했다. 게임/GUI/운영체제/DirectDraw 호출은0이다.

## 남은 하위 몸체

첫 팔레트 cache0의 `ColorSort/gradient/buildPalette`는 아직 현재 C++생성자와 독립 원본 격자에서 닫지 않았다. 원본 띠는 시작 RGB `(1,245/255,117/255)`, 끝 `(1,115/255,7/255)`, 색수8, 보정2.4다. 첫 색 표에 필요한 경계가 없으면 호출 지점에서 예외를 낸다. 캐시가없는 초기 월드의 전체 효과 생성 완료로 세지 않는다.

실제 Frame은 `00424530/CD004a60e0`, source 연소점 선택은 `00424110/CD004a5d30`, 캡처/행선택 초기화는 `00423d50/CD004a5c70`, 입자 추가는 `00423e80/CD004a60a0`, Draw는 `00423ec0/CD004a64f0`다. 추가 함수는 별도564 원본 관찰과 C++2검사가 통과했다([입자 기록](cpp-burning-particle-20261014.md)). **실제 후속 프레임·캡처·표시·삭제는 미완료**다. 실제 vtable+20 Draw와+24 Frame을 PE의 가상 표에서 확인했으며 공통 Kernel의 빈 RunFrame과 혼동하지 않는다.

Burning은 기존 건설 표시와 같은 시스템 목록을 공유해야 한다. 현재 단위는 그 등록 몸체를 재사용하지만 실제 모든 표시 토큰/Host 소유권·같은 월드 수집/출력·DudeNotice/저HP Refresh의 모드별 생성/삭제 연결은 후속이다. Dude70/type24 이동 및 캠페인1-1 완주도 아직 완료되지 않았다.

## 자료

- `cpppj/src/o/RawBurningVisual.h/.cpp`.
- `cpppj/tests/BurningVisualTests.cpp`: 두 판본 warm생성자 전수와 `MissingColdPaletteRejected`3검사. macro `NETSTORM_BURNINGPROCESS_FIXTURE`.
- `tools/decomp_burningprocess_oracle.py`, `cpppj/tests/fixtures/burningprocess-constructor-x86.tsv`, `cpppj/burningprocess-constructor.json`.
- 현재256/전체 source·test·도구·공유기존몸체·실제 exports·보호 PE SHA 감사는 `python -X utf8 tools/decomp_burningprocess_oracle.py --verify`이며 성공했다.
- root 순차 정밀readOnly: `extracted/burning-process`18/19함수, `extracted/burning-process-particle-helper/originalCD`1함수. spec은 같은 이름의 `tools/ghidra/*-functions.json`이다.

이 자료는 첫 생성자 단위로 고정한다. 이후 first팔레트·particle/frame/삭제 자료를 새 독립 파일로 만들며 현재256기록과 source SHA를 보존한다. CMake·공통 manifest·검사 실행·상세LEFT·커밋은 root가 통합한다.
