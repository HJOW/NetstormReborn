using Netstorm.Core.Original;

// 창 생성/소멸보다 오래 유지되는 실제 Chat 전역 상태다.
namespace Netstorm.Client.Chat;

/// <summary>ChatWindow.h의 현재 root/명령 등록/세 자동완성 벡터다.</summary>
public sealed class ChatWindowState
{
    /// <summary>00548740의 BSS0 현재 root 주소다.</summary>
    public UIntBox Current { get; } = new();
    /// <summary>00548744의 최초0이며 clear/quit 등록 뒤1이 된다.</summary>
    public uint Registered { get; set; }
    /// <summary>0054da91의 최초0이며 도움말 후보 초기화 여부다.</summary>
    public byte CompletionReady { get; set; }
    /// <summary>0054de88의 현재 색 문자열 목록이다.</summary>
    public List<string> Colors { get; } = [];
    /// <summary>0054dea8의 현재 배경 문자열 목록이다.</summary>
    public List<string> Backgrounds { get; } = [];
    /// <summary>0054de98의 현재 아이콘 문자열 목록이다.</summary>
    public List<string> Icons { get; } = [];
}
