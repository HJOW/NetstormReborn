# cpppj 자산 파생 생성자 복원

2026-10-06 생성자 복원 기록. [SquidFactory](../../cpppj/src/o/SquidFactory.h)의 기존 base 경로에 실제 자산 생성자를 연결했다. 패치판 **82개 생성자·98개 타입 연결**, CD판 **71개 생성자·81개 타입 연결**이다. 생성자 주소가 0인 기존 fallback도 유지한다. 원본/복사본 게임 프로세스와 클론 GUI는 실행하지 않았다. 후속 [raw 일반 공간 해제·non-void Take](cpp-unpop-reconstruction.md)를 추가했고 아래 +8 프레임 표기는 섬 번호로 정정했다. 아래 108개 검사 수는 당시 범위다.

## 원본 근거와 적용 범위

기존 [생성자 주소 표](cpp-creation-reconstruction.md)에서 자산 타입(70 이상)의 주소를 골라 읽기 전용·헤드리스 Ghidra로 내보냈다. 패치 84개, CD 75개 함수이며 CD에는 Bomb 보조 생성자 `0045f840`도 포함한다. 패치 HP 보조 함수 `004adca0`은 별도 내보내기다. 결과는 `extracted/derived/<판본>/creation.c`, `functions.tsv`, `extracted/derived/patch-helpers/`에 있다. 기존 프로젝트의 함수 정의가 부족한 곳은 메모리에서만 정의하고 변경을 버렸다. 전체 대응 2,496쌍·검토 앵커 24쌍의 기존 기록은 그대로다.

실제 생성자는 raw SID 슬롯 주소를 계산하여 다음 쓰기를 수행한다. [DerivedConstructors.inc](../../cpppj/src/o/DerivedConstructors.inc)는 순서·폭·OR 마스크를 보관하며 원본 주소를 호스트 함수 포인터로 사용하지 않는다.

| 효과 | 패치판 | CD판 |
|---|---|---|
| vtable 기록 | `+0`의 4바이트 | 동일 |
| 일부 자산 플래그 | `+40 OR 2` | `+35 OR 2` |
| playerIsland/battleIsland/anim 섬 번호 | `+8`의 word를 0으로 | 동일 |
| anim HP | `+26`의 **4바이트를 0으로** | `+26`의 **2바이트를 0으로** |
| outpost | 플래그·최종 vtable 쓰기 | 플래그·factory vtable·outpost vtable 쓰기 |
| Bomb 계열 | 해당 생성자의 쓰기 | 실제 Bomb 보조 생성자 뒤 최종 vtable 쓰기 |

생성자 자체는 type 바이트, owner, zOrder, 풀 목록/카운터를 초기화하지 않는다. `Construct(type,sid)`는 생성자 쓰기만 적용한다. `Create`는 Allocate→생성자→type 바이트→postCreate 순서를 유지한다. `Take`는 사용 중인 슬롯이면 기존 void Unpop을 호출하고, free/dead 해제·void 설정→생성자→type→postTake를 수행한다. 지원한 최종 vtable의 `+40/+44/+72`가 각각 기존 base postCreate/postTake/Unpop인지 실제 PE에서 확인했다. 다른 가상 메서드와 유닛의 갱신·전투 동작은 복원하지 않았다.

공통 postTake 자체는 HP를 보존한다. 다만 앞서 호출되는 **anim 생성자는 HP를 지우므로 anim Take 전체가 HP를 보존한다고 설명하면 안 된다.** CD판은 HP 뒤의 두 바이트를 보존한다. 이미 할당한 Take는 같은 타입·검증된 최종 vtable·void 상태만 지원한다. non-void 공간 해제와 contained 수신은 변경 전에 거부한다.

원본의 platform(153)·lightning(159) 생성자는 null을 반환하고 daisExtraFrames(169)는 assert로 중단된다. 원본 주소 표에는 남기되 C++ Create/Construct/Take는 변경 전에 예외로 거부한다. 원본의 null 후 접근/오류 UI를 재현하지 않는다. form/process와 알 수 없는 생성자 주소도 지원하지 않는다. 패치 fakeThreeByThreeSurface(162)의 Create 금지는 유지한다.

## 독립 검증과 한계

