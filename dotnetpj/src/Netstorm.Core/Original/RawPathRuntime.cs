// 원본 경로의 필드 폭과 시간 계산을 같은 메모리 이미지에서 보존한다.
namespace Netstorm.Core.Original;

/// <summary>원본 경로 배열이 저장하는 두 float 좌표다.</summary>
public struct RawPathPoint
{
    /// <summary>기존 Reset/실패 좌표 호출자가 직접 바꾸는 가로 float다.</summary>
    public float X;
    /// <summary>기존 Reset/실패 좌표 호출자가 직접 바꾸는 세로 float다.</summary>
    public float Y;
    /// <summary>좌표 두 값을 원문 float 폭으로 저장한다. 기본 생성/필드 쓰기도 유지한다.</summary>
    public RawPathPoint(float x, float y) { X = x; Y = y; }
}

/// <summary>Step의 반환 위치/각도/도착 여부이며 저장소의 별도 finished 필드와 구분한다.</summary>
/// <param name="Position">이번 호출에서 반환한 현재 위치다.</param>
/// <param name="Angle">이번 호출에서 반환한 각도다.</param>
/// <param name="Finished">마지막 꼭짓점 도착 여부다.</param>
public readonly record struct RawPathSample(RawPathPoint Position, float Angle, bool Finished);

/// <summary>원문 Step이 갱신하는 관찰 전역이며 게임 시계와 구별한다.</summary>
public sealed class RawPathObservation
{
    /// <summary>실제 진행 분기가 OR하는 활성 비트다.</summary>
    public uint Flags { get; set; }
    /// <summary>PATCH의0 또는 CD의 외부 관찰 시계 비트다.</summary>
    public ulong Stamp { get; set; }
}

/// <summary>판본별 배열/끝 필드/원문 가상 표의 물리 위치다.</summary>
/// <param name="Capacity">꼭짓점 배열 용량이다.</param>
/// <param name="Size">전체 객체 바이트 크기다.</param>
/// <param name="Times">도착 시각 배열 오프셋이다.</param>
/// <param name="Directions">방향 배열 오프셋이다.</param>
/// <param name="NextFrame">다음 실행 시각 오프셋이다.</param>
/// <param name="Current">현재 단계 오프셋이다.</param>
/// <param name="Angle">현재 각도 오프셋이다.</param>
/// <param name="Finished">별도 도착 표식 오프셋이다.</param>
/// <param name="TargetA">첫 도착 대상 오프셋이다.</param>
/// <param name="TargetB">둘째 도착 대상 오프셋이다.</param>
/// <param name="Helper">정리 보조 객체 오프셋이다.</param>
/// <param name="DeadFlag">자기 삭제 관찰 토큰 오프셋이다.</param>
/// <param name="BaseVtable">기본 경로 가상 표 주소다.</param>
/// <param name="TeleportVtable">텔레포트 경로 가상 표 주소다.</param>
/// <param name="PriestVtable">사제 경로 가상 표 주소다.</param>
public readonly record struct RawPathLayout(int Capacity, int Size, int Times, int Directions, int NextFrame,
    int Current, int Angle, int Finished, int TargetA, int TargetB, int Helper, int DeadFlag,
    uint BaseVtable, uint TeleportVtable, uint PriestVtable);

