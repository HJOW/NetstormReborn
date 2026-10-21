namespace Netstorm.Core.Original;

/// <summary>
/// 피해가능객체 해제 처리 중 조회·갱신하는 상태 묶음(원본 DamageableReleaseState).
/// </summary>
public sealed class DamageableReleaseState
{
    /// <summary>전투 중인지(원본 00594fbc). 전투가 아니면 허용 genus 검사를 건너뛴다.</summary>
    public bool Battle { get; set; }

    /// <summary>허용 genus 마스크(원본 00542640). 초기값은 0x40480002 다.</summary>
    public uint AllowedGenus { get; set; } = 0x40480002;

    /// <summary>다리 타입 번호(원본 005411a0). WORD 를 쓴 뒤 표면 알림을 받을 대상이다.</summary>
    public uint BridgeType { get; set; } = 82;
}

/// <summary>
/// 피해가능객체 해제가 바깥에 요구하는 콜백 묶음(원본 DamageableReleaseHooks).
/// </summary>
public sealed class DamageableReleaseHooks
{
    /// <summary>사제 생성 몸체(원본 0044b2e0). WORD 전체를 그대로 전달한다.</summary>
    public Action<float, float, uint, ushort>? SpawnPriest { get; set; }

    /// <summary>일반 자산 생성(원본 004af530). 초기화된 할당 슬롯 번호를 돌려준다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>가상 +0x74 소유자 지정(새 객체, 0).</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>가상 +0x90 Pop(새 객체, x, y, 0). 대상을 지정 위치의 공간에 등록한다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }

    /// <summary>실제 WORD 쓰기 뒤 다리 타입에만 부르는 표면 검사(원본 004ac220).</summary>
    public Action<int>? NotifySurface { get; set; }
}

