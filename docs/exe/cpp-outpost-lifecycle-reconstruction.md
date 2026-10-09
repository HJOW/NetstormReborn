# outpost의 배치 추가 목록 등록·삭제 준비

2026-10-09. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** AGENTS.md·두 인계 문서·cpppj 문서를 읽고 현재 호스트 일치를 확인했다. 새 `outpostlifecycle` 4/2/2개를 같은 PC에서 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창은 실행하지 않았다.

[전체 MayPlace 연결](cpp-canon-mayplace-reconstruction.md) 뒤에 남은 [Player 기준점](cpp-player-anchor-reconstruction.md)의 별도 추가 목록을 추적했다. 이 목록은 **자산 타입 126 `outpost`의 postPop/preDestroy**에서 등록·제거한다. `factories`나 소유자별 `ownerFactories`와 합치지 않는다. outpost는 등록과 제거 때 별도의 거리 후보 목록도 함께 갱신한다.

| 몸체/자료 | 10.78 | CD·추가 10.37 |
|---|---|---|
| outpost postPop | `004557c0` | `0047b680` |
| outpost preDestroy | `00455840` | `0047b790` |
| Player 배치 추가 Array | `0055a70c` | `005670e0` |
| 별도 거리 후보 Array | `0059a9d8` | `00565ae0` |
| 지역 소유 투표/갱신 | `00455380(x,y,0)` | `0047b3c0(x,y)` |
| 작업장 postPop 부모 | `004538f0` | `0047a4b0` |
| Damageable preDestroy 부모 | `0044b4b0` | `004615e0` |
| Array 중복 없는 추가/제거 | `00414450` / `0040ea00` | wrapper 내부에 인라인 |

## 복원 계약

[`RawOutpostLifecycle`](../../cpppj/src/o/RawOutpostLifecycle.h)은 두 목록을 호출자가 소유하는 `SquidPostPopList` 참조로 받는다. Player locator의 additional 인자에 **동일한 목록**을 공급하면 목록 변화를 다음 조회에 즉시 반영한다. 호출자가 outpost의 파생 경로를 선택해야 하며 일반 Pop의 지원 가상 표를 넓히지 않았다.

postPop은 `flags & 1`이고 raw extra의 `& 9`가 0일 때만 추가한다. 추가는 목록마다 독립적으로 처리하며 **가득 찬 목록은 그대로**, 여유가 있는 목록은 이미 SID가 있으면 추가하지 않는다. 등록 조건과 무관하게 같은 flags의 작업장 부모 호출은 항상 실행한다.

preDestroy는 extra `& 9`가 0이면 flags와 무관하게 두 목록의 **모든 중복을 순서대로 압축**한다. 새 count 뒤의 꼬리는 지우지 않는다. abstract/buried는 목록과 지역 갱신만 생략하고 Damageable 부모 호출은 그대로 실행한다. 두 목록 처리가 끝난 뒤 현재 raw x/y를 지역 효과에 전달하며, 그 뒤 부모를 호출한다. 지역 콜백이 extra/좌표를 바꿔도 이미 선택한 접두 분기를 다시 검사하지 않는다. 좌표 전달은 NaN과 부호 있는 0의 비트도 보존한다.

알려진 잘못된 count/필수 훅/동일 목록 연결을 검사하고 부모의 순수 사전 검증을 쓰기 전에 수행한다. 실제 읽지 않는 목록은 abstract 객체/두 번째 Pop에서 미리 검사하지 않는다. 지역 콜백의 실패나 변경 이후 모든 예외를 되돌리는 트랜잭션을 주장하지 않는다.

[`MakeOutpostDamageablePreDestroyHooks`](../../cpppj/src/o/RawOutpostLifecycle.cpp)는 부모 삭제 경계를 기존 [Damageable 접두](cpp-damageable-predestroy-reconstruction.md)에 연결하고 다른 풀의 참조를 거부한다. **이 접두 단계의 원본 대조에서는 지역 소유 투표와 작업장 postPop 본문을 대체**했다. 후속 [지역 소유 투표](cpp-region-ownership-reconstruction.md)는 `MakeOutpostRegionOwnershipHooks`로 실제 모듈에 연결했다. 지형 flood/도장·작업장 postPop/Regular·실제 outpost 일반 Pop 전체는 남았다.

