# cpppj 공통 pre/postDestroy 선택·목록·통계·비용 복원

2026-10-07. **1차 기준은 10.78**이다. [공통 destroy 순서](cpp-destroy-reconstruction.md)에 실제 공통 장부 훅을 연결했다. 호스트 **DESKTOP-HJOW에서 원본/복사본 게임·업데이터/설치 도구·클론 창을 실행하지 않았다.** Graph 비활성·AI 미부착·비전투 null 큐 범위를 콘솔로 검사한다.

소스: [SquidDestroyLifecycle.h](../../cpppj/src/o/SquidDestroyLifecycle.h), [구현](../../cpppj/src/o/SquidDestroyLifecycle.cpp). 검사: [DestroyLifecycleTests.cpp](../../cpppj/tests/DestroyLifecycleTests.cpp). 독립 출력: [destroylifecycle-x86.tsv](../../cpppj/tests/fixtures/destroylifecycle-x86.tsv), [근거 기록](../../cpppj/recovery-destroylifecycle-evidence.json), [생성/감사 도구](../../tools/decomp_destroylifecycle_oracle.py).

## 원본 주소와 새 내보내기

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 공통 preDestroy / postDestroy | `004b0950` / `004b0840` | `004add20` / `004adbe0` |
| 선택 객체의 배치 타입/좌표 복구 | `004d5440` | `0041ea70` |
| 소유자 작업장 목록 제거 | `004900a0` | `00407250` |
| 전역 작업장 목록 제거 | `0044f7a0` | `0045ecc0` |
| 공급 목록 제거 | `00472ff0` | `0048a3c0` |
| 현재 로컬 / 전역 타입 수 감소 | `004c25c0` / `004c25a0` | `0040bc30` / `0040bc10` |
| AI 삭제 통지 래퍼 | `004166e0` | `004d8230` → `004d81f0` |
| 삭제 보상/SP 경계 | `0044c2f0` | `004626d0` |

[함수 목록](../../tools/ghidra/destroylifecycle-functions.json)으로 읽기 전용 Ghidra를 실행했다. 패치 **14개**, CD/추가 10.37 각각 **20개**를 새 `extracted/destroylifecycle/<판본>/creation.c`·`functions.tsv`에 내보냈고 완료/read-only 로그를 확인했다. 이전 독립 내보내기/도구/fixture는 보존했다. 10.37은 CD와 한 바이트의 CD 관련 분기만 다른 별도 실제 PE이며 이 훅 몸체는 동일하다.

이전 후속 후보의 “경로” 표현을 정정한다. `0044c2f0` / `004626d0`는 가상 +0x80 비용 조회와 소유자 보상 비율, SP 및 누적 수입을 처리하는 **삭제 보상 계산**이다. 이번에는 인자 `sid, (flags >> 16) & 15, 0`을 보상 사건으로 전달한다. 실제 SP·비율·수입 갱신을 구현한 것은 아니다. AI 통지와 이동 경로 처리는 별개 후속이다.

## 공통 훅의 장부 순서

`SquidDestroyLifecycle`는 기존 `SquidPostPopState`를 생성 훅과 공유한다. `RawSquidDestroy`·`SidPool`·장부 상태가 같은 풀을 참조해야 한다. 타입/판본 크기를 확인하며 호스트에서 원본 32비트 vtable 주소를 실행하지 않는다.

