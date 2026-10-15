# 실제 로그인 설정0x32 송신·수신 복원

현재 PC와 마지막 전체 정밀 디컴파일 PC는 `HJOW-Athlon`, 2026-10-15로 일치한다. 같은 보호10.78 전체 정밀 소스와 capstone 기계어를 다시 읽고, `0044dbc0`, `0040a700`, `004df230`, `0044dc20`, `0040b380`을 실제 격리 실행했다. 원본/AGENTS/dotnetpj/공유제품/CMake를 변경하지 않았고 빌드·게임·GUI 실행은0이다.

## 실제 현재 설정과 조회 순서

`RawMissionSettingsPacket`은 값을 보관하지 않고 `current(MissionSettingsField)`와 `setCurrent(field,DWORD)`를 빌린다. 이7개는 원본이 읽고 쓰는 **같은 현재 전역 설정**이며, ConfigInterface 문자열을 매번 읽는 경로가 아니다. 실제 통합은 기존 설정 저장소를 getter/setter에 연결해야 한다. 값을0으로 반환하거나 송신을 성공으로 대체하는 공급자는 추가하지 않았다.

| payload 필드 | enum | 현재 원본 전역 |
| --- | --- | --- |
| +4 | GeneratorTimer | 005326cc |
| +8 | HackMover | 0055a488 |
| +c | NoDamageDebug | 0055a48c |
| +10 | ShootMyFriends | 0055a490 |
| +14 | FillMyTank | 0055a494 |
| +18 | CombatGumpColumns | 005326d0 |
| +1c | CombatGumpRows | 005326d4 |

보호 descriptor `0052edd2`의 원문은 `32 01 20 00`이다. 실제 생성자 `0040a700`은 헤더를 먼저0으로 쓰고 type32/length32/version1을 지정한다. 고정 최종 헤더는 `01 32 20 00`이고 packet은32BYTE다. 원본의 설정 값은 bool로 정규화하지 않고 모든 DWORD 비트를 그대로 전송한다.

송신 `0044dbc0`의 실제 조회 순서는 **HackMover→NoDamageDebug→GeneratorTimer→FillMyTank→CombatGumpColumns→ShootMyFriends→CombatGumpRows**, payload 번호로 **1,2,0,4,5,3,6**이다. 디컴파일 문장의 표현 순서나 payload 순서로 재정렬하지 않았다. 먼저 읽은 값은 이후 getter가 현재 설정을 바꾸더라도 보관하고, 아직 읽지 않은 값은 그 원문 위치에서 다시 읽는다. 실제 외부 송신 `004e2dc0(target,packet)`은32BYTE 전체를 받는다. 현재 연결gate는 이 외부 송신 소비자의 책임이며 누락/거부 예외를 성공으로 바꾸지 않는다.

## 실제 수신과 등록

`Receive`는 `0044dc20(sender,connection,packet)`처럼 sender/connection을 사용하지 않는다. payload **0,1,2,3,4,5,6** 순서로 DWORD를 현재 packet에서 읽고 같은 현재 설정에 직접 쓴다. 전체 payload를 먼저 복사하지 않는다. 따라서 setter 경계가 같은 수신 buffer의 나머지 값을 바꾸는 검사에서 후속 읽기에 반영된다. 별도 UI/옵션 재계산/추가송신을 만들지 않았다.

원본문체는 헤더를 해석하지 않는다. 공개 `Receive`의 실제 물리길이가 부족하면 그 위치에서 거부하고 그 전까지 수행한 설정 쓰기는 보존한다. 헤더/type/길이/version 검증은 실제 `RawPacketDispatch`에 맡긴다.

`RegisterMissionSettingsPacket`은 원본 `0040b91a`의 등록을 따른다. observer는 **0040b380**이고 실제 기계어가 `ret 4`뿐이므로0/1 인자 모두 설정/packet을 읽거나 쓰지 않고 자연 반환한다. 이 검증된 원본 빈 몸체와 미복원 소비자를 가짜 no-op로 채우는 것은 구분한다. 버전1/불일치정책0과 실제 수신기를0x32에 등록하며, 기존 분배기의 보호PE 고정 길이32 표를 사용한다.

## 독립 원본 관찰과 회귀 검사

`tools/decomp_mission_settings_packet.py`는 PE를 가상메모리에 읽고 위5함수의 정밀 range만 허용한다. 송신117/수신45/observer2, 총 **164개 고유입력**이 정상 반환했다. cdecl/thiscall/stdcall 스택, EBX·ESI·EDI·EBP 보존과 native 쓰기guard를 확인했다. CRT/heap/OS 대체는0이다. 실제 송신만 외부 연결 경계로 관찰하며, getter/store 효과가 다른 현재 필드나 다음 packet 값을 변경하는 입력을 명시 공급한다. 기대 packet은 Python으로 만들지 않고 실제 PE가 쓴 전체32BYTE를 저장한다.

입력은0/ffffffff/부호경계/각7필드의1·80000000·ffffffff/seed패턴/target0·1·9·-1과 현재변경 없음·최초변경·매조회변경을 포함한다. 실제 수신은 헤더와 무관하므로 별도 pattern 헤더를 그대로 보존하는 것도 관찰한다. 각 native 행에서 전체 packet/전체 현재 설정28BYTE/실제 read 또는 write 순서를 대조한다. 같은164개를 재생성·C++ 대조 횟수와 중복 합산하지 않는다.

`MissionSettingsPacketTests.cpp`의 새3검사는 전체164행 재생, 실제 분배기에 등록한 송신→같은현재설정 수신 및 고정길이/버전/자연observer, 누락된 실제 소비자와 물리 부족 위치의 부분쓰기를 검사한다. root가 CMake에 등록해 **Release 경고0·오류0·새3개를 포함한 관련 집중42개 실패0**을 확인했다. 전체 회귀 최종 결과는 [중단 인수인계](cpp-pause-handoff-20261015.md)를 따른다. 실제 Session 설정 저장소/로그인 송신 연결은 이번 독립 모듈과 구분하며 Stage3나 자연완주 성공 근거로 사용하지 않는다.
