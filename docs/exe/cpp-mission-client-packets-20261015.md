# 로그인 패킷의 실제 클라이언트 수신

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 같은 전체 정밀 함수표의 실제 범위를 사용했다. 원본 게임·복사본·GUI·OS 소켓을 실행하지 않았고 보호 원본·AGENTS.md·Session·Client·CMake를 변경하지 않았다. 10.78 PATCH 전용이며 CD10.72는 이번 독립 관찰에 포함하지 않았다.

## 같은 현재 저장소와8종 수신

[`RawMissionClientPackets`](../../cpppj/src/o/RawMissionClientPackets.h)는 단일 Player9, `RawPlayerParticipation`, 실제 `ScrambledSpStore`, 현재 `PlayerWorldState`, State/시계/옵션/주소/CoreData 참조를 사용한다. caller는 참여 모듈에도 같은 Player9/방향별 동맹표를 공급해야 한다. 데이터 수신을 빈 성공으로 처리하는 등록은 없다.

| 패킷 | 원본 수신기 | 복원한 계산·쓰기와 필수 경계 |
| --- | --- | --- |
| `0x29` | `004b3b40` | 서버의 현재owner 진단→실제 두owner/known/CoreData 선택→현재6바이트 Zone 주소와 두seed 복사. owner0은 이전CoreData10바이트를cache에 보존하고 선택포인터/활성전역0 |
| `0x67` | `004b4900` | 현재87바이트 옵션의 지정 길이만 복사. 메타게임 객체가 있으면 실제 옵션 통지, 실제 범위 갱신→에너지 조회→현재TYPE+c4 float쓰기 경계→현재 파생값→선택된 재접속UI 순서 |
| `0x2b` | `004912d0` | 받은 이름 복사→실제 미션/현재local일 때 현재 자기이름 덮어쓰기→실제 이름정제→subscriber/색BYTE쓰기. 클라이언트만 IP/flags40보존 및 나머지 flags/동맹 원문 적용. 현재 채널 추가/제거→실제 `SetAlliance`→조건부 metadata/restore/UI 통지 순서 |
| `0x2a` | `004b3d30` | 현재 자기/다른owner, 서버, missionPost 및 hidden 조건을 보존. 선택된 Template 이미지 준비/읽기/Builder는 필수 경계. **로컬 미션 서버는 맵을 재설치하지 않고 revision만1 증가** |
| `0x28` | `004b8940`/`004b41e0` | 실제 버전10.78 이상 조건과 suppressVersionMessage/Tell. 거부는 현재State12/aux0→필수 동기State 실행. 정상은 payload double을 먼저 읽고 현재wall을 더한 뒤 **현재 phase를 다시 조회**하여 State17/18 또는13/11+camera+State실행. 다른phase는 sourceTicks0/pending3 |
| `0x81` | `004b3e90`/`00447810` | 현재 count를 반복마다 재조회. 음수 항목 생략→유효owner의 실제 같은 분산SP에 int→float 직접 지정. editor의194162.0f와 현재local/known의 조건부SP화면 갱신. AddSp/AI지갑으로 바꾸지 않음 |
| `0x84` | `004a6f30` | packet의signed초DWORD를 먼저 보존→실제CRT clock 저장→**signed clock−signed초×1000**의 절삭 하위DWORD를 현재 baseClock에 기록. callback이 같은 packet을 바꿔도 이미 읽은 값을 보존 |
| `0x6e` | `004a7050` | boss0은 Begin부호bit/현재 Ready mask를 적용하고 그때 현재local에40이 없을 때만40쓰기→실제서버9응답. boss1은 **sender9를 자연 반환**, 다른sender만40쓰기→실제 Ready 전체분배→그후 현재 로그인timer가0일 때만80000000 응답 |

이름정제의 공개 함수는 `SanitizeMissionPlayerName(subscriber, span<uint8_t>)`다. 실제 `0042b0e0`의 금지문자 삭제, NUL까지 이동, 빈 이름의 `Cheat<subscriber 하위BYTE>` 및 그뒤 stale 바이트 보존을 수행한다. Player 이름과 이후 서버의 SendPlayer 작성기가 같은 함수를 재사용할 수 있다. 앞선 Chat 내부 구현과의 공통화는 별도 편집 범위이며 이번에는 Chat 파일을 변경하지 않았다.

## 실제 등록과 남은 큰 서비스

`RegisterMissionClientPackets`는8종 모두 버전1/정책0과 실제 수신 함수를 등록한다. 수신 observer의 인자0 경로도 실제 빈 반환 몸체로 등록한다. 가변 observer의 **송신** 경로는 현재 `RawPacketDispatch::observer(uint32_t)`에 변경할 packet span이 없으므로 명시 거부한다. 고정패킷의 원본 빈 observer와 구분한다. 송신 writer/실제 전송이 이미 확정한 바이트를 사용하는 계약을 가짜 송신 observer로 대체하지 않는다.

`0x2b`는 서버여도 채팅·방향별동맹·UI알림을 수행한다. 같은 `RawPlayerParticipation::SetAlliance`가 현재표/현재owner/현재PlayerKnown을 직접 읽는다. 소유자 하나의 가입채널20 추가/제거와 전체동맹채널40/숨김80 재구성은 별개다. 새 `removeChat(subscriber,flags)`의 실제 `00444ff0` 하위도 연결해야 한다. 기존 `Clear(mask)` 전체채널 초기화로 대신할 수 없다.

