using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 패킷 기록 FILE과 요청 DWORD를 같은 Client 프레임보다 짧은 파일 수명으로 보관한다.
namespace Netstorm.Client.Host;

/// <summary>cpppj/src/client/ClientPacketRecord.cpp/.h의21BYTE 접두와 현재 선언 길이를 기록한다.</summary>
public sealed class ClientPacketRecord : IDisposable
{
    /// <summary>현재 wall/번호를 사용 위치에서 읽는 장기 FrameTime 주소다.</summary>
    private readonly FrameTimeBox _frame;
    /// <summary>명시 검사/호스트 루트다. 없으면 실제 Open에서만 사용자 홈을 해석한다.</summary>
    private readonly UserDataRoot? _user;
    /// <summary>현재 이 소유자가 연 실제 읽기/쓰기 파일이다.</summary>
    private FileStream? _file;
    /// <summary>파일 열기 실패 뒤에도1인 원문005e4774 기록 관문이다.</summary>
    private uint _enabled;
    /// <summary>0054dbac의 최초0 요청이다. Open/Close가 바꾸지 않는다.</summary>
    public UIntBox Requested { get; } = new();
    /// <summary>원문 435220의 옵션 재해석처럼 0054dbac 요청만 0으로 재설정한다. 이미 열린 기록 FILE/enabled는 닫거나 지우지 않는다. 0.933에서 추가됐다.</summary>
    public void ResetRequest() => Requested.Value = 0;

    /// <summary>같은 프레임 소유권만 보관하며 파일/폴더는 생성하지 않는다.</summary>
    public ClientPacketRecord(FrameTimeBox frame, UserDataRoot? user = null) { _frame = frame ?? throw new ArgumentNullException(nameof(frame)); _user = user; }
    /// <summary>enabled0을 먼저 쓰고 현재 파일을 닫는다. 원문 fclose 반환값처럼 닫기 오류로 상태를 되돌리지 않는다.</summary>
    public void Close()
    {
        _enabled = 0;
        try { _file?.Dispose(); } catch (IOException) { } finally { _file = null; }
    }
    /// <summary>기존 파일 닫기→enabled1→이름 leaf 검사→사용자 Diagnostics/w+b 순서를 지킨다.</summary>
    public void Open(string name)
    {
        ArgumentNullException.ThrowIfNull(name); Close(); _enabled = 1;
        int nul = name.IndexOf('\0'); string leaf = Path.GetFileName(nul < 0 ? name : name[..nul]);
        if (leaf is "" or "." or "..") throw new ArgumentException("Client 패킷 기록 파일 이름 누락", nameof(name));
        var user = _user ?? UserDataRoot.Resolve(); UserDataRoot.EnsureDirectory(user.Diagnostics());
        try { _file = new FileStream(Path.Combine(user.Diagnostics(), leaf), FileMode.Create, FileAccess.ReadWrite, FileShare.ReadWrite); }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException) { _file = null; }
    }
    /// <summary>원문 fwrite 오류는 앞선 부분 효과를 취소하거나 나머지 쓰기를 생략하지 않는다.</summary>
    private void WritePart(ReadOnlySpan<byte> bytes)
    {
        try { _file!.Write(bytes); } catch (IOException) { }
    }
    /// <summary>방향·현재 wall·low32 프레임·진입 주소/sender를 쓰고 그 뒤 현재 WORD 길이만큼 패킷을 쓴다.</summary>
    public void Write(bool outgoing, PacketConnection connection, ReadOnlyMemory<byte> packet, ushort length)
    {
        ArgumentNullException.ThrowIfNull(connection); uint address = BinaryPrimitives.ReadUInt32LittleEndian(connection.Address.AsSpan(4, 4)), sender = connection.Sender;
        _ = length; if (_enabled == 0) return;
        if (_file is null) throw new InvalidOperationException("Client 기록 enabled1의 원문 NULL FILE 쓰기 거부");
        Span<byte> bytes = stackalloc byte[8]; bytes[0] = outgoing ? (byte)1 : (byte)0; WritePart(bytes[..1]);
        BinaryPrimitives.WriteDoubleLittleEndian(bytes, _frame.Value.Wall); WritePart(bytes);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes, unchecked((uint)_frame.Value.Number)); WritePart(bytes[..4]);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes, address); WritePart(bytes[..4]); BinaryPrimitives.WriteUInt32LittleEndian(bytes, sender); WritePart(bytes[..4]);
        if (packet.Length < 4) throw new ArgumentOutOfRangeException(nameof(packet), "Client 기록 현재 선언 WORD 물리 부족");
        int count = BinaryPrimitives.ReadUInt16LittleEndian(packet.Span.Slice(2, 2));
        if (count > packet.Length) throw new ArgumentOutOfRangeException(nameof(packet), "Client 기록 현재 선언 길이/물리 범위");
        WritePart(packet.Span[..count]); try { _file.Flush(); } catch (IOException) { }
    }
    /// <summary>실제 Client 종료 시 자기가 연 파일만 닫는다.</summary>
    public void Dispose() => Close();
}
