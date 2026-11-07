# 1127 — 67h 공통 State 부모와 일반 CS 연결

제품1127 / 표시·파일·제품버전11.01이다. 이전 단위는 `aebe0f12 / 20261108-79 CP`로 커밋됐다.

`004b4900` 전체 원문·각 다음 함수까지 기계어와 CS/범위/현재 비용/비율/HUD 하위 근거를 읽었다. `ReceiveStateBattleOptions`로 전체 부모를 공통화하고 기존 실제 일반 `RawMissionClientPackets.Options`가 이 몸체를 호출하게 했다. **메타 등록표는 아직1120의 CS 자식 직접 등록 상태이며 전체 부모 등록·공유 파생 전역·004c7880의 실제 HUD 재생성은 후속**이다.

진단 후 현재 WORD 길이를 다시 읽고 DWORD→나머지BYTE를 순서대로 복사한다. 전체 source/길이 선검사와std::copy_n 대신 선택한 필드 접근에서만 물리 범위를 확인한다. 미복사 꼬리와 실패 전 쓰기를 보존한다. 원본 전역87BYTE 밖 접근·비정상 포인터의 실제 메모리 손상은 재현하지 않고 호스트 오류로 구분한다.

복사 뒤 현재 CS 존재를 읽고 선택된 CS 자식→공급 반경→현재 비용 조회/F32→현재 TYPE 비용→현재 비율 저장→현재battle&&!mission&&saved의 HUD 장치 순서를 유지한다. 공통 부모의7개 함수는 동기const 참조로 빌려 첫 진단/복사 전에 불필요한 함수 객체 복사나 미사용 장치 검사를 추가하지 않는다.

일반 Session에서 비어 있던 `outpostOptions` 훅을 실제 CampaignLoginServices에 연결했다. 이는 역사적 이름이며 CS 자식0042c920을 뜻한다. 현재 Client의 CS·owner·수준9 로그·guy.wav(모든 인자0)·실제 NSInfo 갱신을 사용한다. 바인딩 때 CS/창을 요구하지 않고 상위의 CS 비영 관문 뒤 선택한 효과만 수행한다. 건물 Outpost 구현 완료로 세지 않는다.

첫 빌드 후 함수 장치를 동기 참조로 정리해 최종 재빌드했다. 메타 전체 부모 등록과 공유 공급 반경/파생 전역, 일반/메타의004c7880 전체 HUD 재생성 선택 장치는 다음 작업이다. 기존 MP fixture/새 단위/게임/통신 검사는 실행하지 않았다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일을 전체 메타 장면·MP 실행 성공으로 쓰지 않는다.

다음은 메타67h 전체 부모 등록/공유 반경·파생 전역/004c7880 HUD 재생성·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI이다. 전체 목표는 미완료다.

Netstorm.exe 12,725,248바이트/SHA-256 `20382d95226d68dbd646a29ed630413e6023da1dd0c65d2fcb90599b51c7c7fb`, R.exe 669,696바이트/SHA-256 `b3de455a2780dae152ca79a1731310dc2bae1303ccfd87e7f86981b866c0a49d`다. 보고서는 `cpppj/state-battle-options-20261108.json`, 원문/기계어·빌드·정적 감사는 `extracted/cpp-resume-1127/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 생성 소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