/// <summary>RawPathRuntime.cpp의 전체 이미지/생성/Prepare/Step이다. 경로 탐색을 직선으로 대신하지 않는다.</summary>
public sealed class RawPathRuntime
{
    /// <summary>원본 방향 한 단계의 각도다.</summary>
    private const float DirectionDegrees = 45.0f;
    /// <summary>보간 각도를 보정하는 한 바퀴다.</summary>
    private const float FullTurn = 360.0f;
    /// <summary>서버 시작 시각을 그대로 선택하는 지연 경계다.</summary>
    private const double LateStart = 2.0;
    /// <summary>좌표/시간/방향/꼬리를 함께 소유하는 유일한 raw 이미지다.</summary>
    private readonly byte[] _bytes;
    /// <summary>생성 때 고정한 원본 판본이다.</summary>
    public OriginalEdition Edition { get; }
    /// <summary>현재 판본의 실제 물리 배치다.</summary>
    public RawPathLayout Layout { get; }
    /// <summary>독립 관측과 네트워크 writer가 읽는 같은 전체 이미지다.</summary>
    public ReadOnlyMemory<byte> Bytes => _bytes;
    /// <summary>signed DWORD 꼭짓점 수다.</summary>
    public int Count => unchecked((int)ReadBits(0x10));
    /// <summary>signed DWORD 현재 단계다.</summary>
    public int Current => unchecked((int)ReadBits(Layout.Current));
    /// <summary>마지막 보간 분기가 저장한 float 각도다.</summary>
    public float Angle => FloatAt(Layout.Angle);
    /// <summary>도착 가상 호출이 별도로 쓰는 완료 표식이다.</summary>
    public bool Finished => ReadBits(Layout.Finished) != 0;
    /// <summary>비행 프레임이 읽고 쓰는 실제 QWORD 예약 시각이다.</summary>
    public double NextFrame { get => DoubleAt(Layout.NextFrame); set => WriteBits(Layout.NextFrame, BitConverter.DoubleToUInt64Bits(value), 8); }
    /// <summary>가상 도착 첫 인자의 현재 DWORD다.</summary>
    public uint TargetA => (uint)ReadBits(Layout.TargetA);
    /// <summary>가상 도착 둘째 인자의 현재 DWORD다.</summary>
    public uint TargetB => (uint)ReadBits(Layout.TargetB);
    /// <summary>cleanup이 읽는 보조 객체 DWORD다.</summary>
    public uint Helper => (uint)ReadBits(Layout.Helper);
    /// <summary>같은 helper+4의 signed 이동 회피 깊이다.</summary>
    public int MoveAwayDepth { get => unchecked((int)ReadBits(Layout.Helper + 4)); set => WriteBits(Layout.Helper + 4, unchecked((uint)value)); }
    /// <summary>원문 지역 watch의 활성 의미를 보존하는 비주소 토큰이다.</summary>
    public uint DeathWatchToken { get => (uint)ReadBits(Layout.DeadFlag); set => WriteBits(Layout.DeadFlag, value); }

    /// <summary>PATCH5000/CD600 꼭짓점과 실제 끝 필드를 선택한다.</summary>
    public static RawPathLayout LayoutFor(OriginalEdition edition) => edition == OriginalEdition.Patch1078
        ? new(5000, 0x186f0, 0x9c58, 0x13898, 0x186b8, 0x186c0, 0x186c4, 0x186c8, 0x186cc, 0x186d0, 0x186d4, 0x186ec, 0x50df64, 0x50e284, 0x50e458)
        : new(600, 0x2f28, 0x12d8, 0x2598, 0x2ef8, 0x2f00, 0x2f04, 0x2f08, 0x2f0c, 0x2f10, 0x2f14, 0x2f24, 0x503d28, 0x503d60, 0x503eb0);

    /// <summary>원문 생성자가 지정한 필드와 시간 배열만 초기화하고 heapFill/padding을 보존한다.</summary>
    public RawPathRuntime(OriginalEdition edition, bool priest = false, byte heapFill = 0, bool flying = false,
        bool worm = false, bool balloon = false, bool dude = false, bool bulf = false, bool walker = false)
    {
        Edition = edition; Layout = LayoutFor(edition); _bytes = new byte[Layout.Size]; _bytes.AsSpan().Fill(heapFill);
        WriteBits(0, priest ? Layout.PriestVtable : Layout.TeleportVtable); WriteBits(4, 0); WriteBits(8, 0); WriteBits(0x10, 0);
        if (flying) { if (edition != OriginalEdition.Patch1078 || priest) throw new ArgumentException("비행39 생성자 판본/종류"); WriteBits(0, 0x50e2ec); }
        if (worm) { if (edition != OriginalEdition.Patch1078 || priest || flying) throw new ArgumentException("웜40 생성자 판본/종류"); WriteBits(0, 0x50e4c4); }
        if (balloon) { if (edition != OriginalEdition.Patch1078 || priest || flying || worm) throw new ArgumentException("벌룬42 생성자 판본/종류"); WriteBits(0, 0x50e2b8); }
        if (dude) { if (edition != OriginalEdition.Patch1078 || priest || flying || worm || balloon) throw new ArgumentException("Dude24 생성자 판본/종류"); WriteBits(0, 0x50e3d8); }
        if (bulf) { if (edition != OriginalEdition.Patch1078 || priest || flying || worm || balloon || dude) throw new ArgumentException("벌프41 생성자 판본/종류"); WriteBits(0, 0x50e354); }
        if (walker) { if (edition != OriginalEdition.Patch1078 || priest || flying || worm || balloon || dude || bulf) throw new ArgumentException("워커38 생성자 판본/종류"); WriteBits(0, 0x50e320); }
        _bytes.AsSpan(Layout.Times, Layout.Capacity * 8).Clear(); WriteBits(Layout.NextFrame, 0, 8);
        // 원문 생성자의 개별 DWORD 저장만 적용하고 중간 빈 영역은 남긴다.
        foreach (int offset in new[] { Layout.Current, Layout.Angle, Layout.Finished, Layout.TargetA, Layout.TargetB, Layout.Helper, Layout.Helper + 4, Layout.DeadFlag }) WriteBits(offset, 0);
        int tail = edition == OriginalEdition.Patch1078 ? 0x186e4 : 0x2f1c; WriteBits(tail, 0); WriteBits(tail + 4, 0);
    }

