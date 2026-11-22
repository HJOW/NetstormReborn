using System.Buffers.Binary;

// 실제 Player 고유 전처리가 각 주소 소비 때 현재 기록과 owner 수명을 다시 빌린다.
namespace Netstorm.Core.Original;

/// <summary>한 필드 접근 동안 같은 실제 owner와 현재 Player 바이트를 보유한다.</summary>
/// <param name="Pin">현재 기록을 소유하는 실제 객체다. 임시 대체 owner를 만들지 않는다.</param>
/// <param name="Records">그때의 실제 Player base이며 다음 접근까지 캐시하지 않는다.</param>
public readonly record struct PlayerRecordAccess(object? Pin, Memory<byte> Records);

/// <summary>Player 고유 전처리의 현재 기록 조회와 비가상 공통 Pre 장치다.</summary>
public sealed class PlayerPreDestroyHooks
{
    /// <summary>각 주소 계산에서 실제 owner와 현재 base를 새로 빌린다.</summary>
    public Func<PlayerRecordAccess>? Current { get; set; }
    /// <summary>첫 Technology 정리 뒤 같은 공통 삭제 깊이의 Pre를 실행한다.</summary>
    public Action<int, uint>? Base { get; set; }
}

/// <summary>004906c0의 세 현재 주소 계산을 공통 부모 전후로 구분한다.</summary>
public sealed partial class RawPlayerWorld
{
    /// <summary>자신 Technology만0 → 공통 Pre → 현재 flags bit0 해제 → 다시 현재 bit8 설정 순서다.</summary>
    /// <remarks>cpppj/src/o/RawPlayerWorld.cpp의 PreDestroyNative 전체 본체다. 미설치 장치는 선택 위치에서 실패한다.</remarks>
    public static void PreDestroyNative(RawSidPool pool, int sid, uint flags, PlayerPreDestroyHooks hooks)
    {
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("Native Player 전처리 판본 오류", nameof(pool));

        /// <summary>owner는 getter 전에 읽고 이번 한 DWORD만 같은 view/pin에 묶는다.</summary>
        PlayerRecordAccess Field(int offset)
        {
            byte owner = pool.Slot(sid)[OwnerOffset];
            PlayerRecordAccess access = (hooks.Current ?? throw new InvalidOperationException("Player의 선택된 실제 현재 기록 미연결"))();
            int start = owner * Stride + offset;
            if (start > access.Records.Length || access.Records.Length - start < 4) throw new ArgumentOutOfRangeException(nameof(sid), "Player 전처리 현재 기록 범위");
            return access with { Records = access.Records.Slice(start, 4) };
        }

        PlayerRecordAccess technology = Field(TechnologyField);
        try
        {
            Span<byte> field = technology.Records.Span;
            if (BinaryPrimitives.ReadUInt32LittleEndian(field) == unchecked((uint)sid)) BinaryPrimitives.WriteUInt32LittleEndian(field, 0);
        }
        finally { GC.KeepAlive(technology.Pin); }

        (hooks.Base ?? throw new InvalidOperationException("Player의 선택된 실제 공통 Pre 미연결"))(sid, flags);

        PlayerRecordAccess clearing = Field(0x50);
        try
        {
            Span<byte> field = clearing.Records.Span;
            BinaryPrimitives.WriteUInt32LittleEndian(field, BinaryPrimitives.ReadUInt32LittleEndian(field) & 0xfffffffeu);
        }
        finally { GC.KeepAlive(clearing.Pin); }

        PlayerRecordAccess marking = Field(0x50);
        try
        {
            Span<byte> field = marking.Records.Span;
            BinaryPrimitives.WriteUInt32LittleEndian(field, BinaryPrimitives.ReadUInt32LittleEndian(field) | 0x100u);
        }
        finally { GC.KeepAlive(marking.Pin); }
    }
}
