// 한국어 글꼴의 공통 상수다. 값은 cpppj/assets/korean-font.json에서 빌드 때 생성하며 배포·검증도 같은 JSON을 읽는다.
#pragma once

namespace netstorm::client { // 한국어 화면과 외부 글꼴 탐색이 공유하는 설정 이름 공간이다.
extern const char kKoreanFontFamily[]; // 한국어 확장 글리프의 family 이름이다. Client와 검사가 함께 사용한다.
extern const char kKoreanFontRegularFile[]; // 실행 파일 옆 fonts/에서 찾을 일반 글꼴 파일명이다.
extern const char kKoreanFontBoldFile[]; // 굵기700 이상에 사용할 실제 굵은 글꼴 파일명이다.
extern const char kKoreanFontLicenseFile[]; // 두 글꼴과 함께 배포할 사용 허가 문서의 파일명이다.
}
