# 영상의 단일 수집·Renderer override 연결 (단위670)

원본 Movie Open `00484010`은 dialog 등록 `00466600` 뒤 `004654c0(movie,1)`으로 단일 수집 override를 설정한다. 현재 구현의 `SetOverlay`는 별도50칸 overlay 표와80000h 플래그를 설정해 다른 동작을 했다. 이를 기존 `GumpInput::SetOverride`로 교체했다.

옵션1/2→root 부착→depth -99→clear/present→Begin 순서는 유지한다. 같은 `RawButtonControls::CurrentOverride`가 공통 UI Collect의 시작 머리와 이미 복원한 Renderer 앞단의 override 입력에 전달된다. 영상 중 구름 Update와 행 스캔이 생략되고, Kill 뒤 기존 순회가 복원된다.

원문 `00466480`의 Kill은 현재 override가 자기이면0으로 지운다. 기존 `GumpLifetime::Recurse(false)`가 이를 이미 구현해 별도 close 정리를 추가하지 않았다. End 중에는 override를 유지하고 뒤 Kill에서 즉시 해제하며 singleton 객체 반납은 기존 Cleanup에 남긴다. 원문 기본 소멸자에 없는 임의 직접 Destroy 보정도 넣지 않았다.

## 검증

집중 대상의 최초 두 빌드는 기존 Editor 섬 창 검사 보조 함수의 링크 의존성을 빠뜨려 실패했다. 두 번째에는 정의가 없는 CampaignSessionTests를 연결해 같은 오류가 남았다. 실제 정의를 포함하는 CampaignExistingEditorCheckpointTests를 연결한 최종 세 번째 빌드는 경고/오류0으로 끝났다. 최초 실패 로그를 보존하며 제품 코드 오류나 검사 실패와 구별한다.

최종 집중129등록 중 관련10개/실패0(새2·기존8), 총5회/10함수 호출이다. 새2개는 Open의 최초 파일 준비만 읽고 present/Begin 앞 override·별도overlay 플래그 부재·End/Kill/Cleanup·재진입 Close·실제 UI Collect·같은 Renderer 분기를 확인했다. 새 검사에서 Idle/Draw/PCM 제출/창 재생은 하지 않았다. 기존 실패 Open1·CD 픽셀/네 종료 입력1·Renderer 앞단2·Frame4도 통과했다. 기존 픽셀 검사의 메모리 Draw와 새 최초 준비 범위를 구별한다. 최초 검사 실패는 없다.

기존 원문 앞단720행·투영90행의 합810행을 C++에서 재생했다. 새 Native 실행/fixture·신규MP 검사0이다. 게임 창/실제 PCM/전체 영상/전체34·새ZIP은 추가하지 않았다. 포획/희생·특정 적 공격·승리를 재현하지 않았다. 실제 물리 입력 환경은669의 캡처 실패 상태를 유지한다.

제품 SHA-256은 `468add9f653056db8d0dda9249d1136410c88814e7ff412ee3133d27b1b5e2d9`다. 관련 보호5개(AGENTS/양PE/TAFF/CD 인트로)는 전후 불변이며 앞4개는669 기준과 일치한다. 전체2632/fixture773 해시 재감사는 하지 않았다. 최신 전체34/ZIP은645의644제품이므로 이번 제품의 GUI 근거로 확대하지 않는다. 상세와 실제 로그는 [보고서](../../cpppj/movie-override-20261101.json)를 따른다.

정적 근거는 `extracted/refined/originals/Netstorm.c`의00484010/004654c0/00463fe0/004994b0/00466480과 보호PE `extracted/cpp-movie-override-670-static.txt`다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon /2026-10-23**이며 산출물 변경은 없다.10.72 판본의 추가 코드 복원/원본 실행은 이번 범위가 아니며 CD 영상은 읽기 자료다.
