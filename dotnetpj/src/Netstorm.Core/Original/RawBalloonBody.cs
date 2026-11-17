// 벌룬의 그림자 후처리와 그림자 우선 삭제를 같은 월드에 연결한다.
// cpppj/src/o/RawBalloonBody.h·cpp 의 RawBalloonBody 본체를 이식한다.
// 0.584에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Balloon 우선)
namespace Netstorm.Core.Original;

/// <summary>
/// 현재 TYPE와 실제 가상 월드 효과만 빌린다.
/// (원본 BalloonBodyHooks, 10.78).
/// </summary>
public sealed class BalloonBodyHooks
{
    /// <summary>캐시한 TYPE 주소의 현재+114를 읽는다.</summary>
    public Func<uint, int>? Frames { get; set; }

    /// <summary>실제 생성이다. 초기화된 할당 슬롯 번호를 돌려준다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>현재 가상+90이다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }

    /// <summary>그림자의 현재 가상+48/+10이다.</summary>
    public Action<int, uint>? Unpop { get; set; }

    /// <summary>그림자의 현재 가상+48/+10이다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>현재 부모 가상+cc다. 비서버의 정확한 분기에서만 필요하다.</summary>
    public Action<int>? Check { get; set; }

    /// <summary>클라우드 플로터의 최초 확보 성공 여부다. 없으면 주기 생성만 생략한다.</summary>
    public Func<bool>? AllocateRegular { get; set; }

    /// <summary>확보한 같은 Regular에 사건·지연을 쓰고 부모에 부착한다.</summary>
    public Action<int, uint, float>? ConstructRegular { get; set; }
}

