namespace Netstorm.Client.Display;

/// <summary>
/// 원본 VFX 일반 선의 단색 8비트 경로. 클립·끝점·32비트 분수 위상을 복원한다. 색 표·색상 모드는 포함하지 않는다.
/// 근거: cpppj/src/client/VFXLine.cpp 의 DrawVfxLine (원본 004011f1, mode 0). (R2b, 2026-10-28)
/// </summary>
public static class VfxLine
{
    /// <summary>클립 분류 비트. 실제 우선 순서는 왼쪽→오른쪽→위→아래다.</summary>
    private const uint Left = 8, Right = 4, Top = 2, Bottom = 1;

    /// <summary>반 픽셀. 0.32 분수의 bit 31이다.</summary>
    private const uint Half = 0x80000000u;

    /// <summary>정상 네 경계 교차 밖에서 수렴하지 않는 호스트 입력을 제한하는 반복 상한.</summary>
    private const uint ClipIterations = 32;

    /// <summary>제한한 단색 선의 두 끝점·원래 기준점·기울기. 픽셀 위상은 별도 순회에서 계산한다.</summary>
    private sealed class LineClip
    {
        /// <summary>현재 첫 끝점.</summary>
        public ScreenPoint First;

        /// <summary>현재 마지막 끝점.</summary>
        public ScreenPoint Last;

        /// <summary>원래 시작점 기준.</summary>
        public ScreenPoint Origin;

        /// <summary>화면 좌표 사각형. Right·Bottom은 포함하지 않는다.</summary>
        public ScreenRect Bounds;

        /// <summary>기울기(0.32 분수).</summary>
        public uint Slope;

        /// <summary>주축이 X인지.</summary>
        public bool MajorX;

        /// <summary>기울기가 음인지.</summary>
        public bool NegativeSlope;
    }

