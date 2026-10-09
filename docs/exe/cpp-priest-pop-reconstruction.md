# 사제 일반 Pop·Activate 연결과 영어 글꼴 확인

2026-10-10. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** AGENTS.md·두 인계·cpppj 문서를 다시 읽고 같은 호스트에서 새 `priestpop` 목록 **24/18/18개**를 읽기 전용으로 내보냈다. 10.78 싱글플레이 복원이 우선이며 outpost/LAN은 3차, 한국어/요구사항/MCP는 4차다. 게임 프로세스/창은 실행하지 않았다.

후속 완료(2026-10-10): [보호막 일반 Pop·Regular·공간 삭제 수명](cpp-forcefield-lifecycle-reconstruction.md)을 실제 공통 공간/장부·form/Kernel에 연결했다. 아래 보호막 Pop/destroy 미복원 표기는 이 후속 단계 전의 범위다. SfxProcess/실제 소리 수명·월드/GUI 연결은 계속 후속이다.

[`SquidPostPop::SetPriestPostPop`](../../cpppj/src/o/SquidPostPop.h)는 [전체 사제 후처리](cpp-priest-postpop-tail-reconstruction.md)를 명시 등록한다. 이후 실제 사제 타입 158과 가상 표가 일치하면 `SquidPop`→공통 firstPop/Activate→목록/회복 접두→Carrier/Damageable/공통 장부→현재 이동 불가/지면/방향→낙하/보호막 순서로 실행한다. 연결을 해제하거나 전체 후처리를 공급하지 않으면 기존처럼 공간 변경 전에 거부한다. 전역 `SquidPop::Supports`의 가상 표 목록은 그대로다.

## 원본 대응과 사용 계약

| 동작/가상 표 슬롯 | 10.78 | CD·추가 10.37 |
|---|---|---|
| 실제 사제 가상 표 | `0050f210` | `005003e0` |
| 공통 Pop (+0x90) | `004b02d0` | `004ad490` |
| 공통 firstPop (+0x30) | `004ad470` | `004ae0f0` |
| 공통 Activate | `004ac560` | `004abfb0` |
| 사제 전체 postPop (+0x20) | `004950f0` | `0040c110` |
| 공통 Unpop (+0x48) | `004afe50` | `004ad0b0` |
| Repop (+0x4c) | `0041c0d0` | `00401c60` |
| 공통 표시 갱신 (+0x88/+0x8c) | `004ad500` / `004ad670` | `004ae380` / `004ae3e0` |

실제 PE의 가상 표를 수정하지 않고 위 대응을 확인했다. 사제는 Pop/firstPop/표시를 공통 구현과 공유하고 postPop을 재정의한다. `Activate`는 깊이를 올린 뒤 사제 전체 wrapper를 부른다. 그 wrapper의 Carrier→공통 base가 깊이를 한 번 내리므로 기존 접두 분배 방식처럼 공통 postPop을 추가 호출하면 안 된다. 이번 연결은 반환 깊이가 호출 전 값인지 검사한다.

구성 순서는 공통 `SquidPostPop`/`SquidPop`→실제 HP/사제 접두/Carrier/상태/후반→`SetPriestPostPop(&tail)`이다. 사제 후반과 모든 참조 대상은 연결보다 오래 살아야 한다. 같은 SID 풀은 구성 시 확인한다. **Carrier의 base 효과는 등록 대상인 바로 그 `SquidPostPop::PostPopBase`로 연결해야 한다.** 같은 풀의 다른 장부나 사용자 효과는 자동으로 판별하지 않으며, 잘못 연결된 깊이는 실행 후 진단한다. 외부 효과 뒤 실패의 되돌리기는 제공하지 않는다.

접두의 타입/가상 표/목록 개수 검사는 공간 쓰기 전 `Validate`에서 한다. 현재 좌표의 이동 불가/지면은 미리 읽지 않는다. 이 값은 실제 Pop과 Carrier 효과 뒤에 다시 읽어야 원본 순서를 보존한다. 목록이 꽉 찬 경우의 삽입 실패 등 실행 중 실패까지 모두 사전 판별하는 계약은 아니다.

## 독립 원본과 실제 모듈 검증

[`decomp_priestpop_oracle.py`](../../tools/decomp_priestpop_oracle.py)는 원본 PE의 공통 Pop 진입부터 실제 firstPop/Activate/사제 wrapper의 정상 반환까지 실행한다. 세 판본 **각 1,280개·총 3,840개**, 두 x87 제어 워드 `0x027f`/`0x037f`에서 같은 관찰을 저장했다. 각 PE Pop/Activate/사제 wrapper **2,560회**, firstPop **1,280회**이며 총 정상 Pop 반환 **7,680회**다.

입력은 extra 0/1/8/9/128/129/136/160, flags 0/8/0x50/0x800/0x2000, HP 20/150, 지면 0/6, 방향 A/J, 기존 회복/확보 성공 여부다. 첫 등록·재등록의 정규화 flags, raw 슬롯 전체, 해시 머리, stale 칸을 포함한 사제 목록과 소유자 표, 효과 순서를 대조한다. 스택/비영 EBX·ESI·EDI·EBP/SEH/x87 제어·TOP, 공통 깊이, 허용 코드/쓰기, assert/OS 0, SHA/행/입력 누락·중복과 실제 진입을 감사한다. [fixture](../../cpppj/tests/fixtures/priestpop-x86.tsv), [근거 JSON](../../cpppj/recovery-priestpop-evidence.json).

