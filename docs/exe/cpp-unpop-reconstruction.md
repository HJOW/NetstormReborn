# cpppj raw SID의 일반 공간 해제·Take·반납 연결

2026-10-06. [SquidUnpop](../../cpppj/src/o/SquidUnpop.h)을 기존 SidPool/SquidHash/spot에 연결하고, [SquidFactory](../../cpppj/src/o/SquidFactory.h)의 선택적 non-void Take와 공통 firstPop 플래그를 복원했다. **표시가 비활성인 일반 자산 또는 매몰 객체의 경로**다. 원본/복사본 게임 프로세스와 클론 GUI는 실행하지 않았다. 실제 월드와 섬·다리·건물 부착/파생 영역·표시가 켜진 경로는 남았다.

## 정적 근거와 raw 필드 정정

읽기 전용·헤드리스 Ghidra에서 패치 **22개**, CD **17개** 함수를 내보냈다. 결과는 `extracted/lifecycle/<판본>/creation.c`, `functions.tsv`다. 함수 정의가 부족한 곳은 메모리에서만 정의하고 프로젝트 변경은 버렸다. 내보내기 완료 로그와 행 수를 확인했다. `ExportCreation.java`의 종료 코드만으로는 스크립트 성공을 판단하지 않는다.

| 역할/필드 | 패치판 10.78 | CD판 10.72 |
|---|---|---|
| 공통 Unpop | `004afe50` | `004ad0b0` |
| 공통 firstPop | `004ad470` | `004ae0f0` |
| update88 / update8c | `004ad500` / `004ad670` | `004ae380` / `004ae3e0` |
| 표시 비활성 Renderer 반환 | `004990d0` | `00456a00` |
| 좌표 버킷 / 이전 next 쓰기 | `004b2a90` / `004aba70` | `00479d20` / Unpop 인라인 |
| next / type / state | `+4` word / `+10` byte / `+11` byte | 동일 |
| 섬 번호 | **`+8` word** | 동일 |
| x / y | `+14` float / `+18` float | 동일 |
| 실제 프레임 | **`+36` DWORD** | **`+34` byte** |
| extra / 저장 해시 단계 | `+40` / `+33` | `+35` / `+31` |

**이전 자산 생성자 문서의 “+8 frame 초기화”는 필드 이름을 잘못 붙인 것이었다.** SetIsland `004acd20`↔CD `004aca80`과 표시 갱신의 실제 프레임 읽기를 확인하여 **섬 번호 초기화**로 정정했다. playerIsland/battleIsland/anim 생성자가 쓰는 오프셋·폭·값과 기존 기계어 fixture는 이미 맞았으므로 바꾸지 않았다. 이전 감사 도구의 frame 표현은 당시 라벨이며 고정 해시를 보존했다.

## 연결한 동작과 보호 계약

`SquidUnpop`은 별도 객체 복사본 대신 원본 raw SID 슬롯의 상태·좌표·next를 읽고 쓴다. caller는 해당 type 레코드와 기존 배치 상태의 네 단계 해시·spot을 제공한다. 현재 월드의 임시 `SquidId`와 자동 변환하지 않는다.

활성 객체는 void 설정→발자국 spot의 genus low byte AND 해제→현재 좌표의 **0~3단계** 체인 검색→머리 또는 이전 슬롯 next 변경 순서다. 제거 대상의 next·저장 level·좌표/캐시·HP·섬 번호는 보존한다. free/void는 공간을 바꾸지 않고, dead는 해제를 막지 않는다. 표시 비활성 경로의 실제 가상 함수는 쓰기 없이 반환한다. `flags & 0x2000`의 update8c가 update88로 꼬리 호출하는 것까지 반영했다. PE에서 판독한 [UnpopVtables.inc](../../cpppj/src/o/UnpopVtables.inc)의 **151개 메타데이터 행**으로 경로별 지원 여부를 구분한다. 미복원 화면 override를 공통 함수로 대체하지 않는다.

매몰이 아니면서 섬·다리 `flags2 & 6` 또는 건물군 `flags2 & 0x50444200`이면 실제 영역/표면 알림 효과가 필요하므로 변경 전에 거부한다. 일반 spot 갱신과 매몰로 이 효과를 건너뛰는 경로만 지원한다. 화면 표시/dirty가 켜진 단계에 이 어댑터를 그대로 사용하면 안 된다. Renderer·파생 효과 소비는 후속이다.

`SquidFactory(..., unpop)`는 같은 SidPool을 사용하는 어댑터만 받는다. 같은 타입·검증된 최종 vtable의 non-void Take에서 먼저 공간을 해제한 뒤 기존 생성자→type→postTake 순서로 진행한다. 어댑터가 없으면 기존처럼 non-void를 변경 전에 거부한다. **Take는 free list/freeCount를 고치지 않는다.** 일반 서버 Allocate와의 혼용·상위 수신 목록/카운터/패킷 흐름을 완성한 것은 아니다.

`FirstPopFlags`는 extra의 abstract/buried 비트만 읽는다. 둘 다 없으면 1, abstract이면 2, buried이면 4이며 둘 다 있으면 6이다. 다른 extra 비트는 무시한다. 실제 Pop/Activate/postPop의 전송 플래그·서버/클라이언트 처리 전체에 연결한 것은 아니다.

