// 실제 Path 클래스의 같은 버퍼와 마지막 writer 선택을 기존 전송 체인에 결합한다.
namespace Netstorm.Core.Original;

/// <summary>RawPathData.cpp의 MakePathFormTransmission 실제 분배다.</summary>
public static class RawPathTransmission
{
    /// <summary>선택된 기존 파생 함수만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Path 전송의 선택된 실제 경계 미연결");
    /// <summary>같은 Pool/TYPE/헤더/예약을 사용하고 Native 작성자는 typed 선행guard 없이 정확한 본체에 전달한다.</summary>
    public static ProcessFormTransmission Make(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPathDataHeader header, byte[] buffer, Action<int> transport, ProcessFormTransmission? other = null, Func<SquidProcess, Memory<byte>, bool, uint, int>? nativeWriter = null)
    {
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || buffer.Length < 24 || transport is null) throw new ArgumentException("Path 서버 직렬화 연결 누락");
        ProcessFormTransmission fallback = other?.Clone() ?? new(); bool writingPath = false;
        // 원문 동일 클래스 판정과 실제 선택된 writer만 수행한다.
        int Write(SquidProcess process, bool parentChanged)
        {
            writingPath = process is RawPathProcess;
            if (process is RawPathProcess path)
            {
                if (nativeWriter is not null) return nativeWriter(process, buffer, parentChanged, 0);
                if (!ReferenceEquals(path.Pool, pool) || process.Form() == 0 || process.Parent() == 0) throw new ArgumentException("Path 서버 직렬화 Pool/부착 불일치");
                int type = pool.Slot(process.Parent())[10]; return new RawPathData(pool.Edition).Write(path.Runtime, (ushort)process.Form(), (ushort)process.Parent(), types[type].Flags2, header, buffer);
            }
            return Need(fallback.WriteData)(process, parentChanged);
        }
        return new()
        {
            WriteData = Write,
            WriteFullData = (process, force) =>
            {
                if (process is RawPathProcess) { if (nativeWriter is not null) { writingPath = true; return nativeWriter(process, buffer, false, force); } return Write(process, false); }
                writingPath = false; return Need(fallback.WriteFullData)(process, force);
            },
            Packet = size =>
            {
                if (!writingPath) return Need(fallback.Packet)(size); if (size < 0 || size > buffer.Length) throw new ArgumentOutOfRangeException(nameof(size), "Path 실제 작성 버퍼 범위"); return buffer.AsSpan(0, size).ToArray();
            },
            Transmit = size => { if (writingPath) transport(size); else Need(fallback.Transmit)(size); },
        };
    }
}