## 독립 원본 관찰과 C++ 연결 검사

[`decomp_outpostlifecycle_oracle.py`](../../tools/decomp_outpostlifecycle_oracle.py)는 세 PE 각각 **768개**, 총 **2,304개**를 두 x87 제어 워드 `0x027f`/`0x037f`로 실행했다. 두 파생 wrapper와 패치의 실제 Array helper/CD 인라인 목록 처리를 **ret 4 정상 반환까지** 실행한다. 지역 통지·작업장 postPop·Damageable preDestroy의 진입만 명시 대체한다. 부모 this/flags와 지역의 실제 좌표 비트/패치 세 번째 0 인자를 확인한다.

입력은 Pop/삭제 × extra 6종 × flags 4종 × 목록 장면 8종 × 지역 콜백 변경 2종이다. 서로 다른 빈/중복/가득 찬/거의 찬 목록과 비활성 `0xabababab` 꼬리를 공급한다. 원본의 목록 count/저장소 전체·외부 사건 순서/당시 count·최종 raw 전체를 관찰한다. 기대 등록·삭제 결과를 Python/C++ 알고리즘으로 만들지 않는다.

각 PE 정상 반환 **1,536회**, 부모 대체 **1,536회**, 지역 대체 **672회**다. 패치 Array 추가 **576회**, 제거 **768회**이며 CD/10.37은 해당 처리가 인라인이다. ESP·EBX/ESI/EDI/EBP·x87 제어 워드/TOP·허용 코드/쓰기·assert/OS 0·SHA/입력/행·실제 진입/대체 수를 감사한다. [fixture](../../cpppj/tests/fixtures/outpostlifecycle-x86.tsv)와 [근거 JSON](../../cpppj/recovery-outpostlifecycle-evidence.json)은 LF로 저장한다. 누적 독립 fixture는 **344,146개**다.

새 [C++ 검사](../../cpppj/tests/OutpostLifecycleTests.cpp) 5개는 세 PE 전체 재생, 등록/중복 등록/삭제/재등록→실제 Player 종속·그래프·기준점 조회 연결, 필수 경계/손상된 두 번째 목록/다른 풀/부모 사전 거부/조기 읽기 생략을 검사한다. 실제 Damageable의 붕괴·소리·공통 부모 호출도 연결한다. Player의 작업장 후보가 다른 그래프라서 제외되어도 실제 종속이 linked를 켜면 등록된 outpost만 추가 후보가 되고, 제거 직후 반환이 0으로 바뀐다.

## 재현과 후속

```powershell
tools/ghidra/export_functions.ps1 -Name outpostlifecycle
python -X utf8 tools/decomp_outpostlifecycle_oracle.py
python -X utf8 tools/decomp_outpostlifecycle_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

최종 검사 수/시간과 로그는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 이번 완료 항목을 따른다. 기존 fixture/감사 도구·보호 파일·AGENTS.md·dotnetpj를 수정하지 않았다.

[지역 소유 투표](cpp-region-ownership-reconstruction.md)는 후속 단계에서 복원했다. 다음은 **지역 지형 flood/도장·작업장 postPop 본문/Regular와 실제 저장 맵/raw 세계의 outpost 생성·삭제 경로 연결**이다. 전체 MayPlace는 제한 원본 대조까지 완료했지만, 실제 맵·비표면 raw 객체·공간/Player 장부 수명은 여전히 연결할 부분이 있다. 사제 Pop·보호막 생성/회복 예약·Carrier와 GUI 건설·경제·전투·승패도 남았다. 목록 접두 검증을 실제 게임/미션 완주로 해석하지 않는다. 자산 전수·장시간 변이·최대 지도·창/픽셀 회귀는 계속 인계한다.
