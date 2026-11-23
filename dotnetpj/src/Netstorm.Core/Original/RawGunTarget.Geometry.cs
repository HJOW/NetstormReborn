using System.Buffers.Binary;

// 현재 SHP 포구·발자국·보드 각도의 원문 실수 저장 순서를 유지한다.
namespace Netstorm.Core.Original;

/// <summary>표적 검색과 공유하는 실제 기하 몸체다.</summary>
public sealed partial class RawGunTarget
{
    /// <summary>원본 여덟 방향의 가로 이동 표다.</summary>
    private static readonly int[] Dx = [0, 1, 1, 1, 0, -1, -1, -1];
    /// <summary>원본 여덟 방향의 세로 이동 표다.</summary>
    private static readonly int[] Dy = [-1, -1, 0, 1, 1, 1, 0, -1];
    /// <summary>썬 캐논의 짝수 방향별 허용 그림 프레임이다.</summary>
    private static readonly int[] SunAllowedFrame = [1, 3, 0, 2];
    /// <summary>폭이 얇은 사정선의 최소 경계 확장량이다.</summary>
    private const float EdgeLow = .0001f;
    /// <summary>폭이 얇은 사정선의 최대 경계 확장량이다.</summary>
    private const float EdgeHigh = .1f;
    /// <summary>원본 ceil의 x87 가산에 쓰는 float 상수다.</summary>
    private const float CeilBias = .9999f;
    /// <summary>현재 raw 좌표의 float32를 읽는다.</summary>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadSingleLittleEndian(raw[offset..]);
    /// <summary>같은 절댓값이면 세로를 먼저 선택하는 보드 네 방향이다.</summary>
    private static int Direction(float x, float y, float tx, float ty)
    {
        float dx = (float)((double)tx - x), dy = (float)((double)ty - y);
        return Math.Abs(dx) <= Math.Abs(dy) ? dy > 0 ? 4 : 0 : dx <= 0 ? 6 : 2;
    }
    /// <summary>완전 포함과 모든 접촉 경계를 포함하는 원문 OR 교차식이다.</summary>
    private static bool Intersects(GunTargetRect a, GunTargetRect b) =>
        (a.Left <= b.Left || a.Left <= b.Right) && (b.Left <= a.Right || b.Right <= a.Right) &&
        (a.Top <= b.Top || a.Top <= b.Bottom) && (b.Top <= a.Bottom || b.Bottom <= a.Bottom);
    /// <summary>보드의 현재 유효 점 범위다.</summary>
    private static bool ValidPoint(float x, float y) => x > 0 && y > 0 && x < 256 && y < 256;
    /// <summary>별도 사각형의 incoming corner만 보드 안으로 제한한다.</summary>
    private static void Clamp(ref float x, ref float y) { x = Math.Clamp(x, 1f, 255f); y = Math.Clamp(y, 1f, 255f); }
    /// <summary>정상 CRT __ftol의 0방향 절삭 범위를 유지한다.</summary>
    private static int Truncate(float value)
    {
        if (!float.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value));
        return (int)value;
    }
    /// <summary>중간 float 저장 없이 원문 float bias를 더한 뒤 절삭한다.</summary>
    private static int Ceiling(float value)
    {
        double biased = (double)value + CeilBias;
        if (!double.IsFinite(biased) || biased < -2147483648.0 || biased >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value));
        return (int)biased;
    }
    /// <summary>360을 그대로 허용하는 원문 각도 정규화다.</summary>
    private static float Normalize(float angle)
    {
        if (!float.IsFinite(angle)) throw new ArgumentException("Gun 비유한 각도", nameof(angle));
        if (angle < 0) angle = (float)((double)unchecked(1 - Truncate(angle / 360f)) * 360.0 + angle);
        if (angle > 360) angle = (float)((double)angle % 360.0);
        return angle;
    }
    /// <summary>끝점을 포함하며 긴 구간이면 바깥쪽을 선택하는 원문 원형 구간이다.</summary>
    private static bool ContainsAngle(float value, float first, float last)
    {
        if (first > last) (first, last) = (last, first);
        return last - first <= 180f ? first <= value && value <= last : value <= first || last <= value;
    }
    /// <summary>두 특수 타입을 먼저 처리한 뒤 현재 방향 프레임을 읽는다.</summary>
    public int DirectionFrame(uint type, int direction)
    {
        int index = direction / 2;
        if (index < 0 || index > 3) throw new ArgumentOutOfRangeException(nameof(direction));
        if (type == _state.SunCannonType) return SunAllowedFrame[index];
        if (type == _state.SunFenceType) return index;
        int frame = _targetTypes[(int)type].DirectionFrames[index];
        return frame == -1 ? 0 : frame;
    }
    /// <summary>홀수 방향은 자연 거부하고 Rain/Thunder는 현재 지정 방향만 허용한다.</summary>
    public bool CannonDirection(GunTargetQuery query, int direction)
    {
        if (direction % 2 != 0) return false;
        if (_types[(int)query.Type].Group != 1) throw new InvalidOperationException("Gun Cannon group 원본 assert");
        if (direction < 0 || direction > 7) throw new ArgumentOutOfRangeException(nameof(direction));
        int theme = _targetTypes[(int)query.Type].Theme;
        return (theme != 1 && theme != 2) || direction == query.Direction;
    }
    /// <summary>현재 raw 원점과 TYPE 발자국/선택 확장 높이로 실제 경계를 만든다.</summary>
    public GunTargetRect Footprint(int target, bool extended = true)
    {
        if (target <= 0 || target >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(target));
        ReadOnlySpan<byte> raw = _pool.Slot(target); int number = raw[10];
        if (number >= _types.Count) throw new ArgumentOutOfRangeException(nameof(target));
        OriginalTypeRecord type = _types[number]; float x = Coordinate(raw, 14), y = Coordinate(raw, 18);
        float lowX = (float)((double)x - (type.FootX - 1)), lowY = (float)((double)y - (type.FootY + (extended ? _targetTypes[number].HeightExtension : 0) - 1));
        Clamp(ref lowX, ref lowY); float highX = x, highY = y, left = x, top = y;
        if (ValidPoint(left, top)) { left = Math.Min(left, lowX); top = Math.Min(top, lowY); } else { left = lowX; top = lowY; }
        if (ValidPoint(highX, highY)) { highX = Math.Max(highX, lowX); highY = Math.Max(highY, lowY); } else { highX = lowX; highY = lowY; }
        return new(left, top, highX, highY);
    }
    /// <summary>확장 높이 없이 현재 발자국의 반폭/반높이를 빼는 실제 중심이다.</summary>
    public (float X, float Y) Center(int target)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(target); OriginalTypeRecord type = _types[raw[10]];
        return ((float)((double)Coordinate(raw, 14) - type.FootX * .5), (float)((double)Coordinate(raw, 18) - type.FootY * .5));
    }
    /// <summary>현재 SHP 포구/핫스폿과 사거리로 판본별 원문 사정선을 만든다.</summary>
    public GunTargetRect CannonRay(GunTargetQuery query, int direction)
    {
        Validate(query);
        if (!CannonDirection(query, direction)) return new(-10000, -10000, -10000, -10000);
        if (!float.IsFinite(_state.ScaleX) || !float.IsFinite(_state.ScaleY) || _state.ScaleX == 0 || _state.ScaleY == 0) throw new ArgumentException("Gun Cannon 픽셀 배율 오류");
        int frame = DirectionFrame(query.Type, direction); SquidDisplayShape shape = _shapes[(int)query.Type]; OriginalTypeRecord type = _types[(int)query.Type];
        if (!shape.Loaded || frame < 0 || frame >= shape.Frames.Count) throw new ArgumentOutOfRangeException(nameof(frame));
        if (_pool.Edition == OriginalEdition.Patch1078 && (frame >= shape.FrameCount * ((type.Flags1 & 0x440000) != 0 ? 2 : 1) || type.MaxHitPoints == -0x22222223 || type.MaxHitPoints == -0x32323233)) throw new ArgumentOutOfRangeException(nameof(frame), "Gun Cannon frameCheck 원본 assert");
        var header = shape.Frames[frame];
        float x = (float)((double)header.MuzzleX / _state.ScaleX + query.X), y = (float)((double)header.MuzzleY / _state.ScaleY + query.Y);
        x = (float)((double)-type.HotspotX / _state.ScaleX + x); y = (float)((double)-type.HotspotY / _state.ScaleY + y);
        float endX = (float)((double)Dx[direction] * _targetTypes[(int)query.Type].Range + x), endY = (float)((double)Dy[direction] * _targetTypes[(int)query.Type].Range + y);
        float left = x, top = y, right = x, bottom = y; Clamp(ref endX, ref endY);
        if (ValidPoint(left, top)) { left = Math.Min(left, endX); top = Math.Min(top, endY); } else { left = endX; top = endY; }
        if (ValidPoint(right, bottom)) { right = Math.Max(right, endX); bottom = Math.Max(bottom, endY); } else { right = endX; bottom = endY; }
        if ((double)right - left < 2.0 || (double)bottom - top < 2.0)
        {
            left = (float)((double)Truncate(left) + EdgeLow); top = (float)((double)Truncate(top) + EdgeLow);
            right = (float)((double)Ceiling(right) + EdgeHigh); bottom = (float)((double)Ceiling(bottom) + EdgeHigh);
            Clamp(ref left, ref top); Clamp(ref right, ref bottom);
        }
        var rect = new GunTargetRect(left, top, right, bottom);
        if (_state.ShowRanges && (_state.ReadPlayerFlags(query.Owner) & 0x20) == 0)
            (_hooks.DrawRange ?? throw new InvalidOperationException("Gun Cannon 진단 출력 경계 누락"))(rect, false);
        return rect;
    }
    /// <summary>발사자에서 보이는 반대쪽 경계 중점을 원문 저장 순서로 반환한다.</summary>
    private static (float X, float Y) HitPoint(GunTargetQuery query, GunTargetRect rect)
    {
        float midX = (float)(((double)rect.Left + rect.Right) * .5), midY = (float)(((double)rect.Top + rect.Bottom) * .5);
        int direction = (Direction(query.CenterX, query.CenterY, midX, midY) + 4) & 7;
        if (direction == 0 || direction == 4) return ((float)(((double)rect.Right - rect.Left) * .5 + rect.Left), direction == 0 ? rect.Top : rect.Bottom);
        return (direction == 2 ? rect.Right : rect.Left, (float)(((double)rect.Bottom - rect.Top) * .5 + rect.Top));
    }
}
