# R0-4 두 번째 묶음 — 타입 변경과 비석·주문 전환 L1 대조 (2026-10-28)

작업 PC `HJOW-Athlon`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)
R0-4 영향표 A의 두 번째 묶음(`squid-retype`·`object-conversion`)을 처리했다. cpppj·원본·AGENTS.md는 바꾸지 않았다.

## 1. 옮긴 cpppj (HEAD `b126e900` 기준)

| cpppj | C# | 내용 |
| --- | --- | --- |
| `src/o/SquidFactory.cpp` | `Netstorm.Core/Original/RawSquidFactory.cs` | `Retype` 신설: void(0x22d)·foot 가로(0x22e)·세로(0x22f) 진단을 선택 위치에서만 요구하고 복귀 뒤 계속, free/dead/예약 추가 제외 없음, HP/소유자/할당 보존, 타입 바이트 쓰기 뒤 현재 타입 생성자 재조회. `ConstructorVtable`의 미등록 생성자를 명시 오류로 거부(기존 null 강제 언래핑 자리의 같은 지점 예외). 판본 검사는 C# 풀이 패치판 범위라 생략 |
| `src/o/RawObjectConversion.{h,cpp}` | `Netstorm.Core/Original/RawObjectConversion.cs`(신규) | `ObjectConversionHooks`(unpop·repop·retype·defaultFrame·frame·qa·base), `ObjectConversionBody`(비석 0x50a748→0x462290, 주문 16표→0x41e400, 나머지 0), `Command`(몸체 판별→마스크 코드 판별→Unpop→비석 분기(QA 재조회·Retype·기본 프레임)/주문 분기(비석 타입 Retype·프레임 0·이전 타입 QA)→Repop, 반환 1). 비석 타입 장기 주소와 판본 검사는 값 수신·생략으로 두며 R4에서 연결한다 |

미적용(R4·R6으로 연기): 최종 가상 표 판별(`SupportsConstructedVtable`, CD 표가 없어 추정하지 않음),
약화 전역 공유(`BindWeakenedMana`, 세션 소유 문제).

## 2. 연결한 fixture와 검사

- `squid-retype-x86.tsv` 129행(머리 1 + 자료 128) — `X86RawObjectConversionTests.SquidRetypeReplaysNative128AndPreservesOtherSlots`.
  `cpppj/tests/ObjectConversionTests.cpp`와 같은 장치(50번 슬롯 직접 설치·네 타입 foot·생성자 주소)로 Retype 1회를 재생하고
  슬롯 50바이트 전체 hex·자취·빈 카운터·대상 외 풀 전체 보존을 대조한다.
- `object-conversion-x86.tsv` 37행(머리 1 + 자료 36) — `X86RawObjectConversionTests.ObjectConversionReplaysNative36CommandsAndLateReads`.
  비석·주문 몸체 판별과 Unpop 뒤 QA·타입 재조회를 포함한 Command 1회를 재생하고 반환·슬롯 전체·자취를 대조한다.
  몸체 외 값(`0`·`0x509930`)의 0 반환도 확인한다.
- 계약 1개 — `ObjectConversionRejectsSelectedMissingEffectsAndKeepsWrittenPrefix`(몸체 외 거부·장치 누락 중단과 실패 위치까지의 타입 바이트·sentinel 쓰기 유지,
  진단 미연결 Retype 거부와 슬롯 불변).

`Netstorm.Core.Tests.csproj`에 두 fixture를 `Link`로 연결하고
`python dotnetpj/tools/cpp_sync.py --update --add squid-retype:Core --add object-conversion:Core`로
`cpp-fixture-manifest.tsv`에 등록했다(69개. reflected-commit은 영향 검토가 끝나지 않아 올리지 않는다).
`--ports-add cpppj/src/o/RawObjectConversion.cpp:Netstorm.Core/Original/RawObjectConversion.cs:L1`로 84번째 행을 추가하고
`--ports-reflect HEAD --ports-file RawSquidFactory.cs`로 base를 올렸다(바뀐 쌍 33개 유지).

## 3. 검증

- `dotnet build dotnetpj/Netstorm.sln -c Release`: 경고 0·오류 0.
- `dotnet test dotnetpj/Netstorm.sln -c Release`: **1,405개(Assets 363 + Core 997 + Client 45) 통과**.
- 변별력: 타입 바이트 쓰기를 빼는 변이에 세 검사 전부 실패(재생 96·24행 불일치와 계약 검사의 예외 누락)를 확인하고 원복했다.
