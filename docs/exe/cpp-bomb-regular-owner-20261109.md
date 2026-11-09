# 작업1225 — Bomb 기존6VT의 장기 Regular와 실제5Fh 관찰기

기존 Session에서 처리하던6VT의 공통 Bomb Regular를 같은 Client 저장소에 연결한다. **제품1225의 구현과 허용 검증을 마감했다.** 전체 목표와 M6는 미완료다.

`RawSurfaceStorage`가 같은 Pool/Frame/현재 A 첫 프레임·casttime과 현재 권한/로컬 Player/5Fh 헤더를 사용하는 `RawBombRegular`를 보유한다. Native Session은 같은 본문·상태를 빌리고, 독립 typed는 기존 span/지역 상태·관측 계약을 유지한다. Native는 실제 TYPE reader 생성자를 사용하므로 미사용 TYPE 사본으로 길이 계약을 맞추지 않는다. 시전 시간188DWORD는 같은 실제 자산에서 최초 한 번 읽고 이후 대여/반납에서 현재 값을 덮지 않는다.

현재 Boss는 프레임 효과 뒤, 로컬 Player 전체 DWORD는 방송 효과 뒤 조회한다. Source/Session마다 미리 복사한 권한·소유자로 동작하지 않는다. A 첫 프레임과 casttime은 안내/프레임 효과 뒤 현재 raw TYPE에서 다시 읽는다. 초기5Fh 헤더는 보호 PE의5f01과00544c6c의 BSS WORD0이며, 독립 typed의 기존 선언 길이10 입력은 유지한다.

현재 큰 시전 플레어·41e4b0 전체 대상 검색·420bf0 전체 효과 수신은 weak 대여→실제 전체 Runtime pin→함수 사본→호출 순서로 연결한다. 실제 Runtime이 없는 선택 경로는 명시 실패하며 빈 성공으로 대체하지 않는다.400BYTE의 실제 DWORD100개 확보/해제는 Client가 중첩 호출별 LIFO로 보유한다. 검색과 패킷 대상 복사는 바로 그 확보 배열을 읽고 쓰며, 현재 대상 후단의 예외도 같은 실제 메모리를 반납한다. 현재 프레임/권한만으로 끝나는 분기는 World/Session을 요구하지 않는다.

## 실제 방송·관찰기

Native 공통 본문은210BYTE 예약 전체를 기존 실제 Client 방송/Registry/각 Source에 전달한다. 실제 Source의 관찰기 뒤에 길이를 읽는 기존 TCP/UDP/로컬 경로를 유지하며 별도 선행 길이 확정이나 성공 ACK를 만들지 않는다.

이 경로를 대조하면서 기존 Session 등록의5Fh observer가NULL인 점을 확인했다. 원문40b4b0의 `4ded70(5f,40aca0)` 등록과40aca0의 전체 C/PE에 따라 Client 장기 등록표와 현재 Native Session 등록표에 같은 실제 mutable 관찰기를 연결했다. 수신 방향은 payload를 읽지 않는다. 송신은 DWORD count==-1일 때 같은 Client 원문 진단(`count != (int)-1`, `\\Ns\\O\\_p_packets.cpp`,0x96b)을 호출하고, 정상 복귀 뒤 현재 low WORD를 다시 읽어 길이WORD만 기록한다. 기존 독립 typed의 엄격한 길이 마무리/NULL 관측 계약은 유지했다. 새 관찰기·MP/패킷/통신 실행 검사는 하지 않는다.

## 연결 범위와 남은 일

이번6VT는00502370·00502510·005025d8·00502e68·00502f30·00502ff8이다. 실제 +5c=0041e6a0을 같은 장기 본문으로 보낸다. 미연결10VT를 같은 후단으로 확대하지 않는다. **Regular46/87VT·잔여41VT/22함수**, 기본 Form 삭제4/4와 자산 Pre/Post/통지83/83을 구별한다. 남은10VT도 같은41e6a0을 공유하므로 고유 잔여 함수 수는22 그대로다.

