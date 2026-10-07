# cpppj 일반 공간 탐색과 다리 삭제 훅 연결

2026-10-07. **1차 기준은 10.78**이다. [다리 삭제 전후 훅](cpp-bridgeeffects-reconstruction.md)의 다음 항목이었던 일반 사각형 탐색 Begin/Next를 복원하고 `MakeBridgeLifecycleHooks`로 연결했다. 호스트 **`DESKTOP-HJOW`에서 게임·업데이터·설치 도구·클론 창을 실행하지 않았다.**

소스: [RawSquidFinder.h](../../cpppj/src/o/RawSquidFinder.h), [RawSquidFinder.cpp](../../cpppj/src/o/RawSquidFinder.cpp), [RawBridgeLifecycle.cpp](../../cpppj/src/o/RawBridgeLifecycle.cpp). 검사: [RawFinderTests.cpp](../../cpppj/tests/RawFinderTests.cpp). 독립 출력: [rawfinder-x86.tsv](../../cpppj/tests/fixtures/rawfinder-x86.tsv), [근거 기록](../../cpppj/recovery-rawfinder-evidence.json), [생성/감사 도구](../../tools/decomp_rawfinder_oracle.py).

**2026-10-07 후속:** [공통 destroy](cpp-destroy-reconstruction.md)의 실제 Unpop/SID 반납·종속 체인·전파/선택과 다리 훅 내부 링크 삭제를 새 x86 1,992개로 대조했다. 공통 Pre/Post의 Graph/목록/통계·종속 파생 메서드·파편/낙하/소리·raw GameWorld는 남았다. 아래 1,776개/163개와 외부 삭제 효과 대체 범위는 당시 기록이며 기존 fixture는 보존했다.

## 원본 함수와 자료

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 일반 Begin | `004b16d0` | `004eae20` |
| 일반 Next | `004b1810` | `004eafe0` |
| 정수 좌표 → 버킷 범위 | `00434380` | Begin/Next에 인라인 |
| 타입 번호 조회 | `0049a840` | 인라인 |
| 발자국/높이 확장 | `0049a900` | `004442b0` |
| 지정 단계 버킷 주소 | `004b2b00` | `00479d90` |
| 기본 true 가상 필터 | `0044daa0` | `0040f180` |
| CRT `_ftol` | `004e49c0` | `004f161c` |

읽기 전용 Ghidra로 패치 8개·CD/10.37 각 6개를 새 `extracted/finder/<판본>/creation.c`·`functions.tsv`에 내보냈다. 같은 디렉터리의 `ghidra.log` 세 완료 로그를 확인했고 프로젝트 변경을 저장하지 않았다. 이전 삭제/붕괴 내보내기와 독립 기대값은 보존했다. 원본 파일을 OS 프로세스로 실행하지 않고 내보낸 실제 함수 몸체만 Unicorn의 격리된 x86 메모리에서 실행했다.

## 사각형·단계·체인

왼쪽/위는 0 이상으로 자른다. 오른쪽/아래의 `-1`은 보드 끝이며 최종 상한은 255다. 일반 해시는 **1·2·4·16칸** 크기의 네 단계다. 시작 버킷은 정수 좌표를 단계 크기로 나누고 배열 끝으로 자른다. 종료 버킷은 같은 계산 뒤 **한 칸을 더한 뒤** 배열 끝으로 자른다. Begin은 첫 Next까지 수행한다.

탐색은 **단계 0→3, 단계 안의 y→x, 버킷 안의 raw +4 WORD next** 순서다. flag 1은 0단계를 건너뛰고, flag 4는 기본 발자국 높이에 타입의 추가 높이(패치 +1e4 / CD +1c4)를 더한다. 추가 높이는 `heightExtensions`로 명시적으로 받는다. 기존 flag 8의 [표면 이웃 탐색](cpp-surface-reconstruction.md)은 별도 경로이며 두 탐색기를 합치지 않았다.

후보의 extra & 8(buried)만 제외한다. **free/dead/void/abstract를 임의로 추가 필터링하지 않는다.** 원본 기본 가상 필터는 항상 true다. C++의 선택적 filter 콜백은 다른 가상 필터를 연결하기 위한 계약이며 그 원본 파생 필터들까지 복원한 것은 아니다.

소수 좌표를 0 방향으로 절삭한 오른쪽/아래 칸과 타입의 정수 발자국으로 포함 경계의 교차를 검사한다. 원본 OR 조건과 뒤집힌 사각형의 처리도 유지했다. 패치는 양수이면서 256 미만인 좌표만 읽고 나머지 후보는 건너뛴다. CD는 정수 칸 1 이상이라는 원본 assert 조건도 적용하며 C++에서는 잘못된 좌표를 예외로 거부한다. SHP 크기에 따른 등록 단계 선택은 기존 `SquidHash::ObjectLevel`의 역할이다.

## 변경 시점과 삭제 훅

Next는 후보를 반환하기 **전에 다음 SID를 저장**한다. 현재 객체의 next가 삭제/필터 콜백으로 바뀌어도 이미 읽은 다음 SID를 잃지 않는다. 이후 후보의 extra·좌표·타입, 아직 방문하지 않은 버킷의 머리는 조회 시점의 값을 다시 읽는다. 전체 후보 목록을 미리 스냅샷으로 만들지 않는다.

