using Netstorm.Core.Rules;

namespace Netstorm.Core.Original;

/// <summary>
/// Pop 이 공간 등록 때 이전 위치를 미니맵 갱신 큐에 알리는 연결 지점.
/// 원본 RawMetadisplayDirty(미니맵 갱신 큐)와 같은 클래스이므로 Unpop 쪽 <see cref="IRawMinimapQueue"/>와 모양이 같지만,
/// Pop 은 섬·다리 오브젝트를 등록하기 직전에 알리므로 방향별 문서를 따로 둔다. 구현체가 생기면 연결한다.
/// </summary>
public interface IRawPopMetadisplay
{
    /// <summary>이 큐가 쓰는 슬롯 풀.</summary>
    RawSidPool Pool { get; }

    /// <summary>좌표·화면 좌표 쓰기가 끝난 직후, spot 등록과 체인 연결 전에 오브젝트의 새 위치를 큐에 넣는다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    void Enqueue(int sid);
}

/// <summary>
/// Pop 이 건물 발자국의 왼쪽·오른쪽 가장자리 열(맨 위·맨 아래 행 제외)에 놓는 건물 점유 바이트를 쓰는 건물 공간 모듈의 연결 지점.
/// 원본 RawBuildingSpace 와 같은 풀·같은 공간 해시의 몸체만 연결할 수 있다. 구현체가 생기면 연결한다.
/// </summary>
public interface IRawPopBuildingSide
{
    /// <summary>이 몸체가 쓰는 슬롯 풀.</summary>
    RawSidPool Pool { get; }

    /// <summary>이 몸체가 쓰는 공간 해시.</summary>
    RawSquidHash Hash { get; }

    /// <summary>발자국 옆면의 한 칸에 건물 점유 바이트를 쓴다. Pop 의 표면 플래그 검사 없이 호출된다.</summary>
    /// <param name="cellX">칸 x.</param>
    /// <param name="cellY">칸 y.</param>
    /// <param name="buildingX">건물 오브젝트의 월드 x 좌표.</param>
    /// <param name="buildingY">건물 오브젝트의 월드 y 좌표.</param>
    void PopSide(int cellX, int cellY, float buildingX, float buildingY);
}

/// <summary>Pop 이 공간 등록 효과로 넘기는 투영 값(원본 RawGraphPop). 좌표·등록 단계·타입을 함께 전달한다.</summary>
/// <param name="X">등록할 월드 x 좌표.</param>
/// <param name="Y">등록할 월드 y 좌표.</param>
/// <param name="Level">등록할 공간 해시 단계.</param>
/// <param name="Type">그 오브젝트의 타입 레코드.</param>
public readonly record struct PopProjection(float X, float Y, int Level, OriginalTypeRecord Type);

/// <summary>
/// 파생 firstPop/postPop 효과(생산·건설·다리 등 타입별 등록 후 처리)의 연결 지점.
/// 연결하지 않으면(null) 파생 효과가 없는 경로로 보고 아무것도 하지 않는다. 파생 모듈이 생기면 구현한다.
/// </summary>
public interface IRawPopPostPop
{
    /// <summary>이 모듈이 쓰는 슬롯 풀.</summary>
    RawSidPool Pool { get; }

    /// <summary>만드는 시점에 같은 풀·같은 공간 해시·같은 점유 지도를 쓰는지 확인한다. 어긋나면 예외를 던진다.</summary>
    /// <param name="hash">Pop 이 쓰는 공간 해시.</param>
    /// <param name="spots">Pop 이 쓰는 칸별 점유 바이트 65,536개.</param>
    void ValidateSpace(RawSquidHash hash, byte[] spots);

    /// <summary>이 SID 의 파생 Pop 효과를 이 모듈이 처리하는지 돌려준다. Pop 의 지원 표 검사가 실패했을 때만 호출된다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    bool HandlesDerived(int sid);

    /// <summary>좌표·화면 좌표·spot·체인 쓰기 전에 이 등록을 처리할 수 있는지 검사한다. 처리할 수 없으면 예외를 던진다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">정규화한 Pop 호출 플래그(최초 Pop 비트와 Activate 비트 0x10 포함).</param>
    /// <param name="projection">등록 좌표·단계·타입.</param>
    void Validate(int sid, uint flags, in PopProjection projection);

