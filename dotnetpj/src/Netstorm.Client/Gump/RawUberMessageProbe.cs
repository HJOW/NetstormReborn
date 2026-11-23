// 실제 게임 Root의 메시지 공장 팔레트 프로브 경로를 같은 등록표·파일·문자열 owner에 연결한다.
// cpppj/src/client/UberHud.inc 51~61행의 savedMessage/equalText/constructMessage/formatMessage/testMessage/visitMessage/messageEvent 를 이식한다.
// 0.905에서 RU-3 메시지 공장 묶음용으로 추가됐다.
using System.Text;
using Netstorm.Assets;

namespace Netstorm.Client.Gump;

// 메시지 공장 팔레트 프로브 모듈을 담는 네임스페이스다. 0.905에서 추가됐다.

// 실제 메시지 후단의 외부 장치 묶음이다. 0.905에서 추가됐다.
public sealed class UberMessageProbeHooks
{
    /// <summary>현재 HTML 배경80BYTE 이름이다. 0.905에서 추가됐다.</summary>
    public Func<string>? BackgroundName { get; init; }

    /// <summary>안정된 문자열 토큰의 현재 본문을 읽는다. 0.905에서 추가됐다.</summary>
    public Func<uint, string>? StoredText { get; init; }

    /// <summary>문자열을 같은 안정된 토큰으로 게시한다. 0.905에서 추가됐다.</summary>
    public Func<string, uint>? StoreText { get; init; }

    /// <summary>실제 파일 조회 결과만 조건으로 사용하며 없는 파일을 만들지 않는다. 0.905에서 추가됐다.</summary>
    public Func<string, byte[]?>? TryRead { get; init; }

    /// <summary>원문 진단 줄을 같은 실제 로그에 남긴다. 0.905에서 추가됐다.</summary>
    public Action<string>? LogProbe { get; init; }

    /// <summary>원문이 선택한 같은 메시지 포인터로 실제 전체 부모를 호출한다. 0.905에서 추가됐다.</summary>
    public Action<byte[]>? ChangePalette { get; init; }

    /// <summary>실제 배경 GIF 후단의 토큰 생성 장치다. 0.905에서 추가됐다.</summary>
    public Action<uint, string>? Construct { get; init; }

    /// <summary>실제 배경 GIF 후단의 사건 장치다. 0.905에서 추가됐다.</summary>
    public Action<uint, uint, uint, uint>? MessageEvent { get; init; }
}

// UberHud.inc의 고정 팔레트 경로 메시지 공장 본체다. 0.905에서 추가됐다.
public sealed class RawUberMessageProbe
{
    /// <summary>보호PE의 빈 root 메시지 주소다. 0.905에서 추가됐다.</summary>
    public const uint SavedMessage = 0x565e70u;

    /// <summary>원문 고정 팔레트 경로의 실제 9인자다. 0.905에서 추가됐다.</summary>
    private static readonly uint[] FixedExpression = [0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, SavedMessage, 0xfffffffeu, 0x51501cu, 0u];

    /// <summary>같은 실제 파일 이름 작성기다. 0.905에서 추가됐다.</summary>
    private readonly RawFileName _fileNames;

    /// <summary>선택된 실제 외부 장치 묶음이다. 0.905에서 추가됐다.</summary>
    private readonly UberMessageProbeHooks _hooks;

    /// <summary>같은 실제 파일 이름 작성기와 외부 장치를 빌린다. 값을 복사하거나 초기화하지 않는다. 0.905에서 추가됐다.</summary>
    /// <param name="fileNames">같은 실제 원문 파일 이름 작성기다.</param>
    /// <param name="hooks">선택된 실제 외부 장치 묶음이다.</param>
    public RawUberMessageProbe(RawFileName fileNames, UberMessageProbeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(fileNames);
        ArgumentNullException.ThrowIfNull(hooks);
        _fileNames = fileNames;
        _hooks = hooks;
    }

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.905에서 추가됐다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("메시지 공장의 실제 선택 장치 미연결");

