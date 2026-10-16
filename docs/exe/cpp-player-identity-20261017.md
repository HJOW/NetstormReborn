# Player 식별자 조회와 객체 메뉴의 subscriber DWORD

2026-10-17, `HJOW-Athlon`. 마지막 전체 정밀 디컴파일도 **HJOW-Athlon / 2026-10-16**이다. 기존 정밀 소스와 보호 PE를 읽었다. 새 Ghidra 내보내기·원본/클론 게임·GUI 창 실행은 없다. 모든 기계어 관찰은 격리 메모리에서 수행했다.

## 확인한 형식 오류와 교정

`004c9700` Player 메뉴를 추적하던 중 앞 단위의 `RawObjectMenu`가 `004844f0` 반환을 이름 문자열로 다루고 있음을 확인했다. **실제 반환은 subscriber DWORD**다. `004844f0`는 owner를 넣은 PlayerFinder를 실제 `004841c0`에 전달한 뒤 `+4` DWORD를 반환한다. CD `004019e0`도 같다.

객체 메뉴 연결을 `ownerSubscriber(uint32)->uint32`, `playerMenu(uint32,1,0)`으로 교정했다. `BindObjectMenuIdentity`는 아래 실제 조회 몸체에 연결한다. 숫자를 호스트 주소/문자열로 재해석하지 않는다. `NameForSubscriber`는 이름이 필요한 별도 경계에서만 사용한다.

기존 객체 메뉴 oracle은 이 두 의존을 문자열 반환/문자열 관찰로 대체했다. 따라서 그1,434행은 상위 메뉴 분기와 공급 경계의 대조였으며 **실제 getter 반환 의미를 입증하지 못했다**. 과거 fixture를 보존했고 기존 C++ 대조는 그 고정 opaque DWORD에 붙인 관찰 label을 명시적으로 재생한다. 새48행은 두 실제 getter/검색 몸체를 실행하고 Player 메뉴 경계에 도달한 전체DWORD를 직접 기록한다. 두 관찰을 혼동하지 않는다.

## 실제 조회 몸체

| 몸체 | PATCH10.78 | CD10.72 |
|---|---|---|
| 초기화 | `00484190` | `004015e0` |
| 식별자 Resolve | `004841c0` | `00401610` |
| owner→subscriber | `004844f0` | `004019e0` |
| subscriber→owner | `00484550` | `00401ad0` |
| subscriber→주소 | `00484520` | `00401aa0` |
| subscriber→이름 | `00484410` | `00401890` |

`RawPlayerIdentity`는 같은 현재 전역 getter와9 Player·40 metamap·100채팅 기록을 빌린다. 조회 순서는 로컬 네 조건의 첫 match→battle/editor의 Player1..8→활성 metamap→채팅100이다. 이름/정수/표의 사본이나 별도 사용자 목록을 만들지 않는다. metamap 저장소는 빈 연결을 허용하지만 실제 해당 모드에 진입하면 명시 오류다.

로컬 owner/subscriber/이름/IP 중 하나만 일치해도 현재 로컬 값으로 덮어쓴다. Player 검색은 활성 플래그 조건 없이 owner/subscriber/이름의 첫 OR 일치다. metamap은 활성 bit1과 subscriber/이름만 묻는다. 채팅도 별도 활성 필터 없이 처음부터100개 전체를 검색하고 owner를0으로 쓴다.

원본의 부분완료 반환을 보존한다. Player 단계의 owner/subscriber/이름 세 필드 조건과 뒤의 subscriber/이름 두 필드 조건은 다르다. **Resolve true가 유효 owner 확인을 의미하지 않는다.** 네 getter도 bool 성공을 새 관문으로 추가하지 않고 원본 해당 필드를 반환한다. 이름만 실패 시 현재 SOMEONE 문자열을 사용한다.

초기화는 `+0/+4`, 이름 첫BYTE, `+30/+34`만 쓴다. 비교한60바이트 관찰 영역의 이름 뒤·마지막DWORD 표식을 포함해 미작성 바이트를 보존한다. 이60바이트 대조 범위를 원본 클래스의 `sizeof`에 대한 별도 증거로 사용하지 않는다. 이름·주소·mode의 실제 접근 위치는 기계어 대조에 포함한다. mode를 근거 없이 검색 필터로 사용하지 않는다.