    /// <summary>void 해제·최초 Pop 비트 갱신이 끝난 뒤 파생 등록 효과를 실행한다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="flags">정규화한 Pop 호출 플래그(최초 Pop 비트와 Activate 비트 0x10 포함).</param>
    void Activate(int sid, uint flags);
}

/// <summary>
/// Pop 공간 등록의 결과(원본 RawPopResult, 10.78).
/// </summary>
public enum RawPopResult
{
    /// <summary>번호 0·이미 등록됨·들어 있는 내용물이라 등록하지 않고 그대로 돌려줬다(10.78).</summary>
    Unchanged = 0,

    /// <summary>좌표·화면 좌표·spot·체인·표시·파생 효과까지 등록했다.</summary>
    Registered = 1,

    /// <summary>이미 차 있는 spot 칸과 겹쳐 체인에 넣지 않고 좌표·화면 좌표 쓰기까지만 했다(10.78).</summary>
    Overlap = 2,
}

/// <summary>
/// 오브젝트를 공간 해시와 점유(spot) 지도에 등록하는 원본 공통 Pop(<c>004b02d0</c>, 10.78).
/// 순서는 원본 그대로 "유효 검사 → 좌표·화면 좌표 쓰기 → (섬·다리면) 미니맵 알림 → 발자국 spot 등록(겹치면 Overlap 반환) →
/// 단계·next 쓰기와 머리 연결 → (0단계 다리가 아니면) 섬 번호 127 → 표시 갱신 → 최초 Pop 비트·void 해제 → 파생 Activate"이며,
/// 겹침 반환 때는 좌표·화면 좌표 쓰기와 그때까지의 spot·건물 옆면·미니맵 알림이 남는다.
/// 원본은 손상된 머리·잘못된 발자국·미복원 효과(건물 옆면·표시 갱신의 파생 가상 표)를 <b>풀·공간을 바꾸기 전에</b> 예외로 거부한다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 무효 void 를 조용히 넘기는 등 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/SquidPop.cpp (W02 Pop 조각, 2026-10-20).
/// </summary>
public sealed class RawPop
{
    /// <summary>슬롯 안 x 좌표(float, 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float, 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 화면 x 좌표(short, 2바이트)의 위치.</summary>
    private const int ScreenXOffset = 22;

    /// <summary>슬롯 안 화면 y 좌표(short, 2바이트)의 위치.</summary>
    private const int ScreenYOffset = 24;

    /// <summary>슬롯 안 섬 번호(2바이트)의 위치.</summary>
    private const int IslandOffset = 8;

    /// <summary>슬롯 안 저장 단계 바이트의 위치(10.78).</summary>
    private const int LevelOffset = 33;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>상태 바이트의 전송 중(transmitting) 비트. 이 비트가 켜진 슬롯의 Pop 은 거부한다.</summary>
    private const byte TransmittingBit = 16;

    /// <summary>상태 바이트의 들어 있는 내용물(contained) 비트. 생산 칸 안의 내용물은 base Pop 의 대상이 아니다.</summary>
    private const byte ContainedBit = 8;

    /// <summary>추가 비트의 매몰(buried) 비트. 켜진 오브젝트는 spot 을 쓰지도 지우지도 않으며 건물 공간 연결을 요구하지 않는다.</summary>
    private const byte BuriedBit = 8;

    /// <summary>추가 비트의 최초 Pop(firstPop) 비트. 꺼져 있으면 호출 플래그에 타입 공통 비트를 더하고 켠다.</summary>
    private const byte FirstPopBit = 128;

    /// <summary>등록하지 않고 돌려보내는 호출 플래그 비트(원본 0x8). 꺼져 있으면 파생 Activate 비트 0x10 을 더한다.</summary>
    private const uint DeactivateBit = 8;

    /// <summary>파생 Activate 를 부르는 호출 플래그 비트(원본 0x10).</summary>
    private const uint PostPopBit = 16;

    /// <summary>플래그2에서 미니맵 알림을 받는 섬(2)·다리(4) 비트.</summary>
    private const uint IslandOrBridge = 6;

    /// <summary>플래그2에서 0단계 등록 때 섬 번호를 쓰지 않는 다리 비트.</summary>
    private const uint BridgeBit = 4;

