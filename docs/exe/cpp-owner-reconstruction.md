# cpppj 소유자 지정(Squid vtable +0x74)

2026-10-07. **1차 기준은 10.78**이다. [다리 끝 칸 변환](cpp-bridgeevent-reconstruction.md)이 새 객체에 대해 부르는 가상 함수 **소유자 지정**을 복원했다. 객체의 소유자 바이트를 바꾸고, 대상 타입이면 소유자별 작업장 목록을 고친다.

소스: [SquidOwner.h](../../cpppj/src/o/SquidOwner.h), [SquidOwner.cpp](../../cpppj/src/o/SquidOwner.cpp). 검사: [OwnerTests.cpp](../../cpppj/tests/OwnerTests.cpp). 독립 결과: [owner-x86.tsv](../../cpppj/tests/fixtures/owner-x86.tsv), [감사 기록](../../cpppj/recovery-owner-evidence.json), [실행 도구](../../tools/decomp_owner_oracle.py), [내보낸 함수 목록](../../tools/ghidra/owner-functions.json).

## 원본 함수

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 소유자 지정(base Squid vtable +0x74) | `004adf00` | `004aefa0` |
| 타입 구조체 / genus 조회 | `0049a840` | `004abae0` |
| 소유자 작업장 목록에 추가 | `00490050` | `004071f0` |
| 소유자 작업장 목록에서 제거 | `004900a0` → `0040ea00` | `00407250`(배열 압축 인라인) |

다리 vtable(`005034c8`/CD `00501ab0`)의 +0x74도 이 base 함수다. 작업장 목록은 Player 구조체(패치 0xb4바이트, CD 0xac바이트) 맨 앞의 배열 객체(포인터·용량·개수)이며 Player 배열 포인터는 `0059534c`/CD `0050f834`에 있다. [공통 postPop](cpp-postpop-reconstruction.md)의 추가와 [삭제 장부](cpp-destroylifecycle-reconstruction.md)의 제거가 쓰는 것과 같은 목록이다.

## 동작

1. **범위 검사.** 평소에는 새 번호가 0 또는 1~8이어야 하고, 다른 모드(`00594fc8`/CD `00540a24`, 원본 문자열 `CL_NUM_CHAL_PLAYERS`)에서는 1~39여야 한다. 벗어나면 원본은 assert를 보고한 뒤 계속 진행해 번호의 하위 바이트를 쓴다. C++은 **쓰기 전에 예외로 거부**하며, 장부에 목록이 있는 0~8만 받는다.
2. 요새 모드(`00594fb8`/CD `00540a1c`)나 전투 모드(`00594fbc`/CD `00540a20`)이고 타입 genus에 vortex·factory 비트(`0x4200`)가 있으면, **이전 소유자**의 목록에서 이 번호를 모두 뺀다(중복도 전부, 남는 항목의 순서 유지, 개수 밖의 메모리는 그대로). 소유자 0은 목록이 없다.
3. 소유자 바이트(패치 +0x22, CD +0x20)를 쓴다.
4. 2와 같은 조건에 더해 객체가 void(state & 4)도 buried(extra & 8, 패치 +0x28/CD +0x23)도 아니면 **새 소유자**의 목록 끝에 넣는다. 목록이 가득 차 있으면 넣지 않는다.

CD판은 모드 전역 두 개를 레지스터에 들고 있다가 4에서 다시 쓰지만 판정은 같다.

## x86 대조 결과

[decomp_owner_oracle.py](../../tools/decomp_owner_oracle.py)가 세 실제 PE에서 실행한다. **대체하는 함수는 없다** — 소유자 지정·genus 조회·목록 추가/제거가 모두 원본 명령이다. 입력은 시드 고정 표본 **판본당 1,000개(합계 3,000개)**이며 모드 세 개, genus 8종(대상 비트 유무), state·extra, 이전/새 소유자 0~8, 소유자 아홉 명의 목록(용량 0·1·2·6, 개수, 대상 번호의 유무·중복·가득 참)을 섞는다. 결과로 소유자 바이트, 아홉 목록의 개수와 저장 칸 전체(개수 밖의 옛 값 포함), 객체 슬롯 전체의 Adler-32를 비교한다. 쓰기는 소유자 바이트와 목록 저장 칸·개수 외에는 거부한다. 원본 assert 도달 0, 패치 기준 실제 도달은 제거 467회(배열 압축 411회)·추가 165회다.

**제한:** assert 경로(범위 밖 번호)와 다른 모드의 9~39번은 입력에 없다. 목록이 바뀐 뒤의 생산/AI 재계산은 이 함수의 범위가 아니다. 타입 genus·객체·목록 저장소는 합성이다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name owner
python -X utf8 tools/decomp_owner_oracle.py
python -X utf8 tools/decomp_owner_oracle.py --verify
```

## 남은 것

- 끝 칸 변환의 `setOwner` 훅과 이 함수를 실제 월드에서 연결하는 일(지금은 단위 검사에서만 잇는다).
- 파생 클래스가 +0x74를 재정의하는지 전수 확인하지 않았다. base Squid(`00501dd0`/CD `005058d8`)와 다리 vtable의 +0x74가 이 함수임을 확인했고, 패치판에서 이 함수를 가리키는 DWORD는 73개다.