원본 패치판은 체인 미발견 때 assert로 중단한다. 새 보호 경로는 검색 경로·미복원 효과·NaN/범위/발자국·순환/self next를 **쓰기 전에 검사하여** 풀/spot/머리를 보존하고 예외로 보고한다. 원본 assert 이전의 부분 변경/오류 UI는 재현하지 않는다. CD판은 미발견에도 assert 없이 void 전환을 끝내는 차이를 보존했다. contained/form은 이 raw 자산 경로에 넣지 않는다.

## 독립 검증

[decomp_unpop_oracle.py](../../tools/decomp_unpop_oracle.py)는 원본 PE의 SID/실제 생성자/Unpop/공통 firstPop/표시 비활성 가상 함수/Renderer 반환/Take/void 반납 명령을 격리 실행한다. **대체 함수는 없다.** 합성 타입·기존 배치 상태를 입력하며 실제 Pop이나 게임 초기화로 만든 입력은 아니다. 표시 억제 플래그는 1로, 진단 입력은 비활성으로 둔다. 허용 몸체/쓰기 범위·assert 미도달·정상 EIP/ESP/ret N·시간/명령·x87 TOP/제어 워드 복구를 확인했다.

18시퀀스·**3,618회**: Create 34, firstPop 896, Take 768, Unpop 1,536, Release 384. 두 판본×풀 크기 32,768/65,535×서버/클라이언트×x87 `0x027f`/`0x037f` 조합에서 네 해시 단계·머리/중간/꼬리·fractional anchor·다른 저장 level·상태 상위 비트·반복 void/free 해제·non-void Take→반납을 대조한다. SID **50,000/65,534**도 포함한다. 별도 두 시퀀스에서 extra 0~255의 공통 firstPop을 양 판본 **512회** 확인했다. firstPop 896회 중 나머지 384회는 수명 시퀀스 안의 호출이다.

실제 non-void Unpop은 **각 판본 384회**, 합계 768회다(Take 내부 호출 포함). 각 판본의 실제 공통 update88 384회/update8c 64회, assert 도달 0회다. CD 체인 미발견 16개 활성 입력도 실제로 실행했다. 준비 Reset 18회와 가상 주소 표 151행은 3,618회에 더하지 않는다. 기존 생성자 표 359행도 별도다.

[UnpopTests.cpp](../../cpppj/tests/UnpopTests.cpp)는 대상 raw 슬롯의 모든 바이트를 직접 대조하고 전체 풀·삭제 기록·네 단계 **86,272머리**·**65,536 spot**의 Adler-32를 비교한다. 추가 보호 검사는 변경 전 거부·어댑터 풀 혼용 방지·CD 미발견 차이를 확인한다. Release 빌드 경고/오류 0, CTest 한 실행 파일 내부 **112개 검사 통과**. 누적 제한 x86 입력은 **46,156행**이다. 새 기록과 기존 derived/creation/SID의 원본/도구/fixture SHA-256·행 수 검사도 통과했다. [검증 기록](../../cpppj/recovery-unpop-evidence.json).

**남은 범위:** 표시가 켜진 dirty/그리기·별도 grid/표면 알림·섬/다리/부착 건물 효과·파생 Unpop/삭제·의존 객체·실제 참조/수신 수명·SID 소진 복구·firstPop의 실제 Pop 연결·postPop/Activate·원본 배치·GameWorld 연결. 단순 void 슬롯 반납 검증을 파생 destructor 전체나 미션 완주로 세지 않는다. 기존 공간 등록 878회는 계속 의존성 계약 대체를 포함한 별도 검증이다.

## 창 없는 재현과 다른 PC 인수인계

기존 SID/creation/derived 내보내기와 지정 Python 분석 환경을 먼저 준비한다. 다음은 프로젝트를 저장하지 않는 읽기 전용 내보내기다.

```powershell
& ./tools/ghidra/run_script.ps1 -Script ExportCreation.java -ScriptArgs @('extracted/lifecycle/originals','004afe50','004ad500','004ad670','004ad470','004990d0','004210a0','0049a840','00481510','0040e800','004afd30','004e49c0','0040da00','00425df0','004ac1e0','0040eaf0','004ace40','0049a900','00419850','004b2a90','004aba70','004acd20','004ac560')
& ./tools/ghidra/run_script.ps1 -Edition originalCD -Script ExportCreation.java -ScriptArgs @('extracted/lifecycle/originalCD','004ad0b0','004ae380','004ae3e0','004ae0f0','00456a00','004abf50','004abae0','00442f30','004abac0','004acfc0','004f161c','004acb00','004ae480','004442b0','00479d20','004aca80','004abfb0')
python -X utf8 tools/decomp_unpop_oracle.py
python -X utf8 tools/decomp_unpop_oracle.py --verify
python -X utf8 tools/decomp_derived_oracle.py --verify
python -X utf8 tools/decomp_creation_oracle.py --verify
python -X utf8 tools/decomp_sid_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

C++ 검사는 고정 fixture만 읽으므로 원본 실행 파일/분석 환경/창이 필요 없다. 이 PC에서는 원본/복사본과 `--run`, 창을 띄우는 world/window/renderer/menu 스모크를 실행하지 않는다. 남은 raw Pop/수명/월드 연결 뒤 다른 PC에서 TEST01/1-1의 표시·선택·이동·해제·재진입을 확인한다. 현재 임시 월드와 raw SID는 별개이므로 기존 창 검사 이력을 이번 raw 연동 검증으로 세면 안 된다. 다음 순서는 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 기록했다.
