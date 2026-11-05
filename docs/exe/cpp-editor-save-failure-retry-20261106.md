# Edit 맵 저장 공유 거부와 같은 Session 재시도 — 단위941

2026-11-06, HJOW-Athlon. LEFT_JOBS.md 3절2번의 **저장/재파싱·전환 실패 수명**을 진행했다. 마지막 전체 정밀 디컴파일은 같은 PC의2026-11-03 자료다. 제품939/11.01은 수정하지 않았다.

원문 일반 입력 `004d8163..004d8171`은 두 번째 인자가 비영일 때 `004bfb50`(현재 맵 저장)을 먼저 호출하고, 정상 복귀 뒤 `004b3540`으로 next3을 예약한다. 현재 `DispatchEditorSaveGoMain`도 저장 callback이 예외를 발생시키면 next를 쓰지 않는다. `RawCampaignSession::SaveCurrentMap`은 같은 현재 manager를 읽어 `SnapshotCurrentMap`의 Fort 분기를 실제 `SnapshotFort`에 연결한다.

원문 `0044d8d0`은 이미지 생성 뒤 권한 변경과 `0041bb90 → 0041b4c0`의 mode1 열기/생성 fallback을 수행한다. 현재 `EditorFiles::WriteTemplate`도 같은 r+b→w+b 정책을 사용하며, 둘 다 실패하면 `편집Template작업본파일열기실패`를 발생시킨다. 이미지/snapshot의 앞선 효과나 난수를 되돌리는 처리는 없다. 이 세 함수는 현재 정밀 대응표에 CD 대응 없음으로 표시돼 있어 이번에 CD와 같은 동작을 새로 주장하지 않는다.

새 검사는 기존 편집 설치 도우미로 실제 Client/Session/Template 저장소와 작업본을 준비하고, 실제 다리 품질을 바꾼 뒤 작업본에 Windows 읽기 핸들(`FILE_SHARE_READ`)을 연다. SaveGoMain Yes를 실제 전체 저장에 연결했을 때 다음을 확인했다.

- 기대한 파일 열기 오류가 발생하고 next91·phase·디스크의 기존 바이트를 보존했다.
- 실패 전에 수행한 전체 snapshot과 Badges 생성·난수 변경은 그대로 남았다.
- 검사 소유 잠금을 푼 뒤 같은 Session에서 재저장이 성공해 next3을 예약했다.
- 파일을 다시 파싱해 TypeNames·Technology·Territory·Deck·Money·Mission·Badges가 같은 현재 manager와 일치하고 삭제 깊이는0이었다.

최초 빌드/최초 검사부터 통과했다. 새1개와 기존 No/Yes·readOnly·파일 writer4개를 합쳐 **고유7개·실패0**, Release 집중 빌드 경고/오류0이다. 기존 manager0/Battle 분기 검사는 MP 이월 경로라 이번에 실행하지 않았다. 알려진 `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop` 기대 불일치를 새 검사로 숨기거나 완료로 바꾸지 않는다.

검사에 사용한 편집 내부 로컬 장치는 기존 도우미 그대로다. 새 패킷·TCP/IP·MP/통신 테스트는 없으며 실제 게임 창·State3 Tick·사용자 물리 입력도 실행하지 않았다. 포획/희생·특정 적 공격·승리·공중3종 전용 검사는 없다. 이 결과는 모든 저장 실패나 OS 부분 쓰기의 검증이 아니다.

보호2787개·fixture967개는 전후 불변, 새 fixture0이다. 제품 EXE 해시도940과 같다. [완료 JSON](../../cpppj/editor-save-failure-retry-20261106.json), Git 제외 `extracted/cpp-resume-941/`의 빌드/실행/원문 읽기/보호 해시를 따른다. 다음942도 **3절2번**이며 State1의 미션 요청/비동기 State7과 남은 편집 도구 경계를 대조한다.
