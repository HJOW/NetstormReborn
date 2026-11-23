# webpj 의식 Regular 부착을 cpppj와 같게 (WB04-E1294, 2026-11-24)

E1228 읽기 전용 감사가 `RawCampaignRitual` 행에 남긴 차이 하나를 닫았다: 의식(신전·다이스·제단·룬·사제 기도)이 예약해 둔 Regular를 부착할 때 웹은 `shared` 표시를 켜고 있었다.

## 원문 (cpppj `RawCampaignRitual.inc` `AddRitualRegular`)

- `ReserveRitualRegular()`가 객체 하나를 확보해 두면 `AddRitualRegular(sid, event, payload, type)`가 그 객체를 `event`/`payload`로 다시 만들고 `core.processes.Attach(std::move(pending), type, sid, 0x50)`로 부착한 뒤 `SetNextTime(Now)`를 쓴다. `type`은 46(일반)·61(공유 번호)이다.
- 이 경로는 `SquidProcessHost::AttachRegular`(`shared_=true`, 1078판의 `basePostPop == 0` 진단, `SharedRegularType()` 선택)가 아니라 일반 `Attach`이므로 `shared_`는 거짓으로 남고 진단도 없다. 주석의 "서버/로컬 61·46 Form 정책"은 타입 번호가 정하는 Form 정책을 뜻한다.
- `shared_`는 Form 직렬화(`writeFormPacket`)에서만 쓰인다: 참이면 SharedRegular 28바이트 패킷을 쓰고 거짓이면 0바이트를 돌려준다.

## 웹의 이전 동작과 변경

- 이전: `www/core/raw-campaign-session-ritual.js`의 `addRegular`가 `processes.attachRegular(reserved, sid, type === 61)`을 불러 타입 61(신전·다이스·제단·기도 공유 Regular)에서 `shared=true`가 됐다. 그 결과 Form 직렬화가 타입 61 Regular에 28바이트를 썼고(내부 전송이 이를 소비하는지는 확인하지 못함), 부착 전에 `basePostPop` 깊이 진단이 한 번 더 실행됐다.
- 변경: `processes.attach(reserved, type, sid, RITUAL_ATTACH_FLAGS=0x50)`를 직접 불러 cpppj와 같이 `shared`를 켜지 않고 진단도 하지 않는다. 시각은 그대로 `setNextTime(now)`(웹의 `setNextTime`은 cpppj `SetNextTime`처럼 `time` 칸을 쓴다)이다. 타입 번호 46/61과 flags 0x50은 웹 기본값(`regularType`=46, `sharedRegularType`=61, `LOCAL_MASK`=0x50)과 같아 부착 결과는 같다.
- `tests/raw-campaign-session-ritual.test.mjs`의 두 단언(`reserved.shared`)을 `false`로 고쳤다: 이전 시험은 웹의 `shared===true`를 고정하고 있었다(E1228 노트: "원본 생성자 확정 못함").

## 검증

- 의식·다이스·제단·신전·룬·프로세스 관련 Node 시험 212건 통과.
- 실제 Chromium e2e 22건(`mission-object-commands`·`mission-priest-training-flow`·`fury-technology`·`knowledge-menu`) 통과(번들 기준).
- 영향 범위: `shared`는 Form 직렬화에서만 읽히므로 단독 미션의 의식 진행(희생·해방·룬 후처리)은 달라지지 않고, 멀티플레이 전송 경계(웹 제외)의 28바이트 쓰기만 사라진다.

## 대응표

`RawCampaignRitual.inc`(161줄)를 `partial` → `port`로 올렸다. partial은 9 → 8행, 남은 줄 수는 11,959 → 11,798이다.