다음은 [1224의16VT/19TYPE 행렬](cpp-bomb-regular-audit-20261109.md)을 따라 미연결10VT의 실제 대상 자격/효과를 복원하고 같은 장기 본문에 연결한다. 먼저 TYPE116 bombHeal의41ebb0/41ebd0을 확인한다. 큰 AI·다른 배치/Fort Finish·전수 caller/owner·M6-l/m·M7/M8·K/S/B도 남아 있다. 현재 Runtime이 필요한 큰 후단을 이번에 전부 NoWorld로 복원했다고 세지 않는다.

## 검증 기록

현재 HJOW-Athlon은 마지막 Client 양판본 정밀 디컴파일2026-11-03/R전용2026-11-07과 같은 호스트다. 새 디컴파일/정밀 원문 변경0이다. 이번 독립 전체 C/PE 요청은41e6a0·40aca0·40b4b0·4df590·4df230의5개이며 원시 `extracted/cpp-resume-1225/`에 보존했다. 최신 제품/현재 잔여는 확정 보고서에 기록했다.

시작한 빌드에서 새 검사의 서로 다른 두 auto 타입 선언을 발견해 해당 빌드를 종료한 뒤 교정했다. 종료/고정 소스는 cancelled-*에 보존했다. 다음 최초 빌드는436.142초/종료0·소스10개 고정, **비MP74/74**였다. 실제 TYPE reader와5Fh 관찰기 보강 뒤 빌드는426.420초/종료0·소스13개 고정, 같은74/74가 다시 통과했다. 기존59+신규15이며 새 검사는 실제 프레임·현재 TYPE/권한·시전 경계/signed count/NaN·실제 확보의 예외 반납·실제 World pin·NoWorld Host·대여/반납·기존 typed 입력만 확인한다. 방송/효과 패킷 실행 전에 끝나는 분기만 실행했다. 원본PE741입력의 전체 패킷 fixture는 이번에 실행하지 않았다.

최종 검토에서 Native의 확보 메모리와 실제 검색 출력 배열이 분리된 기존 구조를 확인하여 같은 실제 확보 배열을 사용하도록 보완했다. 이전 reader-observer-*의 빌드/74결과/고정 소스와 첫 GUI 결과는 보존했다. 최종 빌드는451.040초/종료0·경고/오류0·고정 소스13개 불변, **비MP75/75**다. 신규16+기존59이며 마지막 검사는 검색이 실제 확보 주소를 쓰고 패킷 실행 전 예외에서 같은 메모리를 반납하는지 확인한다.

최종 EXE의 Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 실제 새 서버 객체 생성→600프레임 대기/정상 종료0, Edit TEST01↔Main2회/11관측 종료0을 확인하고 PNG를 검토했다. 보호 원본/CD/fixture·AGENTS 2,801파일과 실행 전후 EXE 바이트가 불변이다. 시작한 빌드·집중·GUI 작업은 모두 종료했다. 전체 전투·주문 시전·건물 완성·Test Battle 확인은 아니다. 전체 전투/주문 시전·사제 포획/희생·특정 공격·승리·공중3종 전용·MP 실행 검사는 제외한다.

[현행 인계](../../LEFT_JOBS.md)·[직전 확정 제품1223](cpp-content-owner-20261109.md).

- Netstorm.exe: 14025728 bytes, SHA256 `07d2320656a7dff171db9359b7d6164dae88e74e535bc672461bc9e915329bc2`.
- R.exe: 677376 bytes, SHA256 `6dd6a6e061bb0be2d058b54defc1c9cafee8138ae6fbe601698df693e5a9c953`.

[확정 보고서](../../cpppj/bomb-regular-owner-20261109.json), 원시 `extracted/cpp-resume-1225/`.