## 원문 관찰과 검증 범위

| 구분 | 두 판본 고유 입력 |
|---|---:|
| 초기화 전체 영역 | 8 |
| Resolve 반환/전체 영역 | 358 |
| 네 getter | 1,288 |
| 객체 메뉴→실제 getter→DWORD 전달 | 48 |
| 합계 | **1,702** |

각 입력을 x87 53/64비트 환경에서 실행했고 **3,404정상 반환·정밀도 차이0·실제 명령2,210,198회**다. C locale의 ASCII `stricmp`는 명시 비교 경계다. 객체 메뉴의 번역/항목·하위 메뉴 작성과 Player 메뉴 전체도 명시 외부 경계다. 조회/초기화/네 getter의 실제 몸체는 대체하지 않았다. 게임/OS 호출은0이다.

로컬 네 match, 처음/마지막 행, 중복 subscriber, 대소문자,0/상위32비트, 비활성 metadata,0 subscriber·이름만 있는 기록, 세 기록표의 우선순위와 부분완료를 포함한다. getter에서 사용하지 않는 query 열은 정규화해 중복 관찰 수를 부풀리지 않았다.

새 C++ 집중4검사는1,702행의 반환·전체60바이트·메뉴 명령 순서와 일치했다. 같은 조회기에서 원문 저장소를 변경하면 다음 조회에 반영됨과 누락 metamap/부분완료 계약도 확인했다. 기존 객체 메뉴4검사와 실제 공통 Gump의 Carrier/객체 사슬1검사도 통과했다. 공통 Gump 검사는 실제 새 식별자 몸체로 `fedcba98`을 전달하고 필요한 이름만 별도로 조회한다. Player 전체 몸체를 검사에서 복원한 것으로 합산하지 않는다.

Release 빌드1은 **종료0·경고0·오류0**이다. 전체 CTest는 **내부1,470개 모두 통과·실패0·208.55초·종료0**이며 전체 내부 출력을 `extracted/goal-player-identity-20261017-ctest-output.log`에 보존했다. 시스템 기본 커서 검사도 이번 전체 실행에서 통과했다. 앞선 간헐 실패의 원인을 수정했다고 합산하지 않는다. 로그 접두사는 `extracted/goal-player-identity-20261017-`이다.

```powershell
python tools/decomp_player_identity.py --verify
python tools/decomp_object_menu.py --verify
python tools/decomp_carrier_menu.py --verify
cmake --build cpppj/build --config Release --parallel 3 -- /p:CL_MPcount=3
cpppj/build/bin/Release/netstorm_tests.exe --filter PlayerIdentity_
ctest --test-dir cpppj/build -C Release --output-on-failure
```

새 원문/제품 계보는 `cpppj/player-identity.json`·`player-identity-x86.tsv`에 있다. 기존 객체1,434·Carrier628·TYPE2,352 fixture는 변경하지 않았다. 실제 변경된 소스/공유 CMake의 핀만 각 보고서에 이전/현재 이력을 남겨 갱신한다.

## 남은 사용자 플레이 연결

Player 메뉴 `004c9700` 전체, 내려놓기 위치 `00426a70`, 실제 Session의 같은 현재 전역/기록과 메뉴·명령 소비자·GumpForm 부착이 남아 있다. 사제의 건설 커서 `445b30/445440/4458e0`와 일반 건설 확정/배치도 미연결이므로 **Construct는 아직 비활성**이다.

Early Missions1~6·캠페인1-1/1-2·데모3개의400프레임 통과를 사용자 자연 완주로 합산하지 않는다. 생산/다리·경제·AI 전략·GUI 미션 시작/단계·승패/저장/다음 미션과 전체 그래픽/음향 동일성 검증은 계속 필요하다. 전체 목표는 active이며 전체화면 DirectDraw는 이번1차 범위에서 제외한다.