/// <summary>
/// 피해가능객체 삭제 준비에서 권한 확인 뒤 수행하는 종속 해방 구간을 복원한다(원본 RawDamageableRelease, 0044b626).
/// 순서는 원본 그대로 "부모 상태 검사 → 중심 계산과 스냅 → 종속 순회 → 해방 가능 마스크 →
/// 허용 genus → 사제 생성 또는 일반 자산 생성·WORD 복사·소유자 지정·Pop"다.
/// 좌표 스냅은 0.99999, 공간 조회는 0.9999 편향을 쓰고 중간 float 반올림을 넣지 않는다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawDamageableRelease.cpp (W03 삭제·수명 여섯째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageableRelease
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 WORD 복사 대상의 위치(10.78).</summary>
    private const int WordOffset = 12;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치(10.78).</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 종속 kind WORD 의 위치(10.78).</summary>
    private const int KindOffset = 18;

    /// <summary>슬롯 안 종속 mask WORD 의 위치(10.78).</summary>
    private const int MaskOffset = 20;

    /// <summary>종속 후보의 해방 가능 비트(원본 0x20).</summary>
    private const byte Releasable = 0x20;

    /// <summary>타입 플래그2의 사제 비트(원본 0x200000).</summary>
    private const uint PriestBit = 0x200000;

    /// <summary>타입 플래그2의 부모 중심 보정 비트(원본 0x80400000).</summary>
    private const uint CenteredParent = 0x80400000;

    /// <summary>좌표 스냅 편향(원본 0.99999).</summary>
    private const float SnapBias = 0.99999f;

    /// <summary>공간 조회 편향(원본 0.9999).</summary>
    private const float SpotBias = 0.9999f;

    /// <summary>지도 한 변의 칸 수.</summary>
    private const int Side = 256;

    /// <summary>spot 지도 전체 칸 수.</summary>
    private const int Cells = Side * Side;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>칸별 점유·통행 비트를 읽는 spot 지도.</summary>
    private readonly byte[] _spots;

    /// <summary>권한·타입 전역을 읽는 상태.</summary>
    private readonly ContainedFinderState _finderState;

    /// <summary>전투·허용 genus·다리 타입 전역.</summary>
    private readonly DamageableReleaseState _state;

    /// <summary>외부 효과 콜백 묶음.</summary>
    private readonly DamageableReleaseHooks _hooks;

    /// <summary>
    /// 피해가능객체 해제를 만든다. 타입 표·spot 지도 크기와 다섯 하위 효과를 모두 확인한다.
    /// </summary>
    /// <param name="pool">해방 대상 슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="spots">칸별 점유 지도 65,536칸.</param>
    /// <param name="finderState">내용물 검색기의 권한·타입 전역.</param>
    /// <param name="state">전투·허용 genus·다리 타입 전역.</param>
    /// <param name="hooks">외부 효과 콜백 묶음.</param>
    public RawDamageableRelease(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, byte[] spots,
        ContainedFinderState finderState, DamageableReleaseState state, DamageableReleaseHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(finderState);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != OriginalConstructors.TypeCount || spots.Length != Cells)
        {
            throw new ArgumentException("Damageable 해방 자료 오류", nameof(types));
        }
        if (hooks.SpawnPriest is null || hooks.Create is null || hooks.SetOwner is null
            || hooks.Pop is null || hooks.NotifySurface is null)
        {
            throw new ArgumentException("Damageable 해방 하위 효과 누락", nameof(hooks));
        }
        _pool = pool;
        _types = types;
        _spots = spots;
        _finderState = finderState;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀. 연결 어댑터가 같은 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>발자국·genus 를 조회하며 타입 표 범위를 검사한다.</summary>
    /// <param name="number">타입 번호.</param>
    private OriginalTypeRecord Type(uint number)
    {
        if (number >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(number), number, "Damageable 해방 타입 범위 오류");
        }
        return _types[(int)number];
    }

    /// <summary>현재 raw float 의 비트는 부모의 현재 슬롯에서 읽는다.</summary>
    /// <param name="parent">부모 슬롯 번호.</param>
    /// <returns>x·y 좌표.</returns>
    private float[] Position(int parent)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(parent);
        return
        [
            BitConverter.UInt32BitsToSingle(Read(raw, XOffset, 4)),
            BitConverter.UInt32BitsToSingle(Read(raw, YOffset, 4)),
        ];
    }

    /// <summary>발자국의 절반을 빼고 각 좌표를 float 에 저장해 중심을 만든다.</summary>
    /// <param name="parent">부모 슬롯 번호.</param>
    /// <returns>중심 x·y 좌표.</returns>
    private float[] Center(int parent)
    {
        OriginalTypeRecord type = Type(_pool.Slot(parent)[TypeOffset]);
        if (type.FootX < 1 || type.FootY < 1 || type.FootX > Side || type.FootY > Side)
        {
            throw new ArgumentOutOfRangeException(nameof(parent), "Damageable 해방 발자국 범위 오류");
        }
        float[] point = Position(parent);
        // 좌표 저장 전까지 double 로 계산한다.
        return
        [
            (float)((double)point[0] - (double)type.FootX * 0.5),
            (float)((double)point[1] - (double)type.FootY * 0.5),
        ];
    }

    /// <summary>고정 보정 좌표의 공간 바이트를 읽는다.</summary>
    /// <param name="point">스냅한 좌표.</param>
    /// <returns>해당 칸의 spot 바이트.</returns>
    private byte Spot(float[] point) => _spots[Cell(point[1], SpotBias) * Side + Cell(point[0], SpotBias)];

    /// <summary>
    /// 권한 해방 몸체를 실행한다. 상위에서 확인한 boss·extra·flags 는 다시 평가하지 않는다.
    /// </summary>
    /// <param name="parent">해방할 부모 슬롯 번호.</param>
    public void Release(int parent)
    {
        // 빈 슬롯·할당되지 않은 번호는 여기에서 거부된다.
        Span<byte> raw = _pool.AllocatedBytes(parent);
        if ((raw[StateOffset] & 9) != 0 || raw[TypeOffset] < OriginalConstructors.FirstAssetType)
        {
            throw new InvalidOperationException("Damageable 해방 부모 상태 오류");
        }
        float[] point = Type(raw[TypeOffset]).FootX == 1 ? Position(parent) : Center(parent);
        // 스냅한 정수는 float 로 저장되며 이후 일반 자산의 위치로 고정된다.
        for (int i = 0; i < point.Length; i++)
        {
            point[i] = Cell(point[i], SnapBias);
        }
        var finder = new RawContainedFinder(_pool, _finderState);
        // 반환 전 저장된 next 를 사용하므로 현재 후보의 연결 변경에도 원래 다음 후보로 간다.
        for (int current = finder.Begin(parent, true); current != 0; current = finder.Next())
        {
            if ((_pool.Slot(current)[MaskOffset] & Releasable) == 0)
            {
                continue;
            }
            uint kind = Read(_pool.Slot(current), KindOffset, 2);
            if (!_state.Battle && (Type(kind).Flags2 & _state.AllowedGenus) == 0)
            {
                continue;
            }
            if ((Type(kind).Flags2 & PriestBit) != 0)
            {
                float[] position = Position(parent);
                if ((Type(_pool.Slot(parent)[TypeOffset]).Flags2 & CenteredParent) != 0)
                {
                    position = Center(parent);
                    position[0] += 0.5f;
                    position[1] += 0.5f;
                }
                _hooks.SpawnPriest!(position[0], position[1], kind, (ushort)Read(_pool.Slot(current), WordOffset, 2));
            }
            else
            {
                if ((Spot(point) & 0x10) != 0 || (Spot(point) & 6) == 0)
                {
                    continue;
                }
                int created = _hooks.Create!(kind, 0);
                ushort word = (ushort)Read(_pool.Slot(current), WordOffset, 2);
                Span<byte> target = _pool.AllocatedBytes(created);
                target[WordOffset] = (byte)word;
                target[WordOffset + 1] = (byte)(word >> 8);
                if (target[TypeOffset] == _state.BridgeType)
                {
                    _hooks.NotifySurface!(created);
                }
                _hooks.SetOwner!(created, 0);
                _hooks.Pop!(created, point[0], point[1], 0);
            }
        }
    }

    /// <summary>x87 의 덧셈 뒤 절삭 결과를 유효한 공간 인덱스로 제한한다.</summary>
    /// <param name="coordinate">원본 좌표.</param>
    /// <param name="bias">더할 편향.</param>
    /// <returns>칸 번호.</returns>
    private static int Cell(float coordinate, float bias)
    {
        double value = (double)coordinate + (double)bias;
        if (!double.IsFinite(value) || value <= -1 || value >= Side)
        {
            throw new ArgumentOutOfRangeException(nameof(coordinate), coordinate, "Damageable 해방 좌표 범위 오류");
        }
        return (int)value;
    }

    /// <summary>슬롯 안의 정렬되지 않은 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다.
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
    /// <summary>
    /// 동일한 풀의 실제 종속 해방만 연결하고 다른 Damageable 효과는 유지한다.
    /// </summary>
    /// <param name="pool">연결 대상 슬롯 풀.</param>
    /// <param name="release">실제 해방 몸체.</param>
    /// <param name="hooks">연결할 피해가능객체 사전 파괴 훅.</param>
    /// <returns>해방 몸체가 연결된 훅.</returns>
    public static DamageablePreDestroyHooks MakeDamageableReleaseHooks(RawSidPool pool, RawDamageableRelease release,
        DamageablePreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(release);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, release.Pool))
        {
            throw new ArgumentException("Damageable 해방의 슬롯 풀이 다릅니다", nameof(pool));
        }
        hooks.ReleaseContained = sid => release.Release(sid);
        return hooks;
    }
}