    /// <summary>섬 번호 없음 값(유효 범위 밖 표시, 원본 127).</summary>
    private const uint InvalidIsland = 127;

    /// <summary>화면 x 좌표 환산 배율(원본 004b02d0의 x*16+0.5).</summary>
    private const double ScreenScaleX = 16;

    /// <summary>화면 y 좌표 환산 배율(원본 004b02d0의 y*11+0.5).</summary>
    private const double ScreenScaleY = 11;

    /// <summary>화면 좌표 환산 편향(원본 +0.5, double 으로 더한 뒤 0 쪽 절삭).</summary>
    private const double ScreenBias = 0.5;

    /// <summary>지도 밖 좌표를 대신 쓰는 기본 좌표(원본 10, 10).</summary>
    private const float FallbackCoordinate = 10;

    /// <summary>슬롯을 읽고 쓸 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>좌표별 오브젝트 체인의 머리를 가진 4단계 공간 해시.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>칸별 점유 바이트(y × 256 + x 순서). 호출자가 공유하는 같은 배열이며 이 클래스가 등록 때 바이트를 고친다.</summary>
    private readonly byte[] _spots;

    /// <summary>공통 표시 경로. 없으면 표시 객체가 없는 경로다.</summary>
    private readonly IRawUnpopDisplay? _display;

    /// <summary>파생 등록 효과 몸체. 연결 전이면 null 이며 그 경우 지원 표 밖의 가상 표는 거부된다.</summary>
    private readonly IRawPopPostPop? _postPop;

    /// <summary>건물 옆면 등록 몸체. 연결 전이면 null 이며 건물군(매몰 아님)의 Pop 은 거부된다.</summary>
    private IRawPopBuildingSide? _building;

    /// <summary>미니맵 갱신 큐. 연결 전이면 null.</summary>
    private IRawPopMetadisplay? _metadisplay;

    /// <summary>등록 모듈을 만든다. 점유 지도는 정확히 256×256 바이트여야 한다(부분 지도 밖 쓰기를 막는다).</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">공간 해시.</param>
    /// <param name="spots">칸별 점유 바이트 65,536개. 같은 배열을 Unpop 쪽과 공유한다.</param>
    /// <param name="display">공통 표시 경로. 원본 SquidDisplay 를 공유하므로 Unpop 쪽 인터페이스를 그대로 쓴다.</param>
    /// <param name="postPop">파생 등록 효과 몸체. 표시를 끈 경로는 null.</param>
    public RawPop(RawSidPool pool, RawSquidHash hash, byte[] spots, IRawUnpopDisplay? display = null, IRawPopPostPop? postPop = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(spots);
        if (spots.Length != RawSquidHash.WorldCells * RawSquidHash.WorldCells)
        {
            throw new ArgumentException("Pop spot 지도 크기 오류", nameof(spots));
        }
        if (postPop is not null && !ReferenceEquals(postPop.Pool, pool))
        {
            throw new ArgumentException("Pop/postPop SID 풀 불일치", nameof(postPop));
        }
        _pool = pool;
        _hash = hash;
        _spots = spots;
        _display = display;
        _postPop = postPop;
        postPop?.ValidateSpace(hash, spots);
    }

    /// <summary>이 모듈이 쓰는 슬롯 풀. 팩토리·파생 모듈이 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>건물 옆면 등록 몸체를 연결한다. 이 모듈과 같은 풀·같은 공간 해시를 쓰는 몸체만 받는다. null 이면 연결을 푼다.</summary>
    /// <param name="building">건물 공간 몸체.</param>
    public void SetBuildingSide(IRawPopBuildingSide? building)
    {
        if (building is not null && (!ReferenceEquals(building.Pool, _pool) || !ReferenceEquals(building.Hash, _hash)))
        {
            throw new ArgumentException("Pop/건물 공간의 풀 또는 해시 불일치", nameof(building));
        }
        _building = building;
    }

    /// <summary>미니맵 갱신 큐를 연결한다. 같은 풀을 쓰는 큐만 받는다. null 이면 연결을 푼다.</summary>
    /// <param name="queue">미니맵 큐.</param>
    public void SetMetadisplay(IRawPopMetadisplay? queue)
    {
        if (queue is not null && !ReferenceEquals(queue.Pool, _pool))
        {
            throw new ArgumentException("Pop/미니맵 큐 SID 풀 불일치", nameof(queue));
        }
        _metadisplay = queue;
    }

