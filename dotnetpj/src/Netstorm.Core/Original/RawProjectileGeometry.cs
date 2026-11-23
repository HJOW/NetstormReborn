using System.Buffers.Binary;

// 원문 x87 중간 double/float 저장과 실제 SHP 물리 프레임을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>실제 두 판본의 포구/치수/상자와 PATCH 사정선·방향 본문이다.</summary>
public sealed class RawProjectileGeometry
{
    /// <summary>원문 방향 helper의 근사 원주 double 비트다.</summary>
    private static readonly double TwoPi = BitConverter.Int64BitsToDouble(0x401921f9f01b866e);
    /// <summary>원문 방향 helper의 근사 직각 double 비트다.</summary>
    private static readonly double HalfPi = BitConverter.Int64BitsToDouble(0x3ff921f9f01b866e);
    /// <summary>원문 방향 helper의 역원주 double 비트다.</summary>
    private static readonly double InverseTwoPi = BitConverter.Int64BitsToDouble(0x3fc45f318e7adaf5);
    /// <summary>썬 캐논0..3/24..27의 실제 cardinal 방향이다.</summary>
    private static readonly int[] SunDirections = [4, 0, 6, 2];
    /// <summary>현재 자산과 검색 모듈이 공유하는 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>원문 span처럼 빌리는 현재 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원문 span처럼 빌리는 현재 SHP/방향 자료다.</summary>
    private readonly IReadOnlyList<ProjectileGeometryType> _geometry;
    /// <summary>생성 시 원문 값으로 복사한 가로 배율이다.</summary>
    private readonly float _scaleX;
    /// <summary>생성 시 원문 값으로 복사한 세로 배율이다.</summary>
    private readonly float _scaleY;
    /// <summary>같은 풀의 현재 프레임 방향 getter다.</summary>
    private readonly RawGunMode? _gunMode;
    /// <summary>전체 표/배율과 방향 getter의 같은 풀을 확인한다.</summary>
    public RawProjectileGeometry(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<ProjectileGeometryType> geometry, float scaleX = 16, float scaleY = 11, RawGunMode? gunMode = null)
    {
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || geometry.Count != types.Count || !float.IsFinite(scaleX) || !float.IsFinite(scaleY) || scaleX <= 0 || scaleY <= 0) throw new ArgumentException("투사체 기하 판본/배율 오류"); if (gunMode is not null && !ReferenceEquals(gunMode.Pool, pool)) throw new ArgumentException("투사체 기하/방향 Pool 불일치"); Pool = pool; _types = types; _geometry = geometry; _scaleX = scaleX; _scaleY = scaleY; _gunMode = gunMode;
    }
    /// <summary>물리 SID/TYPE만 검사하며 free/dead는 호출자에 맡긴다.</summary>
    private ReadOnlySpan<byte> Slot(ushort sid) { if (sid < 5 || sid >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid)); ReadOnlySpan<byte> raw = Pool.Slot(sid); if (raw[10] < 70 || raw[10] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid)); return raw; }
    /// <summary>비정렬 little endian DWORD를 그대로 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int at) => BinaryPrimitives.ReadUInt32LittleEndian(raw[at..]);
    /// <summary>두 원문 모서리의 float 값만1..255에 제한한다.</summary>
    private static float[] Clamp(float[] point) => [Math.Clamp(point[0], 1, 255), Math.Clamp(point[1], 1, 255)];
    /// <summary>현재 SHP의 실제 물리 프레임에서 두 signed WORD를 값으로 읽는다.</summary>
    private (short X, short Y) Offset(int type, int frame, bool abstractFrame) { if (frame < 0 || frame >= _geometry[type].ShotOffsets.Length) throw new ArgumentOutOfRangeException(nameof(frame)); _ = abstractFrame; return _geometry[type].ShotOffsets[frame]; }
    /// <summary>기본+현재 포구 픽셀을 정수로 더한 뒤 각각 double 나눗셈과 float 저장을 한다.</summary>
    public float[] ArcherMuzzle(ushort firing)
    {
        ReadOnlySpan<byte> raw = Slot(firing); bool patch = Pool.Edition == OriginalEdition.Patch1078; if (raw[10] != 71) throw new InvalidOperationException("썬 아처 외 포구 파생 미복원"); bool abstractFrame = (raw[patch ? 40 : 35] & 1) != 0; var first = Offset(raw[10], _geometry[raw[10]].DefaultFrame, abstractFrame); var current = Offset(raw[10], patch ? unchecked((int)Read(raw, 36)) : raw[34], abstractFrame); float x = BitConverter.UInt32BitsToSingle(Read(raw, 14)), y = BitConverter.UInt32BitsToSingle(Read(raw, 18)); return [(float)((double)(first.X + current.X) / _scaleX + x), (float)((double)(first.Y + current.Y) / _scaleY + y)];
    }
    /// <summary>71은 실제 두 SHP의 합이고 PATCH128/129/131/140은 raw 좌표를 반환한다.</summary>
    public float[] Muzzle(ushort firing)
    {
        ReadOnlySpan<byte> raw = Slot(firing); if (raw[10] == 71) return ArcherMuzzle(firing); if (Pool.Edition == OriginalEdition.Patch1078 && raw[10] is 128 or 129 or 131 or 140) return [BitConverter.UInt32BitsToSingle(Read(raw, 14)), BitConverter.UInt32BitsToSingle(Read(raw, 18))]; throw new InvalidOperationException("투사체 실제 가상 포구 미복원 TYPE" + raw[10]);
    }
    /// <summary>현재 포대 방향과 목표 축이 다르면 절삭 사정거리로 보정한 뒤 실제 축을 고정한다.</summary>
    public float[] CannonRayEnd(float[] from, float[] to, ushort firing)
    {
        ReadOnlySpan<byte> raw = Slot(firing); if (Pool.Edition != OriginalEdition.Patch1078 || raw[10] is not (128 or 129 or 131)) throw new InvalidOperationException("캐논 사정선 미복원 판본/TYPE"); if (!float.IsFinite(from[0]) || !float.IsFinite(from[1]) || !float.IsFinite(to[0]) || !float.IsFinite(to[1])) throw new ArgumentException("캐논 사정선 비유한 좌표");
        float dx = (float)((double)to[0] - from[0]), dy = (float)((double)to[1] - from[1]); int direction = Math.Abs(dx) <= Math.Abs(dy) ? dy > 0 ? 4 : 0 : dx <= 0 ? 6 : 2; int frame = unchecked((int)Read(raw, 36)), frameDirection;
        if (raw[10] is 129 or 131) frameDirection = unchecked((int)(_gunMode ?? throw new InvalidOperationException("아이스/썬더 사정선 실제 방향 getter 미연결")).Get(firing)); else if (frame >= 0 && frame < 4) frameDirection = SunDirections[frame]; else if (frame >= 24 && frame < 28) frameDirection = SunDirections[frame - 24]; else throw new InvalidOperationException("캐논 사정선 미복원 물리 프레임" + frame);
        if (frameDirection is not (0 or 2 or 4 or 6)) throw new InvalidOperationException("캐논 사정선 cardinal 원문 진단");
        if (frameDirection != direction)
        {
            float range = _geometry[raw[10]].Range; if (!float.IsFinite(range) || (double)range < -2147483648.0 || (double)range >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(firing)); int distance = (int)range, backwards = unchecked(-distance), offsetX = frameDirection == 2 ? distance : frameDirection == 6 ? backwards : 0, offsetY = frameDirection == 4 ? distance : frameDirection == 0 ? backwards : 0; to = Clamp([(float)((double)offsetX + from[0]), (float)((double)offsetY + from[1])]);
        }
        return frameDirection is 0 or 4 ? [from[0], to[1]] : [to[0], from[1]];
    }
    /// <summary>포구의 기본 프레임 합산과 구별되는 현재 SHP 헤더 한 쌍이다.</summary>
    public int[] Hotspot(ushort firing) { ReadOnlySpan<byte> raw = Slot(firing); bool patch = Pool.Edition == OriginalEdition.Patch1078; var offset = Offset(raw[10], patch ? unchecked((int)Read(raw, 36)) : raw[34], (raw[patch ? 40 : 35] & 1) != 0); return [offset.X, offset.Y]; }
    /// <summary>원문 atan2/근사 원주/float 저장/반올림 뒤 방향당 물리 프레임을 곱한다.</summary>
    public int DirectionFrame(float[] from, float[] to, uint type)
    {
        if (Pool.Edition != OriginalEdition.Patch1078 || type >= _geometry.Count) throw new ArgumentOutOfRangeException(nameof(type)); ProjectileGeometryType data = _geometry[(int)type]; if (data.Directions <= 0 || data.FrameCount / data.Directions <= 0) throw new InvalidOperationException("투사체 방향/물리 프레임 자료 누락"); if (!float.IsFinite(from[0]) || !float.IsFinite(from[1]) || !float.IsFinite(to[0]) || !float.IsFinite(to[1])) throw new ArgumentException("투사체 비유한 방향 좌표"); double angle = Math.Atan2(-((double)to[1] - from[1]), (double)to[0] - from[0]) - HalfPi; if (angle < 0) angle += TwoPi; float scaled = (float)((TwoPi - angle) * data.Directions * InverseTwoPi); int direction = (int)((double)scaled + .5); return direction % data.Directions * (data.FrameCount / data.Directions);
    }
    /// <summary>높이 확장을 더하지 않는 실제 현재 TYPE 발자국이다.</summary>
    public int[] Dimensions(ushort target) { OriginalTypeRecord type = _types[Slot(target)[10]]; return [type.FootX, type.FootY]; }
    /// <summary>실제 full 발자국/확장 높이와 invalid 점의 모서리 대체를 보존한다.</summary>
    public float[] TargetBounds(ushort target, float[] position)
    {
        int number = Slot(target)[10]; OriginalTypeRecord type = _types[number]; float[] low = Clamp([position[0] - type.FootX, position[1] - unchecked(type.FootY + _geometry[number].HeightExtension)]); bool valid = position[0] > 0 && position[1] > 0 && position[0] < 256 && position[1] < 256; return valid ? [Math.Min(position[0], low[0]), Math.Min(position[1], low[1]), Math.Max(position[0], low[0]), Math.Max(position[1], low[1])] : [low[0], low[1], low[0], low[1]];
    }
}
