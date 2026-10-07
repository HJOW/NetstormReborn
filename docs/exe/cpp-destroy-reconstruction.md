# cpppj 공통 Squid 삭제와 실제 공간 해제·SID 반납

2026-10-07. **1차 기준은 10.78**이다. [일반 공간 탐색](cpp-rawfinder-reconstruction.md) 다음으로 공통 destroy의 dead 표시·가상 훅·종속 체인·전파/선택·Unpop·반납 순서를 복원했다. 기존 [다리 삭제 훅](cpp-bridgeeffects-reconstruction.md) 내부의 링크 삭제를 실제 공통 destroy에 연결해 콘솔에서 대조했다. 호스트 **`DESKTOP-HJOW`에서 원본/복사본·업데이터/설치 도구·클론 창을 실행하지 않았다.**

소스: [RawSquidDestroy.h](../../cpppj/src/o/RawSquidDestroy.h), [RawSquidDestroy.cpp](../../cpppj/src/o/RawSquidDestroy.cpp). 검사: [RawDestroyTests.cpp](../../cpppj/tests/RawDestroyTests.cpp). 독립 출력: [destroy-x86.tsv](../../cpppj/tests/fixtures/destroy-x86.tsv), [근거 기록](../../cpppj/recovery-destroy-evidence.json), [생성/감사 도구](../../tools/decomp_destroy_oracle.py).

**복원 범위는 공통 삭제의 제어 흐름과 지원 자산의 실제 Unpop/반납이다.** 공통 pre/postDestroy의 그래프·목록·통계·소리/삭제 보상·AI 효과와 종속 객체의 파생 가상 메서드는 콜백 계약이다. 기존 GUI GameWorld는 임시 객체 모델이며 이 어댑터에 연결하지 않았다. 미션 완주나 전체 파생 삭제 복원으로 세지 않는다.

**2026-10-07 후속:** [공통 pre/postDestroy 장부](cpp-destroylifecycle-reconstruction.md)의 선택 복구·작업장/공급/소유자 목록·현재/누적 통계·손실 좌표·비용을 실제 삭제/반납에 연결해 새 x86 1,728개로 대조했다. 아래 1,992개·169개와 공통 훅 대체 설명은 당시 독립 기록이다. Graph·보상/SP·AI·파생 삭제·raw GameWorld는 후속이다.

## 원본 함수와 재디컴파일

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 공통 destroy | `004af780` | `004ab7e0` |
| client SID 판단 | `00441fa0` | destroy에 인라인 |
| 공통 preDestroy | `004b0950` | `004add20` |
| 공통 postDestroy | `004b0840` | `004adbe0` |
| Unpop | `004afe50` | `004ad0b0` |
| SID 반납 | `004abf60` | `004ab250` |
| 삭제 전파 | `004ac260` | `004abb40` |
| 선택 번호 조회 / 해제 | `004d5430` / `004d5c60` | `0041ea30` / `0041ed70` |
| 파편 효과 래퍼 / 생성 | `00460600` / `004604a0` | `00454810` / `00454640` |

새 [내보내기 목록](../../tools/ghidra/destroy-functions.json)으로 읽기 전용 Ghidra를 실행했다. 패치 9개·CD/10.37 각 6개를 `extracted/destroy/<판본>/creation.c`·`functions.tsv`에 내보냈다. 세 `ghidra.log`의 완료 및 read-only 프로젝트 처리를 확인했다. 이전 내보내기/독립 도구/fixture는 변경하지 않았다. 10.37은 CD와 동일한 코드 배치이지만 별도 실제 PE를 읽어 대조했다.

디컴파일의 종속 destroy/전파/postDestroy 인자에 나타나는 **`unaff_retaddr`는 실제 명령의 스택 흐름상 삭제 flags**다. 반환 주소를 전달하는 코드로 옮기지 않았다. 원본 서버 flag(`00540bc0` / CD `00540a28`)와 client 번호 경계를 확인하고 실제 flags의 변경/전달을 독립 x86으로 대조했다.

## 삭제 순서와 콜백 계약