1. 선택 번호가 자기 SID이면 먼저 선택을 처리한다. abstract/buried(extra & 9)이고 타입 flags2 & `0x30000`이면 원본 좌표/타입을 배치 상태로 남긴다. UI 전체 해제는 사건 콜백이며 논리 선택 번호는 해제한다. 이 단계는 장부 억제와 무관하다.
2. 장부 억제가 없고 flags2 & `0x4200`이면 소유자 작업장 목록에서 자기 번호를 모두 제거한다. abstract/buried도 이 목록은 제거하지만 소유자 0 목록은 유지한다.
3. ordinary(extra & 9 == 0)이면 flags & `0x200000`이 없을 때 삭제 보상 사건을 낸다. 로컬 소유자의 **현재 로컬 타입 수**와 모든 소유자의 **현재 전역 타입 수**를 감소시킨다. `localSecondaryCounts`의 **누적 생산 수는 감소시키지 않는다**.
4. group != 10·flags & `0x200800` == 0·로컬 소유자·flags2 & `0x200000` == 0·두 억제 전역이 모두 0이면 unitLost 소리 사건과 마지막 손실 좌표를 기록한다. 패치의 `silentType`은 **소리만 제외하고 좌표는 기록한다**. CD/10.37에는 이 제외 분기가 없다.
5. flags1 & `0x10000000`이면 공급 목록에서 모두 제거하고 productionDirty를 증가시킨다. 실제 AI 래퍼는 null 포인터에서 자연 반환한다. flags2 & `0x4200`이면 전역 작업장 목록에서 모두 제거하고 productionDirty를 다시 증가시킨다. 번호가 목록에 없어도 dirty는 증가한다.
6. pre 깊이를 감소시킨다. post에서는 ordinary 비용을 차감하고 post 깊이를 감소시킨다. **post 비용은 pre 장부 억제와 무관하다.**

목록 제거는 생존 항목의 순서를 유지하며 모든 중복을 압축한다. 활성 count를 줄이되 inactive 꼬리의 이전 DWORD는 지우지 않는다. 타입 수·dirty·직접 훅 호출의 깊이에는 원본 DWORD 감김을 보존한다.

패치 비용은 loader 접두 구간의 `trunc(floatCost + type*23) - type*23`을 차감한다. CD/10.37은 `trunc(double(intTotalCost) - double(floatCost))`의 low DWORD를 저장한다. 예를 들어 total 123 / cost -0.5이면 패치 post 결과는 124, CD는 123이다. 따라서 소수 비용의 생성→삭제에서 모든 판본이 시작 비용을 복구한다고 가정하지 않는다.

`Hooks()`를 `RawSquidDestroy::Destroy`에 공급하면 **dead → 실제 장부 pre → 종속/전파 → 실제 Unpop → 실제 비용 post → SID Release**로 이어진다. 미복원 종속 가상 메서드/전파는 forward 콜백으로 전달한다. 기존 다리의 BasePre/BasePost에도 직접 연결 가능한 API지만 이번 독립 대조는 공통 root의 수명이며 다리·Graph·장부를 전부 합친 월드를 검사하지 않았다.

## 독립 기계어 대조와 보호

| 입력 | 세 PE × 두 x87 정밀도 |
|---|---:|
| 실제 공통 pre/post 직접 쌍 | 1,152 |
| 실제 destroy → 공통 훅 → Unpop/반납 | 576 |
| 합계 | **1,728** |

합성 타입 74/148/169·표면/공급/작업장/배치/소리 제외 flags·소수/음수/큰 누적 비용·소유자 0..8·로컬/타 소유자·ordinary/abstract/buried·선택·장부/손실 억제·중복/빈/없음 목록·inactive 꼬리를 입력한다. 직접 쌍은 상위 카운터 증가 없이 원본 훅을 호출한 DWORD underflow까지 대조하며, 전체 삭제는 깊이가 균형인 정상 수명이다. 건물 계열 전체 삭제 입력은 **이미 void**이므로 건물 Unpop 완료 증명으로 세지 않는다.

root/주변 일곱 슬롯은 모든 바이트를 직접 비교한다. 전체 32768슬롯 풀·네 해시 배열·spot·최근 삭제 기록·세 통계 표·모든 공급/작업장/Player 목록의 물리 꼬리는 Adler-32로 대조한다. 선택 번호·배치 타입/좌표·손실 좌표·freeCount/목록 머리·꼬리/깊이·dirty/비용도 확인한다. 외부 훅용 합성 vtable만 원본 기본 주소로 출력 정규화한다.

