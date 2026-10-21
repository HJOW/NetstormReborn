using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>
/// 피해 가능 객체의 삭제 후 처리에서 매 호출 읽는 원본 전역(원본 DamageablePostDestroyState).
/// </summary>
public sealed class DamageablePostDestroyState
{
    /// <summary>전투 중인지(원본 00594fbc). 주변 HP 피해와 다리 후처리를 허용한다.</summary>
    public bool Battle { get; set; }

    /// <summary>금 간 다리의 실제 가상 삭제 권한(원본 00540bc4).</summary>
    public bool Boss { get; set; } = true;

    /// <summary>현재 생성 수(원본 0055a4f8). 20 보다 크면 생성 수를 절반으로 줄인다.</summary>
    public int DudeCount { get; set; }

    /// <summary>일반 삭제의 섬 정리에서 제외하는 타입(원본 rainBlocker).</summary>
    public uint SilentSurfaceType { get; set; } = 147;

    /// <summary>원본 Regular 이벤트 번호. packed 피해·소유자는 별도 count 에 쓴다.</summary>
    public uint DamageEvent { get; set; } = 6699;

    /// <summary>약화가 쓰는 life 값(원본 0052f960). 실제 life WORD 는 이 값 −1 이다.</summary>
    public uint BridgeCrackLife { get; set; } = 5;

    /// <summary>드롭에서 제외하는 타입 번호(10.78 은 5개).</summary>
    public uint[] ExcludedDropTypes { get; set; } = [133, 156, 144, 138, 187];

    /// <summary>이 타입이면 일반 수량 대신 3개를 생성한다.</summary>
    public uint SpecialDropType { get; set; } = 135;
}

/// <summary>
/// 피해 가능 객체의 삭제 후 처리가 바깥에 요구하는 콜백 묶음(원본 DamageablePostDestroyHooks).
/// </summary>
public sealed class DamageablePostDestroyHooks
{
    /// <summary>드롭 생성 경계. 인덱스와 random 여부를 그대로 전달한다.</summary>
    public Action<int, int, bool>? DropDude { get; set; }

    /// <summary>섬 정리 드롭. 좌표·플래그·소유자를 그대로 전달한다.</summary>
    public Action<float, float, uint, uint>? DropIsland { get; set; }

    /// <summary>실제 AddRegular 직후 count 에 packed 값을 쓴다.</summary>
    public Action<int, uint, float, uint>? Regular { get; set; }

    /// <summary>예약된 다리 붕괴(0x2692) 조회.</summary>
    public Func<int, bool>? HasBridgeFall { get; set; }

    /// <summary>약화 WORD 쓰기 직후의 실제 공간 통지.</summary>
    public Action<int>? NotifySurface { get; set; }

    /// <summary>프레임이 바뀌면 실제 표시 갱신을 부른다.</summary>
    public Action<int>? DisplayUpdate { get; set; }

    /// <summary>금 간 다리의 실제 가상 삭제.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>마지막 공통 postDestroy.</summary>
    public Action<int, uint>? Base { get; set; }
}

