using Netstorm.Client.Chat;
using Netstorm.Core.Original;

// 같은 manager current 슬롯과 별도 할당을 Client 수명으로 보관한다.
namespace Netstorm.Client;

/// <summary>ClientChatManagerGlobals.cpp의 미초기화 물리/비영 토큰/반납 종류를 소유한다.</summary>
public sealed class ClientChatManagerGlobals
{
    /// <summary>현재 슬롯과 독립인 실제 한 할당이다.</summary>
    private sealed class Allocation
    {
        /// <summary>값 초기화를 요청하지 않은 실제 BYTE 저장소다.</summary>
        public byte[] Bytes { get; }
        /// <summary>manager와 반납 가능한 수신자 버퍼를 구별한다.</summary>
        public bool RecipientBuffer { get; }
        /// <summary>같은 물리와 할당 종류만 보관하며 바이트에 쓰지 않는다.</summary>
        public Allocation(byte[] bytes, bool recipientBuffer) { Bytes = bytes; RecipientBuffer = recipientBuffer; }
    }
    /// <summary>00548714의 BSS0이며 Allocate 자체는 이 값을 변경하지 않는다.</summary>
    public UIntBox Current { get; } = new();
    /// <summary>0059a830의 현재 복원된 앞81BYTE이며 재초기화로 지우지 않는다.</summary>
    public byte[] FormatCapture { get; } = new byte[RawChatFormat.CaptureBytes];
    /// <summary>host 포인터를 자르지 않는 비영 토큰이며 소진 뒤0에서 실패한다.</summary>
    private uint _nextToken = 1;
    /// <summary>이전 manager도 유지하며 명시 반납한 수신자만 제거한다.</summary>
    private readonly Dictionary<uint, Allocation> _allocations = new();
    /// <summary>원문 생성자의2712/128BYTE 요청만 받는다. current 쓰기를 추가하지 않는다.</summary>
    public uint Allocate(uint size)
    {
        if (size != RawChatManagerPrefix.ManagerBytes && size != RawChatManagerPrefix.BufferBytes) throw new ArgumentOutOfRangeException(nameof(size), "실제 Chatmanager 할당 크기 오류");
        return AllocateStorage(size, size == RawChatManagerPrefix.BufferBytes);
    }
    /// <summary>확장 수신자 배열의 양수 DWORD 폭을 별도 종류로 할당한다.</summary>
    public uint AllocateRecipientBuffer(uint size)
    {
        if (size == 0 || (size & 3) != 0) throw new ArgumentOutOfRangeException(nameof(size), "Chat 수신자 배열의 실제 DWORD 폭 오류");
        return AllocateStorage(size, true);
    }
    /// <summary>값 초기화를 요청하지 않고 실제 배열을 등록한 뒤에만 토큰을 진행한다.</summary>
    private uint AllocateStorage(uint size, bool recipientBuffer)
    {
        if (_nextToken == 0) throw new OverflowException("Chatmanager host 토큰 소진");
        if (size > Array.MaxLength) throw new ArgumentOutOfRangeException(nameof(size), "Chatmanager CLR 배열 물리 범위");
        byte[] bytes;
        try { bytes = GC.AllocateUninitializedArray<byte>((int)size); } catch (OutOfMemoryException) { return 0; }
        uint token = _nextToken; _allocations.Add(token, new(bytes, recipientBuffer)); _nextToken = unchecked(token + 1); return token;
    }
    /// <summary>free(NULL)을 허용하고 이미 반납한 토큰이나 manager 자체는 거부한다.</summary>
    public void ReleaseRecipientBuffer(uint token)
    {
        if (token == 0) return;
        if (!_allocations.TryGetValue(token, out var allocation) || !allocation.RecipientBuffer) throw new ArgumentOutOfRangeException(nameof(token), "Chat 수신자 반납의 실제 할당 없음/종류 오류");
        _allocations.Remove(token);
    }
    /// <summary>같은 살아 있는 배열을 빌리며0/해제/미등록 토큰은 실제 접근에서 실패한다.</summary>
    public byte[] Bytes(uint token)
    {
        if (token == 0 || !_allocations.TryGetValue(token, out var allocation)) throw new ArgumentOutOfRangeException(nameof(token), "Chatmanager 실제 할당 토큰 없음");
        return allocation.Bytes;
    }
    /// <summary>같은 장기 소유자/물리/순수CRT만 연결하며 설정·명령·랭크 장치는 실제 호출자가 제공한다.</summary>
    public ChatManagerPrefixHooks MakePrefixHooks() => RawChatManagerPrefix.BindChatManagerMemoryCopy(new()
    {
        Allocate = Allocate, Memory = token => Bytes(token),
    });
}