`MakeBridgeLifecycleHooks(finder, emit)`은 Begin/Next를 기존 raw 다리 preDestroy/postDestroy에 공급한다. 같은 버킷의 두 등록 순서, 먼저 삭제된 링크의 상태, 먼저 낙하한 walker가 다음 후보의 타입을 바꾸는 순서까지 실제 일반 탐색과 실제 삭제 훅을 함께 대조했다. 콜백이 finder를 참조하므로 finder가 더 오래 살아야 하고, 외부 효과 안에서 같은 finder로 다른 탐색을 시작하면 안 된다.

새 C++는 시작 전 Next, 미지원 flag, 누락된 높이 배열, 풀 밖 SID, 같은 버킷의 next 순환, 잘못된 타입/발자국을 예외로 거부한다. 원본의 assert/잘못된 메모리 접근/무한 순환을 그대로 실행하는 계약은 아니다. 다른 버킷에서 같은 번호가 다시 나타나는 것은 원본처럼 허용한다. 정상 조회는 풀·타입·해시를 변경하지 않는다.

## 독립 대조와 재현

| 종류 | 입력 수 |
|---|---:|
| 일반 Begin/Next 커서 전체 | 1,752 |
| 실제 일반 탐색 + preDestroy | 12 |
| 실제 일반 탐색 + postDestroy | 12 |
| 합계 | **1,776** |

세 실제 PE × x87 53/64비트로 실행했다. 네 단계·체인·소수 좌표·발자국/확장 높이·buried/상태 조합·경계 접촉·빈 범위·전체 보드·-1 표식·상한 밖/뒤집힌 범위와 첫 반환 뒤 next/extra/좌표/타입 변경을 포함한다. 객체 번호뿐 아니라 finder +4..+60의 **15 DWORD 전체 상태와 마지막 0 반환**을 비교한다.

패치의 실제 내부 호출은 Begin 592 / Next 1,746 / Pre 4 / Link 8 / Post 4다. CD/10.37은 각각 Begin 592 / Next 1,746 / Pre 4 / Post 4이며 Link는 인라인이다. 내부 호출 수는 상위 입력 1,776개에 더하지 않는다. 원본 assert 도달은 세 판본 모두 0이다. 정상 반환·스택·x87 TOP/제어 워드·FS 복구를 기존 실행기로 검사한다. 일반 탐색 단독 실행의 쓰기는 **finder 커서·가상 스택·FS 예외 목록만 허용**하고 풀·타입·해시 쓰기는 거부한다.

**대체 경계:** Begin/Next·버킷·발자국·CRT 절삭·기본 필터는 원본 명령이다. 등록 입력의 버킷/next는 직접 준비한 합성 상태다. 삭제 훅의 destroy/fall·공통 pre/postDestroy·소리·제거 통지는 이전 검사기의 외부 효과 대체를 유지했다. 일반 Pop 전체나 실제 삭제/낙하/소리 성공, 실제 게임에서의 공간 등록 상태를 증명하지 않는다. 기록에 PE·몸체·도구·fixture SHA와 대체 호출 수를 남겼다.

```powershell
python -X utf8 tools/decomp_rawfinder_oracle.py
python -X utf8 tools/decomp_rawfinder_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

생성에는 기존 `extracted/oracle-python`과 원본 PE·내보내기가 필요하다. `--verify`는 SHA/행 수/탐색 대체 없음만 감사하며 기계어를 재실행하지 않는다. C++ 검사에는 저장된 fixture만 필요하다.

## 남은 연결

최종 x64 Release 경고/오류 0·CTest 내부 **163개·실패 0**(56.38초)다. 기존 157개에 새 6개를 더했으며 누적 제한 x86 입력은 **83,592개**(81,816 + 1,776)다. 기존 삭제/붕괴 감사와 원본 여섯 디렉터리 **2,782개 파일 SHA/목록 동일**을 확인했다. 로그는 `extracted/finder/build.log`·`ctest.log`다. AGENTS.md·C#·LEFT_JOBS.dotnetpj.md는 변경하지 않았다.

공통 destroy의 순서·실제 Unpop/SID 반납은 위 후속에서 연결했다. 공통 pre/post의 Graph 분할/목록/통계·종속 파생 효과, 파편 생성 `00460600`(기존 “제거 통지 목록” 해석 정정), walker vtable +200/칸 위 carrier 처리는 남았다. `0x2692` 이벤트의 등록/실행/취소와 `004215d0` 끝 칸 변환도 남았다. 다른 일반 탐색 파생 필터와 raw GameWorld 연결, 실제 다리 배치/소유자·Construction, 건설/경제/전투/AI/승패를 이어야 한다.

기존 GUI GameWorld는 임시 객체 모델이며 이번 raw 탐색/삭제 훅을 사용하지 않는다. 호스트 `DESKTOP-HJOW`의 이번 검사는 콘솔 범위다. 창 검증은 다른 허용 PC에서 raw 월드 연결 후 다리 붕괴/낙하/소리·TEST01/1-1 생성/삭제/재진입을 확인한다. 이전 작업의 일회성 창 허용을 이번에 적용하지 않았다. 최신 검사 수와 완료 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)와 [cpppj README](../../cpppj/README.md)를 따른다.
