using Netstorm.Core.Original;

namespace Netstorm.Client.Display;

/// <summary>
/// 실제 캡처가 읽는 현재 fixed16 배율과 TYPE 추가 기준점 옵션.
/// 근거: cpppj/src/client/RawShapeToBuffer.h 의 ShapeToBufferState. (R2d-2, 2026-10-29)
/// </summary>
public sealed class ShapeToBufferState
{
    /// <summary>현재 fixed16 배율. 패치 0059a918·CD 00565c30이며 캡처 메타데이터에만 곱한다.</summary>
    public int Zoom { get; set; } = 65536;

    /// <summary>TYPE 추가 기준점 보정 켜기. 패치 00542414·CD 005207b0이 마지막 기준점 보정을 켠다.</summary>
    public bool AddTypeHotspot { get; set; }

    /// <summary>현재 타입별 색표 토큰. 색표가 있으면 실제 mapped 경로가 필요하다. 비어 있으면 소유자 경로는 미연결 진단이다.</summary>
    public uint[] OwnerTypeMaps { get; set; } = [];

    /// <summary>현재 타입·owner로 실제 색 번호를 선택한 256바이트 표. null이면 plain 경로를 쓴다.</summary>
    public Func<uint, uint, byte[]?>? OwnerColors { get; set; }

    /// <summary>extra&amp;2일 때 현재 파생의 실제 가상+78. -1이면 기본 그림을 생략한다.</summary>
    public Func<int, int>? MainFrame { get; set; }

    /// <summary>같은 캡처 버퍼에 그리는 가상+a4. TYPE 기준점 보정 전에 호출한다.</summary>
    public Action<int, ConstructionVisualImage, int, int>? Additional { get; set; }
}

/// <summary>
/// 실제 ShapeToBuffer의 메모리 캡처를 기존 VFX RLE 몸체와 보호 자산에 연결한다.
/// 근거: cpppj/src/client/RawShapeToBuffer.cpp (실제 plain/mapped·파생 가상 그림 캡처). (R2d-2, 2026-10-29)
/// 파생 분배는 현재 소비자가 공급한다. 세션 자산(SHP·TYPE 실자료) L1 대조는 R4에서 잇는다.
/// </summary>
public sealed class RawShapeToBuffer
{
    /// <summary>원본 해제 가상 표와 타입 maxHP의 두 진단 표식.</summary>
    private const uint FreedA = 0xddddddddU;

    /// <summary>원본 해제 가상 표와 타입 maxHP의 두 진단 표식.</summary>
    private const uint FreedB = 0xcdcdcdcdU;

    /// <summary>현재 TYPE/SHP 번호는 내부 Form을 제외한 자산 70부터 같은 로딩 순서여야 한다.</summary>
    private readonly int _assetCount;

    /// <summary>현재 raw 타입·frame·owner와 free·extra를 읽는 실제 SID 저장소.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 maxHP 표식·추가 기준점과 타입 번호 계약의 실제 TYPE 표.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>원본 loaded·frameCount 및 Squid 추가 헤더의 현재 자료.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;

    /// <summary>보호 SHP의 실제 RLE 스트림. 기존 DrawShape를 재사용한다.</summary>
    private readonly ShapeDatabase _database;

    /// <summary>매 캡처의 현재 배율·기준점 옵션·색표 토큰.</summary>
    private readonly ShapeToBufferState _state;