공통 Pre/Post·선택 복구·목록 압축/타입 감소/AI null 래퍼/비용과 실제 destroy/Unpop/Release·CRT 기록 이동은 **원본 명령**이다. 보상/SP·unitLost 소리·선택 UI·전파·로그만 명시적으로 대체한다. 표시 억제·소진 전·Graph 비활성·AI 미부착·비전투 null 큐 범위이며 원본 assert 0, 정상 반환/ESP·x87 제어 워드/TOP를 검사한다. 별도 FS 감사나 게임/OS 실행은 없다.

각 PE의 실제 내부 도달은 Pre/Post 각각 576·Destroy/Release 각각 192·Unpop 104·소유자 목록 제거 232·공급 제거 60·전역 작업장 제거 58·AI 래퍼 140이다. 이 수와 준비 Reset/Create·비용 인코딩 접두 실행은 상위 1,728개에 더하지 않는다. unitLost 소리 대체는 패치 20/CD·10.37 각각 24회이며 선택 UI 384·보상 98·로그 192회다. 새 통합 입력은 client 번호라 전파 효과는 발생하지 않으며, 다른 SID 영역/전파는 앞선 공통 destroy fixture의 범위다.

C++에서는 필요한 Graph 효과·AI 통지·손상 목록·비유한 비용·누락 효과·다른 풀/잘못된 root를 해당 장부 함수 쓰기 전에 거부한다. **상위 Destroy는 이미 dead를 표시한 뒤 훅을 호출하며, 콜백이나 이후 post/Unpop 오류까지 전체 롤백하지 않는다.** 직접 훅 호출 보호와 전체 삭제 원자성을 혼동하지 않는다. 생성→삭제의 같은 상태 공유와 현재 수 복구/누적 생산 수 유지도 콘솔로 검사한다.

```powershell
python -X utf8 tools/decomp_destroylifecycle_oracle.py
python -X utf8 tools/decomp_destroylifecycle_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

생성에는 실제 PE·읽기 전용 내보내기·`extracted/oracle-python`이 필요하다. `--verify`는 저장 SHA/행 수/원본 도달 경계를 감사하며 기계어를 재실행하지 않는다. C++ 검사에는 저장 fixture만 필요하다. 최신 빌드/검사 수와 원본 보호 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다. 로그는 `extracted/destroylifecycle/build.log`·`ctest.log`·`oracle.log`다.

최종 x64 Release 경고/오류 0·CTest 실행 파일 1개 안의 내부 **175개·실패 0**(60.85초), 누적 제한 x86 입력 **87,312개**(85,584 + 1,728)다. 새/기존 공통 삭제·일반 탐색·다리 훅·붕괴 SHA/행 수/호출 경계 감사와 여섯 원본 디렉터리 **2,782개 파일 SHA/목록 동일**을 확인했다. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md는 변경하지 않았다.

## 다음 단계

공통 pre의 표면 Graph 분할/직접 Free와 post의 주변 finder→표면 Add를 기존 RawGraph/RawSquidFinder에 연결한다. 그 뒤 실제 삭제 보상/SP·AI 통지, 종속 form/process의 파생 release/destroy·참조 수명, Flyingshrapnel 파편·walker 낙하·0x2692 실행/취소·끝 칸 변환·raw GameWorld를 잇는다. 다리 배치/Construction·SID 소진·건설/경제/전투/승패도 후속이다.

원본/복사본·업데이터/설치 도구 실행과 모든 창 검사는 호스트 **DESKTOP-HJOW에서 금지**되어 있다. 창 검증은 다른 허용 PC에서 raw 월드 연결 후 TEST01/1-1 생성→선택→삭제/반납→재진입, 파편/소리/낙하·전체화면을 확인한다. 해당 PC의 AGENTS.md/사용자 지시를 먼저 확인한다.
