# 실제 기본 가상 방어와 현재 표 분배 (2026-10-14)

현재 분석/마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 보호10.78/CD10.72 PE를 읽기만 했고 게임/GUI/OS를 실행하지 않았다. 새 Ghidra 실행 없이 두 PE의 현재 가상 표와 실제 첫 RET4까지5바이트를 확인했다.

기본 가상+0x9c의 실제 몸체는 **10.78 `0043d9b0` / CD10.72 `004aed90`**이며 두 원본 모두 `33c0c20400`(`xor eax,eax; ret4`)다. this와 공격SID를 읽지 않고 false를 반환한다. `RawSquidDefaultDefense`는 이 실제 반환만 복원하며 임의의 미복원 가상 메서드의 성공 기본값으로 쓰지 않는다.

| 현재 객체 | 10.78 vtable | CD10.72 vtable |
| --- | --- | --- |
| base Squid | `00501dd0` | `005058d8` |
| Sun Disc Thrower71 | `0050b888` | `005014f8` |
| Sun Cannon128 | `0050b9b8` | `00501120` |
| Priest158 | `0050f210` | `005003e0` |
| anim160 | `00509758` | `00503b30` |

이 표의 원본+0x9c DWORD는 판본별 기본 주소와 모두 일치한다. Hit+0x98의 파생 분배와 혼동하지 않는다. 71/128의+0x98은 실제 DamageableHit, 사제는 PriestHit, anim은 BaseHit다. 신전 같은 다른 파생 Hit/방어는 계속 별도 몸체다.

`MakeRawSquidDefenseHooks`는 현재 raw vtable와 실제 진입 대응표를 받아 이 기본 몸체만 선택한다. 타입 번호로 현재 가상 표를 추측하지 않으며 현재 표가 다른 경우 기존 callback을 보존한다. 그 callback도 없으면 예외다. 풀과 대응표 수명은 callback보다 길어야 한다.

독립 `decomp_squiddefense_oracle.py`는 양판본 **48개 정상 고유 반환**, 각24개를 실제5바이트 몸체로 실행한다. 유효 victim/다양한 공격 SID, 반환0·RET4/스택·전체raw불변을 직접 관찰했다. 실행/쓰기 허용 범위 밖 주소와 OS 호출은 없으며 actual vtable/PE/source/fixture SHA도 기록한다. current-vtable guard 자체는 새 C++ 소비자이며 원본이 raw5바이트 안에서 그 guard를 수행한다고 주장하지 않는다.

현재4개 C++ 검사는 두 원본 반환 전수, 실제 Factory의71/128/158/160 생성 가상 표와 fallback 보존, 미복원 다른 파생/누락 표/SID 거부를 확인했다. **공유 Release 경고/오류0, SquidDefense4검사/실패0, 양판본48개 전수통과**다. 기본 방어로 실제 HP 피해·사망/효과 완료를 대신하지 않는다. 앞 단위09의 일반 Hit나08 보고서는 수정하지 않았다. 단위10 **a349e63**의 고유집중35·전체1023검사/실패0이다.

등록 대상은 `RawSquidDefense.cpp`, `SquidDefenseTests.cpp`, fixture macro `NETSTORM_SQUIDDEFENSE_FIXTURE`다. 감사 명령은 `python -X utf8 tools/decomp_squiddefense_oracle.py --verify`이며 보존된 실제 생성자 recipe와 현재 원본 가상 표를 소비한다. 캠페인1-1 월드 이관과 완주 동일성은 계속 미완료다.
