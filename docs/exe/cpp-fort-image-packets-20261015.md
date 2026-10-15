# 현재 Fort 섹션 이미지와 로그인 SendFort 복원

현재 PC와 마지막 전체 정밀 디컴파일 PC는 `HJOW-Athlon`, 2026-10-15로 일치한다. 보호10.78 `Netstorm.exe`의 `004bf520`, `0044d640`, `004bcd80`, `004a7240`, `0040a580`, `0040a540`과 실제 이름표/메모리stream/packet helper 몸체를 다시 읽었다. 보호 원본·AGENTS·dotnetpj·CMake·실제 Session을 변경하지 않았고 게임/GUI/공용 빌드는 실행하지 않았다.

## 현재 섹션의 실제 이미지

`RawFortImage`는 같은 현재 `FortTemplate&`의 `sections`를 Save마다 읽는다. 예전 파일 bytes나 해석된 Money/Technology 값으로 대체하지 않는다. 이 모듈은 원본 DataManager의 이미 작성된 raw stream을 저장하는 범위다. 월드 상태를 각 섹션에 수집하는 큰 Template Save 계층과 VFS/파일 선택은 별도 실제 소비자다.

원본은 먼저32768바이트를 할당하고 전체를0으로 만든다. 현재 typeCount의 `>255` 진단 뒤 `F`와 flag를 쓴다. 원본 reader가 저장한 flag 상태는 raw `fe`만 참이며, 직렬화 결과는 `fe` 또는0이다. `FortTemplate::SectionNames()`의 고정35순서(Subscriber부터 Reserved4, Terr00~Terr19)로 각 현재 payload를 기록한다. 원본 연결리스트가 역순이고 이름의 대소문자가 달라도 같은 이름 순서로 저장된다. 각 길이WORD는 자기2바이트를 포함하며, 이전 섹션 길이는 다음 섹션 진입 직전, 마지막 길이는 반환 직전에 수정한다. 현재 파일에 없는 tail도 실제 빈stream의 길이2가 된다.

논리끝과 물리할당을 함께 소유하는 `FortImageData`는 최종 `Bytes()`와 전체 `storage`를 구분한다. 논리끝 뒤의 모든BYTE가0인 것을 native와 C++ 검사에서 각각 확인한다. DataManager의 엄격한 `pos+end < alloc` 진단은 정확히32768이 되는 경우에도 호출되며, 그 위치의 실제 진단이 반환하면 물리범위 안 쓰기는 계속된다. 물리배열 초과는 이식 C++에서 명시 거부한다. 원본의 범위 밖 쓰기는 지원 범위로 확장하지 않는다.

## SendFort 현재 조회와 패킷

`RawMissionLoginPackets`는 같은9×180 Player 원문 span과 현재Template 조회기를 빌린다. 별도 Player/watchers 사본을 만들지 않는다. 실제 `004a7240`의 세 manager 읽기는 다음 위치를 유지한다.

1. 최초 현재Template 없음은 아무 로그/이미지/전송 없이 자연false 반환한다.
2. 실제 sendFort 로그 뒤 wrapper가 manager를 다시 읽고, NULL이면 실제 진단을 요청한다.
3. 진단 뒤에도 manager를 다시 읽어 그 실제 현재 대상을 Save한다.

따라서 로그가 Template를 교체하거나 진단이 설치하는 변경이 같은 호출에 반영된다. 초기 길이 `00544c48`의 현재WORD는 packet helper 앞에서 읽고, 현재 Player+58 BYTE는 그 뒤에 읽는다. 읽기 callback이 watcher를 바꾸는 회귀 입력도 보존한다. 누락된 로그/송신/진단은 실제 해당 분기가 선택되었을 때 거부하며, 가짜 성공으로 채우지 않는다.

| 오프셋 | 원본 필드 |
| --- | --- |
| +0/+1 | version1/type2a |
| +2 | 초기 `max(currentLength,4)` WORD, 이후 길이observer가 갱신 |
| +4 | owner DWORD |
| +8 | predictable DWORD |
| +c | imageSize DWORD |
| +10 | 현재 Player+58 watcher BYTE |
| +11 | 실제 현재 image payload |