    /// <summary>선택된 실제 이미지 폭만 little endian으로 읽는다. Data decoder가 같은 저장소를 사용한다.</summary>
    internal ulong ReadBits(int offset, int width = 4)
    {
        if (offset < 0 || offset > _bytes.Length || width < 1 || width > 8 || width > _bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(offset), "경로 raw 읽기 물리 범위");
        ulong value = 0;
        // 정렬이나 호스트 엔디언에 의존하지 않고 낮은 바이트부터 조합한다.
        for (int index = 0; index < width; index++) value |= (ulong)_bytes[offset + index] << (index * 8);
        return value;
    }
    /// <summary>선택된 실제 이미지 폭만 저장하고 뒤 실패에서 앞선 저장을 되돌리지 않는다.</summary>
    internal void WriteBits(int offset, ulong value, int width = 4)
    {
        if (offset < 0 || offset > _bytes.Length || width < 1 || width > 8 || width > _bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(offset), "경로 raw 쓰기 물리 범위");
        // 원문 BYTE/WORD/DWORD/QWORD의 지정 폭만 바꾼다.
        for (int index = 0; index < width; index++) _bytes[offset + index] = unchecked((byte)(value >> (index * 8)));
    }
    /// <summary>현재 DWORD를 float 비트로 해석한다.</summary>
    private float FloatAt(int offset) => BitConverter.UInt32BitsToSingle((uint)ReadBits(offset));
    /// <summary>현재 QWORD를 double 비트로 해석한다.</summary>
    private double DoubleAt(int offset) => BitConverter.UInt64BitsToDouble(ReadBits(offset, 8));
    /// <summary>현재 count에 대한 원문 꼭짓점 인덱스 계약을 확인한다.</summary>
    private void Check(int index)
    {
        if (index < 0 || index >= Count) throw new ArgumentOutOfRangeException(nameof(index), "경로 꼭짓점 범위");
    }
    /// <summary>현재 꼭짓점의 두 float를 읽는다.</summary>
    public RawPathPoint Point(int index) { Check(index); return new(FloatAt(checked(0x14 + index * 8)), FloatAt(checked(0x18 + index * 8))); }
    /// <summary>현재 꼭짓점 도착 시각의 원문 double을 읽는다.</summary>
    public double Time(int index) { Check(index); return DoubleAt(checked(Layout.Times + index * 8)); }
    /// <summary>현재 꼭짓점의 signed 방향 DWORD를 읽는다.</summary>
    public int Direction(int index) { Check(index); return unchecked((int)ReadBits(checked(Layout.Directions + index * 4))); }
    /// <summary>벌룬 고도 계산이 사용하는 count 밖 또는-1 인접 QWORD도 물리 범위 내에서 읽는다.</summary>
    public double PhysicalTime(int index)
    {
        if (index < -1 || index >= Layout.Capacity) throw new ArgumentOutOfRangeException(nameof(index), "경로 시각 물리 배열 범위");
        return DoubleAt(Layout.Times + index * 8);
    }
    /// <summary>이미 실제 탐색이 만든 꼭짓점/방향/선택 시각을 같은 배열에 저장한다.</summary>
    public void SetPath(ReadOnlySpan<RawPathPoint> points, ReadOnlySpan<int> directions, ReadOnlySpan<double> times = default)
    {
        if (points.Length != directions.Length || points.Length >= Layout.Capacity || (!times.IsEmpty && times.Length != points.Length)) throw new ArgumentException("경로 입력 크기");
        WriteBits(0x10, (uint)points.Length);
        // count 뒤 각 원문 point/direction/time 슬롯만 순서대로 쓴다.
        for (int index = 0; index < points.Length; index++)
        {
            WriteBits(0x14 + index * 8, BitConverter.SingleToUInt32Bits(points[index].X)); WriteBits(0x18 + index * 8, BitConverter.SingleToUInt32Bits(points[index].Y));
            WriteBits(Layout.Directions + index * 4, unchecked((uint)directions[index])); WriteBits(Layout.Times + index * 8, times.IsEmpty ? 0 : BitConverter.DoubleToUInt64Bits(times[index]), 8);
        }
    }
    /// <summary>방향 차의 양쪽 거리를 비교하고 동률180도는 양수를 선택한다.</summary>
    public static double DirectionDelta(int previous, int next)
    {
        long forward = (long)next - previous, backward = (long)previous - next;
        if (next < previous) forward += 8; if (previous < next) backward += 8;
        return (forward <= backward ? forward : -backward) * (double)DirectionDegrees;
    }
    /// <summary>늦은 시작/판본별 float 중간 저장을 보존하며 각 도착 double을 실제 배열에 기록한다.</summary>
    public void Prepare(double now, double startTime, float speed, float turningSpeed, int firstDirection)
    {
        if (Count < 1 || speed == 0 || turningSpeed == 0) throw new ArgumentException("경로 시각 입력");
        double time = now - startTime > LateStart ? startTime : now; WriteBits(Layout.Times, BitConverter.DoubleToUInt64Bits(time), 8); WriteBits(Layout.Directions, unchecked((uint)firstDirection));
        float inverseSpeed = (float)(1.0 / speed), inverseTurning = (float)(1.0 / turningSpeed);
        // 각 점마다 원문 double 저장을 마치고 다음 점의 시간 계산으로 이어진다.
        for (int index = 1; index < Count; index++)
        {
            RawPathPoint a = Point(index - 1), b = Point(index); double dx = (double)b.X - a.X, dy = (double)b.Y - a.Y;
            double distance = Math.Sqrt(dy * dy + dx * dx), delta = Math.Abs(DirectionDelta(Direction(index - 1), Direction(index)));
            time = Edition == OriginalEdition.Patch1078 ? (double)inverseTurning * (float)delta + distance * inverseSpeed + time
                : distance / speed + (float)(delta / turningSpeed) + time;
            WriteBits(Layout.Times + index * 8, BitConverter.DoubleToUInt64Bits(time), 8);
        }
    }
    /// <summary>한 호출에 한 꼭짓점만 전진하고 저장 angle/별도finished/반환 도착 여부를 구별한다.</summary>
    public RawPathSample Step(double now, RawPathPoint parent, RawPathObservation observation, Action? stepSound, ulong cdObservationStamp = 0)
    {
        RawPathPoint position = parent; float angle = Angle; bool finished = false; int current = Current;
        if (current >= Count)
        {
            if (current < 1 || current != Count) throw new ArgumentOutOfRangeException(nameof(current), "경로 종료 단계");
            return new(Point(current - 1), (float)Direction(current - 1) * DirectionDegrees, true);
        }
        observation.Flags |= 1; observation.Stamp = Edition == OriginalEdition.Patch1078 ? 0 : cdObservationStamp;
        RawPathPoint target = Point(current); double targetTime = Time(current);
        if (now < targetTime)
        {
            if (current != 0)
            {
                double previousTime = Time(current - 1); RawPathPoint previous = Point(current - 1);
                if (previousTime <= now)
                {
                    float scale = (float)((now - previousTime) / (targetTime - previousTime));
                    if (!(scale >= 0 && scale <= 1)) throw new ArgumentOutOfRangeException(nameof(now), "경로 보간 비율");
                    float basis = (float)Direction(current - 1) * DirectionDegrees, change = (float)(DirectionDelta(Direction(current - 1), Direction(current)) * scale);
                    double calculated = (double)basis + change; if (Edition == OriginalEdition.Cd1072) calculated = (float)calculated;
                    if (calculated < 0) calculated += FullTurn; else if (calculated >= FullTurn) calculated -= FullTurn;
                    angle = (float)calculated; WriteBits(Layout.Angle, BitConverter.SingleToUInt32Bits(angle));
                    position = new((float)(((double)target.X - previous.X) * scale + previous.X), (float)(((double)target.Y - previous.Y) * scale + previous.Y));
                }
            }
        }
        else
        {
            position = target; angle = (float)Direction(current) * DirectionDegrees; WriteBits(Layout.Current, unchecked((uint)(current + 1)));
            finished = current + 1 >= Count;
            if (!finished && ((current + 1) & 1) == 0) (stepSound ?? throw new InvalidOperationException("경로 발소리 실제 효과 미연결"))();
        }
        return new(position, angle, finished);
    }
    /// <summary>PATCH 비행 Init의 point/direction/time/count 쓰기를 수행한다. 기존 count를 지우지 않는다.</summary>
    public void InitializeFlight(RawPathPoint parent, RawPathPoint destination, int cruiseHeight = 0, byte currentHeight = 0)
    {
        if (Edition != OriginalEdition.Patch1078) throw new InvalidOperationException("CD 비행 Init 미복원");
        // 비행 append는 원문처럼 다음 한 칸을 남기고 count를 마지막에 증가시킨다.
        void Append(RawPathPoint point, int direction)
        {
            int index = Count; if (index < 0 || (long)index + 1 >= Layout.Capacity) throw new ArgumentOutOfRangeException(nameof(index), "비행 경로 append 범위");
            WriteBits(0x14 + index * 8, BitConverter.SingleToUInt32Bits(point.X)); WriteBits(0x18 + index * 8, BitConverter.SingleToUInt32Bits(point.Y));
            WriteBits(Layout.Times + index * 8, 0, 8); WriteBits(Layout.Directions + index * 4, unchecked((uint)direction)); WriteBits(0x10, unchecked((uint)index + 1));
        }
        const float heightToCell = 0.09090909361839294f; // 원본 ROM504194의 고도→칸 배율이다.
        Append(parent, 0); float rise = (float)((double)cruiseHeight * heightToCell);
        if ((int)currentHeight < unchecked((int)((uint)cruiseHeight - 1u))) Append(new(parent.X, (float)((double)parent.Y - ((double)rise - (double)currentHeight * heightToCell))), 0);
        if (cruiseHeight != 0) Append(new(destination.X, (float)((double)destination.Y - rise)), 0);
        Append(destination, RawSquidNeighbors.Direction(parent.X, parent.Y, destination.X, destination.Y)); ResetCurrent();
    }
    /// <summary>Init이 실제로 바꾸는 current DWORD만0으로 쓴다.</summary>
    public void ResetCurrent() => WriteBits(Layout.Current, 0);
    /// <summary>도착 본체가 관리하는 별도 finished DWORD만 쓴다.</summary>
    public void SetFinished(bool value) => WriteBits(Layout.Finished, value ? 1u : 0u);
    /// <summary>실제 현재 단계/각도/finished를 입력 이미지에서 이관한다.</summary>
    public void RestoreStep(int current, float angle, bool finished = false)
    {
        if (current < 0 || current > Count || !float.IsFinite(angle)) throw new ArgumentOutOfRangeException(nameof(current), "경로 상태 이관");
        WriteBits(Layout.Current, (uint)current); WriteBits(Layout.Angle, BitConverter.SingleToUInt32Bits(angle)); SetFinished(finished);
    }
    /// <summary>가상 도착 대상 두 DWORD만 쓴다.</summary>
    public void SetTargets(uint first, uint second) { WriteBits(Layout.TargetA, first); WriteBits(Layout.TargetB, second); }
    /// <summary>실제 명령 helper/꼬리DWORD들과 PATCH 목적지 두 float만 쓴다.</summary>
    public void SetCommandFields(uint helper, uint tailWord, uint tailByte, RawPathPoint destination)
    {
        WriteBits(Layout.Helper, helper); int tail = Edition == OriginalEdition.Patch1078 ? 0x186e4 : 0x2f1c;
        WriteBits(tail, tailWord); WriteBits(tail + 4, tailByte);
        if (Edition == OriginalEdition.Patch1078) { WriteBits(0x186dc, BitConverter.SingleToUInt32Bits(destination.X)); WriteBits(0x186e0, BitConverter.SingleToUInt32Bits(destination.Y)); }
    }
    /// <summary>SharedRegular 예약이 원문에서 지우는 helper 하나만0으로 쓴다.</summary>
    public void ClearHelper() => WriteBits(Layout.Helper, 0);
    /// <summary>현재 count 안의 좌표 쌍만 바꾸고 시각/방향은 보존한다.</summary>
    public void ReplacePoint(int index, RawPathPoint point)
    {
        Check(index); WriteBits(0x14 + index * 8, BitConverter.SingleToUInt32Bits(point.X)); WriteBits(0x18 + index * 8, BitConverter.SingleToUInt32Bits(point.Y));
    }
    /// <summary>실제 Host 부착이 확정한16비트 Form/부모를 원문 base DWORD에 저장한다.</summary>
    public void SetAttachment(ushort form, ushort parent) { WriteBits(4, form); WriteBits(8, parent); }
}
