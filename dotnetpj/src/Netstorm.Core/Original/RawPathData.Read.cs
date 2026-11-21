// 실제 수신은 기존 배열에 append하고 앞선 필드/로그/부분 효과를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>489a10의 현재 payload/의식점/TYPE/시계 조회 순서를 보존하는 수신 부분이다.</summary>
public sealed partial class RawPathData
{
    /// <summary>원문 시각DWORD를 double로 복원하는 배율이다.</summary>
    private const double ReadTimeScale = 0.125;
    /// <summary>의식 시작점을 첫 실제 좌표로 바꾸는 원문 거리 경계다.</summary>
    private const double StageSnapDistance = 1.5;
    /// <summary>보호 PE0052f83c의8방향 가로 DWORD다.</summary>
    private static readonly int[] StepX = [0, 1, 1, 1, 0, -1, -1, -1];
    /// <summary>보호 PE0052f85c의8방향 세로 DWORD다.</summary>
    private static readonly int[] StepY = [-1, -1, 0, 1, 1, 1, 0, -1];
    /// <summary>같은 PATCH Runtime/Pool/188TYPE의 실제 배열에 현재 패킷을 원문 순서로 복원한다.</summary>
    public void Read(RawPathRuntime runtime, ReadOnlyMemory<byte> packet, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPathDataReadHooks hooks)
    {
        if (_edition != OriginalEdition.Patch1078 || runtime.Edition != _edition || pool.Edition != _edition || types.Count != 188) throw new ArgumentException("경로 수신의 실제 PATCH 저장소/판본 불일치");
        if (packet.Length < Payload) throw new ArgumentOutOfRangeException(nameof(packet), "경로 수신24BYTE 머리 물리 부족");
        int parent = (ushort)Bits(packet.Span, 6, 2); if (parent >= pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "경로 수신 부모 물리 범위");
        uint x = packet.Span[15], y = packet.Span[16]; double startTime = unchecked((int)Bits(packet.Span, 17)) * ReadTimeScale;
        RawPathLayout layout = runtime.Layout; runtime.WriteBits(layout.Finished, 0); runtime.WriteBits(layout.TargetA, 0); runtime.WriteBits(layout.TargetB, 0);
        runtime.WriteBits(layout.Helper, Bits(packet.Span, 11, 2)); runtime.WriteBits(0x186e4, Bits(packet.Span, 21, 2)); runtime.WriteBits(0x186e8, packet.Span[23]);
        // 선택한 payload BYTE만 검사하며 뒤 입력 부족에서 앞선 append를 되돌리지 않는다.
        byte PayloadByte(int index)
        {
            if (index < 0 || index >= packet.Length - Payload) throw new ArgumentOutOfRangeException(nameof(packet), "경로 수신의 선택된 payload 부족");
            return packet.Span[Payload + index];
        }
        // 직접 좌표 루프는 각 조건에서 현재 signed WORD를 다시 읽는다.
        int NodeCount() => unchecked((short)Bits(packet.Span, 13, 2));
        // 원문 point/direction/time→count 증가→진단 순서로 같은 배열에 추가한다.
        void Append(RawPathPoint point, uint direction)
        {
            int index = runtime.Count; if (index < 0 || index >= layout.Capacity) throw new ArgumentOutOfRangeException(nameof(runtime), "경로 수신 append 실제 배열 범위");
            runtime.WriteBits(0x14 + index * 8, BitConverter.SingleToUInt32Bits(point.X)); runtime.WriteBits(0x18 + index * 8, BitConverter.SingleToUInt32Bits(point.Y));
            runtime.WriteBits(layout.Directions + index * 4, direction); runtime.WriteBits(layout.Times + index * 8, 0, 8); runtime.WriteBits(0x10, unchecked((uint)index + 1));
            if ((long)index + 1 >= layout.Capacity) Need(hooks.Assertion)("nVert < MAX_PATH_LENGTH", @"c:\ns\o\path.h", 0x16);
        }
        // 좌표 DWORD 감김을 signed %d로 해석한 원문 서식을 전달한다.
        void TracePoint(int index, uint px, uint py, uint direction) => Need(hooks.Trace)(FormattableString.Invariant($"[{index}] ({unchecked((int)px)},{unchecked((int)py)}) , {unchecked((int)direction)}\n"));
        // 좌표 DWORD 감김 뒤 signed int→float의 실제 저장 폭을 사용한다.
        RawPathPoint CurrentPoint() => new(unchecked((int)x), unchecked((int)y));
        int type = pool.Slot(parent)[10]; if (type >= types.Count) throw new ArgumentOutOfRangeException(nameof(types), "경로 수신 현재 부모TYPE 범위");
        uint firstDirection = 0;
        if ((types[type].Flags2 & 0x120000u) == 0)
        {
            int count = NodeCount(); firstDirection = count > 0 ? (uint)(PayloadByte(0) & 7) : 0; TracePoint(0, x, y, firstDirection); Append(CurrentPoint(), firstDirection);
            uint previous = firstDirection, last = firstDirection;
            // 압축 run은 진입에서 읽은 signed count를 보존하고 로그 뒤 실제 append를 실행한다.
            for (int index = 0; index < count; index++)
            {
                byte packed = PayloadByte(index); uint length = (uint)(packed >> 3); last = (uint)(packed & 7);
                Need(hooks.Trace)(FormattableString.Invariant($"n[{index}] {last} | {length}\n"));
                if (last != previous)
                {
                    Append(CurrentPoint(), previous); TracePoint(runtime.Count - 1, x, y, previous);
                    Append(CurrentPoint(), last); TracePoint(runtime.Count - 1, x, y, last);
                }
                x = unchecked(x + (uint)StepX[last] * length); y = unchecked(y + (uint)StepY[last] * length); previous = last;
            }
            if (count != 0) Append(CurrentPoint(), last); TracePoint(runtime.Count - 1, x, y, last);
        }
        else
        {
            Append(CurrentPoint(), 0);
            if ((packet.Span[13] & 1) != 0) Need(hooks.Assertion)("(zwp->nodeCount & 1) == 0", @"\Ns\O\Pathprocess.cpp", 0x1ca);
            // 현재 signed nodeCount를 조건마다 읽고 현재 좌표쌍을 순서대로 추가한다.
            for (int index = 0; index < NodeCount(); index += 2)
            {
                byte px = PayloadByte(index), py = PayloadByte(index + 1); Append(new(px, py), 0);
            }
        }
        uint target = (uint)runtime.ReadBits(0x186e4);
        if (target != 0 && target < pool.Capacity && (pool.Slot((ushort)target)[11] & 1) == 0)
        {
            int stage = unchecked((int)runtime.ReadBits(0x186e8)); int destination = (ushort)target;
            if (stage > 0 && stage < 5)
            {
                RawPathPoint corner = Need(hooks.StagePoint)(destination, (uint)(stage - 1));
                double dx = (double)BitConverter.UInt32BitsToSingle((uint)Bits(pool.Slot(parent), 14)) - corner.X;
                double dy = (double)BitConverter.UInt32BitsToSingle((uint)Bits(pool.Slot(parent), 18)) - corner.Y;
                if (Math.Sqrt(dx * dx + dy * dy) < StageSnapDistance)
                {
                    runtime.WriteBits(0x14, BitConverter.SingleToUInt32Bits(corner.X)); runtime.WriteBits(0x18, BitConverter.SingleToUInt32Bits(corner.Y));
                }
            }
            stage = unchecked((int)runtime.ReadBits(0x186e8));
            if (stage >= 0 && stage < 5)
            {
                RawPathPoint corner = Need(hooks.StagePoint)(destination, (uint)stage); long at = (long)runtime.Count * 8 + 0xc;
                if (at < 0 || at > int.MaxValue - 4) throw new ArgumentOutOfRangeException(nameof(runtime), "경로 수신 마지막 점 물리 범위");
                runtime.WriteBits((int)at, BitConverter.SingleToUInt32Bits(corner.X)); runtime.WriteBits((int)at + 4, BitConverter.SingleToUInt32Bits(corner.Y));
            }
        }
        float turning = Need(hooks.Metadata)(parent).TurningSpeed, speed = Need(hooks.Metadata)(parent).Speed;
        runtime.Prepare(Need(hooks.Now)(), startTime, speed, turning, (int)firstDirection); Need(hooks.ClearAnimation)(parent);
    }
}
