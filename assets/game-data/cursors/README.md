# 원본 마우스 커서

Netstorm 10.78 `originals/Netstorm.exe`의 `RT_CURSOR` 리소스에서 이미 추출된 파일을 복사했다. 원본 실행 파일을 수정하거나 클론 실행 중 읽지 않는다. 각 파일은 핫스폿 좌표 4바이트와 단색 DIB를 합친 308바이트이며 크기는 32×32다.

| 파일 | 그룹 ID | 상태 | 핫스폿 |
|---|---:|---|---|
| `RT_CURSOR_8.bin` | 113 | 화살표 | (10, 6) |
| `RT_CURSOR_5.bin` | 109 | 금지 | (16, 16) |
| `RT_CURSOR_20.bin` | 148 | 사제 이동: 얇은 × | (16, 16) |
| `RT_CURSOR_7.bin` | 111 | 사제 선택 후 내 템플 위: 안쪽 화살표 네 개 | (16, 16) |
| `RT_CURSOR_6.bin` | 110 | 사제 건물 배치: 굵은 × | (16, 16) |

원본 실행 중 커서와 리소스의 픽셀 대조 근거는 [TEST02 분석](../../../docs/videos/auto-test02-construct-20261004.md) 5절이다. `CursorBitmap`이 Windows AND/XOR 비트를 SDL 단색 커서의 data/mask로 바꾸므로 투명·검정·흰색·배경 반전 픽셀을 보존한다. [SDL_CreateCursor의 픽셀 규칙](https://wiki.libsdl.org/SDL2/SDL_CreateCursor)을 따른다.
