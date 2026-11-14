using System.Buffers.Binary;

// 사제 나선 생성의 현재 지면·WORD·생성/소유자/Pop/Carrier 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>원문 사제 생성이 소비하는 현재 전역이다.</summary>
public sealed class PriestSpawnState
{
    /// <summary>WORD 쓰기 뒤 표면 알림을 선택하는 현재 다리TYPE다.</summary>
    public uint BridgeType { get; set; } = 82;
    /// <summary>PATCH의 모든 자리 실패 뒤 원문 진단을 선택하는 현재 checking이다.</summary>
    public RawCheckingFlag CheckingPlacement { get; set; } = new(true);
}

/// <summary>사제 생성의 실제 배치·생성·소유자·공간·알림·Carrier 효과다.</summary>
public sealed class PriestSpawnHooks
{
    /// <summary>점유된 후보의 전체 실제 배치 검사다.</summary>
    public Func<PriestPlacementQuery, bool>? MayPlace { get; set; }
    /// <summary>요청TYPE/flags0으로 실제 초기화된 SID를 만든다.</summary>
    public Func<uint, uint, int>? Create { get; set; }
    /// <summary>생성 뒤 원래 소유자WORD를 읽는 실제 가상 소유자 지정이다.</summary>
    public Action<int, uint>? SetOwner { get; set; }
    /// <summary>생성된 객체를 선택한 지도 좌표/flags0에 등록한다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>생성 뒤 현재 다리TYPE가 선택한 표면 알림이다.</summary>
    public Action<int>? NotifySurface { get; set; }
    /// <summary>Pop 뒤 현재TYPE가 선택한 실제 Carrier 지면 확인이다.</summary>
    public Action<int>? CheckCarrier { get; set; }
    /// <summary>원문 함수 객체처럼 복사하며 캡처한 장기 실제 장치는 빌린다.</summary>
    internal PriestSpawnHooks Copy() => new() { MayPlace = MayPlace, Create = Create, SetOwner = SetOwner, Pop = Pop, NotifySurface = NotifySurface, CheckCarrier = CheckCarrier };
}

/// <summary>0044b2e0의 스냅/나선/WORD·Carrier 생성 본체다.</summary>
public sealed class RawPriestSpawn
{
    /// <summary>원문 float 좌표 스냅 배율이다.</summary>
    private const float SnapBias = 0.99999f;
    /// <summary>최초 진행 방향과 회전 순서의 x 증분이다.</summary>
    private static readonly int[] Dx = [0, 1, 0, -1];
    /// <summary>최초 진행 방향과 회전 순서의 y 증분이다.</summary>
    private static readonly int[] Dy = [-1, 0, 1, 0];
    /// <summary>현재 물리 슬롯을 빌린다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>생성 뒤 현재TYPE의 genus를 읽는 실제 대여 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>각 후보 위치의 원문BYTE를 읽는 같은 지도다.</summary>
    private readonly byte[] _spots;
    /// <summary>효과 뒤 현재 다리/checking을 다시 읽는 장기 전역이다.</summary>
    private readonly PriestSpawnState _state;
    /// <summary>구성 때 확인/복사한 실제 효과다.</summary>
    private readonly PriestSpawnHooks _hooks;
    /// <summary>판본별 TYPE 길이·지도·필수 여섯 효과를 장치 실행 전에 확인한다.</summary>
    public RawPriestSpawn(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, byte[] spots, PriestSpawnState state, PriestSpawnHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(spots); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || spots.Length != 65536 || hooks.MayPlace is null || hooks.Create is null || hooks.SetOwner is null || hooks.Pop is null || hooks.NotifySurface is null || hooks.CheckCarrier is null)
            throw new ArgumentException("사제 생성 자료/하위 효과 누락");
        _pool = pool; _types = types; _spots = spots; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>같은 풀 여부를 검사할 실제 참조다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>double 합 뒤 signed32로 절삭한 값을 나선 이동용64비트로 보관한다.</summary>
    private static long Snap(float coordinate)
    {
        double value = (double)coordinate + SnapBias;
        if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(coordinate), "사제 생성 스냅 좌표 범위 오류");
        return (int)value;
    }
    /// <summary>현재TYPE의 Carrier genus를 Pop 효과 뒤 검사한다.</summary>
    private void CheckCarrier(int sid)
    {
        byte type = _pool.AllocatedBytes(sid)[10];
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "사제 Carrier 검사 타입 범위 오류");
        if ((_types[type].Flags2 & 0x30000) != 0) _hooks.CheckCarrier!(sid);
        else if (_pool.Edition == OriginalEdition.Patch1078) throw new ArgumentException("사제 Carrier 검사 genus 누락: 원본 assert");
    }
    /// <summary>최대132후보를 원문 순서로 방문하고 최초 허용 위치에만 실제 객체를 만든다.</summary>
    public void Spawn(float x, float y, uint type, ushort word)
    {
        if (type < OriginalConstructors.FirstAssetType || type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(type), "사제 생성 타입 범위 오류");
        long cellX = Snap(x), cellY = Snap(y); uint owner = (uint)(word & 0x7f); int direction = 0;
        // 두 번씩 같은 길이를 방문하면서 위/오른쪽/아래/왼쪽으로 나선을 넓힌다.
        for (int stage = 2; stage < 24; stage++)
        {
            // 현재 방향으로 stage/2개의 후보만 확인한다.
            for (int step = 0; step < stage / 2; step++)
            {
                if (cellX > 0 && cellY > 0 && cellX < 255 && cellY < 255)
                {
                    byte spot = _spots[(int)cellY * 256 + (int)cellX]; float candidateX = cellX, candidateY = cellY;
                    if (spot == 0 || (_hooks.MayPlace!(new PriestPlacementQuery(type, candidateX, candidateY, 0, owner, 0)) && (spot & 0x10) == 0))
                    {
                        int created = _hooks.Create!(type, 0); Span<byte> raw = _pool.AllocatedBytes(created); BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], word);
                        if (raw[10] == _state.BridgeType) _hooks.NotifySurface!(created);
                        _hooks.SetOwner!(created, owner); _hooks.Pop!(created, candidateX, candidateY, 0); CheckCarrier(created); return;
                    }
                }
                cellX += Dx[direction]; cellY += Dy[direction];
            }
            direction = (direction + 1) % Dx.Length;
        }
        if (_pool.Edition == OriginalEdition.Patch1078 && _state.CheckingPlacement.Read() != 0) throw new InvalidOperationException("사제 생성 자리 탐색 실패: 원본 debug assert");
    }
    /// <summary>같은 풀의 실제 사제 생성만 종속 해방에 연결하고 나머지 함수 사본은 보존한다.</summary>
    public static DamageableReleaseHooks MakePriestSpawnHooks(RawSidPool pool, RawPriestSpawn spawn, DamageableReleaseHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(spawn); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, spawn.Pool)) throw new ArgumentException("사제 생성/종속 해방의 SID 풀이 다릅니다");
        return new DamageableReleaseHooks { SpawnPriest = spawn.Spawn, Create = hooks.Create, SetOwner = hooks.SetOwner, Pop = hooks.Pop, NotifySurface = hooks.NotifySurface };
    }
}
