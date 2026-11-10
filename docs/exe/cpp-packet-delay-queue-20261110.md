# 기본 지연 FIFO와 실제 소켓 Poll 연결 — 작업1255

**지연 FIFO 본체·Client 장기 전역·실제 TCP/UDP Poll의 구현 단계를 마감했다. Native Session 재등록 연결이 남아 M6-l 전체/NoWorld/M6/목표는 미완료다.** 짧은 Session 콜백을 안전한 고정 F로 가장하거나 대기 중 old Runtime/lease pin을 보유하지 않는다.

원문4def40/4df010/4dee60/4def20/4df5e0/4df740/4dfc00과 종료/접속4df9f0/4dfa40을 대조했다. 보호 PE 분포는 기본00543600=`01ff`, lag1/2/3은005435e0/ec/f4이며double0050a668=0.001이다. ClientConnectionGlobals의 한 분포 저장소가 같은 FrameTime.wall(OS Capture가 갱신하는0055b4d8)과 실제 Client CRT를 보유한다. 최초439863의 기본 선택은 title 요청 전에 실행하며 후속435220의1/2/3만 현재 선택을 바꾼다. 기존 큐의 설치 분포는 Session/설정 교체 때 재설정하지 않는다.

100칸 ring/빈칸2미만 두 배 확장·signed BYTE 합계(ff/50상한)·동일 bucket 경계·고정 지연에서도 첫 bucket CRT 소비·delay-1만 추가 CRT를 보존한다. 집합은 반환>=0일 때 첫 지연을 공유한다. record/observer 앞에 저장한 type으로 그 슬롯의 현재 F를 한 번 선택하고 큐에는 불변 CanonicalPacketReceiver를 보관한다. enqueue 생성자의 observer1은 할당 뒤 그때 실제 packet[1]의 현재 observer를 읽는다. 송신자는 함수 진입 때, 주소20BYTE는 observer1 전에 값으로 저장하고 그 뒤 현재 선언WORD 길이만 복사한다.

FIFO 선두의 엄격한 deadline<현재frame wall만 처리한다. 같은 실제 connection의 깊이를 올리고 저장 주소로 바꾼 뒤 저장 F를 호출한다. packet 메모리 반납→주소복구→깊이감소→node반납/index 순서와 closed=-1을 유지한다. callback의 enqueue/Grow에도 안정된 Entry 포인터를 사용한다. 호스트 예외는 주소/깊이를 복구하고 실패한 선두를 보존하여 정상 배송으로 세지 않는다. Node release는 남은 큐를 반납한다. 지연 종료2 통지를 강제 close로 먼저 버리지 않는다.

실제 Client TCP/UDP의 원문 Poll에서만 기본 분포가 있고 큐가 없으면 설치한다. 내부 미션·Editor·R에 큐를 임의 설치하지 않는다. 기존 단일/집합 metadata·기록·폐기7f/80 정책은 유지한다.

## 발견한 필수 잔여

Native Session의 RegisterConstructionCommands/RegisterCarrierCommands/RegisterFactoryResearchCommands 등은 글로벌 고정 F를 짧은 this 콜백으로 다시 등록한다. RawPacketDispatch::Register는 canonicalReceive를 지우고 Touch는 lease 표식만 붙인다. 따라서 해당 상태의 실제 소켓 큐는 SnapshotReceiver에서 명시 실패한다. **현재 일반 미션 등록까지 지연 수신이 완성됐다는 뜻이 아니다.** 다음 단위에서 실제 Native 범위의 F/observer/version/길이를 대조하고 같은 고정 F 대여를 유지해야 한다. 독립 typed/CD·대기실 전용 표를 일괄 치환하지 않는다. 추가 ConnectionActivity/Chat/BattleLeave 등록도 실제 caller별로 감사한다. [발견 원시](../../extracted/cpp-resume-1255/native-registration-finding.json).

## 검증과 경계

최초 Release 빌드585.474초/종료0, 순서 교정 빌드50.553초/종료0 뒤 저장 type 차이를 찾아 다시 빌드했다. 최종 Release/x64 제품2·집중1대상 **644.173초/종료0·경고/오류0**, 고정13파일 불변·기존 **비MP300/300** 통과다. 새 MP 단위/패킷/통신/게임/기록 실행은0이며 지연큐 자체의 동작 통과를 주장하지 않는다. 원문/PE 정적 대조8행과 교정 내역을 보존했다.

최종 EXE Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 생성→600프레임 대기→정상 종료0/PNG 확인을 마쳤다. 단순 비MP 회귀이며 지연큐/MP·건물 완공·청취·전체 동일성 검증은 아니다. 실제 포획/희생·특정 적 공격·승리/자연 완주·공중3종 전용 실행도0이다.

보호 원본/CD/fixture·AGENTS75e4eed9e **2,808파일 불변**이다. 마지막 Client 양판본 디컴파일2026-11-03/R2026-11-07은 현재와 같은 HJOW-Athlon이며 새 디컴파일/원문 수정/ZIP은 없다. 글꼴 외부 fonts 계약은 M8/K1/K4/K5에서 계속 확인한다. Regular87/87은 유지하며 큰 NoWorld 하위·최초 후처리·전수 process/caller/owner·Path/AI/배치/Fort Finish·M6-m/n도 남는다.

[확정 단계 보고서](../../cpppj/packet-delay-queue-20261110.json), 원시 `extracted/cpp-resume-1255/`.
