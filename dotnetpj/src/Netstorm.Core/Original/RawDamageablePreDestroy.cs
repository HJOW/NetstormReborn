namespace Netstorm.Core.Original;

/// <summary>
/// 피해가능객체(Damageable) 삭제 접두에서 나는 소리의 종류(원본 DamageableSound).
/// Collapse 는 붕괴, Explosion 은 폭발, PriestFree 는 사제(종속) 해방의 전역 소리다.
/// </summary>
public enum DamageableSound
{
    /// <summary>붕괴 소리. 좌표는 효과 뒤의 현재 raw 좌표다.</summary>
    Collapse,

    /// <summary>폭발 소리. 좌표는 효과 뒤의 현재 raw 좌표다.</summary>
    Explosion,

    /// <summary>사제 해방 소리. 전역 소리라 좌표 인자가 0 이다.</summary>
    PriestFree,
}

/// <summary>
/// 피해가능객체 소리 한 건의 인자(원본 DamageableSoundEvent).
/// </summary>
/// <param name="Sound">소리 종류.</param>
/// <param name="X">소리 x 좌표(PriestFree 는 0).</param>
/// <param name="Y">소리 y 좌표(PriestFree 는 0).</param>
public readonly record struct DamageableSoundEvent(DamageableSound Sound, float X, float Y);

/// <summary>
/// 피해가능객체 전처리 삭제에서 조회·갱신하는 상태 묶음(원본 DamageablePreDestroyState).
/// </summary>
public sealed class DamageablePreDestroyState
{
    /// <summary>종속 kind WORD 와 비교하는 타입 번호(원본 005412d0). 기본값은 158 이다.</summary>
    public uint PriestType { get; set; } = 158;
}

/// <summary>
/// 피해가능객체 전처리 삭제가 바깥에 요구하는 콜백 묶음(원본 DamageablePreDestroyHooks).
/// 붕괴·폭발·소리·해방·공통 사전 검사·공통 본체 여섯 개가 모두 있어야 직접 부를 수 있다.
/// </summary>
public sealed class DamageablePreDestroyHooks
{
    /// <summary>붕괴 효과의 하위 몸체(원본 004605f0).</summary>
    public Action<int>? Collapse { get; set; }

    /// <summary>폭발 효과의 하위 몸체(원본 004605d0).</summary>
    public Action<int>? Explosion { get; set; }

    /// <summary>좌표 소리와 전역 사제 해제 소리의 요청 경계.</summary>
    public Action<DamageableSoundEvent>? Sound { get; set; }

    /// <summary>권한·플래그 조건 뒤에 해방 몸체를 부른다.</summary>
    public Action<int>? ReleaseContained { get; set; }

    /// <summary>공통 장부·깊이·후속 공간 효과의 순수 사전 검사.</summary>
    public Action<int, uint>? ValidateBase { get; set; }

    /// <summary>같은 플래그의 공통 preDestroy(원본 004b0950).</summary>
    public Action<int, uint>? Base { get; set; }
}