/// <summary>
/// 피해 가능 객체의 삭제 후 실제 드롭·섬 정리·주변 HP 예약·다리 약화와 공통 post 를 연결한다(원본 RawDamageablePostDestroy, 0044b9e0).
/// 순서는 원본 그대로 "드롭 → 섬 정리 → HP 예약 → 다리 순회 → 공통 post"다.
/// 각 효과 뒤 현재 raw 타입·상태·좌표와 전역을 다시 읽는다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawDamageablePostDestroy.cpp (W03 삭제·수명 일곱째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageablePostDestroy
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 WORD 의 위치(10.78).</summary>
    private const int WordOffset = 12;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 소유자 바이트의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>슬롯 안 프레임 번호의 위치(10.78, 4바이트).</summary>
    private const int FrameOffset = 36;

    /// <summary>드롭 대상 genus 마스크(원본 0x174200).</summary>
    private const uint DropMask = 0x174200;

    /// <summary>genus 의 소용돌이 비트(원본 0x200).</summary>
    private const uint Vortex = 0x200;

    /// <summary>genus 의 다리 비트(원본 4).</summary>
    private const uint Bridge = 4;

    /// <summary>섬 정리 판정용 genus 비트(원본 0x1000000).</summary>
    private const uint Rubble = 0x1000000;

    /// <summary>finder 반경 계산의 편향(원본 0.99999).</summary>
    private const float AreaBias = 0.99999f;

    /// <summary>다리 프레임의 금간 플래그(원본 0x20).</summary>
    private const byte CrackedFlag = 0x20;

    /// <summary>다리 프레임의 단단한 플래그(원본 0x40).</summary>
    private const byte HardFlag = 0x40;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>일반 finder 가 쓰는 4단계 공간 해시.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>타입별 프레임 코드 표.</summary>
    private readonly IReadOnlyList<IReadOnlyList<FrameCode>> _frames;

    /// <summary>피해 가능 객체의 HP 조회 몸체.</summary>
    private readonly RawHitPoints _hp;

    /// <summary>발자국 중심을 계산하는 이웃 몸체.</summary>
    private readonly RawSquidNeighbors _footprint;

    /// <summary>현재 원본 전역.</summary>
    private readonly DamageablePostDestroyState _state;

    /// <summary>외부 효과 콜백 묶음.</summary>
    private readonly DamageablePostDestroyHooks _hooks;

    /// <summary>
    /// 피해 가능 객체의 삭제 후 처리를 만든다. 실제 자료와 여덟 하위 효과를 모두 확인한다.
    /// </summary>
    /// <param name="pool">삭제 대상 슬롯 풀.</param>
    /// <param name="hash">일반 finder 가 쓰는 공간 해시.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="frames">타입별 프레임 코드 표.</param>
    /// <param name="hp">HP 조회 몸체. 같은 풀을 써야 한다.</param>
    /// <param name="footprint">발자국 중심 몸체. 같은 풀·같은 해시를 써야 한다.</param>
    /// <param name="state">현재 원본 전역.</param>
    /// <param name="hooks">외부 효과 콜백 묶음.</param>
    public RawDamageablePostDestroy(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<IReadOnlyList<FrameCode>> frames, RawHitPoints hp, RawSquidNeighbors footprint,
        DamageablePostDestroyState state, DamageablePostDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(hp);
        ArgumentNullException.ThrowIfNull(footprint);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != OriginalConstructors.TypeCount || frames.Count != types.Count)
        {
            throw new ArgumentException("Damageable 사후 파괴 타입/프레임 표 크기 오류", nameof(types));
        }
        if (!ReferenceEquals(hp.Pool, pool))
        {
            throw new ArgumentException("Damageable 사후 파괴와 HP 조회 몸체의 슬롯 풀이 다릅니다", nameof(hp));
        }
        if (!ReferenceEquals(footprint.Pool, pool) || !ReferenceEquals(footprint.Hash, hash))
        {
            throw new ArgumentException("Damageable 사후 파괴와 이웃 몸체의 풀·해시가 다릅니다", nameof(footprint));
        }
        if (hooks.DropDude is null || hooks.DropIsland is null || hooks.Regular is null || hooks.HasBridgeFall is null
            || hooks.NotifySurface is null || hooks.DisplayUpdate is null || hooks.Destroy is null || hooks.Base is null)
        {
            throw new ArgumentException("Damageable 사후 파괴 하위 효과 누락", nameof(hooks));
        }
        _pool = pool;
        _hash = hash;
        _types = types;
        _frames = frames;
        _hp = hp;
        _footprint = footprint;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀. 연결 어댑터가 같은 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>삭제 뒤 raw 의 dead 상태도 원본처럼 읽으며 명시 번호·타입 범위만 제한한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private ReadOnlySpan<byte> Slot(int sid)
    {
        if (sid <= 0 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "Damageable 사후 파괴 번호 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), raw[TypeOffset], "Damageable 사후 파괴 타입 오류");
        }
        return raw;
    }

    /// <summary>콜백 뒤 새 raw 타입을 다시 조회한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private OriginalTypeRecord Type(int sid) => _types[Slot(sid)[TypeOffset]];

    /// <summary>현재 원본 genus(타입 플래그2)다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private uint Genus(int sid) => Type(sid).Flags2;

    /// <summary>현재 extra 의 abstract·매림 비트만 검사한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private bool Ordinary(int sid) => (Slot(sid)[ExtraOffset] & 9) == 0;

    /// <summary>부모·후보의 raw 좌표를 읽는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private float[] Position(int sid)
    {
        ReadOnlySpan<byte> raw = Slot(sid);
        return
        [
            BitConverter.UInt32BitsToSingle(Read(raw, XOffset)),
            BitConverter.UInt32BitsToSingle(Read(raw, YOffset)),
        ];
    }

    /// <summary>일반 finder 로 한 칸 genus 조회만 하는 helper 다.</summary>
    /// <param name="x">조회 x 좌표.</param>
    /// <param name="y">조회 y 좌표.</param>
    /// <param name="genus">찾을 genus 마스크.</param>
    /// <returns>첫 일치 후보 번호. 없으면 0.</returns>
    public int GenusAt(float x, float y, uint genus)
    {
        int ix = Truncate(x);
        int iy = Truncate(y);
        var finder = new RawSquidFinder(_pool, _hash, _types);
        // 원본의 첫 통과 후보만 반환하고 이 helper 가 free/dead 필터를 따로 추가하지 않는다.
        for (int current = finder.Begin(new SquidSearchArea(ix, iy, ix, iy), (genus & 6) == 0 ? 1u : 0u);
            current != 0; current = finder.Next())
        {
            if ((Genus(current) & genus) != 0)
            {
                return current;
            }
        }
        return 0;
    }

    /// <summary>중심 float 저장 뒤 원본 거의 올림을 적용해 탐색 영역을 만든다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="radius">반경.</param>
    private SquidSearchArea Area(int sid, int radius)
    {
        float[] center = _footprint.Center(sid);
        int x = Truncate((double)center[0] + AreaBias);
        int y = Truncate((double)center[1] + AreaBias);
        return new SquidSearchArea(Add(x, -radius), Add(y, -radius), Add(x, radius), Add(y, radius));
    }

    /// <summary>현재 다리 프레임의 플래그 바이트를 읽는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private byte FrameFlags(int sid)
    {
        ReadOnlySpan<byte> raw = Slot(sid);
        uint frame = Read(raw, FrameOffset);
        IReadOnlyList<FrameCode> codes = _frames[raw[TypeOffset]];
        if (frame >= codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), frame, "Damageable 사후 파괴 다리 프레임 오류");
        }
        return (byte)codes[(int)frame].Flags;
    }

    /// <summary>약화 WORD 쓰기·현재 전역·공간 콜백 이후의 타입·프레임 재조회 순서를 유지한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private void Weaken(int sid)
    {
        uint word = Read(Slot(sid), WordOffset, 2);
        uint next = (word & ~0x78u) | ((_state.BridgeCrackLife - 1u) * 8u & 0x78u);
        Write(_pool.AllocatedBytes(sid), WordOffset, next, 2);
        _hooks.NotifySurface!(sid);
        ReadOnlySpan<byte> raw = Slot(sid);
        uint old = Read(raw, FrameOffset);
        int frame = BridgeFrameSearch.CrackedFrame(_frames[raw[TypeOffset]], (int)old, true);
        if (frame != -1)
        {
            Write(_pool.AllocatedBytes(sid), FrameOffset, (uint)frame, 4);
            _hooks.DisplayUpdate!(sid);
        }
    }

    /// <summary>실제 생성 효과의 수량을 정하고 매 반복 요청한다.</summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="genus">현재 genus.</param>
    /// <param name="random">무작위 배치를 쓸지.</param>
    public void Drops(int sid, uint genus, bool random)
    {
        if (sid == 0)
        {
            return;
        }
        ReadOnlySpan<byte> raw = Slot(sid);
        if ((raw[ExtraOffset] & 1) != 0 || (genus & DropMask) == 0)
        {
            return;
        }
        uint count = 1;
        if ((genus & 0x4200u) != 0)
        {
            count = 5;
        }
        if ((genus & 0x20000u) != 0)
        {
            count = 2;
        }
        uint type = raw[TypeOffset];
        // 제외 타입과 현재 특별 타입을 원본 우선순위로 검사한다.
        foreach (uint excluded in _state.ExcludedDropTypes)
        {
            if (type == excluded)
            {
                return;
            }
        }
        if (type == _state.SpecialDropType)
        {
            count = 3;
        }
        if (_state.DudeCount > 20)
        {
            count /= 2;
        }
        // 생성 뒤 부모·전역이 변해도 처음 계산한 반복 수는 다시 계산하지 않는다.
        for (uint index = 0; index < count; index++)
        {
            _hooks.DropDude!(sid, (int)index, random);
        }
    }

    /// <summary>
    /// 삭제 후 처리를 실행한다. 첫 드롭·섬 정리 뒤 부모 상태를 현재 읽고 HP 피해 예약과 다리 순회를 수행한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void PostDestroy(int sid, uint flags)
    {
        _ = Slot(sid);
        if ((flags & 0x300000u) != 0)
        {
            Drops(sid, Genus(sid), true);
        }
        if (Ordinary(sid) && (Type(sid).Flags1 & 0x400u) != 0)
        {
            float[] position = Position(sid);
            if (GenusAt(position[0], position[1], Rubble) != 0 && (Genus(sid) & 0x10000000u) == 0
                && ((flags & 0x200000u) != 0 || Slot(sid)[TypeOffset] != _state.SilentSurfaceType))
            {
                float[] current = Position(sid);
                _hooks.DropIsland!(current[0], current[1], flags, Slot(sid)[OwnerOffset]);
            }
        }
        if (_state.Battle && _hp.HasHitPoints(sid) && Ordinary(sid))
        {
            uint attackerOwner = (flags >> 16) & 15u;
            if ((flags & 0x200000u) == 0 && (Type(sid).Group == 0 || Type(sid).Group == 1 || (Genus(sid) & Vortex) != 0))
            {
                int damage = Add(_hp.MaxHitPoints(sid) / 2, 1);
                if ((Genus(sid) & Vortex) != 0)
                {
                    damage = 400;
                }
                if ((flags & 0x400000u) == 0)
                {
                    int radius = Add(Type(sid).FootX / 2, 1);
                    var finder = new RawSquidFinder(_pool, _hash, _types);
                    // 첫 후보부터 현재 일반 finder 의 skipSurface=1 순서로 HP 대상을 처리한다.
                    for (int current = finder.Begin(Area(sid, radius), 1); current != 0; current = finder.Next())
                    {
                        if (_hp.HasHitPoints(current) && (Slot(current)[StateOffset] & 2) == 0
                            && (Slot(current)[ExtraOffset] & 1) == 0)
                        {
                            _hooks.Regular!(current, _state.DamageEvent, 0.0f,
                                attackerOwner * 65536u + (uint)damage);
                        }
                    }
                }
            }
            if ((Genus(sid) & 0x130000u) == 0)
            {
                var finder = new RawSquidFinder(_pool, _hash, _types);
                // 기존 순회가 저장한 next 를 사용하며 각 후보 frame·state 는 효과 뒤 새로 읽는다.
                for (int current = finder.Begin(Area(sid, 2)); current != 0; current = finder.Next())
                {
                    if ((Genus(current) & Bridge) != 0)
                    {
                        if ((FrameFlags(current) & CrackedFlag) == 0 || _hooks.HasBridgeFall!(current))
                        {
                            if ((FrameFlags(current) & HardFlag) == 0)
                            {
                                Weaken(current);
                            }
                        }
                        else if (_state.Boss)
                        {
                            _hooks.Destroy!(current, 0);
                        }
                    }
                }
            }
        }
        _hooks.Base!(sid, flags);
    }

    /// <summary>_ftol 의 signed 64 절삭 후 하위 DWORD 를 보존한다.</summary>
    /// <param name="value">절삭할 값.</param>
    private static int Truncate(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "Damageable 사후 파괴 좌표 범위 오류");
        }
        return unchecked((int)unchecked((uint)(long)value));
    }

    /// <summary>signed add 의 원본 DWORD 감김을 유지한다.</summary>
    /// <param name="value">원래 값.</param>
    /// <param name="delta">더할 값.</param>
    private static int Add(int value, int delta) => unchecked((int)unchecked((uint)value + (uint)delta));

    /// <summary>슬롯 안의 정렬되지 않은 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 필드의 각 바이트만 결합한다.
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (i * 8);
        }
        return value;
    }

    /// <summary>원본 WORD·프레임의 폭만 써 이웃 필드를 보존한다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Write(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 원본 지정 폭만 대입한다.
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }
}
