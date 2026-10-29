using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 현재 VFX 클립에 실제 버튼/그림/비트맵/채우기 픽셀을 쓰는 영역이다.

/// <summary>원본 Gump 그림/선 출력을 같은8비트 표면의 현재 VFX 클립에 연결한다.
/// 근거: cpppj/src/client/GumpDraw.h·GumpDraw.cpp 의 GumpPixelTarget.
/// C++ span의 빌린 표면은 Memory로 보유해 부분 버퍼와 guard도 복사 없이 사용할 수 있다.</summary>
public sealed class GumpPixelTarget
{
    /// <summary>행 간격을 포함한 빌린 실제 출력 영역.</summary>
    private readonly Memory<byte> _pixels;
    /// <summary>한 행의 바이트 간격.</summary>
    private readonly int _pitch;
    /// <summary>실제 행 수.</summary>
    private readonly int _height;
    /// <summary>매 요청에서 다시 읽는 공유 클립.</summary>
    private readonly GumpDrawState _state;

    /// <summary>표면 용량을64비트로 검증하고 현재 상태를 빌린다. pitch는 실제 행 간격이다.</summary>
    public GumpPixelTarget(Memory<byte> pixels, int pitch, int height, GumpDrawState state)
    {
        ArgumentNullException.ThrowIfNull(state);
        if (pitch < 1 || height < 1 || (long)pitch * height > pixels.Length) throw new ArgumentException("Gump 픽셀 표면 용량 오류", nameof(pixels));
        _pixels = pixels; _pitch = pitch; _height = height; _state = state;
    }

    /// <summary>현재 inclusive VFX 끝점+1을64비트로 계산해 실제 표면 안의 half-open 영역으로 바꾼다.</summary>
    private ScreenRect Clip()
    {
        ScreenRect clip = _state.VfxClip;
        return new((int)Math.Clamp((long)clip.Left, 0, _pitch), (int)Math.Clamp((long)clip.Top, 0, _height),
            (int)Math.Clamp((long)clip.Right + 1, 0, _pitch), (int)Math.Clamp((long)clip.Bottom + 1, 0, _height));
    }

    /// <summary>양 끝 포함 직교/대각선 선을 입력 순서대로 출력한다. 각 선에서 현재 클립을 읽고 대각선은 실제 VFX 분수/클립을 사용한다.</summary>
    public void Lines(ReadOnlySpan<GumpLine> lines)
    {
        // 원래 선 호출 순서를 유지해 뒤 선이 모서리를 덮게 한다.
        foreach (GumpLine line in lines)
        {
            if (line.First.X != line.Last.X && line.First.Y != line.Last.Y)
            {
                VfxLine.DrawVfxLine(_pixels.Span, _pitch, _height, Clip(), line.First, line.Last, line.Color); continue;
            }
            ScreenRect clip = Clip();
            long left = Math.Max(clip.Left, Math.Min(line.First.X, line.Last.X)); long top = Math.Max(clip.Top, Math.Min(line.First.Y, line.Last.Y));
            long right = Math.Min(clip.Right, (long)Math.Max(line.First.X, line.Last.X) + 1); long bottom = Math.Min(clip.Bottom, (long)Math.Max(line.First.Y, line.Last.Y) + 1);
            if (left >= right || top >= bottom) continue;
            // 모든 주소를 실제 표면 안으로 제한한 유효 행만 채운다.
            for (long y = top; y < bottom; y++) _pixels.Span.Slice((int)(y * _pitch + left), (int)(right - left)).Fill(unchecked((byte)line.Color));
        }
    }

    /// <summary>실제 투명 마스크/선택적 소스 색 치환으로 해제 그림을 현재 클립에 합성한다. position은 왼쪽 위다.</summary>
    public void Image(IndexedImage image, ScreenPoint position, byte[]? colors = null) =>
        RendererDraw.DrawIndexedImage(_pixels.Span, _pitch, _height, image, position.X, position.Y, Clip(), colors);

