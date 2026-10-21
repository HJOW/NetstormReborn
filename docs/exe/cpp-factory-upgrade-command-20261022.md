# Factory 업그레이드 요청과58h 수신

2026-10-22, 시작 HEAD `0a8ce947 / 20261022-05 CP`. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-21**로 같다. 기존 정밀 산출물과 보호10.78 PE 명령을 정적으로 읽었다. 새 디컴파일/원본 수정/검사·게임 재개는 없다.

[`RawFactoryUpgradeCommand`](../../cpppj/src/o/RawFactoryUpgradeCommand.cpp)는 원본 `00453bf0` 요청과 `00453d70`의58h kind0/mode1 수신을 복원한다. 기존 연구와 같은 메시지 생성/분배를 사용한다. **진행·완료·예측 그림과 입력 관문은 아직 후속**이며 전체 업그레이드 완료로 세지 않는다.

## 요청과 실제 수신

`Request(factory)`는 원본 new(0x28) 성공 시 event4/TYPE61 SharedRegular을 payload20초로 부착한다. 그 뒤 같은58h 생성자를 사용해 kind0/mode1/현재 now+**15초**의16바이트 요청을 채널9로 송신한다. 대기 예약20초와 메시지 완료 예정15초는 서로 다르다. 할당 실패에도 송신하며 예약 효과 뒤의 현재 시각을 읽는다. 자금/이미 진행 중인지/소유자 검사를 이 하위에 임의로 추가하지 않는다. 그 검사는 `004547d0/00453c90` 입력 부모의 후속이다.

`Receive(packet)`은 다음 원본 순서를 유지한다.

1. 현재 부모의 free/dead bit3이면 반환하고, kind0/mode1만 선택한다. 검사 모드에서는 Factory genus 진단 후 현재 genus를 다시 읽는다. 같은58h 연구 분기와 달리 sender/owner 일치를 추가하지 않는다.
2. 현재 BYTE TYPE의 복호화 비용 단위에 부호를 뒤집어 float로 저장하고 같은 실제 `SquidReward::AddSp`에 현재 owner를 전달한다. 로컬/원격 SP·AI 지갑 경로를 우회하지 않는다. 원본 종류4 예약 로그가 뒤따른다.
3. 예약 성공 시 현재 완료 예정 시각-now를 읽어 event3 SharedRegular payload로 전달한다. 유한 값은 최소1초다. 실제 x87 **test ah,41**은 unordered에서도 원래 차이를 선택하므로 NaN을1초로 바꾸지 않는다. 할당 실패해도 비용/방송은 되돌리지 않는다.
4. 하위 효과 뒤의 현재 boss가 참이면 같은 실제 `RawBattleBroadcast::Send`로 선언된 원문16바이트를 원격에만 방송한다. 호스트 span 뒤의 여분을 섞지 않는다. 원문/물리 범위 오류는 해당 필드를 읽는 지점에서 명시 실패하며 앞선 쓰기를 임의 복구하지 않는다.

Session에는 실제 분산SP·현재 지갑·예약 메모리→TYPE61 Attach/현재 시각·실제 요청 송신/원격 방송을 연결했다. `RawFactoryResearch::Receive`의 같은58h kind0/mode1은 이 수신 몸체로 위임한다. 명시 `RequestFactoryUpgrade`는 원본 요청 하위만 실행한다.

## 남은 업그레이드 경계

`RawAssetRegular::Factory`의 event3과 `004545e0` 완료는 아직 미복원이다. 따라서 AI 시설의 업그레이드 하위는 기존 선택 오류를 유지한다. 요청/수신을 구현했다는 이유로 AI 업그레이드를 자동 활성화하지 않는다. 다음 단계에서 현재 level/HP/상태 프레임, 예측 시설 그림·건설 진행/감시 쌍, 소리/플레어, event3/4·예측 그림 정리와 AI 재등록·지갑 보정을 원본대로 복원한다.

원본 수신의 실제 예약이 준비됐다는 사실은 자연 게임에서 업그레이드가 끝났다는 증거가 아니다. 입력 부모의 자금 부족/이미 진행 중 안내도 별도로 복원해야 한다.

## 확인 범위

새9검사는 모듈8개+실제 Session1개다. 요청20초/완료15초 wire·할당 실패·현재 시각, 수신 비용/예약/정확한16바이트 방송·최소1초/unordered·상태/genus/검사·하위 뒤 현재 owner/clock/boss·누락/짧은 원문의 접두 보존·기존 연구 분배와 실제 Session58h/분산SP/SharedRegular 연결을 다룬다. **작성·컴파일만 하며 실행하지 않는다.** Session 검사는 event3의 진행 프레임을 실행하지 않도록 구분했다.

Release 전체 빌드는 **종료0·경고0·오류0**이다. 세션53958의 실제 종료까지 조회했고 로그를 내용 변경 없이 UTF-8로 변환했다. 빌드/실제 종료는 `extracted/cpp-factory-upgrade-command-20261022-build1*`, 구현 근거6함수/남은 진행·완료2함수와 현재 소스/바이너리·링크 이름은 [`cpp_factory_upgrade_command_audit.py`](../../tools/cpp_factory_upgrade_command_audit.py)와 [신규 JSON](../../cpppj/factory-upgrade-command-static-20261022.json)에 남겼다. **검사/CTest/게임/native 실행0**이다. 이전 AI 시설 생산 보고서는 당시 증거로 보존한다. 인수인계 뒤`20261022-06 CP`로 로컬 커밋하며 원격 push하지 않는다.

전체 x87 확장 정밀도/예외·잘못된 네트워크 프로토콜 입력 전체·CD10.72와 자연15미션/데모 반복/Edit 동일성은 미검증이다. 다른 AI 행동·자동 프레임/종료와 Template/State 수명·저장도 남아 있다.
