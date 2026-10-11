# cpppj 표시 목록 수집·가시 영역·정렬 제출 복원

2026-10-11, **HJOW-Athlon**. 10.78을 기준으로 RenderProcess의 표시 영역 환산·교차·목록 수집을 복원하고 실제 건설 표시 프로세스의 선택적 Draw에 연결했다. 기존 깊이 비교 함수도 새 독립 관찰에 포함했다. 실제 클라이언트 프레임 루프와 SHP/장치 출력은 다음 단계다.

## 원본 근거

AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 먼저 확인했다. 착수 시 작업 트리는 깨끗했고 현재 PC와 마지막 디컴파일 PC가 같았다. 같은 Ghidra 프로젝트에서 새 `rendercollection` 목록 **5/4/4개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.**

| 몸체 | originals 10.78 | originalCD / original1037 |
|---|---|---|
| 전체 표시 목록 수집 | `00487f10` | `0045c4b0` |
| 표시 영역 월드 환산 | `00487a40` | `0045c9e0` |
| 부모 raw 조회/head 확인 | `0041bfb0` | `0048f870` |
| 영역 교차 | `00487740` | 수집 몸체 안의 실제 인라인 명령 |
| 공유 깊이/y/x 비교 | `00497900` | `00458240` |

내보내기는 `extracted/rendercollection/<판본>/{creation.c,functions.tsv}`에 있다. 세 원본 PE는 읽기만 했다. 원본/복사본 게임·analyzeManager·클론 창·실제 장치는 실행하지 않았고 OS 호출도 없다.

## 복원 동작

- `RawRenderCollection::Bounds`는 부모 head를 먼저 확인하고 등록 영역의 x/y를 현재 배율로 나눈 월드 영역을 만든다. 패치판은 역수와 좌표의 곱, CD판은 좌표의 직접 나눗셈이다. 역수를 먼저 float로 저장하지 않는다. 카메라 좌표를 빼는 일은 개별 Draw 단계다.
- 일반 영역은 네 변이 엄격히 교차해야 한다. 경계만 닿거나 역전된 영역은 제외하며 내부의 점/선 영역은 허용한다. **CD판의 x87 분기에서는 view.top 또는 bounds.bottom이 NaN인 일부 경우가 통과한다.** 패치판은 이를 제외한다. Ghidra 조건식만 옮기지 않고 실제 `test ah,1`/`test ah,0x41` 분기와 저장 관찰로 구별했다.
- 목록을 0번부터 읽고 빈 칸은 건너뛴다. 출력 limit을 채우면 다음 부모를 조회하기 전에 끝낸다. 부모 free/dead/void/abstract 상태를 따로 걸러내지 않는다. 가시 객체는 부모를 다시 조회해 현재 x와 y를 읽는다.
- 출력은 원본처럼 **16바이트**다. x/y float, 호스트 객체 token DWORD, 등록 깊이의 **signed 하위 WORD**, 종류 BYTE=3을 쓴다. y에는 원본 float 비트 `0x3a83126f`를 더한다. 마지막 BYTE와 미사용 출력 항목은 보존한다. 원본 포인터만 호스트 토큰으로 표현한다.
- `ConstructionVisualSystem::Collect`는 실제 등록 목록과 현재 부모 raw를 연결한다. 생성 중인 호스트 객체는 건너뛴다. `DrawCollected`는 수집된 항목의 현재 토큰을 조회하여 삭제된 객체를 건너뛰고, 재사용된 슬롯의 새 객체를 이전 목록에 추가하지 않는다.
- `client::DrawVisibleConstruction`은 작업 버퍼 크기로 목록을 제한한 뒤 기존 `CompareDrawOrder`로 깊이 내림차순/y·x 오름차순 정렬하고 실제 프로세스 Draw를 제출한다. 정렬 끔도 지원한다. 선택적 제출이 현재 카메라 환산/프레임 출력 버퍼에 연결되는지 통합 검사했다.

호스트 계약으로 같은 풀/유효 lookup·부모 번호/head·유한 비영 배율·출력 용량을 검사한다. 손상 자료의 원본 assert나 잘못된 포인터 접근을 실행하는 복원이 아니다. 정렬 어댑터는 비유한 x/y를 Draw 전에 거부한다. 완전 동률의 순서는 `stable_sort`의 안정 순서이며 **원본 CRT qsort의 동률 순서와 같다고 보장하지 않는다.**