    /// <summary>공유 클립을 바꾸지 않고 전체 Gump 경계와 현재 표면의 교차 영역에서 선을 그린다.</summary>
    public void LinesClipped(ReadOnlySpan<GumpLine> lines, ScreenRect bounds)
    {
        var clipped = new GumpDrawState { Clip = _state.Clip, VfxClip = _state.VfxClip };
        GumpDraw.SetGumpDrawClip(clipped, ScreenGeometry.ClipScreenRect(bounds, Clip()));
        new GumpPixelTarget(_pixels, _pitch, _height, clipped).Lines(lines);
    }

    /// <summary>소스를 행 경계 없는 평면 주소로 읽는다. 가려진 guard는 읽지 않고 보이는 할당 밖 바이트만 outside 경계에서 요구한다.</summary>
    public void RawBitmap(ReadOnlySpan<byte> source, int stride, long sourceBase, ScreenPoint size, ScreenPoint destination, ScreenRect bounds, Func<long, byte>? outside = null)
    {
        if (stride <= 0) throw new ArgumentException("원시 bitmap의 양수 행 간격 필요", nameof(stride));
        if (size.X <= 0 || size.Y <= 0) return;
        ScreenRect clip = ScreenGeometry.ClipScreenRect(bounds, Clip());
        long left = Math.Max(clip.Left, destination.X); long top = Math.Max(clip.Top, destination.Y);
        long right = Math.Min(clip.Right, (long)destination.X + size.X); long bottom = Math.Min(clip.Bottom, (long)destination.Y + size.Y);
        // 보이는 행만 원본 stride로 읽고 남은 여백은 그대로 둔다.
        for (long y = top; y < bottom; y++)
        {
            // 음수 sourceBase도 최종 평면 주소가 할당 안이면 그대로 읽는다.
            for (long x = left; x < right; x++)
            {
                long delta = (y - destination.Y) * stride + (x - destination.X);
                if (sourceBase > long.MaxValue - delta) throw new OverflowException("원시 bitmap 소스 주소 넘침");
                long offset = sourceBase + delta; byte value;
                if (offset >= 0 && offset < source.Length) value = source[(int)offset];
                else
                {
                    if (outside is null) throw new ArgumentOutOfRangeException(nameof(source), $"원시 bitmap의 보이는 할당 밖 BYTE 공급 필요: 주소={offset}, 할당={source.Length}, 시작={sourceBase}, 크기={size.X}x{size.Y}, 목적={destination.X},{destination.Y}");
                    value = outside(offset);
                }
                _pixels.Span[(int)(y * _pitch + x)] = value;
            }
        }
    }

    /// <summary>원본 signed16.16 직교 배율/축 반전의 끝점/표본을 실제 VFX 변환기로 같은 현재 표면에 그린다.</summary>
    public void ScaledImage(IndexedImage image, ShapeFrame frame, ScreenPoint position, int scaleX, int scaleY, byte[]? colors = null) =>
        VfxTransform.DrawScaledVfxImage(_pixels.Span, _pitch, _height, image, frame, position, scaleX, scaleY, Clip(), colors);

    /// <summary>현재 클립 안의 half-open 사각형을 채운다. checker는 절대 화면 x/y 홀짝이 다른 픽셀만 쓴다.</summary>
    public void Fill(ScreenRect rect, uint color, bool checker = false)
    {
        ScreenRect visible = ScreenGeometry.ClipScreenRect(rect, Clip()); byte ink = unchecked((byte)color);
        if (visible.Left >= visible.Right || visible.Top >= visible.Bottom) return;
        // 실제 표면의 유효 행만 갱신한다.
        for (int y = visible.Top; y < visible.Bottom; y++)
        {
            int row = y * _pitch;
            if (!checker) _pixels.Span.Slice(row + visible.Left, visible.Right - visible.Left).Fill(ink);
            else
            {
                // 잘린 영역에서도 원본 화면 절대 좌표의 체크 위상을 유지한다.
                for (int x = visible.Left; x < visible.Right; x++) if (((uint)x ^ (uint)y) % 2 != 0) _pixels.Span[row + x] = ink;
            }
        }
    }
}
