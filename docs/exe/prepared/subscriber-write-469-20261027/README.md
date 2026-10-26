# 469 Fort Subscriber 저장 child 준비

제품 src/tests/CMake/기존 fixture는 수정하지 않았다. `work/`의 새 모듈/도구/fixture/replay/보고서/문서와 `fort-subscriber-write-469.patch`를 검토한 뒤 다음 단위에 적용한다. `base/`는 좁은 Storage API 변경의 비교용이다.

## 호출 계약

`o::SaveFortSubscriber(owner,capturedId,capturedHandle,hooks)`를 사용한다. `capturedHandle`은 NUL을 미리 검색/절단하지 않은 기존 물리 span이다. `capturedHandle.data()==nullptr`인 경우만 nullopt로 변환한다. 현재 manager getter/실제 FinishFortSubscriberRegistration/진단을 연결한다.

Create는 전달된 manager에 `storage.CreateManagerSection(manager,"Subscriber")`를 실행하고 반환 typed stream을 한 동기 호출 동안 고정한다. 일반 write는 같은 stream에 `storage.WriteSection`을 호출한다. 부모가 이미 선택한 manager를 다시 slot에서 고르지 않는다. 이 API는 NewFort와 DataManager 초기화 staging에서도 공유할 수 있다.

## 채택할 파일과 CMake

- 새 `src/o/RawFortSubscriberWrite.h/.cpp`
- `src/client/CampaignTemplateStorage.h/.cpp`에 추가한 `CreateManagerSection` hunk. 기존 본문은 보존한다.
- 새 `tests/FortSubscriberWriteTests.cpp` 및 `tests/fixtures/fort-subscriber-write-x86.tsv`
- 새 `tools/decomp_fort_subscriber_write.py`, `cpppj/fort-subscriber-write-native.json`, 작업 문서

CMake는 수정하지 않았다. 원문 모듈 target에 새 RawFortSubscriberWrite cpp/h를, 테스트 target에 FortSubscriberWriteTests.cpp를 추가한다. fixture는 runtime 저장소 검색 또는 `NETSTORM_FORT_SUBSCRIBER_WRITE_FIXTURE` 정의로 제공한다. 최초 fixture의 LF 바이트를 보존해야 Native SHA 검증이 유지된다.

## 검증 상태

독립 PE45입력×2=90관측·575,800명령, 같은 도구 `--verify` 실제 재실행90/차이0을 확인했다. C++ 검사2개는 미컴파일/미실행이다. 게임/OS/GUI/원본 저장과 fixture 덮어쓰기는 하지 않았다. 생성기 멤버 설명 주석 보완의 SHA 메타데이터 갱신만 기록했고 최초 fixture SHA는 유지했다.

최종 patch 적용 검사/UTF-8/공백/각 파일 해시는 `stage-manifest.json`에 기록한다. 기존 제품 빌드 동결 중이므로 실제 적용/빌드/커밋/LEFT_JOBS 갱신은 root가 담당한다.

준비 완료 patch SHA256은 `e4952ea6ce97170dbd9dbcce639cadeeffadc71cfc7d347d0e70f47ace9c941b`다. 현재 Storage 원문2파일 SHA 일치, patch9파일/+608줄의 `git apply --check --whitespace=error`를 통과했다. JSON 보고서의 CRLF만 LF로 정규화해 patch 공백 경계를 맞췄고 Native fixture 바이트는 변경하지 않았다.
