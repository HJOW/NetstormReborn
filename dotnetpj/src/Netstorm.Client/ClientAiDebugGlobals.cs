using Netstorm.Core.Original;

// AI 진단의 현재 DWORD와 상세를 짧은 Session보다 긴 Client 수명에 보관한다.
namespace Netstorm.Client;

/// <summary>ClientAiDebugGlobals.h의 실제 PE/BSS 소유자다. 생성 시 게임/파일/AI를 실행하지 않는다.</summary>
public sealed class ClientAiDebugGlobals
{
    /// <summary>보호 PE의 일곱 AI 로그 주소다.</summary>
    private static readonly uint[] Addresses = [0x52f3cc, 0x52f3d0, 0x52f3d4, 0x52f3d8, 0x52f3dc, 0x52f3e0, 0x52f3e4];
    /// <summary>같은 순서의 실제 PE 초기값이다. 미션 대여/반납으로 되감지 않는다.</summary>
    private readonly uint[] _words = [99, 99, 29, 99, 99, 99, 29];
    /// <summary>005e4794의 signed 현재 전체 DWORD이며 최초 BSS0이다.</summary>
    public IntBox Checking { get; } = new();
    /// <summary>005e4798의 현재 optional 문자열 소유자이며 최초 null이다.</summary>
    public StringBox AssertionDetail { get; } = new();
    /// <summary>원문 주소와 정확히 일치한 현재 물리만 쓰기 참조로 반환한다.</summary>
    public ref uint Word(uint address)
    {
        // 일곱 원문 주소를 순서대로 찾으며 임의 호스트 포인터는 역참조하지 않는다.
        for (int index = 0; index < Addresses.Length; index++) if (Addresses[index] == address) return ref _words[index];
        throw new ArgumentOutOfRangeException(nameof(address), "현재AI진단DWORD주소미연결");
    }
    /// <summary>읽기 조회도 같은 현재 저장 위치를 소비한다.</summary>
    public uint Read(uint address) => Word(address);
    /// <summary>ClientRendererFrame의 같은 showSoftAsserts를 읽고 누락 키는 현재값을 보존한다.</summary>
    public void ReadOptions(ConfigInterface configuration)
    {
        ArgumentNullException.ThrowIfNull(configuration); int value = Checking.Value; configuration.ReadInt("showSoftAsserts", ref value); Checking.Value = value;
    }
}
