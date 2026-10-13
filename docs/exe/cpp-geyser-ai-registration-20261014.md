# 최초 가이저 등록의 현재 Player AI 순회

## 범위와 출처

현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. `tools/ghidra/geyser-ai-registration-functions.json`의 PATCH2/CD1 readOnly 내보내기를 사용했다. 보호 PE SHA는 10.78 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, CD10.72 `613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233`이다. 기존 단위13 자료는 수정하지 않았다. 이 자료는 후속 단위14 준비다.

| 용도 | 10.78 | CD10.72 |
| --- | --- | --- |
| 모든 Player에 가이저 등록/제거 전달 | `00416720` | `004d8670` |
| 직접 AI callback | `004153c0` | 등록 몸체 안에 inlined |
| 실제 Player 배열 전역 | `0059534c` | `0050f834` |
| Player 한 기록 크기/AI 필드 | `0xb4` / `+0xb0` | `0xac` / `+0xa8` |
| 첫 nonnull 관찰 중단 위치 | `004153c0` 진입 전 | `004d86a0`에서 하위 분기 실행 전 |

`RawAssetPostPop::Geyser`의 최초 `geyserRegistration(sid,1)`이 이 몸체를 요구한다. Packet0x5b의 `RawGeyserMined`와 별도다. 원본은 Player0을 건너뛰고 Player1..8을 번호순으로 읽으며, 현재 AI 포인터가0이면 다음 Player로 진행한다. 모두0인 경우 SID나 state를 검사하지 않고 자연 반환한다.

## 구현 계약

새 `RawGeyserAiRegistration`은 동일 월드가 소유한 실제 Player 9기록의 BYTE span을 참조한다. `RawFortMoneyLoad`와 같은 저장소를 공급할 수 있다. 별도 Player 벡터로 복사하지 않으며 이후 현재 포인터의 변경도 다음 호출에서 읽는다. 유효한 nonnull 원본 Player base와 9개 살아 있는 기록이 입력 계약이다. base 주소의32비트 감김/해방된 저장소는 이번 범위 밖이다.

nonnull AI를 처음 발견하면 `GeyserAiRegistrationBoundary`에 **owner/AI pointer/SID/state의 네 DWORD**를 보존해 명시 거부한다. 미복원 AI 하위를 빈 callback으로 실행했다고 처리하지 않는다. state0은 원본에서 다섯 AI 목록의 제거, nonzero는 거리·동맹·사제 위치 판정과 목록/후속 AI 요청을 요구한다. 이 하위 몸체들은 미완료다. PATCH callback 내보내기는 의미 감사용이며 완료 몸체 수에 합산하지 않는다.

## 독립 원본 관찰

`tools/decomp_geyser_ai_registration_oracle.py`는 새 정밀 등록 entry의 실제 x86 명령만 실행한다. 현재 Player AI 읽기순서, 첫 하위 요청의 인자, 전체9기록 불변과 스택/x87 상태를 관찰한다. 실제 가이저나 AI 힙·원본 게임·GUI·OS 호출은 만들거나 실행하지 않는다.

| 판본 | 정상 null AI 고유 입력 | 미복원 하위 거부 고유 입력 | assert 입력 | 정상 반복 반환 | 하위 반복 중단 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 10.78 | 84 | 218 | 0 | 168 | 436 |
| CD10.72 | 84 | 218 | 0 | 168 | 436 |
| 합계 | **168** | **436** | **0** | 336 | 872 |

총604행에서 정상168과 미복원 하위 경계436을 분리한다. 거부436을 정상/assert 또는 전체 AI·전체 PostPop 완료 관찰 수에 더하지 않는다. 두 x87 정밀도의 반복도 고유 입력에 더하지 않는다. fixture SHA는 `d7b6d2a738a8c84fe5b9ec8ebe92776bb7a9d69bd0e55684aff0e58304e89850`이다. `cpppj/recovery-geyser-ai-registration-evidence.json`에 현재 생산 h/cpp·Test·tool·spec·PE·정밀 export·fixture 전체 SHA를 고정했다.

새 C++3개 검사 `GeyserAiRegistration_ReplaysPatch`, `...ReplaysCd`, `...ReadsSharedCurrentPlayersAndRejectsMissingStorage`를 준비했다. **현재 CMake 등록/공유 C++ 빌드·대조 전**이다. root 등록에는 `NETSTORM_GEYSER_AI_REGISTRATION_FIXTURE`가 필요하다. 원본 전수 생성과 `--verify` 감사는 통과했다.

## 미션 상위 전제

정상 새 미션의 reset은 기존 AI를 삭제하고 Player 기록을 전체0으로 다시 만든다. 미션 stage3에서 이미 로드된 사제의 존재를 확인한 뒤 AI를 부착한다. 그러나 실제 War Begins의 Battle/packet 저장맵 설치 caller 전체는 별도 조사 중이다. 이 작은 등록의 성공을 실제 캠페인172개 설치·미션 초기화·AI 전체 성공으로 확대하지 않는다. 상위 주소·Ready 전제·owner/depth 분리는 [설치 경계 감사](cpp-raw-fort-install-boundaries-audit-20261014.md)를 따른다.
