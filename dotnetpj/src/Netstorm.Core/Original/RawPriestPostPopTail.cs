using System.Buffers.Binary;
using Netstorm.Assets;

// 사제의 실제 등록 접두·Carrier와 현재 지면/프레임에 따른 후반 처리를 조합한다.
namespace Netstorm.Core.Original;

/// <summary>사제 후반이 도달한 시점에 요구하는 전체 낙하·보호막 효과다.</summary>
public sealed class PriestPostPopTailHooks
{
    /// <summary>전체 낙하 요청이며 내부 공간 재등록도 연결 대상이 수행한다.</summary>
    public Action<int>? Fall { get; set; }
    /// <summary>이동 불가 분기에서 현재 보호막을 확보한다.</summary>
    public Action<int>? EnsureShield { get; set; }
    /// <summary>이동 가능 분기에서 현재 보호막을 조회해 지운다.</summary>
    public Action<int>? ClearShield { get; set; }
}

/// <summary>PATCH004950f0의 접두→Carrier→현재 상태→낙하/보호막 순서를 실행한다.</summary>
public sealed class RawPriestPostPopTail
{
    /// <summary>지면 조회의 원본 float 거의 올림 배율이다.</summary>
    private const float CoordinateBias = 0.9999f;
    /// <summary>같은 실제 원시 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>목록·회복 예약·소유자 상태를 처리하는 접두다.</summary>
    private readonly RawPriestPostPop _prefix;
    /// <summary>현재 지면 소유자와 공통 장부 깊이를 끝내는 실제 Carrier다.</summary>
    private readonly RawCarrierPostPop _carrier;
    /// <summary>같은 HP·현재 TYPE·점유 지도의 사제 상태 조회다.</summary>
    private readonly RawPriestState _state;
    /// <summary>원문 생성자처럼 복사해 소유한 타입별 방향 코드 표다.</summary>
    private readonly FrameCode[][] _frames;
    /// <summary>현재 지면 비트를 읽는 동일 점유 지도다.</summary>
    private readonly byte[] _spots;
    /// <summary>도달한 효과만 요구하는 실제 낙하·보호막 소비자다.</summary>
    private readonly PriestPostPopTailHooks _hooks;

    /// <summary>같은 풀·실제 모듈·188프레임 표·65536셀을 연결한다. 훅은 도달 시 요구한다.</summary>
    public RawPriestPostPopTail(RawSidPool pool, RawPriestPostPop prefix, RawCarrierPostPop carrier,
        RawPriestState state, IReadOnlyList<RiftTypeFrames> frames, byte[] spots, PriestPostPopTailHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(prefix);
        ArgumentNullException.ThrowIfNull(carrier); ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(spots); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(pool, prefix.Pool())
            || !ReferenceEquals(pool, carrier.Pool) || !ReferenceEquals(pool, state.Pool)
            || frames.Count != 188 || spots.Length != 65536)
            throw new ArgumentException("사제 후반 실제 PATCH 풀·프레임·점유 지도 연결 오류");
        _pool = pool; _prefix = prefix; _carrier = carrier; _state = state; _spots = spots; _hooks = hooks;
        _frames = new FrameCode[frames.Count][];
        // 원문 vector 복사와 같이 각 타입의 방향 코드도 독립 저장한다.
        for (int index = 0; index < frames.Count; index++)
            _frames[index] = frames[index].Codes.Select(code => new FrameCode((char)code.Side,
                (char)code.Variant, code.Number, (FrameCodeFlags)code.Flags)).ToArray();
    }

    /// <summary>필요해진 효과만 현재 연결에서 선택한다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function
        ?? throw new InvalidOperationException("사제 postPop 후반 선택된 실제 하위 미연결");

    /// <summary>float 입력과 float 배율을 double로 합산하고 0방향 절삭한다.</summary>
    private static int Cell(float value)
    {
        double shifted = (double)value + CoordinateBias;
        if (!float.IsFinite(value) || shifted < 0 || shifted >= 256)
            throw new ArgumentOutOfRangeException(nameof(value), "사제 후반 지면 좌표 오류");
        return (int)shifted;
    }

    /// <summary>Carrier 콜백 뒤 현재 extra·HP·위치를 다시 읽으며 낙하 뒤에도 보호막 확보를 유지한다.</summary>
    public void PostPop(int sid, uint flags)
    {
        _prefix.Prefix(sid, flags); _carrier.PostPop(sid, flags);
        if ((_pool.Slot(sid)[40] & 9) != 0) return;
        if (!_state.Immobile(sid)) { Need(_hooks.ClearShield)(sid); return; }
        if ((flags & 0x800) == 0)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            int x = Cell(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[14..])));
            int y = Cell(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..])));
            if ((_spots[y * 256 + x] & 6) == 0)
            {
                int frame = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
                if (RawPathAnimation.Direction(_frames[raw[10]], frame) != 9) Need(_hooks.Fall)(sid);
            }
        }
        Need(_hooks.EnsureShield)(sid);
    }

    /// <summary>공간 변경 전 접두 계약만 검사하며 지면·현재 이동 가능 여부는 미리 읽지 않는다.</summary>
    public void Validate(int sid) => _prefix.Validate(sid);
    /// <summary>일반 Pop 가상 분배가 같은 실제 풀인지 검증하는 참조다.</summary>
    public RawSidPool Pool() => _pool;
}