## 독립 관찰과 검사

`tools/decomp_rendercollection_oracle.py`는 Ghidra가 내보낸 실제 함수 범위만 격리 x86으로 실행한다. 결과 계산/함수 응답/효과 대체가 **없다**. 목록·객체·부모 raw는 입력이며 실행 중 쓰기는 출력·스택·SEH에만 허용한다. CD의 Pentium FDIV 우회 전역은 0인 입력으로 실행했다.

| 관찰 | 판본당 | 세 판본 합계 |
|---|---:|---:|
| Bounds | 226 | 678 |
| Intersect | 225 | 675 |
| Collect | 948 | 2,844 |
| Order | 36 | 108 |
| 합계 | 1,435 | **4,305** |

두 x87 정밀도(0x027f/0x037f)에서 **정상 루트 반환 8,610회**, 출력과 실제 helper 진입 순서가 같았다. 독립 원본 관찰 누적은 **498,660 → 502,965개**다. 빈/1/5/40/80칸·구멍 네 형태·limit -1/0/1/2/5/8·부모 상태 6종·깊이 WORD 경계·큰 y·점/선/역전/NaN/무한대·배율/좌표 입력을 관찰했다. 전 범위 수치 입력의 동치를 증명한 것은 아니며 저장 입력 계획을 대조한 결과다.

fixture는 각 행에 완성된 객체 입력을 저장한다. C++ 검사는 입력 생성 공식을 복제하지 않고 원본 반환값과 환산/전체 출력 바이트를 읽어 비교한다. 감사는 PE·내보내기·도구·fixture SHA, 입력 순서/폭, 실제 진입과 루트 반환 총계를 확인한다. 원본 assert/응답 경계/OS 호출은 모두 **0**이다.

새 C++ 검사 **7개**: 세 판본 재생, 연결 계약/조기 제한, 실제 가시 제출/카메라/용량/정렬 끔, 수집 후 삭제/슬롯 재사용, 안정 동률/비유한 정렬 좌표. 생성 중 Collect 차단은 기존 재진입 검사에 추가했다. **Release 경고/오류 0, 전체 CTest 내부 696개·실패 0**(159.54초, 전체 159.59초), **근거 감사 106종 모두 통과**했다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name rendercollection
python -X utf8 tools/decomp_rendercollection_oracle.py
python -X utf8 tools/decomp_rendercollection_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
cpppj/build/bin/Release/netstorm_tests.exe --filter RenderCollection
ctest --test-dir cpppj/build -C Release --output-on-failure
```

변이 검사는 저장소 밖 `extracted/mutation-work` 사본에서만 실행했다. 새 변이 **8개 모두 검출**, 미검출/건너뜀 **0개**다(`--focused`, 변이 전 사본 전체 검사 통과). 역수 float 반올림·경계 포함·CD unordered 분기 뒤바꿈·개수 제한·depth BYTE 절단·y 보정 생략·마지막 BYTE 덮어쓰기·정렬 방향 뒤바꿈을 검사했고 사본의 소스를 각 변이 뒤 원래 바이트로 복구했다. 로그는 `extracted/rendercollection-{export,oracle,build,ctest,audits,mutations}.log`다. AGENTS.md·보호 원본·dotnetpj·dotnet 인계는 수정하지 않았고 커밋/푸시하지 않았다.

## 다음 작업

1. 원본 클라이언트의 자산/구름/다른 효과와 공통 목록 병합·CRT qsort·종류별 분배·프레임 호출 순서를 복원하고 실제 Client/GameWorld에 연결한다. 이번 helper는 건설 표시 목록만 제출하며 GUI에서 자동 호출되지 않는다.
2. 건설 캡처의 실제 SHP·공유 팔레트 생성/현재 색 표·dirty와 소프트웨어 화면/장치 출력을 연결한다. 이번 통합 검사는 합성 픽셀/색 표/HP 경계를 사용한다.
3. 타입 준비 소리(+0x90)·가상 표시/부모 Unpop/재등록·하위 경로/접근점/자원 정책·사제 contained·집합/지연 분배·건설 GUI를 진행한다.

**게임 내 건설과 미션 완주는 아직 불가능하다.** 완료한 부분과 남은 순서는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 최신 cpppj 절을 따른다.
