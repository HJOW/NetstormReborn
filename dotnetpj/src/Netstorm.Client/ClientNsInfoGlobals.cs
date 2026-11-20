using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

// NSInfo의 현재 전역과 원문 서버 이름 물리를 Client 수명에 보유한다.
namespace Netstorm.Client;

/// <summary>같은 현재 NSInfo·zone·정렬표·서버 행을 보유하며 getter나 구성으로 통신하지 않는다.</summary>
public sealed class ClientNsInfoGlobals
{
    /// <summary>00593150의 실제 현재 NSInfo 포인터 토큰이다.</summary>
    public UIntBox Current { get; } = new();
    /// <summary>005467b8의 같은 실제 zone DWORD다.</summary>
    public UIntBox Zone { get; } = new();
    /// <summary>005930b0의 같은 정렬40DWORD이며 목록 갱신은1..39만 사용한다.</summary>
    public uint[] RosterOrder { get; } = new uint[40];
    /// <summary>0054db4c의 최초 BSS0과 현재 curServer 설정이다.</summary>
    public IntBox ServerSelection { get; } = new();
    /// <summary>0054db70의 현재 passedDiags 옵션 DWORD다.</summary>
    public IntBox PassedDiags { get; } = new();
    /// <summary>0054db54의 현재 rootServerBroadcastPort DWORD다.</summary>
    public IntBox RootServerBroadcastPort { get; } = new();
    /// <summary>00594970의 실제 공유80BYTE이며 무효 서버 조회는 기존 내용을 보존한다.</summary>
    public byte[] ServerName { get; } = new byte[80];
    /// <summary>00532714/71c의 현재 좁은 바이트 서버 행이다. 각 문자열은 Latin1로 바이트를 보존한다.</summary>
    public List<string> Servers { get; } = [];
    /// <summary>00532724의 바이트 합계이며 기존 행 교체에서는 원문처럼 그대로 둔다.</summary>
    public uint ServerListChecksum { get; set; }
    /// <summary>00540421의 최초 NSInfo 종류BYTE6이다.</summary>
    public byte NsKind { get; set; } = 6;
    /// <summary>00540440의 최초 PlayerInfo 종류BYTE37이다.</summary>
    public byte PlayerKind { get; set; } = 37;
    /// <summary>00540457의 최초 BattleInfo 종류BYTE60이다.</summary>
    public byte BattleKind { get; set; } = 60;
    /// <summary>00540bac의 실제 초기 zone 형식 문자열 토큰이다.</summary>
    public uint ZoneFormat { get; set; } = 0x50c894;
    /// <summary>00542714의 현재 Leave this Battle 명령이다.</summary>
    public uint LeaveMenuCommand { get; set; } = 0x02000006;
    /// <summary>005429dc의 같은 현재 추방 확인 명령이다.</summary>
    public uint KickoffCommand { get; set; } = 0x020000ba;
    /// <summary>00542b80의 현재 정렬 선택 명령이다.</summary>
    public uint SortOptionsCommand { get; set; } = 0x0200012e;
    /// <summary>00542b7c의 현재 개별 설정 명령이다.</summary>
    public uint SetOptionCommand { get; set; } = 0x0200012d;
    /// <summary>005429e0의 현재 TYPE 제외 추가 명령이다.</summary>
    public uint ExcludeTypeCommand { get; set; } = 0x020000bb;
    /// <summary>005429e4의 현재 TYPE 제외 제거 명령이다.</summary>
    public uint RestoreTypeCommand { get; set; } = 0x020000bc;

    /// <summary>원문 InterpretFortFileOptions의 세 NSInfo DWORD를 현재값에서 읽어 누락 키는 보존한다.</summary>
    public void ReadOptions(ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(configuration);
        int value = ServerSelection.Value;
        configuration.ReadInt("curServer", ref value); ServerSelection.Value = value;
        value = RootServerBroadcastPort.Value;
        configuration.ReadInt("rootServerBroadcastPort", ref value); RootServerBroadcastPort.Value = value;
        value = PassedDiags.Value;
        configuration.ReadInt("passedDiags", ref value); PassedDiags.Value = value;
    }

    /// <summary>현재 C문자열의 첫 NUL 전까지만 읽는다.</summary>
    private static string Text(string value)
    {
        int end = value.IndexOf('\0'); return end < 0 ? value : value[..end];
    }

    /// <summary>원문 C locale의 바이트 ASCII 소문자 변환만 수행하며 표시 언어의 Unicode 변환을 섞지 않는다.</summary>
    private static string Lower(string value) => string.Create(value.Length, value, static (target, source) =>
    {
        // 좁은 문자열의 각 실제 바이트에 해당하는 ASCII 대문자만 바꾼다.
        for (int index = 0; index < source.Length; index++)
            target[index] = source[index] is >= 'A' and <= 'Z' ? (char)(source[index] + 32) : source[index];
    });

    /// <summary>외부 행 입력의 원문 BYTE 폭을 보존하고 관리 Unicode 손실 대체를 거부한다.</summary>
    private static byte[] Bytes(string value)
    {
        byte[] result = new byte[value.Length];
        // 원문은 처음부터 BYTE 문자열이므로 각 관리 문자의 단일BYTE 변환만 허용한다.
        for (int index = 0; index < value.Length; index++) result[index] = checked((byte)value[index]);
        return result;
    }