실제 예약은 `imageSize+20`이며 전체를ff로 초기화한다. 마지막3BYTE는ff로 남는다. 실제 `0040a580(this,0)`이 image+11 주소를 구하고, 원본 `00482ba0`이 image를 복사한다. 길이observer `0040a540(this,write)`는 false일 때 packet조차 읽지 않는다. true는 imageSize==ffffffff 진단을 먼저 요청하고 하위WORD에 `imageSize+17`을 저장한다. DWORD/WORD wrap을 유지한다. 최종 `MissionFortPacket::Bytes()`는 예약tail을 제외한 실제 선언길이만 반환한다.

원본 `004a7240`은 외부 전송 `004e2dc0`에 **길이observer 전** 예약을 넘긴다. native 관찰기는 이 실제 전송 경계 진입 시 전체예약을 저장한 뒤 등록된 실제 `0040a540`을 실행하고 observer 뒤 예약/wire를 따로 저장한다. C++ `SendFort`의 외부 send 계약은 **이미 길이를 확정한 wire**다. 후속 실제 전송층에 연결할 때 이 차이를 적용해야 하며, 원본 전송/연결gate가 이미 복원되었다는 의미가 아니다.

## 독립 원본 관찰과 검사

`tools/decomp_fort_image_packets.py`는 보호PE를 Unicorn 가상메모리에 복사하고 허용된15몸체만 실행한다. 정상 반환주소/호출규약 스택/EBX·ESI·EDI·EBP/SEH 복귀와 실제 쓰기를 검사한다. 이미지 전체32768BYTE와 논리끝 뒤0을 매 입력 확인한다. 이미지 기대값은 Python으로 생성하지 않고 실제 PE가 작성한 bytes를 읽는다.

허용 몸체는 `004bf520`, `0044d640`, `0044cc20`, `0044cce0`, `004bcd80`, `004bccb0`, `004437f0`, `004a8850`, `004a8800`, `004a7240`, `0040a580`, `00482ba0`, `004df590`, `004e4a40`, `0040a540`이다. 메모리 할당/해제, ASCII 대소문자 비교와 Terr%02d CRT만 명시 대체다. 현재 manager/stream 준비는 명시 입력이며 로그/진단/실제 전송은 관찰 경계다. 원본 이름 생성기·이름 분리·검색 순서·실제 memset/이미지복사/packet helper는 원본문체를 실행한다.

현재35섹션·빈tail·역순 이름목록·flag 정규화·typeCount256·32768 경계·owner0/1/8·target-1/9·DWORD predictable·watcher0/1/ff·초기길이0/ffff·현재Template 교체/진단 복구/길이조회 중 watcher 변경과 false/WORD wrap observer를 포함한다. 보호 실제8미션은 다음 fort를 읽고 Reserved2의 **현재raw섹션만 수정**한 입력도 별도 포함한다.

| 실제 미션 | 보호 fort |
| --- | --- |
| Tutorial1~6 | BridgeTheGap / SecretWorkshop / CaptureThePriest / TacticalCombat / SubtleDefense / RawPower |
| 캠페인1-1/1-2 | TheWarBegins / MasterOfWhirligigs |

실제 고유입력은 **219개(I116/S93/F10)**이고 모두 정상 반환했다. 이미지 저장208회·sendFort93회(최초미설치1회 포함)·길이observer102회(송신92+직접10)를 실행했다. `--verify`가 fixture 행수/고유입력/SHA 감사를 통과했다. fixture SHA는 `49abe4d0f80a4a5c4293ae8d59580ae6e4f34feff69ea140594290bf2bf88d8a`다. 같은219 입력을 중간 프로브나 재검증 수와 중복 합산하지 않는다.

`FortImagePacketsTests.cpp`의 새4검사는 전체 fixture 대조, 보호8현재섹션의 변경/해석필드 무영향, 물리범위/선택진단, 실제 필수소비자 경계를 검사한다. root가 CMake에 등록해 Release 빌드와 새4개를 포함한 관련 집중42개 실패0을 확인했다. 전체 회귀 최종 결과는 [중단 인수인계](cpp-pause-handoff-20261015.md)를 따른다. 이것을 실제 로그인 전송/Stage3/AI 활성화나 자연 완주의 증거로 합산하지 않는다.