    /// <summary>원문 대소문자를 구분하지 않는 ASCII 비교다. 0.905에서 추가됐다.</summary>
    internal static bool AsciiEqual(string first, string second)
    {
        // 원문 o::AsciiLower와 같이 영문 대문자만 내린다.
        static string Lower(string text)
        {
            // 각 문자를 순서대로 검사하며 소문자 변환 범위를 벗어나면 그대로 둔다.
            var output = new StringBuilder(text.Length);
            foreach (char c in text)
            {
                output.Append(c is >= 'A' and <= 'Z' ? (char)(c + 32) : c);
            }

            return output.ToString();
        }

        return string.Equals(Lower(first), Lower(second), StringComparison.Ordinal);
    }

    /// <summary>Uber 실제 메시지 주소 일치와 현재 배경 이름 일치를 함께 판정한다. 0.905에서 추가됐다.</summary>
    /// <param name="token">검사할 실제 메시지 주소다.</param>
    /// <param name="text">비교할 배경 이름이다.</param>
    public bool EqualText(uint token, string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        if (token != SavedMessage)
        {
            return false;
        }

        return AsciiEqual(Need(_hooks.BackgroundName)(), text);
    }

    /// <summary>같은 9인자를 전체 원문 작성기의 네 부품으로 전달한다. 0.905에서 추가됐다.</summary>
    /// <param name="expression">검사할 실제 원문 9인자다.</param>
    public uint FormatMessage(IReadOnlyList<uint> expression)
    {
        ArgumentNullException.ThrowIfNull(expression);
        // 보호PE의 빈 root/d/현재 이름/.col 순서를 그대로 요구한다.
        if (expression.Count != FixedExpression.Length)
        {
            throw new ArgumentException("메시지 공장의 실제 원문 9인자 개수 불일치", nameof(expression));
        }

        // 각 인자를 순서대로 비교하며 원문 인자 변경을 명시 거부한다.
        for (int index = 0; index < FixedExpression.Length; index++)
        {
            if (expression[index] != FixedExpression[index])
            {
                throw new ArgumentException("메시지 공장의 실제 원문 인자 변경", nameof(expression));
            }
        }

        string background = Need(_hooks.BackgroundName)();
        // 같은 9인자를 전체 원문 작성기의 네 부품으로 전달한다.
        var parts = new FileNamePart[]
        {
            new(-2, ReadOnlyMemory<byte>.Empty),
            new(-2, Encoding.Latin1.GetBytes("d")),
            new(-3, Encoding.Latin1.GetBytes(background)),
            new(-4, Encoding.Latin1.GetBytes("col")),
        };
        string path = Encoding.Latin1.GetString(_fileNames.Format(parts).Span);
        return Need(_hooks.StoreText)(path);
    }

    /// <summary>실제 VFS 조회 결과만 조건으로 사용하며 없는 파일을 만들지 않는다. 0.905에서 추가됐다.</summary>
    /// <param name="token">검사할 안정된 문자열 토큰이다.</param>
    public uint TestMessage(uint token)
    {
        string path = Need(_hooks.StoredText)(token);
        bool found = Need(_hooks.TryRead)(path) is not null;
        Need(_hooks.LogProbe)($"Uber palette probe {path} exists {found}");
        return found ? 1u : 0u;
    }

    /// <summary>원문이 선택한 같은 메시지 포인터로 실제 전체 부모를 호출한다. 0.905에서 추가됐다.</summary>
    /// <param name="token">읽을 안정된 문자열 토큰이다.</param>
    public void VisitMessage(uint token)
    {
        string text = Need(_hooks.StoredText)(token);
        // 원문처럼 끝 NUL을 포함한 바이트 포인터를 전달한다.
        Need(_hooks.ChangePalette)(Encoding.Latin1.GetBytes(text + '\0'));
    }

    /// <summary>Uber 실제 배경 이름 주소가 아니면 명시 거부한다. 0.905에서 추가됐다.</summary>
    /// <param name="token">생성할 토큰이다.</param>
    /// <param name="name">검사할 실제 배경 이름 주소다.</param>
    public void ConstructMessage(uint token, uint name)
    {
        if (name != SavedMessage)
        {
            throw new ArgumentException("메시지 공장의 실제 배경 이름 주소 불일치", nameof(name));
        }

        Need(_hooks.Construct)(token, Need(_hooks.BackgroundName)());
    }

    /// <summary>실제 배경 GIF 후단에 사건을 전달한다. 0.905에서 추가됐다.</summary>
    public void DispatchMessageEvent(uint token, uint @event, uint first, uint second) => Need(_hooks.MessageEvent)(token, @event, first, second);
}
