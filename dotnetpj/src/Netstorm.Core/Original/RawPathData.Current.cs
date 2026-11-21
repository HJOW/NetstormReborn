// Native 작성은 진단/로그 뒤 현재 이미지와 WORD를 다시 읽는다.
namespace Netstorm.Core.Original;

/// <summary>4895b0의 실제 부분 쓰기/진단 복귀/정수 감김을 유지하는 작성 부분이다.</summary>
public sealed partial class RawPathData
{
    /// <summary>masked signed64 _ftol의 정상 절삭 낮은DWORD와 비정상 결과0을 보존한다.</summary>
    private static uint NativeFtol(double value)
    {
        const double signed64Limit = 9223372036854775808.0; // 원본 signed64의 배타적 양수 상한이다.
        if (!double.IsFinite(value) || value < -signed64Limit || value >= signed64Limit) return 0;
        return unchecked((uint)(long)Math.Truncate(value));
    }
    /// <summary>원문 SUB/ABS의 DWORD 감김을 보존하며 int.MinValue도 정규화하지 않는다.</summary>
    private static uint NativeDistance(int from, int to)
    {
        uint delta = unchecked((uint)from - (uint)to), sign = (delta & 0x80000000u) != 0 ? uint.MaxValue : 0;
        return unchecked((delta ^ sign) - sign);
    }
    /// <summary>실제 현재 Runtime/버퍼를 사용한다. typed의count/좌표/길이 자격 검사를 앞에 추가하지 않는다.</summary>
    public int WriteCurrent(RawPathRuntime runtime, ushort form, ushort parent, RawPathDataHeader header, Memory<byte> buffer, RawPathDataWriteHooks hooks)
    {
        if (_edition != OriginalEdition.Patch1078) throw new ArgumentException("Native 경로 작성의 실제 PATCH 판본 필요");
        ReadOnlyMemory<byte> image = runtime.Bytes; RawPathLayout layout = RawPathRuntime.LayoutFor(_edition);
        if (image.Length != layout.Size) throw new ArgumentException("Native 경로 작성의 실제 이미지 물리 불일치");
        // 각 진단/조건에서 같은 원문 current count를 다시 조회한다.
        int Count() => unchecked((int)Bits(image.Span, 0x10));
        // 진단 정상 복귀 뒤 count를 재검사하지 않고 선택된 물리를 읽는다.
        void Check(int index, uint line)
        {
            if (index >= Count()) Need(hooks.Assertion)("i < nVert", @"c:\ns\o\path.h", line);
        }
        // 배열 count가 아닌 실제 이미지 경계에서 좌표와 signed64 절삭을 선택한다.
        int CurrentCoordinate(int index, bool y) => unchecked((int)NativeFtol(BitConverter.UInt32BitsToSingle((uint)Bits(image.Span, 0x14L + (long)index * 8 + (y ? 4 : 0)))));
        // 로그/진단이 바꾼 현재 nodeCount WORD를 매번 읽는다.
        ushort NodeWord() => (ushort)Bits(buffer.Span, 13, 2);
        // payload 주소와 반환은 원문 signed WORD다.
        int SignedWord() => unchecked((short)NodeWord());
        // 원문 signed DWORD printf의 현재 point 서식을 문화권과 무관하게 전달한다.
        void TracePoint(int index, int x, int y, uint direction) => Need(hooks.Trace)(FormattableString.Invariant($"[{index}] ({x},{y}) , {unchecked((int)direction)}\n"));
        Put(buffer.Span, 1, header.PacketId); Put(buffer.Span, 0, (uint)(header.PacketFlags & 0x7f)); Put(buffer.Span, 2, Math.Max(header.PacketLength, 4u), 2);
        Put(buffer.Span, 4, form, 2); Put(buffer.Span, 6, parent, 2);
        Check(0, 0x1f); Put(buffer.Span, 15, unchecked((uint)CurrentCoordinate(0, false)));
        Check(0, 0x1f); Put(buffer.Span, 16, unchecked((uint)CurrentCoordinate(0, true)));
        Check(0, 0x24); Put(buffer.Span, 17, NativeFtol(BitConverter.UInt64BitsToDouble(Bits(image.Span, layout.Times, 8)) * TimeScale), 4);
        Put(buffer.Span, 11, Bits(image.Span, layout.Helper, 2), 2); Put(buffer.Span, 21, Bits(image.Span, 0x186e4, 2), 2); Put(buffer.Span, 23, Bits(image.Span, 0x186e8, 1));
        Check(0, 0x1f); int x = CurrentCoordinate(0, false); Check(0, 0x1f); int y = CurrentCoordinate(0, true); Put(buffer.Span, 13, 0, 2);
        uint parentGenus = Need(hooks.ParentGenus)((ushort)Bits(buffer.Span, 6, 2));
        if ((parentGenus & 0x120000u) == 0)
        {
            Check(0, 0x29); TracePoint(0, x, y, (uint)Bits(image.Span, layout.Directions));
            int index = 1;
            if (index < Count())
            {
                Check(index, 0x1f);
                // 최초x 진단 뒤 각y/방향 진단과 루프말 현재count를 원문 순서대로 조회한다.
                do
                {
                    int nextX = CurrentCoordinate(index, false); Check(index, 0x1f); int nextY = CurrentCoordinate(index, true);
                    Check(index, 0x29); uint direction = (uint)Bits(image.Span, layout.Directions + (long)index * 4); TracePoint(index, nextX, nextY, direction);
                    if (x != nextX || y != nextY)
                    {
                        uint bits = NativeDistance(x, nextX); if ((direction & 1) == 0) bits = unchecked(bits + NativeDistance(y, nextY)); int remaining = unchecked((int)bits);
                        // 양수signed 거리만31칸씩 쓰고 run 로그 복귀 뒤 현재WORD로 주소를 정한다.
                        while (remaining > 0)
                        {
                            int step = Math.Min(remaining, 31);
                            Need(hooks.Trace)(FormattableString.Invariant($"n[{SignedWord()}] {direction & 7} | {step & 0x1f}\n"));
                            Put(buffer.Span, Payload + (long)SignedWord(), ((uint)step << 3) | (direction & 7)); Put(buffer.Span, 13, unchecked((ushort)(NodeWord() + 1)), 2);
                            remaining = unchecked((int)((uint)remaining - (uint)step));
                        }
                    }
                    x = nextX; y = nextY; index++;
                } while (index < Count());
            }
        }
        else
        {
            int index = 1;
            // 직접 좌표 분기는 WORD 증가→진단→현재 좌표→저장 순서를 각각 보존한다.
            while (index < Count())
            {
                int before = SignedWord(); Put(buffer.Span, 13, unchecked((ushort)(NodeWord() + 1)), 2); Check(index, 0x1f); Put(buffer.Span, Payload + (long)before, unchecked((uint)CurrentCoordinate(index, false)));
                before = SignedWord(); Put(buffer.Span, 13, unchecked((ushort)(NodeWord() + 1)), 2); Check(index, 0x1f); Put(buffer.Span, Payload + (long)before, unchecked((uint)CurrentCoordinate(index, true))); index++;
            }
        }
        return SignedWord() + Payload;
    }
}
