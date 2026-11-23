using System.Buffers.Binary;

// 원문의 직접 삼각함수 각 입력·공유 난수와 현재 후보/필드 재조회를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>실제 일반 Finder/HP/후속 미사일과 지역8BYTE의 선택 경계다.</summary>
public sealed class IceRicochetHooks
{
    /// <summary>현재 사각형과 flags0의 일반 탐색 시작이다.</summary>
    public Func<SquidSearchArea, uint, ushort>? Begin { get; set; }
    /// <summary>현재 Finder의 다음 후보다.</summary>
    public Func<ushort>? Next { get; set; }
    /// <summary>현재 원본 HP 보유 판정이다.</summary>
    public Func<ushort, bool>? HasHitPoints { get; set; }
    /// <summary>실제 생성·Pop·Host 수명을 잇는 후속 요청이다.</summary>
    public Action<GunProjectileRequest>? CreateProjectile { get; set; }
    /// <summary>변위0의 미작성 지역8BYTE이며 임의 정상 점으로 바꾸지 않는다.</summary>
    public Action<byte[]>? InitialPointBytes { get; set; }
    /// <summary>같은 Client의 현재 checking/진단이다.</summary>
    public BoardAngleDiagnostics Angles { get; set; } = new();
    /// <summary>함수 슬롯과 각 진단 선언을 복사하고 실제 owner는 공유한다.</summary>
    internal IceRicochetHooks Copy() { var current = (IceRicochetHooks)MemberwiseClone(); current.Angles = Angles.Clone(); return current; }
}