1. dead(state & 2)이면 즉시 반환한다. 정상 객체에는 **dead를 먼저 표시**하므로 preDestroy 안에서 같은 객체를 다시 삭제해도 중첩 효과/반납이 없다.
2. 서버가 client SID를 삭제하면 flags에 `0x10`을 더한다. 서버가 아닌 측의 서버/예측 SID 삭제에는 수신 flag `8`이 필요하다.
3. `curDestroy` 증가 → 가상 +0x14 preDestroy → 카운터 원상 복구 검사를 한다. pre 훅은 `CompletePreDestroy()`를 정확히 한 번 호출한다. 같은 어댑터의 중첩 삭제가 이 카운터를 공유한다.
4. **pre 이후의 +6 WORD head**부터 종속 객체를 순회한다. contained(state & 8) 또는 form(type < 70)이어야 한다. **next(+4 WORD)는 가상 +0x1c release(parent SID) 전에 저장**한다. release가 free로 바꾸지 않은 종속 객체만 가상 destroy(flags | `0x40`)에 넘긴다.
5. flags & `0x50`이 0인 서버의 비client 삭제는 receiver -1·command -2·payload 0·flags | 8로 전파한다. 현재 선택 번호가 자기 번호이면 선택 해제 사건을 낸다.
6. 현재 void(state & 4)가 아니면 **기존 `SquidUnpop::Unpop(..., 0)`**을 실행한다. 표시 대상 nullptr는 원본 표시 억제 경로다. type/void는 훅 이후 다시 읽는다.
7. `curPostDestroy` 증가 → 가상 +0x18 postDestroy → 카운터 복구 검사 후 **기존 `SidPool::Release`**를 호출한다. post 훅은 `CompletePostDestroy()`를 정확히 한 번 호출한다. 전체 슬롯 초기화·이전 타입 보존·FIFO 꼬리·영역별 최근 삭제 기록이 실제 구현으로 이어진다.

`SquidDestroyHooks.emit`이 미복원 가상 효과/전파/선택 해제를 연결하고 `selected`가 현재 선택 번호를 공급한다. 이 콜백은 같은 풀을 변경할 수 있다. 종속 destroy 사건은 공통 어댑터를 form 객체에 직접 호출하라는 의미가 아니라 **해당 파생 가상 메서드**를 연결하라는 계약이다. Pre/Post 완료 호출은 깊이 감소만 수행하며 공통 훅 전체의 대체 구현이 아니다.

다리 연결 검사에서는 실제 `RawSquidFinder`·`RawBridgeLifecycle`를 사용했다. `DestroyLink` 사건이 `RawSquidDestroy::Destroy(link, 0, hooks)`에 재진입하고 링크의 공간 해제/반납으로 같은 버킷의 next/앞 슬롯이 바뀐다. finder가 미리 저장한 next를 유지하여 뒤 후보를 계속 읽는다. 이 검사에서 링크의 Pre/Post는 깊이 감소 콜백이며, 바깥 다리의 BasePre/BasePost도 그 범위다. 다른 다리의 훅이 중첩 탐색을 시작할 때는 별도 finder가 필요하다.

## 파편 효과의 의미 정정

기존 문서에서 `00460600` → `004604a0`를 “제거 통지 목록”으로 설명했지만, 새 디컴파일에서는 **Flyingshrapnel 파편/입자 생성**으로 확인했다. 래퍼는 두 번째 인자 2를 넘기며 생성 함수는 0x68바이트를 할당하고 `004603a0` 생성자(vtable `0050a4f0`)와 `00460000` 위치/파편 목록 처리를 연결한다. 실제 SHP 경계와 zOrder도 읽는다.

호환성을 위해 기존 사건 이름 `BridgeLifecycleEffect::NotifyRemoval`은 유지한다. 이 사건의 실제 후속 대상은 파편 효과이며 이 단계에서 구현하거나 출력하지 않는다. 이전 독립 도구/fixture의 N 사건도 그대로 보존한다. 공통 pre/postDestroy에 있는 작업장/소유자 목록 및 통계 제거는 이 파편 함수와 별개다.

## 독립 대조와 보호

| 입력 종류 | 새 x86 입력 |
|---|---:|
| 공통 destroy·종속 체인·전파/선택·실제 Unpop/반납 | 1,800 |
| 실제 다리 훅·일반 finder·중첩 링크 삭제/반납 | 192 |
| 합계 | **1,992** |

세 실제 PE × x87 53/64비트다. client/server/predictable 번호·서버 권한·flags 0/8/0x10/0x40/0x50/0x800/0x2000000/0xffffffff·void/dead·form/contained 종속·release가 현재 next를 지우거나 free로 바꾸는 효과·pre의 head/void 변경·선택 해제·다리 훅 내부 링크 삭제를 포함한다. pre에서 void만 켜거나 종속 release에서 free만 켜는 사례는 **명시적인 부분 가상 효과 대체**이며 실제 월드의 완전한 공간/풀 계약을 뜻하지 않는다.

