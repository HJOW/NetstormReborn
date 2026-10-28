namespace Netstorm.Client.Display;

/// <summary>
/// 원본 VFX 변환 커널의 회전 없는 16.16 확대·축소를 현재 8비트 표면에 복원한다.
/// 근거: cpppj/src/client/VFXTransform.cpp (원본 004028cb, CD 004662ab의 angle=0 분기). (R2d-1, 2026-10-28)
/// 합성은 <see cref="RendererDraw.DrawIndexedImage"/>와 같은 규칙(투명 유지·색 치환 뒤 255도 유지)이다.
/// </summary>
public static class VfxTransform
{
    /// <summary>원본 배율 1과 텍스처 좌표 한 픽셀이 쓰는 같은 16.16 단위.</summary>
    private const int FixedUnit = 0x10000;

    /// <summary>텍스처 좌표의 반 픽셀 편향. 꼭짓점 64비트 곱의 하위 DWORD에만 더한다.</summary>
    private const uint TextureBias = 0x8000;

    /// <summary>변환 임시 표면의 투명 번호. 색 치환 뒤에도 같은 값은 대상 픽셀을 유지한다.</summary>
    private const byte ScratchTransparent = 255;

    /// <summary>x86 고정소수점 좌표의 정상 signed WORD 하한.</summary>
    private const int WordMinimum = -32768;

    /// <summary>x86 고정소수점 좌표의 정상 signed WORD 상한.</summary>
    private const int WordMaximum = 32767;

    /// <summary>
    /// 기준점 이동의 하위 DWORD를 보존한다. C++ signed 넘침을 일으키지 않는다.
    /// </summary>
    /// <param name="left">왼쪽 피연산자.</param>
    /// <param name="right">오른쪽 피연산자.</param>
    private static int Add(int left, int right) => unchecked((int)((uint)left + (uint)right));

    /// <summary>
    /// 꼭짓점의 원본 IMUL/ADD 8000/ADC를 계산한다. 음수 몫은 아래 정수로 내린다.
    /// 반 픽셀 일반 반올림과 달리 편향은 64비트 곱의 하위 DWORD에만 더한다.
    /// </summary>
    /// <param name="coordinate">프레임 사각형 좌표.</param>
    /// <param name="scale">16.16 배율(음수는 축 반전).</param>
    private static int ScaleCoordinate(int coordinate, int scale)
    {
        long shifted = (int)((uint)coordinate << 16);
        long biased = shifted * scale + TextureBias;
        const long divisor = 1L << 32;
        long quotient = biased / divisor - (biased % divisor < 0 ? 1 : 0);
        return (int)quotient;
    }

    /// <summary>
    /// 원본 signed IDIV의 절삭 기울기다. 분모는 양수이며 검증한 간격만 전달한다.
    /// </summary>
    /// <param name="delta">텍스처 끝과 시작의 차이.</param>
    /// <param name="span">화면 끝과 시작의 간격.</param>
    private static int TextureStep(int delta, int span) => (int)((long)delta * FixedUnit / span);

    /// <summary>
    /// 원본 좌표 누산의 DWORD 감김을 유지한다. 잘린 첫 픽셀까지도 같은 기울기로 전진한다.
    /// </summary>
    /// <param name="value">현재 16.16 텍스처 좌표.</param>
    /// <param name="step">한 픽셀의 기울기.</param>
    /// <param name="count">전진할 픽셀 수.</param>
    private static uint Advance(uint value, int step, int count) => unchecked(value + (uint)step * (uint)count);

    /// <summary>
    /// 픽셀 중심 좌표를 만든다. 호출자는 0~32767의 원본 픽셀 인덱스만 넘긴다.
    /// </summary>
    /// <param name="index">원본 픽셀 인덱스.</param>
    private static uint TextureStart(int index) => unchecked(((uint)index << 16) + TextureBias);

    /// <summary>
    /// 원본 WORD 좌표를 정규화 없이 해석할 수 있는 범위인지 검사한다.
    /// </summary>
    /// <param name="value">검사할 좌표.</param>
    private static bool IsWord(int value) => value >= WordMinimum && value <= WordMaximum;

