# 제품1218 — 다리 Regular의 장기 본문 연결

제품1218의 제한된 구현/검증을 마감했다. 전체 목표는 진행 중이다.

## 구현 범위

원본 PATCH00422740/004215d0과 CD00449a20의 정밀 C/전체 함수 PE를 읽기 전용으로 보존했다. 마지막 정밀 Client양판본2026-11-03·R전용2026-11-07은 현재와 같은 HJOW-Athlon이다. 새 디컴파일·원문 변경0이다.

현재 다리 VT005034c8의 +5c가 기존 `RawBridgeEvents`를 장기 Storage에서 호출한다. 사건2691은 현재 boss0이면0, 그 외 실제 가상삭제(flags0) 뒤-1이다. 사건2692는 같은 관문 뒤 원래 float payload의 _ftol 하위DWORD로 좌표를 풀고 기존 끝 칸 변환을 호출한다. 다른 사건은 count/TYPE/owner를 읽지 않고 payload를 반환한다. CD는 기존 인라인 대응 본문과 typed 경계를 유지한다.

`BindCurrentState`는 본문보다 오래 사는 실제 boss·같은 Native 가상삭제 모드의 debugKeep·현재 links.bridgeType 주소만 최초 결합한다. 값을 초기화하거나 임시 전역 사본을 갱신하지 않는다. 기존 독립 typed/CD는 원래 BridgeEventState를 계속 사용한다. Native World/Session과 NoWorld의 다리 Regular는 같은 Storage 본문을 공유하므로 World의 기본 authority 사본을 사용하지 않는다.

같은 장기 Neighbors/Factory·공통 owner/Pop·가상 삭제·건물 표면 통지를 사용한다. 끝 칸 변환의 임시 프레임/이웃 조회/원상 복구·이웃0 삭제·수명비트/알림·hard-frame 관문·새 객체 필드 복사/옛 삭제/Pop 본문은 재작성하지 않았다. World/Session의 강한 역참조·가짜 성공·새 Pool/난수 초기화를 추가하지 않는다.

## 검증 계획과 한계

신규 비MP8개는 무초기화·다른 사건 payload·현재 boss0의 두 사건/직접 변환·본문 생성 후 권한 변경과 실제 client SID 삭제·실제 L/J 프레임의 비변환/이웃0 삭제·실제 client ProcessForm 기본 Regular 종료·Host 현재 VT 재조회를 확인한다. 기존 BridgeEventTests7개의 양판본/10.37 x86 fixture와 공통 통합도 원래 기대값으로 대조한다. client SID와 원문 flags52/extra9·editor의 정상 관문을 사용하며 실제 네트워크 장치는 금지 callback이다. 새 MP/패킷/통신/공중3종 전용 실행은 하지 않는다.

분배상 연결은37/87VT, 잔여50VT/25함수로 좁혀지지만 큰 하위/모든 NoWorld caller·owner/실제 플레이 전체 완료 수는 아니다. 내장 Form 삭제3종·큰 비영 AI·다른 배치/Fort 상위 Finish·Source/listener/표/Poll 등 전수 감사·지연큐/JoinTestBattle·상위16계획/등록34버그는 유지한다. 자연 완주·포획/희생·특정 공격·승리 검사는 사용자 확인으로 남긴다.

## 확정 결과

Release/x64 제품2·집중5대상은 단일 최종 빌드 종료0/경고·오류0, 소스9개 고정이다. 비MP **328/328**(기존313+신규8+기존 다리7)을 통과했다. 원래 PATCH/CD/10.37 각1207입력·PATCH 추가 변환726입력과 기존 공통 통합을 기대 변경 없이 대조했다. 최초 빌드 C2065는 기존 다리 검사에 필요한 추가 BridgeDecay fixture 경로 정의 누락이었다. 원래 fixture 경로를 같은 집중 대상에 연결했고 제품 본문/fixture/검사 기대는 바꾸지 않았다. 최초 집중325/328의 신규 Host2는 abstract/void 부모 입력으로 원문 관문이 반환한 원인을 교정했다. 기존 공간 변환1의 미결합 서버 Pop 전송은 원문0x10 억제 입력을 연결했다. 모든 기존 물리/장부/해시/정리 단언과 원문fixture 기대는 유지했다.

같은 EXE에서 Tutorial4 정상 UI TYPE75/owner1 실제 생성/건설 요청 증가→600프레임 대기와015 PNG를 확인했다. 같은 Client800×600 Edit TEST01↔Main2회/11관측도 종료0이다. 다리 변환의 모든 실제 플레이·건물 완성/TestBattle·공중3종/포획희생/특정공격/승리/MP를 검증한 결과는 아니다. 모든 시작 빌드/집중/GUI 핸들의 실제 종료를 확인했다. 보호 원본/CD/fixture·AGENTS2800파일 불변, 원본 실행·MP 실행·원격 push0이다.

- Netstorm.exe: 13944832 bytes, SHA256 `4d3e07db776210488030a24bbec081da6945dd7ad8f5b11fa2fe037d1aa71a93`.
- R.exe: 677376 bytes, SHA256 `76570652136861aec6db951358795962f5f4e5844550517f1f491f83e5efb581`.

[현행 인계](../../LEFT_JOBS.md)·[최종 보고서](../../cpppj/bridge-regular-owner-20261109.json), 원시 `extracted/cpp-resume-1218/`의 original-evidence/static-audit/source-frozen/build/tests/GUI/보호 결과를 따른다.