**원본 실행의 경계:** Carrier 몸체는 정규화 flags 기록과 base 깊이 감소로 대체한다. Regular 검색/new/생성, 낙하, 보호막 생성/삭제도 명시 경계다. 타입/프레임/SHP/지면은 합성 입력이고 비멀티플레이·표시 억제 경로다. Carrier/공통 장부·사제 회복/낙하 몸체는 앞 단계 독립 근거와 이번 C++ 합성 검사로 연결한다. 전체 원본 게임/오디오/GUI 실행 검사는 아니다.

[`PriestPopTests.cpp`](../../cpppj/tests/PriestPopTests.cpp)는 세 판본 fixture 재생과 두 raw 배치의 실제 Pop/장부·HP·회복 예약/SharedRegular/Kernel·낙하/프레임 지정/진행·Display/Unpop/재등록을 검사한다. 최초 Pop의 저 HP→회복·낙하 예약→권한 있는 0x800 중첩 Pop→J 진행→SHP 크기 단계에 따른 해시 1→3 이동→착지/예약 제거에서 비용/개수/목록이 중복 갱신되지 않는다. 생성자는 합성 입력이며 실제 사제 가상 표를 사용한다. 보호막/소리는 명시 호출 경계다. 연결 누락·다른 풀·잘못된 목록 개수는 공간 변경 전 거부한다.

최종 검증 결과: Release 경고/오류 **0**, CTest 내부 **462개·실패 0**(115.39초), 원본 근거 감사 **70종 모두 통과**. 새 C++ 검사 **5개**, 새 원본 **3,840개**, 누적 인계 **384,382개**. 기록은 `extracted/priestpop-{export,oracle,build-final,ctest}.log`·`priestpop-audits.json`이다.

## 영어 원본 글꼴과 한국어 D2Coding

영어는 이미 [`FontStore`](../../cpppj/src/client/BitmapFont.h)의 원본 `.chfnt` 비트맵 캐시를 사용한다. 기본은 원본 Arial이며 콘솔 슬롯은 Courier New다. 캐시가 없거나 손상되면 원본 슬롯/스타일에 맞춘 GDI 글꼴을 메모리에서 만들며 원본 폴더에 캐시를 쓰지 않는다. Arial이라는 이름만 맞추는 것보다 원본에 저장된 문자 폭/픽셀을 우선 사용하는 방식이다.

기존 cpppj 브리핑/안내 본문은 슬롯 5(Arial 14, weight 0)를 사용했다. [원본 브리핑·도움말 캡처 대조 기록](../dotnet-reconstruction-20261010.md)에 따르면 브리핑 본문·버튼은 **슬롯 0 `!Arial.normal.14.700.chfnt`**, 제목은 슬롯 3(20/700), 실제 도움말 문서 본문은 슬롯 5(14/0)다. [`UberGump`](../../cpppj/src/client/UberGump.cpp)의 브리핑/안내 본문 기본 슬롯을 **0**으로 수정했다. 현재 Help 메뉴는 About 안내이며 전체 도움말 문서 UI는 미복원이다. 이후 그 문서는 슬롯 5를 써야 한다. 이번 수정은 전체 HTML 스타일/간격/현재 창 픽셀 대조 완료를 뜻하지 않는다.

[`cpp_renderer_smoke.py --console-only`](../../tools/cpp_renderer_smoke.py)를 추가해 창/설정 변경 없이 실제 C++ 콘솔 덤프와 독립 Python 해독을 대조했다. **18개 캐시·4,608자·302,480픽셀**, 모든 폭/픽셀 일치. 두 판본 커서 리소스 표도 각 18개를 확인하고 원본 1,394개 파일의 이름/내용 해시 불변을 확인했다. 결과 `extracted/cpp-renderer-smoke/console-report.json`. 재현: `python -X utf8 tools/cpp_renderer_smoke.py --console-only`(pefile/Pillow 의존성 필요).

**언어 정책은 영어=원본 캐시, 한국어=D2Coding으로 유지한다.** `fonts/D2Coding-Ver1.4.0-20261003-all.ttc`가 준비되어 있다. 현재 cpppj의 텍스트 경로는 256문자/CP1252이므로 한국어 UI용 Unicode 레이아웃·D2Coding 렌더링·언어별 선택은 아직 구현되지 않았다. D2Coding을 영어 기본 글꼴로 대체하지 않았다. 한국어 경로는 4차 목표에서 이 정책대로 추가해야 한다.

## 다음 단계

일반 사제 Pop/Activate 분배는 이번에 연결했다. 보호막의 실제 Pop/destroy/소리 수명, 월드/GUI가 이 raw 연결을 사용하는 객체 수명, 저장 맵 비표면 객체·미션 목록 수명, GUI 건설·경제·전투·승패가 남았다. 미션 완주는 아직 불가능하다. 장시간 변이·실제 자산 전수·최대 지도/SID 소진·창 픽셀·Windows 반복 실행은 별도 검증이다.
