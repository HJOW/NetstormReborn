# CD 10.72 전투 옵션 초기화 복원 (제품 1308, 2026-11-14)

마지막 디컴파일 PC: **HJOW-Athlon**. 기존 `extracted/refined/originalCD/NETSTORM.c`를 참고했으며 원문 변경이나 재디컴파일은 없다. CD 원본 `originalCD/NETSTORM.EXE` SHA-256은 `613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233`이다. 원본은 정적 PE 조회로 읽기만 했다.

## 실제 CD 초기화 순서

CD Tutorial Init `0040f6e0`은 `00453520`을 호출한다. `00453520`은 전역 옵션 `00565b00`의 주소를 `00453540`에 넘기고, 다음 순서로 처리한다.

1. `00453570`이 전달된 옵션 `+0` DWORD, `+4` WORD, `+6` BYTE, `+7` BYTE에 0을 써서 총 **8바이트**를 초기화한다.
2. `00453540`은 포인터 `005200f8`에서 BYTE 하나를 읽어 옵션 `+0`에 쓴다. 포인터를 `0x18`(24바이트)씩 증가시키며 `005201a0`보다 작은 동안 순회한다.
3. 일곱 기본 선택이 옵션 `+0~+6`에 들어간다. 옵션 `+7`에는 이 복사 루프가 쓰지 않으므로 앞선 초기화의 **0**이 남는다. 8번째 값을 입력에서 보존하는 동작이 아니다.

| 옵션 위치 | CD PE 가상주소 | BYTE 값 |
| --- | --- | --- |
| +0 | `005200f8` | 1 |
| +1 | `00520110` | 2 |
| +2 | `00520128` | 0 |
| +3 | `00520140` | 1 |
| +4 | `00520158` | 1 |
| +5 | `00520170` | 0 |
| +6 | `00520188` | 1 |
| +7 | 기본 선택 루프 쓰기 없음 | 앞선 8바이트 초기화 결과 0 |

이 표는 제품 상수를 읽어 기대값으로 삼은 것이 아니라 원본 PE RVA에서 직접 읽은 값이다. 준비 경로 `evidence.json`에 주소·순서·원본 해시와 두 제품 입력/준비본 해시를 함께 보존했다.

## 구현과 범위

`ResetMissionBattleOptions(Cd1072, options)`에 정확한 8바이트 입력만 허용하는 분기를 추가했다. 전체8바이트를 0으로 만든 뒤 위 일곱 값을 복사하고 반환한다. 10.78의 기존 87바이트 분기·22개 기본 선택·40바이트 예약 영역 보존과 기존 fixture는 변경하지 않는다.

CD에 길이0~7·9 이상을 전달하면 쓰기를 시작하기 전에 명시 오류를 낸다. `RawMissionBattleOptions` 실행기 자체의 CD 지원, TYPE 171/188과 원문 stride468/500, 실제 Client의 CD8바이트 저장소·Tutorial 연결은 이 준비본 범위에 포함하지 않는다. 따라서 이 패치만으로 실제 CD Init이나 튜토리얼 조립이 완료되는 것은 아니다.

## 검사

- `MissionBattleOptions_CdResetEightBytesAndNeighborCanaries`: 입력0·0x5a·0xff 각각에서 일곱 기본값·8번째0과 양쪽4바이트 canary를 포함한 **16바이트 전체**를 고정 기대 배열과 비교한다.
- `MissionBattleOptions_CdResetInvalidSizesRejectBeforeWrite`: 길이0~7·9·16·86·87에서 거부되고 양쪽 이웃을 포함한95바이트 전체가 그대로인지 비교한다.
- 기존 `MissionBattleOptions_ResetNative296`와 PATCH 초기화 부분은 변경하지 않는다. 기존 `ResetRejectsBeforeWrite`의 CD87바이트 거부 단언도 계속 유효하다.

Release `NetstormCpp`와 신규 `netstorm_mission_option_reset_tests` 대상 빌드가 종료 코드 0으로 완료됐다. `--filter MissionBattleOptions_CdReset`의 새 2건과 `--filter MissionBattleOptions_Reset`의 기존 2건(독립 원문 296관측 포함)이 모두 통과했다. 다른 전투/MP 시험은 실행하지 않았다. 원본 실행 파일의 SHA-256을 다시 읽어 위 값과 일치함을 확인했다.

이 단위는 아직 Client의 CD 옵션 저장소/Init에 연결되지 않은 순수 초기화 몸체 복원이다. 현재 CD 메뉴는 1307에서 확인한 `RawBattleCompletion`의 미초기화 Player 검사에서 미완료이며 게임 실행 검증을 다시 했다고 표시하지 않는다. 다음은 [설치 경계 조사](cpp-cd1072-tutorial-integration-audit-20261114.md)에 따른 CD Player 초기화와 실제 소유자·후반 프레임 연결이다. 사제 포획/희생·공격·승리·MP·사용자 직접 확인 검사는 실행하지 않았다.
