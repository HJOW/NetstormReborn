# 초기 지형 도장의 실제 월드 소비자

작업 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-13**이다. AGENTS.md와 두 인수인계를 재독했고 보호 원본/AGENTS는 보존했다. 게임·GUI를 실행하지 않았다.

`MakeTerrainStampWorldHooks`는 기존 `RawTerrainStamp`의 생성·소유자·Pop·지역 효과를 단일 `RawSurfaceWorld`에 연결한다. 실제 Factory와 `MakeOwnerDispatch`, Shape Pop/Display/PostPop, `SetSquidRegion`을 사용하며 코드 몸체를 복제하지 않는다. 현재 `bridgeState`를 `RawWorldCore`의 참조로 공개해 owner 분배가 실제 mutable 전역을 읽게 했다.

외부 ChunkMap 지역 조회·도장 assertion·지역 setter 진단은 필수다. 빠진 효과를0이나 빈 함수로 채우지 않는다. 반환한 훅을 쓰는 동안 world/state/진단 대상의 수명이 유지되어야 한다.

## 실제 소비자 확인

양판본에서 isle97 constructor→owner0→원본 초기 FrameAt→실제 Pop→표면 hash0→장부→지역5→현재 렌더 입력이 같은 SID/풀을 유지하는지 검사했다. SHP와8개 P/P 프레임 후보는 합성 입력이다. 정상 입력에서 ChunkMap 대체 조회가 요청되지 않음도 확인한다.

**새 소비자2검사와 기존 RawSurfaceWorld6검사가 통과**했다. 앞 단위의 TerrainStamp1560개 원본 입력은 재사용 근거로 구분하며 새 원본 관찰은0개다. 최종 공유 빌드/전체 검사는 단위08 종합 보고서에 기록한다.

`analyze_world_bridge_state_lineage.py`는 기준 커밋 `b76adf9`의 전체 헤더/구현을 참조 선언1개·반환 인자1개만 제거해 복원한다. 기존983개 완료 보고서/로그는 수정하지 않고 SHA로 연결했다.

## 연결 범위

원본 10.78과 CD10.72의 Factory·프레임/필드 폭·owner/Pop 분배는 기존 판본별 모듈을 그대로 사용한다. 전체 초기 성장/wrapper·실제 군집 배치·Fort 로딩 순서·게임 시각/전역 난수·GameWorld 입력/표시와 캠페인 완주는 후속이다. `BeginLoad`와 실제 Fort 로더의 loadingDepth를 조립할 때 원본의 증가 순서를 별도로 확인해야 한다.