/// <summary>
/// 피해가능객체(Damageable)의 붕괴·폭발·해방 소리 접두와 공통 삭제 호출을 복원한다(원본 RawDamageablePreDestroy, 0044b4b0).
/// 순서는 원본 그대로 "asset/타입 검사 → 좌표 spot 확인 → 붕괴 효과와 소리 → 폭발 효과와 소리 →
/// spot 안쪽 여부에 따른 사제 해방 소리 → 권한에 따른 해방 몸체 → 공통 preDestroy"다.
/// extra 의 abstract·매림 비트가 켜졌으면 좌표를 읽지 않고 공통 본체로 바로 넘어간다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawDamageablePreDestroy.cpp (W03 삭제·수명 다섯째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageablePreDestroy
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치(10.78).</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 종속 kind WORD 의 위치(10.78). 원본은 y 좌표와 같은 자리에서 WORD 만 읽는다.</summary>
    private const int KindOffset = 18;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>추가 비트의 abstract·매림 마스크.</summary>
    private const byte AbstractBuriedMask = 9;

    /// <summary>플래그의 붕괴 효과 비트(원본 0x200000).</summary>
    private const uint CollapseBit = 0x200000;

    /// <summary>플래그의 폭발 효과 비트(원본 0x100000).</summary>
    private const uint ExplosionBit = 0x100000;

    /// <summary>플래그의 해방 금지 비트(원본 0x800).</summary>
    private const uint NoReleaseBit = 0x800;

    /// <summary>원본 spot 좌표의 fadd 상수. float 저장 전 double 계산으로 x87 덧셈을 보존한다.</summary>
    private const float SpotBias = 0.9999f;

    /// <summary>지도 한 변의 칸 수.</summary>
    private const int Side = 256;

    /// <summary>spot 지도 전체 칸 수.</summary>
    private const int Cells = Side * Side;

    /// <summary>spot 비트 가운데 안쪽(2)과 통행 불가(4) 두 개를 본다.</summary>
    private const byte InsideOrBlockedMask = 6;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>칸별 점유·통행 비트를 읽는 spot 지도.</summary>
    private readonly byte[] _spots;

    /// <summary>권한·타입 전역을 읽는 상태.</summary>
    private readonly ContainedFinderState _finderState;

    /// <summary>사제 해방 판정 전역.</summary>
    private readonly DamageablePreDestroyState _state;

    /// <summary>외부 효과 콜백 묶음.</summary>
    private readonly DamageablePreDestroyHooks _hooks;

    /// <summary>
    /// 피해가능객체 전처리 삭제를 만든다. 공간 크기와 여섯 하위 효과를 모두 확인한다.
    /// </summary>
    /// <param name="pool">삭제 대상 슬롯 풀.</param>
    /// <param name="spots">칸별 점유 지도 65,536칸.</param>
    /// <param name="finderState">내용물 검색기의 권한·타입 전역.</param>
    /// <param name="state">사제 해방 판정 전역.</param>
    /// <param name="hooks">외부 효과 콜백 묶음.</param>
    public RawDamageablePreDestroy(RawSidPool pool, byte[] spots, ContainedFinderState finderState,
        DamageablePreDestroyState state, DamageablePreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(finderState);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (spots.Length != Cells)
        {
            throw new ArgumentException("Damageable 사전 파괴 spot 지도 크기 오류", nameof(spots));
        }
        if (hooks.Collapse is null || hooks.Explosion is null || hooks.Sound is null
            || hooks.ReleaseContained is null || hooks.ValidateBase is null || hooks.Base is null)
        {
            throw new ArgumentException("Damageable 사전 파괴 하위 효과 누락", nameof(hooks));
        }
        _pool = pool;
        _spots = spots;
        _finderState = finderState;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀. 연결 어댑터가 참조 대상을 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 자산 상태·타입과 연결된 공통 경계를 효과 전에 검사한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void Validate(int sid, uint flags)
    {
        // 빈 슬롯·할당되지 않은 번호는 여기에서 거부된다.
        Span<byte> raw = _pool.AllocatedBytes(sid);
        if ((raw[StateOffset] & 9) != 0 || raw[TypeOffset] < OriginalConstructors.FirstAssetType
            || raw[TypeOffset] >= OriginalConstructors.TypeCount)
        {
            throw new InvalidOperationException("Damageable 사전 파괴 raw 자산 상태/타입 오류");
        }
        // abstract·매림이면 좌표를 읽지 않는다.
        if ((raw[ExtraOffset] & AbstractBuriedMask) == 0)
        {
            _ = Spot(sid);
        }
        _hooks.ValidateBase!(sid, flags);
    }

    /// <summary>
    /// 피해가능객체의 사전 파괴를 실행한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void PreDestroy(int sid, uint flags)
    {
        Validate(sid, flags);
        // extra 의 진입 조건은 효과 중 extra 가 바뀌어도 다시 평가하지 않는다.
        if ((_pool.Slot(sid)[ExtraOffset] & AbstractBuriedMask) == 0)
        {
            if ((flags & CollapseBit) != 0)
            {
                _hooks.Collapse!(sid);
                SoundAt(sid, DamageableSound.Collapse);
            }
            if ((flags & ExplosionBit) != 0)
            {
                _hooks.Explosion!(sid);
                SoundAt(sid, DamageableSound.Explosion);
            }
            if ((Spot(sid) & InsideOrBlockedMask) != 0)
            {
                var finder = new RawContainedFinder(_pool, _finderState);
                // dead 부모도 허용한다. 소리 콜백 뒤 현재 kind/타입 전역을 다시 읽는다.
                for (int current = finder.Begin(sid, true); current != 0; current = finder.Next())
                {
                    if (Read(_pool.Slot(current), KindOffset, 2) == _state.PriestType)
                    {
                        _hooks.Sound!(new DamageableSoundEvent(DamageableSound.PriestFree, 0, 0));
                    }
                }
            }
            if (_finderState.Boss && (flags & NoReleaseBit) == 0)
            {
                _hooks.ReleaseContained!(sid);
            }
        }
        _hooks.Base!(sid, flags);
    }

    /// <summary>
    /// raw 현재 좌표에 0.9999 를 더한 뒤 0 방향 절삭해 spot 을 읽는다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <returns>해당 칸의 spot 바이트.</returns>
    private byte Spot(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        int x = Cell(BitConverter.UInt32BitsToSingle(Read(raw, XOffset, 4)));
        int y = Cell(BitConverter.UInt32BitsToSingle(Read(raw, YOffset, 4)));
        return _spots[y * Side + x];
    }

    /// <summary>
    /// 붕괴·폭발 효과가 raw 좌표를 바꿀 수 있어 소리 직전에 다시 읽는다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="sound">요청할 소리 종류.</param>
    private void SoundAt(int sid, DamageableSound sound)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        _hooks.Sound!(new DamageableSoundEvent(sound,
            BitConverter.UInt32BitsToSingle(Read(raw, XOffset, 4)),
            BitConverter.UInt32BitsToSingle(Read(raw, YOffset, 4))));
    }

    /// <summary>유효한 보드 좌표만 절삭한다. 더하기 뒤 별도 float 반올림을 넣지 않는다.</summary>
    /// <param name="coordinate">원본 좌표.</param>
    /// <returns>칸 번호.</returns>
    private static int Cell(float coordinate)
    {
        double value = (double)coordinate + (double)SpotBias;
        if (!double.IsFinite(value) || value <= -1 || value >= Side)
        {
            throw new ArgumentOutOfRangeException(nameof(coordinate), coordinate, "Damageable spot 좌표 범위 오류");
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
}
