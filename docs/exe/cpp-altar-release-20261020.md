# 제단 해방의 가상 소유자와 홈섬 밖 경고 — 단위241

작업일은 **2026-10-20**, 시작 커밋은 `00ce6d72`다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**이다. 기존 정밀 자료를 다시 읽었으며 새 디컴파일·원본/CD 변경은 없다. 다른 세션의 dotnetpj 변경은 수정하거나 이 커밋에 포함하지 않는다.

## 원본 호출과 변경

10.78 사제 해방 `00448500`은 새 사제를 만든 뒤 포획 내용물의 WORD를 복사하고 **가상+74(0)**을 호출한다. 사제의 `00491790`은 전투/비로딩 상태에서 인자0 대신 그 WORD 하위7비트의 원래 소유자를 적용하고 현재 로컬 표식도 갱신한다. 기존 Session의 제단 연결은 공통 `core.owner.Set`을 직접 불러 새 사제를 소유자0으로 지정했다.

`RawCampaignRitual.inc`의 `altar.setOwner`를 기존 `RawCampaignOwnership::SetOwner`에 연결했다. 기존 사제 전용 분배기를 통하므로 원래 소유자/로컬 표식/가상 통지가 원래 순서로 실행된다. 해방 위치·Pop·5.0/0 회피·내용물 삭제·음향은 기존 몸체 그대로다. 이 문제는 소스 대조로 확인했으며 이번 실제 실행에서 재현했다고 주장하지 않는다.

다이스 `00449320`의 홈섬 밖 경고는 `004cf960("WarnCantUpgradeAltarOffHomeIsland")`, 즉 같은 **Tell(key,0)** 안내 창이다. 미연결 `life.chat`을 기존 `services.priest.shield.tell`에 연결했다. 로컬/비로딩/다른 홈섬/옵션/`missionPost` 관문은 기존 `RawDaisLifecycle`이 그대로 판단한다. 캠페인에서 억제되는 경고를 강제로 띄우는 상태 변경은 없다.

CD10.72 해방 대응은 `00482ec0`이고 가상 소유자 호출도 동일하다. CD의 추가 `enemyPriestEsacping.wav` 등 판본 차이는 [기존 해방 분석](cpp-altar-sacrifice-20261014.md)에 남겨 두며 이번 Session은 10.78만 대상으로 한다.

## 검증과 이월

`CampaignAltar_AlliedReleaseUsesVirtualPriestOwner`를 추가했다. 실제 `thewarbegins.fort`를 로드하고 명시 다이스/소유자1 포획 내용물을 만든다. 기본 동맹1→1을 제단의 실제21 예약이 조회하도록 진행하고 새 사제의 현재 owner1·원래 WORD·로컬 표식, 포획 내용물 제거·제단P/예약 종료·깊이 복구를 확인하도록 작성했다. 기존 사제나 결과 상태를 덮어쓰지 않으며 자연 포획/플레이의 증거가 아니다.

Release 빌드·해시·현재 핀 보존은 `cpppj/altar-release-session.json`과 `python tools/cpp_altar_release_audit.py --verify`를 따른다. 이전 단위의 `cpp_dais_load_audit.py`도 후속 핀 이력을 역변환해 역사 보고서를 대조하도록 수정했다. 과거 결과/원본 fixture는 보존하며 현재 소스 갱신을 과거 검사의 재실행으로 표시하지 않는다.

사용자의 통합 검증 이월 지시대로 **게임·단위 검사·CTest·전체 회귀를 실행하지 않는다.** 빌드가 성공해도 새 자동 해방 검사, 실제 경고 UI와 완주가 통과한 것은 아니다. 통합 검증에서 이번 검사와 앞 단위의 저장 다이스 검사·공통 객체 명령 검사, Early5 UI 시전·지형 효과·포획/Drop·제단/희생, 나머지 지정 미션/F8 Guide를 함께 확인한다. 초기 다이스 추가 그림·기타 미연결 게임 기능과 전체 동일성은 남아 있다.
