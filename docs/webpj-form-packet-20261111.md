# webpj Form 패킷 작성기(RawFormPacket) — WB04-E312 (2026-11-11, 4-C)

3.1절 4-C 의 **RawFormPacket**(`o/`, partial, 기준선 118줄 = `.cpp` 76 + `.h` 42, 기준선 뒤 +12줄)을 `core/raw-form-packet.js` 로 옮겼다. 원본 Form 가상+54(감시 Form 13바이트 / 내용물 Form 18바이트)와 공통 삭제 가상+58(8바이트)의 실제 패킷 직렬화(10.78: 004ae560·004aeb40·004ac340, 헤더 작성 004df590)이다. 앞서 `squid-vtables.js` 에 Form 가상 표 4종 칸만 있어 partial 이었다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawFormPacket(pool, headers, stamp)`: `pool` 은 단일 실제 SID 풀, `headers` 는 `createFormPacketHeaders()` 의 현재 선언 헤더(감시 `0x65/1/13`, 내용물 `0x2d/1/18`, 삭제 `0x2e/1/8`; 요청 중에도 직접 읽음), `stamp` 는 `createFormDeletionStamp()` 의 한 번 소비 삭제 표식(`{ sid, token }`, 다른 삭제 요청과 공유)이다. 모두 빌린다.
- 헤더(004df590): `[protocol & 0x7f, message, 선언 길이 WORD(4 미만이면 4)]`. 반환 크기(13/18/8)와 선언 길이를 같게 강제하지 않고, 반환 크기 뒤의 옛 버퍼 바이트는 보존한다. 출력 폭이 부족하면 쓰기 전에 거부한다.
- `writeData(form, packet)`: 현재 raw 슬롯의 가상 표로 고른다 — Depend `0x512110` → 감시 13바이트(`+4` SID, `+6` 부모, `+8` 상태 BYTE, `+9`/`+11` 상대 Form 둘), Content `0x512208` → 내용물 18바이트(`+4` SID, `+6` `+12` WORD, `+8`/`+10` 상대 둘, `+12` 패치 WORD(`+24`), `+14` 부모, `+16` 순번 BYTE(`+23`), `+17` 상태 BYTE). 다른 가상 표는 거부(free/dead/void 필터는 추가하지 않음). 부모 getter 는 Depend/Process/Gump/Content 네 실제 표 중 하나여야 한다. 부모는 서버 번호여야 하고(5..14999 거부, 0..4 는 client 로 분류하지 않음), 감시 Form 은 서버 권한이 필요하며 0 이 아닌 상대 Form 과 그 부모도 서버 번호여야 한다.
- `writeDeletion(sid, flags, packet[, assertion])`: **현재 선언 헤더를 진단보다 먼저** 실제 버퍼에 쓰고(버퍼가 8바이트 미만이면 표식 변경 전에 거부), client SID(5..14999)면 선택한 실제 Client 진단 장치(`'!isClientSid()'`, `\Ns\O\Squid.cpp`, 줄 0x2f6)를 부르고 **정상 복귀하면 계속**한다(장치가 없으면 기존 명시 오류). 이어서 `+4` SID, `+6` WORD = `((flags >>> 16) & 0xfff0) | (표식 sid 가 같으면 token 의 낮은 WORD, 아니면 0)` 이고, 표식이 일치하면 sid 만 0 으로 지운다. 호출자가 현재 +58 분배를 확인한다.

## 2. 기준선 뒤 변경 판정 (+12줄, `b8cadde14`)

삭제 writer 의 헤더 선기록·client SID 진단 정상 복귀(독립 호출은 기존 명시 오류 유지)는 3.6절 ③ 4-C 로 넘겨 둔 항목이었고 이 단위에서 **② 반영**했다. 시험이 행 단위로 확인한다. Form 가상 표 4종 칸은 ①(`squid-vtables.js` 가 계속 제공). CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-form-packet.test.mjs` 5건)

- **원본 독립 관측 `form-packet-x86.tsv` originals 576행**(종류 D/W/C/V/X/Z 각 96행): 실제 Form 생성자로 만든 서버 영역 SID 슬롯 셋(20000~20002)에 행의 합성 raw 슬롯 50바이트를 쓰고, 초기 64바이트 버퍼·현재 선언 헤더·삭제 표식 입력에서 전체 출력·반환 크기·남은 표식·입력 슬롯 불변·래퍼 방송 수(W/V/Z=1)를 대조한다. originalCD 행은 읽지 않는다.
- 손 시험(cpppj `FormPacketTests.cpp` 이식 + 기준선 뒤 계약): 짧은 삭제/데이터 버퍼의 쓰기 전 거부(표식 불변), 삭제의 헤더 선기록·진단 복귀·표식 소비·flags 상위 WORD 합성, 미복원 가상 표(Gump/Process/미지) 거부, 부모/상대 Form 의 client SID 경계(0·4·15000 통과, 5·14999 거부)와 server 권한 거부, 구성 검사와 현재 헤더 재조회(선언 길이의 WORD 취급·4 미만 보정).
- 변형 확인 60개 중 시험 보강 뒤 **2개만 남았고 모두 동등**하다(`read` 의 `>>> 0` 은 가상 표 비교에서 음수 값도 어느 실제 표와도 같지 않아 같은 거부, 표식 token 의 WORD 마스크는 합성 뒤 낮은 WORD 만 쓰므로 같은 결과).

## 4. 남은 경계

- 3.1절 4-C 는 9/30 이다. 다음: RawSquidTransmission(공통 Squid 전송 2048BYTE 버퍼·헤더·압축과 가상 writer, 완료 뒤 `RawSurfaceWorld.bindSquidTransmission` 에 연결).