/// <summary>RawIceRicochet.cpp의 반사점·확률·후보 선택/후속 생성 본문이다.</summary>
public sealed class RawIceRicochet
{
    /// <summary>원문00502310의 float 보드 칸 편향이다.</summary>
    private const float SnapBias = .99999f;
    /// <summary>원문0050b534의 float 회전 역수다.</summary>
    private const float InverseTurn = 1f / 360f;
    /// <summary>후보의 두 비행 속성 마스크다.</summary>
    private const uint Airborne = 0x120000;
    /// <summary>실제 표시/후보가 공유하는 물리 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>원문 span처럼 빌리는 현재 TYPE다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 표시 TYPE의 반사 사거리다.</summary>
    private readonly IReadOnlyList<MissileTypeData> _missileTypes;
    /// <summary>SP/음향/공격과 같은32비트 난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>최초 값복사한 실제 함수와 각 진단이다.</summary>
    private readonly IceRicochetHooks _hooks;
    /// <summary>PATCH 전체 자료와 같은 공유 난수를 빌린다.</summary>
    public RawIceRicochet(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<MissileTypeData> missileTypes, GameRandom random, IceRicochetHooks hooks)
    {
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || missileTypes.Count != types.Count) throw new ArgumentException("아이스 반사 PATCH/자료 오류"); _pool = pool; _types = types; _missileTypes = missileTypes; _random = random; _hooks = hooks.Copy();
    }
    /// <summary>선택된 실제 경계만 요구한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("아이스 반사 선택된 실제 경계 미연결");
    /// <summary>원문이 허용하는 저번호/서버 후보의 실제 TYPE만 검사한다.</summary>
    private ReadOnlySpan<byte> Slot(ushort sid) { if (sid >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid)); ReadOnlySpan<byte> raw = _pool.Slot(sid); if (raw[10] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid)); return raw; }
    /// <summary>원문 비정렬 float를 읽는다.</summary>
    private static float F(ReadOnlySpan<byte> raw, int at) => BinaryPrimitives.ReadSingleLittleEndian(raw[at..]);
    /// <summary>현재 좌표쌍을 원문 값으로 읽는다.</summary>
    private static float[] ReadPoint(ReadOnlySpan<byte> raw, int at) => [F(raw, at), F(raw, at + 4)];
    /// <summary>정확히360은 유지하고 음수 보정에서 float 역수를 쓴다.</summary>
    private static float Normalize(float angle) { if (angle < 0) angle = (float)((double)angle + (1 - (int)((double)angle * InverseTurn)) * 360.0); return angle > 360 ? (float)((double)angle % 360.0) : angle; }
    /// <summary>원문 편향/절삭 후 float 저장과 재절삭으로 보드 칸을 구한다.</summary>
    private static int Cell(float value) => (int)(float)(int)((double)value + SnapBias);
    /// <summary>NaN도 거부하는 원문 정상 좌표 범위다.</summary>
    private static bool Valid(float[] point) => point[0] > 0 && point[0] < 256 && point[1] > 0 && point[1] < 256;
    /// <summary>변위0은 출력/난수를 보존하고 그 밖에는 직접 cos/sin 각으로 반사점을 쓴다.</summary>
    public bool Point(MissileFields fields, float[] output)
    {
        ReadOnlySpan<byte> raw = Slot(fields.Parent()); float[] current = ReadPoint(raw, 14), origin = ReadPoint(fields.Bytes, 56); if (current[0] == origin[0] && current[1] == origin[1]) return false; float direction = RawArcherAnimation.DirectionAngle(origin, current, _pool.Edition, _hooks.Angles); float sign = _random.Next(100) > 49 ? -1f : 1f; double offset = _random.Next(65536) * (1.0 / 65536.0) * 120.0 + 30.0; float angle = Normalize((float)(offset * sign + direction)), range = _missileTypes[raw[10]].Range; output[0] = Math.Clamp((float)(Math.Cos(angle) * range + current[0]), 1, 255); output[1] = Math.Clamp((float)(Math.Sin(angle) * range + current[1]), 1, 255); return true;
    }
    /// <summary>확률·지역점·현재 후보 판정 뒤 target0도 허용하는 후속 미사일을 요청한다.</summary>
    public void Generate(MissileFields fields)
    {
        if (_random.Next(99) <= 19) return; ReadOnlySpan<byte> display = Slot(fields.Parent()); float[] point = [0, 0]; if (!Point(fields, point)) { byte[] bytes = new byte[8]; Need(_hooks.InitialPointBytes)(bytes); point = ReadPoint(bytes, 0); } if (!Valid(point)) return; Func<SquidSearchArea, uint, ushort> begin = Need(_hooks.Begin); Func<ushort> next = Need(_hooks.Next); Func<ushort, bool> hasHitPoints = Need(_hooks.HasHitPoints); int x = Cell(point[0]), y = Cell(point[1]); float bestDistance = 10000; ushort best = 0;
        // flags2는 HP효과 전에 캡처하고 나머지 raw/표적은 복귀 뒤 현재값을 읽으며 동률은 처음을 유지한다.
        for (ushort sid = begin(new(x - 1, y - 1, x + 1, y + 1), 0); sid != 0; sid = next())
        {
            ReadOnlySpan<byte> raw = Slot(sid); uint flags2 = _types[raw[10]].Flags2; if (!hasHitPoints(sid) || (sid >= 5 && sid <= 14999) || (raw[40] & 1) != 0 || (flags2 & Airborne) != 0 || sid == fields.Target()) continue; float[] candidate = ReadPoint(raw, 14); double dx = (double)candidate[0] - point[0], dy = (double)candidate[1] - point[1], distance = Math.Sqrt(dx * dx + dy * dy); if (distance < bestDistance) { best = sid; bestDistance = (float)distance; }
        }
        Need(_hooks.CreateProjectile)(new() { Firing = fields.Parent(), Target = best, Speed = F(fields.Bytes, 48), DisplayType = display[10], EffectType = 0, TargetPosition = point, TargetCenter = [-10000, -10000], Ricochet = 1, FiringPosition = ReadPoint(display, 14) });
    }
    /// <summary>같은 실제 일반 Finder/HP를 연결하고 후속 생성·지역점은 그대로 요구한다.</summary>
    public static IceRicochetHooks MakeWorldHooks(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, RawSquidReward reward, IceRicochetHooks hooks)
    {
        if (!ReferenceEquals(pool, reward.Pool)) throw new ArgumentException("아이스 반사 Finder/HP Pool 불일치"); var finder = new RawSquidFinder(pool, hash, types); IceRicochetHooks current = hooks.Copy(); current.Begin = (area, flags) => checked((ushort)finder.Begin(area, flags)); current.Next = () => checked((ushort)finder.Next()); current.HasHitPoints = sid => reward.HasHitPoints(sid); return current;
    }
}