    /// <summary>
    /// 같은 판본·로딩 순서의 TYPE와 SHP를 읽기 전용으로 참조하며 창이나 장치를 만들지 않는다.
    /// </summary>
    /// <param name="pool">현재 raw를 읽는 실제 SID 저장소.</param>
    /// <param name="types">현재 TYPE 표. 판본별 개수(10.78은 188·CD는 171)와 같아야 한다.</param>
    /// <param name="shapes">현재 SHP 표시 자료. TYPE와 같은 개수·순서여야 한다.</param>
    /// <param name="database">보호 SHP의 실제 RLE 저장소.</param>
    /// <param name="state">매 캡처의 현재 배율·기준점·색표 옵션.</param>
    public RawShapeToBuffer(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<SquidDisplayShape> shapes, ShapeDatabase database, ShapeToBufferState state)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(shapes);
        ArgumentNullException.ThrowIfNull(database);
        ArgumentNullException.ThrowIfNull(state);
        bool patch = pool.Edition == OriginalEdition.Patch1078;
        _assetCount = patch ? 188 : 171;
        if (types.Count != _assetCount || shapes.Count != _assetCount)
        {
            throw new ArgumentException("ShapeToBuffer TYPE/SHP 판본 크기 오류", nameof(types));
        }
        _pool = pool;
        _types = types;
        _shapes = shapes;
        _database = database;
        _state = state;
    }

    /// <summary>
    /// 원본 fixed16의 곱셈 감김과 음수의 산술 시프트를 C# signed 넘침 없이 계산한다.
    /// </summary>
    /// <param name="value">헤더 치수·기준점.</param>
    private int Scale(int value)
    {
        long product = (long)value * _state.Zoom;
        int truncated = unchecked((int)(uint)product);
        // 음수는 -((-값+65535)/65536)으로 아래쪽 산술 시프트와 같게 한다
        return truncated >= 0 ? truncated / 65536 : (int)-((-(long)truncated + 65535) / 65536);
    }

    /// <summary>
    /// 현재 raw의 비정렬 DWORD를 호스트 정렬과 무관하게 읽는다.
    /// </summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽을 위치.</param>
    private static uint BufferDword(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 원본 low byte부터 네 바이트를 조합한다
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)raw[offset + i] << (i * 8);
        }
        return value;
    }

    /// <summary>
    /// Sprite source의 현재 프레임은 그림자 레이어가 아닌 헤더·물리표의 실제 번호다.
    /// 현재 frame 헤더→투명 버퍼 확보→실제 plain RLE→선택 TYPE 기준점의 원본 순서다.
    /// CD의 invalid 현재 객체·frame은 8×8·4×4 투명 그림으로 반환한다.
    /// </summary>
    /// <param name="sid">캡처할 객체 번호.</param>
    public ConstructionVisualImage Capture(int sid)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint type = raw[RawSidPool.TypeOffset];
        if (type < OriginalConstructors.FirstAssetType || type >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "ShapeToBuffer 현재 자산 타입 오류");
        }
        uint vtable = BufferDword(raw, 0);
        int frame = patch ? unchecked((int)BufferDword(raw, 36)) : raw[34];
        SquidDisplayShape shape = _shapes[(int)type];
        bool valid = (raw[RawSidPool.StateOffset] & 1) == 0 && vtable != 0 && vtable != FreedA && vtable != FreedB
            && frame >= 0 && frame < shape.FrameCount && shape.Loaded;
        if (!valid)
        {
            if (patch)
            {
                throw new InvalidOperationException("ShapeToBuffer 현재 source/frame 원본 진단");
            }
            return new ConstructionVisualImage { Width = 8, Height = 8, HotspotX = 4, HotspotY = 4, Pixels = new byte[64] };
        }
        OriginalTypeRecord record = _types[(int)type];
        if (patch && (unchecked((uint)record.MaxHitPoints) == FreedA || unchecked((uint)record.MaxHitPoints) == FreedB))
        {
            throw new InvalidOperationException("ShapeToBuffer frameCheck 원본 진단");
        }
        bool additional = (raw[patch ? 40 : 35] & 2) != 0;
        if (additional && (_state.MainFrame is null || _state.Additional is null))
        {
            throw new InvalidOperationException("ShapeToBuffer 추가 가상 그림 미연결");
        }
        uint owner = raw[patch ? 34 : 32];
        byte[]? colors = null;
        // mapped일 때만 현재 실제 소유자 색표를 선택한다
        if (owner != 0)
        {
            if (type >= (uint)_state.OwnerTypeMaps.Length)
            {
                throw new InvalidOperationException("ShapeToBuffer 현재 소유자 색표 미연결");
            }
            if (_state.OwnerTypeMaps[type] != 0)
            {
                if (_state.OwnerColors is null)
                {
                    throw new InvalidOperationException("ShapeToBuffer mapped 소유자 색 소비자 미연결");
                }
                colors = _state.OwnerColors(type, owner);
                if (colors is null || colors.Length != 256)
                {
                    throw new InvalidOperationException("ShapeToBuffer mapped 소유자 색표 크기 오류");
                }
            }
        }
        if (frame >= shape.Frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "ShapeToBuffer 실제 프레임 헤더 오류");
        }
        SquidDisplayFrame header = shape.Frames[frame];
        var image = new ConstructionVisualImage
        {
            Width = Scale(header.Width),
            Height = Scale(header.Height),
            HotspotX = Scale(header.HotspotX),
            HotspotY = Scale(header.HotspotY),
        };
        long size = (long)image.Width * image.Height;
        if (image.Width < 0 || image.Height < 0 || size > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "ShapeToBuffer 버퍼 크기 오류");
        }
        image.Pixels = new byte[size];
        Array.Fill(image.Pixels, (byte)255);
        int main = additional ? _state.MainFrame!(sid) : frame;
        // 캡처는 새 픽셀 버퍼 전체를 pane으로 사용하며 그리기는 메타 배율로 RLE를 다시 늘리지 않는다
        if (main != -1)
        {
            // 원본 mapped 추가 분기는 가상 main 대신 현재 frame을 쓴다. 이 비대칭을 수정하지 않는다
            if (colors is null)
            {
                VfxDraw.DrawShape(image.Pixels, image.Width, image.Height, _database,
                    (int)type - OriginalConstructors.FirstAssetType, main,
                    image.HotspotX, image.HotspotY, new ShapeRect(0, 0, image.Width - 1, image.Height - 1));
            }
            else
            {
                VfxMappedDraw.DrawMappedShape(image.Pixels, image.Width, image.Height, _database,
                    (int)type - OriginalConstructors.FirstAssetType, frame,
                    image.HotspotX, image.HotspotY, new ShapeRect(0, 0, image.Width - 1, image.Height - 1), colors);
            }
        }
        if (additional)
        {
            _state.Additional!(sid, image, image.HotspotX, image.HotspotY);
        }
        if (_state.AddTypeHotspot)
        {
            image.HotspotX = unchecked(image.HotspotX + Scale(record.HotspotX));
            image.HotspotY = unchecked(image.HotspotY + Scale(record.HotspotY));
        }
        return image;
    }

    /// <summary>
    /// 같은 실제 피해·건설 캡처 어댑터가 다른 월드의 풀을 혼용하지 않도록 확인한다.
    /// </summary>
    public RawSidPool Pool() => _pool;
}