    /// <summary>
    /// 실제 표면에만 쓰며 양 끝 포함 클립으로 단색 선을 그린다. 색은 BYTE로 쓴다.
    /// </summary>
    /// <param name="pixels">8비트 화면.</param>
    /// <param name="pitch">행 간격.</param>
    /// <param name="height">화면 높이.</param>
    /// <param name="clip">클립(half-open).</param>
    /// <param name="first">선 시작점(양 끝 포함).</param>
    /// <param name="last">선 끝점(양 끝 포함).</param>
    /// <param name="color">색. 하위 바이트만 쓴다.</param>
    public static void DrawVfxLine(Span<byte> pixels, int pitch, int height, ScreenRect clip,
        ScreenPoint first, ScreenPoint last, uint color)
    {
        if (pitch < 1 || height < 1 || (long)pitch * height > pixels.Length)
        {
            throw new ArgumentException("VFX 선 표면 용량이 작습니다", nameof(pixels));
        }
        clip = ScreenGeometry.ClipScreenRect(clip, new ScreenRect(0, 0, pitch, height));
        if (clip.Left >= clip.Right || clip.Top >= clip.Bottom)
        {
            return;
        }
        var bounds = new ScreenRect(clip.Left, clip.Top, clip.Right - 1, clip.Bottom - 1);
        long dx = (long)last.X - first.X;
        long dy = (long)last.Y - first.Y;
        ulong ax = (ulong)(dx < 0 ? -dx : dx);
        ulong ay = (ulong)(dy < 0 ? -dy : dy);
        byte ink = (byte)color;
        if (dx == 0 || dy == 0)
        {
            int left = Math.Max(clip.Left, Math.Min(first.X, last.X));
            int right = Math.Min(clip.Right, Math.Max(first.X, last.X) + 1);
            int top = Math.Max(clip.Top, Math.Min(first.Y, last.Y));
            int bottom = Math.Min(clip.Bottom, Math.Max(first.Y, last.Y) + 1);
            if (left >= right || top >= bottom)
            {
                return;
            }
            // 직교 선은 양 끝 포함으로 채운다. 한 점도 같은 경로다
            for (int y = top; y < bottom; y++)
            {
                pixels.Slice(y * pitch + left, right - left).Fill(ink);
            }
            return;
        }
        if (ax > int.MaxValue || ay > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(last), "VFX 선 signed 거리 한도를 넘었습니다");
        }
        uint slope = ax == ay ? 0xffffffffu : (uint)(((ulong)Math.Min(ax, ay) << 32) / Math.Max(ax, ay));
        var line = new LineClip
        {
            First = first,
            Last = last,
            Origin = first,
            Bounds = bounds,
            Slope = slope,
            MajorX = ax >= ay,
            NegativeSlope = (dx < 0) != (dy < 0),
        };
        if (!Clip(line))
        {
            return;
        }
        int startMajor = line.MajorX ? line.First.X : line.First.Y;
        int endMajor = line.MajorX ? line.Last.X : line.Last.Y;
        int originMajor = line.MajorX ? first.X : first.Y;
        long distance = (long)startMajor - originMajor;
        uint phase = unchecked((uint)((ulong)(distance < 0 ? -distance : distance) * slope) + Half);
        ScreenPoint point = line.First;
        int sx = dx < 0 ? -1 : 1;
        int sy = dy < 0 ? -1 : 1;
        long span = (long)endMajor - startMajor;
        ulong count = (ulong)(span < 0 ? -span : span) + 1;
        // 원래 방향과 분수 위상을 유지하며 잘린 시작·끝 사이의 실제 주축 픽셀만 방문한다
        for (ulong index = 0; index < count; index++)
        {
            if (Outcode(point, bounds) == 0)
            {
                pixels[point.Y * pitch + point.X] = ink;
            }
            ulong next = (ulong)phase + slope;
            bool carry = next > 0xffffffffUL;
            phase = (uint)next;
            if (line.MajorX)
            {
                point = new ScreenPoint(Add(point.X, unchecked((uint)sx)), point.Y);
                if (ax == ay || carry)
                {
                    point = new ScreenPoint(point.X, Add(point.Y, unchecked((uint)sy)));
                }
            }
            else
            {
                point = new ScreenPoint(point.X, Add(point.Y, unchecked((uint)sy)));
                if (carry)
                {
                    point = new ScreenPoint(Add(point.X, unchecked((uint)sx)), point.Y);
                }
            }        }
    }

    /// <summary>반 평면 비교는 현재 양 끝 포함 경계를 사용한다. 겹치는 비트가 있으면 선 전체를 버릴 수 있다.</summary>
    private static uint Outcode(ScreenPoint point, ScreenRect bounds)
    {
        uint code = 0;
        if (point.X < bounds.Left)
        {
            code |= Left;
        }
        if (point.X > bounds.Right)
        {
            code |= Right;
        }
        if (point.Y < bounds.Top)
        {
            code |= Top;
        }
        if (point.Y > bounds.Bottom)
        {
            code |= Bottom;
        }
        return code;
    }

    /// <summary>실제 클립 곱의 high DWORD와 low DWORD bit 31을 더해 반 픽셀을 반올림한다.</summary>
    private static uint RoundedProduct(uint distance, uint slope)
    {
        ulong product = (ulong)distance * slope;
        return (uint)(product >> 32) + ((uint)product >= Half ? 1u : 0u);
    }

    /// <summary>원본 DIV는 32비트 몫만 받는다. 불가능한 DIV 오버플로는 거부한다.</summary>
    private static uint Inverse(uint distance, uint slope, bool start)
    {
        ulong numerator = ((ulong)(start ? distance - 1u : distance) << 32) | Half;
        ulong quotient = numerator / slope;
        ulong remainder = numerator % slope;
        if (quotient > uint.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(slope), "VFX 선 클립 DIV 범위를 넘었습니다");
        }
        return (uint)quotient + (start ? 1u : 0u) - (remainder == 0 ? 1u : 0u);
    }

    /// <summary>한 끝점을 원래 시작점 기준으로 교차한다. 현재 끝점에서 다시 기울기를 계산하지 않는다.</summary>
    private static void Cut(LineClip line, uint code, bool start)
    {
        bool xAxis = (code & (Left | Right)) != 0;
        int boundary = xAxis
            ? ((code & Left) != 0 ? line.Bounds.Left : line.Bounds.Right)
            : ((code & Top) != 0 ? line.Bounds.Top : line.Bounds.Bottom);
        int origin = xAxis ? line.Origin.X : line.Origin.Y;
        long delta = (long)boundary - origin;
        uint distance = (uint)(delta < 0 ? -delta : delta);
        uint offset = xAxis == line.MajorX ? RoundedProduct(distance, line.Slope) : Inverse(distance, line.Slope, start);
        bool negative = (delta < 0) != line.NegativeSlope;
        int other = Add(xAxis ? line.Origin.Y : line.Origin.X, negative ? unchecked(0u - offset) : offset);
        if (xAxis)
        {
            if (start)
            {
                line.First = new ScreenPoint(boundary, other);
            }
            else
            {
                line.Last = new ScreenPoint(boundary, other);
            }
        }
        else
        {
            if (start)
            {
                line.First = new ScreenPoint(other, boundary);
            }
            else
            {
                line.Last = new ScreenPoint(other, boundary);
            }
        }
    }

    /// <summary>두 끝점의 바깥 분류를 반복한다. 정상 좌표 밖에서 수렴하지 않는 입력만 거부한다.</summary>
    private static bool Clip(LineClip line)
    {
        // 정상 좌표의 교차·거부 순서를 유지하고 비정상 수렴 실패만 상한에서 거부한다
        for (uint count = 0; count < ClipIterations; count++)
        {
            uint first = Outcode(line.First, line.Bounds);
            uint last = Outcode(line.Last, line.Bounds);
            if ((first | last) == 0)
            {
                return true;
            }
            if ((first & last) != 0)
            {
                return false;
            }
            if (first != 0)
            {
                Cut(line, first, true);
            }
            else
            {
                Cut(line, last, false);
            }
        }
        throw new InvalidOperationException("VFX 선 클립이 수렴하지 않았습니다");
    }

    /// <summary>원본 DWORD 좌표 덧셈·부호 반전을 부호 오버플로 없이 수행한다.</summary>
    private static int Add(int a, uint b) => unchecked((int)(unchecked((uint)a) + b));
}
