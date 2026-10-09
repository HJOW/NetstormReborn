# 사제 postPop 후반·전체 wrapper 연결

2026-10-10. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** AGENTS.md·두 인계·cpppj 계획과 주석 규칙을 다시 읽었다. 같은 PC에서 새 `priestpostpoptail` 목록 **18/20/20개**를 읽기 전용으로 내보냈다. 10.78 싱글플레이 복원이 현재 우선이며 outpost/LAN은 3차, 한국어·요구사항/MCP는 4차다. 원본 게임/복사본·클론 창은 실행하지 않았다.

[`RawPriestPostPopTail`](../../cpppj/src/o/RawPriestPostPopTail.h)은 [목록/회복 접두](cpp-priest-postpop-reconstruction.md)→[Carrier/Damageable/공통 장부](cpp-carrier-postpop-reconstruction.md)→현재 이동 불가/지면/프레임→낙하/보호막 요청의 전체 wrapper 순서를 조합한다. 직접 호출용이며 일반 `SquidPop`의 사제 가상 표 지원은 계속 제한한다. **낙하 몸체·보호막 Pop/destroy·오디오·GUI 완료가 아니다.**

| 동작 | 10.78 | CD·추가 10.37 |
|---|---|---|
| 전체 postPop wrapper | `004950f0` | `0040c110` |
| Carrier 직접 호출 | `00427720` | `004e6360` |
| 실제 이동 불가 조회 | `00427030` → `00492090` | `004e5150` → `0040d7b0` |
| 거의 올림 지면 조회 | `0040d9d0` → `004e49c0` | inline → `004f161c` |
| signed 방향 조회 | `004ad710` | `0040c5b0` → `004ae4b0` → `004abf50` |
| 낙하 전체 효과 경계 | `004941f0` | `0040c880` |
| 보호막 생성 요청 | `00493d30` | `0040bfb0` |
| 이동 가능한 보호막 삭제 wrapper | `00491980` | `0040c0d0` |

## 현재 상태를 읽는 순서

기존 접두는 최초 flags 1일 때 사제 목록/회복을 예약하고 현재 소유자 칸을 초기화한다. **Carrier는 extra 차단과 무관하게 항상 같은 flags로 호출**한다. 이후 현재 extra(+40/CD +35)를 다시 읽고 `extra & 9`면 후반을 생략한다. Carrier가 차단을 새로 켜거나 해제했을 때도 이 순서를 유지한다.

현재 `RawPriestState::Immobile`이 거짓이면 보호막을 조회/삭제하고 후반을 끝낸다. 참이면 `flags & 0x800`을 검사한다. 이 비트가 있으면 **좌표/지면/프레임 조회 자체를 생략**하고 보호막을 요청한다. 비트가 없으면 현재 좌표에 **0.9999f를 더한 x87 중간 합을 float으로 좁히지 않고 0방향 절삭**하여 spot `& 6`을 읽는다. HP 상태 조회와 같은 거의 올림 칸이며 HP 전환 재등록의 0.99999f와는 구별한다. 지면 비트가 0일 때만 현재 프레임 방향을 읽으며, signed side − `'A'`가 **9(J)**이면 낙하 요청을 생략한다. 방향을 0~7로 줄이지 않는다. 프레임 번호는 패치 +36 DWORD/CD +34 BYTE다.

지면이 없고 J가 아니면 전체 낙하 경계를 부른다. **낙하 효과가 extra/HP/지면을 바꾸어도 이미 선택한 이동 불가 분기의 보호막 요청은 계속 실행**한다. 원본 낙하 몸체 자체도 먼저 보호막 생성 wrapper를 부르고 fall 이벤트 `0x25b`·Unpop/Repop `0x800`·`priestFall.wav`를 요청한다. 이 낙하 내부는 이번 C++ 경계이며 중복 보호막 조회로 생성 중복을 막는 것은 기존 생성 모듈의 계약이다.

`MakePriestPostPopTailShieldHooks`는 [실제 보호막 생성](cpp-priest-shield-reconstruction.md)과 조회에 연결한다. 이동 가능 분기의 조회가 유효한 SID를 돌려주면 가상 destroy를 flags 0으로 요청한다. 풀 밖 결과/0에는 destroy를 부르지 않는다. 생성 wrapper와 lookup은 같은 풀이어야 하며 destroy/fall은 필수다. 전체 조합은 prefix/Carrier/상태 풀과 표 크기·필수 효과를 검사한다. 호출자는 상태 조회와 후반에 같은 월드의 spot/타입/프레임을 공급해야 한다. 외부 효과 뒤의 실패를 되돌리는 트랜잭션은 제공하지 않는다.