    /// <summary>시작 서버 파일의 비어 있지 않은 행을 원래 순서로 읽고 정상 복귀 뒤 목록/합계를 바꾼다.</summary>
    public void LoadServerNames(ReadOnlySpan<byte> file)
    {
        var loaded = new List<string>();
        // 원문 getline의 LF 경계이며 길이 판정은 CR 제거보다 먼저 수행한다.
        foreach (string line in Encoding.Latin1.GetString(file).Split('\n'))
        {
            if (line.Length > 256) throw new ArgumentException("시작 서버 목록의 긴 원본 행 미연결", nameof(file));
            string row = line.Replace("\r", "", StringComparison.Ordinal);
            if (row.Length != 0) loaded.Add(row);
        }
        Servers.Clear(); Servers.AddRange(loaded); RecalculateServerChecksum();
    }

    /// <summary>LAN은 strcpy 범위만 쓰고 무효 선택은 보존하며 유효 행만 같은80BYTE를 초기화한다.</summary>
    public ReadOnlyMemory<byte> ResolveServerName(Func<string, ReadOnlyMemory<byte>>? translate)
    {
        if (ServerSelection.Value == -1)
        {
            ReadOnlySpan<byte> value = (translate ?? throw new InvalidOperationException("현재 LAN 서버 이름 번역 미연결"))("LAN Server").Span;
            int nul = value.IndexOf((byte)0); if (nul >= 0) value = value[..nul];
            if (value.Length >= ServerName.Length) throw new ArgumentException("LAN 서버 이름의 원본80BYTE 경계 초과");
            value.CopyTo(ServerName); ServerName[value.Length] = 0;
        }
        else if (ServerSelection.Value >= 0 && ServerSelection.Value < Servers.Count)
            ReadServerFields(ServerSelection.Value, [], ServerName, null);
        int end = Array.IndexOf(ServerName, (byte)0);
        return ServerName.AsMemory(0, end < 0 ? ServerName.Length : end);
    }

    /// <summary>무효 index에서는 하위 parser와 출력0쓰기를 모두 생략한다.</summary>
    public void ReadServerFields(int index, Span<byte> endpoint, Span<byte> name, IntBox? subscriber)
    {
        if (index >= 0 && index < Servers.Count) RootServerRecords.Read(Bytes(Text(Servers[index])), endpoint, name, subscriber);
    }

    /// <summary>원문 flags7의 첫 대소문자 무시 부분검색과 기존 교체의 stale 체크섬을 유지한다.</summary>
    public void UpsertServer(ReadOnlySpan<byte> row, bool preserveValue)
    {
        byte[] endpoint = new byte[80], description = new byte[80]; var value = new IntBox();
        RootServerRecords.Read(row, endpoint.AsSpan(0, 79), description.AsSpan(0, 79), value);
        string name = Lower(Text(Encoding.Latin1.GetString(endpoint))), label = Text(Encoding.Latin1.GetString(description));
        // endpoint 전체 일치가 아니라 저장된 첫 C문자열의 부분 일치를 선택한다.
        for (int index = 0; index < Servers.Count; index++)
        {
            if (!Lower(Text(Servers[index])).Contains(name, StringComparison.Ordinal)) continue;
            var previous = new IntBox(); ReadServerFields(index, [], [], previous);
            Servers[index] = name + ';' + label + ';' + (preserveValue ? previous.Value : value.Value).ToString(CultureInfo.InvariantCulture);
            return;
        }
        string formatted = name + ';' + label + ';' + value.Value.ToString(CultureInfo.InvariantCulture);
        // flags4는 새 행 전체를 첫 NUL 앞 ASCII 바이트 기준으로 비교한다.
        foreach (string candidate in Servers)
            if (Lower(Text(candidate)) == Lower(formatted)) return;
        Servers.Add(formatted); RecalculateServerChecksum();
    }

    /// <summary>유효한 현재 행만 다시 판독하고 DWORD를 signed 십진 값으로 교체한다.</summary>
    public void SetServerValue(int index, uint value)
    {
        if (index < 0 || index >= Servers.Count) return;
        byte[] endpoint = new byte[80], description = new byte[80];
        ReadServerFields(index, endpoint.AsSpan(0, 79), description.AsSpan(0, 79), null);
        string row = Text(Encoding.Latin1.GetString(endpoint)) + ';' + Text(Encoding.Latin1.GetString(description)) + ';' + unchecked((int)value).ToString(CultureInfo.InvariantCulture);
        UpsertServer(Bytes(row), false);
    }

    /// <summary>구분자나 종료NUL을 추가하지 않고 각 현재 unsignedBYTE+1을 DWORD 모듈러 합계로 만든다.</summary>
    public void RecalculateServerChecksum()
    {
        ServerListChecksum = 0;
        // 원문 현재 모든 행을 매번 다시 읽고 각 첫 NUL 뒤는 생략한다.
        foreach (string row in Servers)
            // 관리 문자열의 각 원문 BYTE가 현재 합계에 참여한다.
            foreach (byte value in Bytes(Text(row))) ServerListChecksum = unchecked(ServerListChecksum + value + 1u);
    }
}
