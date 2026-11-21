using System.Buffers.Binary;
using Netstorm.Client.Scripting;

// 원문 PE의 정적 say와 인접 명령표를 같은 Client 수명으로 보관한다.
namespace Netstorm.Client;

/// <summary>분배의00557db4/00557df0 데이터다. 실제 Dispatch 본체는 후속 호출자에서 연결한다.</summary>
public sealed class DotCommandDispatchState
{
    /// <summary>실패 출력도 다시 읽는 기본 명령 슬롯의 최초0이다.</summary>
    public uint FallbackIndex { get; set; }
    /// <summary>마지막 검색 그룹3의 상대 위치이며 최초-1이다.</summary>
    public int SeparatorOffset { get; set; } = -1;
}
/// <summary>ClientDotCommandGlobals.cpp의 전체2568BYTE와 장기 분배 검색 데이터를 소유한다.</summary>
public sealed class ClientDotCommandGlobals
{
    /// <summary>이름/count/함수/표시를 분리하지 않는 동일 인접 물리다.</summary>
    public byte[] Bytes { get; } = new byte[RawDotCommandRegister.StateBytes];
    /// <summary>재진입과 미션 교체로 초기화하지 않는 같은 검색 상태다.</summary>
    public DotCommandDispatchState DispatchState { get; } = new();
    /// <summary>최초 PE의 비영 say/함수/표시/count1만 한 번 구성한다.</summary>
    public ClientDotCommandGlobals()
    {
        Bytes[0] = (byte)'s'; Bytes[1] = (byte)'a'; Bytes[2] = (byte)'y';
        BinaryPrimitives.WriteUInt32LittleEndian(Bytes.AsSpan(0x800, 4), 0x42f1e0); BinaryPrimitives.WriteUInt32LittleEndian(Bytes.AsSpan(0x900, 4), RawDotCommandRegister.NamesToken); BinaryPrimitives.WriteUInt32LittleEndian(Bytes.AsSpan(0xa04, 4), 1);
    }
    /// <summary>같은 현재 count/표시를 검증된 등록 본체에 전달하며 표를 재구성하지 않는다.</summary>
    public uint Register(ReadOnlySpan<byte> name, uint callback, uint flag, DotCommandRegisterHooks? hooks = null) => RawDotCommandRegister.RegisterDotCommand(Bytes, name, callback, flag, hooks);
}
