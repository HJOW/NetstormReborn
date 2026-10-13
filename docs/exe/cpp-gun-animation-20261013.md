# 썬 캐논 GunAnimation 복원 (2026-10-13)

작업 PC 및 마지막 디컴파일 PC는 **HJOW-Athlon**이다. `AGENTS.md`와 두 인수인계 문서를 확인하고 현재 호스트의 정밀 결과를 사용했다. 보호 원본은 읽기만 했다.

## 복원 몸체와 실제 소비자

`RawGunAnimation`은 캠페인 1-1에서 건설하는 썬 캐논의 회전·발사·원위치 애니메이션을 복원한다. `GunAnimation`은 같은 `SquidProcessHost` 및 `Kernel`에 실제 ProcessForm을 붙인다. `CreateGunAnimation`은 현재 GunProcess Form에서 부모를 읽고 stage 2/5/8을 원본 타입 36/35/36 및 flags `0x10`으로 생성한다. `MakeGunAnimationProcessHooks`는 기존 `RawGunProcess::startAnimation` 소비자에 실제 factory를 제공한다.

| 항목 | 10.78 | CD | 의미 |
| --- | --- | --- | --- |
| 회전 frame | `00467130` | `004a1fc0` | 현재 중심 4방향 → 방향별 28프레임 표 → 0.2f 간격 |
| 발사 frame | `00467330` | `004a21b0` | 표적 위치 추적 → 발사 프레임 → 0.07f 간격 |
| 원위치 frame | `00467240` | `004a20c0` | 방향 0/2/4/6/8의 다섯 행을 순서대로 소비 |
| 완료 | `00466eb0` | `004a11a0` | 현재 gunForm 조회 → event 1 통지 → animation Kill(0x10) |
| launch | `00466f50` | `004a12f0` | live/HP 표적 확인 → 실제 Missile ctor 인자 12개 |
| Missile ctor | `00481a50` | `0044ac70` | 현재 단위의 관찰 종료 경계. 몸체는 후속 단위 |

회전 완료는 info 3/value 4, 발사 완료는 info 6/value 7, 원위치 완료는 info 9/value 10이다. 발사 코드의 `0x4000`은 launch, `0x8000`은 완료이며 실제 부모 프레임에는 낮은 `0x3fff`만 쓴다. 모든 프레임 변경은 Unpop(0) → 패치 DWORD/CD BYTE 프레임 쓰기 → Repop(0x40)의 순서를 유지한다.

썬 캐논 전체 회전 표는 정상 네 방향과 원위치 마지막 방향 8이 사용하는 다섯 번째 행까지 실제 PE에서 읽었다. 타이머는 지나간 간격 수만큼 여러 프레임을 합성하지 않고 현재 게임 시각에 원본 float 간격을 더한다.

## 독립 관찰

`tools/decomp_gunanimation_oracle.py`는 두 PE를 제한 x86 환경에 읽기 전용으로 매핑한다. 게임·OS·창을 실행하지 않는다. 내보낸 불연속 함수 몸체 외 실행/허용 범위 밖 쓰기는 거부한다. 공간·중심·방향·피해점·난수·문자열·소리·완료 통지는 명시 관찰 경계다.

총 **2,560개** 고유 입력 관찰은 **정상 반환 2,510개**와 **Missile ctor 진입 직전 접두 정지 50개**로 구분한다. 각 판본은 반환 1,255개/접두 25개다. 접두 정지는 생성자의 실제 12개 스택 인자를 관찰하고 그 몸체를 실행하지 않는다. 이를 실제 투사체 생성·Pop·충돌의 완료 관찰로 더하지 않는다. C++ 대조도 같은 생성 경계에서 예외로 멈춘다.

CD 정밀 내보내기는 `004a2cb0`에서 기존 잘못 소유된 함수 정의 때문에 19/77개로 끝났다. 원본 PE에서 `004a2cb0`은 assert fallback 가상 함수의 정상 시작이다. 큰 containing 함수를 삭제하지 않았다. 사용한 CD 짧은 가상 thunk/시간 setter는 Capstone으로 실제 RET 명령까지 범위를 따로 확인하고 그 코드 bytes를 근거 JSON에 저장했다. 실제 frame의 가상 표와 내보낸 본체는 계속 직접 대조한다. 단순 판본 대응표만 신뢰하지 않고 **썬 캐논 회전 CD `00505520`, 발사 CD `00505480`**의 실제 가상 표를 확인했다.

관찰 및 SHA 자료는 `cpppj/tests/fixtures/gunanimation-x86.tsv`와 `cpppj/gunanimation.json`이다. 감사 명령은 `python -X utf8 tools/decomp_gunanimation_oracle.py --verify`다. C++ fixture 대조 두 항목, 미연결 Missile 거부, 실제 Host 완료 통지/Kernel/Form 삭제 항목은 `GunAnimationTests.cpp`에 있다.

## 남은 범위

실제 Missile 생성자·투사체 자산 생성/소유자/Pop·이동·충돌·피해·효과·삭제와 최종 GameWorld 연결은 계속 미완료다. `createProjectile` 또는 필요한 월드 경계가 누락되면 예외를 유지한다. 빈 callback으로 성공시킨 결과는 실제 전투 복원으로 인정하지 않는다. 썬 캐논 외 파생 애니메이션도 명시적으로 미복원 예외를 유지한다.

후속 함수 목록은 `tools/ghidra/gunmissile-functions.json`이다. CD Frame은 실제 Missile vtable `00501ca0 + 0x18`의 **`0044b260`**이며 이름이 비슷한 다른 `0044b4b0`를 사용하지 않는다. 공통 CMake, 인수인계 및 최종 통합 근거/커밋은 root가 갱신한다.