/// <summary>
/// 벌룬의 그림자 후처리와 그림자 우선 삭제를 같은 월드에 연결한다.
/// 상위 Pop/삭제의 공통 깊이는 기존 실제 Damageable/Carrier가 각각 한 번 끝낸다.
/// (원본 RawBalloonBody, 10.78).
/// </summary>
public sealed class RawBalloonBody
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 부모 번호(16비트)의 위치.</summary>
    private const int ParentOffset = 16;

    /// <summary>슬롯 안 추가 비트 바이트의 위치.</summary>
    private const int ExtraOffset = 40;

    /// <summary>원본 높이→칸 변환 상수(504194)다.</summary>
    private const float HeightToCell = 0.09090909361839294f;

    /// <summary>원본 그림자 X 보정(5022ec)이다.</summary>
    private const float ShadowX = 0.10000000149011612f;

    /// <summary>원본 그림자 Y 보정(504190)이다.</summary>
    private const float ShadowY = 0.30000001192092896f;

    /// <summary>원본 PATCH 벌룬/에어십 가상 표 값이다.</summary>
    private const uint BalloonVtable = 0x005040c0u;

    /// <summary>원본 PATCH 클라우드 플로터 가상 표 값이다.</summary>
    private const uint CloudVtable = 0x00504198u;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>최초 TYPE 주소를 효과 뒤에도 보존한다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>고도0의 깊이를 결정하는 현재 칸 비트다.</summary>
    private readonly byte[] _spots;

    /// <summary>마지막 check와 Carrier 전처리가 읽는 현재 생성 권한이다.</summary>
    private readonly Func<bool> _boss;

    /// <summary>같은 공통 깊이를 끝내는 실제 Damageable 후처리다.</summary>
    private readonly RawCarrierPostPop _post;

    /// <summary>그림자 삭제 뒤 실제 Damageable/내용물/보충 부모다.</summary>
    private readonly RawCarrierPreDestroy _pre;

    /// <summary>현재 TYPE 프레임과 큰 가상 월드 경계다.</summary>
    private readonly BalloonBodyHooks _hooks;

    /// <summary>
    /// 벌룬 본체를 만든다. 풀·타입·지도·권한·후처리·전처리·훅은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">부모와 그림자의 단일 원시 저장소다.</param>
    /// <param name="types">최초 TYPE 주소 표다. PATCH 188개여야 한다.</param>
    /// <param name="spots">고도 칸 비트 65,536칸이다.</param>
    /// <param name="boss">마지막 check와 Carrier 전처리가 읽는 현재 생성 권한이다.</param>
    /// <param name="post">같은 공통 깊이를 끝내는 실제 Damageable 후처리다.</param>
    /// <param name="pre">그림자 삭제 뒤 실제 Damageable/내용물/보충 부모다.</param>
    /// <param name="hooks">현재 TYPE 프레임과 큰 가상 월드 경계다.</param>
    public RawBalloonBody(
        RawSidPool pool,
        IReadOnlyList<OriginalTypeRecord> types,
        byte[] spots,
        Func<bool> boss,
        RawCarrierPostPop post,
        RawCarrierPreDestroy pre,
        BalloonBodyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(boss);
        ArgumentNullException.ThrowIfNull(post);
        ArgumentNullException.ThrowIfNull(pre);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != OriginalConstructors.TypeCount || spots.Length != RawSurfaceStorage.SpotCells
            || !ReferenceEquals(post.Pool, pool) || !ReferenceEquals(pre.Pool, pool))
        {
            throw new ArgumentException("벌룬의 실제 원본 월드 연결 오류", nameof(pool));
        }

        if (hooks.Frames is null || hooks.Create is null || hooks.Pop is null || hooks.Unpop is null
            || hooks.Destroy is null || hooks.Check is null)
        {
            throw new ArgumentException("벌룬 본체 효과 경계 누락", nameof(hooks));
        }

        _pool = pool;
        _types = types;
        _spots = spots;
        _boss = boss;
        _post = post;
        _pre = pre;
        _hooks = new()
        {
            Frames = hooks.Frames,
            Create = hooks.Create,
            Pop = hooks.Pop,
            Unpop = hooks.Unpop,
            Destroy = hooks.Destroy,
            Check = hooks.Check,
            AllocateRegular = hooks.AllocateRegular,
            ConstructRegular = hooks.ConstructRegular,
        };
    }

    /// <summary>같은 유일한 슬롯 저장소다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>가상 표만 확인하며 원본의 두 동일 생성자 타입을 임의로 합성하지 않는다.</summary>
    /// <param name="sid">확인할 객체 번호.</param>
    public bool Handles(int sid)
    {
        uint table = Read(_pool.Slot(sid), 0);
        return table == BalloonVtable || table == CloudVtable;
    }

    /// <summary>현재 VT가 벌룬 경로일 때만 실행하며 TYPE 조회 전에 frame DWORD를 보관한다.</summary>
    /// <param name="sid">확인할 객체 번호.</param>
    public bool Robust(int sid)
    {
        if (!Handles(sid))
        {
            throw new ArgumentException("벌룬 Robust의 다른 가상 표", nameof(sid));
        }

        int frame = unchecked((int)Read(_pool.Slot(sid), 36));
        byte kind = _pool.Slot(sid)[TypeOffset];
        if (kind >= _types.Count || _hooks.Frames is null)
        {
            throw new InvalidOperationException("벌룬 Robust의 현재 TYPE 프레임 조회 누락");
        }

        return frame < _hooks.Frames(kind);
    }

    /// <summary>공간 변경 전 현재 부모만 검사하며 상위 공통 ValidateBase와 효과를 중복 실행하지 않는다.</summary>
    /// <param name="sid">검사할 객체 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void Validate(int sid, uint flags)
    {
        _ = flags;
        ReadOnlySpan<byte> raw = _pool.AllocatedBytes(sid);
        if (!Handles(sid) || (raw[StateOffset] & 9) != 0 || raw[TypeOffset] < OriginalConstructors.FirstAssetType || raw[TypeOffset] >= _types.Count)
        {
            throw new InvalidOperationException("벌룬 후처리의 현재 원시 부모 오류");
        }
    }

    /// <summary>그림자 생성/좌표/표시깊이/전용 check 뒤 Damageable을 호출한다.</summary>
    /// <param name="sid">후처리할 객체 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void PostPop(int sid, uint flags)
    {
        if (Read(_pool.Slot(sid), 0) == CloudVtable && (flags & 1u) != 0)
        {
            if (_hooks.AllocateRegular is null)
            {
                throw new InvalidOperationException("클라우드 플로터 Regular 실제 할당 미연결");
            }

            if (_hooks.AllocateRegular())
            {
                if (_hooks.ConstructRegular is null)
                {
                    throw new InvalidOperationException("클라우드 플로터 Regular 실제 생성 미연결");
                }

                _hooks.ConstructRegular(sid, 3, 0.1f);
            }
        }

        // 실제42aed0은 추가 주기를 먼저 만든 뒤 현재 부모로42ac90을 호출한다.
        byte kind = _pool.Slot(sid)[TypeOffset];
        if (kind >= _types.Count || _hooks.Frames is null)
        {
            throw new InvalidOperationException("벌룬의 현재 TYPE 프레임 미연결");
        }

        OriginalTypeRecord type = _types[kind];
        bool first = (flags & 1u) != 0;
        // 최초 TYPE 주소와 별도 보존한 최초 flag다.
        if (first && unchecked((int)Read(_pool.Slot(sid), 36)) < _hooks.Frames(kind) && (type.Flags1 & 0x400000u) != 0)
        {
            int shadow = (_hooks.Create ?? throw new InvalidOperationException("벌룬 그림자 실제 생성 미연결"))(_pool.Slot(sid)[TypeOffset], 2);
            Put(_pool.AllocatedBytes(shadow), 36, Frame(_pool, sid, _hooks.Frames(kind)));
            _pool.AllocatedBytes(shadow)[40] |= 8;
            Put(_pool.AllocatedBytes(sid), 8, unchecked((uint)shadow), 2);
            _hooks.Pop?.Invoke(shadow, ReadSingle(_pool.Slot(sid), 14), ReadSingle(_pool.Slot(sid), 18), 0);
        }

        // Unpop 중 부모 WORD가 바뀌어도 이 첫 그림자 주소를 유지한다.
        int current = unchecked((int)(ushort)(Read(_pool.Slot(sid), 8, 2)));
        if (current != 0)
        {
            (_hooks.Unpop ?? throw new InvalidOperationException("벌룬 그림자 실제 해제 미연결"))(current, 0);
            Put(_pool.AllocatedBytes(current), 36, Frame(_pool, sid, _hooks.Frames(kind)));
            float x = ReadSingle(_pool.Slot(sid), 14);
            float y = ReadSingle(_pool.Slot(sid), 18);
            double height = _pool.Slot(sid)[30] * HeightToCell;
            _pool.AllocatedBytes(current)[35] = 0xe6;
            // 42ad64~42ad9d는 중간 float 저장 없이 두 좌표의 fstp에서만 반올림한다.
            _hooks.Pop?.Invoke(current, (float)(height * 0.5 + x + ShadowX), (float)(height + y - ShadowY), 0);
        }

        // 첫 가상 Pop 효과 뒤 현재 그림자 WORD를 다시 읽는다.
        current = unchecked((int)(ushort)(Read(_pool.Slot(sid), 8, 2)));
        if (current != 0)
        {
            Span<byte> raw = _pool.AllocatedBytes(current);
            raw[40] ^= (byte)((_pool.Slot(sid)[40] ^ raw[40]) & 0x10);
            raw[35] = 0xe6;
            _pool.AllocatedBytes(sid)[35] = 0xe2;
            if (_pool.Slot(sid)[30] == 0)
            {
                float x = ReadSingle(_pool.Slot(sid), 14);
                float y = ReadSingle(_pool.Slot(sid), 18);
                if (!float.IsFinite(x) || !float.IsFinite(y) || x < 0 || x >= 256 || y < 0 || y >= 256)
                {
                    throw new ArgumentOutOfRangeException(nameof(sid), "벌룬 표시 깊이의 현재 칸 범위");
                }

                _pool.AllocatedBytes(sid)[35] = unchecked((byte)(-20 - ((_spots[unchecked((int)y) * 256 + unchecked((int)x)] & 0x10) != 0 ? 1 : 0)));
                raw[35] = 0xec;
            }
        }

        if (!first && !_boss())
        {
            (_hooks.Check ?? throw new InvalidOperationException("벌룬 부모 가상 확인 미연결"))(sid);
        }

        _post.DamageablePostPop(sid, flags);
    }

    /// <summary>원본42af60은 count와 payload를 변경하지 않고 event3의 반환도 입력 payload다.</summary>
    /// <param name="sid">정기 대상 번호.</param>
    /// <param name="event">예약 사건 번호.</param>
    /// <param name="count">전달 개수.</param>
    /// <param name="payload">전달 실수 인자.</param>
    /// <param name="advance">실제 프레임 진행 경계다.</param>
    /// <param name="base">기본 Carrier 정기 경계다.</param>
    public float CloudRegular(
        int sid,
        uint @event,
        uint count,
        float payload,
        Action<int, int, uint>? advance,
        Func<int, uint, uint, float, float>? @base)
    {
        if (Read(_pool.Slot(sid), 0) != CloudVtable)
        {
            throw new ArgumentException("클라우드 플로터 전용 Regular의 다른 가상 표", nameof(sid));
        }

        if (@event == 3)
        {
            if (advance is null)
            {
                throw new InvalidOperationException("클라우드 플로터 실제 프레임 진행 미연결");
            }

            advance(sid, 1, 0);
            return payload;
        }

        if (@base is null)
        {
            throw new InvalidOperationException("클라우드 플로터 기본 Carrier Regular 미연결");
        }

        return @base(sid, @event, count, payload);
    }

    /// <summary>현재 그림자를 먼저 삭제하고 같은 Carrier 기본 전처리를 실행한다.</summary>
    /// <param name="sid">삭제할 객체 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void PreDestroy(int sid, uint flags)
    {
        // 42ae80은 그림자 삭제 뒤 426890과 동일한 기본 효과 순서를 갖는다.
        int shadow = unchecked((int)(ushort)Read(_pool.Slot(sid), 8, 2));
        if (shadow != 0)
        {
            (_hooks.Destroy ?? throw new InvalidOperationException("벌룬 그림자 실제 삭제 미연결"))(shadow, 0);
        }

        _pre.PreDestroy(sid, flags);
    }

    /// <summary>그림자의 동일 TYPE 내 두 번째 그림 묶음을 원본 signed DWORD 감김으로 계산한다.</summary>
    private static uint Frame(RawSidPool pool, int parent, int count) =>
        unchecked((uint)count + Read(pool.Slot(parent), 36));

    /// <summary>비정렬 필드를 실제 BYTE 폭으로 조립한다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 낮은 BYTE부터 읽는다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>현재 raw WORD/DWORD만 쓰고 인접 바이트는 보존한다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 지정 폭의 낮은 BYTE만 기록한다.
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>원문 비트를 float 그대로 읽는다.</summary>
    private static float ReadSingle(ReadOnlySpan<byte> raw, int offset) =>
        BitConverter.UInt32BitsToSingle(Read(raw, offset));
}