디컴파일 출력의 지면 helper에는 x87 `fadd`가 드러나지 않는다. 초기 C++의 단순 절삭은 floor/ceil spot이 다른 원본 입력에서 실패했고, 실제 `0040d9d5 fadd [00500edc]`와 CD inline 명령·상수 비트 `0x3f7ff972`를 확인하여 0.9999f 덧셈을 복원했다. 원본 fixture/기대 관찰값은 변경하지 않았다.

## 독립 원본 관찰

[`decomp_priestpostpoptail_oracle.py`](../../tools/decomp_priestpostpoptail_oracle.py)는 세 PE 각각 **2,048개**, 총 **6,144개**를 두 x87 제어 워드 `0x027f`/`0x037f`로 실행하고 관찰이 같은 한 행을 저장한다. **전체 wrapper·접두의 실제 목록/HP/소유자 칸·이동 불가/지면/프레임/CRT 조회는 진입부터 정상 ret 4까지 원본 명령**이다. Regular 검색/new/생성자와 Carrier·낙하·보호막 생성/삭제 몸체만 명시 대체한다. C++ 연결의 실제 Carrier/공통 장부·보호막 생성·ProcessHost는 별도 통합 검사다.

1,920개 격자는 extra 0/1/8/9/32, flags 0/1/0x800/0x801, HP 20/100, 단순 절삭/거의 올림 칸의 spot 0/6, signed side A/J/0xff, 기존 회복과 확보 성공/실패를 교차한다. 추가 128개는 Carrier 뒤 extra 차단/강제 이동 불가·좌표/HP/지면 변경과 낙하 뒤 extra 변경을 공급한다. Python/C++은 낙하/보호막 예상 분기를 계산하지 않는다.

각 PE 정상 반환/실제 wrapper/Carrier 경계 **4,096회**, 실제 이동 불가 wrapper **1,696회**, 실제 지면/HP 상태 조회 **864회**, 실제 방향 조회 **400회**다. 낙하 경계 **264회**, 보호막 생성 **1,440회**, 보호막 삭제 **256회**, Regular 검색 **832회**, new **416회**, Regular 생성 **224회**다. 패치 거의 올림 spot helper는 **720회**이며 CD는 inline이다. ESP·EBX/ESI/EDI/EBP·x87 제어/TOP·SEH FS:[0] 정상 복구, 허용 코드/쓰기·원본 this/flags·raw 전체/목록/소유자 칸·assert/OS 0·SHA/행/입력 순서·실제/대체 진입 수를 감사한다. 예외 SEH 해제나 원본 할당 실패 처리 전체는 재현하지 않는다.

새 [fixture](../../cpppj/tests/fixtures/priestpostpoptail-x86.tsv)·[근거 JSON](../../cpppj/recovery-priestpostpoptail-evidence.json)은 UTF-8/LF다. 기존 fixture/감사 도구는 수정하지 않았다. 누적 인계 **347,962 + 6,144 = 354,106개**다.

## C++ 연결과 남은 범위

새 [C++ 검사](../../cpppj/tests/PriestPostPopTailTests.cpp) 6개는 세 PE 전체 관찰 재생, 실제 ProcessHost/Form/Kernel·공통 비용/통계/깊이·factory/owner/보호막 조회→중복 방지→이동 가능한 보호막 삭제 요청→강제 이동 불가 재생성, 잘못된 경계/풀/표, 불필요한 NaN 좌표/프레임 읽기 생략을 검사한다. 통합의 보호막 Pop/destroy는 등록/해제 입력을 공급하는 외부 효과이며 전체 공간 몸체의 복원 근거가 아니다. 새 코드와 수정한 클래스/멤버함수에는 설명과 사용법을 먼저 한국어로 주석 처리했다.

Release **경고/오류 0**, CTest 내부 **441개·실패 0**(119.45초), 원본 근거 감사 **66종 모두 통과**. [LEFT_JOBS.md](../../LEFT_JOBS.md)에 최종 검사와 후속 범위를 기록했다.

```powershell
tools/ghidra/export_functions.ps1 -Name priestpostpoptail
python -X utf8 tools/decomp_priestpostpoptail_oracle.py
python -X utf8 tools/decomp_priestpostpoptail_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

다음은 **전체 낙하 `0x25b` 생성/처리·Carrier +0xcc의 실제 이동 불가 검사·일반 사제 Pop/보호막 공간 수명**이다. 이어 비표면 raw 월드/GUI·건설·경제·전투·승패를 연결한다. 원본 함수를 더 복원하기 전에 일반 사제 Pop 지원을 넓히지 않는다. outpost/LAN은 3차 목표로 유지한다.