    /// <summary>
    /// 오브젝트를 공간에 등록한다. 끝나면 슬롯은 void 가 풀리고 좌표·화면 좌표·spot·체인·표시·파생 효과가 차례로 들어간다.
    /// 번호 0·이미 등록됨(void 꺼짐)·들어 있는 내용물은 등록하지 않고 <see cref="RawPopResult.Unchanged"/>로 돌려준다(10.78).
    /// 이미 차 있는 spot 칸과 겹치면 체인에 넣지 않고 <see cref="RawPopResult.Overlap"/>으로 돌려준다.
    /// </summary>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="type">그 오브젝트의 타입 레코드(플래그·발자국).</param>
    /// <param name="frameWidth">그림(SHP) 가로 칸 수. 등록할 해시 단계를 정한다.</param>
    /// <param name="frameHeight">그림(SHP) 세로 칸 수. 등록할 해시 단계를 정한다.</param>
    /// <param name="x">등록할 월드 x 좌표(0보다 크고 256보다 작아야 하며, 벗어나면 10으로 둔다).</param>
    /// <param name="y">등록할 월드 y 좌표(같다).</param>
    /// <param name="flags">호출 플래그. 0x2000 은 소용돌이 경로를 뜻한다.</param>
    public RawPopResult Pop(int sid, OriginalTypeRecord type, float frameWidth, float frameHeight, float x, float y, uint flags = 0)
    {
        ArgumentNullException.ThrowIfNull(type);
        ReadOnlySpan<byte> old = _pool.Slot(sid);
        if (sid == 0 || (old[RawSidPool.StateOffset] & RawSidPool.VoidBit) == 0 || (old[RawSidPool.StateOffset] & ContainedBit) != 0)
        {
            return RawPopResult.Unchanged;
        }
        if ((old[RawSidPool.StateOffset] & (RawSidPool.FreeBit | TransmittingBit)) != 0 || old[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType)
        {
            throw new InvalidOperationException("Pop 원본 상태 오류");
        }
        if (!float.IsFinite(x) || !float.IsFinite(y))
        {
            throw new ArgumentOutOfRangeException(nameof(x), "Pop 등록 좌표가 유한하지 않습니다");
        }
        // 지도 밖·0 이하 좌표는 원본 그대로 10으로 둔다
        if (x <= 0 || y <= 0 || x >= RawSquidHash.WorldCells || y >= RawSquidHash.WorldCells)
        {
            x = FallbackCoordinate;
            y = FallbackCoordinate;
        }
        bool buried = (old[ExtraOffset] & BuriedBit) != 0;
        if (!buried && (type.Flags2 & OriginalTypeRecord.BuildingMask) != 0 && _building is null)
        {
            throw new InvalidOperationException("Pop 건물 부착 공간이 연결되지 않았습니다");
        }
        uint vtable = Read(old, 0, 4);
        if (!OriginalPopDisplay.Supports(vtable, flags)
            && !(_postPop is not null && _postPop.HandlesDerived(sid) && OriginalUnpopDisplay.Supports(vtable, flags)))
        {
            throw new InvalidOperationException("Pop 파생 등록 효과를 아직 지원하지 않습니다");
        }
        int level = SpotRules.HashLevel(type.Flags2, frameWidth, frameHeight);
        ref ushort head = ref _hash.Bucket(level, x, y);
        if (head == sid || head >= _pool.Capacity)
        {
            throw new InvalidOperationException("Pop 해시 머리 오류");
        }
        bool writesSpots = !buried && ((type.Flags2 & OriginalTypeRecord.SpotWriteMask) != 0 || (type.Flags1 & OriginalTypeRecord.Surface) != 0);
        int right = (int)x;
        int bottom = (int)y;
        int left = right;
        int top = bottom;
        if (writesSpots)
        {
            if (type.FootX < 0 || type.FootY < 0 || type.FootX > RawSquidHash.WorldCells || type.FootY > RawSquidHash.WorldCells)
            {
                throw new InvalidOperationException("Pop 발자국 크기 오류");
            }
            left = right - type.FootX + 1;
            top = bottom - type.FootY + 1;
            if (left < 1 || top < 1)
            {
                throw new ArgumentOutOfRangeException(nameof(type), "Pop 지도 밖 발자국");
            }
        }
        _display?.Validate(old, flags);
        uint normalized = flags;
        if ((old[ExtraOffset] & FirstPopBit) == 0)
        {
            normalized |= FirstPopBits(old[ExtraOffset]);
        }
        if ((normalized & DeactivateBit) == 0)
        {
            normalized |= PostPopBit;
        }
        _postPop?.Validate(sid, normalized, new PopProjection(x, y, level, type));
        Span<byte> bytes = _pool.RawSlot(sid);
        Put(bytes, XOffset, BitConverter.SingleToUInt32Bits(x));
        Put(bytes, YOffset, BitConverter.SingleToUInt32Bits(y));
        Put(bytes, ScreenXOffset, (uint)((double)x * ScreenScaleX + ScreenBias), 2);
        Put(bytes, ScreenYOffset, (uint)((double)y * ScreenScaleY + ScreenBias), 2);
        // 섬·다리(매몰 아님)는 spot 등록 전에 새 위치를 미니맵 큐에 알린다
        if (!buried && (type.Flags2 & IslandOrBridge) != 0)
        {
            _metadisplay?.Enqueue(sid);
        }
        if (writesSpots)
        {
            // 겹치면 그때까지 쓴 좌표·spot·옆면·미니맵 알림을 되돌리지 않고 Overlap 으로 돌려준다
            for (int cy = top; cy <= bottom; cy++)
            {
                // 행 안의 칸을 왼쪽부터 등록한다
                for (int cx = left; cx <= right; cx++)
                {
                    byte genus = (byte)SpotRules.EffectiveGenus(type.Flags2, x, y, type.FootX, type.FootY, cx, cy);
                    if (genus != 0)
                    {
                        if ((_spots[cy * RawSquidHash.WorldCells + cx] & genus) != 0)
                        {
                            return RawPopResult.Overlap;
                        }
                        _spots[cy * RawSquidHash.WorldCells + cx] |= genus;
                    }
                    // 건물군이면 발자국의 왼쪽·오른쪽 가장자리 열(맨 위·맨 아래 행 제외)에서 건물 옆면도 함께 쓴다
                    if ((type.Flags2 & OriginalTypeRecord.BuildingMask) != 0 && (cx == left || cx == right) && cy != top && cy != bottom)
                    {
                        _building!.PopSide(cx, cy, x, y);
                    }
                }
            }
        }
        bytes[LevelOffset] = (byte)level;
        Put(bytes, RawSidPool.NextOffset, head, 2);
        head = (ushort)sid;
        if (level == 0 && (type.Flags2 & BridgeBit) == 0)
        {
            Put(bytes, IslandOffset, InvalidIsland, 2);
        }
        // 머리를 연결한 뒤, 최초 Pop·void 해제 전에 이전 위치의 표시를 갱신한다
        _display?.Update(bytes, flags);
        if ((bytes[ExtraOffset] & FirstPopBit) == 0)
        {
            flags |= FirstPopBits(bytes[ExtraOffset]);
            bytes[ExtraOffset] |= FirstPopBit;
        }
        bytes[RawSidPool.StateOffset] &= unchecked((byte)~RawSidPool.VoidBit);
        if ((flags & DeactivateBit) == 0)
        {
            flags |= PostPopBit;
        }
        _postPop?.Activate(sid, flags);
        return RawPopResult.Registered;
    }

    /// <summary>
    /// 최초 Pop 의 공통 플래그를 추가 비트에서 읽는다(원본 004ad470와 Pop 안의 같은 계산).
    /// 추가 비트의 1·8 이 모두 꺼져 있으면 1, 아니면 1 비트(abstract)가 2, 8 비트(매립)가 4 를 더한다.
    /// </summary>
    /// <param name="extra">슬롯의 추가 비트 바이트.</param>
    private static uint FirstPopBits(byte extra) => (extra & 9) == 0 ? 1U : (((extra & 1) != 0 ? 2U : 0U) | ((extra & 8) != 0 ? 4U : 0U));

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 필요한 폭만큼 읽는다(이웃 필드를 읽지 않는다).</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수(1~4).</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 4의 배수가 아닌 위치(+14의 좌표 등)에도 안전하다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }
}