root/주변 슬롯은 바이트별 대조하고 전체 32768슬롯 풀·네 해시 배열·spot·두 삭제 기록의 Adler-32, freeCount·예측 커서·목록 머리/꼬리·선택·두 훅 깊이를 비교한다. 합성 외부 가상 호출용 vtable 주소만 기본 vtable 기록값으로 정규화한다. 다른 바이트/체크섬은 원본 결과를 그대로 저장한다.

각 PE의 실제 내부 호출은 Destroy 712 / Unpop 440 / Release 600 / BridgePre 48 / BridgePost 48 / Begin 96 / Next 704다. 준비 Reset/Create/Take와 중첩 내부 호출은 상위 1,992개에 더하지 않는다. 원본 assert 0·정상 반환/ESP·x87 제어 워드/TOP를 확인했다. 이 실행기에 별도 FS 목록 감사는 추가하지 않았다.

**대체 경계:** 공통 Pre/Post와 종속 release/destroy, 전파·선택·로그·파편·소리·walker 낙하만 진입점 대체다. 공통 destroy·다리 Pre/Post·일반 Begin/Next·Unpop·SID 반납과 삭제 기록의 CRT 겹친 이동은 실제 원본 명령이다. 타입/SHP·해시 등록은 합성 입력이며 표시 억제·소진 전·비전투 null 큐 범위다. 외부 OS 호출/게임 프로세스/창은 없다.

새 C++ 보호는 잘못된 자기 SID·다른 풀 연결·free/contained/form root·누락 콜백·서버 권한을 dead 표시 전에 거부한다. 종속의 free/상태/순환/풀 밖 번호와 훅 완료 누락은 해당 단계에서 예외로 중단한다. **훅 이후 오류나 미복원 Unpop 효과 거부까지 전체 롤백하지 않는다.** 원본의 assert/무한 루프를 호스트에서 재현하는 계약은 아니다.

```powershell
python -X utf8 tools/decomp_destroy_oracle.py
python -X utf8 tools/decomp_destroy_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

생성에는 원본 PE·읽기 전용 내보내기·`extracted/oracle-python`이 필요하다. `--verify`는 PE/도구/몸체/fixture SHA·고정 입력/실제 호출 수·assert 0을 감사하며 기계어를 재실행하지 않는다. C++ 검사에는 저장된 fixture만 필요하다. 최종 검사 수/보호 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)와 [cpppj README](../../cpppj/README.md)를 따른다. 로그는 `extracted/destroy/build.log`·`ctest.log`에 보존한다.

최종 x64 Release 경고/오류 0·CTest 내부 **169개·실패 0**(68.81초)다. 기존 163개에 새 6개를 더했으며 누적 제한 x86 입력은 **85,584개**(83,592 + 1,992)다. 최초 C++ 합성 입력의 타입 개수와 비종속 next를 정정한 뒤 통과했으며 원본 기계어 기대값은 바꾸지 않았다. 새 fixture/기록은 SHA가 Git 체크아웃 뒤에도 유지되도록 LF로 생성한다. 기존 탐색/삭제 훅/붕괴 감사와 여섯 원본 디렉터리 **2,782개 파일 SHA/목록 동일**도 확인했다. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md는 변경하지 않았다.

## 바로 다음

공통 선택/목록/통계/손실 좌표/비용은 [후속 장부 훅](cpp-destroylifecycle-reconstruction.md)에서, Graph 분할/Free·주변 표면 Add는 [삭제 Graph](cpp-destroygraph-reconstruction.md)에서, 삭제 보상 `0044c2f0`/CD `004626d0`는 [보상 복원](cpp-reward-reconstruction.md)에서 연결했다. 남은 일은 AI 통지, 종속 form/process의 가상 release/destroy와 파편 `004604a0`, walker vtable +200/칸 위 carrier·이벤트 0x2692의 실행/취소·끝 칸 변환이다. 다리 배치/Construction·SID 소진·raw GameWorld·건설/경제/전투/AI/승패도 남았다.

호스트 **`DESKTOP-HJOW`의 기존 창 제한을 유지한다.** 다른 허용 PC에서 raw 월드 연결 후 다리 붕괴→그래프/공간/반납→파편/소리/낙하와 TEST01/1-1 생성·삭제·재진입을 검사한다. 이전 작업의 일회성 창 허용은 이번에 적용하지 않았다.
