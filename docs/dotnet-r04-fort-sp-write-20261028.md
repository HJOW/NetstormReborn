# R0-4 SP 묶음 — Fort SP writer 신규 이식과 L1 대조 (2026-10-28)

작업 PC `HJOW-Athlon`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)
R0-4 영향표 A의 첫 묶음(`sp-add-bits`·`sp-initialization`·`fort-sp-write`) 중 남은 하나를 처리했다.
이로써 SP 묶음 3/3이 끝났다. cpppj·원본·AGENTS.md는 바꾸지 않았다.

## 1. 옮긴 cpppj (HEAD `91a62e79` 기준)

| cpppj | C# | 내용 |
| --- | --- | --- |
| `src/o/RawFortSpWrite.cpp`·`RawFortSpWrite.h` | `Netstorm.Core/Original/RawFortSpWrite.cs`(신규) | `Write`: float32 1회 _ftol 계약 → 32비트 순서대로 고정 → 비트마다 Next(32) 16회 소비(같은 비트 잡음은 OR 제외, 소비는 유지) → 폭(8비트 미만 1·16비트 미만 2·나머지 4)으로 인라인 쓰기 → 현재 위치·끝 갱신. 용량 초과(signed 비교) 때만 진단 콜백 뒤 현재 위치·자료를 다시 읽는다. 버퍼 포인터 0·장치 누락은 명시 예외, 물리 범위 밖은 호스트 계약 오류로 멈춘다 |
| `src/o/RawMemStream.h` | 같은 파일의 `MemStreamAssertion`·`MemStreamHooks`·`FortSpWriteHooks` | writer가 쓰는 두 갈래(버퍼 조회·쓰기 용량 진단)만 쓴다. 할당·반납 멤버는 MemStream 본체 이식 때 연결한다 |

## 2. 연결한 fixture와 검사

- `fort-sp-write-x86.tsv` 198행 — `X86RawFortSpWriteTests.FortSpWriteReplaysIndependentNative198`.
  `cpppj/tests/FortSpWriteTests.cpp`와 같은 장치(`SpDevice`: 256바이트 버퍼 3개·무늬 초기화·변경 입력·`R:`·`A:36` 자취)로
  Native 178 정상·20 부분중단을 재생하고 상태·난수·호출 수·세 버퍼 전체 hex·`F:` 변환 자취를 대조한다.
  정상행 중 버퍼 방식 0·변경 없음 행은 보호 비트만 모아 `_ftol` 하위 DWORD도 대조한다.
- 계약 3개 — `FortSpWriteEqualNoiseKeepsOwnBitAndExactWidths`(잡음 생략·8·8·16·총 88바이트와 꼬리 보존),
  `FortSpWriteMissingSelectedDevicesPreserveRandomPrefix`(장치 누락 때 첫 16 난수 효과만 남김),
  `FortSpWritePhysicalFailurePreservesPriorWrites`(짧은 물리 때 앞 2바이트·48회 난수 유지, 위치·끝 복구 없음).

`Netstorm.Core.Tests.csproj`에 fixture를 `Link`로 연결하고
`python dotnetpj/tools/cpp_sync.py --update --add fort-sp-write:Core`로
`cpp-fixture-manifest.tsv`에 등록했다(67개. reflected-commit은 영향 검토가 끝나지 않아 올리지 않는다).
`python dotnetpj/tools/cpp_sync.py --ports-add cpppj/src/o/RawFortSpWrite.cpp:Netstorm.Core/Original/RawFortSpWrite.cs:L1`로
83번째 행을 추가했다(바뀐 쌍 33개 유지).

## 3. 검증

- `dotnet build dotnetpj/Netstorm.sln -c Release`: 경고 0·오류 0.
- `dotnet test dotnetpj/Netstorm.sln -c Release`: **1,402개(Assets 363 + Core 994 + Client 45) 통과**.
- 변별력: 인라인 폭(1·2·4)을 2·2·4로 바꾸는 변이에 재생 192행 불일치와 정확 폭 검사의 장치 누락 실패를 확인하고 원복했다.
