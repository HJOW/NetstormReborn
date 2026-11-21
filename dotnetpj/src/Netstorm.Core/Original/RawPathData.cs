// 같은 경로 이미지와 공유 패킷 버퍼의 원문 필드 폭/부분 쓰기를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>0052edce/cf·00544c50의 현재 메시지/프로토콜/초기 길이를 소유한다.</summary>
public sealed class RawPathDataHeader
{
    /// <summary>원문 최초30h이며 작성 시 현재 BYTE를 읽는다.</summary>
    public byte PacketId { get; set; } = 0x30;
    /// <summary>원문 최초1이며 작성 시 하위7비트만 쓴다.</summary>
    public byte PacketFlags { get; set; } = 1;
    /// <summary>최초BSS0인 DWORD이며 unsigned 최소4 후 낮은WORD를 쓴다.</summary>
    public uint PacketLength { get; set; }
}

/// <summary>실제 Read가 선택 위치에서만 빌리는 현재 의식점/TYPE/시계/예약/진단 장치다.</summary>
public sealed class RawPathDataReadHooks
{
    /// <summary>같은 현재 신전/Rune SHP에서 목표점 하나를 읽는다.</summary>
    public Func<int, uint, RawPathPoint>? StagePoint { get; set; }
    /// <summary>현재 부모의 speed/turningSpeed를 각 원문 조회 위치에서 읽는다.</summary>
    public Func<int, (float Speed, float TurningSpeed)>? Metadata { get; set; }
    /// <summary>Prepare 직전에 같은 게임의 현재 시각을 읽는다.</summary>
    public Func<double>? Now { get; set; }
    /// <summary>Prepare 뒤 실제 첫5105 Regular만 flags40으로 제거한다.</summary>
    public Action<int>? ClearAnimation { get; set; }
    /// <summary>같은 현재 로그에 원문 point/run 문장을 순서대로 전달한다.</summary>
    public Action<string>? Trace { get; set; }
    /// <summary>식/파일/줄의 원문 진단이며 정상 복귀 뒤 부분 처리를 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>Native 작성기의 현재 부모 부류/로그/진단 장치다.</summary>
public sealed class RawPathDataWriteHooks
{
    /// <summary>고정 prefix 작성 뒤 현재 packet+6 부모의 실제4ac200 부류를 읽는다.</summary>
    public Func<int, uint>? ParentGenus { get; set; }
    /// <summary>원문 count 진단이며 정상 복귀 뒤 선택된 실제 물리를 계속 읽는다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>각 point/run의 현재 실제 로그 장치다.</summary>
    public Action<string>? Trace { get; set; }
}

/// <summary>cpppj/src/o/RawPathData.cpp의 typed 작성/Native 작성/수신/송신 관찰 본체다.</summary>
public sealed partial class RawPathData
{
    /// <summary>방향/거리 또는 직접 좌표 payload의 실제 시작 오프셋이다.</summary>
    private const int Payload = 24;
    /// <summary>첫 double 시각을 원문 DWORD로 압축하는 배율이다.</summary>
    private const double TimeScale = 8.0;
    /// <summary>실제 이미지/필드 배치를 고정하는 판본이다.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>원본 판본만 고정하며 이미지/버퍼/상태 사본을 만들지 않는다.</summary>
    public RawPathData(OriginalEdition edition) => _edition = edition;
    /// <summary>선택한 실제 함수만 요구하며 누락을 빈 성공으로 대신하지 않는다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("경로 데이터의 선택된 실제 장치 미연결");
    /// <summary>선택한 실제 BYTE~QWORD를 little endian으로 읽는다.</summary>
    private static ulong Bits(ReadOnlySpan<byte> bytes, long offset, int width = 4)
    {
        if (offset < 0 || offset > bytes.Length || width < 1 || width > 8 || width > bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(offset), "경로 데이터 읽기 물리 범위");
        ulong value = 0;
        // 정렬/호스트 엔디언에 의존하지 않고 선택 폭만 조합한다.
        for (int index = 0; index < width; index++) value |= (ulong)bytes[(int)offset + index] << (index * 8);
        return value;
    }
    /// <summary>선택한 실제 쓰기만 경계에서 검사하고 앞선 prefix를 되돌리지 않는다.</summary>
    private static void Put(Span<byte> bytes, long offset, ulong value, int width = 1)
    {
        if (offset < 0 || offset > bytes.Length || width < 1 || width > 8 || width > bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(offset), "경로 데이터 쓰기 물리 범위");
        // 반환 길이 밖의 stale buffer와 미작성 여백을 지우지 않는다.
        for (int index = 0; index < width; index++) bytes[(int)offset + index] = unchecked((byte)(value >> (index * 8)));
    }
    /// <summary>typed writer의 유효 signed32 절삭 계약이다. Native의 masked signed64와 구별한다.</summary>
    private static int Truncate(double value)
    {
        double result = Math.Truncate(value);
        if (!double.IsFinite(result) || result < int.MinValue || result > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(value), "경로 typed _ftol 범위");
        return (int)result;
    }
    /// <summary>실제 float 좌표를 typed signed32 절삭으로 읽는다.</summary>
    private static int Coordinate(ReadOnlySpan<byte> image, int offset) => Truncate(BitConverter.UInt32BitsToSingle((uint)Bits(image, offset)));
    /// <summary>현재 Runtime 이미지에 같은 typed writer를 직접 적용한다.</summary>
    public int Write(RawPathRuntime runtime, ushort form, ushort parent, uint parentGenus, RawPathDataHeader header, Span<byte> buffer)
        => Write(runtime.Bytes.Span, form, parent, parentGenus, header, buffer);
    /// <summary>두 판본의 실제 값 작성기다. 부모 변경/force로 경로 필드를 선별하지 않는다.</summary>
    public int Write(ReadOnlySpan<byte> image, ushort form, ushort parent, uint parentGenus, RawPathDataHeader header, Span<byte> buffer)
    {
        RawPathLayout layout = RawPathRuntime.LayoutFor(_edition);
        if (image.Length != layout.Size || buffer.Length < Payload) throw new ArgumentException("경로 typed 이미지/버퍼 크기");
        Put(buffer, 1, header.PacketId); Put(buffer, 0, (uint)(header.PacketFlags & 0x7f)); Put(buffer, 2, Math.Max(header.PacketLength, 4u), 2);
        Put(buffer, 4, form, 2); Put(buffer, 6, parent, 2); int count = unchecked((int)Bits(image, 0x10));
        if (count < 1 || count > layout.Capacity) throw new ArgumentOutOfRangeException(nameof(image), "경로 typed i < nVert");
        int x = Coordinate(image, 0x14), y = Coordinate(image, 0x18); Put(buffer, 15, unchecked((uint)x)); Put(buffer, 16, unchecked((uint)y));
        Put(buffer, 17, unchecked((uint)Truncate(BitConverter.UInt64BitsToDouble(Bits(image, layout.Times, 8)) * TimeScale)), 4);
        Put(buffer, 11, Bits(image, layout.Helper, 2), 2); int tail = _edition == OriginalEdition.Patch1078 ? 0x186e4 : 0x2f1c;
        Put(buffer, 21, Bits(image, tail, 2), 2); Put(buffer, 23, Bits(image, tail + 4, 1)); Put(buffer, 13, 0, 2); int written = 0;
        // 실제 부모 부류가 정한 두 payload 형식 중 하나만 작성한다.
        for (int index = 1; index < count; index++)
        {
            int nextX = Coordinate(image, 0x14 + index * 8), nextY = Coordinate(image, 0x18 + index * 8);
            if ((parentGenus & 0x120000u) != 0)
            {
                Put(buffer, Payload + written, unchecked((uint)nextX)); written++; Put(buffer, 13, (uint)written, 2);
                Put(buffer, Payload + written, unchecked((uint)nextY)); written++; Put(buffer, 13, (uint)written, 2);
            }
            else if (x != nextX || y != nextY)
            {
                uint direction = (uint)Bits(image, layout.Directions + index * 4); long distance = Math.Abs((long)x - nextX);
                if ((direction & 1) == 0) distance += Math.Abs((long)y - nextY);
                if (distance > int.MaxValue) throw new ArgumentOutOfRangeException(nameof(image), "경로 typed 거리 범위");
                // 원문처럼31칸씩 분할하고 각 BYTE 저장 뒤 현재 길이WORD를 갱신한다.
                while (distance > 0)
                {
                    long step = Math.Min(distance, 31); Put(buffer, Payload + written, ((ulong)step << 3) | (direction & 7)); written++; Put(buffer, 13, (uint)written, 2); distance -= step;
                }
            }
            x = nextX; y = nextY;
        }
        if (written > short.MaxValue) throw new ArgumentOutOfRangeException(nameof(buffer), "경로 typed signed 길이 범위");
        return written + Payload;
    }
    /// <summary>40a660은 송신에서만 signed nodeCount 진단 뒤 현재WORD+24로 낮은 길이WORD를 쓴다.</summary>
    public static void Observe(Span<byte> packet, uint sending, Action<string, string, uint>? assertion)
    {
        if (sending == 0) return;
        if (packet.Length < 15) throw new ArgumentOutOfRangeException(nameof(packet), "경로 관찰 nodeCount 물리 부족");
        if (unchecked((short)Bits(packet, 13, 2)) == -1) Need(assertion)("nodeCount != (short)-1", @"\Ns\O\_p_packets.cpp", 0x716);
        ushort length = unchecked((ushort)(unchecked((short)Bits(packet, 13, 2)) + Payload)); Put(packet, 2, length, 2);
    }
}