[decomp_derived_oracle.py](../../tools/decomp_derived_oracle.py)는 실제 PE를 Unicorn 메모리에 복사하여 확인한 몸체에서만 실행한다. **대체 함수 없이**, ctor 반환 포인터·EIP/ESP·쓰기/시간/명령 범위·assert 미도달을 확인한다. 두 오염 패턴의 생성자 쓰기와 mov/or 명령을 해독하며, 컴파일러의 load→register OR→store도 슬롯 OR로 확인한다. vtable·섬 번호·HP·플래그 이외의 쓰기는 거부한다(기존 감사 도구의 frame 라벨은 위 정정 참고). 153개 생성자의 준비 감사 306회와 Reset 10회는 아래 fixture 호출 수에 더하지 않는다.

8시퀀스(두 판본×서버/클라이언트×mana 약화 옵션), **6,028회**: Create 716, Construct 1,432, postCreate 716, postTake 716, Take 2,448. 지원하는 **179개 타입 연결을 모두** 실행한다. 기존 free 슬롯 수신과 반복 void Take, 오염 payload/상태 상위 비트, HP 경계/음수, 예측/클라이언트 flags를 포함한다. 대상 슬롯은 모든 바이트를 직접 비교하고 전체 풀은 Adler-32와 머리/꼬리/카운터를 대조한다. 실제 void Unpop도 패치 948회/CD 784회 호출하며 대체하지 않았다.

[CreationTests.cpp](../../cpppj/tests/CreationTests.cpp)의 추가 검사는 fixture 재생, null/assert/unknown 주소의 변경 전 거부, anim의 판본별 HP 폭과 기존 void 반납 연결, 독립 Construct의 호출자 필드 보존/잘못된 슬롯 거부다. **Release 빌드 경고·오류 0, CTest 한 실행 파일 내부 108개 검사 통과.** 원본/도구/표/fixture SHA-256과 행 수는 [검증 기록](../../cpppj/recovery-derived-evidence.json) 및 `--verify`로 확인한다. 기존 base 생성 검증 기록은 수정하지 않았다. 누적 제한 x86 입력은 **42,538행**이고 생성자 표 359행은 별도다.

이 검증은 합성 타입 필드와 이미 확보한 메모리에서의 생성자/공통 초기화/void 수신이다. 실제 자산 파서 전체, malloc/소진 복구, non-void Unpop·파생 영역/삭제/의존 객체·실제 참조 수명·네트워크 목록/카운터 호출 순서·GameWorld 연결은 남았다. Take는 free list/freeCount를 변경하지 않으므로 일반 서버 Allocate와의 혼용 흐름이 완료된 것은 아니다. 생성자 복원을 유닛 클래스 전체·미션 완주·원본 화면 일치로 해석하지 않는다.

## 창 없는 재현과 실행 인수인계

먼저 기존 SID/base 내보내기가 준비되어 있어야 한다. 자산 주소는 고정 fixture에서 판본별로 골라 같은 Ghidra 스크립트에 전달한다.

```powershell
# 각 판본의 자산 생성자만 골라 읽기 전용으로 내보낸다.
foreach ($taskEdition in @('originals','originalCD')) {
    $taskAddresses = @(Get-Content -Encoding UTF8 cpppj/tests/fixtures/constructors-x86.tsv |
        Where-Object { $_.StartsWith($taskEdition + "`t") } | ForEach-Object {
            $taskRow = $_ -split "`t"
            if ([int]$taskRow[1] -ge 70 -and [int]$taskRow[2] -gt 0) { '{0:x8}' -f [int]$taskRow[2] }
        } | Select-Object -Unique)
    if ($taskEdition -eq 'originalCD') { $taskAddresses += '0045f840' }
    & ./tools/ghidra/run_script.ps1 -Edition $taskEdition -Script ExportCreation.java -ScriptArgs (@("extracted/derived/$taskEdition") + $taskAddresses)
}
& ./tools/ghidra/run_script.ps1 -Script ExportCreation.java -ScriptArgs @('extracted/derived/patch-helpers','004adca0')
python -X utf8 tools/decomp_derived_oracle.py
python -X utf8 tools/decomp_derived_oracle.py --verify
python -X utf8 tools/decomp_creation_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

Ghidra wrapper는 스크립트 예외가 있어도 종료 코드가 0일 수 있으므로 완료 로그와 함수 수를 확인한다. C++ 검사는 저장된 fixture만 읽으며 원본/분석 환경/창이 필요 없다. 여기서는 `--run`과 창 스모크를 실행하지 않는다. 다음 non-void 수명/월드 연결 순서와 다른 PC의 TEST01/1-1 창 검증은 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 남겼다.
