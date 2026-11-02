# cpppj 단위722 — 초기 지면99cache의 Client 수명 복원

2026-11-02 / HJOW-Athlon / Windows10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이며 새 디컴파일은 없다. 제품722 / 11.01이다.

## 문제와 수정

원본10.78 `004c04b0`는 전역 `005c8be4`가NULL일 때만99개DWORD(0x18cBYTE)를 할당한다. 최초 `_rand`99회와 인접mod4 중복 교정 뒤에는 다른 지형 생성자/월드에서도 같은 배열을 읽는다.10.72 `0049cf20`의 `00537190`도 기존 원본 fixture로 대조한다. 전체 raw.c의 직접 참조는 이 최초 생성/조회에 있으며, 간접 alias의 전체 부재까지 증명한 것은 아니다.

현재 `RawCampaignSession::Impl::stamp`는 값형 상태라 Session을 반납할 때99개 캐시가 사라졌다. 그 뒤 같은 게임 실행에서 미션/편집 월드를 다시 만들면 CRT99회를 다시 소비하고 내부3×3 지면의 변형 그림도 바뀔 수 있었다. 캐시만의 수명 문제이며,721에서 서로 다른 게임 실행을 비교했을 때의474개 frame 차이 전체를 이 결함 탓으로 단정하지 않는다.

이제 `ClientGameplayGlobals`가 `TerrainStampState`를 소유하고 Session은 같은 상태를 참조로 빌린다. 새Client에서는 variants가 비어 있으며 최초FrameAt가 실제99개 배열을 만든다. 일반/준비 대여와 반납은 이를 초기화하지 않는다. 초기 지면 캐시를 미리보기나 RegionPaint의 별도 캐시와 합치지 않는다. 콜백/Pool/지도/실행기는 기존 Session 수명에 남는다. 독립 Session의 자체 ClientGameplayGlobals fallback도 유지한다.

## 검증

`netstorm_terrain_cache_tests` **집중10개·실패0**이다. 신규2개와 기존8개이며, 기존 TerrainStamp 양판본의780행씩 총1560행을 그대로 대조한다. 신규 캐시 검사는 첫AA 요청의 frame/CRT/99개 값 전체를 원본 관측과 비교한 뒤, 다른 지도/Pool/실행기와 일반→준비 대여에서도 같은 배열·frame/추가CRT소비0을 확인한다. 새Client는 미생성 상태다.

두 번째 신규 검사는 실제 원본 `thewarbegins.fort`를 두 실제 `RawCampaignSession`으로 순차 적재한다. 같은 Client owner를 전달해 첫Load에서 장기99cache가 생성되는지 확인하고 첫Session을 소멸시킨다. 두 번째Session의 합법SP재시드 뒤 CRT에 명시 입력을 주고 Load하여, 캐시 전체와 첫 월드의 모든 관측된 AA 내부 타일 frame이 보존되고 CRT값도 추가 소비 없이 유지됨을 확인했다. RunFrame·AI 공격·포획/희생/승리나 게임 창은 실행하지 않았다.

최초tests-v1은10개 중 신규실제Session 검사1개가 생성자에서 실패했다. 검사장치에서 `gameplayGlobals`와 같은 Client의 `subscriberGlobals`를 전달하지 않아 기존 소유자 정체성 검사가 거부한 것이다. 제품의 거부 조건과 기대값은 유지하고, 검사 서비스에 실제 같은 두DWORD owner를 전달했다. tests-v2는10개 실패0이다. 최초 로그는 보존한다.

Release build-v1(제품/집중 실행기)·build-v2(검사 구성/명시algorithm include만) 모두 경고/오류0이다. EXE SHA-256은 `0f787b72ef4f81e95ac293a39c3217656d3629bc1871d9c96cbc47173f37bdc0`이다. 추가 실제 게임 실행은0이며,721 GUI 관측을722의 실제 실행 성공으로 합산하지 않는다. AGENTS·originals/CD·기존fixture2640개 전체 변경0이다.

## 남은 범위

원본과 모든 실행의 난수seed/99cache·그림 픽셀 동일성을 확인한 결과가 아니다. 배치 직후678/670과 다중 섬/편집 분할·병합, 남은 UI/파일/시나리오 수명을 이어 간다. 전체 C++·34·ZIP·PCM/물리 입력/Windows11·새MP/통신·제외된 재현 검사는 실행하지 않았다. 전체 동일성과 자연 완주는 사용자 확인 대상이다. [보고서](../../cpppj/terrain-cache-client-20261102.json), 원시 `extracted/cpp-terrain-cache-722/`를 따른다.