숨김9BYTE `0052f81e`는 **옵션87BYTE `0052f7e0`의+0x3e**다. 수신기 생성자는 이 alias를 확인하여 별도 stale 배열을 연결하지 못하게 한다. 같은 참여 모듈도 이 현재 span을 빌려야 한다.

옵션 파생 범위/에너지/현재TYPE, 실제 Chat 채널, peer 주소, State 실행·카메라·SP Gump·Player 알림, 선택된 Template 파일/이미지/Builder는 도달 위치의 **필수 callback**이다. 누락 시 앞서 발생한 원본 쓰기는 보존하고 그뒤를 정상 수신으로 처리하지 않는다. `missionPost`에서 `004b54e0`은 원문처럼 자연 반환한다. 이를 빈 미등록 서비스 성공과 구분한다. 일반 Fort 메뉴/전투 자기파일 전체는 `localFort`의 명시 큰 경계이며 해당 내부 효과의 완료를 주장하지 않는다.

후속 실제 Session 연결에는 [서버 로그인](cpp-mission-login-20261015.md)의 `applyOptions/clearTypes/sendPlayer/metadata/Fort writer/설정패킷` 소비자도 필요하다. 중앙 metadata 패킷의 원본 타입은 **0x72**, 수신기는 `004b3bc0`이며 별도 root 작업 범위다. 이번8종에 중복 등록하지 않았다. 원본 Zone82000004의 bit100이0인 해당 수신기는 로그 뒤 자연 반환한다.

## 원본 대조와 검증 한계

[`관찰 도구`](../../tools/decomp_mission_client_packets.py), [`fixture`](../../cpppj/tests/fixtures/mission-client-packets-x86.tsv), [`SHA 보고서`](../../cpppj/mission-client-packets.json)에 **157개 고유 정상반환 입력**을 보존했다. 같은 PC의 전체 정밀 함수 범위를 좁게 허용하고 실제 명령의 쓰기 주소도 Player/동맹/현재전역/TYPE/이름문자표/스택 및 원본 SEH포인터4바이트로 제한했다. guard에 걸린 SEH쓰기0주소는 실제 `004b54e0`의 복원 쓰기임을 확인하여 그4바이트만 허용했다. 메타옵션/IP formatter의 실제 callee-clean8/12바이트 경계를 확인하고 반환 규약을 바로잡았다.

관찰은8개 수신기와 실제 이름정제/동맹/owner지정/버전검증/SP지정wrapper/Ready송신을 실행한다. 큰 Template/Builder·옵션 파생/TYPE조회·Chat/UI·시계·설정·CRT 문자열·실제 전송은 명시 외부 경계다. 현재 Player1620바이트, 방향별81DWORD/324바이트, 선택된 현재전역/double비트, 주소6/옵션87/cache10/TYPEfloat4, 논리SP9의 전체비트와 외부효과 순서, 정상 cdecl stack 및 EBX/ESI/EDI/EBP 보존을 확인한다.

SP 하위 `0040edb0`는 원본 관찰에서 논리SP 쓰기의 명시 경계다. C++은 실제 기존 `ScrambledSpStore::Set`을 직접 사용한다. [`검사`](../../cpppj/tests/MissionClientPacketsTests.cpp)는 그 실제저장소에서 읽은9개 float비트 전부를 원본 논리SP9와 비교하고, SP이외 모든 외부효과 순서를 비교한다. 원본 fixture의 `Set` 진입 기록은 기대 순서 비교에서 분리했다. 따라서 이 단위의157입력으로 분산SP 배열 전체나 난수소비를 새로 독립 대조했다고 주장하지 않는다. 해당 하위는 기존 SP 몸체의 별도 검증 근거를 따른다.

명시 callback 변경 입력은 wall조회가 현재phase를 바꾼 경우, Chat추가가 현재server를 바꾼 경우, 실제 localSP 표시갱신이 packet의 count를 바꾼 경우, Ready전체분배가 로그인timer를0으로 바꾼 경우를 다룬다. 기존 값을 캐시하여 다음 원본 읽기를 생략하면 이 입력에서 달라진다. 로컬서버9의 Ready 자연반환, 자기Fort 자연생략, 버전거부, 일부옵션복사와 기존stale값도 포함한다.

최초 공용 집중 실행에서 현재전역16개/효과순서17개의 CHECK가 실패했다(`extracted/goal-start-client-packets-focused.log`). 현재전역 실패는 decomp가 누락한 `004a6f50`의double1000곱과`fsubp`를 실제 어셈블리로 확인해 제품을 교정했다. 효과순서 실패는 native의옵션A5가숨김9에도겹치는반면 C++검사만별도0배열을썼던 입력 전제 오류다. 현재옵션+0x3e를직접빌리도록교정하고제품생성자의alias계약도추가했다. 독립 기대fixture나 비교문은 약화하지 않았다.

native 생성/고유성/SHA/UTF-8/후행공백 검사는 통과했다. 교정 뒤 고정소스로 root가 실행한 Release 빌드는 **경고0·오류0**, 새 수신2개(157입력 전수)를 포함한 관련 **집중52개 실패0**, 전체 **CTest1/1·내부1,290개 실패0·203.58초**다. `extracted/goal-login-packets-final-{build,focused}.log`, `extracted/goal-login-packets-{ctest,tests}.log`를 보존했다. 이 agent는 공용 빌드를 실행하지 않았다. 실제 게임 시작/완주·전략·모든 픽셀/음향 동일성은 미완료다.