    /// <summary>
    /// 일반 VFX 해제 그림에서 임시 255 표면과 같은 결과를 직접 샘플링한다.
    /// 배율이 1이면 일반 출력 분기로 위임하며 255도 불투명 색으로 유지한다.
    /// </summary>
    /// <param name="pixels">그리기 화면(행 간격 포함 8비트 버퍼).</param>
    /// <param name="pitch">한 행의 바이트 간격.</param>
    /// <param name="height">화면 세로 크기.</param>
    /// <param name="image">원본 해제 그림(팔레트 번호와 불투명 마스크).</param>
    /// <param name="frame">그림의 실제 프레임 영역(기준점 상대).</param>
    /// <param name="position">프레임 기준점의 화면 위치.</param>
    /// <param name="scaleX">가로 16.16 배율(음수는 축 반전).</param>
    /// <param name="scaleY">세로 16.16 배율(음수는 축 반전).</param>
    /// <param name="clip">출력을 제한하는 클리핑 영역(half-open 화면 좌표).</param>
    /// <param name="colors">소스 색 치환표 256바이트. 없으면 원본 번호를 쓴다.</param>
    public static void DrawScaledVfxImage(Span<byte> pixels, int pitch, int height,
        IndexedImage image, ShapeFrame frame, ScreenPoint position,
        int scaleX, int scaleY, ScreenRect clip, byte[]? colors = null)
    {
        ArgumentNullException.ThrowIfNull(image);
        ArgumentNullException.ThrowIfNull(frame);
        if (colors is not null && colors.Length != 256)
        {
            throw new ArgumentException("VFX 변환 색 치환표는 256바이트여야 합니다", nameof(colors));
        }
        if (pitch < 1 || height < 1 || (long)pitch * height > pixels.Length)
        {
            throw new ArgumentException("VFX 변환 표면 용량 오류", nameof(pixels));
        }
        if (image.Width < 1 || image.Height < 1 || image.Width > WordMaximum + 1 ||
            image.Height > WordMaximum + 1 || image.Width * image.Height != image.Indices.Length ||
            image.Opacity.Length != image.Indices.Length)
        {
            throw new ArgumentException("VFX 변환 이미지 용량 오류", nameof(image));
        }
        ShapeRect rect = frame.Rect;
        if (frame.IsSpecial() || (long)rect.Right - rect.Left + 1 != image.Width ||
            (long)rect.Bottom - rect.Top + 1 != image.Height)
        {
            throw new ArgumentException("VFX 변환 실제 프레임 영역 오류", nameof(frame));
        }
        // 원본의 조기 일반 출력 분기에서는 255도 불투명 색으로 유지한다
        if (scaleX == FixedUnit && scaleY == FixedUnit)
        {
            RendererDraw.DrawIndexedImage(pixels, pitch, height, image,
                Add(position.X, rect.Left), Add(position.Y, rect.Top), clip, colors);
            return;
        }
        if (!IsWord(rect.Left) || !IsWord(rect.Right) || !IsWord(rect.Top) || !IsWord(rect.Bottom))
        {
            throw new ArgumentOutOfRangeException(nameof(frame), "VFX 변환 프레임 WORD 좌표 범위 초과");
        }
        int firstX = Add(position.X, ScaleCoordinate(rect.Left, scaleX));
        int lastX = Add(position.X, ScaleCoordinate(rect.Right, scaleX));
        int firstY = Add(position.Y, ScaleCoordinate(rect.Top, scaleY));
        int lastY = Add(position.Y, ScaleCoordinate(rect.Bottom, scaleY));
        if (!IsWord(firstX) || !IsWord(lastX) || !IsWord(firstY) || !IsWord(lastY))
        {
            throw new ArgumentOutOfRangeException(nameof(position), "VFX 변환 화면 WORD 좌표 범위 초과");
        }
        // 최저/최고 y가 같으면 원본은 행을 그리지 않는다
        if (firstY == lastY)
        {
            return;
        }
        int left = Math.Min(firstX, lastX);
        int right = Math.Max(firstX, lastX);
        int top = Math.Min(firstY, lastY);
        int bottom = Math.Max(firstY, lastY);
        int spanX = right - left;
        int spanY = bottom - top;
        if (spanX > WordMaximum || spanY > WordMaximum)
        {
            throw new ArgumentOutOfRangeException(nameof(scaleX), "VFX 변환 고정소수점 간격 범위 초과");
        }
        ScreenRect visible = ScreenGeometry.ClipScreenRect(
            new ScreenRect(left, top, right + 1, bottom + 1),
            ScreenGeometry.ClipScreenRect(clip, new ScreenRect(0, 0, pitch, height)));
        if (visible.Left >= visible.Right || visible.Top >= visible.Bottom)
        {
            return;
        }
        int sourceX = image.Width - 1;
        int sourceY = image.Height - 1;
        int stepX = spanX != 0 ? TextureStep(lastX > firstX ? sourceX : -sourceX, spanX) : 0;
        int stepY = TextureStep(lastY > firstY ? sourceY : -sourceY, spanY);
        // 동일 x의 두 변 중 오른쪽 변을 먼저 선택하는 <= 비교와 최저 y의 마지막 꼭짓점을 유지한다
        int startX = spanX != 0 ? (lastX > firstX ? 0 : sourceX) : (lastY > firstY ? sourceX : 0);
        uint firstU = Advance(TextureStart(startX), stepX, visible.Left - left);
        uint v = Advance(TextureStart(lastY > firstY ? 0 : sourceY), stepY, visible.Top - top);
        // 마지막 y도 포함한다. 잘린 행은 원본 기울기를 누적한 위치에서 시작한다
        for (int y = visible.Top; y < visible.Bottom; y++)
        {
            long sourceRow = (long)(v >> 16) * image.Width;
            long targetRow = (long)y * pitch;
            uint u = firstU;
            // 마지막 x도 포함하고 투명/색 치환 뒤 255는 기존 대상 픽셀을 유지한다
            for (int x = visible.Left; x < visible.Right; x++)
            {
                long input = sourceRow + (u >> 16);
                if (image.Opacity[input] != 0)
                {
                    byte ink = colors is null ? image.Indices[input] : colors[image.Indices[input]];
                    if (ink != ScratchTransparent)
                    {
                        pixels[(int)targetRow + x] = ink;
                    }
                }
                u = Advance(u, stepX, 1);
            }
            v = Advance(v, stepY, 1);
        }
    }
}
