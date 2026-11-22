using System.Runtime.InteropServices;
using Netstorm.Core.Original;

namespace Netstorm.Client.Display; // 화면 교체와 독립적인 최초 ColorSort 입력/출력을 소유한다.

/// <summary>Source ClientColorSortInput의 고정 RGB·primary·9표·현재 소유자 표 수명이다.</summary>
public sealed partial class ClientColorSortInput
{
    /// <summary>주소를 바꾸지 않는 최초 RGBX256개다.</summary>
    private readonly uint[] _logical = new uint[256];
    /// <summary>정상 생성한 실제 raw primary만 발행한다.</summary>
    private RawColorSortPrimary? _primary;
    /// <summary>최초 캡처 또는 명시 재캡처 여부다.</summary>
    public bool Captured { get; private set; }
    /// <summary>실제 캐시 읽기 또는 전체 계산의 완료 여부다.</summary>
    public bool MapsReady { get; private set; }
    /// <summary>캡처나 화면 교체에서 새로 만들지 않는 같은 TYPE 색표 owner다.</summary>
    public OwnerColorTableState OwnerTables { get; } = new();
    /// <summary>메모리 순서의 같은9개256BYTE 출력이다.</summary>
    private readonly byte[][] _maps = Enumerable.Range(0, 9).Select(_ => new byte[256]).ToArray();
    /// <summary>캐시 부모가 채우는 같은8×256BYTE 명도 표다.</summary>
    private readonly byte[] _ownerHighlight = new byte[2048];
    /// <summary>캐시 부모가 채우는 같은8×256BYTE 고정색 표다.</summary>
    private readonly byte[] _ownerSolid = new byte[2048];
    /// <summary>미캡처를 현재 화면 RGB로 대신하지 않는다.</summary>
    public ReadOnlyMemory<uint> Logical => Captured ? _logical : throw new InvalidOperationException("ColorSort 최초 논리RGB 미캡처");
    /// <summary>미생성을 지연 생성으로 바꾸지 않는다.</summary>
    public RawColorSortPrimary Primary => _primary is { Constructed: true } ? _primary : throw new InvalidOperationException("ColorSort 실제primary 미연결");
    /// <summary>실제 준비 뒤 같은 배열을 빌린다.</summary>
    public byte[][] Maps => MapsReady ? _maps : throw new InvalidOperationException("ColorSort 실제9표 미생성");
    /// <summary>준비된 같은 소유자 명도 표다.</summary>
    public ReadOnlyMemory<byte> OwnerHighlight { get { _ = Maps; return _ownerHighlight; } }
    /// <summary>준비된 같은 소유자 고정색 표다.</summary>
    public ReadOnlyMemory<byte> OwnerSolid { get { _ = Maps; return _ownerSolid; } }
    /// <summary>명시 독립 logical 캡처는 이전 primary 연결을 끊는다.</summary>
    public void Capture(ReadOnlySpan<uint> logical)
    {
        if (logical.Length != 256) throw new ArgumentException("ColorSort RGBX256 계약", nameof(logical));
        _primary = null; logical.CopyTo(_logical); Captured = true; MapsReady = false;
    }
    /// <summary>원문 malloc의 미작성 padding을 현재 실제 할당에서 읽는다.</summary>
    public void CaptureWithPrimary(ReadOnlySpan<uint> logical)
    {
        byte[] allocation = new byte[RawColorSortPrimary.TotalBytes]; ClientAllocationBytes.ReadCurrent(allocation); CaptureWithPrimary(logical, allocation);
    }
    /// <summary>같은 RGB를 먼저 복사하고 정상 생성 뒤에만 새 primary를 공개한다.</summary>
    public void CaptureWithPrimary(ReadOnlySpan<uint> logical, byte[] allocation)
    {
        if (logical.Length != 256) throw new ArgumentException("ColorSort RGBX256 계약", nameof(logical));
        _primary = null; Captured = false; MapsReady = false; logical.CopyTo(_logical);
        var primary = new RawColorSortPrimary(allocation); primary.Construct(MemoryMarshal.AsBytes(_logical.AsSpan()).ToArray()); _primary = primary; Captured = true;
    }
    /// <summary>Client가 없는 독립 소비자만 명시 현재 입력을 쓰며 잘못된 고정 폭은 거부한다.</summary>
    public static ReadOnlySpan<uint> LogicalInput(ReadOnlySpan<uint> fixedInput, ReadOnlySpan<uint> independentCurrent)
    {
        if (fixedInput.IsEmpty) return independentCurrent;
        if (fixedInput.Length != 256) throw new ArgumentException("ColorSort 고정 RGBX256 계약", nameof(fixedInput));
        return fixedInput;
    }
    /// <summary>최초 고정 RGB와 현재 빨강의 lowBYTE로 전체9표를 같은 배열에 재작성한다.</summary>
    public byte[][] Rebuild(uint currentRed)
    {
        var logical = Logical.Span; var sprite = SpriteColorMapBuilder.BuildSpriteColorMaps(logical); var preview = PreviewColorMapBuilder.BuildPreviewColorMaps(logical, currentRed);
        preview.Destroy.CopyTo(_maps[1], 0); preview.Illegal.CopyTo(_maps[2], 0); sprite.Bright.CopyTo(_maps[4], 0); preview.LabelShadow.CopyTo(_maps[5], 0); sprite.Dead.CopyTo(_maps[6], 0); sprite.Shadow.CopyTo(_maps[7], 0);
        const double normalize = 1.0 / 255.0, scale = 1.600000023841858, offset = 0.20000000298023224, third = 0.3333333333333333; // 원문 저장 double 상수다.
        double[] red = new double[256], green = new double[256], blue = new double[256], light = new double[256];
        // 원본 번호순 정규화와 저장 명도를 만든다.
        for (int index = 0; index < 256; index++)
        {
            red[index] = (logical[index] & 255) * normalize; green[index] = ((logical[index] >> 8) & 255) * normalize; blue[index] = ((logical[index] >> 16) & 255) * normalize;
            light[index] = (Math.Max(Math.Max(red[index], green[index]), blue[index]) + Math.Min(Math.Min(red[index], green[index]), blue[index])) * 0.5;
        }
        // RGB 거리의 최소값만 float로 좁히며 명도 최소값은 double로 유지한다.
        for (int index = 0; index < 256; index++)
        {
            double targetRed = red[index] * scale, targetGreen = green[index] * scale, targetBlue = blue[index] * scale, targetLight = (light[index] + offset) * third;
            float minimum = 1e6f; double lightMinimum = 1e6; byte selected = 0;
            // 동률의 첫 후보와 실제 저장 순서를 보존한다.
            for (int candidate = 0; candidate < 256; candidate++)
            {
                double distance = Math.Abs(targetRed - red[candidate]) + Math.Abs(targetGreen - green[candidate]) + Math.Abs(targetBlue - blue[candidate]);
                if (distance < minimum) { minimum = (float)distance; _maps[0][index] = (byte)candidate; }
                double lightDistance = Math.Abs(targetLight - light[candidate]);
                if (lightDistance < lightMinimum) { lightMinimum = lightDistance; selected = (byte)candidate; }
            }
            _maps[8][index] = selected; _maps[3][index] = (byte)index;
        }
        // 원문 꼬리의 예약 번호 덮어쓰기다.
        for (int index = 228; index <= 245; index++) _maps[0][index] = (byte)index;
        _maps[0][254] = 254; _maps[0][255] = 255; _maps[8][254] = 254; _maps[8][255] = 255; MapsReady = true; return _maps;
    }
    /// <summary>실제 파일 장치를 원문 파일 순서의 같은9표에 연결한다. 호출자는 현재 primary를 먼저 생성해야 한다.</summary>
    public void InitializeCache(ColorMapCacheHooks hooks, Memory<byte> reader, Memory<byte> writer, uint currentRed)
    {
        _ = Primary;
        if (MapsReady || hooks.Owners is null) throw new InvalidOperationException("색표 캐시 최초 초기화 계약");
        var globals = new ColorMapCacheGlobals { Primary = () => Primary.Bytes, OwnerColors = new uint[] { 84, 249, 184, 140, 150, 112, 156, 222 }, OwnerHighlight = _ownerHighlight, OwnerSolid = _ownerSolid, Maps = new Memory<byte>[] { _maps[2], _maps[7], _maps[6], _maps[4], _maps[3], _maps[5], _maps[1], _maps[0], _maps[8] } };
        hooks.BuildMaps = () => Rebuild(currentRed); RawColorMapCache.Initialize(globals, hooks, reader, writer); MapsReady = true;
    }
}
